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

#include <Engine/StdH.h>

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

#include <stdint.h>
#include <vector>
#include <math.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// pointer to global sound library object
CSoundLibrary *_pSound = NULL;

// console variables
FLOAT snd_tmMixAhead = 0.2f; // mix-ahead in seconds
FLOAT snd_fSoundVolume = 1.0f;   // master volume for sound playing [0..1]
FLOAT snd_fMusicVolume = 1.0f;   // master volume for music playing [0..1]
// NOTES: 
// - these 3d sound parameters have been set carefully, take extreme if changing !
// - ears distance of 20cm causes phase shift of up to 0.6ms which is very noticable
//   and is more than enough, too large values cause too much distorsions in other effects
// - pan strength needs not to be very strong, since lrfilter has panning-like influence also
// - if down filter is too large, it makes too much influence even on small elevation changes
//   and messes the situation completely
FLOAT snd_fDelaySoundSpeed = 1E10;   // sound speed used for delay [m/s]
FLOAT snd_fDopplerSoundSpeed = 330.0f; // sound speed used for doppler [m/s]
FLOAT snd_fEarsDistance = 0.2f;   // distance between listener's ears
FLOAT snd_fPanStrength = 0.1f;   // panning modifier (0=none, 1= full)
FLOAT snd_fLRFilter = 3.0f;   // filter for left-right
FLOAT snd_fBFilter = 5.0f;   // filter for back
FLOAT snd_fUFilter = 1.0f;   // filter for up
FLOAT snd_fDFilter = 3.0f;   // filter for down

ENGINE_API INDEX snd_iFormat = 3;
INDEX snd_bMono = FALSE;
static INDEX snd_iDevice = -1;
static INDEX snd_iInterface = 2;   // 0=WaveOut, 1=DirectSound, 2=EAX
static INDEX snd_iMaxOpenRetries = 3;
static INDEX snd_iMaxExtraChannels = 32;
static FLOAT snd_tmOpenFailDelay = 0.5f;
static FLOAT snd_fEAXPanning = 0.0f;

static FLOAT snd_fNormalizer = 0.9f;
static FLOAT _fLastNormalizeValue = 1;

extern HWND _hwndMain; // global handle for application window
static HWND _hwndCurrent = NULL;
static HINSTANCE _hInstDS = NULL;
static INDEX _iWriteOffset = 0;
static INDEX _iWriteOffset2 = 0;
static BOOL _bMuted = FALSE;
static INDEX _iLastEnvType = 1234;
static FLOAT _fLastEnvSize = 1234;
static FLOAT _fLastPanning = 1234;


// TEMP! - for writing mixer buffer to file
static FILE *_filMixerBuffer;
static BOOL _bOpened = FALSE;


#define WAVEOUTBLOCKSIZE 1024
#define MINPAN (1.0f)
#define MAXPAN (9.0f)

// OpenSL static data
struct AudioEngineData {
    SLObjectItf slEngineObj_;
    SLEngineItf slEngineItf_;
    SLObjectItf outputMixObjectItf_;
    SLObjectItf playerObjectItf_;
    SLBufferQueueItf playBufferQueueItf_;
    SLPlayItf playItf_;

    // buffers
    int numBuffers = 2;
    int whichBuffer;
    int framesPerBuffer;
    std::vector<int16_t> buffer;
};
AudioEngineData engine;

static ULONG getFramesPerSec(CSoundLibrary::SoundFormat sl_EsfFormat) {
  switch (sl_EsfFormat) {
    case CSoundLibrary::SF_11025_16:
      return 11025;
    case CSoundLibrary::SF_22050_16:
      return 22050;
    case CSoundLibrary::SF_44100_16:
      return 44100;
    case CSoundLibrary::SF_NONE:
      return 0;
    default:
      ASSERTALWAYS("Unknown Sound format");
      break;
  }
  return 0;
};

/**
 * ----------------------------
 *    Sound Library functions
 * ----------------------------
**/

/*
 *  Construct uninitialized sound library.
 */
