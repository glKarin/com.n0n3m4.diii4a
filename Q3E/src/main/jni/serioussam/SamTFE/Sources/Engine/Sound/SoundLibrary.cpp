/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifdef _DIII4A //karin: sound on Android
#ifdef _ANDROID_SOUND_OBOE
#include "SoundLibrary_oboe.cpp"
#else
#include "SoundLibrary_opensl.cpp"
#endif
#else

#include "Engine/StdH.h"


// !!! FIXME : rcg12162001 This file really needs to be ripped apart and
// !!! FIXME : rcg12162001  into platform/driver specific subdirectories.


// !!! FIXME : rcg10132001 what is this file?
#ifdef PLATFORM_WIN32
#include "initguid.h"
#endif

// !!! FIXME : Move all the SDL stuff to a different file...
#ifdef PLATFORM_UNIX
#include "SDL.h"
#endif

#include <Engine/Engine.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Base/Translation.h>

#include <Engine/Base/Shell.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/Console.h>
#include <Engine/Base/Console_internal.h>
#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Base/IFeel.h>

#include <Engine/Sound/SoundProfile.h>
#include <Engine/Sound/SoundListener.h>
#include <Engine/Sound/SoundData.h>
#include <Engine/Sound/SoundObject.h>
#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Network/Network.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

template class CStaticArray<CSoundListener>;

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

// pointer to global sound library object
CSoundLibrary *_pSound = NULL;


// console variables
__extern FLOAT snd_tmMixAhead  = 0.2f; // mix-ahead in seconds
__extern FLOAT snd_fSoundVolume = 1.0f;   // master volume for sound playing [0..1]
__extern FLOAT snd_fMusicVolume = 1.0f;   // master volume for music playing [0..1]
// NOTES: 
// - these 3d sound parameters have been set carefully, take extreme if changing !
// - ears distance of 20cm causes phase shift of up to 0.6ms which is very noticable
//   and is more than enough, too large values cause too much distorsions in other effects
// - pan strength needs not to be very strong, since lrfilter has panning-like influence also
// - if down filter is too large, it makes too much influence even on small elevation changes
//   and messes the situation completely
__extern FLOAT snd_fDelaySoundSpeed   = 1E10;   // sound speed used for delay [m/s]
__extern FLOAT snd_fDopplerSoundSpeed = 330.0f; // sound speed used for doppler [m/s]
__extern FLOAT snd_fEarsDistance = 0.2f;   // distance between listener's ears
__extern FLOAT snd_fPanStrength  = 0.1f;   // panning modifier (0=none, 1= full)
__extern FLOAT snd_fLRFilter = 3.0f;   // filter for left-right
__extern FLOAT snd_fBFilter  = 5.0f;   // filter for back
__extern FLOAT snd_fUFilter  = 1.0f;   // filter for up
__extern FLOAT snd_fDFilter  = 3.0f;   // filter for down

ENGINE_API __extern INDEX snd_iFormat = 3;
__extern INDEX snd_bMono = FALSE;

static INDEX snd_iDevice = -1;
static INDEX snd_iInterface = 2;   // 0=WaveOut, 1=DirectSound, 2=EAX
static INDEX snd_iMaxOpenRetries = 3;
static INDEX snd_iMaxExtraChannels = 32;
static FLOAT snd_tmOpenFailDelay = 0.5f;
static FLOAT snd_fEAXPanning = 0.0f;

static FLOAT snd_fNormalizer = 0.9f;
static FLOAT _fLastNormalizeValue = 1;

#ifdef PLATFORM_WIN32
extern HWND  _hwndMain; // global handle for application window
static HWND  _hwndCurrent = NULL;
static HINSTANCE _hInstDS = NULL;
#else
static CTString snd_strDeviceName;
#endif

static BOOL  _bMuted  = FALSE;
static INDEX _iLastEnvType = 1234;
static FLOAT _fLastEnvSize = 1234;

#ifdef PLATFORM_WIN32
static FLOAT _fLastPanning = 1234;
static INDEX _iWriteOffset  = 0;
static INDEX _iWriteOffset2 = 0;

// TEMP! - for writing mixer buffer to file
static FILE *_filMixerBuffer;
static BOOL _bOpened = FALSE;
#endif

#define WAVEOUTBLOCKSIZE 1024
#define MINPAN (1.0f)
#define MAXPAN (9.0f)



/**
 * ----------------------------
 *    Sound Library functions
 * ----------------------------
**/

// rcg12162001 Simple Directmedia Layer sound implementation.
#ifdef PLATFORM_UNIX

static Uint8 sdl_silence = 0;
static volatile SLONG sdl_backbuffer_allocation = 0;
static Uint8 *sdl_backbuffer = NULL;
static volatile SLONG sdl_backbuffer_pos = 0;
static volatile SLONG sdl_backbuffer_remain = 0;
static SDL_AudioDeviceID sdl_audio_device = 0;

static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
{
  ASSERT(!_bDedicatedServer);
  ASSERT(sdl_backbuffer != NULL);
  ASSERT(sdl_backbuffer_remain <= sdl_backbuffer_allocation);
  ASSERT(sdl_backbuffer_remain >= 0);
  ASSERT(sdl_backbuffer_pos < sdl_backbuffer_allocation);
  ASSERT(sdl_backbuffer_pos >= 0);

      // "avail" is just the byte count before the end of the buffer.
      // "cpysize" is how many bytes can actually be copied.
  int avail = sdl_backbuffer_allocation - sdl_backbuffer_pos;
  int cpysize = (len < sdl_backbuffer_remain) ? len : sdl_backbuffer_remain;
  Uint8 *src = sdl_backbuffer + sdl_backbuffer_pos;

  if (avail < cpysize)  // Copy would pass end of ring buffer?
    cpysize = avail;

  if (cpysize > 0) {
    memcpy(stream, src, cpysize);  // move first block to SDL stream.
    sdl_backbuffer_remain -= cpysize;
    ASSERT(sdl_backbuffer_remain >= 0);
    len -= cpysize;
    ASSERT(len >= 0);
    stream += cpysize;
    sdl_backbuffer_pos += cpysize;
  } // if

  // See if we need to rotate to start of ring buffer...
  ASSERT(sdl_backbuffer_pos <= sdl_backbuffer_allocation);
  if (sdl_backbuffer_pos == sdl_backbuffer_allocation) {
    sdl_backbuffer_pos = 0;

    // we might need to feed SDL more data now...
    if (len > 0) {
      cpysize = (len < sdl_backbuffer_remain) ? len : sdl_backbuffer_remain;
      if (cpysize > 0) {
        memcpy(stream, sdl_backbuffer, cpysize);  // move 2nd block.
        sdl_backbuffer_pos += cpysize;
        ASSERT(sdl_backbuffer_pos < sdl_backbuffer_allocation);
        sdl_backbuffer_remain -= cpysize;
        ASSERT(sdl_backbuffer_remain >= 0);
        len -= cpysize;
        ASSERT(len >= 0);
        stream += cpysize;
      } // if
    } // if
  } // if

  // SDL _still_ needs more data than we've got! Fill with silence. (*shrug*)
  if (len > 0) {
      ASSERT(sdl_backbuffer_remain == 0);
      memset(stream, sdl_silence, len);
  } // if
} // sdl_audio_callback


// initialize the SDL audio subsystem.

