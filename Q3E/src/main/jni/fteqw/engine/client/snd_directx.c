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

#ifdef AVAIL_DSOUND

//#define DIRECTSOUND_VERSION 0x800 //either < 0x800 (eax-only) or 0x800 (microsoft's fx stuff).
#include <dsound.h>

#if !defined(IDirectSoundFXI3DL2Reverb_SetAllParameters) && DIRECTSOUND_VERSION>=0x800
	//mingw defines version as 0x900, but doesn't provide all the extra interfaces (only the core stuff).
	//which makes it kinda pointless, so lets provide the crap that its missing.
	typedef struct {
		LONG lRoom;
		LONG lRoomHF;
		FLOAT flRoomRolloffFactor;
		FLOAT flDecayTime;
		FLOAT flDecayHFRatio;
		LONG lReflections;
		FLOAT flReflectionsDelay;
		LONG lReverb;
		FLOAT flReverbDelay;
		FLOAT flDiffusion;
		FLOAT flDensity;
		FLOAT flHFReference;
	} DSFXI3DL2Reverb;

	typedef struct IDirectSoundFXI3DL2Reverb
	{
		struct
		{
			STDMETHOD(QueryInterface)(struct IDirectSoundFXI3DL2Reverb *this_, REFIID riid, LPVOID * ppvObj);
			STDMETHOD_(ULONG,AddRef)(struct IDirectSoundFXI3DL2Reverb *this_);
			STDMETHOD_(ULONG,Release)(struct IDirectSoundFXI3DL2Reverb *this_);
			STDMETHOD(SetAllParameters)(struct IDirectSoundFXI3DL2Reverb *this_, const DSFXI3DL2Reverb *pcDsFxI3DL2Reverb);
			//INCOMPLETE
		} *lpVtbl;
	} IDirectSoundFXI3DL2Reverb;
	#define IDirectSoundFXI3DL2Reverb8 IDirectSoundFXI3DL2Reverb
	#define IID_IDirectSoundFXI3DL2Reverb8 IID_IDirectSoundFXI3DL2Reverb

	#define IDirectSoundFXI3DL2Reverb_Release(a) (a)->lpVtbl->Release(a)
	#define IDirectSoundFXI3DL2Reverb_SetAllParameters(a,b) (a)->lpVtbl->SetAllParameters(a,b)
#endif

#if _MSC_VER <= 1200
	#define FORCE_DEFINE_GUID DEFINE_GUID
#else
	#ifndef DECLSPEC_SELECTANY
		#define DECLSPEC_SELECTANY
	#endif
	#define FORCE_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
			EXTERN_C const GUID DECLSPEC_SELECTANY name \
					= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif

#if DIRECTSOUND_VERSION >= 0x0800
FORCE_DEFINE_GUID(IID_IDirectSoundBuffer8, 0x6825a449, 0x7524, 0x4d82, 0x92, 0x0f, 0x50, 0xe3, 0x6a, 0xb3, 0xab, 0x1e);
FORCE_DEFINE_GUID(IID_IDirectSound8, 0xC50A7E93, 0xF395, 0x4834, 0x9E, 0xF6, 0x7F, 0xA9, 0x9D, 0xE5, 0x09, 0x66);
FORCE_DEFINE_GUID(IID_IDirectSoundFXI3DL2Reverb, 0x4b166a6a, 0x0d66, 0x43f3, 0x80, 0xe3, 0xee, 0x62, 0x80, 0xde, 0xe1, 0xa4);
FORCE_DEFINE_GUID(GUID_DSFX_STANDARD_I3DL2REVERB, 0xef985e71, 0xd5c7, 0x42d4, 0xba, 0x4d, 0x2d, 0x07, 0x3e, 0x2e, 0x96, 0xf4);
HRESULT (WINAPI *pDirectSoundCreate8)(GUID FAR *lpGUID, LPDIRECTSOUND8 FAR *lplpDS, IUnknown FAR *pUnkOuter);
#endif

FORCE_DEFINE_GUID(IID_IDirectSound, 0x279AFA83, 0x4981, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
FORCE_DEFINE_GUID(IID_IKsPropertySet, 0x31efac30, 0x515c, 0x11d0, 0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93);
HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);
HRESULT (WINAPI *pDirectSoundEnumerate)(LPDSENUMCALLBACKA lpCallback, LPVOID lpContext);
#if defined(VOICECHAT)
HRESULT (WINAPI *pDirectSoundCaptureCreate)(GUID FAR *lpGUID, LPDIRECTSOUNDCAPTURE FAR *lplpDS, IUnknown FAR *pUnkOuter);
HRESULT (WINAPI *pDirectSoundCaptureEnumerate)(LPDSENUMCALLBACK lpDSEnumCallback, LPVOID lpContext);
#endif

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

typedef struct {
	LPDIRECTSOUND pDS;
	LPDIRECTSOUNDBUFFER pDSBuf;
	LPDIRECTSOUNDBUFFER pDSPBuf;

#if DIRECTSOUND_VERSION >= 0x0800
	//dsound8 interfaces, for reverb effects
	LPDIRECTSOUND8 pDS8;
	LPDIRECTSOUNDBUFFER8 pDSBuf8;
	IDirectSoundFXI3DL2Reverb8 *pReverb;
#endif

	DWORD gSndBufSize;
	DWORD		mmstarttime;

	int curreverb;
	int curreverbmodcount;	//so it updates if the effect itself is updated

#ifdef _IKsPropertySet_
	LPKSPROPERTYSET	EaxKsPropertiesSet;
#endif
} dshandle_t;

HINSTANCE hInstDS;

static void DSOUND_Restore(soundcardinfo_t *sc)
{
	DWORD	dwStatus;
	dshandle_t *dh = sc->handle;
	if (IDirectSoundBuffer_GetStatus (dh->pDSBuf, &dwStatus) != ERROR_SUCCESS)
		Con_Printf ("Couldn't get sound buffer status\n");

	if (dwStatus & DSBSTATUS_BUFFERLOST)
		IDirectSoundBuffer_Restore (dh->pDSBuf);

	if (!(dwStatus & DSBSTATUS_PLAYING))
		IDirectSoundBuffer_Play(dh->pDSBuf, 0, 0, DSBPLAY_LOOPING);
}

