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

#ifndef SE_INCL_CRC_H
#define SE_INCL_CRC_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

extern ENGINE_API ULONG crc_aulCRCTable[256];

// begin crc calculation
inline void CRC_Start(ULONG &ulCRC) { ulCRC = 0xFFFFFFFF; };

// add data to a crc value
inline void CRC_AddBYTE( ULONG &ulCRC, UBYTE ub)
{
  ulCRC = (ulCRC>>8)^crc_aulCRCTable[UBYTE(ulCRC)^ub];
};

inline void CRC_AddWORD( ULONG &ulCRC, UWORD uw)
{
  CRC_AddBYTE(ulCRC, UBYTE(uw>> 8));
  CRC_AddBYTE(ulCRC, UBYTE(uw>> 0));
};

inline void CRC_AddLONG( ULONG &ulCRC, ULONG ul)
{
  CRC_AddBYTE(ulCRC, UBYTE(ul>>24));
  CRC_AddBYTE(ulCRC, UBYTE(ul>>16));
  CRC_AddBYTE(ulCRC, UBYTE(ul>> 8));
  CRC_AddBYTE(ulCRC, UBYTE(ul>> 0));
};

inline void CRC_AddLONGLONG( ULONG &ulCRC, unsigned long long x)
{
  CRC_AddBYTE(ulCRC, UBYTE(x>>56));
  CRC_AddBYTE(ulCRC, UBYTE(x>>48));
  CRC_AddBYTE(ulCRC, UBYTE(x>>40));
  CRC_AddBYTE(ulCRC, UBYTE(x>>32));
  CRC_AddBYTE(ulCRC, UBYTE(x>>24));
  CRC_AddBYTE(ulCRC, UBYTE(x>>16));
  CRC_AddBYTE(ulCRC, UBYTE(x>> 8));
  CRC_AddBYTE(ulCRC, UBYTE(x>> 0));
}

inline void CRC_AddFLOAT(ULONG &ulCRC, FLOAT f)
{
  CRC_AddLONG(ulCRC, *(ULONG*)&f);
};

// add memory block to a CRC value
inline void CRC_AddBlock(ULONG &ulCRC, UBYTE *pubBlock, ULONG ulSize)
{
  for( INDEX i=0; (ULONG)i<ulSize; i++) CRC_AddBYTE( ulCRC, pubBlock[i]);
};

// end crc calculation
inline void CRC_Finish(ULONG &ulCRC) { ulCRC ^= 0xFFFFFFFF; };

// in 32bit mode, it just returns iPtr ULONG,
// in 64bit mode it returns the CRC hash of iPtr (or 0 if ptr == NULL)
// so either way you should get a value that very likely uniquely identifies the pointer
inline ULONG IntPtrToID(size_t iPtr)
{
#if PLATFORM_32BIT
  return (ULONG)iPtr;
#else
  // in case the code relies on 0 having special meaning because of NULL-pointers...
  if(iPtr == 0)  return 0;
  ULONG ret;
  CRC_Start(ret);
  CRC_AddLONGLONG(ret, iPtr);
  CRC_Finish(ret);
  return ret;
#endif
}

// in 32bit mode, it just returns the pointer's address as ULONG,
// in 64bit mode it returns the CRC hash of the pointer's address (or 0 if ptr == NULL)
// so either way you should get a value that very likely uniquely identifies the pointer
inline ULONG PointerToID(void* ptr)
{
#if PLATFORM_32BIT
  return (ULONG)(size_t)ptr;
#else
  return IntPtrToID((size_t)ptr);
#endif
}

#endif  /* include-once check. */

