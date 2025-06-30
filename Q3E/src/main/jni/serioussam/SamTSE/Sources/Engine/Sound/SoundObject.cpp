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

#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/CRC.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Functions.h>
#include <Engine/Sound/SoundObject.h>
#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Sound/SoundData.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Sound/SoundListener.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Base/ListIterator.inl>

#include <Engine/Templates/Stock_CSoundData.h>

// sound event codes for prediction
#define EVENT_SOUNDPLAY         0x0101
#define EVENT_SOUNDSTOP         0x0102
#define EVENT_SOUNDSETOFFSET    0x0103

extern FLOAT snd_fEarsDistance;
extern FLOAT snd_fDelaySoundSpeed;
extern FLOAT snd_fDopplerSoundSpeed;
extern FLOAT snd_fPanStrength;
extern FLOAT snd_fLRFilter;
extern FLOAT snd_fBFilter;
extern FLOAT snd_fUFilter;
extern FLOAT snd_fDFilter;

extern BOOL _bPredictionActive;

// console variables for volume
extern FLOAT snd_fSoundVolume;
extern FLOAT snd_fMusicVolume;

#if 0 // DG: unused.
static CTString GetPred(CEntity*pen)
{
  CTString str1;
  if (pen->IsPredictor()) {
    str1 = "predictor";
  } else if (pen->IsPredicted()) {
    str1 = "predicted";
  } else if (pen->en_ulFlags & ENF_WILLBEPREDICTED) {
    str1 = "will be predicted";
  } else {
    str1 = "???";
  }
  CTString str;
  str.PrintF("%08x-%s", pen, (const char *) str1);
  return str;
}
#endif // 0 (unused)
/* ====================================================
 *
 *  Class global methods
 */

/*
 *  Constructor
 */
CSoundObject::CSoundObject()
{
  so_pCsdLink = NULL;
  so_psdcDecoder = NULL;
  so_penEntity = NULL;
  so_slFlags = SOF_NONE;

  // clear sound settings
  so_spNew.sp_fLeftVolume   = 1.0f;
  so_spNew.sp_fRightVolume  = 1.0f;
  so_spNew.sp_slLeftFilter  = 0x7FFF;
  so_spNew.sp_slRightFilter = 0x7FFF;
  so_spNew.sp_fPitchShift   = 1.0f;
  so_spNew.sp_fPhaseShift   = 0.0f;
  so_spNew.sp_fDelay        = 0.0f;

  so_sp = so_spNew;

  so_fLeftOffset   = 0.0f;
  so_fRightOffset  = 0.0f;
  so_fOffsetDelta  = 0.0f;
  so_fDelayed      = 0.0f;
  so_fLastLeftVolume   = 1.0f;
  so_fLastRightVolume  = 1.0f;
  so_swLastLeftSample  = 0;
  so_swLastRightSample = 0;

  // 3d effects
  so_sp3.sp3_fFalloff = 0.0f;
  so_sp3.sp3_fHotSpot = 0.0f;
  so_sp3.sp3_fMaxVolume   = 0.0f;
  so_sp3.sp3_fPitch       = 1.0f;
}

/*
 *  Destructor
 */
CSoundObject::~CSoundObject()
{
  Stop_internal();
}

// copy from another object of same class
void CSoundObject::Copy(CSoundObject &soOther)
{
  Stop_internal();
  so_sp = so_spNew = soOther.so_sp;
  so_sp3 = soOther.so_sp3;
  so_penEntity = NULL;
  so_slFlags = soOther.so_slFlags;
  if (soOther.so_slFlags&SOF_PLAY) {
    Play(soOther.so_pCsdLink, soOther.so_slFlags);
  }
}

// Set 3D parameters
void CSoundObject::Set3DParameters( FLOAT fFalloff, FLOAT fHotSpot,
                             FLOAT fMaxVolume, FLOAT fPitch)
{
  ASSERT( (fFalloff > 0) && (fHotSpot >= 0));
  ASSERT(  fMaxVolume >= 0);
  ASSERT(  fFalloff >= fHotSpot);
  ASSERT(  fPitch > 0);

  CSoundObject *pso = this;
  // if the sound's entity is a predictor
  if (_bPredictionActive && so_penEntity!=NULL) {
    if (so_penEntity->IsPredictionHead()) {
      // get your prediction tail
      //CPrintF("SET3D: ");
      CEntity *pen = so_penEntity->GetPredictionTail();
      if (pen!=so_penEntity) {
        pso = (CSoundObject *)( ((UBYTE*)pen) + (size_t(this)-size_t(so_penEntity)) );
      }
    }
  }
  
  pso->so_sp3.sp3_fFalloff = fFalloff;
  pso->so_sp3.sp3_fHotSpot = fHotSpot;
  pso->so_sp3.sp3_fMaxVolume   = fMaxVolume;
  pso->so_sp3.sp3_fPitch       = fPitch;
}


