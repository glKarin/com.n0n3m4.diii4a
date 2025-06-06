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
#include <Engine/Network/Compression.h>
#include <Engine/Base/Synchronization.h>

#if defined(PLATFORM_UNIX) && !SE1_ZLIB
#include <zlib.h>
#else
#include <Engine/zlib/zlib.h>
#endif

extern CTCriticalSection zip_csLock; // critical section for access to zlib functions

/* Unpack from stream to stream. */
void CCompressor::UnpackStream_t(CTMemoryStream &strmSrc, CTStream &strmDst) // throw char *
{
  // read the header
  SLONG slSizeDst, slSizeSrc;
  strmSrc>>slSizeDst;
  strmSrc>>slSizeSrc;
  // get the buffer of source stream
  UBYTE *pubSrc = strmSrc.mstrm_pubBuffer + strmSrc.mstrm_slLocation;
  // allocate buffer for decompression
  UBYTE *pubDst = (UBYTE*)AllocMemory(slSizeDst);
  // compress there
  BOOL bOk = Unpack(pubSrc, slSizeSrc, pubDst, slSizeDst);
  // if failed
  if (!bOk) {
    // report error
    FreeMemory(pubDst);
    ThrowF_t(TRANS("Error while unpacking a stream."));
  }

  // write the uncompressed data to destination
  strmDst.Write_t(pubDst, slSizeDst);
  strmDst.SetPos_t(0);
  FreeMemory(pubDst);
}

void CCompressor::PackStream_t(CTMemoryStream &strmSrc, CTStream &strmDst) // throw char *
{
  // get the buffer of source stream
  UBYTE *pubSrc = strmSrc.mstrm_pubBuffer + strmSrc.mstrm_slLocation;
  SLONG slSizeSrc = strmSrc.GetStreamSize();
  // allocate buffer for compression
  SLONG slSizeDst = NeededDestinationSize(slSizeSrc);
  UBYTE *pubDst = (UBYTE*)AllocMemory(slSizeDst);
  // compress there
  BOOL bOk = Pack(pubSrc, slSizeSrc, pubDst, slSizeDst);
  // if failed
  if (!bOk) {
    // report error
    FreeMemory(pubDst);
    ThrowF_t(TRANS("Error while packing a stream."));
  }

  // write the header to destination
  strmDst<<slSizeSrc;
  strmDst<<slSizeDst;
  // write the compressed data to destination
  strmDst.Write_t(pubDst, slSizeDst);
  FreeMemory(pubDst);
}

/////////////////////////////////////////////////////////////////////
// RLE compressor

/*
RLE packed format(s):
NOTE:
  In order to decompress packed data, size of original data must be known, it is not
  written in the packed data by the algorithms. Depending on the usage, data size may be
  included in the header by the called function. This generally saves even more space
  if smaller chunks of known size are compressed (e.g. small networks packets).

Basic implemented packing format here is BYTE-BYTE. It packes data by bytes and uses bytes
for packing codes. Other sized RLE formats can be implemented as needed (WORD-WORD,
LONG-LONG, BYTE-WORD, WORD-BYTE etc.).

Packed data is divided in sections, each section has a leading byte as code and compressed
data following it. The data is interpreted differently, depending on the code.

  1) CODE<0 => REPLICATION
  If the code is negative, following data is one byte that is replicated given number of
  times:
  CODE DATA                 -> DATA DATA DATA ... DATA ((-CODE)+1 times)
  (note that it is not possible to encode just one byte this way, since one byte can be coded
   well with copying)
  (count = -code+1  =>  code = -count+1)

  2) CODE>=0 => COPYING
  If the code is positive, following data is given number of bytes that are just copied:
  CODE DAT0 DAT1 ... DATn   -> DAT0 DAT1 ... DATn (n=CODE)
  (note that CODE=0 means copy next one byte)
  (count = code+1  =>  code = count-1)

Packing algorithm used here relies on the destination buffer being big enough to
hold packed data. Generally, for BYTE-BYTE packing, maximum buffer is 129/128 of
original size (degenerate case with no data replication).
*/

/*
 * Calculate needed size for destination buffer when packing memory.
 */
SLONG CRLEBBCompressor::NeededDestinationSize(SLONG slSourceSize)
{
  // calculate worst case possible for size of RLEBB packed data
  // *129/128+1 would be enough, but we add some more to ensure that we don't
  // overwrite the temporary buffer
   return slSourceSize*129/128 + 5;
}

