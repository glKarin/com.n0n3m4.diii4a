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
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Sound/Wave.h>

/* ====================================================
 *
 *      CONVERSION FUNCTIONS
 *
 */

// check wave format
void PCMWaveInput::CheckWaveFormat_t(WAVEFORMATEX wfeCheck, const char *pcErrorString)
{
  // check format tag
  if (wfeCheck.wFormatTag != 1) {
    ThrowF_t(TRANS("%s: Invalid format tag, not a PCM Wave file!"), pcErrorString);
  }
  // check bits per sample
  if (wfeCheck.wBitsPerSample != 8 &&
      wfeCheck.wBitsPerSample != 16) {
    ThrowF_t(TRANS("%s: Unknown Bits Per Sample value!"), pcErrorString);
  }
  // check number of channels
  if (wfeCheck.nChannels != 1 &&
      wfeCheck.nChannels != 2) {
    ThrowF_t(TRANS("%s: Invalid number of channels!"), pcErrorString);
  }
  //ASSERT( wfeCheck.wBitsPerSample==16);
}


// Get next data
inline ULONG PCMWaveInput::GetData_t(CTStream *pCstrInput)
{
  ASSERT(pwi_bInfoLoaded);
  // read data according to bits per sample value
  if (pwi_wfeWave.wBitsPerSample==8) {
    // read UBYTE
    UBYTE ubData;
    *pCstrInput >> ubData;
    return ((ULONG)ubData) <<16;            // (shift) prepare data for shrink/expand operation
  } else {
    // read UWORD
    SWORD swData;
    *pCstrInput >> swData;
    return ((ULONG)(swData+0x8000)) <<8;   // (shift) prepare data for shrink/expand operation
  }
}


// Store data
inline void PCMWaveInput::StoreData(ULONG ulData)
{
  ASSERT( pwi_wfeDesired.wBitsPerSample==16);
  *pwi_pswMemory++ = ((SWORD)(ulData>>8) -0x8000);  // (shift) restore data format
}


/*
 *  Copy data
 */
void PCMWaveInput::CopyData_t(CTStream *pCstrInput)
{
  // for all input data (mono and stereo)
  ULONG ulDataCount = GetDataLength() * pwi_wfeWave.nChannels;
  while (ulDataCount > 0) {
    StoreData(GetData_t(pCstrInput));   // read and store data from input (hidden BitsPerSample conversion!)
    ulDataCount--;                      // to next data
  }
}



/*
 *  Shrink data
 */
// Shrink data
void PCMWaveInput::ShrinkData_t(CTStream *pCstrInput)
{
  ASSERT(pwi_dRatio>1.0);

  // *** MONO ***
  if (pwi_wfeWave.nChannels == 1) {
    DOUBLE dInterData, dTempData, dRatio;
    ULONG ulDataCount;

    // data intermediate value
    dInterData = 0.0;
    // for all input data (mono)
    ulDataCount = GetDataLength();
    dRatio = pwi_dRatio;
    while (ulDataCount > 0) {
      // read part of data (<100%)
      if (dRatio<1.0) {
        dTempData = GetData_t(pCstrInput);
        dInterData += dTempData*dRatio;
        StoreData(ULONG(dInterData/pwi_dRatio));
        // new intermediate value
        dRatio = 1 - dRatio;
        dInterData = dTempData*dRatio;
        dRatio = pwi_dRatio - dRatio;

      // read complete data (100%)
      } else {
        dInterData += GetData_t(pCstrInput);
        dRatio -= 1.0;
      }
      ulDataCount--;        // to next data
    }
    StoreData(ULONG(dInterData/(pwi_dRatio-dRatio)));


  // *** STEREO ***
  } else if (pwi_wfeWave.nChannels == 2) {
    DOUBLE dLInterData, dRInterData, dLTempData, dRTempData, dRatio;
    ULONG ulDataCount;

    // data intermediate value
    dLInterData = 0.0;
    dRInterData = 0.0;
    // for all input data (mono)
    ulDataCount = GetDataLength();
    dRatio = pwi_dRatio;
    while (ulDataCount > 0) {
      // read part of data (<100%)
      if (dRatio<1.0) {
        dLTempData = GetData_t(pCstrInput);
        dRTempData = GetData_t(pCstrInput);
        dLInterData += dLTempData*dRatio;
        dRInterData += dRTempData*dRatio;
        StoreData(ULONG(dLInterData/pwi_dRatio));
        StoreData(ULONG(dRInterData/pwi_dRatio));
        // new intermediate value
        dRatio = 1 - dRatio;
        dLInterData = dLTempData*dRatio;
        dRInterData = dRTempData*dRatio;
        dRatio = pwi_dRatio - dRatio;

      // read complete data (100%)
      } else {
        dLInterData += GetData_t(pCstrInput);
        dRInterData += GetData_t(pCstrInput);
        dRatio -= 1.0;
      }
      ulDataCount--;        // to next data
    }
    StoreData(ULONG(dLInterData/(pwi_dRatio-dRatio)));
    StoreData(ULONG(dRInterData/(pwi_dRatio-dRatio)));
  }
}