/* ====================================================
 * Sound control methods
 */

// get proper sound object for predicted events - return NULL the event is already predicted
CSoundObject *CSoundObject::GetPredictionTail(ULONG ulTypeID, ULONG ulEventID)
{
  // if the sound has an entity
  if (so_penEntity!=NULL) {
    //CPrintF(" {%s}", GetPred(so_penEntity));
    // if the entity is temporary predictor
    if (so_penEntity->GetFlags()&ENF_TEMPPREDICTOR) {
      //CPrintF(" temppred\n");
      // it must not play the sound
      return NULL;
    }
    SLONG slOffset = size_t(this)-size_t(so_penEntity);

    ULONG ulCRC;
    CRC_Start(ulCRC);
    CRC_AddLONG(ulCRC, slOffset);
    CRC_AddLONG(ulCRC, ulTypeID);
    CRC_Finish(ulCRC);

    // if the event is predicted
    if (so_penEntity->CheckEventPrediction(ulCRC, ulEventID)) {
      //CPrintF(" predicted\n");
      // return nothing
      return NULL;
    }
    CEntity *pen = so_penEntity;
    // find eventual prediction tail sound object
    if (pen->IsPredictor()) {
      pen = pen->GetPredictionTail();
      if (pen!=so_penEntity) {
        //CPrintF(" ROUTED\n");
        return (CSoundObject *)( ((UBYTE*)pen) + slOffset );
      }
    }
  }
  // if no specific prediction states - use this object
  //CPrintF(" ORIGINAL\n");
  return this;
}
/*
 *  Play
 */
void CSoundObject::Play(CSoundData *pCsdLink, SLONG slFlags)
{
  // synchronize access to sounds
  CTSingleLock slSounds( &_pSound->sl_csSound, TRUE);

  //CPrintF("PLAY: '%s'", (const char*)pCsdLink->GetName().FileName());
  // get prediction tail
  CSoundObject *psoTail = GetPredictionTail(EVENT_SOUNDPLAY, PointerToID(pCsdLink));
  // if the event is predicted
  if (psoTail==NULL) {
    // do nothing;
    return;
  }

  // play the sound in the given object
  psoTail->Play_internal(pCsdLink, slFlags);
}

// play sound - internal function - doesn't account for prediction
void CSoundObject::Play_internal( CSoundData *pCsdLink, SLONG slFlags)
{
  ASSERT(so_penEntity==NULL || !so_penEntity->IsPredictor());

  // check if should continue with new sound
  BOOL bContinue = 
    ((slFlags&SOF_SMOOTHCHANGE) &&
     (so_slFlags&SOF_PREPARE) &&
     (so_slFlags&SOF_PLAY));

  Stop_internal();

  // mark new data as referenced once more
  if(pCsdLink != NULL) pCsdLink->AddReference();
  // mark old data as referenced once less
  if(so_pCsdLink != NULL) so_pCsdLink->RemReference();

  // store init SoundData
  so_pCsdLink = pCsdLink;
  // add to link list
  so_pCsdLink->AddObjectLink(*this);

  // store flags
  so_slFlags = slFlags;

  // if should continue with new sound
  if (bContinue) {
    // play buffer immediately
    so_slFlags = so_slFlags | SOF_PREPARE | SOF_PLAY;
  } else {
    // play buffer
    so_slFlags = (so_slFlags & ~(SOF_PREPARE|SOF_PAUSED)) | SOF_PLAY;
  }

  // if the sound data is encoded
  if (so_pCsdLink->sd_ulFlags&SDF_ENCODED) {
    // create decoder
    if (so_pCsdLink->sd_ulFlags&SDF_STREAMING) {
      so_psdcDecoder = new CSoundDecoder(so_pCsdLink->GetName());
    } else {
      ASSERT(FALSE);  // nonstreaming not supported anymore
    }
  }

  // remember starting parameters
  so_sp = so_spNew;

  // initialize mixer temporary variables
  if (!(slFlags&SOF_LOADED)) {
    so_fLastLeftVolume  = so_sp.sp_fLeftVolume;
    so_fLastRightVolume = so_sp.sp_fRightVolume;
    so_fLeftOffset  = 0.0f;
    so_fRightOffset = 0.0f;
    so_fOffsetDelta = 0.0f;
    so_fDelayed     = 0.0f;
    if (!bContinue) {
      so_swLastLeftSample  = 0;
      so_swLastRightSample = 0;
    } else {
      // adjust for master volume
      if(so_slFlags&SOF_MUSIC) {
        so_fLastLeftVolume  *= snd_fMusicVolume;
        so_fLastRightVolume *= snd_fMusicVolume;
      } else {
        so_fLastLeftVolume  *= snd_fSoundVolume;
        so_fLastRightVolume *= snd_fSoundVolume;
      }
    }
  }
}


