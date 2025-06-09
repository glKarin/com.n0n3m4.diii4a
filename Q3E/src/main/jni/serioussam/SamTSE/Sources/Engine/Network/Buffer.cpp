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

#include <Engine/Math/Functions.h>
#include <Engine/Base/Memory.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Stream.h>
#include <Engine/Network/Buffer.h>

// default constructor
CBuffer::CBuffer(void)
{
  bu_slAllocationStep = 1024;
  bu_slWriteOffset = 0;
  bu_slReadOffset = 0;

  bu_slFree = 0;
  bu_slSize = 0;
  bu_pubBuffer = NULL;
}

// destructor
CBuffer::~CBuffer(void)
{
  Clear();
}

// free buffer
void CBuffer::Clear(void)
{
  bu_slWriteOffset = 0;
  bu_slReadOffset = 0;

  if (bu_slSize>0) {
    ASSERT(bu_pubBuffer!=NULL);
    FreeMemory(bu_pubBuffer);
  }
  bu_slFree = 0;
  bu_slSize = 0;
  bu_pubBuffer = NULL;
}

// expand buffer to be given number of bytes in size
void CBuffer::Expand(SLONG slNewSize)
{
  ASSERT(slNewSize>0);
  ASSERT(bu_slSize>=0);
  // if not already allocated
  if (bu_slSize==0) {
    // allocate a new empty buffer
    ASSERT(bu_pubBuffer==NULL);
    bu_pubBuffer = (UBYTE*)AllocMemory(slNewSize);
    bu_slWriteOffset = 0;
    bu_slReadOffset = 0;
    bu_slFree = slNewSize;
    bu_slSize = slNewSize;

    // if already allocated
  } else {
    ASSERT(slNewSize>bu_slSize);
    SLONG slSizeDiff = slNewSize-bu_slSize;
    ASSERT(bu_pubBuffer!=NULL);
    // grow buffer
    GrowMemory((void**)&bu_pubBuffer, slNewSize);

    // if buffer is currently wrapping
    if (bu_slReadOffset>bu_slWriteOffset||bu_slFree==0) {
      // move part at the end of buffer to the end
      memmove(bu_pubBuffer+bu_slReadOffset+slSizeDiff, bu_pubBuffer+bu_slReadOffset,
        bu_slSize-bu_slReadOffset);
      bu_slReadOffset+=slSizeDiff;
    }
    bu_slFree += slNewSize-bu_slSize;
    bu_slSize = slNewSize;

    ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
    ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);
  }
}

// set how many bytes to add when buffer overflows
void CBuffer::SetAllocationStep(SLONG slStep)
{
  ASSERT(slStep>0);
  bu_slAllocationStep = slStep;
}


#ifndef __min
#define __min(x, y) ((x) < (y) ? (x) : (y))
#endif

// read bytes from buffer
SLONG CBuffer::ReadBytes(void *pv, SLONG slSize)
{
  ASSERT(slSize>0 && pv!=NULL);
  UBYTE *pub = (UBYTE*)pv;

  // clamp size to amount of bytes actually in the buffer
  SLONG slUsed = bu_slSize-bu_slFree;
  if (slUsed<slSize) {
    slSize = slUsed;
  }
  // if there is nothing to read
  if (slSize==0) {
    // do nothing
    return 0;
  }

  // read part of block after read pointer to the end of buffer
  SLONG slSizeEnd = __min(bu_slSize-bu_slReadOffset, slSize);
  memcpy(pub, bu_pubBuffer+bu_slReadOffset, slSizeEnd);
  pub+=slSizeEnd;
  // if that is not all
  if (slSizeEnd<slSize) {
    // read rest from start of buffer
    memcpy(pub, bu_pubBuffer, slSize-slSizeEnd);
  }
  // move read pointer
  bu_slReadOffset+=slSize;
  bu_slReadOffset%=bu_slSize;
  bu_slFree+=slSize;

  ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);

  return slSize;
}