CSoundLibrary::CSoundLibrary(void) {
  sl_csSound.cs_iIndex = 3000;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear sound format
  memset( &sl_SwfeFormat, 0, sizeof(WAVEFORMATEX));
  sl_EsfFormat = SF_NONE;

  sl_pslMixerBuffer = NULL;
  sl_pswDecodeBuffer = NULL;
  sl_pubBuffersMemory = NULL;

  sl_bUsingDirectSound = FALSE;
  sl_bUsingEAX = FALSE;
}


/*
 *  Destruct (and clean up).
 */
CSoundLibrary::~CSoundLibrary(void) {
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // clear sound enviroment
  Clear();

  // clear any installed sound decoders
  CSoundDecoder::EndPlugins();
}

static void SndPostFunc(void *pArgs)
{
}

/*
 *  Initialization
 */
void CSoundLibrary::Init(void) {
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);
  
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fSoundVolume;", &snd_fSoundVolume);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fMusicVolume;", &snd_fMusicVolume);
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
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fNormalizer;",  (void *) &snd_fNormalizer);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_tmMixAhead post:SndPostFunc;", (void *) &snd_tmMixAhead);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iInterface post:SndPostFunc;", (void *) &snd_iInterface);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iDevice post:SndPostFunc;", (void *) &snd_iDevice);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iFormat post:SndPostFunc;", (void *) &snd_iFormat);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iMaxExtraChannels;", (void *) &snd_iMaxExtraChannels);
  _pShell->DeclareSymbol( "persistent user INDEX snd_iMaxOpenRetries;",   (void *) &snd_iMaxOpenRetries);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_tmOpenFailDelay;",   (void *) &snd_tmOpenFailDelay);
  _pShell->DeclareSymbol( "persistent user FLOAT snd_fEAXPanning;", (void *) &snd_fEAXPanning);
  
#if 0
  _pShell->DeclareSymbol( "persistent user CTString snd_strDeviceName;", (void *) &snd_strDeviceName);
#endif

  // print header
  CPrintF(TRANS("Initializing sound...\n"));

  // initialize sound library and set no-sound format
  SetFormat(SF_NONE);

  // initialize any installed sound decoders
  CSoundDecoder::InitPlugins();

  sl_ctWaveDevices = 0;  // rcg11012005 valgrind fix.

  if (!SetFormat(SF_44100_16)) {
    CPrintF("  disabling audio.\n");
    return;
  }

  CPrintF("  audio initialized.\n");
}


/*
 *  Clear Sound Library
 */
void CSoundLibrary::Clear(void) {
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
void CSoundLibrary::ClearLibrary(void) {
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);

  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  // remove timer handler if added
  if (sl_thTimerHandler.th_Node.IsLinked()) {
    _pTimer->RemHandler(&sl_thTimerHandler);
  }

  // close audio output
  sl_bUsingDirectSound = FALSE;
  sl_bUsingEAX = FALSE;

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
  return FALSE;
}

// mute all sounds (erase playing buffer(s) and supress mixer)
void CSoundLibrary::Mute(void) {
	_bMuted = TRUE;
}

const char *getSlError(SLresult res) {
#define CASE(msg) case msg: return #msg;
  switch(res) {
    CASE(SL_RESULT_SUCCESS)
    CASE(SL_RESULT_PRECONDITIONS_VIOLATED)
    CASE(SL_RESULT_PARAMETER_INVALID)
    CASE(SL_RESULT_MEMORY_FAILURE)
    CASE(SL_RESULT_RESOURCE_ERROR)
    CASE(SL_RESULT_RESOURCE_LOST)
    CASE(SL_RESULT_IO_ERROR)
    CASE(SL_RESULT_BUFFER_INSUFFICIENT)
    CASE(SL_RESULT_CONTENT_CORRUPTED)
    CASE(SL_RESULT_CONTENT_UNSUPPORTED)
    CASE(SL_RESULT_CONTENT_NOT_FOUND)
    CASE(SL_RESULT_PERMISSION_DENIED)
    CASE(SL_RESULT_FEATURE_UNSUPPORTED)
    CASE(SL_RESULT_INTERNAL_ERROR)
    CASE(SL_RESULT_UNKNOWN_ERROR)
    CASE(SL_RESULT_OPERATION_ABORTED)
    CASE(SL_RESULT_CONTROL_LOST)
    default: return "Unknown";
  }
#undef CASE
}

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
 * set sound format
 */