static BOOL StartUp_SDLaudio( CSoundLibrary &sl, BOOL bReport=TRUE)
{
  bReport=TRUE;  // !!! FIXME ...how do you configure this externally?

  // not using DirectSound (obviously)
  sl.sl_bUsingDirectSound = FALSE;
  sl.sl_bUsingEAX = FALSE;
  snd_iDevice = 0;

  ASSERT(!_bDedicatedServer);
  if (_bDedicatedServer) {
    CPrintF("Dedicated server; not initializing audio.\n");
    return FALSE;
  }

  if( bReport) CPrintF(TRANSV("SDL audio initialization ...\n"));

  SDL_AudioSpec desired, obtained;
  SDL_zero(desired);
  SDL_zero(obtained);

  Sint16 bps = sl.sl_SwfeFormat.wBitsPerSample;
  if (bps <= 8)
    desired.format = AUDIO_U8;
  else if (bps <= 16)
    desired.format = AUDIO_S16LSB;
  else if (bps <= 32)
    desired.format = AUDIO_S32LSB;
  else {
    CPrintF(TRANSV("Unsupported bits-per-sample: %d\n"), bps);
    return FALSE;
  }
  desired.freq = sl.sl_SwfeFormat.nSamplesPerSec;

    // I dunno if this is the best idea, but I'll give it a try...
    //  should probably check a cvar for this...
  if (desired.freq <= 11025)
    desired.samples = 512;
  else if (desired.freq <= 22050)
    desired.samples = 1024;
  else if (desired.freq <= 44100)
    desired.samples = 2048;
  else
    desired.samples = 4096;  // (*shrug*)

  desired.channels = sl.sl_SwfeFormat.nChannels;
  desired.userdata = &sl;
  desired.callback = sdl_audio_callback;

  // !!! FIXME rcg12162001 We force SDL to convert the audio stream on the
  // !!! FIXME rcg12162001  fly to match sl.sl_SwfeFormat, but I'm curious
  // !!! FIXME rcg12162001  if the Serious Engine can handle it if we changed
  // !!! FIXME rcg12162001  sl.sl_SwfeFormat to match what the audio hardware
  // !!! FIXME rcg12162001  can handle. I'll have to check later.
  sdl_audio_device = SDL_OpenAudioDevice(snd_strDeviceName.IsEmpty() ? NULL : (const char *) snd_strDeviceName, 0, &desired, &obtained, 0);
  if (!sdl_audio_device) {
    CPrintF( TRANSV("SDL_OpenAudioDevice() error: %s\n"), SDL_GetError());
    return FALSE;
  }

  sdl_silence = obtained.silence;
  sdl_backbuffer_allocation = (obtained.size * 4);
  sdl_backbuffer = (Uint8 *)AllocMemory(sdl_backbuffer_allocation);
  sdl_backbuffer_remain = 0;
  sdl_backbuffer_pos = 0;

  // report success
  if( bReport) {
    STUBBED("Report actual SDL device name?");
    CPrintF( TRANSV("  opened device: %s\n"), "SDL audio stream");
    CPrintF( TRANSV("  %dHz, %dbit, %s\n"),
             sl.sl_SwfeFormat.nSamplesPerSec,
             sl.sl_SwfeFormat.wBitsPerSample,
             SDL_GetCurrentAudioDriver());
  }

  // determine whole mixer buffer size from mixahead console variable
  sl.sl_slMixerBufferSize = (SLONG)(ceil(snd_tmMixAhead*sl.sl_SwfeFormat.nSamplesPerSec) *
                            sl.sl_SwfeFormat.wBitsPerSample/8 * sl.sl_SwfeFormat.nChannels);
  // align size to be next multiply of WAVEOUTBLOCKSIZE
  sl.sl_slMixerBufferSize += WAVEOUTBLOCKSIZE - (sl.sl_slMixerBufferSize % WAVEOUTBLOCKSIZE);
  // decoder buffer always works at 44khz
  sl.sl_slDecodeBufferSize = sl.sl_slMixerBufferSize *
                           ((44100+sl.sl_SwfeFormat.nSamplesPerSec-1)/sl.sl_SwfeFormat.nSamplesPerSec);
  if( bReport) {
    CPrintF(TRANSV("  parameters: %d Hz, %d bit, stereo, mix-ahead: %gs\n"),
            sl.sl_SwfeFormat.nSamplesPerSec, sl.sl_SwfeFormat.wBitsPerSample, snd_tmMixAhead);
    CPrintF(TRANSV("  output buffers: %d x %d bytes\n"), 2, obtained.size);
    CPrintF(TRANSV("  mpx decode: %d bytes\n"), sl.sl_slDecodeBufferSize);
  }

  // initialize mixing and decoding buffer
  sl.sl_pslMixerBuffer  = (SLONG*)AllocMemory( sl.sl_slMixerBufferSize *2); // (*2 because of 32-bit buffer)
  sl.sl_pswDecodeBuffer = (SWORD*)AllocMemory( sl.sl_slDecodeBufferSize+4); // (+4 because of linear interpolation of last samples)

  // the audio callback can now safely fill the audio stream with silence
  //  until there is actual audio data to mix...
  SDL_PauseAudioDevice(sdl_audio_device, 0);

  // done
  return TRUE;
} // StartUp_SDLaudio


// SDL audio shutdown procedure
static void ShutDown_SDLaudio( CSoundLibrary &sl)
{
  SDL_PauseAudioDevice(sdl_audio_device, 1);

  if (sdl_backbuffer != NULL) {
    FreeMemory(sdl_backbuffer);
    sdl_backbuffer = NULL;
  }

  if (sl.sl_pslMixerBuffer != NULL) {
    FreeMemory( sl.sl_pslMixerBuffer);
    sl.sl_pslMixerBuffer = NULL;
  }

  if (sl.sl_pswDecodeBuffer != NULL) {
    FreeMemory(sl.sl_pswDecodeBuffer);
    sl.sl_pswDecodeBuffer = NULL;
  }

  SDL_CloseAudioDevice(sdl_audio_device);
  sdl_audio_device = 0;
} // ShutDown_SDLaudio


// SDL_LockAudio() must be in effect when calling this!
//  ...and stay in effect until after CopyMixerBuffer_SDLaudio() is called!
static SLONG PrepareSoundBuffer_SDLaudio( CSoundLibrary &sl)
{
  ASSERT(sdl_backbuffer_remain >= 0);
  ASSERT(sdl_backbuffer_remain <= sdl_backbuffer_allocation);
  return(sdl_backbuffer_allocation - sdl_backbuffer_remain);
} // PrepareSoundBuffer_SDLaudio


// SDL_LockAudio() must be in effect when calling this!
//  ...and have been in effect since PrepareSoundBuffer_SDLaudio was called!
static void CopyMixerBuffer_SDLaudio( CSoundLibrary &sl, SLONG datasize)
{
  ASSERT((sdl_backbuffer_allocation - sdl_backbuffer_remain) >= datasize);

  SLONG fillpos = sdl_backbuffer_pos + sdl_backbuffer_remain;
  if (fillpos > sdl_backbuffer_allocation)
    fillpos -= sdl_backbuffer_allocation;

  SLONG cpysize = datasize;
  if ( (cpysize + fillpos) > sdl_backbuffer_allocation)
    cpysize = sdl_backbuffer_allocation - fillpos;

  Uint8 *src = sdl_backbuffer + fillpos;
  CopyMixerBuffer_stereo(0, src, cpysize);
  datasize -= cpysize;
  sdl_backbuffer_remain += cpysize;
  if (datasize > 0) {  // rotate to start of ring buffer?
    CopyMixerBuffer_stereo(cpysize, sdl_backbuffer, datasize);
    sdl_backbuffer_remain += datasize;
  } // if

  ASSERT(sdl_backbuffer_remain <= sdl_backbuffer_allocation);
} // CopyMixerBuffer_SDLaudio


#endif  // defined PLATFORM_UNIX  (SDL audio implementation)




/*
 *  Construct uninitialized sound library.
 */
CSoundLibrary::CSoundLibrary(void)
{
  sl_csSound.cs_iIndex = 3000;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear sound format
  memset( &sl_SwfeFormat, 0, sizeof(WAVEFORMATEX));
  sl_EsfFormat = SF_NONE;

  // reset buffer ptrs
  sl_pslMixerBuffer   = NULL;
  sl_pswDecodeBuffer  = NULL;
  sl_pubBuffersMemory = NULL;

#ifdef PLATFORM_WIN32
  // clear wave out data
  sl_hwoWaveOut = NULL;

  // clear direct sound data
  _hInstDS = NULL;
  sl_pDS   = NULL;
  sl_pKSProperty = NULL;
  sl_pDSPrimary    = NULL;
  sl_pDSSecondary  = NULL;
  sl_pDSSecondary2 = NULL;
  sl_pDSListener    = NULL;
  sl_pDSSourceLeft  = NULL;
  sl_pDSSourceRight = NULL;
#endif

  sl_bUsingDirectSound = FALSE;
  sl_bUsingEAX = FALSE;
}


/*
 *  Destruct (and clean up).
 */
CSoundLibrary::~CSoundLibrary(void)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear sound enviroment
  Clear();

  // clear any installed sound decoders
  CSoundDecoder::EndPlugins();
}



// post sound console variables' functions

static FLOAT _tmLastMixAhead = 1234;
static INDEX _iLastFormat = 1234;
static INDEX _iLastDevice = 1234;
static INDEX _iLastAPI = 1234;

static void SndPostFunc(void *pArgs)
{
  // clamp variables
  snd_tmMixAhead = Clamp( snd_tmMixAhead, 0.1f, 0.9f);
  snd_iFormat    = Clamp( snd_iFormat, (INDEX)CSoundLibrary::SF_NONE, (INDEX)CSoundLibrary::SF_44100_16);
  snd_iDevice    = Clamp( snd_iDevice, (INDEX)-1, (INDEX)15);
  snd_iInterface = Clamp( snd_iInterface, (INDEX)0, (INDEX)2);
  // if any variable has been changed
  if( _tmLastMixAhead!=snd_tmMixAhead || _iLastFormat!=snd_iFormat
   || _iLastDevice!=snd_iDevice || _iLastAPI!=snd_iInterface) {
    // reinit sound format
    _pSound->SetFormat( (enum CSoundLibrary::SoundFormat)snd_iFormat, TRUE);
  }
}



/*
 *  some internal functions
 */