// skip bytes from buffer (read without actually reading)
SLONG CBuffer::SkipBytes(SLONG slSize)
{
  ASSERT(slSize>0);

  // clamp size to amount of bytes actually in the buffer
  SLONG slUsed = bu_slSize-bu_slFree;
  if (slUsed<slSize) {
    slSize = slUsed;
  }
  // if there is nothing to skip
  if (slSize==0) {
    // do nothing
    return 0;
  }

  // move read pointer
  bu_slReadOffset+=slSize;
  bu_slReadOffset%=bu_slSize;
  bu_slFree+=slSize;

  ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);

  return slSize;
}

// read bytes from buffer to stream
SLONG CBuffer::ReadBytesToStream(CTStream &strm, SLONG slSize)
{
  ASSERT(slSize>0);

  // clamp size to amount of bytes actually in the buffer
  SLONG slUsed = bu_slSize-bu_slFree;
  if (slUsed<slSize) {
    slSize = slUsed;
  }
  // if there is nothing to read
  if (slSize==0) {
    // do nothing
    return 0;
  }

  // read part of block after read pointer to the end of buffer
  SLONG slSizeEnd = __min(bu_slSize-bu_slReadOffset, slSize);
  strm.Write_t(bu_pubBuffer+bu_slReadOffset, slSizeEnd);
  // if that is not all
  if (slSizeEnd<slSize) {
    // read rest from start of buffer
    strm.Write_t(bu_pubBuffer, slSize-slSizeEnd);
  }
  // move read pointer
  bu_slReadOffset+=slSize;
  bu_slReadOffset%=bu_slSize;
  bu_slFree+=slSize;

  ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);

  return slSize;
}

// unread bytes from buffer
void CBuffer::UnreadBytes(SLONG slSize)
{
  ASSERT(bu_slFree>=slSize);

  if (slSize==0) return;
  bu_slReadOffset-=slSize;
  bu_slReadOffset%=bu_slSize;
  if (bu_slReadOffset<0) {
    bu_slReadOffset+=bu_slSize;
  }
  bu_slFree-=slSize;

  ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);
}

// check how many bytes are there to read
SLONG CBuffer::QueryReadBytes(void)
{
  // return amount of bytes actually in the buffer
  return bu_slSize-bu_slFree;
}

// write bytes to buffer
void CBuffer::WriteBytes(const void *pv, SLONG slSize)
{
  ASSERT(slSize>=0 && pv!=NULL);
  // if there is nothing to write
  if (slSize==0) {
    // do nothing
    return;
  }
  // check for errors
  if (slSize<0) {
    CPrintF("WARNING: WriteBytes(): slSize<0\n!");
    return;
  }

  // if there is not enough free space
  if (bu_slFree<slSize) {
    // expand the buffer
    Expand(bu_slSize+
      ((slSize-bu_slFree+bu_slAllocationStep-1)/bu_slAllocationStep)*bu_slAllocationStep );
    ASSERT(bu_slFree>=slSize);
  }

  UBYTE *pub = (UBYTE*)pv;

  // write part of block at the end of buffer
  SLONG slSizeEnd = __min(bu_slSize-bu_slWriteOffset, slSize);
  memcpy(bu_pubBuffer+bu_slWriteOffset, pub, slSizeEnd);
  pub+=slSizeEnd;
  memcpy(bu_pubBuffer, pub, slSize-slSizeEnd);
  // move write pointer
  bu_slWriteOffset+=slSize;
  bu_slWriteOffset%=bu_slSize;
  bu_slFree-=slSize;

  ASSERT(bu_slWriteOffset>=0 && bu_slWriteOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);
}

// move all data from another buffer to this one
void CBuffer::MoveBuffer(CBuffer &buFrom)
{
  // repeat
  for(;;){
    // read a block from the other buffer
    UBYTE aub[256];
    SLONG slSize = buFrom.ReadBytes(aub, sizeof(aub));
    // if nothing read
    if (slSize<=0) {
      // stop
      return;
    }
    // write here what was read
    WriteBytes(&aub, slSize);
  }
}

void CBlockBufferStats::Clear(void)
{
  bbs_tvTimeUsed.Clear();
}