CSoundLibrary::SoundFormat CSoundLibrary::SetFormat(CSoundLibrary::SoundFormat EsfNew, BOOL bReport/*=FALSE*/) {
#define PROCESS_RES(msg) if (res != SL_RESULT_SUCCESS) { \
    CPrintF("  Cannot create OpenSL object: " msg " returned 0x%04X (%s)\n", res, getSlError(res)); \
    sl_EsfFormat = SF_NONE; \
    return sl_EsfFormat; \
  }

  if( bReport) CPrintF(TRANSV("Setting sound format ...\n"));

  // remove timer handler if added
  if (sl_thTimerHandler.th_Node.IsLinked()) {
    _pTimer->RemHandler(&sl_thTimerHandler);
  }

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);

  sl_EsfFormat = EsfNew;

  // pause playing all sounds
  FOREACHINLIST(CSoundData, sd_Node, sl_ClhAwareList, itCsdStop) {
    itCsdStop->PausePlayingObjects();
  }

  if( sl_EsfFormat == CSoundLibrary::SF_NONE) {
    if( bReport) CPrintF(TRANSV("  (no sound)\n"));
	return sl_EsfFormat;
  }

  memset(&engine, 0, sizeof(engine));

  SLresult res;
  res = slCreateEngine(&engine.slEngineObj_, 0, NULL, 0, NULL, NULL);
  PROCESS_RES("slCreateEngine()")

  res = (*engine.slEngineObj_)->Realize(engine.slEngineObj_, SL_BOOLEAN_FALSE);
  PROCESS_RES("sl->Realize()")

  res = (*engine.slEngineObj_)->GetInterface(engine.slEngineObj_, SL_IID_ENGINE, &engine.slEngineItf_);
  PROCESS_RES("sl->GetInterface(SL_IID_ENGINE)")

  // mix
  res = (*engine.slEngineItf_)->CreateOutputMix(engine.slEngineItf_, &engine.outputMixObjectItf_, 0, NULL, NULL);
  PROCESS_RES("engine->CreateOutputMix()")

  res = (*engine.outputMixObjectItf_)->Realize(engine.outputMixObjectItf_, SL_BOOLEAN_FALSE);
  PROCESS_RES("mixer->Realize(false)")

  // format
  SLDataFormat_PCM format_pcm;
  format_pcm.formatType = SL_DATAFORMAT_PCM;
  format_pcm.numChannels = 2;
  format_pcm.samplesPerSec = getFramesPerSec(sl_EsfFormat) * 1000;
  format_pcm.bitsPerSample = 16;
  format_pcm.containerSize = 16;
  format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
  format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

  SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, engine.outputMixObjectItf_};
  SLDataSink audioSnk = {&loc_outmix, NULL};

  SLDataLocator_BufferQueue loc_bufq = {SL_DATALOCATOR_BUFFERQUEUE, 2};
  SLDataSource audioSrc = {&loc_bufq, &format_pcm};

  // create player
  SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
  SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  res = (*engine.slEngineItf_)->CreateAudioPlayer(engine.slEngineItf_, &engine.playerObjectItf_,
                                                  &audioSrc, &audioSnk,
                                                  sizeof(ids) / sizeof(ids[0]), ids, req);
  PROCESS_RES("engine->CreateAudioPlayer()")

  res = (*engine.playerObjectItf_)->Realize(engine.playerObjectItf_, SL_BOOLEAN_FALSE);
  PROCESS_RES("playerObject->Realize(false)")

  // get buffer queue
  res = (*engine.playerObjectItf_)->GetInterface(engine.playerObjectItf_, SL_IID_BUFFERQUEUE,
                                                 &engine.playBufferQueueItf_);
  PROCESS_RES("player->GetInterface(SL_IID_BUFFERQUEUE)")

  // get the play interface
  res = (*engine.playerObjectItf_)->GetInterface(engine.playerObjectItf_, SL_IID_PLAY,
                                                 &engine.playItf_);
  PROCESS_RES("player->GetInterface(SL_IID_PLAY)");

  res = (*engine.playItf_)->SetPlayState(engine.playItf_, SL_PLAYSTATE_PLAYING);
  PROCESS_RES("play->SetPlayState(PLAYING)");

  // allocate buffers
  // numBuffers * framesPerBuffer * 2(channels) * 2(SoundMixer.cpp use 4 bytes for math) * 2(sizeof(short))
  engine.numBuffers = 2;
  engine.whichBuffer = 0;
  engine.framesPerBuffer = (int) (snd_tmMixAhead * getFramesPerSec(sl_EsfFormat));
  engine.buffer.resize(engine.numBuffers * engine.framesPerBuffer * 4);

  // create decoder buffer
  sl_slDecodeBufferSize = engine.buffer.size() * 5;
  sl_pswDecodeBuffer = (SWORD *) AllocMemory(sl_slDecodeBufferSize + 4); // (+4 because of linear interpolation of last samples)

  // add timer handler
  _pTimer->AddHandler(&sl_thTimerHandler);

  SetWaveFormat( EsfNew, sl_SwfeFormat);

  return sl_EsfFormat;

