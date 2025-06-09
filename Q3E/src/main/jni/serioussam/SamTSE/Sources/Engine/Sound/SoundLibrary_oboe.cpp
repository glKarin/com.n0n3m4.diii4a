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

#include "Engine/StdH.h"


// !!! FIXME : rcg12162001 This file really needs to be ripped apart and
// !!! FIXME : rcg12162001  into platform/driver specific subdirectories.


// !!! FIXME : rcg10132001 what is this file?

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

#include "oboeaudio/snd_oboe.h"
#define SDL_LockAudioDevice(x) Q3E_Oboe_Lock()
#define SDL_UnlockAudioDevice(x) Q3E_Oboe_Unlock()
inline void SDL_PauseAudioDevice(int d, int paused)
{
	if(paused)
		Q3E_Oboe_Stop();
	else
		Q3E_Oboe_Start();
}
typedef uint8_t Uint8;
typedef int16_t Sint16;
typedef int SDL_AudioDeviceID;

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

static CTString snd_strDeviceName;

static BOOL  _bMuted  = FALSE;
static INDEX _iLastEnvType = 1234;
static FLOAT _fLastEnvSize = 1234;

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

static void sdl_audio_callback(Uint8 *stream, int len)
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
#if 0
	short *bbb = (short*)src;
  printf("\n=====================\n");
  for(int i = 0; i < cpysize/2; i++)
  {
	  printf("%d ", bbb[i]);
  }
  printf("\n\n");
#endif
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

  int format;
  Sint16 bps = sl.sl_SwfeFormat.wBitsPerSample;
  if (bps <= 8)
  {
    //format = AUDIO_U8; // not support
	format = Q3E_OBOE_FORMAT_SINT16;
	sl.sl_SwfeFormat.wBitsPerSample = 16;
  }
  else if (bps <= 16)
  {
    //format = AUDIO_S16LSB;
    format = Q3E_OBOE_FORMAT_SINT16;
  }
  else if (bps <= 32)
  {
    //format = AUDIO_S32LSB;
    //format = Q3E_OBOE_FORMAT_SINT32; // require API 31
	format = Q3E_OBOE_FORMAT_SINT16;
	sl.sl_SwfeFormat.wBitsPerSample = 16;
  }
  else {
    CPrintF(TRANSV("Unsupported bits-per-sample: %d\n"), bps);
	format = Q3E_OBOE_FORMAT_SINT16;
	sl.sl_SwfeFormat.wBitsPerSample = 16;
    //return FALSE;
  }
  int freq = sl.sl_SwfeFormat.nSamplesPerSec;
  int samples;

    // I dunno if this is the best idea, but I'll give it a try...
    //  should probably check a cvar for this...
	int rate;
  if (freq <= 11025)
  {
    samples = 512;
	rate = 11025;
  }
  else if (freq <= 22050)
  {
    samples = 1024;
	rate = 22050;
  }
  else if (freq <= 44100)
  {
    samples = 2048;
	rate = 44100;
  }
  else
  {
    samples = 4096;  // (*shrug*)
	rate = 44100;
  }

  int channels = sl.sl_SwfeFormat.nChannels;

  // !!! FIXME rcg12162001 We force SDL to convert the audio stream on the
  // !!! FIXME rcg12162001  fly to match sl.sl_SwfeFormat, but I'm curious
  // !!! FIXME rcg12162001  if the Serious Engine can handle it if we changed
  // !!! FIXME rcg12162001  sl.sl_SwfeFormat to match what the audio hardware
  // !!! FIXME rcg12162001  can handle. I'll have to check later.
  CPrintF("Q3E Oboe audio initialized.\n");
  Q3E_Oboe_Init(rate, channels, format, sdl_audio_callback);

  int size = samples * channels * (16/8); //TODO: S16
  sdl_backbuffer_allocation = (size * 4);
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
             "Oboe");
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
    CPrintF(TRANSV("  output buffers: %d x %d bytes\n"), 2, size);
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
	CPrintF("Closing Q3E audio audio device...\n");
	Q3E_Oboe_Shutdown();

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

  CPrintF("Q3E Oboe audio shutdown.\n");
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
  bSoundOK = StartUp_SDLaudio(sl, bReport);

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

  ShutDown_SDLaudio(*this);

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

  SDL_LockAudioDevice(sdl_audio_device);
  _bMuted = TRUE;
  sdl_backbuffer_remain = 0;  // ditch pending audio data...
  sdl_backbuffer_pos = 0;
  SDL_UnlockAudioDevice(sdl_audio_device);
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

  _bMuted = FALSE; // enable mixer
  _sfStats.StartTimer(CStatForm::STI_SOUNDUPDATE);
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_UPDATESOUNDS);

  // synchronize access to sounds
  CTSingleLock slSounds( &sl_csSound, TRUE);


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

  SDL_LockAudioDevice(sdl_audio_device);
  slDataToMix = PrepareSoundBuffer_SDLaudio(*this);

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
  CopyMixerBuffer_SDLaudio(*this, slDataToMix);
  SDL_UnlockAudioDevice(sdl_audio_device);

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