static DWORD	dsound_locksize;
static void *DSOUND_Lock(soundcardinfo_t *sc, unsigned int *sampidx)
{
	void *ret;
	int reps;
	DWORD	dwSize2=0;
	DWORD	*pbuf2;
	HRESULT	hresult;

	dshandle_t *dh = sc->handle;
	dsound_locksize=0;

	reps = 0;

	while ((hresult = IDirectSoundBuffer_Lock(dh->pDSBuf, 0, dh->gSndBufSize, (void**)&ret, &dsound_locksize,
								   (void**)&pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Con_Printf ("S_TransferStereo16: DS::Lock Sound Buffer Failed\n");
			return NULL;
		}

		if (++reps > 10000)
		{
			Con_Printf ("S_TransferStereo16: DS: couldn't restore buffer\n");
			return NULL;
		}

		DSOUND_Restore(sc);
	}

	return ret;
}

//called when the mixer is done with it.
static void DSOUND_Unlock(soundcardinfo_t *sc, void *buffer)
{
	dshandle_t *dh = sc->handle;
	IDirectSoundBuffer_Unlock(dh->pDSBuf, buffer, dsound_locksize, NULL, 0);
}

/*
==================
FreeSound
==================
*/
//per device
static void DSOUND_Shutdown_Internal (soundcardinfo_t *sc)
{
	dshandle_t *dh = sc->handle;
	if (!dh)
		return;

	sc->handle = NULL;
#ifdef _IKsPropertySet_
	if (dh->EaxKsPropertiesSet)
		IKsPropertySet_Release(dh->EaxKsPropertiesSet);
	dh->EaxKsPropertiesSet = NULL;
#endif

#if DIRECTSOUND_VERSION >= 0x0800
	if (dh->pReverb)
		IDirectSoundFXI3DL2Reverb_Release(dh->pReverb);
	dh->pReverb = NULL;
	if (dh->pDSBuf8)
		IDirectSoundBuffer8_Release(dh->pDSBuf8);
	dh->pDSBuf8 = NULL;
#endif

	if (dh->pDSBuf)
	{
		IDirectSoundBuffer_Stop(dh->pDSBuf);
		IDirectSoundBuffer_Release(dh->pDSBuf);
		if (dh->pDSBuf == dh->pDSPBuf)
			dh->pDSPBuf = NULL;
		dh->pDSBuf = NULL;
	}

	if (dh->pDSPBuf)
		IDirectSoundBuffer_Release(dh->pDSPBuf);
	dh->pDSPBuf = NULL;

	if (dh->pDS)
	{
		IDirectSound_SetCooperativeLevel (dh->pDS, mainwindow, DSSCL_NORMAL);
		IDirectSound_Release(dh->pDS);
	}

	dh->pDS = NULL;

	Z_Free(dh);
}

static void DSOUND_Shutdown (soundcardinfo_t *sc)
{
#ifdef MULTITHREAD
	if  (sc->thread)
	{
		//thread does the actual closing.
		sc->selfpainting = false;
		Sys_WaitOnThread(sc->thread);
		sc->thread = NULL;
	}
	else
#endif
		DSOUND_Shutdown_Internal(sc);
}

/*
	Direct Sound.
	These following defs should be moved to winquake.h somewhere.

	We tell DS to use a different wave format. We do this to gain extra channels. >2
	We still use the old stuff too, when we can for compatability.

	EAX 2 is also supported.
	This is a global state. Once applied, it's applied for other programs too.
	We have to do a few special things to try to ensure support in all it's different versions.
*/

/* new formatTag:*/
#ifndef WAVE_FORMAT_EXTENSIBLE
# define WAVE_FORMAT_EXTENSIBLE (0xfffe)
#endif

/* Speaker Positions:*/
# define SPEAKER_FRONT_LEFT              0x1
# define SPEAKER_FRONT_RIGHT             0x2
# define SPEAKER_FRONT_CENTER            0x4
# define SPEAKER_LOW_FREQUENCY           0x8
# define SPEAKER_BACK_LEFT               0x10
# define SPEAKER_BACK_RIGHT              0x20
# define SPEAKER_FRONT_LEFT_OF_CENTER    0x40
# define SPEAKER_FRONT_RIGHT_OF_CENTER   0x80
# define SPEAKER_BACK_CENTER             0x100
# define SPEAKER_SIDE_LEFT               0x200
# define SPEAKER_SIDE_RIGHT              0x400
# define SPEAKER_TOP_CENTER              0x800
# define SPEAKER_TOP_FRONT_LEFT          0x1000
# define SPEAKER_TOP_FRONT_CENTER        0x2000
# define SPEAKER_TOP_FRONT_RIGHT         0x4000
# define SPEAKER_TOP_BACK_LEFT           0x8000
# define SPEAKER_TOP_BACK_CENTER         0x10000
# define SPEAKER_TOP_BACK_RIGHT          0x20000

/* Bit mask locations reserved for future use*/
#ifndef SPEAKER_RESERVED
# define SPEAKER_RESERVED                0x7FFC0000
#endif

/* Used to specify that any possible permutation of speaker configurations*/
# define SPEAKER_ALL                     0x80000000

/* DirectSound Speaker Config*/
# define KSAUDIO_SPEAKER_MONO            (SPEAKER_FRONT_CENTER)
# define KSAUDIO_SPEAKER_STEREO          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
# define KSAUDIO_SPEAKER_QUAD            (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                         SPEAKER_BACK_LEFT  | SPEAKER_BACK_RIGHT)
# define KSAUDIO_SPEAKER_SURROUND        (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                         SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER)
# define KSAUDIO_SPEAKER_5POINT1         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                         SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                         SPEAKER_BACK_LEFT  | SPEAKER_BACK_RIGHT)
# define KSAUDIO_SPEAKER_7POINT1         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                         SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                         SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | \
                                         SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER)

typedef struct {
	WAVEFORMATEX    Format;
	union {
		WORD wValidBitsPerSample;       /* bits of precision  */
		WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
		WORD wReserved;                 /* If neither applies, set to */
										/* zero. */
	} Samples;
	DWORD           dwChannelMask;      /* which channels are */
										/* present in stream  */
	GUID            SubFormat;
} QWAVEFORMATEX;