// hard set sound offset in seconds
void CSoundObject::SetOffset( FLOAT fOffset)
{
  // synchronize access to sounds
  CTSingleLock slSounds( &_pSound->sl_csSound, TRUE);

  // get prediction tail
  //CPrintF("SETOFF: ");
  CSoundObject *psoTail = GetPredictionTail(EVENT_SOUNDSETOFFSET, 0);
  // if the event is predicted
  if (psoTail==NULL) {
    // do nothing;
    return;
  }

  // if sound not playing
  if (psoTail->so_pCsdLink==NULL) {
    // do nothing
    return;
  }

  // safety check
  ASSERT( fOffset>=0);
  if( fOffset<0) {
    CPrintF( "BUG: Trying to set negative offset (%.2g) in sound '%s' !\n", fOffset, (const char *) (CTString&)psoTail->so_pCsdLink->GetName());
    fOffset = 0.0f;
  }

  // on the other hand, don't set offset for real - might be source for some bugs!
  return;
  
  // update sound offsets
  CPrintF("Setting offset: %g\n", fOffset);
  psoTail->so_fLeftOffset = psoTail->so_fRightOffset = psoTail->so_pCsdLink->sd_wfeFormat.nSamplesPerSec*fOffset;
}


/*
 *  Stop
 */
void CSoundObject::Stop(void)
{
  // synchronize access to sounds
  CTSingleLock slSounds( &_pSound->sl_csSound, TRUE);

  //CPrintF("STOP");
  if (so_pCsdLink!=NULL) {
    //CPrintF(" '%s'", (const char*)so_pCsdLink->GetName().FileName());
  }

  CSoundObject *psoTail = this;
  // get prediction tail
  psoTail = GetPredictionTail(EVENT_SOUNDSTOP, PointerToID(so_pCsdLink));
  // if the event is predicted
  if (psoTail==NULL) {
    // do nothing;
    return;
  }

  psoTail->Stop_internal();
}
void CSoundObject::Stop_internal(void)
{
  // sound is stopped
  so_slFlags &= ~(SOF_PLAY|SOF_PREPARE|SOF_PAUSED);

  // destroy decoder if exists
  if( so_psdcDecoder!=NULL) {
    delete so_psdcDecoder;
    so_psdcDecoder = NULL;
  }

  // if added in link list, remove it from list
  if( IsHooked()) {
    ASSERT(so_pCsdLink != NULL);
    so_pCsdLink->RemoveObjectLink(*this);
    // remove reference from SoundData
    so_pCsdLink->RemReference();
    // clear SoundData link
    so_pCsdLink = NULL;
  }
}