// on entry, slDstSize holds maximum size of output buffer,
// on exit, it is filled with resulting size
/* Pack a chunk of data using given compression. */
BOOL CRLEBBCompressor::Pack(const void *pvSrc, SLONG slSrcSize, void *pvDst, SLONG &slDstSize)
{
  // cannot pack zero bytes
  ASSERT(slSrcSize>=1);

  // calculate limits for source and destination buffers
  const SBYTE *pbSourceFirst = (const SBYTE *)pvSrc;            // start marker
  const SBYTE *pbSourceLimit = (const SBYTE *)pvSrc+slSrcSize;   // end marker

//  SLONG slDestinationSize=NeededDestinationSize(slSrcSize);
  SBYTE *pbDestinationFirst = (SBYTE *)pvDst;            // start marker
  SBYTE *pbDestinationLimit = (SBYTE *)pvDst+slDstSize;  // end marker

  UBYTE *pbCountFirst = (UBYTE *)pbDestinationLimit-slSrcSize; // start marker
  UBYTE *pbCountLimit = (UBYTE *)pbDestinationLimit;           // end marker

  {
    /* PASS 1: Use destination buffer to cache number of forward-same bytes. */

    // set the count of the last byte to one
    UBYTE *pbCount = pbCountLimit-1;
    *pbCount-- = 1;

    // for all bytes from one before last to the first one
    for(const SBYTE *pbSource = pbSourceLimit-2;
        pbSource>=pbSourceFirst;
        pbSource--, pbCount--) {
      // if the byte is same as its successor, and the count will fit in code
      if (pbSource[0]==pbSource[1] && (SLONG)pbCount[1]+1<=-(SLONG)MIN_SBYTE) {
        // set its count to the count of its successor plus one
        pbCount[0] = pbCount[1]+1;
      // if the byte is different than its successor
      } else {
        // set its count to one
        pbCount[0] = 1;
      }
    }
  }


  /* PASS 2: Pack bytes from source to the destination buffer. */

  // start at the beginning of the buffers
  const SBYTE *pbSource      = pbSourceFirst;
  const UBYTE *pbCount       = pbCountFirst;
  SBYTE       *pbDestination = pbDestinationFirst;

  // while there is some data to pack
  while(pbSource<pbSourceLimit) {
    ASSERT(pbCount<pbCountLimit);

    // if current byte is replicated
    if (*pbCount>1) {
      // write the replicate-packed data
      INDEX ctSameBytes = (INDEX)*pbCount;
      SLONG slCode = -ctSameBytes+1;
      ASSERT((SLONG)MIN_SBYTE<=slCode && slCode<0);
      *pbDestination++ = (SBYTE)slCode;
      *pbDestination++ = pbSource[0];
      pbSource+=ctSameBytes;
      pbCount +=ctSameBytes;
    // if current byte is not replicated
    } else {
      // count bytes to copy before encountering byte replicated more than 3 times
      INDEX ctDiffBytes=1;
      while( (ctDiffBytes < (SLONG)MAX_SBYTE + 1)
          && (&pbSource[ctDiffBytes]<pbSourceLimit) ) {
        if ((SLONG)pbCount[ctDiffBytes-1]<=3) {
          ctDiffBytes++;
        } else {
          break;
        }
      }
      // write the copy-packed data
      SLONG slCode = ctDiffBytes-1;
      ASSERT(0<=slCode && slCode<=(SLONG)MAX_SBYTE);
      *pbDestination++ = (SBYTE)slCode;
      memcpy(pbDestination, pbSource, ctDiffBytes);
      pbSource      += ctDiffBytes;
      pbCount       += ctDiffBytes;
      pbDestination += ctDiffBytes;
    }
  }
  // packing must exactly be finished now
  ASSERT(pbSource==pbSourceLimit);
  ASSERT(pbCount ==pbCountLimit);

  // calculate size of packed data
  slDstSize = pbDestination-pbDestinationFirst;
  return TRUE;
}

// on entry, slDstSize holds maximum size of output buffer,
// on exit, it is filled with resulting size
/* Unpack a chunk of data using given compression. */
BOOL CRLEBBCompressor::Unpack(const void *pvSrc, SLONG slSrcSize, void *pvDst, SLONG &slDstSize)
{
  const SBYTE *pbSource      = (const SBYTE *)pvSrc;            // current pointer
  const SBYTE *pbSourceLimit = (const SBYTE *)pvSrc+slSrcSize;  // end marker

  SBYTE *pbDestination      = (SBYTE *)pvDst;  // current pointer
  SBYTE *pbDestinationFirst = (SBYTE *)pvDst;  // start marker

  // repeat
  do {
    // get code
    SLONG slCode = *pbSource++;
    // if it is replication
    if (slCode<0) {
      // get next byte and replicate it given number of times
      INDEX ctSameBytes = -slCode+1;
      memset(pbDestination, *pbSource++, ctSameBytes);
      pbDestination += ctSameBytes;
    // if it is copying
    } else {
      // copy given number of next bytes
      INDEX ctCopyBytes = slCode+1;
      memcpy(pbDestination, pbSource, ctCopyBytes);
      pbSource      += ctCopyBytes;
      pbDestination += ctCopyBytes;
    }
  // until all data is unpacked
  } while (pbSource<pbSourceLimit);

  // data must be unpacked correctly
  ASSERT(pbSource==pbSourceLimit);

  // calculate size of data that was unpacked
  slDstSize = pbDestination-pbDestinationFirst;
  return TRUE;
}