static const GUID  QKSDATAFORMAT_SUBTYPE_PCM		= {0x00000001,0x0000,0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID QKSDATAFORMAT_SUBTYPE_IEEE_FLOAT	= {0x00000003,0x0000,0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

#ifdef _IKsPropertySet_
//const static GUID  CLSID_EAXDIRECTSOUND = {0x4ff53b81, 0x1ce0, 0x11d3,
//{0xaa, 0xb8, 0x0, 0xa0, 0xc9, 0x59, 0x49, 0xd5}};
static const GUID  DSPROPSETID_EAX20_LISTENERPROPERTIES = {0x306a6a8, 0xb224, 0x11d2,
{0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22}};

typedef struct _EAXLISTENERPROPERTIES
{
    long lRoom;                    // room effect level at low frequencies
    long lRoomHF;                  // room effect high-frequency level re. low frequency level
    float flRoomRolloffFactor;     // like DS3D flRolloffFactor but for room effect
    float flDecayTime;             // reverberation decay time at low frequencies
    float flDecayHFRatio;          // high-frequency to low-frequency decay time ratio
    long lReflections;             // early reflections level relative to room effect
    float flReflectionsDelay;      // initial reflection delay time
    long lReverb;                  // late reverberation level relative to room effect
    float flReverbDelay;           // late reverberation delay time relative to initial reflection
    unsigned long dwEnvironment;   // sets all listener properties
    float flEnvironmentSize;       // environment size in meters
    float flEnvironmentDiffusion;  // environment diffusion
    float flAirAbsorptionHF;       // change in level per meter at 5 kHz
    unsigned long dwFlags;         // modifies the behavior of properties
} EAXLISTENERPROPERTIES, *LPEAXLISTENERPROPERTIES;
enum
{
    EAX_ENVIRONMENT_GENERIC,
    EAX_ENVIRONMENT_PADDEDCELL,
    EAX_ENVIRONMENT_ROOM,
    EAX_ENVIRONMENT_BATHROOM,
    EAX_ENVIRONMENT_LIVINGROOM,
    EAX_ENVIRONMENT_STONEROOM,
    EAX_ENVIRONMENT_AUDITORIUM,
    EAX_ENVIRONMENT_CONCERTHALL,
    EAX_ENVIRONMENT_CAVE,
    EAX_ENVIRONMENT_ARENA,
    EAX_ENVIRONMENT_HANGAR,
    EAX_ENVIRONMENT_CARPETEDHALLWAY,
    EAX_ENVIRONMENT_HALLWAY,
    EAX_ENVIRONMENT_STONECORRIDOR,
    EAX_ENVIRONMENT_ALLEY,
    EAX_ENVIRONMENT_FOREST,
    EAX_ENVIRONMENT_CITY,
    EAX_ENVIRONMENT_MOUNTAINS,
    EAX_ENVIRONMENT_QUARRY,
    EAX_ENVIRONMENT_PLAIN,
    EAX_ENVIRONMENT_PARKINGLOT,
    EAX_ENVIRONMENT_SEWERPIPE,
    EAX_ENVIRONMENT_UNDERWATER,
    EAX_ENVIRONMENT_DRUGGED,
    EAX_ENVIRONMENT_DIZZY,
    EAX_ENVIRONMENT_PSYCHOTIC,

    EAX_ENVIRONMENT_COUNT
};
typedef enum
{
    DSPROPERTY_EAXLISTENER_NONE,
    DSPROPERTY_EAXLISTENER_ALLPARAMETERS,
    DSPROPERTY_EAXLISTENER_ROOM,
    DSPROPERTY_EAXLISTENER_ROOMHF,
    DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAXLISTENER_DECAYTIME,
    DSPROPERTY_EAXLISTENER_DECAYHFRATIO,
    DSPROPERTY_EAXLISTENER_REFLECTIONS,
    DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
    DSPROPERTY_EAXLISTENER_REVERB,
    DSPROPERTY_EAXLISTENER_REVERBDELAY,
    DSPROPERTY_EAXLISTENER_ENVIRONMENT,
    DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
    DSPROPERTY_EAXLISTENER_FLAGS
} DSPROPERTY_EAX_LISTENERPROPERTY;

/*const static GUID DSPROPSETID_EAX20_BUFFERPROPERTIES ={
    0x306a6a7,
    0xb224,
    0x11d2,
    {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22}};*/

static const GUID CLSID_EAXDirectSound ={
		0x4ff53b81,
		0x1ce0,
		0x11d3,
		{0xaa, 0xb8, 0x0, 0xa0, 0xc9, 0x59, 0x49, 0xd5}};

typedef struct _EAXBUFFERPROPERTIES
{
    long lDirect;                // direct path level
    long lDirectHF;              // direct path level at high frequencies
    long lRoom;                  // room effect level
    long lRoomHF;                // room effect level at high frequencies
    float flRoomRolloffFactor;   // like DS3D flRolloffFactor but for room effect
    long lObstruction;           // main obstruction control (attenuation at high frequencies)
    float flObstructionLFRatio;  // obstruction low-frequency level re. main control
    long lOcclusion;             // main occlusion control (attenuation at high frequencies)
    float flOcclusionLFRatio;    // occlusion low-frequency level re. main control
    float flOcclusionRoomRatio;  // occlusion room effect level re. main control
    long lOutsideVolumeHF;       // outside sound cone level at high frequencies
    float flAirAbsorptionFactor; // multiplies DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF
    unsigned long dwFlags;       // modifies the behavior of properties
} EAXBUFFERPROPERTIES, *LPEAXBUFFERPROPERTIES;

typedef enum
{
    DSPROPERTY_EAXBUFFER_NONE,
    DSPROPERTY_EAXBUFFER_ALLPARAMETERS,
    DSPROPERTY_EAXBUFFER_DIRECT,
    DSPROPERTY_EAXBUFFER_DIRECTHF,
    DSPROPERTY_EAXBUFFER_ROOM,
    DSPROPERTY_EAXBUFFER_ROOMHF,
    DSPROPERTY_EAXBUFFER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAXBUFFER_OBSTRUCTION,
    DSPROPERTY_EAXBUFFER_OBSTRUCTIONLFRATIO,
    DSPROPERTY_EAXBUFFER_OCCLUSION,
    DSPROPERTY_EAXBUFFER_OCCLUSIONLFRATIO,
    DSPROPERTY_EAXBUFFER_OCCLUSIONROOMRATIO,
    DSPROPERTY_EAXBUFFER_OUTSIDEVOLUMEHF,
    DSPROPERTY_EAXBUFFER_AIRABSORPTIONFACTOR,
    DSPROPERTY_EAXBUFFER_FLAGS
} DSPROPERTY_EAX_BUFFERPROPERTY;
#endif

static long GainToMillibels(float gain)
{
	return 100*20*(0.43429*log(gain));
}
static void DSOUND_SetReverb(soundcardinfo_t *sc, size_t reverbidx)
{
	dshandle_t *dh = sc->handle;
	struct reverbproperties_s *prop;
	if (reverbidx >= numreverbproperties)
		return;	//invalid
	if (dh->curreverb == reverbidx && dh->curreverbmodcount == reverbproperties[reverbidx].modificationcount)
		return;	//nothing changed.
	dh->curreverb = reverbidx;
	dh->curreverbmodcount = reverbproperties[reverbidx].modificationcount;

	prop = &reverbproperties[dh->curreverb].props;

#ifdef _IKsPropertySet_
	//attempt at eax support.
	//EAX is a global thing. Get it going in a game and your media player will be doing it too.

	if (dh->EaxKsPropertiesSet)	//only on ds cards.
	{
		EAXLISTENERPROPERTIES ListenerProperties =  {0};

		ListenerProperties.flEnvironmentSize = prop->flEchoTime;
		ListenerProperties.flEnvironmentDiffusion = prop->flDiffusion;
		ListenerProperties.lRoom = GainToMillibels(prop->flGain);
		ListenerProperties.lRoomHF = GainToMillibels(prop->flGainHF);
		ListenerProperties.flRoomRolloffFactor = prop->flRoomRolloffFactor;
		ListenerProperties.flAirAbsorptionHF = prop->flAirAbsorptionGainHF;
		ListenerProperties.lReflections = GainToMillibels(prop->flReflectionsGain);
		ListenerProperties.flReflectionsDelay  = prop->flReflectionsDelay;
		ListenerProperties.lReverb = GainToMillibels(prop->flLateReverbGain);
		ListenerProperties.flReverbDelay = prop->flLateReverbDelay;
		ListenerProperties.flDecayTime = prop->flDecayTime;
		ListenerProperties.flDecayHFRatio = prop->flDecayHFRatio;
		ListenerProperties.dwFlags = 0x3f;
		ListenerProperties.dwEnvironment = reverbidx?EAX_ENVIRONMENT_UNDERWATER:0;

		if (FAILED(IKsPropertySet_Set(dh->EaxKsPropertiesSet, &DSPROPSETID_EAX20_LISTENERPROPERTIES,
					DSPROPERTY_EAXLISTENER_ALLPARAMETERS, 0, 0, &ListenerProperties,
					sizeof(ListenerProperties))))
			Con_SafePrintf ("EAX set failed\n");
	}
#endif

#if DIRECTSOUND_VERSION >= 0x0800
	if (dh->pReverb)
	{
		DSFXI3DL2Reverb reverb;
		reverb.lRoom				= bound(-10000, GainToMillibels(prop->flGain), 0);			// [-10000, 0]      default: -1000 mB
		reverb.lRoomHF				= bound(-10000, GainToMillibels(prop->flGainHF), 0);			// [-10000, 0]      default: 0 mB
		reverb.flRoomRolloffFactor	= bound(0.0, prop->flRoomRolloffFactor, 10.0);				// [0.0, 10.0]      default: 0.0
		reverb.flDecayTime			= bound(0.1, prop->flDecayTime, 20.0);						// [0.1, 20.0]      default: 1.49s
		reverb.flDecayHFRatio		= bound(0.1, prop->flDecayHFRatio, 2.0);						// [0.1, 2.0]       default: 0.83
		reverb.lReflections			= bound(-10000, GainToMillibels(prop->flReflectionsGain), 1000);	// [-10000, 1000]   default: -2602 mB
		reverb.flReflectionsDelay	= bound(0.0, prop->flReflectionsDelay, 0.3);					// [0.0, 0.3]       default: 0.007 s
		reverb.lReverb				= bound(-10000, GainToMillibels(prop->flLateReverbGain), 2000);	// [-10000, 2000]   default: 200 mB
		reverb.flReverbDelay		= bound(0.0, prop->flLateReverbDelay, 0.1);					// [0.0, 0.1]       default: 0.011 s
		reverb.flDiffusion			= bound(0.0, prop->flDiffusion*100, 100.0);					// [0.0, 100.0]     default: 100.0 %
		reverb.flDensity			= bound(0.0, prop->flDensity*100, 100.0);						// [0.0, 100.0]     default: 100.0 %
		reverb.flHFReference		= bound(20.0, prop->flHFReference, 20000.0);						// [20.0, 20000.0]  default: 5000.0 Hz

		IDirectSoundFXI3DL2Reverb_SetAllParameters(dh->pReverb, &reverb);
	}
#endif
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
static unsigned int DSOUND_GetDMAPos(soundcardinfo_t *sc)
{
	DWORD	mmtime;
	int		s;
	DWORD	dwWrite;

	dshandle_t *dh = sc->handle;

	IDirectSoundBuffer_GetCurrentPosition(dh->pDSBuf, &mmtime, &dwWrite);
	s = mmtime - dh->mmstarttime;

	s /= sc->sn.samplebytes;
	s %= (sc->sn.samples);
	return s;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
static void DSOUND_Submit(soundcardinfo_t *sc, int start, int end)
{
}

static qboolean DSOUND_InitOutputLibrary(void)
{
	if (!hInstDS)
		hInstDS = LoadLibrary("dsound.dll");
	if (!hInstDS)
	{
		Con_SafePrintf ("Couldn't load dsound.dll\n");
		return false;
	}
	if (!pDirectSoundCreate)
		pDirectSoundCreate = (void *)GetProcAddress(hInstDS,"DirectSoundCreate");
#if DIRECTSOUND_VERSION >= 0x0800
	if (!pDirectSoundCreate8)
		pDirectSoundCreate8 = (void *)GetProcAddress(hInstDS,"DirectSoundCreate8");	//xp+
	if (!pDirectSoundCreate8)
#endif
	{
		if (!pDirectSoundCreate)
		{
			Con_SafePrintf ("Couldn't get DS proc addr\n");
			return false;
		}
	}
	if (!pDirectSoundEnumerate)
		pDirectSoundEnumerate = (void *)GetProcAddress(hInstDS,"DirectSoundEnumerateA");
	return true;
}
/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
static int DSOUND_InitCard_Internal (soundcardinfo_t *sc, char *cardname)
{
	extern cvar_t snd_inactive;
	extern cvar_t snd_eax;
	int usereverb;	//2=eax, 1=ds8
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	DWORD			dwSize, dwWrite;
	DSCAPS			dscaps;
	QWAVEFORMATEX	format, pformat;
	HRESULT			hresult;
	int				reps;
	qboolean		primary_format_set;
	dshandle_t *dh;
	char *buffer;
	GUID guid, *dsguid;


	memset (&format, 0, sizeof(format));

	if (*sc->name)
	{
		wchar_t mssuck[128];
		mbstowcs(mssuck, sc->name, sizeof(mssuck)/sizeof(mssuck[0])-1);
		CLSIDFromString(mssuck, &guid);
		dsguid = &guid;
	}
	else
	{
		memset(&guid, 0, sizeof(GUID));
		dsguid = NULL;
	}

	if (sc->sn.numchannels >= 8) // 7.1 surround
	{
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.cbSize = 22;
		memcpy(&format.SubFormat, &QKSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID));

		format.dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
		sc->sn.numchannels = 8;
	}
	else if (sc->sn.numchannels >= 6)	//5.1 surround
	{
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.cbSize = 22;
		memcpy(&format.SubFormat, &QKSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID));

		format.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
		sc->sn.numchannels = 6;
	}
	else if (sc->sn.numchannels >= 4)	//4 speaker quad
	{
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.cbSize = 22;
		memcpy(&format.SubFormat, &QKSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID));

		format.dwChannelMask = KSAUDIO_SPEAKER_QUAD;
		sc->sn.numchannels = 4;
	}
	else if (sc->sn.numchannels >= 2)	//stereo
	{
		format.Format.wFormatTag = WAVE_FORMAT_PCM;
		format.Format.cbSize = 0;
		sc->sn.numchannels = 2;
	}
	else //mono time
	{
		format.Format.wFormatTag = WAVE_FORMAT_PCM;
		format.Format.cbSize = 0;
		sc->sn.numchannels = 1;
	}

	switch(sc->sn.samplebytes)
	{
	case 4:
		//FTE does not support 32bit int audio, rather we interpret samplebits 32 as floats.
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		format.Format.cbSize = 22;
		memcpy(&format.SubFormat, &QKSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(GUID));
		sc->sn.sampleformat = QSF_F32;
		break;
	case 2:
		sc->sn.sampleformat = QSF_S16;
		break;
	case 1:
		sc->sn.sampleformat = QSF_U8;
		break;
	}

	format.Format.nChannels = sc->sn.numchannels;
	format.Format.wBitsPerSample = sc->sn.samplebytes*8;
	format.Format.nSamplesPerSec = sc->sn.speed;
	format.Format.nBlockAlign = format.Format.nChannels * format.Format.wBitsPerSample / 8;
	format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nBlockAlign;

	if (!DSOUND_InitOutputLibrary())
		return false;

	sc->handle = Z_Malloc(sizeof(dshandle_t));
	dh = sc->handle;

	usereverb = !!snd_eax.ival;
 //EAX attempt
#if 1//_MSC_VER > 1200
#ifdef _IKsPropertySet_
	dh->pDS = NULL;
	if (usereverb)
	{
		CoInitialize(NULL);
		if (FAILED(CoCreateInstance( &CLSID_EAXDirectSound, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectSound, (void **)&dh->pDS )))
			dh->pDS=NULL;
		else
		{
			IDirectSound_Initialize(dh->pDS, dsguid);
			usereverb = 2;
		}
	}

	if (!dh->pDS)
#endif
#endif
	{
		for(;;)
		{
			dh->pDS = NULL;
#if DIRECTSOUND_VERSION >= 0x0800
			dh->pDS8 = NULL;
			if (pDirectSoundCreate8)
			{
				hresult = pDirectSoundCreate8(dsguid, &dh->pDS8, NULL);
				dh->pDS = (void*)dh->pDS8;	//evil cast
			}
			else
#endif
				hresult = pDirectSoundCreate(dsguid, &dh->pDS, NULL);
			if (hresult == DS_OK)
				break;

			if (hresult != DSERR_ALLOCATED)
			{
				Con_SafePrintf (": create failed\n");
				return false;
			}

//			if (MessageBox (NULL,
//							"The sound hardware is in use by another app.\n\n"
//							"Select Retry to try to start sound again or Cancel to run Quake with no sound.",
//							"Sound not available",
//							MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
//			{
				Con_SafePrintf (": failure\n"
								"  hardware already in use\n"
								"  Close the other app then use snd_restart\n");
				return false;
//			}
		}
	}

#ifdef FTE_SDL
#define mainwindow GetDesktopWindow()
#endif
	if (DS_OK != IDirectSound_SetCooperativeLevel (dh->pDS, mainwindow, DSSCL_EXCLUSIVE))
	{
		Con_SafePrintf ("Set coop level failed\n");
		DSOUND_Shutdown_Internal (sc);
		return false;
	}

	dscaps.dwSize = sizeof(dscaps);

	if (DS_OK != IDirectSound_GetCaps (dh->pDS, &dscaps))
	{
		Con_SafePrintf ("Couldn't get DS caps\n");
	}

	if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
	{
		Con_SafePrintf ("No DirectSound driver installed\n");
		DSOUND_Shutdown_Internal (sc);
		return false;
	}


// get access to the primary buffer, if possible, so we can set the
// sound hardware format
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

#ifdef DSBCAPS_GLOBALFOCUS
#ifndef FTE_SDL
	if (snd_inactive.ival || sys_parentwindow
		) /*always inactive if we have a parent window, because we can't tell properly otherwise*/
#endif
	{
		dsbuf.dwFlags |= DSBCAPS_GLOBALFOCUS;
		sc->inactive_sound = true;
	}
#endif

	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	primary_format_set = false;

	if (!COM_CheckParm ("-snoforceformat"))
	{
		if (DS_OK == IDirectSound_CreateSoundBuffer(dh->pDS, &dsbuf, &dh->pDSPBuf, NULL))
		{
			pformat = format;

			if (DS_OK != IDirectSoundBuffer_SetFormat (dh->pDSPBuf, (WAVEFORMATEX *)&pformat))
			{
//				if (snd_firsttime)
//					Con_SafePrintf ("Set primary sound buffer format: no\n");
			}
			else
//			{
//				if (snd_firsttime)
//					Con_SafePrintf ("Set primary sound buffer format: yes\n");

				primary_format_set = true;
//			}
		}
	}

	if (!primary_format_set || !COM_CheckParm ("-primarysound"))
	{
	// create the secondary buffer we'll actually work with
		memset (&dsbuf, 0, sizeof(dsbuf));
		dsbuf.dwSize = sizeof(DSBUFFERDESC);
		dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY|DSBCAPS_LOCSOFTWARE;	//dmw 29 may, 2003 removed locsoftware

#if DIRECTSOUND_VERSION >= 0x0800
		if (usereverb == 1)
			dsbuf.dwFlags |= DSBCAPS_CTRLFX;
#endif

#ifdef DSBCAPS_GLOBALFOCUS
		if (snd_inactive.ival)
		{
			dsbuf.dwFlags |= DSBCAPS_GLOBALFOCUS;
			sc->inactive_sound = true;
		}
#endif
		dsbuf.dwBufferBytes = sc->sn.samples / format.Format.nChannels;
		if (!dsbuf.dwBufferBytes)
		{
			dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
			// the fast rates will need a much bigger buffer
			if (format.Format.nSamplesPerSec > 48000)
				dsbuf.dwBufferBytes *= 4;
		}
		dsbuf.lpwfxFormat = (WAVEFORMATEX *)&format;

		memset(&dsbcaps, 0, sizeof(dsbcaps));
		dsbcaps.dwSize = sizeof(dsbcaps);

		if (DS_OK != IDirectSound_CreateSoundBuffer(dh->pDS, &dsbuf, &dh->pDSBuf, NULL))
		{
			Con_SafePrintf ("DS:CreateSoundBuffer Failed");
			DSOUND_Shutdown_Internal (sc);
			return false;
		}

		sc->sn.numchannels = format.Format.nChannels;
		sc->sn.samplebytes = format.Format.wBitsPerSample/8;
		sc->sn.speed = format.Format.nSamplesPerSec;

		if (DS_OK != IDirectSoundBuffer_GetCaps (dh->pDSBuf, &dsbcaps))
		{
			Con_SafePrintf ("DS:GetCaps failed\n");
			DSOUND_Shutdown_Internal (sc);
			return false;
		}

//		if (snd_firsttime)
//			Con_SafePrintf ("Using secondary sound buffer\n");
	}
	else
	{
		if (DS_OK != IDirectSound_SetCooperativeLevel (dh->pDS, mainwindow, DSSCL_WRITEPRIMARY))
		{
			Con_SafePrintf ("Set coop level failed\n");
			DSOUND_Shutdown_Internal (sc);
			return false;
		}

		if (DS_OK != IDirectSoundBuffer_GetCaps (dh->pDSPBuf, &dsbcaps))
		{
			Con_Printf ("DS:GetCaps failed\n");
			DSOUND_Shutdown_Internal (sc);
			return false;
		}

		dh->pDSBuf = dh->pDSPBuf;
//		Con_SafePrintf ("Using primary sound buffer\n");
	}

	dh->gSndBufSize = dsbcaps.dwBufferBytes;

#if DIRECTSOUND_VERSION >= 0x0800
	if (usereverb == 1)
	{
		if (SUCCEEDED(IDirectSoundBuffer_QueryInterface(dh->pDSBuf, &IID_IDirectSoundBuffer8, (void*)&dh->pDSBuf8)))
		{
			DSEFFECTDESC effects[1];
			DWORD results[1];
			memset(effects, 0, sizeof(effects));
			effects[0].dwSize = sizeof(effects[0]);
			effects[0].dwFlags = 0;
			effects[0].guidDSFXClass = GUID_DSFX_STANDARD_I3DL2REVERB;
			if (SUCCEEDED(IDirectSoundBuffer8_SetFX(dh->pDSBuf8, 1, effects, results)))
				if (SUCCEEDED(IDirectSoundBuffer8_GetObjectInPath(dh->pDSBuf8, &GUID_DSFX_STANDARD_I3DL2REVERB, 0, &IID_IDirectSoundFXI3DL2Reverb8, (void*)&dh->pReverb)))
					usereverb = 0;
		}
	}
#endif

#if 1
	// Make sure mixer is active
	IDirectSoundBuffer_Play(dh->pDSBuf, 0, 0, DSBPLAY_LOOPING);

	Con_DPrintf("   %d channel(s)\n"
				"   %d bits/sample\n"
				"   %d bytes/sec\n",
				sc->sn.numchannels, sc->sn.samplebytes*8, sc->sn.speed);


// initialize the buffer
	reps = 0;

	while ((hresult = IDirectSoundBuffer_Lock(dh->pDSBuf, 0, dh->gSndBufSize, (void**)&buffer, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Con_SafePrintf ("SNDDMA_InitDirect: DS::Lock Sound Buffer Failed\n");
			DSOUND_Shutdown_Internal (sc);
			return false;
		}

		if (++reps > 10000)
		{
			Con_SafePrintf ("SNDDMA_InitDirect: DS: couldn't restore buffer\n");
			DSOUND_Shutdown_Internal (sc);
			return false;
		}
	}

	memset(buffer, 0, dwSize);
//		lpData[4] = lpData[5] = 0x7f;	// force a pop for debugging

//	Sleep(500);

	IDirectSoundBuffer_Unlock(dh->pDSBuf, buffer, dwSize, NULL, 0);


	IDirectSoundBuffer_Stop(dh->pDSBuf);
#endif
	IDirectSoundBuffer_GetCurrentPosition(dh->pDSBuf, &dh->mmstarttime, &dwWrite);
	IDirectSoundBuffer_Play(dh->pDSBuf, 0, 0, DSBPLAY_LOOPING);

	sc->sn.samples = dh->gSndBufSize/sc->sn.samplebytes;
	sc->sn.samplepos = 0;
	sc->sn.buffer = NULL;

	dh->curreverb = ~0;
	dh->curreverbmodcount = ~0;


	sc->Lock		= DSOUND_Lock;
	sc->Unlock		= DSOUND_Unlock;
	sc->SetEnvironmentReverb	= DSOUND_SetReverb;
	sc->Submit		= DSOUND_Submit;
	sc->Shutdown	= DSOUND_Shutdown;
	sc->GetDMAPos	= DSOUND_GetDMAPos;
	sc->Restore		= DSOUND_Restore;

#if 1//_MSC_VER > 1200
#ifdef _IKsPropertySet_
	//attempt at eax support
	if (usereverb == 2)
	{
		int r;
		DWORD support;

		if (SUCCEEDED(IDirectSoundBuffer_QueryInterface(dh->pDSBuf, &IID_IKsPropertySet, (void*)&dh->EaxKsPropertiesSet)))
		{
			r = IKsPropertySet_QuerySupport(dh->EaxKsPropertiesSet, &DSPROPSETID_EAX20_LISTENERPROPERTIES, DSPROPERTY_EAXLISTENER_ALLPARAMETERS, &support);
			if(!SUCCEEDED(r) || (support&(KSPROPERTY_SUPPORT_GET|KSPROPERTY_SUPPORT_SET))
					!= (KSPROPERTY_SUPPORT_GET|KSPROPERTY_SUPPORT_SET))
			{
				IKsPropertySet_Release(dh->EaxKsPropertiesSet);
				dh->EaxKsPropertiesSet = NULL;
				Con_SafePrintf ("EAX 2 not supported\n");
				//otherwise successful. It can be used for normal sound anyway.
			}
			else
				usereverb = 0; //worked. EAX is fully inited.
		}
		else
		{
			Con_DPrintf ("Couldn't get extended properties\n");
			dh->EaxKsPropertiesSet = NULL;
		}
	}
#endif
#endif

	if (usereverb)
		Con_SafePrintf ("Couldn't enable environmental reverb effects\n");

	return true;
}




#ifdef MULTITHREAD
static int DSOUND_Thread(void *arg)
{
	soundcardinfo_t *sc = arg;
	void *cond = sc->handle;
	sc->handle = NULL;

	//once creating the thread, the main thread will wait for us to signal that we have inited the dsound device.
	if (!DSOUND_InitCard_Internal(sc, sc->name))
		sc->selfpainting = false;

	//wake up the main thread.
	Sys_ConditionSignal(cond);

	while(sc->selfpainting)
	{
		S_MixerThread(sc);
		/* Quote:
		On NT (Win2K and XP) the cursors in SW buffers (and HW buffers on some devices) move in 10ms increments, so calling GetCurrentPosition() every 10ms is ideal.
		Calling it more often than every 5ms will cause some perf degradation.
		*/
		Sleep(9);
	}

	//we created the device, we need to kill it.
	DSOUND_Shutdown_Internal(sc);
	return 0;
}
#endif

static qboolean QDECL DSOUND_InitCard (soundcardinfo_t *sc, const char *device)
{
	if (COM_CheckParm("-wavonly"))
		return false;

	Q_strncpyz(sc->name, device?device:"", sizeof(sc->name));

#ifdef MULTITHREAD
	if (snd_mixerthread.ival)
	{
		void *cond;
		sc->selfpainting = true;
		sc->handle = cond = Sys_CreateConditional();
		Sys_LockConditional(cond);
		sc->thread = Sys_CreateThread("dsoundmixer", DSOUND_Thread, sc, THREADP_HIGHEST, 0);
		if (!sc->thread)
		{
			Con_SafePrintf ("Unable to create sound mixing thread\n");
			return false;
		}

		//wait for the thread to finish (along with all its error con printfs etc
		if (!Sys_ConditionWait(cond))
			Con_SafePrintf ("Looks like the sound thread isn't starting up\n");
		Sys_UnlockConditional(cond);
		Sys_DestroyConditional(cond);

		if (!sc->selfpainting)
		{
			Sys_WaitOnThread(sc->thread);
			sc->thread = NULL;
			return false;
		}
		return true;
	}
	else
#endif
		return DSOUND_InitCard_Internal(sc, sc->name);
}

#define SDRVNAME "DSound"
static BOOL (CALLBACK  DSound_EnumCallback)(GUID FAR *guid, LPCSTR str1, LPCSTR str2, LPVOID parm)
{
	char guidbuf[128];
	wchar_t mssuck[128];
	void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename) = parm;
	if (guid == NULL)	//we don't care about the (dupe) default device
		return TRUE;

	StringFromGUID2(guid, mssuck, sizeof(mssuck)/sizeof(mssuck[0]));
	wcstombs(guidbuf, mssuck, sizeof(guidbuf));
	callback(SDRVNAME, guidbuf, va("DS: %s", str1));
	return TRUE;
}
static qboolean QDECL DSOUND_Enumerate(void (QDECL *cb) (const char *drivername, const char *devicecode, const char *readablename))
{
	if (!DSOUND_InitOutputLibrary())
		return false;
	if (pDirectSoundEnumerate)
	{
		pDirectSoundEnumerate(&DSound_EnumCallback, cb);
		return true;
	}
	return false;
}

sounddriver_t DSOUND_Output =
{
	SDRVNAME,
	DSOUND_InitCard,
	DSOUND_Enumerate
};

#endif












#if defined(VOICECHAT) && defined(AVAIL_DSOUND)


typedef struct
{
	LPDIRECTSOUNDCAPTURE DSCapture;
	LPDIRECTSOUNDCAPTUREBUFFER DSCaptureBuffer;
	long lastreadpos;
} dsndcapture_t;
static const long bufferbytes = 1024*1024;

static const long inputwidth = 2;

static BOOL CALLBACK dsound_capture_enumerate_ds(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	char guidbuf[128];
	wchar_t mssuck[128];
	void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename) = lpContext;

	if (lpGuid == NULL)	//we don't care about the (dupe) default device
		return TRUE;

	StringFromGUID2(lpGuid, mssuck, sizeof(mssuck)/sizeof(mssuck[0]));
	wcstombs(guidbuf, mssuck, sizeof(guidbuf));
	callback(SDRVNAME, guidbuf, va("DS: %s", lpcstrDescription));
	return TRUE;
}