// Update all 3d effects
void CSoundObject::Update3DEffects(void)
{
  // if not 3d sound
  if( !(so_slFlags & SOF_3D)) {
    // do nothing;
    return;
  }

//  if (!(so_slFlags&SOF_PREPARE)) {
    // if the sound's entity is a predictor
/*    if (so_penEntity!=NULL && so_penEntity->IsPredictor()) {
      // kill the sound
      so_slFlags&=~SOF_PLAY;
      //CPrintF("Update canceled %s (%s)\n", (const char*)so_pCsdLink->GetName(), GetPred(so_penEntity));
      // do nothing;
      return;
    }
    */
    //CPrintF("Update PASSED %s (%s)\n", (const char*)so_pCsdLink->GetName(), GetPred(so_penEntity));
//  }

  // total parameters (accounting for all listeners)
  FLOAT fTLVolume = 0, fTRVolume = 0;
  FLOAT fTLFilter = UpperLimit(0.0f), fTRFilter = UpperLimit(0.0f);
  FLOAT fTLDelay  = UpperLimit(0.0f), fTRDelay  = UpperLimit(0.0f);
  FLOAT fTPitchShift = 0;

  // get your position parameters
  FLOAT3D vPosition(0,0,0);
  FLOAT3D vSpeed(0,0,0);
  if (so_penEntity!=NULL) {
    vPosition = so_penEntity->en_plPlacement.pl_PositionVector;
    if (so_penEntity->en_ulPhysicsFlags&EPF_MOVABLE) {
      CMovableEntity *penMovable = (CMovableEntity *)so_penEntity;
      vSpeed = penMovable->en_vCurrentTranslationAbsolute;
    }
  }

  // for each listener
  INDEX ctEffectiveListeners = 0;
  {FOREACHINLIST( CSoundListener, sli_lnInActiveListeners, _pSound->sl_lhActiveListeners, itsli)
  {
    CSoundListener &sli = *itsli;

    // if local, but not of this listener
    if ((so_slFlags&SOF_LOCAL) && so_penEntity!=sli.sli_penEntity) {
      // don't add this listener
      continue;
    }

    // calculated parameters for this listener
    FLOAT fLVolume, fRVolume;
    FLOAT fLFilter, fRFilter;
    FLOAT fLDelay , fRDelay ;
    FLOAT fPitchShift;

    // calculate distance from listener
    FLOAT3D vAbsDelta = vPosition - sli.sli_vPosition;
    FLOAT fAbsDelta = vAbsDelta.Length();

    // if too far away
    if (fAbsDelta>so_sp3.sp3_fFalloff) {
      // don't add this listener
      continue;
    }

    // calculate distance falloff factor
    FLOAT fDistanceFactor;
    if( fAbsDelta <= so_sp3.sp3_fHotSpot) {
      fDistanceFactor = 1;
    } else {
      fDistanceFactor = (so_sp3.sp3_fFalloff - fAbsDelta) /
                        (so_sp3.sp3_fFalloff - so_sp3.sp3_fHotSpot);
    }
    ASSERT(fDistanceFactor>=0 && fDistanceFactor<=+1);

    // calculate volumetric influence
    // NOTE: decoded sounds must be treated as volumetric
    FLOAT fNonVolumetric = 1.0f;
    FLOAT fNonVolumetricAdvanced = 1.0f;
    if( (so_slFlags & SOF_VOLUMETRIC) || so_psdcDecoder!=NULL) {
      fNonVolumetric = 1.0f-fDistanceFactor;
      fNonVolumetricAdvanced = 0.0f;
    }
    ASSERT(fNonVolumetric>=0 && fNonVolumetric<=+1);

    // find doppler effect pitch shift
    fPitchShift = 1.0f;
    if (fAbsDelta>0.001f) {
      FLOAT3D vObjectDirection = vAbsDelta/fAbsDelta;
      FLOAT fObjectSpeed = vSpeed%vObjectDirection; // negative towards listener
      FLOAT fListenerSpeed = sli.sli_vSpeed%vObjectDirection; // positive towards object
      fPitchShift =
        (snd_fDopplerSoundSpeed+fListenerSpeed*fNonVolumetricAdvanced)/
        (snd_fDopplerSoundSpeed+fObjectSpeed*fNonVolumetricAdvanced);
    }

    // find position of sound relative to viewer orientation
    FLOAT3D vRelative = vAbsDelta*!sli.sli_mRotation;
    // find distances from left and right ear
    FLOAT fLDistance = (FLOAT3D(-snd_fEarsDistance*fNonVolumetricAdvanced/2,0,0)-vRelative).Length();
    FLOAT fRDistance = (FLOAT3D(+snd_fEarsDistance*fNonVolumetricAdvanced/2,0,0)-vRelative).Length();
    // calculate sound delay to each ear
    fLDelay = fLDistance/snd_fDelaySoundSpeed;
    fRDelay = fRDistance/snd_fDelaySoundSpeed;

    // calculate relative sound directions
    FLOAT fLRFactor=0;  // positive right
    FLOAT fFBFactor=0;  // positive front
    FLOAT fUDFactor=0;  // positive up

    if (fAbsDelta>0.001f) {
      FLOAT3D vDir = vRelative/fAbsDelta;
      fLRFactor = +vDir(1);
      fFBFactor = -vDir(3);
      fUDFactor = +vDir(2);
    }
    ASSERT(fLRFactor>=-1.1 && fLRFactor<=+1.1);
    ASSERT(fFBFactor>=-1.1 && fFBFactor<=+1.1);
    ASSERT(fUDFactor>=-1.1 && fUDFactor<=+1.1);


    // calculate panning influence factor
    FLOAT fPanningFactor= fNonVolumetric*snd_fPanStrength;
    ASSERT(fPanningFactor>=0 && fPanningFactor<=+1);

    // calc volume for left and right channel
    FLOAT fVolume = so_sp3.sp3_fMaxVolume * fDistanceFactor;
    if( fLRFactor > 0) {
      fLVolume = (1-fLRFactor*fPanningFactor) * fVolume;
      fRVolume = fVolume;
    } else {
      fLVolume = fVolume;
      fRVolume = (1+fLRFactor*fPanningFactor) * fVolume;
    }

    // calculate filters
    FLOAT fListenerFilter = sli.sli_fFilter;
    if (so_slFlags&SOF_NOFILTER) {
      fListenerFilter = 0.0f;
    }
    fLFilter = fRFilter = 1+fListenerFilter;
    if( fLRFactor > 0) {
      fLFilter += fLRFactor*snd_fLRFilter*fNonVolumetricAdvanced;
    } else {
      fRFilter -= fLRFactor*snd_fLRFilter*fNonVolumetricAdvanced;
    }
    if( fFBFactor<0) {
      fLFilter -= snd_fBFilter*fFBFactor*fNonVolumetricAdvanced;
      fRFilter -= snd_fBFilter*fFBFactor*fNonVolumetricAdvanced;
    }
    if( fUDFactor>0) {
      fLFilter += snd_fUFilter*fUDFactor*fNonVolumetricAdvanced;
      fRFilter += snd_fUFilter*fUDFactor*fNonVolumetricAdvanced;
    } else {
      fLFilter -= snd_fDFilter*fUDFactor*fNonVolumetricAdvanced;
      fRFilter -= snd_fDFilter*fUDFactor*fNonVolumetricAdvanced;
    }

    // adjust calculated volume to the one of listener
    fLVolume *= sli.sli_fVolume;
    fRVolume *= sli.sli_fVolume;

    // update parameters for all listener
    fTLVolume = Max( fTLVolume, fLVolume);
    fTRVolume = Max( fTRVolume, fRVolume);
    fTLDelay  = Min( fTLDelay , fLDelay );
    fTRDelay  = Min( fTRDelay , fRDelay );
    fTLFilter = Min( fTLFilter, fLFilter);
    fTRFilter = Min( fTRFilter, fRFilter);
    fTPitchShift += fPitchShift;
    ctEffectiveListeners++;
  }}

  fTPitchShift /= ctEffectiveListeners;

  // calculate 2d parameters
  FLOAT fPitchShift = fTPitchShift * so_sp3.sp3_fPitch;
  FLOAT fPhaseShift = fTLDelay-fTRDelay;
  FLOAT fDelay      = Min( fTRDelay,fTLDelay);

//  CPrintF("V:%f %f F:%f %f P:%f S:%f\n", 
//    fTLVolume, fTRVolume,
//    fTLFilter, fTRFilter,
//    fPhaseShift,
//    fPitchShift);

  // set sound parameters
  fTLVolume = Clamp( fTLVolume, SL_VOLUME_MIN, SL_VOLUME_MAX);
  fTRVolume = Clamp( fTRVolume, SL_VOLUME_MIN, SL_VOLUME_MAX);
  SetVolume( fTLVolume, fTRVolume);

  if( fTLVolume>0 || fTRVolume>0) {
    // do safety clamping
    fTLFilter   = ClampDn( fTLFilter, 1.0f);
    fTRFilter   = ClampDn( fTRFilter, 1.0f);
    fDelay      = ClampDn( fDelay, 0.0f);
    fPitchShift = ClampDn( fPitchShift, 0.001f);
    fPhaseShift = Clamp(   fPhaseShift, -1.0f, +1.0f);
    // set sound params
    SetFilter( fTLFilter, fTRFilter);
    SetDelay(  fDelay);
    SetPitch(  fPitchShift);
    SetPhase(  fPhaseShift);
  }
}


