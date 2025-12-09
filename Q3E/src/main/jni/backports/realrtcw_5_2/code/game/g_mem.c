/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//
// g_mem.c
//


#include "g_local.h"

// Ridah, increased this (fixes Dan's crash)
//#define POOLSIZE	(256 * 1024)
//#define POOLSIZE	(2048 * 1024)
#define POOLSIZE    ( 8192 * 1024 )   //----(SA)	upped to try to get assault_34 going // RealRTCW was 4096

#ifdef __ANDROID__ //karin: static memory allocation from client: too large
static char *memoryPool = NULL;
static char * (*g_require_memory_pool)(size_t size);

__attribute__((visibility("default"))) void G_SetAllocMemoryPoolFcn(void *ptr)
{
	fprintf(stderr, "[qagame]: set alloc memory pool function: %p\n", ptr );
	g_require_memory_pool = (char *(*)(size_t))ptr;
}

char * trap_RequireMemoryPool( size_t size ) {
	if(!g_require_memory_pool)
	{
		G_Error( "Must get alloc memory function pointer first!" );
		return NULL;
	}
	char *pool = (char *)g_require_memory_pool(size);
	G_Printf( "[qagame]: alloc memory pool: %zd -> %p\n", size, pool );
	return (char *)g_require_memory_pool(size);
}
#else
static char memoryPool[POOLSIZE];
#endif
static int allocPoint;

void *G_Alloc( int size ) {
	char    *p;

	if ( g_debugAlloc.integer ) {
		G_Printf( "G_Alloc of %i bytes (%i left)\n", size, POOLSIZE - allocPoint - ( ( size + 31 ) & ~31 ) );
	}

	if ( allocPoint + size > POOLSIZE ) {
		G_Error( "G_Alloc: failed on allocation of %i bytes", size );
		return NULL;
	}
#ifdef __ANDROID__ //karin: static memory allocation from client
	if ( !memoryPool ) {
		G_Error( "G_Alloc: failed on allocation because NULL memory" );
		return NULL;
	}
#endif

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

void G_InitMemory( void ) {
#ifdef __ANDROID__ //karin: static memory allocation from client
	memoryPool = trap_RequireMemoryPool(POOLSIZE);
	if(memoryPool)
		G_Printf("G_InitMemory: Game require %d bytes memory pool: %p.\n", POOLSIZE, memoryPool);
	else
	{
		G_Error("G_InitMemory: Game require %d bytes memory pool fail!", POOLSIZE);
		return;
	}
#endif
	allocPoint = 0;
}

void Svcmd_GameMem_f( void ) {
	G_Printf( "Game memory status: %i out of %i bytes allocated\n", allocPoint, POOLSIZE );
}
