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
#include <Engine/Base/Timer.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/CRC.h>
#include <Engine/Math/Functions.h>
#include <Engine/Network/Diff.h>

#include <Engine/Templates/StaticStackArray.cpp>

#define DIFF_OLD  0   // copy from old file
#define DIFF_NEW  1   // copy from new file
#define DIFF_XOR  2   // xor between an old block and a new block

UBYTE *_pubOld = NULL;
SLONG _slSizeOld = 0;
UBYTE *_pubNew = NULL;
SLONG _slSizeNew = 0;
ULONG _ulCRC;

CTStream *_pstrmOut;

// emit one block copied from old file
void EmitOld_t(SLONG slOffsetOld, SLONG slSizeOld)
{
  (*_pstrmOut)<<UBYTE(DIFF_OLD);
  (*_pstrmOut)<<slOffsetOld;
  (*_pstrmOut)<<slSizeOld;
}
// emit one block copied from new file
void EmitNew_t(SLONG slOffsetNew, SLONG slSizeNew)
{
  (*_pstrmOut)<<UBYTE(DIFF_NEW);
  (*_pstrmOut)<<slSizeNew;
  (*_pstrmOut).Write_t(_pubNew+slOffsetNew, slSizeNew);
}

// emit one block xor-ed between new and old file
void EmitXor_t(SLONG slOffsetOld, SLONG slSizeOld, SLONG slOffsetNew, SLONG slSizeNew)
{
  // xor it
  SLONG slSizeXor = Min(slSizeOld, slSizeNew);
  UBYTE *pub0 = _pubOld+slOffsetOld;
  UBYTE *pub1 = _pubNew+slOffsetNew;
  for (INDEX i=0; i<slSizeXor; i++) {
    *pub1++ ^= *pub0++;
  }

  // emit it
  (*_pstrmOut)<<UBYTE(DIFF_XOR);
  (*_pstrmOut)<<slOffsetOld;
  (*_pstrmOut)<<slSizeOld;
  (*_pstrmOut)<<slSizeNew;
  (*_pstrmOut).Write_t(_pubNew+slOffsetNew, slSizeNew);
}

struct EntityBlockInfo {
  ULONG ebi_ulID;
  SLONG ebi_slOffset;
  SLONG ebi_slSize;
};

CStaticStackArray<EntityBlockInfo> _aebiOld;
CStaticStackArray<EntityBlockInfo> _aebiNew;


#define ENT4    0x34544E45 // looks like "ENT4" in ASCII.

// make array of entity offsets in a block
void MakeInfos(CStaticStackArray<EntityBlockInfo> &aebi, 
               UBYTE *pubBlock, SLONG slSize, UBYTE *pubFirst, UBYTE *&pubEnd)
{
  // clear all offsets
  aebi.PopAll();

  // until end of block
  UBYTE *pub = pubFirst;
  while (pub<pubBlock+slSize) {
    // if no more entities
    if (*(ULONG*)pub != ENT4) {
      pubEnd = pub;
      // stop
      return;
    }
    // remember it
    EntityBlockInfo &ebi = aebi.Push();
    ebi.ebi_slOffset = pub-pubBlock;
    pub+=sizeof(ULONG);

    // get id and size
    ULONG ulID = *(ULONG*)pub;
    ebi.ebi_ulID     = ulID;
    pub+=sizeof(ULONG);

    SLONG slSizeChunk = *(SLONG*)pub;
    pub+=sizeof(ULONG);
    ebi.ebi_slSize   = slSizeChunk+sizeof(SLONG)*3;

    pub+=slSizeChunk;
  }
}

// find first entity in given block
UBYTE *FindFirstEntity(UBYTE *pubBlock, SLONG slSize)
{
  UBYTE *pub = pubBlock;
  while (pub<pubBlock+slSize) {
    if (*(ULONG*)pub == ENT4) {
      UBYTE *pubTmp = pub;
      pubTmp+=sizeof(ULONG);
      //ULONG ulID = *(ULONG*)pubTmp;
      pubTmp+=sizeof(ULONG);
      SLONG slSizeChunk = *(SLONG*)pubTmp;
      pubTmp+=sizeof(ULONG);
      if (*(ULONG*)(pubTmp+slSizeChunk) == ENT4) {
        return pub;
      }
    }
    pub++;
  }
  return NULL;
}