// Prepare sound
void CSoundObject::PrepareSound(void)
{
  ASSERT(so_penEntity==NULL || !so_penEntity->IsPredictor());

  so_fLastLeftVolume = so_spNew.sp_fLeftVolume;
  so_fLastRightVolume = so_spNew.sp_fRightVolume;
  // adjust for master volume
  if(so_slFlags&SOF_MUSIC) {
    so_fLastLeftVolume  *= snd_fMusicVolume;
    so_fLastRightVolume *= snd_fMusicVolume;
  } else {
    so_fLastLeftVolume  *= snd_fSoundVolume;
    so_fLastRightVolume *= snd_fSoundVolume;
  }
}


// Obtain sound and play it for this object
void CSoundObject::Play_t(const CTFileName &fnmSound, SLONG slFlags) // throw char *
{
  // obtain it (adds one reference)
  CSoundData *ptd = _pSoundStock->Obtain_t(fnmSound);
  // set it as data (adds one more reference, and remove old reference)
  Play(ptd, slFlags);
  // release it (removes one reference)
  _pSoundStock->Release(ptd);
  // total reference count +1+1-1 = +1 for new data -1 for old data
}



// read/write functions
void CSoundObject::Read_t(CTStream *pistr)  // throw char *
{
  int iDroppedOut;

  // load file name
  CTFileName  fnmSound;
  *pistr >> fnmSound;

  // load object preferences
  *pistr >> iDroppedOut;
  *pistr >> so_slFlags;

  *pistr >> so_spNew.sp_fLeftVolume;
  *pistr >> so_spNew.sp_fRightVolume;
  *pistr >> so_spNew.sp_slLeftFilter;
  *pistr >> so_spNew.sp_slRightFilter;
  *pistr >> so_spNew.sp_fPitchShift;
  *pistr >> so_spNew.sp_fPhaseShift;
  *pistr >> so_spNew.sp_fDelay;

  *pistr >> so_fDelayed;
  *pistr >> so_fLastLeftVolume;
  *pistr >> so_fLastRightVolume;
  *pistr >> so_swLastLeftSample;
  *pistr >> so_swLastRightSample;
  *pistr >> so_fLeftOffset;
  *pistr >> so_fRightOffset;
  *pistr >> so_fOffsetDelta;

  // load 3D parameters
  so_penEntity = NULL;
  *pistr >> so_sp3.sp3_fFalloff;
  *pistr >> so_sp3.sp3_fHotSpot;
  *pistr >> so_sp3.sp3_fMaxVolume;
  *pistr >> so_sp3.sp3_fPitch;

  // update current state
  so_sp = so_spNew;

  // Obtain and play object (sound)
  if ( fnmSound != "" && (so_slFlags&SOF_PLAY)) {
    Play_t( fnmSound, so_slFlags|SOF_LOADED);
  }
}