#ifdef PLATFORM_WIN32
// DirectSound shutdown procedure
static void ShutDown_dsound( CSoundLibrary &sl)
{
  // free direct sound buffer(s)
  sl.sl_bUsingDirectSound = FALSE;
  sl.sl_bUsingEAX = FALSE;

  if( sl.sl_pDSSourceRight!=NULL) {
    sl.sl_pDSSourceRight->Release();
    sl.sl_pDSSourceRight = NULL;
  }
  if( sl.sl_pDSSourceLeft != NULL) {
    sl.sl_pDSSourceLeft->Release();
    sl.sl_pDSSourceLeft = NULL;
  }
  if( sl.sl_pDSListener != NULL) {
    sl.sl_pDSListener->Release();
    sl.sl_pDSListener = NULL;
  }

  if( sl.sl_pDSSecondary2 != NULL) {
    sl.sl_pDSSecondary2->Stop();
    sl.sl_pDSSecondary2->Release();
    sl.sl_pDSSecondary2 = NULL;
  }
  if( sl.sl_pDSSecondary != NULL) {
    sl.sl_pDSSecondary->Stop();
    sl.sl_pDSSecondary->Release();
    sl.sl_pDSSecondary = NULL;
  }
  if( sl.sl_pDSPrimary!=NULL) {
    sl.sl_pDSPrimary->Stop();
    sl.sl_pDSPrimary->Release();
    sl.sl_pDSPrimary = NULL;
  }

  if( sl.sl_pKSProperty != NULL) {
    sl.sl_pKSProperty->Release();
    sl.sl_pKSProperty = NULL;
  }

  // free direct sound object
  if( sl.sl_pDS!=NULL) {
    // reset cooperative level
    if( _hwndCurrent!=NULL) sl.sl_pDS->SetCooperativeLevel( _hwndCurrent, DSSCL_NORMAL);
    sl.sl_pDS->Release();
    sl.sl_pDS = NULL;
  }
  // free direct sound library
  if( _hInstDS != NULL) {
    FreeLibrary(_hInstDS);
    _hInstDS = NULL;
  }
  // free memory
  if( sl.sl_pslMixerBuffer!=NULL) {
    FreeMemory( sl.sl_pslMixerBuffer);
    sl.sl_pslMixerBuffer = NULL;
  }
  if( sl.sl_pswDecodeBuffer!=NULL) {
    FreeMemory( sl.sl_pswDecodeBuffer);
    sl.sl_pswDecodeBuffer = NULL;
  }
}
#endif


/*
 *  Set wave format from library format
 */
static void SetWaveFormat( CSoundLibrary::SoundFormat EsfFormat, WAVEFORMATEX &wfeFormat)
{
  // change Library Wave Format
  memset( &wfeFormat, 0, sizeof(WAVEFORMATEX));
  wfeFormat.wFormatTag = WAVE_FORMAT_PCM;
  wfeFormat.nChannels = 2;
  wfeFormat.wBitsPerSample = 16;
  switch( EsfFormat) {
    case CSoundLibrary::SF_11025_16: wfeFormat.nSamplesPerSec = 11025;  break;
    case CSoundLibrary::SF_22050_16: wfeFormat.nSamplesPerSec = 22050;  break;
    case CSoundLibrary::SF_44100_16: wfeFormat.nSamplesPerSec = 44100;  break;
    case CSoundLibrary::SF_NONE: ASSERTALWAYS( "Can't set to NONE format"); break;
    default:                     ASSERTALWAYS( "Unknown Sound format");     break;
  }
  wfeFormat.nBlockAlign     = (wfeFormat.wBitsPerSample / 8) * wfeFormat.nChannels;
  wfeFormat.nAvgBytesPerSec = wfeFormat.nSamplesPerSec * wfeFormat.nBlockAlign;
}


/*
 *  Set library format from wave format
 */
static void SetLibraryFormat( CSoundLibrary &sl)
{
  // if library format is none return
  if( sl.sl_EsfFormat == CSoundLibrary::SF_NONE) return;

  // else check wave format to determine library format
  ULONG ulFormat = sl.sl_SwfeFormat.nSamplesPerSec;
  // find format
  switch( ulFormat) {
    case 11025: sl.sl_EsfFormat = CSoundLibrary::SF_11025_16; break;
    case 22050: sl.sl_EsfFormat = CSoundLibrary::SF_22050_16; break;
    case 44100: sl.sl_EsfFormat = CSoundLibrary::SF_44100_16; break;
    // unknown format
    default:
      ASSERTALWAYS( "Unknown sound format");
      FatalError( TRANS("Unknown sound format"));
      sl.sl_EsfFormat = CSoundLibrary::SF_ILLEGAL;
  }
}


#ifdef PLATFORM_WIN32
static BOOL DSFail( CSoundLibrary &sl, char *strError) 
{
  CPrintF(strError);
  ShutDown_dsound(sl);
  snd_iInterface=1; // if EAX failed -> try DirectSound
  return FALSE;
}