// get time when block of given size will be finished if started now
CTimerValue CBlockBufferStats::GetBlockFinalTime(SLONG slSize)
{
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

  // calculate how much should block be delayed due to latency and due to bandwidth
  CTimerValue tvBandwidth;
  if (bbs_fBandwidthLimit<=0.0f) {
    tvBandwidth = CTimerValue(0.0);
  } else {
    tvBandwidth = CTimerValue(DOUBLE((slSize*8)/bbs_fBandwidthLimit));
  }
  CTimerValue tvLatency;
  if (bbs_fLatencyLimit<=0.0f && bbs_fLatencyVariation<=0.0f) {
   tvLatency = CTimerValue(0.0);
  } else {
   tvLatency = CTimerValue(DOUBLE(bbs_fLatencyLimit+(bbs_fLatencyVariation*rand())/(float)(RAND_MAX)));
  }

  // start of packet receiving is later of
  CTimerValue tvStart(
    Max(
      // current time plus latency and
      (tvNow+tvLatency).tv_llValue,
      // next free point in time
      bbs_tvTimeUsed.tv_llValue));
  // remember next free time and return it
  bbs_tvTimeUsed = tvStart+tvBandwidth;
  return bbs_tvTimeUsed;
}

// default constructor
CBlockBuffer::CBlockBuffer(void)
{
  bb_slBlockSizeRead = 0;
  bb_slBlockSizeWrite = 0;
  bb_pbbsStats = NULL;
}

// destructor
CBlockBuffer::~CBlockBuffer(void)
{
  bb_slBlockSizeRead = 0;
  bb_slBlockSizeWrite = 0;
  bb_pbbsStats = NULL;
}

// free buffer
void CBlockBuffer::Clear(void)
{
  bb_slBlockSizeRead = 0;
  bb_slBlockSizeWrite = 0;
  bb_pbbsStats = NULL;
  CBuffer::Clear();
}

#ifdef NETSTRUCTS_PACKED
  #pragma pack(1)
#endif

struct BlockHeader {
  SLONG bh_slSize;              // block size

  #ifdef NETSTRUCTS_PACKED
    UBYTE packing[4];
  #endif

  CTimerValue bh_tvFinalTime;   // block may be read only after this moment in time
};

#ifdef NETSTRUCTS_PACKED
  #pragma pack()
#endif

// read one block if possible
BOOL CBlockBuffer::ReadBlock(void *pv, SLONG &slSize)
{
  // must not be inside block reading
  ASSERT(bb_slBlockSizeRead==0);

  // read header of next block in incoming buffer

// rcg10272001 !!! FIXME: Taking sizeof (bh), with the intention of
// rcg10272001 !!! FIXME:  sending that many bytes over the network,
// rcg10272001 !!! FIXME:  is really, really risky. DON'T DO IT.
// rcg10272001 !!! FIXME:  Instead, send pertinent information field by
// rcg10272001 !!! FIXME:  field, and rebuild the structure on the other
// rcg10272001 !!! FIXME:  side, swapping byte order as necessary.

  struct BlockHeader bh;
  SLONG slbhSize;
  slbhSize = ReadBytes(&bh, sizeof(bh));

  // if the header information is not in buffer
  if (static_cast<size_t>(slbhSize) < sizeof(bh)) {
    // unwind
    UnreadBytes(slbhSize);
    // nothing to receive
    return FALSE;
  }

  // if the block has not yet been received
  if (QueryReadBytes() < bh.bh_slSize) {
    // unwind
    UnreadBytes(slbhSize);
    // nothing to receive
    return FALSE;
  }

  // if there is too much data for the receiving memory space
  if (bh.bh_slSize > slSize) {
    // unwind
    UnreadBytes(slbhSize);
    // mark how much space we would need
    slSize = bh.bh_slSize;
    // nothing to receive
    ASSERT(FALSE);  // this shouldn't happen
    return FALSE;
  }

  // if using stats
  if (bb_pbbsStats!=NULL) {
    // if block could not have been received yet, due to time limits
    if (bh.bh_tvFinalTime>_pTimer->GetHighPrecisionTimer()) {
      // unwind
      UnreadBytes(slbhSize);
      // nothing to receive
      return FALSE;
    }
  }

  // read the block
  slSize = ReadBytes(pv, bh.bh_slSize);
  ASSERT(slSize == bh.bh_slSize);

  // received
  return TRUE;
}