static qboolean QDECL DSOUND_Capture_Enumerate (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename))
{
	if (!pDirectSoundCaptureEnumerate)
	{
		/*make sure its loaded*/
		if (!hInstDS)
			hInstDS = LoadLibrary("dsound.dll");
		if (hInstDS)
			pDirectSoundCaptureEnumerate = (void *)GetProcAddress(hInstDS,"DirectSoundCaptureEnumerateA");

		if (!pDirectSoundCaptureEnumerate)
			return false;
	}

	if (!FAILED(pDirectSoundCaptureEnumerate(dsound_capture_enumerate_ds, callback)))
		return true;
	return false;
}

static void *QDECL DSOUND_Capture_Init (int rate, const char *device)
{
	dsndcapture_t *result;
	DSCBUFFERDESC bufdesc;

	WAVEFORMATEX  wfxFormat;
	GUID *dsguid, guid;

	wfxFormat.wFormatTag = WAVE_FORMAT_PCM;
	wfxFormat.nChannels = 1;
	wfxFormat.nSamplesPerSec = rate;
	wfxFormat.wBitsPerSample = 8*inputwidth;
	wfxFormat.nBlockAlign = wfxFormat.nChannels * (wfxFormat.wBitsPerSample / 8);
	wfxFormat.nAvgBytesPerSec = wfxFormat.nSamplesPerSec * wfxFormat.nBlockAlign;
	wfxFormat.cbSize = 0;

	bufdesc.dwSize = sizeof(bufdesc);
	bufdesc.dwBufferBytes = bufferbytes;
	bufdesc.dwFlags = 0;
	bufdesc.dwReserved = 0;
	bufdesc.lpwfxFormat = &wfxFormat;

	if (device && *device)
	{
		wchar_t mssuck[128];
		mbstowcs(mssuck, device, sizeof(mssuck)/sizeof(mssuck[0])-1);
		CLSIDFromString(mssuck, &guid);
		dsguid = &guid;
	}
	else
	{
		memset(&guid, 0, sizeof(GUID));
		dsguid = NULL;
	}

	/*probably already inited*/
	if (!hInstDS)
	{
		hInstDS = LoadLibrary("dsound.dll");

		if (hInstDS == NULL)
		{
			Con_SafePrintf ("Couldn't load dsound.dll\n");
			return NULL;
		}
	}
	/*global pointer, used only in this function*/
	if (!pDirectSoundCaptureCreate)
	{
		pDirectSoundCaptureCreate = (void *)GetProcAddress(hInstDS,"DirectSoundCaptureCreate");

		if (!pDirectSoundCaptureCreate)
		{
			Con_SafePrintf ("Couldn't get DS proc addr\n");
			return NULL;
		}
	}

	result = Z_Malloc(sizeof(*result));
	if (!FAILED(pDirectSoundCaptureCreate(dsguid, &result->DSCapture, NULL)))
	{
		if (!FAILED(IDirectSoundCapture_CreateCaptureBuffer(result->DSCapture, &bufdesc, &result->DSCaptureBuffer, NULL)))
		{
			return result;
		}
		IDirectSoundCapture_Release(result->DSCapture);
		Con_SafePrintf ("Couldn't create a capture buffer\n");
	}
	Z_Free(result);
	return NULL;
}