// some helper functions for DirectSound
static BOOL DSInitSecondary( CSoundLibrary &sl, LPDIRECTSOUNDBUFFER &pBuffer, SLONG slSize)
{
  // eventuallt adjust for EAX
  DWORD dwFlag3D = NONE;
  if( snd_iInterface==2) {
    dwFlag3D = DSBCAPS_CTRL3D;
    sl.sl_SwfeFormat.nChannels=1;  // mono output
    sl.sl_SwfeFormat.nBlockAlign/=2;
    sl.sl_SwfeFormat.nAvgBytesPerSec/=2;
    slSize/=2;
  }
  DSBUFFERDESC dsBuffer;
  memset( &dsBuffer, 0, sizeof(dsBuffer));
  dsBuffer.dwSize  = sizeof(DSBUFFERDESC);
  dsBuffer.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | dwFlag3D;
  dsBuffer.dwBufferBytes = slSize; 
  dsBuffer.lpwfxFormat = &sl.sl_SwfeFormat;
  HRESULT hResult = sl.sl_pDS->CreateSoundBuffer( &dsBuffer, &pBuffer, NULL);
  if( snd_iInterface==2) {
      // revert back to original wave format (stereo)
    sl.sl_SwfeFormat.nChannels=2;
    sl.sl_SwfeFormat.nBlockAlign*=2;
    sl.sl_SwfeFormat.nAvgBytesPerSec*=2;
  }
  if( hResult != DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot create secondary buffer.\n"));
  return TRUE;
}


static BOOL DSLockBuffer( CSoundLibrary &sl, LPDIRECTSOUNDBUFFER pBuffer, SLONG slSize, LPVOID &lpData, DWORD &dwSize)
{
  INDEX ctRetries = 1000;  // too much?
  if( sl.sl_bUsingEAX) slSize/=2; // buffer is mono in case of EAX
  FOREVER {
    HRESULT hResult = pBuffer->Lock( 0, slSize, &lpData, &dwSize, NULL, NULL, 0);
    if( hResult==DS_OK && slSize==dwSize) return TRUE;
    if( hResult!=DSERR_BUFFERLOST) return DSFail( sl, TRANS("  ! DirectSound error: Cannot lock sound buffer.\n"));
    if( ctRetries-- == 0) return DSFail( sl, TRANS("  ! DirectSound error: Couldn't restore sound buffer.\n"));
    pBuffer->Restore();
  }
}
 

static void DSPlayBuffers( CSoundLibrary &sl)
{
  DWORD dw;
  BOOL bInitiatePlay = FALSE;
  ASSERT( sl.sl_pDSSecondary!=NULL && sl.sl_pDSPrimary!=NULL);
  if( sl.sl_bUsingEAX && sl.sl_pDSSecondary2->GetStatus(&dw)==DS_OK && !(dw&DSBSTATUS_PLAYING)) bInitiatePlay = TRUE;
  if( sl.sl_pDSSecondary->GetStatus(&dw)==DS_OK && !(dw&DSBSTATUS_PLAYING)) bInitiatePlay = TRUE;
  if( sl.sl_pDSPrimary->GetStatus(&dw)==DS_OK && !(dw&DSBSTATUS_PLAYING))  bInitiatePlay = TRUE;

  // done if all buffers are already playing
  if( !bInitiatePlay) return;

  // stop buffers (in case some buffers are playing
  sl.sl_pDSPrimary->Stop();
  sl.sl_pDSSecondary->Stop();
  if( sl.sl_bUsingEAX) sl.sl_pDSSecondary2->Stop();
  
  // check sound buffer lock and clear sound buffer(s) 
  LPVOID lpData;
  DWORD	 dwSize;
  if( !DSLockBuffer( sl, sl.sl_pDSSecondary, sl.sl_slMixerBufferSize, lpData, dwSize)) return;
  memset( lpData, 0, dwSize);
  sl.sl_pDSSecondary->Unlock( lpData, dwSize, NULL, 0);
  if( sl.sl_bUsingEAX) { 
    if( !DSLockBuffer( sl, sl.sl_pDSSecondary2, sl.sl_slMixerBufferSize, lpData, dwSize)) return;
    memset( lpData, 0, dwSize); 
    sl.sl_pDSSecondary2->Unlock( lpData, dwSize, NULL, 0);
    // start playing EAX additional buffer
    sl.sl_pDSSecondary2->Play( 0, 0, DSBPLAY_LOOPING);
  }
  // start playing standard DirectSound buffers
  sl.sl_pDSPrimary->Play(   0, 0, DSBPLAY_LOOPING);
  sl.sl_pDSSecondary->Play( 0, 0, DSBPLAY_LOOPING);
  _iWriteOffset  = 0;
  _iWriteOffset2 = 0;

  // adjust starting offsets for EAX
  if( sl.sl_bUsingEAX) {
    DWORD dwCursor1, dwCursor2;
    SLONG slMinDelta = MAX_SLONG;
    for( INDEX i=0; i<10; i++) { // shoud be enough to screw interrupts
      sl.sl_pDSSecondary->GetCurrentPosition(  &dwCursor1, NULL);
      sl.sl_pDSSecondary2->GetCurrentPosition( &dwCursor2, NULL);
      SLONG slDelta1 = dwCursor2-dwCursor1;
      sl.sl_pDSSecondary2->GetCurrentPosition( &dwCursor2, NULL);
      sl.sl_pDSSecondary->GetCurrentPosition(  &dwCursor1, NULL);
      SLONG slDelta2 = dwCursor2-dwCursor1;
      SLONG slDelta  = (slDelta1+slDelta2) /2;
      if( slDelta<slMinDelta) slMinDelta = slDelta;
      //CPrintF( "D1=%5d,  D2=%5d,  AD=%5d,  MD=%5d\n", slDelta1, slDelta2, slDelta, slMinDelta);
    }
    if( slMinDelta<0) _iWriteOffset  = -slMinDelta*2; // 2 because of offset is stereo
    if( slMinDelta>0) _iWriteOffset2 = +slMinDelta*2; 
    _iWriteOffset  += _iWriteOffset  & 3;  // round to 4 bytes
    _iWriteOffset2 += _iWriteOffset2 & 3;

    // assure that first writing offsets are inside buffers
    if( _iWriteOffset >=sl.sl_slMixerBufferSize) _iWriteOffset  -= sl.sl_slMixerBufferSize;
    if( _iWriteOffset2>=sl.sl_slMixerBufferSize) _iWriteOffset2 -= sl.sl_slMixerBufferSize;
    ASSERT( _iWriteOffset >=0 && _iWriteOffset <sl.sl_slMixerBufferSize);
    ASSERT( _iWriteOffset2>=0 && _iWriteOffset2<sl.sl_slMixerBufferSize);
  }
}


// init and set DirectSound format (internal)

static BOOL StartUp_dsound( CSoundLibrary &sl, BOOL bReport=TRUE)
{
  // startup
  sl.sl_bUsingDirectSound = FALSE;
  ASSERT( _hInstDS==NULL && sl.sl_pDS==NULL);
  ASSERT( sl.sl_pDSSecondary==NULL && sl.sl_pDSPrimary==NULL);
  // update window handle (just in case)
  HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);
  
  if( bReport) CPrintF(TRANSV("Direct Sound initialization ...\n"));
  ASSERT( _hInstDS==NULL);
  _hInstDS = LoadLibraryA( "dsound.dll");
  if( _hInstDS==NULL) {
    CPrintF( TRANS("  ! DirectSound error: Cannot load 'DSOUND.DLL'.\n"));
    return FALSE;
  }
  // get main procedure address
  pDirectSoundCreate = (HRESULT(WINAPI *)(GUID FAR *, LPDIRECTSOUND FAR *, IUnknown FAR *))GetProcAddress( _hInstDS, "DirectSoundCreate");
  if( pDirectSoundCreate==NULL) return DSFail( sl, TRANS("  ! DirectSound error: Cannot get procedure address.\n"));

  // init dsound
  HRESULT	hResult;
  hResult = pDirectSoundCreate( NULL, &sl.sl_pDS, NULL);
  if( hResult != DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot create object.\n"));

  // get capabilities
  DSCAPS dsCaps;
  dsCaps.dwSize = sizeof(dsCaps);
  hResult = sl.sl_pDS->GetCaps( &dsCaps);
  if( hResult != DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot determine capabilites.\n"));

  // fail if in emulation mode
  if( dsCaps.dwFlags & DSCAPS_EMULDRIVER) {
    CPrintF( TRANS("  ! DirectSound error: No driver installed.\n"));
    ShutDown_dsound(sl);
    return FALSE;
  }

  // set cooperative level to priority
  _hwndCurrent = _hwndMain;
  hResult = sl.sl_pDS->SetCooperativeLevel( _hwndCurrent, DSSCL_PRIORITY);
  if( hResult != DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot set cooperative level.\n"));

  // prepare 3D flag if EAX
  DWORD dwFlag3D = NONE;
  if( snd_iInterface==2) dwFlag3D = DSBCAPS_CTRL3D;

  // create primary sound buffer (must have one)
  DSBUFFERDESC dsBuffer;
  memset( &dsBuffer, 0, sizeof(dsBuffer));
  dsBuffer.dwSize  = sizeof(dsBuffer);
  dsBuffer.dwFlags = DSBCAPS_PRIMARYBUFFER | dwFlag3D;
  dsBuffer.dwBufferBytes = 0;
  dsBuffer.lpwfxFormat = NULL;
  hResult = sl.sl_pDS->CreateSoundBuffer( &dsBuffer, &sl.sl_pDSPrimary, NULL);
  if( hResult != DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot create primary sound buffer.\n"));

  // set primary buffer format
  WAVEFORMATEX wfx = sl.sl_SwfeFormat;
  hResult = sl.sl_pDSPrimary->SetFormat(&wfx);
  if( hResult != DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot set primary sound buffer format.\n"));

  // startup secondary sound buffer(s)
  SLONG slBufferSize = (SLONG)(ceil(snd_tmMixAhead*sl.sl_SwfeFormat.nSamplesPerSec) *
                       sl.sl_SwfeFormat.wBitsPerSample/8 * sl.sl_SwfeFormat.nChannels);
  if( !DSInitSecondary( sl, sl.sl_pDSSecondary, slBufferSize)) return FALSE;

  // set some additionals for EAX
  if( snd_iInterface==2)
  {
    // 2nd secondary buffer
    if( !DSInitSecondary( sl, sl.sl_pDSSecondary2, slBufferSize)) return FALSE;
    // set 3D for all buffers
    HRESULT hr1,hr2,hr3,hr4;
    hr1 = sl.sl_pDSPrimary->QueryInterface(  IID_IDirectSound3DListener, (LPVOID*)&sl.sl_pDSListener);
    hr2 = sl.sl_pDSSecondary->QueryInterface(  IID_IDirectSound3DBuffer, (LPVOID*)&sl.sl_pDSSourceLeft);
    hr3 = sl.sl_pDSSecondary2->QueryInterface( IID_IDirectSound3DBuffer, (LPVOID*)&sl.sl_pDSSourceRight);
    if( hr1!=DS_OK || hr2!=DS_OK || hr3!=DS_OK) return DSFail( sl, TRANS("  ! DirectSound3D error: Cannot set 3D sound buffer.\n"));

    hr1 = sl.sl_pDSListener->SetPosition( 0,0,0, DS3D_DEFERRED);
    hr2 = sl.sl_pDSListener->SetOrientation( 0,0,1, 0,1,0, DS3D_DEFERRED);
    hr3 = sl.sl_pDSListener->SetRolloffFactor( 1, DS3D_DEFERRED);
    if( hr1!=DS_OK || hr2!=DS_OK || hr3!=DS_OK) return DSFail( sl, TRANS("  ! DirectSound3D error: Cannot set 3D parameters for listener.\n"));
    hr1 = sl.sl_pDSSourceLeft->SetMinDistance(  MINPAN, DS3D_DEFERRED);
    hr2 = sl.sl_pDSSourceLeft->SetMaxDistance(  MAXPAN, DS3D_DEFERRED);
    hr3 = sl.sl_pDSSourceRight->SetMinDistance( MINPAN, DS3D_DEFERRED);
    hr4 = sl.sl_pDSSourceRight->SetMaxDistance( MAXPAN, DS3D_DEFERRED);
    if( hr1!=DS_OK || hr2!=DS_OK || hr3!=DS_OK || hr4!=DS_OK) {
      return DSFail( sl, TRANS("  ! DirectSound3D error: Cannot set 3D parameters for sound source.\n"));
    }
    // apply
    hResult = sl.sl_pDSListener->CommitDeferredSettings();
    if( hResult!=DS_OK) return DSFail( sl, TRANS("  ! DirectSound3D error: Cannot apply 3D parameters.\n"));
    // reset EAX parameters
    _fLastPanning = 1234; 
    _iLastEnvType = 1234;
    _fLastEnvSize = 1234;

    // query property interface to EAX
    hResult = sl.sl_pDSSourceLeft->QueryInterface( IID_IKsPropertySet, (LPVOID*)&sl.sl_pKSProperty);
    if( hResult != DS_OK) return DSFail( sl, TRANS("  ! EAX error: Cannot set property interface.\n"));
    // query support
    ULONG ulSupport = 0;
    hResult = sl.sl_pKSProperty->QuerySupport( DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT, &ulSupport);
    if( hResult != DS_OK || !(ulSupport&KSPROPERTY_SUPPORT_SET)) return DSFail( sl, TRANS("  ! EAX error: Cannot query property support.\n"));
    hResult = sl.sl_pKSProperty->QuerySupport( DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, &ulSupport);
    if( hResult != DS_OK || !(ulSupport&KSPROPERTY_SUPPORT_SET)) return DSFail( sl, TRANS("  ! EAX error: Cannot query property support.\n"));
    // made it - EAX's on!
    sl.sl_bUsingEAX = TRUE;
  }

  // mark that dsound is operative and set mixer buffer size (decoder buffer always works at 44khz)
  _iWriteOffset  = 0;
  _iWriteOffset2 = 0;
  sl.sl_bUsingDirectSound  = TRUE;
  sl.sl_slMixerBufferSize  = slBufferSize;
  sl.sl_slDecodeBufferSize = sl.sl_slMixerBufferSize *
                           ((44100+sl.sl_SwfeFormat.nSamplesPerSec-1) /sl.sl_SwfeFormat.nSamplesPerSec);
  // allocate mixing and decoding buffers
  sl.sl_pslMixerBuffer  = (SLONG*)AllocMemory( sl.sl_slMixerBufferSize *2); // (*2 because of 32-bit buffer)
  sl.sl_pswDecodeBuffer = (SWORD*)AllocMemory( sl.sl_slDecodeBufferSize+4); // (+4 because of linear interpolation of last samples)

  // report success
  if( bReport) {
    CTString strDevice = TRANS("default device");
    if( snd_iDevice>=0) strDevice.PrintF( TRANS("device %d"), snd_iDevice); 
    CPrintF( TRANS("  %dHz, %dbit, %s, mix-ahead: %gs\n"), 
             sl.sl_SwfeFormat.nSamplesPerSec, sl.sl_SwfeFormat.wBitsPerSample, strDevice, snd_tmMixAhead); 
    CPrintF(TRANSV("  mixer buffer size:  %d KB\n"), sl.sl_slMixerBufferSize /1024);
    CPrintF(TRANSV("  decode buffer size: %d KB\n"), sl.sl_slDecodeBufferSize/1024);
    // EAX?
    CTString strEAX = TRANS("Disabled");
    if( sl.sl_bUsingEAX) strEAX = TRANS("Enabled");
    CPrintF( TRANS("  EAX: %s\n"), strEAX);
  } 
  // done
  return TRUE;
}


// set WaveOut format (internal)

static INDEX _ctChannelsOpened = 0;
static BOOL StartUp_waveout( CSoundLibrary &sl, BOOL bReport=TRUE)
{
  // not using DirectSound (obviously)
  sl.sl_bUsingDirectSound = FALSE;
  sl.sl_bUsingEAX = FALSE;
  if( bReport) CPrintF(TRANSV("WaveOut initialization ...\n"));
  // set maximum total number of retries for device opening
  INDEX ctMaxRetries = snd_iMaxOpenRetries;
  _ctChannelsOpened = 0;
  MMRESULT res;
  // repeat
  FOREVER {
    // try to open wave device
    HWAVEOUT hwo;
    res = waveOutOpen( &hwo, (snd_iDevice<0)?WAVE_MAPPER:snd_iDevice, &sl.sl_SwfeFormat, NULL, NULL, NONE);
    // if opened
    if( res == MMSYSERR_NOERROR) {
      _ctChannelsOpened++;
      // if first one
      if (_ctChannelsOpened==1) {
        // remember as used waveout
        sl.sl_hwoWaveOut = hwo;
      // if extra channel
      } else {
        // remember under extra
        sl.sl_ahwoExtra.Push() = hwo;
      }
      // if no extra channels should be taken
      if (_ctChannelsOpened>=snd_iMaxExtraChannels+1) {
        // no more tries
        break;
      }    
    // if cannot open
    } else {
      // decrement retry counter
      ctMaxRetries--;
      // if no more retries
      if (ctMaxRetries<0) {
        // quit trying
        break;
      // if more retries left
      } else {
        // wait a bit (probably sound-scheme is playing)
        Sleep(int(snd_tmOpenFailDelay*1000));
      }
    }
  }

  // if couldn't set format
  if( _ctChannelsOpened==0 && res != MMSYSERR_NOERROR) {
    // report error
    CTString strError;
    switch (res) {
    case MMSYSERR_ALLOCATED:    strError = TRANS("Device already in use.");     break;
    case MMSYSERR_BADDEVICEID:  strError = TRANS("Bad device number.");         break;
    case MMSYSERR_NODRIVER:     strError = TRANS("No driver installed.");       break;
    case MMSYSERR_NOMEM:        strError = TRANS("Memory allocation problem."); break;
    case WAVERR_BADFORMAT:      strError = TRANS("Unsupported data format.");   break;
    case WAVERR_SYNC:           strError = TRANS("Wrong flag?");                break;
    default: strError.PrintF( "%d", res);
    };
    CPrintF( TRANS("  ! WaveOut error: %s\n"), strError);
    return FALSE;
  }

  // get waveout capabilities
  WAVEOUTCAPS woc;
  memset( &woc, 0, sizeof(woc));
  res = waveOutGetDevCaps((int)sl.sl_hwoWaveOut, &woc, sizeof(woc));
  // report success
  if( bReport) {
    CTString strDevice = TRANS("default device");
    if( snd_iDevice>=0) strDevice.PrintF( TRANS("device %d"), snd_iDevice); 
    CPrintF( TRANS("  opened device: %s\n"), woc.szPname);
    CPrintF( TRANS("  %dHz, %dbit, %s\n"), 
             sl.sl_SwfeFormat.nSamplesPerSec, sl.sl_SwfeFormat.wBitsPerSample, strDevice);
  }

  // determine whole mixer buffer size from mixahead console variable
  sl.sl_slMixerBufferSize = (SLONG)(ceil(snd_tmMixAhead*sl.sl_SwfeFormat.nSamplesPerSec) *
                            sl.sl_SwfeFormat.wBitsPerSample/8 * sl.sl_SwfeFormat.nChannels);
  // align size to be next multiply of WAVEOUTBLOCKSIZE
  sl.sl_slMixerBufferSize += WAVEOUTBLOCKSIZE - (sl.sl_slMixerBufferSize % WAVEOUTBLOCKSIZE);
  // determine number of WaveOut buffers
  const INDEX ctWOBuffers = sl.sl_slMixerBufferSize / WAVEOUTBLOCKSIZE;
  // decoder buffer always works at 44khz
  sl.sl_slDecodeBufferSize = sl.sl_slMixerBufferSize *
                           ((44100+sl.sl_SwfeFormat.nSamplesPerSec-1)/sl.sl_SwfeFormat.nSamplesPerSec);
  if( bReport) {
    CPrintF(TRANSV("  parameters: %d Hz, %d bit, stereo, mix-ahead: %gs\n"),
            sl.sl_SwfeFormat.nSamplesPerSec, sl.sl_SwfeFormat.wBitsPerSample, snd_tmMixAhead);
    CPrintF(TRANSV("  output buffers: %d x %d bytes\n"), ctWOBuffers, WAVEOUTBLOCKSIZE),
    CPrintF(TRANSV("  mpx decode: %d bytes\n"), sl.sl_slDecodeBufferSize),
    CPrintF(TRANSV("  extra sound channels taken: %d\n"), _ctChannelsOpened-1);
  }

  // initialise waveout sound buffers
  sl.sl_pubBuffersMemory = (UBYTE*)AllocMemory( sl.sl_slMixerBufferSize);
  memset( sl.sl_pubBuffersMemory, 0, sl.sl_slMixerBufferSize);
  sl.sl_awhWOBuffers.New(ctWOBuffers); 
  for( INDEX iBuffer = 0; iBuffer<sl.sl_awhWOBuffers.Count(); iBuffer++) {
    WAVEHDR &wh = sl.sl_awhWOBuffers[iBuffer];
    wh.lpData = (char*)(sl.sl_pubBuffersMemory + iBuffer*WAVEOUTBLOCKSIZE);
    wh.dwBufferLength = WAVEOUTBLOCKSIZE;
    wh.dwFlags = 0;
  }
  // initialize mixing and decoding buffer
  sl.sl_pslMixerBuffer  = (SLONG*)AllocMemory( sl.sl_slMixerBufferSize *2); // (*2 because of 32-bit buffer)
  sl.sl_pswDecodeBuffer = (SWORD*)AllocMemory( sl.sl_slDecodeBufferSize+4); // (+4 because of linear interpolation of last samples)

  // done
  return TRUE;
}
#endif



/*
 *  set sound format
 */
static void SetFormat_internal( CSoundLibrary &sl, CSoundLibrary::SoundFormat EsfNew, BOOL bReport)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl.sl_csSound, TRUE);

  // remember library format
  sl.sl_EsfFormat = EsfNew;
  // release library
  sl.ClearLibrary();

  // if none skip initialization
  _fLastNormalizeValue = 1;
  if( bReport) CPrintF(TRANSV("Setting sound format ...\n"));
  if( sl.sl_EsfFormat == CSoundLibrary::SF_NONE) {
    if( bReport) CPrintF(TRANSV("  (no sound)\n"));
    return;
  }

  // set wave format from library format
  SetWaveFormat( EsfNew, sl.sl_SwfeFormat);
  snd_iDevice    = Clamp( snd_iDevice, (INDEX)-1, (INDEX)(sl.sl_ctWaveDevices-1));
  snd_tmMixAhead = Clamp( snd_tmMixAhead, 0.1f, 0.9f);
  snd_iInterface = Clamp( snd_iInterface, (INDEX)0, (INDEX)2);

  BOOL bSoundOK = FALSE;
#ifdef PLATFORM_WIN32
  if( snd_iInterface==2) {
    // if wanted, 1st try to set EAX
    bSoundOK = StartUp_dsound( sl, bReport);  
  }
  if( !bSoundOK && snd_iInterface==1) {
    // if wanted, 2nd try to set DirectSound
    bSoundOK = StartUp_dsound( sl, bReport);  
  }
  // if DirectSound failed or not wanted
  if( !bSoundOK) { 
    // try waveout
    bSoundOK = StartUp_waveout( sl, bReport); 
    snd_iInterface = 0; // mark that DirectSound didn't make it
  }
#else
    bSoundOK = StartUp_SDLaudio(sl, bReport);
#endif

  // if didn't make it by now
  if( bReport) CPrintF("\n");
  if( !bSoundOK) {
    // revert to none in case sound init was unsuccessful
    sl.sl_EsfFormat = CSoundLibrary::SF_NONE;
    return;
  } 
  // set library format from wave format
  SetLibraryFormat(sl);

  // add timer handler
  _pTimer->AddHandler(&sl.sl_thTimerHandler);
}


/*
 *  Initialization
 */
void CSoundLibrary::Init(void)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  _pShell->DeclareSymbol( "void SndPostFunc(INDEX);", (void *) &SndPostFunc);

  _pShell->DeclareSymbol( "           user INDEX snd_bMono;", (void *) &snd_bMono);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fEarsDistance;",      (void *) &snd_fEarsDistance);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fDelaySoundSpeed;",   (void *) &snd_fDelaySoundSpeed);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fDopplerSoundSpeed;", (void *) &snd_fDopplerSoundSpeed);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fPanStrength;", (void *) &snd_fPanStrength);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fLRFilter;",    (void *) &snd_fLRFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fBFilter;",     (void *) &snd_fBFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fUFilter;",     (void *) &snd_fUFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fDFilter;",     (void *) &snd_fDFilter);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fSoundVolume;", (void *) &snd_fSoundVolume);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fMusicVolume;", (void *) &snd_fMusicVolume);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fNormalizer;",  (void *) &snd_fNormalizer);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_tmMixAhead post:SndPostFunc;", (void *) &snd_tmMixAhead);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iInterface post:SndPostFunc;", (void *) &snd_iInterface);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iDevice post:SndPostFunc;", (void *) &snd_iDevice);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iFormat post:SndPostFunc;", (void *) &snd_iFormat);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iMaxExtraChannels;", (void *) &snd_iMaxExtraChannels);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iMaxOpenRetries;",   (void *) &snd_iMaxOpenRetries);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_tmOpenFailDelay;",   (void *) &snd_tmOpenFailDelay);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fEAXPanning;", (void *) &snd_fEAXPanning);

