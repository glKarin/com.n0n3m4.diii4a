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

#ifndef ZONE_H
#define ZONE_H

#include <stddef.h>
#include <stdalign.h>
#include "qtypes.h"
#include "qdefs.h"
#include "com_list.h"

// Work around incomplete C11 support in Microsoft's stddef.h
// This matches the Clang 14 header. Microsoft's double and long double are the same.
#if defined(_MSC_VER)
	typedef double max_align_t;
#endif

extern qbool mem_bigendian;

// div0: heap overflow detection paranoia
#define MEMPARANOIA 0

#define POOLNAMESIZE 128
// if set this pool will be printed in memlist reports
#define POOLFLAG_TEMP 1

typedef struct memheader_s
{
	// address returned by Chunk_Alloc (may be significantly before this header to satisify alignment)
	void *baseaddress;
	// next and previous memheaders in chain belonging to pool
	struct llist_s list;
	// pool this memheader belongs to
	struct mempool_s *pool;
	// size of the memory after the header (excluding header and sentinel2)
	size_t size;
	// file name and line where Mem_Alloc was called
	const char *filename;
	int fileline;
	// should always be equal to MEMHEADER_SENTINEL_FOR_ADDRESS()
	unsigned int sentinel;
	// immediately followed by data, which is followed by another copy of mem_sentinel[]
}
memheader_t;

typedef struct mempool_s
{
	// should always be MEMPOOL_SENTINEL
	unsigned int sentinel1;
	// chain of individual memory allocations
	struct llist_s chain;
	// POOLFLAG_*
	unsigned flags;
	// total memory allocated in this pool (inside memheaders)
	size_t totalsize;
	// total memory allocated in this pool (actual malloc total)
	size_t realsize;
	// updated each time the pool is displayed by memlist, shows change from previous time (unless pool was freed)
	size_t lastchecksize;
	// linked into global mempool list
	struct mempool_s *next;
	// parent object (used for nested memory pools)
	struct mempool_s *parent;
	// file name and line where Mem_AllocPool was called
	const char *filename;
	int fileline;
	// name of the pool
	char name[POOLNAMESIZE];
	// should always be MEMPOOL_SENTINEL
	unsigned int sentinel2;
}
mempool_t;

#define Mem_Alloc(pool,size) _Mem_Alloc(pool, NULL, size, alignof(max_align_t), __FILE__, __LINE__)
#define Mem_AllocType(pool,type,size) (type *)_Mem_Alloc(pool, NULL, size, alignof(type), __FILE__, __LINE__)
#define Mem_Realloc(pool,data,size) _Mem_Alloc(pool, data, size, alignof(max_align_t), __FILE__, __LINE__)
#define Mem_ReallocType(pool,data,type,size) (type *)_Mem_Alloc(pool, data, size, alignof(type), __FILE__, __LINE__)
#define Mem_Free(mem) _Mem_Free(mem, __FILE__, __LINE__)
#define Mem_strdup(pool, s) _Mem_strdup(pool, s, __FILE__, __LINE__)
#define Mem_CheckSentinels(data) _Mem_CheckSentinels(data, __FILE__, __LINE__)
#if MEMPARANOIA
#define Mem_CheckSentinelsGlobal()  _Mem_CheckSentinelsGlobal(__FILE__, __LINE__)
#else
#define Mem_CheckSentinelsGlobal() if(developer_memorydebug.integer) { _Mem_CheckSentinelsGlobal(__FILE__, __LINE__); }
#endif
#define Mem_AllocPool(name, flags, parent) _Mem_AllocPool(name, flags, parent, __FILE__, __LINE__)
#define Mem_FreePool(pool) _Mem_FreePool(pool, __FILE__, __LINE__)
#define Mem_EmptyPool(pool) _Mem_EmptyPool(pool, __FILE__, __LINE__)

void *_Mem_Alloc(mempool_t *pool, void *data, size_t size, size_t alignment, const char *filename, int fileline);
void _Mem_Free(void *data, const char *filename, int fileline);
mempool_t *_Mem_AllocPool(const char *name, unsigned flags, mempool_t *parent, const char *filename, int fileline);
void _Mem_FreePool(mempool_t **pool, const char *filename, int fileline);
void _Mem_EmptyPool(mempool_t *pool, const char *filename, int fileline);
void _Mem_CheckSentinels(void *data, const char *filename, int fileline);
void _Mem_CheckSentinelsGlobal(const char *filename, int fileline);
// if pool is NULL this searches ALL pools for the allocation
qbool Mem_IsAllocated(mempool_t *pool, const void *data);

char *_Mem_strdup(mempool_t *pool, const char *s, const char *filename, int fileline);

/// Returns the current size of an allocation
// not a macro so that it doesn't allow the size to be changed.
static inline size_t Mem_Size(void *data)
{
	return ((memheader_t *)((unsigned char *)data - sizeof(memheader_t)))->size;
}

typedef struct memexpandablearray_array_s
{
	unsigned char *data;
	unsigned char *allocflags;
	size_t numflaggedrecords;
}
memexpandablearray_array_t;

typedef struct memexpandablearray_s
{
	mempool_t *mempool;
	size_t recordsize;
	size_t numrecordsperarray;
	size_t numarrays;
	size_t maxarrays;
	memexpandablearray_array_t *arrays;
}
memexpandablearray_t;

void Mem_ExpandableArray_NewArray(memexpandablearray_t *l, mempool_t *mempool, size_t recordsize, int numrecordsperarray);
void Mem_ExpandableArray_FreeArray(memexpandablearray_t *l);
void *Mem_ExpandableArray_AllocRecord(memexpandablearray_t *l);
void Mem_ExpandableArray_FreeRecord(memexpandablearray_t *l, void *record);
size_t Mem_ExpandableArray_IndexRange(const memexpandablearray_t *l) DP_FUNC_PURE;
void *Mem_ExpandableArray_RecordAtIndex(const memexpandablearray_t *l, size_t index) DP_FUNC_PURE;

// used for temporary allocations
extern mempool_t *tempmempool;

void Memory_Init (void);
void Memory_Shutdown (void);
void Memory_Init_Commands (void);

extern mempool_t *zonemempool;
#define Z_Malloc(size) Mem_Alloc(zonemempool, size)
#define Z_Realloc(data, size) Mem_Realloc(zonemempool, data, size)
#define Z_strdup(s) Mem_strdup(zonemempool, s)
#define Z_Free(data) Mem_Free(data)

extern struct cvar_s developer_memory;
extern struct cvar_s developer_memorydebug;
extern struct cvar_s developer_memoryreportlargerthanmb;

#endif