// read one block from buffer to stream
BOOL CBlockBuffer::ReadBlockToStream(CTStream &strm)
{
  // must not be inside block reading
  ASSERT(bb_slBlockSizeRead==0);

  // read header of next block in incoming buffer
  struct BlockHeader bh;
  SLONG slbhSize;
  slbhSize = ReadBytes(&bh, sizeof(bh));

  // if the header information is not in buffer
  if (static_cast<size_t>(slbhSize) < sizeof(bh)) {
    // unwind
    UnreadBytes(slbhSize);
    // nothing to receive
    return FALSE;
  }

  // if the block has not yet been received
  if (QueryReadBytes() < bh.bh_slSize) {
    // unwind
    UnreadBytes(slbhSize);
    // nothing to receive
    return FALSE;
  }

  // if using stats
  if (bb_pbbsStats!=NULL) {
    // if block could not have been received yet, due to time limits
    if (bh.bh_tvFinalTime>_pTimer->GetHighPrecisionTimer()) {
      // unwind
      UnreadBytes(slbhSize);
      // nothing to receive
      return FALSE;
    }
  }

  // read from buffer to destination buffer
  try {
    SLONG slSize = ReadBytesToStream(strm, bh.bh_slSize);
    ASSERT(slSize == bh.bh_slSize);
  } catch (const char *strError) {
    ASSERT(FALSE);
    CPrintF(TRANSV("Buffer error reading to stream: %s\n"), strError);
    return FALSE;
  }

  return TRUE;
}

// write one block
void CBlockBuffer::WriteBlock(const void *pv, SLONG slSize)
{
  // must not be inside block writing
  ASSERT(bb_slBlockSizeWrite==0);

  // prepare block header
  struct BlockHeader bh;
  bh.bh_slSize = slSize;
  if (bb_pbbsStats!=NULL) {
    bh.bh_tvFinalTime = bb_pbbsStats->GetBlockFinalTime(slSize);
  } else {
    bh.bh_tvFinalTime.Clear();
  }
  // write the data to send-buffer
  WriteBytes((void*)&bh, sizeof(bh));
  WriteBytes(pv, slSize);
}

// unread one block
void CBlockBuffer::UnreadBlock(SLONG slSize)
{
  UnreadBytes(slSize+sizeof(struct BlockHeader));
}

// read raw block data
SLONG CBlockBuffer::ReadRawBlock(void *pv, SLONG slSize)
{
  // if inside block reading
  if(bb_slBlockSizeRead>0) {
    // clamp size to prevent reading across real blocks
    slSize = Min(slSize, bb_slBlockSizeRead);

    // read the raw block
    SLONG slResult = ReadBytes(pv, slSize);
    ASSERT(slResult==slSize);
    // decrement block size counter
    bb_slBlockSizeRead-=slResult;
    // must not underflow
    ASSERT(bb_slBlockSizeRead>=0);
    return slResult;

  // if not inside block reading
  } else {
    // read header of next block in incoming buffer
    struct BlockHeader bh;
    SLONG slbhSize;
    slbhSize = ReadBytes(&bh, sizeof(bh));

    // if the header information is not in buffer
    if (static_cast<size_t>(slbhSize) < sizeof(bh)) {
      // unwind
      UnreadBytes(slbhSize);
      // nothing to receive
      return FALSE;
    }

    // if the block has not yet been received
    if (QueryReadBytes() < bh.bh_slSize) {
      // unwind
      UnreadBytes(slbhSize);
      // nothing to receive
      return FALSE;
    }

    // if using stats
    if (bb_pbbsStats!=NULL) {
      // if block could not have been received yet, due to time limits
      if (bh.bh_tvFinalTime>_pTimer->GetHighPrecisionTimer()) {
        // unwind
        UnreadBytes(slbhSize);
        // nothing to receive
        return FALSE;
      }
    }

    // remember block size counter
    bb_slBlockSizeRead = bh.bh_slSize+sizeof(struct BlockHeader);
    // unwind header
    UnreadBytes(slbhSize);

    // clamp size to prevent reading across real blocks
    slSize = Min(slSize, bb_slBlockSizeRead);

    // read the raw block with header
    SLONG slResult = ReadBytes(pv, slSize);
    ASSERT(slResult==slSize);
    // decrement block size counter
    bb_slBlockSizeRead-=slResult;
    // must not underflow
    ASSERT(bb_slBlockSizeRead>=0);

    return slResult;
  }
}