#ifdef PLATFORM_UNIX
  _pShell->DeclareSymbol( "persistent user CTString snd_strDeviceName;", (void *) &snd_strDeviceName);
#endif

// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer) {
    CPrintF(TRANSV("Dedicated server; not initializing sound.\n"));
    return;
  }
#endif

  // print header
  CPrintF(TRANSV("Initializing sound...\n"));

  // initialize sound library and set no-sound format
  SetFormat(SF_NONE);

  // initialize any installed sound decoders

  CSoundDecoder::InitPlugins();

  sl_ctWaveDevices = 0;  // rcg11012005 valgrind fix.

#ifdef PLATFORM_WIN32
  // get number of devices
  const INDEX ctDevices = waveOutGetNumDevs();
  CPrintF(TRANSV("  Detected devices: %d\n"), ctDevices);
  sl_ctWaveDevices = ctDevices;
  
  // for each device
  for(INDEX iDevice=0; iDevice<ctDevices; iDevice++) {
    // get description
    WAVEOUTCAPS woc;
    memset( &woc, 0, sizeof(woc));
    MMRESULT res = waveOutGetDevCaps(iDevice, &woc, sizeof(woc));
    CPrintF(TRANSV("    device %d: %s\n"), 
      iDevice, woc.szPname);
    CPrintF(TRANSV("      ver: %d, id: %d.%d\n"), 
      woc.vDriverVersion, woc.wMid, woc.wPid);
    CPrintF(TRANSV("      form: 0x%08x, ch: %d, support: 0x%08x\n"), 
      woc.dwFormats, woc.wChannels, woc.dwSupport);
  }
  // done