void MakeDiff_t(void)
{
  // write header with size of files
  (*_pstrmOut).WriteID_t("DIFF");
  (*_pstrmOut)<<_slSizeOld<<_slSizeNew<<_ulCRC;

  // find first entities in blocks
  UBYTE *pubOldEnts = FindFirstEntity(_pubOld, _slSizeOld);
  UBYTE *pubNewEnts = FindFirstEntity(_pubNew, _slSizeNew);
  if (pubOldEnts==NULL || pubNewEnts==NULL) {
    ThrowF_t(TRANS("Invalid stream for Diff!"));
  }

  // make arrays of entity offsets
  UBYTE *pubEntEndOld;
  MakeInfos(_aebiOld, _pubOld, _slSizeOld, pubOldEnts, pubEntEndOld);
  UBYTE *pubEntEndNew;
  MakeInfos(_aebiNew, _pubNew, _slSizeNew, pubNewEnts, pubEntEndNew);

  // emit chunk before entities by xor
  EmitXor_t(0, pubOldEnts-_pubOld, 0, pubNewEnts-_pubNew);

  // for each entity in new
  for(INDEX ieibNew = 0; ieibNew<_aebiNew.Count(); ieibNew++) {
    EntityBlockInfo &ebiNew = _aebiNew[ieibNew];
    // find same in old file
    INDEX ieibOld = -1;
    for(INDEX i=0; i<_aebiOld.Count(); i++) {
      if (_aebiOld[i].ebi_ulID==ebiNew.ebi_ulID) {
        ieibOld = i;
        break;
      }
    }
    BOOL bDone = FALSE;

    // if found
    if (ieibOld>=0) {
      EntityBlockInfo &ebiOld = _aebiOld[ieibOld];

      // if same
      if ( ebiOld.ebi_slSize==ebiNew.ebi_slSize) {
        if (memcmp(_pubOld+ebiOld.ebi_slOffset, 
        _pubNew+ebiNew.ebi_slOffset, ebiNew.ebi_slSize)==0) {
          //CPrintF("Same blocks\n");
          // emit copy from old
          EmitOld_t(ebiOld.ebi_slOffset, ebiOld.ebi_slSize);
          bDone = TRUE;
        } else {
          //CPrintF("Different blocks\n");
        }
      } else {
        //CPrintF("Different sizes\n");
      }

      if (!bDone) {
        // emit xor
        EmitXor_t(
          ebiOld.ebi_slOffset, ebiOld.ebi_slSize,
          ebiNew.ebi_slOffset, ebiNew.ebi_slSize);
        bDone = TRUE;
      }
    } else {
      //CPrintF("Not found\n");
    }
    if (!bDone) 
    {
      // emit from new
      EmitNew_t(ebiNew.ebi_slOffset, ebiNew.ebi_slSize);
      bDone = TRUE;
    }
  }

  // emit chunk after entities by xor
  EmitXor_t(
    pubEntEndOld-_pubOld, _pubOld+_slSizeOld-pubEntEndOld,
    pubEntEndNew-_pubNew, _pubNew+_slSizeNew-pubEntEndNew);
}


#define DIFF 0x46464944   //  looks like "DIFF" in ASCII.