#undef PROCESS_RES
}

/* Update all 3d effects and copy internal data. */
void CSoundLibrary::UpdateSounds(void) {
  // see if we have valid handle for direct sound and eventually reinit sound

  _bMuted = FALSE; // enable mixer
  _sfStats.StartTimer(CStatForm::STI_SOUNDUPDATE);
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_UPDATESOUNDS);

  // synchronize access to sounds
  CTSingleLock slSounds(&sl_csSound, TRUE);


  // determine number of listeners and get listener
  INDEX ctListeners = 0;
  CSoundListener *sli;
  {FOREACHINLIST(CSoundListener, sli_lnInActiveListeners, _pSound->sl_lhActiveListeners, itsli) {
    sli = itsli;
    ctListeners++;
  }}
#if 0
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
#endif

  // for each sound
  FOREACHINLIST(CSoundData, sd_Node, sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST(CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      _sfStats.IncrementCounter(CStatForm::SCI_SOUNDSACTIVE);
      itCsoSoundObject->Update3DEffects();
    }
  }

  // for each sound
  FOREACHINLIST(CSoundData, sd_Node, sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST(CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      CSoundObject &so = *itCsoSoundObject;
      // if sound is playing
      if (so.so_slFlags & SOF_PLAY) {
        // copy parameters
        so.so_sp = so.so_spNew;
        // prepare sound if not prepared already
        if (!(so.so_slFlags & SOF_PREPARE)) {
          so.PrepareSound();
          so.so_slFlags |= SOF_PREPARE;
        }
        // if it is not playing
      } else {
        // remove it from list
        so.so_Node.Remove();
      }
    }
  }

  // remove all listeners
  {FORDELETELIST(CSoundListener, sli_lnInActiveListeners, sl_lhActiveListeners, itsli) {
    itsli->sli_lnInActiveListeners.Remove();
  }}

  _pfSoundProfile.StopTimer(CSoundProfile::PTI_UPDATESOUNDS);
  _sfStats.StopTimer(CStatForm::STI_SOUNDUPDATE);
}


/*
 * This is called every TickQuantum seconds.
 */
void CSoundTimerHandler::HandleTimer(void) {
  _pSound->MixSounds();
}