#else
  const int ctDevices = SDL_GetNumAudioDevices(0);
  CPrintF(TRANSV("  Detected devices: %d\n"), ctDevices);
  sl_ctWaveDevices = ctDevices;
  for (int iDevice = 0; iDevice < ctDevices; iDevice++) {
    CPrintF(TRANSV("    device %d: %s\n"), 
      iDevice, SDL_GetAudioDeviceName(iDevice, 0));
  }
#endif

  CPrintF("\n");
}


/*
 *  Clear Sound Library
 */
void CSoundLibrary::Clear(void)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear all sounds and datas buffers
  {FOREACHINLIST(CSoundData, sd_Node, sl_ClhAwareList, itCsdStop) {
    FOREACHINLIST(CSoundObject, so_Node, (itCsdStop->sd_ClhLinkList), itCsoStop) {
      itCsoStop->Stop();
    }
    itCsdStop->ClearBuffer();
  }}

  // clear wave out data
  ClearLibrary();
  _fLastNormalizeValue = 1;
}


/* Clear Library WaveOut */
void CSoundLibrary::ClearLibrary(void)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);

  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // remove timer handler if added
  if (sl_thTimerHandler.th_Node.IsLinked()) {
    _pTimer->RemHandler(&sl_thTimerHandler);
  }

  sl_bUsingDirectSound = FALSE;
  sl_bUsingEAX = FALSE;

#ifdef PLATFORM_WIN32
  // shut down direct sound buffers (if needed)
  ShutDown_dsound(*this);

  // shut down wave out player buffers (if needed)
  if( sl_hwoWaveOut!=NULL)
  { // reset wave out play buffers (stop playing)
    MMRESULT res;
    res = waveOutReset(sl_hwoWaveOut);
    ASSERT(res == MMSYSERR_NOERROR);
     // clear buffers
    for( INDEX iBuffer = 0; iBuffer<sl_awhWOBuffers.Count(); iBuffer++) {
      res = waveOutUnprepareHeader( sl_hwoWaveOut, &sl_awhWOBuffers[iBuffer],
                                    sizeof(sl_awhWOBuffers[iBuffer]));
      ASSERT(res == MMSYSERR_NOERROR);
    }
    sl_awhWOBuffers.Clear();
    // close waveout device
    res = waveOutClose( sl_hwoWaveOut);
    ASSERT(res == MMSYSERR_NOERROR);
    sl_hwoWaveOut = NULL;
  }

  // for each extra taken channel
  for(INDEX iChannel=0; iChannel<sl_ahwoExtra.Count(); iChannel++) {
    // close its device
    MMRESULT res = waveOutClose( sl_ahwoExtra[iChannel]);
    ASSERT(res == MMSYSERR_NOERROR);
  }
  // free extra channel handles
  sl_ahwoExtra.PopAll();

#else
  ShutDown_SDLaudio(*this);
#endif

  // free memory
  if( sl_pslMixerBuffer!=NULL) {
    FreeMemory( sl_pslMixerBuffer);
    sl_pslMixerBuffer = NULL;
  }
  if( sl_pswDecodeBuffer!=NULL) {
    FreeMemory( sl_pswDecodeBuffer);
    sl_pswDecodeBuffer = NULL;
  }
  if( sl_pubBuffersMemory!=NULL) {
    FreeMemory( sl_pubBuffersMemory);
    sl_pubBuffersMemory = NULL;
  }
}


// set listener enviroment properties (EAX)
BOOL CSoundLibrary::SetEnvironment( INDEX iEnvNo, FLOAT fEnvSize/*=0*/)
{
  if( !sl_bUsingEAX) return FALSE;
#ifdef PLATFORM_WIN32
  // trim values
  if( iEnvNo<0   || iEnvNo>25)   iEnvNo=1;
  if( fEnvSize<1 || fEnvSize>99) fEnvSize=8;
  HRESULT hResult;
  hResult = sl_pKSProperty->Set( DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT, NULL, 0, &iEnvNo, sizeof(DWORD));
  if( hResult != DS_OK) return DSFail( *this, TRANS("  ! EAX error: Cannot set environment.\n"));
  hResult = sl_pKSProperty->Set( DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE, NULL, 0, &fEnvSize, sizeof(FLOAT));
  if( hResult != DS_OK) return DSFail( *this, TRANS("  ! EAX error: Cannot set environment size.\n"));
#endif
  return TRUE;
}


// mute all sounds (erase playing buffer(s) and supress mixer)
void CSoundLibrary::Mute(void)
{
  ASSERT(this!=NULL);
  // stop all IFeel effects
  IFeel_StopEffect(NULL);

// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

#ifdef PLATFORM_WIN32
  // erase direct sound buffer (waveout will shut-up by itself), but skip if there's no more sound library
  if( this==NULL || !sl_bUsingDirectSound) return;

  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // supress future mixing and erase sound buffer
  _bMuted = TRUE;
  static LPVOID lpData;
  static DWORD  dwSize;

  // flush one secondary buffer
  if( !DSLockBuffer( *this, sl_pDSSecondary, sl_slMixerBufferSize, lpData, dwSize)) return;
  memset( lpData, 0, dwSize);
  sl_pDSSecondary->Unlock( lpData, dwSize, NULL, 0);
  // if EAX is in use
  if( sl_bUsingEAX) {
    // flush right buffer, too
    if( !DSLockBuffer( *this, sl_pDSSecondary2, sl_slMixerBufferSize, lpData, dwSize)) return;
    memset( lpData, 0, dwSize);
    sl_pDSSecondary2->Unlock( lpData, dwSize, NULL, 0);
  } 

#else
  SDL_LockAudioDevice(sdl_audio_device);
  _bMuted = TRUE;
  sdl_backbuffer_remain = 0;  // ditch pending audio data...
  sdl_backbuffer_pos = 0;
  SDL_UnlockAudioDevice(sdl_audio_device);
#endif
}





/*
 * set sound format
 */
CSoundLibrary::SoundFormat CSoundLibrary::SetFormat( CSoundLibrary::SoundFormat EsfNew, BOOL bReport/*=FALSE*/)
{
// !!! FIXME : rcg12162001 Do this for all platforms?
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer) {
    sl_EsfFormat = SF_NONE;
    return(sl_EsfFormat);
  }
