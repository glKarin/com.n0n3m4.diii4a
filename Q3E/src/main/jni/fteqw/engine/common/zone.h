/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
 memory allocation


H_??? The hunk manages the entire memory block given to quake.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object


Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.


------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/

#if 0//defined(_DEBUG) && defined(__linux__) && !defined(ANDROID)
	#include <valgrind/memcheck.h>
#else
	#define VALGRIND_MAKE_MEM_UNDEFINED(ptr,sz)	//as an alternative to memzero..
	#define VALGRIND_MAKE_MEM_NOACCESS(ptr,sz)
	#define VALGRIND_MAKE_MEM_DEFINED_IF_ADDRESSABLE(ptr,sz) //undo VALGRIND_MAKE_MEM_UNDEFINED, to make sure we don't read past the end of buffers.
#endif

void Memory_Init (void);
void Memory_DeInit(void);

//Prefixes:
//Z - just general 'zone' memory.
//B - allocated memory is not zero-filled.
//F - allocation can return NULL (otherwise sys_errors)
//G - special set of functions with its own rules. Frees must not be mixed.
//Tag - additional special set of functions with their own rules. Frees must not be mixed.
void VARGS Z_Free (void *ptr);
void *Z_Malloc (size_t size); // returns 0 filled memory
void *ZF_Malloc (size_t size); // allowed to fail
void *Z_MallocNamed (size_t size, char *file, int line); // returns 0 filled memory
void *ZF_MallocNamed (size_t size, char *file, int line); // allowed to fail
//#define Z_Malloc(x) Z_MallocNamed2(x, __FILE__, __LINE__ )
void *Z_TagMalloc (size_t size, int tag);
void VARGS Z_TagFree(void *ptr);
void VARGS Z_FreeTags(int tag);
qboolean ZF_ReallocElements(void **ptr, size_t *elements, size_t newelements, size_t elementsize);	//returns false on error
qboolean ZF_ReallocElementsNamed(void **ptr, size_t *elements, size_t newelements, size_t elementsize, const char *file, int line);	//returns false on error
#define Z_ReallocElements(ptr,elements,newelements,elementsize) do{if (!ZF_ReallocElements(ptr,elements,newelements,elementsize))Sys_Error("Z_ReallocElements failed (%s %i)\n", __FILE__, __LINE__);}while(0)	//returns false on error

//Big Zone: allowed to fail, doesn't clear. The expectation is a large file, rather than sensitive data structures.
//(this is a nicer name for malloc)
void *BZ_Malloc(size_t size);
void *BZF_Malloc(size_t size);
void *BZ_MallocNamed (size_t size, const char *file, int line); // returns 0 filled memory
void *BZF_MallocNamed (size_t size, const char *file, int line); // allowed to fail
void *BZ_Realloc(void *ptr, size_t size);
void *BZ_ReallocNamed(void *data, size_t newsize, const char *file, int line);
void *BZF_Realloc(void *data, size_t newsize);
void *BZF_ReallocNamed(void *data, size_t newsize, const char *file, int line);
void BZ_Free(void *ptr);

//ctx should start off as void*ctx=NULL
typedef struct zonegroup_s
{
	void *first;
	int totalbytes;	//combined size of all mallocs in this group
} zonegroup_t;
void *QDECL ZG_Malloc(zonegroup_t *ctx, size_t size);
void *ZG_MallocNamed(zonegroup_t *ctx, size_t size, char *file, int line);
void QDECL ZG_Free(zonegroup_t *ctx, void *ptr);
void QDECL ZG_FreeGroup(zonegroup_t *ctx);

#ifdef USE_MSVCRT_DEBUG
#define BZ_Malloc(size) BZ_MallocNamed(size, __FILE__, __LINE__)
#define Z_Malloc(size) Z_MallocNamed(size, __FILE__, __LINE__)
#define BZ_Realloc(ptr, size) BZ_ReallocNamed(ptr, size, __FILE__, __LINE__)
#define BZF_Malloc(size) BZF_MallocNamed(size, __FILE__, __LINE__)
#define ZF_Malloc(size) ZF_MallocNamed(size, __FILE__, __LINE__)
#define BZF_Realloc(ptr, size) BZF_ReallocNamed(ptr, size, __FILE__, __LINE__)
#define ZG_Malloc(ctx, size) ZG_MallocNamed(ctx, size, __FILE__, __LINE__)
#define ZF_ReallocElements(p,e,n,s) ZF_ReallocElementsNamed(p,e,n,s,__FILE__,__LINE__)
#endif
#define Z_StrDup(s) strcpy(Z_Malloc(strlen(s)+1), s)
#define Z_StrDupPtr(v,s) do{Z_Free(*v),*(v) = strcpy(Z_Malloc(strlen(s)+1), s);}while(0)

char	*Z_StrDupf(const char *format, ...);
void Z_StrCat(char **ptr, const char *append);
void Z_StrCatLen(char **ptr, const char *append, size_t newlen); //still doesn't allow nulls, but src doesn't need null termination.

/*
void *Hunk_Alloc (int size);		// returns 0 filled memory
void *Hunk_AllocName (int size, char *name);
*/

void *Hunk_TempAlloc (size_t size);
void *Hunk_TempAllocMore (size_t size); //Don't clear old temp

/*
typedef struct cache_user_s
{
	void	*data;
	qboolean fake;
} cache_user_t;
*/
void Cache_Flush (void);
/*
void *Cache_Check (cache_user_t *c);
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void Cache_Free (cache_user_t *c);

void *Cache_Alloc (cache_user_t *c, int size, char *name);
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.

void Cache_Report (void);
*/
