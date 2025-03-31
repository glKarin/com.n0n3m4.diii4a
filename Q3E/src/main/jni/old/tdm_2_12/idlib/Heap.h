/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __HEAP_H__
#define __HEAP_H__

#include <cstdlib>
#include <cstddef> // for NULL

/*
===============================================================================

	Memory Management

	This is a replacement for the compiler heap code (i.e. "C" malloc() and
	free() calls). On average 2.5-3.0 times faster than MSVC malloc()/free().
	Worst case performance is 1.65 times faster and best case > 70 times.
 
===============================================================================
*/


typedef struct {
	int		num;
	size_t		minSize;
	size_t		maxSize;
	size_t		totalSize;
} memoryStats_t;


void		Mem_Init( void );
void		Mem_Shutdown( void );
void		Mem_EnableLeakTest( const char *name );
void		Mem_ClearFrameStats( void );
void		Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees );
void		Mem_GetStats( memoryStats_t &stats );
void		Mem_Dump_f( const class idCmdArgs &args );
void		Mem_DumpCompressed_f( const class idCmdArgs &args );
void		Mem_AllocDefragBlock( void );


#ifndef ID_DEBUG_MEMORY

void *		Mem_Alloc( const int size );
void *		Mem_ClearedAlloc( const int size );
void		Mem_Free( void *ptr );
char *		Mem_CopyString( const char *in );
void *		Mem_Alloc16( const int size );
void		Mem_Free16( void *ptr );

#ifdef ID_REDIRECT_NEWDELETE

__inline void *operator new( size_t s ) {
	return Mem_Alloc( s );
}
__inline void operator delete( void *p ) {
	Mem_Free( p );
}
__inline void *operator new[]( size_t s ) {
	return Mem_Alloc( s );
}
__inline void operator delete[]( void *p ) {
	Mem_Free( p );
}

#endif

#else /* ID_DEBUG_MEMORY */

void *		Mem_Alloc( const int size, const char *fileName, const int lineNumber );
void *		Mem_ClearedAlloc( const int size, const char *fileName, const int lineNumber );
void		Mem_Free( void *ptr, const char *fileName, const int lineNumber );
char *		Mem_CopyString( const char *in, const char *fileName, const int lineNumber );
void *		Mem_Alloc16( const int size, const char *fileName, const int lineNumber );
void		Mem_Free16( void *ptr, const char *fileName, const int lineNumber );

#ifdef ID_REDIRECT_NEWDELETE

__inline void *operator new( size_t s, int t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber );
}
__inline void operator delete( void *p, int t1, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}
__inline void *operator new[]( size_t s, int t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber );
}
__inline void operator delete[]( void *p, int t1, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}
__inline void *operator new( size_t s ) {
	return Mem_Alloc( s, "", 0 );
}
__inline void operator delete( void *p ) {
	Mem_Free( p, "", 0 );
}
__inline void *operator new[]( size_t s ) {
	return Mem_Alloc( s, "", 0 );
}
__inline void operator delete[]( void *p ) {
	Mem_Free( p, "", 0 );
}

#define ID_DEBUG_NEW						new( 0, 0, __FILE__, __LINE__ )
#undef new
#define new									ID_DEBUG_NEW

#endif

#define		Mem_Alloc( size )				Mem_Alloc( size, __FILE__, __LINE__ )
#define		Mem_ClearedAlloc( size )		Mem_ClearedAlloc( size, __FILE__, __LINE__ )
#define		Mem_Free( ptr )					Mem_Free( ptr, __FILE__, __LINE__ )
#define		Mem_CopyString( s )				Mem_CopyString( s, __FILE__, __LINE__ )
#define		Mem_Alloc16( size )				Mem_Alloc16( size, __FILE__, __LINE__ )
#define		Mem_Free16( ptr )				Mem_Free16( ptr, __FILE__, __LINE__ )

#endif /* ID_DEBUG_MEMORY */

#endif /* !__HEAP_H__ */