void CSoundObject::Write_t(CTStream *pistr) // throw char *
{
  int iDroppedOut=0;

  // save file name
  if (so_pCsdLink!=NULL) {
    *pistr << (so_pCsdLink->GetName());
  } else {
    *pistr << CTFILENAME("");
  }

  // save object preferences
  *pistr << iDroppedOut;
  *pistr << so_slFlags;

  *pistr << so_spNew.sp_fLeftVolume;
  *pistr << so_spNew.sp_fRightVolume;
  *pistr << so_spNew.sp_slLeftFilter;
  *pistr << so_spNew.sp_slRightFilter;
  *pistr << so_spNew.sp_fPitchShift;
  *pistr << so_spNew.sp_fPhaseShift;
  *pistr << so_spNew.sp_fDelay;

  *pistr << so_fDelayed;
  *pistr << so_fLastLeftVolume;
  *pistr << so_fLastRightVolume;
  *pistr << so_swLastLeftSample;
  *pistr << so_swLastRightSample;
  *pistr << so_fLeftOffset;
  *pistr << so_fRightOffset;
  *pistr << so_fOffsetDelta;

  // save 3D parameters
  *pistr << so_sp3.sp3_fFalloff;
  *pistr << so_sp3.sp3_fHotSpot;
  *pistr << so_sp3.sp3_fMaxVolume;
  *pistr << so_sp3.sp3_fPitch;
}