/* Update Mixer */
void CSoundLibrary::MixSounds(void) {
  // synchronize access to sounds
  //CTSingleLock slSounds( &sl_csSound, TRUE);

  // do nothing if no sound
  if( sl_EsfFormat==SF_NONE || _bMuted) return;

  SLBufferQueueState state;
  SLresult res = (*engine.playBufferQueueItf_)->GetState(engine.playBufferQueueItf_, &state);
  if (res != SL_RESULT_SUCCESS){
    WarningMessage("GetState returned: 0x%04X (%s)", res, getSlError(res));
    return;
  }
  if (state.count >= engine.numBuffers) {
    // all buffers are full
    return;
  }

  _sfStats.StartTimer(CStatForm::STI_SOUNDMIXING);
  _pfSoundProfile.IncrementAveragingCounter();
  _pfSoundProfile.StartTimer(CSoundProfile::PTI_MIXSOUNDS);
  _pfSoundProfile.IncrementCounter(CSoundProfile::PCI_MIXINGS, 1);

  int framesToMix = engine.framesPerBuffer;
  int16_t *buffer = &engine.buffer[engine.whichBuffer * engine.framesPerBuffer * 4];

  // prepare mixer buffer
  ResetMixer((SLONG *) buffer, framesToMix * 4);

  BOOL bGamePaused = _pNetwork->IsPaused() || (_pNetwork->IsServer() && _pNetwork->GetLocalPause());

  // for each sound
  FOREACHINLIST(CSoundData, sd_Node, sl_ClhAwareList, itCsdSoundData) {
    FORDELETELIST(CSoundObject, so_Node, itCsdSoundData->sd_ClhLinkList, itCsoSoundObject) {
      CSoundObject & so = *itCsoSoundObject;
      // if the sound is in-game sound, and the game paused
      if (!(so.so_slFlags & SOF_NONGAME) && bGamePaused) {
        // don't mix it it
        continue;
      }
      // if sound is prepared and playing
      if (so.so_slFlags & SOF_PLAY &&
          so.so_slFlags & SOF_PREPARE &&
          !(so.so_slFlags & SOF_PAUSED)) {
        // mix it
        MixSound(&so);
      }
    }
  }

  // eventually normalize mixed sounds
  snd_fNormalizer = Clamp(snd_fNormalizer, 0.0f, 1.0f);
  NormalizeMixerBuffer(snd_fNormalizer, framesToMix * 4, _fLastNormalizeValue);

  res = (*engine.playBufferQueueItf_)->Enqueue(engine.playBufferQueueItf_, buffer, framesToMix * 4);
  if (res != SL_RESULT_SUCCESS){
    WarningMessage("Cannot enqueue frames to audio driver: 0x%04X (%s)", res, getSlError(res));
  }

  SLuint32 playerState;
  res = (*engine.playItf_)->GetPlayState(engine.playItf_, &playerState);
  if (res == SL_RESULT_SUCCESS && playerState != SL_PLAYSTATE_PLAYING) {
    res = (*engine.playItf_)->SetPlayState(engine.playItf_, SL_PLAYSTATE_PLAYING);
  }
#if 0
  printf("\n=====================\n");
  for(int i = 0; i < framesToMix * 2; i++)
  {
	  printf("%d ", buffer[i]);
  }
  printf("\n\n");
#endif

  // advance buffer
  if (++engine.whichBuffer >= engine.numBuffers) {
    engine.whichBuffer = 0;
  }

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
void CSoundLibrary::AddSoundAware(CSoundData &CsdAdd) {
  // add sound to list tail
  sl_ClhAwareList.AddTail(CsdAdd.sd_Node);
};

/*
 *  Remove a display mode aware object.
 */
void CSoundLibrary::RemoveSoundAware(CSoundData &CsdRemove) {
  // remove it from list
  CsdRemove.sd_Node.Remove();
};

// listen from this listener this frame
void CSoundLibrary::Listen(CSoundListener &sl) {
  // just add it to list
  if (sl.sli_lnInActiveListeners.IsLinked()) {
    sl.sli_lnInActiveListeners.Remove();
  }
  sl_lhActiveListeners.AddTail(sl.sli_lnInActiveListeners);
}

