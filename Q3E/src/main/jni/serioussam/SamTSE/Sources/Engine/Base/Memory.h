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

#ifndef SE_INCL_MEMORY_H
#define SE_INCL_MEMORY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Types.h>

// global memory management functions

/* Get amount of free memory in system. */
ENGINE_API extern SLONG GetFreeMemory( void );

/* Allocate a block of memory - fatal error if not enough memory. */
#if (defined _MSC_VER) && (defined  PLATFORM_64BIT)
ENGINE_API extern void *AllocMemory(UINT64 memsize );
ENGINE_API extern void *AllocMemoryAligned(UINT64 memsize, UINT64 slAlignPow2);
#else
ENGINE_API extern void *AllocMemory(SLONG memsize);
ENGINE_API extern void *AllocMemoryAligned(SLONG memsize, SLONG slAlignPow2);
#endif
ENGINE_API extern void *_debug_AllocMemory(SLONG memsize, int iType, const char *strFile, int iLine);
/* Free a block of memory. */
ENGINE_API extern void FreeMemory( void *memory);
ENGINE_API extern void FreeMemoryAligned( void *memory);

ENGINE_API extern void ResizeMemory( void **memory, SLONG memsize );
ENGINE_API extern void GrowMemory( void **memory, SLONG memsize );
ENGINE_API extern void ShrinkMemory( void **memory,SLONG memsize );

/* Allocate a copy of a string. - fatal error if not enough memory. */
ENGINE_API extern char *StringDuplicate(const char *strOriginal);

ENGINE_API extern BOOL MemoryConsistencyCheck( void );
ENGINE_API extern BOOL AllMemoryFreed( void );

// return position (offset) where we encounter zero byte or iBytes
ENGINE_API extern INDEX FindZero( UBYTE *pubMemory, INDEX iBytes);


#ifdef _MSC_VER  /* rcg10042001 */
#ifndef NDEBUG

// use debug version of operator new
#include <crtdbg.h>
/*void * __cdecl operator new(
        unsigned int,
        int,
        const char *,
        int
        );*/
#define DEBUG_NEW_CT new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW_CT
#define DEBUG_ALLOC(size) _debug_AllocMemory(size, _NORMAL_BLOCK, __FILE__, __LINE__)
#define AllocMemory(size) DEBUG_ALLOC(size)
#define ReportLostMemory _CrtDumpMemoryLeaks

#else
#define ReportLostMemory() ((void)0)

#endif // NDEBUG
#endif // _MSC_VER

#endif  /* include-once check. */