#endif

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // pause playing all sounds
  {FOREACHINLIST( CSoundData, sd_Node, sl_ClhAwareList, itCsdStop) {
    itCsdStop->PausePlayingObjects();
  }}

  // change format and keep console variable states
  SetFormat_internal( *this, EsfNew, bReport);
  _tmLastMixAhead = snd_tmMixAhead;
  _iLastFormat = snd_iFormat;
  _iLastDevice = snd_iDevice;
  _iLastAPI = snd_iInterface;

  // continue playing all sounds
  CListHead lhToReload;
  lhToReload.MoveList(sl_ClhAwareList);
  {FORDELETELIST( CSoundData, sd_Node, lhToReload, itCsdContinue) {
    CSoundData &sd = *itCsdContinue;
    if( !(sd.sd_ulFlags&SDF_ENCODED)) {
      sd.Reload();
    } else {
      sd.sd_Node.Remove();
      sl_ClhAwareList.AddTail(sd.sd_Node);
    }
    sd.ResumePlayingObjects();
  }}

  // done
  return sl_EsfFormat;
}



/* Update all 3d effects and copy internal data. */
void CSoundLibrary::UpdateSounds(void)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // see if we have valid handle for direct sound and eventually reinit sound
#ifdef PLATFORM_WIN32
  if( sl_bUsingDirectSound && _hwndCurrent!=_hwndMain) {
    _hwndCurrent = _hwndMain;
    SetFormat( sl_EsfFormat);
  }
#endif

  _bMuted = FALSE; // enable mixer
  _sfStats.StartTimer(CStatForm::STI_SOUNDUPDATE);
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_UPDATESOUNDS);

  // synchronize access to sounds
  CTSingleLock slSounds( &sl_csSound, TRUE);

#ifdef PLATFORM_WIN32
  // make sure that the buffers are playing
  if( sl_bUsingDirectSound) DSPlayBuffers(*this);
#endif

  // determine number of listeners and get listener
  INDEX ctListeners=0;
  CSoundListener *sli;
  {FOREACHINLIST( CSoundListener, sli_lnInActiveListeners, _pSound->sl_lhActiveListeners, itsli) {
    sli = itsli;
    ctListeners++;
  }}
  // if there's only one listener environment properties have been changed (in split-screen EAX is not supported)
  if( ctListeners==1 && (_iLastEnvType!=sli->sli_iEnvironmentType || _fLastEnvSize!=sli->sli_fEnvironmentSize)) {
    // keep new properties and eventually update environment (EAX)
    _iLastEnvType = sli->sli_iEnvironmentType;
    _fLastEnvSize = sli->sli_fEnvironmentSize;
    SetEnvironment( _iLastEnvType, _fLastEnvSize);
  }
  // if there are no listeners - reset environment properties
  if( ctListeners<1 && (_iLastEnvType!=1 || _fLastEnvSize!=1.4f)) {
    // keep new properties and update environment
    _iLastEnvType = 1;
    _fLastEnvSize = 1.4f;
    SetEnvironment( _iLastEnvType, _fLastEnvSize);
  }

  // adjust panning if needed
#ifdef PLATFORM_WIN32
  snd_fEAXPanning = Clamp( snd_fEAXPanning, -1.0f, +1.0f);
  if( sl_bUsingEAX && _fLastPanning!=snd_fEAXPanning)
  { // determine new panning
    _fLastPanning = snd_fEAXPanning;
    FLOAT fPanLeft  = -1.0f;
    FLOAT fPanRight = +1.0f;
    if( snd_fEAXPanning<0) fPanRight = MINPAN + Abs(snd_fEAXPanning)*MAXPAN;  // pan left
    if( snd_fEAXPanning>0) fPanLeft  = MINPAN + Abs(snd_fEAXPanning)*MAXPAN;  // pan right
    // set and apply
    HRESULT hr1,hr2,hr3;
    hr1 = sl_pDSSourceLeft->SetPosition(  fPanLeft, 0,0, DS3D_DEFERRED);
    hr2 = sl_pDSSourceRight->SetPosition( fPanRight,0,0, DS3D_DEFERRED);
    hr3 = sl_pDSListener->CommitDeferredSettings();
    if( hr1!=DS_OK || hr2!=DS_OK || hr3!=DS_OK) DSFail( *this, TRANS("  ! DirectSound3D error: Cannot set 3D position.\n"));
  }
#endif

  // for each sound
  {FOREACHINLIST( CSoundData, sd_Node, sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST( CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      _sfStats.IncrementCounter(CStatForm::SCI_SOUNDSACTIVE);
      itCsoSoundObject->Update3DEffects();
    }
  }}

  // for each sound
  {FOREACHINLIST( CSoundData, sd_Node, sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST( CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      CSoundObject &so = *itCsoSoundObject;
      // if sound is playing
      if( so.so_slFlags&SOF_PLAY) {
        // copy parameters
        so.so_sp = so.so_spNew;
        // prepare sound if not prepared already
        if ( !(so.so_slFlags&SOF_PREPARE)) {
          so.PrepareSound();
          so.so_slFlags |= SOF_PREPARE;
        }
      // if it is not playing
      } else {
        // remove it from list
        so.so_Node.Remove();
      }
    }
  }}

  // remove all listeners
  {FORDELETELIST( CSoundListener, sli_lnInActiveListeners, sl_lhActiveListeners, itsli) {
    itsli->sli_lnInActiveListeners.Remove();
  }}

  _pfSoundProfile.StopTimer(CSoundProfile::PTI_UPDATESOUNDS);
  _sfStats.StopTimer(CStatForm::STI_SOUNDUPDATE);
}


/*
 * This is called every TickQuantum seconds.
 */
void CSoundTimerHandler::HandleTimer(void)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  /* memory leak checking routines
  ASSERT( _CrtCheckMemory());
  ASSERT( _CrtIsMemoryBlock( (void*)_pSound->sl_pswDecodeBuffer,
                             (ULONG)_pSound->sl_slDecodeBufferSize, NULL, NULL, NULL));
  ASSERT( _CrtIsValidPointer( (void*)_pSound->sl_pswDecodeBuffer,
                              (ULONG)_pSound->sl_slDecodeBufferSize, TRUE)); */
  // mix all needed sounds
  _pSound->MixSounds();
}



/*
 *  MIXER helper functions
 */


// copying of mixer buffer to sound buffer(s)

#ifdef PLATFORM_WIN32
static LPVOID _lpData, _lpData2;
static DWORD  _dwSize, _dwSize2;

static void CopyMixerBuffer_dsound( CSoundLibrary &sl, SLONG slMixedSize)
{
  LPVOID lpData;
  DWORD dwSize;
  SLONG slPart1Size, slPart2Size;

  // if EAX is in use
  if( sl.sl_bUsingEAX)
  {
    // lock left buffer and copy first part of 1st mono block
    if( !DSLockBuffer( sl, sl.sl_pDSSecondary, sl.sl_slMixerBufferSize, lpData, dwSize)) return;
    slPart1Size = Min( sl.sl_slMixerBufferSize-_iWriteOffset, slMixedSize);
    CopyMixerBuffer_mono( 0, ((UBYTE*)lpData)+_iWriteOffset/2, slPart1Size);
    // copy second part of 1st mono block
    slPart2Size = slMixedSize - slPart1Size;
    CopyMixerBuffer_mono( slPart1Size, lpData, slPart2Size);
    _iWriteOffset += slMixedSize;
    if( _iWriteOffset>=sl.sl_slMixerBufferSize) _iWriteOffset -= sl.sl_slMixerBufferSize;
    ASSERT( _iWriteOffset>=0 && _iWriteOffset<sl.sl_slMixerBufferSize);
    sl.sl_pDSSecondary->Unlock( lpData, dwSize, NULL, 0); 

    // lock right buffer and copy first part of 2nd mono block
    if( !DSLockBuffer( sl, sl.sl_pDSSecondary2, sl.sl_slMixerBufferSize, lpData, dwSize)) return;
    slPart1Size = Min( sl.sl_slMixerBufferSize-_iWriteOffset2, slMixedSize);
    CopyMixerBuffer_mono( 2, ((UBYTE*)lpData)+_iWriteOffset2/2, slPart1Size);
    // copy second part of 2nd mono block
    slPart2Size = slMixedSize - slPart1Size;
    CopyMixerBuffer_mono( slPart1Size+2, lpData, slPart2Size);
    _iWriteOffset2 += slMixedSize;
    if( _iWriteOffset2>=sl.sl_slMixerBufferSize) _iWriteOffset2 -= sl.sl_slMixerBufferSize;
    ASSERT( _iWriteOffset2>=0 && _iWriteOffset2<sl.sl_slMixerBufferSize);
    sl.sl_pDSSecondary2->Unlock( lpData, dwSize, NULL, 0);
  }
  // if only standard DSound (no EAX)
  else
  {
    // lock stereo buffer and copy first part of block
    if( !DSLockBuffer( sl, sl.sl_pDSSecondary, sl.sl_slMixerBufferSize, lpData, dwSize)) return;
    slPart1Size = Min( sl.sl_slMixerBufferSize-_iWriteOffset, slMixedSize);
    CopyMixerBuffer_stereo( 0, ((UBYTE*)lpData)+_iWriteOffset, slPart1Size);
    // copy second part of block
    slPart2Size = slMixedSize - slPart1Size;
    CopyMixerBuffer_stereo( slPart1Size, lpData, slPart2Size);
    _iWriteOffset += slMixedSize;
    if( _iWriteOffset>=sl.sl_slMixerBufferSize) _iWriteOffset -= sl.sl_slMixerBufferSize;
    ASSERT( _iWriteOffset>=0 && _iWriteOffset<sl.sl_slMixerBufferSize);
    sl.sl_pDSSecondary->Unlock( lpData, dwSize, NULL, 0);
  }
}