static void QDECL DSOUND_Capture_Start(void *ctx)
{
	DWORD capturePos;
	dsndcapture_t *c = ctx;
	IDirectSoundCaptureBuffer_Start(c->DSCaptureBuffer, DSBPLAY_LOOPING);

	c->lastreadpos = 0;
	IDirectSoundCaptureBuffer_GetCurrentPosition(c->DSCaptureBuffer, &capturePos, &c->lastreadpos);
}

static void QDECL DSOUND_Capture_Stop(void *ctx)
{
	dsndcapture_t *c = ctx;
	IDirectSoundCaptureBuffer_Stop(c->DSCaptureBuffer);
}

static void QDECL DSOUND_Capture_Shutdown(void *ctx)
{
	dsndcapture_t *c = ctx;
	if (c->DSCaptureBuffer)
	{
		IDirectSoundCaptureBuffer_Stop(c->DSCaptureBuffer);
		IDirectSoundCaptureBuffer_Release(c->DSCaptureBuffer);
	}
	if (c->DSCapture)
	{
		IDirectSoundCapture_Release(c->DSCapture);
	}
	Z_Free(ctx);
}

/*minsamples is a hint*/
static unsigned int QDECL DSOUND_Capture_Update(void *ctx, unsigned char *buffer, unsigned int minbytes, unsigned int maxbytes)
{
	dsndcapture_t *c = ctx;
	HRESULT hr;
	LPBYTE lpbuf1 = NULL;
	LPBYTE lpbuf2 = NULL;
	DWORD dwsize1 = 0;
	DWORD dwsize2 = 0;

	DWORD capturePos;
	DWORD readPos;
	long  filled;

// Query to see how much data is in buffer.
	hr = IDirectSoundCaptureBuffer_GetCurrentPosition(c->DSCaptureBuffer, &capturePos, &readPos);
	if (hr != DS_OK)
	{
		return 0;
	}
	filled = readPos - c->lastreadpos;
	if (filled < 0)
		filled += bufferbytes; // unwrap offset

	if (filled > maxbytes)	//figure out how much we need to empty it by, and if that's enough to be worthwhile.
		filled = maxbytes;
	else if (filled < minbytes)
		return 0;

//	filled /= inputwidth;
//	filled *= inputwidth;

	// Lock free space in the DS
	hr = IDirectSoundCaptureBuffer_Lock(c->DSCaptureBuffer, c->lastreadpos, filled, (void **) &lpbuf1, &dwsize1, (void **) &lpbuf2, &dwsize2, 0);
	if (hr == DS_OK)
	{
		// Copy from DS to the buffer
		memcpy(buffer, lpbuf1, dwsize1);
		if(lpbuf2 != NULL)
		{
			memcpy(buffer+dwsize1, lpbuf2, dwsize2);
		}
		// Update our buffer offset and unlock sound buffer
 		c->lastreadpos = (c->lastreadpos + dwsize1 + dwsize2) % bufferbytes;
		IDirectSoundCaptureBuffer_Unlock(c->DSCaptureBuffer, lpbuf1, dwsize1, lpbuf2, dwsize2);
	}
	else
	{
		return 0;
	}
	return filled;
}
snd_capture_driver_t DSOUND_Capture =
{
	1,
	SDRVNAME,
	DSOUND_Capture_Enumerate,
	DSOUND_Capture_Init,
	DSOUND_Capture_Start,
	DSOUND_Capture_Update,
	DSOUND_Capture_Stop,
	DSOUND_Capture_Shutdown
};
#endif