/* ====================================================
 *
 *      WAVE FUNCTIONS
 *
 */

/*
 *  Load Wave info
 */

WAVEFORMATEX PCMWaveInput::LoadInfo_t(CTStream *pCstrInput)
{
  // if already loaded -> exception
  if (pwi_bInfoLoaded) {
    throw (TRANS("PCM Wave Input: Info already loaded."));
  }

  /* Read Riff */
  pCstrInput->ExpectID_t(CChunkID("RIFF"));     // ID "RIFF"
  (*pCstrInput) >> pwi_ulRiffLength;            // Ucitaj duljinu file-a

  /* Read Wave */
  pCstrInput->ExpectID_t(CChunkID("WAVE"));     // ID "WAVE"
  pCstrInput->ExpectID_t(CChunkID("fmt "));     // ID "fmt "
  // read Format Chunk length
  SLONG  slFmtLength;
  (*pCstrInput) >> slFmtLength;

  ULONG ul = 0;

  // read WAVE format
  (*pCstrInput) >> pwi_wfeWave.wFormatTag;
  (*pCstrInput) >> pwi_wfeWave.nChannels;
  (*pCstrInput) >> ul; pwi_wfeWave.nSamplesPerSec = (DWORD) ul;
  (*pCstrInput) >> ul; pwi_wfeWave.nAvgBytesPerSec = (DWORD) ul;
  (*pCstrInput) >> pwi_wfeWave.nBlockAlign;
  (*pCstrInput) >> pwi_wfeWave.wBitsPerSample;
  pwi_wfeWave.cbSize = 0;   // Only for PCM Wave !!!

  // WARNING !!! - Only for PCM Wave - Skip extra information if exists
  if( slFmtLength > 16) {
    //WarningMessage("PCM Wave Input: Wave format Extra information skipped!");
    pCstrInput->Seek_t(slFmtLength - 16, CTStream::SD_CUR);
  }

  // WARNING - If exist Fact chunk skip it (purpose unknown)
  if( pCstrInput->GetID_t() == CChunkID("fact")) {
    //WarningMessage("PCM Wave Input: Fact Chunk skipped!");
    SLONG  slSkipLength;
    (*pCstrInput) >> slSkipLength;
    pCstrInput->Seek_t(slSkipLength, CTStream::SD_CUR);
  // seek back on Chunk ID
  } else {
    pCstrInput->Seek_t(-CID_LENGTH, CTStream::SD_CUR);
  }

  /* Read Data */
  pCstrInput->ExpectID_t(CChunkID("data"));      // ID "data"
  // read Data length (in bytes)
  (*pCstrInput) >> pwi_ulDataLength;

  /* Check PCM format */
  CheckWaveFormat_t(pwi_wfeWave, "PCM Wave Input (input)");

  // mark Info loaded
  pwi_bInfoLoaded = TRUE;

  // ASSERT( pwi_wfeWave.wBitsPerSample==16);
  // return Wave Format
  return pwi_wfeWave;
}


