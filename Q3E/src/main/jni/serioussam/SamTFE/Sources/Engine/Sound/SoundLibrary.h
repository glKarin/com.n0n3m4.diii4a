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

#ifndef SE_INCL_SOUNDLIBRARY_H
#define SE_INCL_SOUNDLIBRARY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif


#include <Engine/Base/Lists.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/Synchronization.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Templates/DynamicArray.h>

#ifdef PLATFORM_WIN32 /* rcg10042001 */
#include <Engine/Sound/DSound.h>
#include <Engine/Sound/EAX.h>
#endif

// Mixer
// set master volume and resets mixer buffer (wipes it with zeroes and keeps pointers)
void ResetMixer( const SLONG *pslBuffer, const SLONG slBufferSize);
// copy mixer buffer to the output buffer(s)
void CopyMixerBuffer_stereo( const SLONG slSrcOffset, void *pDstBuffer, const SLONG slBytes);
void CopyMixerBuffer_mono(   const SLONG slSrcOffset, void *pDstBuffer, const SLONG slBytes);
// normalize mixed sounds
void NormalizeMixerBuffer( const FLOAT snd_fNormalizer, const SLONG slBytes, FLOAT &_fLastNormalizeValue);
// mix in one sound object to mixer buffer
void MixSound( class CSoundObject *pso);


/*
 * Timer handler for sound mixing.
 */
class ENGINE_API CSoundTimerHandler : public CTimerHandler {
public:
  /* This is called every TickQuantum seconds. */
  virtual void HandleTimer(void);
};

/*
 *  Sound Library class
 */
class ENGINE_API CSoundLibrary {
public:
  enum SoundFormat {
    SF_NONE     = 0,
    SF_11025_16 = 1,
    SF_22050_16 = 2,
    SF_44100_16 = 3,
    SF_ILLEGAL  = 4
  };

//private:
public:
  CTCriticalSection sl_csSound;          // sync. access to sounds
  CSoundTimerHandler sl_thTimerHandler;  // handler for mixing sounds in timer
  INDEX sl_ctWaveDevices;                // number of devices detected

/* rcg !!! FIXME: This needs to be abstracted. */
  BOOL  sl_bUsingDirectSound;
  BOOL  sl_bUsingEAX;

#ifdef PLATFORM_WIN32
  HWAVEOUT sl_hwoWaveOut;                   // wave out handle
  CStaticStackArray<HWAVEOUT> sl_ahwoExtra; // preventively taken channels

  LPDIRECTSOUND   sl_pDS;                   // direct sound 'handle'
  LPKSPROPERTYSET sl_pKSProperty;           // for setting properties of EAX
  LPDIRECTSOUNDBUFFER sl_pDSPrimary;        // and buffers
  LPDIRECTSOUNDBUFFER sl_pDSSecondary;      // 2D usage 
  LPDIRECTSOUNDBUFFER sl_pDSSecondary2; 
  LPDIRECTSOUND3DLISTENER sl_pDSListener;   // 3D EAX
  LPDIRECTSOUND3DBUFFER   sl_pDSSourceLeft;
  LPDIRECTSOUND3DBUFFER   sl_pDSSourceRight;

  CStaticArray<WAVEHDR> sl_awhWOBuffers; // the waveout buffers
#endif

  UBYTE *sl_pubBuffersMemory;            // memory allocated for the sound buffer(s) output

  SoundFormat  sl_EsfFormat;             // sound format (external)
  WAVEFORMATEX sl_SwfeFormat;            // primary sound buffer format
  SLONG *sl_pslMixerBuffer;              // buffer for mixing sounds (32-bit!)
  SWORD *sl_pswDecodeBuffer;             // buffer for decoding encoded sounds (ogg, mpeg...)
  SLONG  sl_slMixerBufferSize;           // mixer buffer size
  SLONG  sl_slDecodeBufferSize;          // decoder buffer size

  CListHead sl_ClhAwareList;	 					         // list of sound mode aware objects
  CListHead sl_lhActiveListeners;                // active listeners for current frame of listening

  /* Return library state (active <==> format <> NONE */
  inline BOOL IsActive(void) {return sl_EsfFormat != SF_NONE;};
  /* Clear Library WaveOut */
  void ClearLibrary(void);
public:
  /* Constructor */
  CSoundLibrary(void);
  /* Destructor */
  ~CSoundLibrary(void);
  DECLARE_NOCOPYING(CSoundLibrary);

  /* Initialization */
  void Init(void);
  /* Clear Sound Library */
  void Clear(void);

  /* Set Format */
  SoundFormat SetFormat( SoundFormat EsfNew, BOOL bReport=FALSE);
  /* Get Format */
  inline SoundFormat GetFormat(void) { return sl_EsfFormat; };

  /* Update all 3d effects and copy internal data. */
  void UpdateSounds(void);
  /* Update Mixer */
  void MixSounds(void);
  /* Mute output until next UpdateSounds() */
  void Mute(void);

  /* Set listener enviroment properties (EAX) */
  BOOL SetEnvironment( INDEX iEnvNo, FLOAT fEnvSize=0);

  /* Add sound in sound aware list */
  void AddSoundAware( CSoundData &CsdAdd);
  /* Remove a sound mode aware object */
  void RemoveSoundAware( CSoundData &CsdRemove);

  // listen from this listener this frame
  void Listen(CSoundListener &sl);
};


// pointer to global sound library object
ENGINE_API extern CSoundLibrary *_pSound;


#endif  /* include-once check. */