// write raw block data
void CBlockBuffer::WriteRawBlock(const void *pv, SLONG slSize)
{
  // while there is something to write
  while (slSize>0) {

    // if inside block writing
    if(bb_slBlockSizeWrite>0) {
      SLONG slToWrite = Min(bb_slBlockSizeWrite, slSize);
      // write the raw block
      WriteBytes(pv, slToWrite);
      slSize-=slToWrite;
      ((UBYTE*&)pv)+=slToWrite;
      // decrement block size counter
      bb_slBlockSizeWrite-=slToWrite;
      // must not underflow
      ASSERT(bb_slBlockSizeWrite>=0);

    // if not inside block writing
    } else {
      // must contain at least the header
      ASSERT(slSize>sizeof(struct BlockHeader));
      // find the header in the raw block
      struct BlockHeader &bh = *(struct BlockHeader*)pv;
      // remember block size counter
      bb_slBlockSizeWrite = bh.bh_slSize+sizeof(struct BlockHeader);
      // create new block timestamp
      if (bb_pbbsStats!=NULL) {
        bh.bh_tvFinalTime = bb_pbbsStats->GetBlockFinalTime(bb_slBlockSizeWrite);
      } else {
        bh.bh_tvFinalTime.Clear();
      }

      SLONG slToWrite = Min(bb_slBlockSizeWrite, slSize);
      // write the raw block, with the new header
      WriteBytes(pv, slToWrite);
      slSize-=slToWrite;
      ((UBYTE*&)pv)+=slToWrite;
      // decrement block size counter
      bb_slBlockSizeWrite-=slToWrite;
      // must not underflow
      ASSERT(bb_slBlockSizeWrite>=0);
    }
  }
}

// peek sizes of next block
void CBlockBuffer::PeekBlockSize(SLONG &slExpectedSize, SLONG &slReceivedSoFar)
{
  // if inside block reading
  if(bb_slBlockSizeRead>0) {
    // no information available
    slExpectedSize = 0;
    slReceivedSoFar = 0;

  // if not inside block reading
  } else {
    // read header of next block in incoming buffer
    struct BlockHeader bh;
    SLONG slbhSize;
    slbhSize = ReadBytes(&bh, sizeof(bh));
    // unwind
    UnreadBytes(slbhSize);

    // if the header information is not in buffer
    if (static_cast<size_t>(slbhSize) < sizeof(bh)) {
      // no information available
      slExpectedSize = 0;
      slReceivedSoFar = 0;
    // if the header information is present
    } else {
      // total size is size of block
      slExpectedSize = bh.bh_slSize;
      // received so far is how much is really present
      slReceivedSoFar = QueryReadBytes()-sizeof(struct BlockHeader);
    }
  }
}

// unread raw block data
void CBlockBuffer::UnreadRawBlock(SLONG slSize)
{
  bb_slBlockSizeRead+=slSize;
  UnreadBytes(slSize);
}

// move all data from another buffer to this one
void CBlockBuffer::MoveBlockBuffer(CBlockBuffer &buFrom)
{
  // repeat
  for(;;){
    // read a block from the other buffer
    UBYTE aub[256];
    SLONG slSize = buFrom.ReadRawBlock(aub, sizeof(aub));
    // if nothing read
    if (slSize<=0) {
      // stop
      return;
    }
    // write here what was read
    WriteRawBlock(&aub, slSize);
  }
}