/////////////////////////////////////////////////////////////////////
// LZRW1 compressor
// uses algorithm by Ross Williams

//#define TRUE 1

//#define UBYTE unsigned char /* Unsigned     byte (1 byte )        */
//#define UWORD unsigned int  /* Unsigned     word (2 bytes)        */
//#define ULONG unsigned long /* Unsigned longword (4 bytes)        */
#define FLAG_BYTES    1     /* Number of bytes used by copy flag. */
#define FLAG_COMPRESS 0     /* Signals that compression occurred. */
#define FLAG_COPY     1     /* Signals that a copyover occurred.  */
//void fast_copy(p_src,p_dst,len) /* Fast copy routine.             */
//UBYTE *p_src,*p_dst; {while (len--) *p_dst++=*p_src++;}
inline void fast_copy(const UBYTE *p_src, UBYTE *p_dst, SLONG len)
{
  memcpy(p_dst, p_src, len);
}

/******************************************************************************/

void lzrw1_compress(const UBYTE *p_src_first, ULONG src_len,UBYTE *p_dst_first, ULONG *p_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone (OZ).     */
/* Input  : Point p_dst_len to a ULONG to receive the output length.    */
/* Input  : Input block and output zone must not overlap.               */
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : May write in OZ=Mem[p_dst_first..p_dst_first+src_len+256-1].*/
/* Output : Upon completion guaranteed *p_dst_len<=src_len+FLAG_BYTES.  */
#define PS *p++!=*s++  /* Body of inner unrolled matching loop.         */
#define ITEMMAX 16     /* Maximum number of bytes in an expanded item.  */
{const UBYTE *p_src=p_src_first;
 UBYTE *p_dst=p_dst_first;
 const UBYTE *p_src_post=p_src_first+src_len;
 UBYTE *p_dst_post=p_dst_first+src_len;
 const UBYTE *p_src_max1=p_src_post-ITEMMAX,*p_src_max16=p_src_post-16*ITEMMAX;
 const UBYTE *hash[4096];
 memset(hash, 0, sizeof(hash));
 UBYTE *p_control; UWORD control=0,control_bits=0;
 *p_dst=FLAG_COMPRESS; p_dst+=FLAG_BYTES; p_control=p_dst; p_dst+=2;
 while (TRUE)
   {const UBYTE *p,*s; UWORD unroll=16,len,index; ULONG offset;
    if (p_dst>p_dst_post) goto overrun;
    if (p_src>p_src_max16)
      {unroll=1;
       if (p_src>p_src_max1)
         {if (p_src==p_src_post) break; goto literal;}}
    begin_unrolled_loop:
       index=((40543*((((p_src[0]<<4)^p_src[1])<<4)^p_src[2]))>>4) & 0xFFF;
       p=hash[index];
       hash[index]=s=p_src;
       offset=s-p;
       if (offset>4095 || p<p_src_first || offset==0 || PS || PS || PS)
         {literal: *p_dst++=*p_src++; control>>=1; control_bits++;}
       else
         {PS || PS || PS || PS || PS || PS || PS ||
          PS || PS || PS || PS || PS || PS || s++; len=s-p_src-1;
          *p_dst++=(UBYTE)(((offset&0xF00)>>4)+(len-1)); *p_dst++=(UBYTE)(offset&0xFF);
          p_src+=len; control=(control>>1)|0x8000; control_bits++;}
    /*end_unrolled_loop:*/ if (--unroll) goto begin_unrolled_loop;
    if (control_bits==16)
      {*p_control=control&0xFF; *(p_control+1)=control>>8;
       p_control=p_dst; p_dst+=2; control=control_bits=0;}
   }
 control>>=16-control_bits;
 *p_control++=control&0xFF; *p_control++=control>>8;
 if (p_control==p_dst) p_dst-=2;
 *p_dst_len=(p_dst-p_dst_first);
 return;
 overrun: fast_copy(p_src_first,p_dst_first+FLAG_BYTES,src_len);
          *p_dst_first=FLAG_COPY; *p_dst_len=src_len+FLAG_BYTES;
}

/******************************************************************************/

void lzrw1_decompress(const UBYTE *p_src_first, ULONG src_len, UBYTE *p_dst_first, ULONG *p_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone.          */
/* Input  : Point p_dst_len to a ULONG to receive the output length.    */
/* Input  : Input block and output zone must not overlap. User knows    */
/* Input  : upperbound on output block length from earlier compression. */
/* Input  : In any case, maximum expansion possible is eight times.     */
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : Writes only  in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
{UWORD controlbits=0, control;
 const UBYTE *p_src=p_src_first+FLAG_BYTES;
 UBYTE *p_dst=p_dst_first;
 const UBYTE *p_src_post=p_src_first+src_len;
 if (*p_src_first==FLAG_COPY)
   {fast_copy(p_src_first+FLAG_BYTES,p_dst_first,src_len-FLAG_BYTES);
    *p_dst_len=src_len-FLAG_BYTES; return;}
 while (p_src!=p_src_post)
   {if (controlbits==0)
      {control=*p_src++; control|=(*p_src++)<<8; controlbits=16;}
    if (control&1)
      {UWORD offset,len; UBYTE *p;
       offset=(*p_src&0xF0)<<4; len=1+(*p_src++&0xF);
       offset+=*p_src++&0xFF; p=p_dst-offset;
       while (len--) *p_dst++=*p++;}
    else
       *p_dst++=*p_src++;
    control>>=1; controlbits--;
   }
 *p_dst_len=p_dst-p_dst_first;
}


/*
 * Calculate needed size for destination buffer when packing memory.
 */
SLONG CLZCompressor::NeededDestinationSize(SLONG slSourceSize)
{
  // calculate worst case possible for size of LZ packed data
  return slSourceSize+256;
}

// on entry, slDstSize holds maximum size of output buffer,
// on exit, it is filled with resulting size
/* Pack a chunk of data using given compression. */
BOOL CLZCompressor::Pack(const void *pvSrc, SLONG slSrcSize, void *pvDst, SLONG &slDstSize)
{
  // this is just wrapper for original function by Ross Williams
  SLONG slDestinationSizeResult = slDstSize;
  lzrw1_compress(
    (const UBYTE *)pvSrc, (ULONG)slSrcSize,
    (UBYTE *)pvDst, (ULONG *)&slDestinationSizeResult);
  slDstSize = slDestinationSizeResult;
  return TRUE;
}

// on entry, slDstSize holds maximum size of output buffer,
// on exit, it is filled with resulting size
/* Unpack a chunk of data using given compression. */
BOOL CLZCompressor::Unpack(const void *pvSrc, SLONG slSrcSize, void *pvDst, SLONG &slDstSize)
{
  // this is just wrapper for original function by Ross Williams
  SLONG slDestinationSizeResult = slDstSize;
  lzrw1_decompress(
    (const UBYTE *)pvSrc, (ULONG)slSrcSize,
    (UBYTE *)pvDst, (ULONG *)&slDestinationSizeResult);
  slDstSize = slDestinationSizeResult;
  return TRUE;
}

/* Calculate needed size for destination buffer when packing memory. */
SLONG CzlibCompressor::NeededDestinationSize(SLONG slSourceSize)
{
  // calculate worst case possible for size of zlib packed data
  // NOTE: zlib docs state 0.1% of uncompressed size + 12 bytes, 
  // we just want to be on the safe side
  return SLONG(slSourceSize*1.1f)+32;
}

// on entry, slDstSize holds maximum size of output buffer,
// on exit, it is filled with resulting size
/* Pack a chunk of data using given compression. */
BOOL CzlibCompressor::Pack(const void *pvSrc, SLONG slSrcSize, void *pvDst, SLONG &slDstSize)
{
/*
int ZEXPORT compress (dest, destLen, source, sourceLen)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
    */

  CTSingleLock slZip(&zip_csLock, TRUE);
  uLongf dstlen = (uLongf) slDstSize;
  const int iResult = compress(
    (Bytef *)pvDst, &dstlen,
    (const Bytef *)pvSrc, (uLong)slSrcSize);
  slDstSize = (SLONG) dstlen;
  if (iResult==Z_OK) {
    return TRUE;
  } else {
    return FALSE;
  }
}

// on entry, slDstSize holds maximum size of output buffer,
// on exit, it is filled with resulting size
/* Unpack a chunk of data using given compression. */
BOOL CzlibCompressor::Unpack(const void *pvSrc, SLONG slSrcSize, void *pvDst, SLONG &slDstSize)
{
/*
int ZEXPORT uncompress (dest, destLen, source, sourceLen)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
    */

  CTSingleLock slZip(&zip_csLock, TRUE);
  uLongf dstlen = (uLongf) slDstSize;
  const int iResult = uncompress(
    (Bytef *)pvDst, &dstlen,
    (const Bytef *)pvSrc, (uLong)slSrcSize);
  slDstSize = (SLONG) dstlen;
  if (iResult==Z_OK) {
    return TRUE;
  } else {
    return FALSE;
  }
}