static void CopyMixerBuffer_waveout( CSoundLibrary &sl)
{
  MMRESULT res;
  SLONG slOffset = 0;
  for( INDEX iBuffer = 0; iBuffer<sl.sl_awhWOBuffers.Count(); iBuffer++)
  { // skip prepared buffer
    WAVEHDR &wh = sl.sl_awhWOBuffers[iBuffer];
    if( wh.dwFlags&WHDR_PREPARED) continue;
    // copy part of a mixer buffer to wave buffer
    CopyMixerBuffer_stereo( slOffset, wh.lpData, WAVEOUTBLOCKSIZE);
    slOffset += WAVEOUTBLOCKSIZE;
    // write wave buffer (ready for playing)
    res = waveOutPrepareHeader( sl.sl_hwoWaveOut, &wh, sizeof(wh));
    ASSERT( res==MMSYSERR_NOERROR);
    res = waveOutWrite( sl.sl_hwoWaveOut, &wh, sizeof(wh));
    ASSERT( res==MMSYSERR_NOERROR);
  }
}


// finds room in sound buffer to copy in next crop of samples
static SLONG PrepareSoundBuffer_dsound( CSoundLibrary &sl)
{
  // determine writable block size (difference between write and play pointers)
  HRESULT hr1,hr2;
  DWORD dwCurrentCursor, dwCurrentCursor2;
  SLONG slDataToMix;
  ASSERT( sl.sl_pDSSecondary!=NULL && sl.sl_pDSPrimary!=NULL);

  // if EAX is in use
  if( sl.sl_bUsingEAX)
  {
    hr1 = sl.sl_pDSSecondary->GetCurrentPosition(  &dwCurrentCursor,  NULL);
    hr2 = sl.sl_pDSSecondary2->GetCurrentPosition( &dwCurrentCursor2, NULL);
    if( hr1!=DS_OK || hr2!=DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot obtain sound buffer write position.\n"));
    dwCurrentCursor *=2; // stereo mixer
    dwCurrentCursor2*=2; // stereo mixer
    // store pointers and wrapped block sizes
    SLONG slDataToMix1 = dwCurrentCursor - _iWriteOffset;
    if( slDataToMix1<0) slDataToMix1 += sl.sl_slMixerBufferSize;
    ASSERT( slDataToMix1>=0 && slDataToMix1<=sl.sl_slMixerBufferSize);
    slDataToMix1 = Min( slDataToMix1, sl.sl_slMixerBufferSize); 
    SLONG slDataToMix2 = dwCurrentCursor2 - _iWriteOffset2;
    if( slDataToMix2<0) slDataToMix2 += sl.sl_slMixerBufferSize;
    ASSERT( slDataToMix2>=0 && slDataToMix2<=sl.sl_slMixerBufferSize);
    slDataToMix = Min( slDataToMix1, slDataToMix2);
  }
  // if only standard DSound (no EAX)
  else
  {
    hr1 = sl.sl_pDSSecondary->GetCurrentPosition( &dwCurrentCursor, NULL);
    if( hr1!=DS_OK) return DSFail( sl, TRANS("  ! DirectSound error: Cannot obtain sound buffer write position.\n"));
    // store pointer and wrapped block size
    slDataToMix = dwCurrentCursor - _iWriteOffset;
    if( slDataToMix<0) slDataToMix += sl.sl_slMixerBufferSize;
    ASSERT( slDataToMix>=0 && slDataToMix<=sl.sl_slMixerBufferSize);
    slDataToMix = Min( slDataToMix, sl.sl_slMixerBufferSize); 
  }

  // done
  //CPrintF( "LP/LW: %5d / %5d,   RP/RW: %5d / %5d,    MIX: %5d\n", dwCurrentCursor, _iWriteOffset, dwCurrentCursor2, _iWriteOffset2, slDataToMix); // grgr
  return slDataToMix;
}


static SLONG PrepareSoundBuffer_waveout( CSoundLibrary &sl)
{
  // scan waveout buffers to find all that are ready to receive sound data (i.e. not playing)
  SLONG slDataToMix=0;
  for( INDEX iBuffer=0; iBuffer<sl.sl_awhWOBuffers.Count(); iBuffer++)
  { // if done playing
    WAVEHDR &wh = sl.sl_awhWOBuffers[iBuffer];
    if( wh.dwFlags&WHDR_DONE) {
      // unprepare buffer
      MMRESULT res = waveOutUnprepareHeader( sl.sl_hwoWaveOut, &wh, sizeof(wh));
      ASSERT( res == MMSYSERR_NOERROR);
    }
    // if unprepared
    if( !(wh.dwFlags&WHDR_PREPARED)) {
      // increase mix-in data size
      slDataToMix += WAVEOUTBLOCKSIZE;
    }
  }
  // done
  ASSERT( slDataToMix <= sl.sl_slMixerBufferSize);
  return slDataToMix;
}
#endif

  
/* Update Mixer */
void CSoundLibrary::MixSounds(void)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // synchronize access to sounds
  CTSingleLock slSounds( &sl_csSound, TRUE);

  // do nothing if no sound
  if( sl_EsfFormat==SF_NONE || _bMuted) return;

  _sfStats.StartTimer(CStatForm::STI_SOUNDMIXING);
  _pfSoundProfile.IncrementAveragingCounter();
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_MIXSOUNDS);

  // seek available buffer(s) for next crop of samples
  SLONG slDataToMix;

#ifdef PLATFORM_WIN32
  if( sl_bUsingDirectSound) { // using direct sound
    slDataToMix = PrepareSoundBuffer_dsound( *this);
  } else { // using wave out 
    slDataToMix = PrepareSoundBuffer_waveout(*this);
  }
#else
  SDL_LockAudioDevice(sdl_audio_device);
  slDataToMix = PrepareSoundBuffer_SDLaudio(*this);
#endif

  // skip mixing if all sound buffers are still busy playing
  ASSERT( slDataToMix>=0);
  if( slDataToMix<=0) {
    #ifdef PLATFORM_UNIX
      SDL_UnlockAudioDevice(sdl_audio_device);
    #endif
    _pfSoundProfile.StopTimer(CSoundProfile::PTI_MIXSOUNDS);
    _sfStats.StopTimer(CStatForm::STI_SOUNDMIXING);
    return;
  }

  // prepare mixer buffer
  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_MIXINGS, 1);
  ResetMixer( sl_pslMixerBuffer, slDataToMix);

  BOOL bGamePaused = _pNetwork->IsPaused() || (_pNetwork->IsServer() && _pNetwork->GetLocalPause());

  // for each sound
  FOREACHINLIST( CSoundData, sd_Node, sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST( CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      CSoundObject &so = *itCsoSoundObject;
      // if the sound is in-game sound, and the game paused
      if (!(so.so_slFlags&SOF_NONGAME) && bGamePaused) {
        // don't mix it it
        continue;
      }
      // if sound is prepared and playing
      if( so.so_slFlags&SOF_PLAY && 
          so.so_slFlags&SOF_PREPARE &&
        !(so.so_slFlags&SOF_PAUSED)) {
        // mix it
        MixSound(&so);
      }
    }
  }

  // eventually normalize mixed sounds
  snd_fNormalizer = Clamp( snd_fNormalizer, 0.0f, 1.0f);
  NormalizeMixerBuffer( snd_fNormalizer, slDataToMix, _fLastNormalizeValue);

  // write mixer buffer to file
  // if( !_bOpened) _filMixerBuffer = fopen( "d:\\MixerBufferDump.raw", "wb");
  // fwrite( (void*)sl_pslMixerBuffer, 1, slDataToMix, _filMixerBuffer);
  // _bOpened = TRUE;

  // copy mixer buffer to buffers buffer(s)
#ifdef PLATFORM_WIN32
  if( sl_bUsingDirectSound) { // using direct sound
    CopyMixerBuffer_dsound( *this, slDataToMix);
  } else { // using wave out 
    CopyMixerBuffer_waveout(*this);
  }
#else
  CopyMixerBuffer_SDLaudio(*this, slDataToMix);
  SDL_UnlockAudioDevice(sdl_audio_device);
#endif

  // all done
  _pfSoundProfile.StopTimer(CSoundProfile::PTI_MIXSOUNDS);
  _sfStats.StopTimer(CStatForm::STI_SOUNDMIXING);
}


//
//  Sound mode awareness functions
//

/*
 *  Add sound in sound aware list
 */
void CSoundLibrary::AddSoundAware(CSoundData &CsdAdd)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // add sound to list tail
  sl_ClhAwareList.AddTail(CsdAdd.sd_Node);
};

/*
 *  Remove a display mode aware object.
 */
void CSoundLibrary::RemoveSoundAware(CSoundData &CsdRemove)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // remove it from list
  CsdRemove.sd_Node.Remove();
};

// listen from this listener this frame
void CSoundLibrary::Listen(CSoundListener &sl)
{
// !!! FIXME : rcg12162001 This should probably be done everywhere, honestly.
#ifdef PLATFORM_UNIX
  if (_bDedicatedServer)
    return;
#endif

  // just add it to list
  if (sl.sli_lnInActiveListeners.IsLinked()) {
    sl.sli_lnInActiveListeners.Remove();
  }
  sl_lhActiveListeners.AddTail(sl.sli_lnInActiveListeners);
}
#endif