void UnDiff_t(void)
{
  // start at beginning
  //UBYTE *pubOld = _pubOld;
  UBYTE *pubNew = _pubNew;
  SLONG slSizeOldStream = 0;
  //SLONG slSizeOutStream = 0;
  // get header with size of files
  if (*(SLONG*)pubNew!=DIFF) {
    ThrowF_t(TRANS("Not a DIFF stream!"));
  }
  pubNew+=sizeof(SLONG);
  slSizeOldStream = *(SLONG*)pubNew; pubNew+=sizeof(SLONG);
  /* slSizeOutStream = *(SLONG*)pubNew; */ pubNew+=sizeof(SLONG);
  ULONG ulCRC =  *(ULONG*)pubNew; pubNew+=sizeof(ULONG);

  CRC_Start(_ulCRC);

  if (slSizeOldStream!=_slSizeOld) {
    ThrowF_t(TRANS("Invalid DIFF stream!"));
  }
  // while not end of diff file
  while (pubNew<_pubNew+_slSizeNew) {
    // read block type
    UBYTE ubType = *(pubNew++);
    switch(ubType) {
    // if block type is 'copy from old file'
    case DIFF_OLD: {
      // get data offset and size
      SLONG slOffsetOld = *(SLONG*)pubNew;  pubNew+=sizeof(SLONG);
      SLONG slSizeOld = *(SLONG*)pubNew;    pubNew+=sizeof(SLONG);
      // copy it from there
      (*_pstrmOut).Write_t(_pubOld+slOffsetOld, slSizeOld);
      CRC_AddBlock(_ulCRC, _pubOld+slOffsetOld, slSizeOld);
                   } break;
    // if block type is 'copy from new file'
    case DIFF_NEW: {
      // get data size
      SLONG slSizeNew = *(SLONG*)pubNew;    pubNew+=sizeof(SLONG);
      // copy it from there
      (*_pstrmOut).Write_t(pubNew, slSizeNew);
      CRC_AddBlock(_ulCRC, pubNew, slSizeNew);
      pubNew+=slSizeNew;
                   } break;
    // if block type is 'xor between an old block and a new block'
    case DIFF_XOR: {
      // get data offset and sizes
      SLONG slOffsetOld = *(SLONG*)pubNew;  pubNew+=sizeof(SLONG);
      SLONG slSizeOld = *(SLONG*)pubNew;    pubNew+=sizeof(SLONG);
      SLONG slSizeNew = *(SLONG*)pubNew;    pubNew+=sizeof(SLONG);

      // xor it
      SLONG slSizeXor = Min(slSizeOld, slSizeNew);
      UBYTE *pub0 = _pubOld+slOffsetOld;
      UBYTE *pub1 = pubNew;

      for (INDEX i=0; i<slSizeXor; i++) {
        *(pub1++) ^= *(pub0++);
      }

      // copy the xor-ed data
      (*_pstrmOut).Write_t(pubNew, slSizeNew);
      CRC_AddBlock(_ulCRC, pubNew, slSizeNew);
      pubNew+=slSizeNew;
                   } break;
    default:
      ThrowF_t(TRANS("Invalid DIFF block type!"));
    }
  }

  CRC_Finish(_ulCRC);

//printf("CRC is (%lu), expected (%lu).\n", _ulCRC, ulCRC);

  if (_ulCRC!=ulCRC) {
    ThrowF_t(TRANS("CRC error in DIFF!"));
  }
}

void Cleanup(void)
{
  if (_pubOld!=NULL) {
    FreeMemory(_pubOld);
  }

  if (_pubNew!=NULL) {
    FreeMemory(_pubNew);
  }
  _pubOld = NULL;
  _pubNew = NULL;
}

// make a difference file from two saved games
void DIFF_Diff_t(CTStream *pstrmOld, CTStream *pstrmNew, CTStream *pstrmDiff)
{
  try {
    //CTimerValue tv0 = _pTimer->GetHighPrecisionTimer();

    _slSizeOld = pstrmOld->GetStreamSize()-pstrmOld->GetPos_t();
    _pubOld = (UBYTE*)AllocMemory(_slSizeOld);
    pstrmOld->Read_t(_pubOld, _slSizeOld);

    _slSizeNew = pstrmNew->GetStreamSize()-pstrmNew->GetPos_t();
    _pubNew = (UBYTE*)AllocMemory(_slSizeNew);
    pstrmNew->Read_t(_pubNew, _slSizeNew);

    CRC_Start(_ulCRC);
    CRC_AddBlock(_ulCRC, _pubNew, _slSizeNew);
    CRC_Finish(_ulCRC);

    _pstrmOut = pstrmDiff;

    MakeDiff_t();

    //CTimerValue tv1 = _pTimer->GetHighPrecisionTimer();
    //CPrintF("diff encoded in %.2gs\n", (tv1-tv0).GetSeconds());

    Cleanup();

  } catch (const char *) {
    Cleanup();
    throw;
  }
}

// make a new saved game from difference file and old saved game
void DIFF_Undiff_t(CTStream *pstrmOld, CTStream *pstrmDiff, CTStream *pstrmNew)
{
  try {
    //CTimerValue tv0 = _pTimer->GetHighPrecisionTimer();

    _slSizeOld = pstrmOld->GetStreamSize()-pstrmOld->GetPos_t();
    _pubOld = (UBYTE*)AllocMemory(_slSizeOld);
    pstrmOld->Read_t(_pubOld, _slSizeOld);

    _slSizeNew = pstrmDiff->GetStreamSize()-pstrmDiff->GetPos_t();
    _pubNew = (UBYTE*)AllocMemory(_slSizeNew);
    pstrmDiff->Read_t(_pubNew, _slSizeNew);

    _pstrmOut = pstrmNew;

    UnDiff_t();

    //CTimerValue tv1 = _pTimer->GetHighPrecisionTimer();
    //CPrintF("diff decoded in %.2gs\n", (tv1-tv0).GetSeconds());

    Cleanup();

  } catch (const char *) {
    Cleanup();
    throw;
  }
}