/*
 *  Load and convert Wave data
 */
void PCMWaveInput::LoadData_t(CTStream *pCstrInput, SWORD *pswMemory, WAVEFORMATEX &SwfeDesired)
{
  // if info not loaded -> exception
  if (!pwi_bInfoLoaded) {
    throw (TRANS("PCM Wave Input: Info not loaded."));
  }
  // if already loaded -> exception
  if (pwi_bDataLoaded) {
    throw (TRANS("PCM Wave Input: Data already loaded"));
  }

  // set memory pointer
  pwi_pswMemory = pswMemory;

  // store and check desired sound format
  CheckWaveFormat_t(SwfeDesired, "PCM Wave Input (desired)");
  pwi_wfeDesired = SwfeDesired;

  // calculate expand/shrink ratio (number of channels remain the same)
  pwi_dRatio = (DOUBLE)pwi_wfeDesired.nSamplesPerSec / (DOUBLE)pwi_wfeWave.nSamplesPerSec;

  // determine converion type from input and desired sound frequency, and convert sound
  if (pwi_dRatio < 1) {
    pwi_dRatio = 1/pwi_dRatio;
    ShrinkData_t(pCstrInput);
  } else if (pwi_dRatio > 1) {
    ASSERTALWAYS("Can't expand wave data");
    memset(pwi_pswMemory, 0, DetermineBufferSize(pwi_wfeDesired));
  // copy data
  } else {
    ASSERT(pwi_dRatio==1.0f);
    CopyData_t(pCstrInput);
  }

  // data is loaded (and maybe converted from 16-bits)
  if( pwi_wfeWave.wBitsPerSample==8) SwfeDesired.nBlockAlign *= 2; 
  pwi_bDataLoaded = TRUE;
}



/*
 *  Length in bytes
 */
ULONG PCMWaveInput::GetByteLength(void)
{
  ASSERT(pwi_bInfoLoaded);
  return pwi_ulDataLength;
}

/*
 *  Length in blocks
 */
ULONG PCMWaveInput::GetDataLength(void)
{
  ASSERT(pwi_bInfoLoaded);
  return GetByteLength() / (pwi_wfeWave.nChannels * pwi_wfeWave.wBitsPerSample/8);
}


ULONG PCMWaveInput::GetDataLength(WAVEFORMATEX SwfeDesired)
{
  ASSERT(pwi_bInfoLoaded);
  // return buffer size
  return DetermineBufferSize(SwfeDesired) / (SwfeDesired.nChannels * SwfeDesired.wBitsPerSample/8);
}

/*
 *  Length in seconds
 */
DOUBLE PCMWaveInput::GetSecondsLength(void)
{
  ASSERT(pwi_bInfoLoaded);
  return (DOUBLE)GetDataLength() / (DOUBLE)pwi_wfeWave.nSamplesPerSec;
}


/*
 *  Buffer length in bytes
 */
ULONG PCMWaveInput::DetermineBufferSize(void)
{
  return DetermineBufferSize(pwi_wfeWave);
}


ULONG PCMWaveInput::DetermineBufferSize( WAVEFORMATEX SwfeDesired)
{
  ASSERT(pwi_bInfoLoaded);
  DOUBLE dRatio;

  // calculate ratio between formats
  dRatio = (DOUBLE)SwfeDesired.nSamplesPerSec / (DOUBLE)pwi_wfeWave.nSamplesPerSec
         * (DOUBLE)SwfeDesired.wBitsPerSample / (DOUBLE)pwi_wfeWave.wBitsPerSample;
  // return buffer size (must calculate with data length to avoid miss align data, for example:
  // 16 bit sound with 2 channels must be aligned to 4 bytes boundary and a multiply with
  // random ratio can as result give any possible number
  DOUBLE ret = ceil(dRatio*GetDataLength()) * (pwi_wfeWave.nChannels*(pwi_wfeWave.wBitsPerSample/8));
  return (ULONG)ret;
}
