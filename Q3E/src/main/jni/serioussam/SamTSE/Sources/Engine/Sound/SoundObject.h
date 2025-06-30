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

#ifndef SE_INCL_SOUNDOBJECT_H
#define SE_INCL_SOUNDOBJECT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Functions.h>


// sound control values
#define SOF_NONE         (0L)
#define SOF_LOOP         (1L<<0)   // looping sound
#define SOF_3D           (1L<<1)   // has 3d effects
#define SOF_VOLUMETRIC   (1L<<2)   // no 3d effects inside hot-spot
#define SOF_SURROUND     (1L<<3)   // surround effect
#define SOF_LOCAL        (1L<<4)   // local to listener with same entity
#define SOF_SMOOTHCHANGE (1L<<5)   // for smooth transition from one sound to another on same channel
#define SOF_MUSIC        (1L<<6)   // use music-volume master control instead of sound-volume
#define SOF_NONGAME      (1L<<7)   // game sounds are not mixed while the game is paused
#define SOF_NOFILTER     (1L<<8)   // used to disable listener-specific filters - i.e. underwater

#define SOF_PAUSED       (1L<<28)  // playing, but paused (internal)
#define SOF_LOADED       (1L<<29)  // sound just loaded (internal)
#define SOF_PREPARE      (1L<<30)  // prepared for playing (internal)
#define SOF_PLAY         (1L<<31)  // currently playing

// sound parameters       
class CSoundParameters {
public:
  FLOAT sp_fLeftVolume;      // left channel volume  (0.0f-1.0f)
  FLOAT sp_fRightVolume;     // right channel volume
  SLONG sp_slLeftFilter;     // left channel bass enhance (32767-0, 0=max)
  SLONG sp_slRightFilter;    // right channel bass enhance
  FLOAT sp_fPhaseShift;      // right channel(!) delay in seconds (signed! fixint 16:16)
  FLOAT sp_fPitchShift;      // playing speed factor (>0, 1.0=normal)
  FLOAT sp_fDelay;           // seconds to wait before actual sound play start
};

// 3d sound parameters
class CSoundParameters3D {
public:
  FLOAT   sp3_fPitch;         // sound pitch 1=normal
  FLOAT   sp3_fFalloff;   // distance when sound can't be heard any more
  FLOAT   sp3_fHotSpot;   // sound at maximum volume
  FLOAT   sp3_fMaxVolume;     // maximum sound volume
};

class ENGINE_API CSoundObject {
public:
  // Sound Object Aware class (notify class when direct sound pointer is not valid)
  CListNode so_Node;        // for linking in list
  class CSoundDecoder *so_psdcDecoder; // only for sounds that are mpx/ogg

public: //private:
  CSoundData *so_pCsdLink;   // linked on SoundData
  SLONG so_slFlags;          // playing flags

  // internal mixer parameters
  FLOAT so_fDelayed;         // seconds already passed from start playing sound request
  FLOAT so_fLastLeftVolume;  // volume from previous mixing (for seamless transition)
  FLOAT so_fLastRightVolume;
  SWORD so_swLastLeftSample; // samples from previous mixing (for filtering purposes)
  SWORD so_swLastRightSample;
  FLOAT so_fLeftOffset;    // current playing offset of left channel
  FLOAT so_fRightOffset;   // current playing offset of right channel
  FLOAT so_fOffsetDelta;   // difference between offsets in samples (for seamless transition between phases)

  // sound parameters
  CEntity *so_penEntity;        // entity that owns this sound (may be null)
  CSoundParameters so_sp;       // currently active parameters
  CSoundParameters so_spNew;    // parameters to set on next update
  CSoundParameters3D so_sp3;    // 3d sound parameters

  /* Play Buffer */
  void PlayBuffer(void);
  /* Stop Buffer */
  void StopBuffer(void);
  /* Update all 3d effects. */
  void Update3DEffects(void);
  /* Prepare sound */
  void PrepareSound(void);

  // get proper sound object for predicted events - return NULL the event is already predicted
  CSoundObject *GetPredictionTail(ULONG ulTypeID, ULONG ulEventID);

  // play sound - internal function - doesn't account for prediction
  void Play_internal( CSoundData *pCsdLink, SLONG slFlags);
  void Stop_internal(void);

public:
  // Constructor
  CSoundObject();
  // Destructor
  ~CSoundObject();
  // copy from another object of same class
  void Copy(CSoundObject &soOther);

  // play sound
  void Play( CSoundData *pCsdLink, SLONG slFlags);
  // stop playing sound
  void Stop( void);
  // Pause -> Stop playing sound but keep it linked to data
  inline void Pause(void)  { so_slFlags |= SOF_PAUSED; };
  // Resume -> Resume playing stoped sound
  inline void Resume(void) { so_slFlags &= ~SOF_PAUSED; };
  // check if sound is playing
  inline BOOL IsPlaying(void) { 
    return (so_slFlags&SOF_PLAY); 
  };
  // check if sound is paused
  inline BOOL IsPaused(void) { 
    return (so_slFlags&SOF_PAUSED); 
  };

  // Check if hooked
  inline BOOL IsHooked(void) const { return so_Node.IsLinked(); };

  // Set volume
  inline void SetVolume( FLOAT fLeftVolume, FLOAT fRightVolume) {
    ASSERT( fLeftVolume  <= SL_VOLUME_MAX && fLeftVolume  >= SL_VOLUME_MIN);
    ASSERT( fRightVolume <= SL_VOLUME_MAX && fRightVolume >= SL_VOLUME_MIN);
    so_spNew.sp_fLeftVolume  = fLeftVolume *(1.0f/SL_VOLUME_MAX);
    so_spNew.sp_fRightVolume = fRightVolume*(1.0f/SL_VOLUME_MAX);
  };
  // Set filter
  inline void SetFilter( FLOAT fLeftFilter, FLOAT fRightFilter) { // 1=no filter (>1=more bass)
    ASSERT( (fLeftFilter >= 1) && (fRightFilter >= 1));
    so_spNew.sp_slLeftFilter  = FloatToInt(32767.0f/fLeftFilter);
    so_spNew.sp_slRightFilter = FloatToInt(32767.0f/fRightFilter);
  };
  // Set pitch shifting
  inline void SetPitch( FLOAT fPitch) { // 1.0 for normal (<1 = slower, >1 = faster playing)
    ASSERT( fPitch > 0);
    so_spNew.sp_fPitchShift = fPitch;
  };
  // Set phase shifting
  inline void SetPhase( FLOAT fPhase) { // right channel delay in seconds (0 = no delay)
    ASSERT( (fPhase <= 1) && (fPhase >= -1));
    so_spNew.sp_fPhaseShift = fPhase;
  };
  // Set delay
  inline void SetDelay( FLOAT fDelay) { // in seconds (0 = no delay)
    ASSERT( fDelay >= 0);
    so_spNew.sp_fDelay = fDelay;
  };

  // Set Position in 3D
  inline void SetOwner(CEntity*pen) {
    so_penEntity = pen;
  };
  // Set 3D parameters
  void Set3DParameters(FLOAT fMaxDistance, FLOAT fMinDistance, FLOAT fMaxVolume, FLOAT fPitch);

  // read/write functions
  void Read_t(CTStream *pistr);  // throw char *
  void Write_t(CTStream *postr); // throw char *

  // Obtain sound and play it for this object
  void Play_t(const CTFileName &fnmSound, SLONG slFlags); // throw char *
  // hard set sound offset in seconds
  void SetOffset(FLOAT fOffset);
};


#endif  /* include-once check. */

