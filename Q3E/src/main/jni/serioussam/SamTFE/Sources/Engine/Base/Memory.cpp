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

#include <Engine/Base/Memory.h>
#include <Engine/Base/Translation.h>

#include <Engine/Base/ErrorReporting.h>

/* rcg10282005  "new.h" is deprecated in newer C++ standards... --ryan. */
#ifdef _MSC_VER
#include <new.h>
#else
#include <new>
#endif

__extern FLOAT _bCheckAllAllocations = FALSE;

#ifdef PLATFORM_WIN32
/*
 * Declarations for setting up the 'new_handler'.
 */
_CRTIMP int __cdecl _query_new_mode( void );
_CRTIMP int __cdecl _set_new_mode( int );        

#ifndef _PNH_DEFINED
typedef int (__cdecl * _PNH)( size_t );
#define _PNH_DEFINED
#endif

/*_CRTIMP _PNH __cdecl _query_new_handler( void );
_CRTIMP _PNH __cdecl _set_new_handler( _PNH );*/

#ifndef NDEBUG
// include this for debug version of operator new
#pragma comment (lib, "msvcrtd.lib")
#endif

// include user32 library (because of message box)
#pragma comment (lib, "user32.lib")
#endif


// if not enough memory
#ifdef _MSC_VER
int NewHandler(size_t size)
{
  // terminate program
  FatalError(TRANS("Not enough memory (%d bytes needed)!"), size);
  return 0;
}
#else
void NewHandler(void)
{
  // terminate program
  FatalError(TRANS("Not enough memory!"));
}
#endif

/* Static class used for initializing memory handlers. */
static class CMemHandlerInit {
public:
  // constructor
  CMemHandlerInit(void);
} MemHandlerInit;

CMemHandlerInit::CMemHandlerInit(void)
{
  // set our not-enough-memory handler
  _set_new_handler(NewHandler);
  // make malloc use that handler
  #ifdef _MSC_VER
  _set_new_mode(1);
  #endif
}

#undef AllocMemory

#if (defined _MSC_VER) && (defined  PLATFORM_64BIT)
void *AllocMemory(UINT64 memsize)
#else
void *AllocMemory( SLONG memsize)
#endif
{
  void *pmem;
  ASSERTMSG(memsize>0, "AllocMemory: Block size is less or equal zero.");
  if (_bCheckAllAllocations) {
    _CrtCheckMemory();
  }
  pmem = malloc( memsize);
  // memory handler asures no null results here?!
  if (pmem==NULL) {
    _CrtCheckMemory();
    FatalError(TRANS("Not enough memory (%d bytes needed)!"), memsize);
  }
  return pmem;
}

#ifdef _MSC_VER
#ifndef NDEBUG
void *_debug_AllocMemory( SLONG memsize, int iType, const char *strFile, int iLine)
{
  void *pmem;
  ASSERTMSG(memsize>0, "AllocMemory: Block size is less or equal zero.");

  if (_bCheckAllAllocations) {
    _CrtCheckMemory();
  }
  pmem = _malloc_dbg( memsize, iType, strFile, iLine);
  // memory handler asures no null results here?!
  if (pmem==NULL) {
    _CrtCheckMemory();
    FatalError(TRANS("Not enough memory (%d bytes needed)!"), memsize);
  }
  return pmem;
}
#endif
#endif

#if (defined _MSC_VER) && (defined  PLATFORM_64BIT)
void *AllocMemoryAligned(UINT64 memsize, UINT64 slAlignPow2)
{
  UINT64 ulMem = (UINT64) AllocMemory(memsize + slAlignPow2 * 2);
  UINT64 ulMemAligned = ((ulMem + slAlignPow2 - 1) & ~(slAlignPow2 - 1)) + slAlignPow2;
  ((UINT64 *) ulMemAligned)[-1] = ulMem;
  return (void *) ulMemAligned;
}
void FreeMemoryAligned(void *memory)
{
  FreeMemory((void *) (((UINT64 *) memory)[-1]));
}
#else
void *AllocMemoryAligned( SLONG memsize, SLONG slAlignPow2)
{
  ASSERT(slAlignPow2 >= (sizeof (void*) * 2));
  size_t ulMem = (size_t)AllocMemory(memsize+slAlignPow2*2);
  size_t ulMemAligned = ((ulMem+slAlignPow2-1) & ~(slAlignPow2-1)) + slAlignPow2;
  ((size_t *)ulMemAligned)[-1] = ulMem;
  return (void*)ulMemAligned;
}
void FreeMemoryAligned( void *memory)
{
  FreeMemory((void*) ( ( (size_t*)memory )[-1] ) );
}
#endif

void FreeMemory( void *memory )
{
  ASSERTMSG(memory!=NULL, "FreeMemory: NULL pointer input.");
  free( (char *)memory);
}

void ResizeMemory( void **ppv, SLONG slSize )
{
  if (_bCheckAllAllocations) {
    _CrtCheckMemory();
  }
  void *pv = realloc(*ppv, slSize);
  // memory handler asures no null results here?!
  if (pv==NULL) {
    _CrtCheckMemory();
    FatalError(TRANS("Not enough memory (%d bytes needed)!"), slSize);
  }
  *ppv = pv;
}

void GrowMemory( void **ppv, SLONG newSize )
{
  ResizeMemory(ppv, newSize);
}

void ShrinkMemory( void **ppv, SLONG newSize )
{
  ResizeMemory(ppv, newSize);
}


/*
 * Allocate a copy of a string. - fatal error if not enough memory.
 */
char *StringDuplicate(const char *strOriginal) {
  // get the size
  size_t slSize = strlen(strOriginal) + (size_t)1;
  // allocate that much memory
  char *strCopy = (char *)AllocMemory(slSize);
  // copy it there
  memcpy(strCopy, strOriginal, slSize);
  // result is the pointer to the copied string
  return strCopy;
}



// return position where we encounter zero byte or iBytes
INDEX FindZero( UBYTE *pubMemory, INDEX iBytes)
{
  for( INDEX i=0; i<iBytes; i++) if( pubMemory[i]==0) return i;
  return iBytes;
}
