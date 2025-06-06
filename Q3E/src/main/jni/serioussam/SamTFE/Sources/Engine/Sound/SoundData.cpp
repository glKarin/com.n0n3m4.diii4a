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

#include <Engine/Sound/SoundData.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Sound/Wave.h>
#include <Engine/Sound/SoundDecoder.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Sound/SoundObject.h>

#include <Engine/Templates/Stock_CSoundData.h>

/* ====================================================
 *
 *  Sound data awareness functions
 */

/*
 *  Pause all playing
 */
void CSoundData::PausePlayingObjects(void)
{
  // for all objects linked to data pause playing
  FOREACHINLIST(CSoundObject, so_Node, sd_ClhLinkList, itCsoPause) {
    // pause playing
    itCsoPause->Pause();
  }
}

/*
 *  For all objects resume play
 */
void CSoundData::ResumePlayingObjects(void)
{
  // for all objects resume play
  FOREACHINLIST(CSoundObject, so_Node, sd_ClhLinkList, itCsoResume) {
    // call play method again
    itCsoResume->Resume();
  }
}


/*
 *  Add object in sound aware list
 */
void CSoundData::AddObjectLink(CSoundObject &CsoAdd)
{
  // add object to list tail
  sd_ClhLinkList.AddTail(CsoAdd.so_Node);
}

/*
 *  Remove a object from aware list
 */
void CSoundData::RemoveObjectLink(CSoundObject &CsoRemove)
{
  // remove it from list
  CsoRemove.so_Node.Remove();
}


/* ====================================================
 *
 *  Class global methods
 */

// Constructor
CSoundData::CSoundData()
{
  sd_pswBuffer = NULL;
}

// Destructor
CSoundData::~CSoundData()
{
  Clear();
}


// Free Buffer (and all linked Objects)
void CSoundData::ClearBuffer(void)
{
  // if buffer exist
  if( sd_pswBuffer!=NULL) {
    // release it
    FreeMemory( sd_pswBuffer);
    sd_pswBuffer = NULL;
  }
}


// Get Sound Length in seconds
double CSoundData::GetSecondsLength(void)
{
  // if not encoded
  if (!(sd_ulFlags&SDF_ENCODED) ) {
    // len is read from wave
    return sd_dSecondsLength;
  // if encoded
  } else {
    // implement this!!!!
    ASSERT(FALSE);
    return 0;
  }
}


/* ====================================================
 *
 *  Input methods (load sound in SoundData class)
 */

// Read sound in memory
void CSoundData::Read_t(CTStream *inFile)  // throw char *
{
  // synchronize access to sounds
  CTSingleLock slSounds(&_pSound->sl_csSound, TRUE);

  ASSERT( sd_pswBuffer==NULL);
  sd_ulFlags = NONE;

  // get filename
  CTFileName fnm = inFile->GetDescription();
  // if this is encoded file
  if (fnm.FileExt()==".ogg" || fnm.FileExt()==".mp3") {
    CSoundDecoder *pmpd = new CSoundDecoder(fnm);
    if (pmpd->IsOpen()) {
      pmpd->GetFormat(sd_wfeFormat);
    }
    delete pmpd;
    // mark that this is streaming encoded file
    sd_ulFlags = SDF_ENCODED|SDF_STREAMING;

  // if this is wave file
  } else {
    // load wave info
    PCMWaveInput CpwiLoad;
    sd_wfeFormat = CpwiLoad.LoadInfo_t(inFile);
    // store sample length in seconds and average byte rate
    sd_dSecondsLength = CpwiLoad.GetSecondsLength();

    // if sound library is in lower format convert sound to library format
    if ((_pSound->sl_SwfeFormat).nSamplesPerSec < sd_wfeFormat.nSamplesPerSec) {
      sd_wfeFormat.nSamplesPerSec = (_pSound->sl_SwfeFormat).nSamplesPerSec;
    }
    // same goes for bits/sample (must be 16)
    sd_wfeFormat.wBitsPerSample = 16;

    // if library is active create buffer and load sound data
    if (_pSound->IsActive()) {
      // create Buffer
      sd_slBufferSampleSize = CpwiLoad.GetDataLength(sd_wfeFormat);
      SLONG slBufferSize = CpwiLoad.DetermineBufferSize(sd_wfeFormat);
      sd_pswBuffer = (SWORD*)AllocMemory( slBufferSize+8);
      // load data into buffer
      CpwiLoad.LoadData_t( inFile, sd_pswBuffer, sd_wfeFormat);
      // copy first sample to the last one (this is needed for linear interpolation)
      (ULONG&)(((UBYTE*)sd_pswBuffer)[slBufferSize]) = *(ULONG*)sd_pswBuffer;
    }
  }

  // add to sound aware list
  _pSound->AddSoundAware(*this);
}


// Sound can't be written to file
void CSoundData::Write_t( CTStream *outFile)
{
  ASSERTALWAYS("Cannot write sounds!");
  throw TRANS("Cannot write sounds!");
}

/* Get the description of this object. */
CTString CSoundData::GetDescription(void)
{
  CTString str;
  str.PrintF("%dkHz %dbit %s %.2lfs",
    sd_wfeFormat.nSamplesPerSec/1000,
    sd_wfeFormat.wBitsPerSample,
    sd_wfeFormat.nChannels==1 ? "mono" : "stereo",
    sd_dSecondsLength);
  return str;
}


/* ====================================================
 *
 *  Class CLEAR method
 */

// Free memory allocated for sound and Release DXBuffer
void CSoundData::Clear(void)
{
  // synchronize access to sounds
  CTSingleLock slSounds(&_pSound->sl_csSound, TRUE);

  // clear BASE class
  CSerial::Clear();

  // free DXBuffer
  ClearBuffer();

  // if added as sound aware, remove it from sound aware list
  if(IsHooked()) {
    _pSound->RemoveSoundAware(*this);
  }
}


// check if this kind of objects is auto-freed
BOOL CSoundData::IsAutoFreed(void)
{
  return FALSE;
}


// get amount of memory used by this object
SLONG CSoundData::GetUsedMemory(void)
{
  SLONG slUsed = sizeof(*this);
  if( sd_pswBuffer!=NULL) {
    ASSERT( sd_wfeFormat.nChannels==1 || sd_wfeFormat.nChannels==2);
    slUsed += sd_slBufferSampleSize * sd_wfeFormat.nChannels *2; // all sounds are 16-bit
  }
  return slUsed;
}


/* ====================================================
 *
 *  Reference counting functions
 */

// Add one reference
void CSoundData::AddReference(void)
{
  ASSERT(this!=NULL);
  MarkUsed();
}


// Remove one reference
void CSoundData::RemReference(void)
{
  ASSERT(this!=NULL);
  _pSoundStock->Release(this);
}
