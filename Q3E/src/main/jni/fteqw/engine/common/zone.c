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
// Z_zone.c

#include "quakedef.h"
#ifdef _WIN32
#include "winquake.h"
#endif
#ifdef __GLIBC__
#include <malloc.h>
#endif

#define NOZONE

#ifdef _DEBUG
//#define MEMDEBUG	8192 //Debugging adds sentinels (the number is the size - I have the ram) 
#endif

//must be multiple of 4.
//#define TEMPDEBUG 4
//#define ZONEDEBUG 4
//#define HUNKDEBUG 4

//these need to be defined because it makes some bits of code simpler
#ifndef HUNKDEBUG
#define HUNKDEBUG 0
#endif
#ifndef ZONEDEBUG
#define ZONEDEBUG 0
#endif
#ifndef TEMPDEBUG
#define TEMPDEBUG 0
#endif

#if ZONEDEBUG>0 || HUNKDEBUG>0 || TEMPDEBUG>0
qbyte sentinelkey;
#endif

#define TAGLESS 1

size_t zmemtotal;
size_t zmemdelta;

typedef struct memheader_s {
	size_t size;
	int tag;
} memheader_t;

typedef struct zone_s {
	struct zone_s *next;
	struct zone_s *pvdn; // down if first, previous if not
	memheader_t mh;
} zone_t;
zone_t *zone_head;
#ifdef MULTITHREAD
void *zonelock;
#endif

#if 0
static void Z_DumpTree(void)
{
	zone_t *zone;
	zone_t *nextlist;
	zone_t *t;
	zone_t *prev;

	zone = zone_head;
	while(zone)
	{
		nextlist = zone->pvdn;

		fprintf(stderr, " +-+ %016x (tag: %08x)\n", zone, zone->mh.tag);

		prev = zone;
		t = zone->next;
		while(t)
		{
			if (t->pvdn != prev)
				fprintf(stderr, "Previous link failure\n");

			prev = t;
			t = t->next;
		}

		while(zone)
		{
			fprintf(stderr, "   +-- %016x\n", zone);

			zone = zone->next;
		}

		zone = nextlist;
	}
}
#endif

void *Z_TagMalloc(size_t size, int tag)
{
	zone_t *zone;

	zone = (zone_t *)malloc(size + sizeof(zone_t));
	if (!zone)
		Sys_Error("Z_Malloc: Failed on allocation of %"PRIuSIZE" bytes", size);
	Q_memset(zone, 0, size + sizeof(zone_t));
	zone->mh.tag = tag;
	zone->mh.size = size;

#ifdef MULTITHREAD
	if (zonelock)
		Sys_LockMutex(zonelock);
#endif

#if 0
	fprintf(stderr, "Before alloc:\n");
	Z_DumpTree();
	fprintf(stderr, "\n");
#endif

	if (zone_head == NULL)
		zone_head = zone;
	else
	{
		zone_t *s = zone_head;

		while (s && s->mh.tag != tag)
			s = s->pvdn;

		if (s)
		{ // tag match
			zone->next = s->next;
			if (s->next)
				s->next->pvdn = zone;
			zone->pvdn = s;
			s->next = zone;
		}
		else
		{
			zone->pvdn = zone_head;
		//	if (s->next)
		//		s->next->pvdn = zone;
			zone_head = zone;
		}
	}

#if 0
	fprintf(stderr, "After alloc:\n");
	Z_DumpTree();
	fprintf(stderr, "\n");
#endif

#ifdef MULTITHREAD
	if (zonelock)
		Sys_UnlockMutex(zonelock);
#endif

	return (void *)(zone + 1);
}

#ifdef USE_MSVCRT_DEBUG
void *ZF_MallocNamed(int size, char *file, int line)
{
	return _calloc_dbg(size, 1, _NORMAL_BLOCK, file, line);
}
void *Z_MallocNamed(int size, char *file, int line)
{
	void *mem = ZF_MallocNamed(size, file, line);
	if (!mem)
		Sys_Error("Z_Malloc: Failed on allocation of %i bytes", size);

	return mem;
}
#else
void *ZF_Malloc(size_t size)
{
#ifdef ANDROID
	void *ret = NULL;
	//android is linux, but not gnu and thus not posix...
	ret = memalign(max(sizeof(float)*4, sizeof(void*)), size);
	if (ret)
		memset(ret, 0, size);
	return ret;
#elif defined(__linux__)
	void *ret = NULL;

	if (!posix_memalign(&ret, max(sizeof(float)*4, sizeof(void*)), size))
		memset(ret, 0, size);
	return ret;
#else
	return calloc(size, 1);
#endif
}
void *Z_Malloc(size_t size)
{
	void *mem = ZF_Malloc(size);
	if (!mem)
		Sys_Error("Z_Malloc: Failed on allocation of %"PRIuSIZE" bytes", size);

	return mem;
}
#endif

char	*Z_StrDupf(const char *format, ...)
{
	va_list		argptr;
	char		*string;
	int n;

#if defined(_MSC_VER) || defined(_WIN32)
	int try;
	//msvc is shitty shit shit and doesn't even do c99. sadly mingw uses the same libraries.
	try = 256;
	for(;;)
	{
		string = Z_Malloc(try+1);
		va_start (argptr, format);
		n = _vsnprintf (string,try, format,argptr);
		va_end (argptr);
		if (n >= 0 && n < try)
			break;
		Z_Free(string);
		try *= 2;
	}
	string[n] = 0;
#else
	va_start (argptr, format);
	n = vsnprintf (NULL,0, format,argptr);
	va_end (argptr);
	if (n < 0)
		n = 1024;	//windows bullshit. whatever. good luck with that.

	string = Z_Malloc(n+1);
	va_start (argptr, format);
	vsnprintf (string,n+1, format,argptr);
	va_end (argptr);
	string[n] = 0;
#endif

	return string;
}
void Z_StrCatLen(char **ptr, const char *append, size_t newlen)
{
	size_t oldlen = *ptr?strlen(*ptr):0;
	char *newptr = BZ_Malloc(oldlen+newlen+1);
	memcpy(newptr, *ptr, oldlen);
	memcpy(newptr+oldlen, append, newlen);
	newptr[oldlen+newlen] = 0;
	BZ_Free(*ptr);
	*ptr = newptr;
}
void Z_StrCat(char **ptr, const char *append)
{
	size_t oldlen = *ptr?strlen(*ptr):0;
	size_t newlen = strlen(append);
	char *newptr = BZ_Malloc(oldlen+newlen+1);
	memcpy(newptr, *ptr, oldlen);
	memcpy(newptr+oldlen, append, newlen);
	newptr[oldlen+newlen] = 0;
	BZ_Free(*ptr);
	*ptr = newptr;
}

void VARGS Z_TagFree(void *mem)
{
	zone_t *zone = ((zone_t *)mem) - 1;

#if 0
	fprintf(stderr, "Before free:\n");
	Z_DumpTree();
	fprintf(stderr, "\n");
#endif

#ifdef MULTITHREAD
	if (zonelock)
		Sys_LockMutex(zonelock);
#endif
	if (zone->next)
		zone->next->pvdn = zone->pvdn;

	if (zone->pvdn && zone->pvdn->mh.tag == zone->mh.tag)
		zone->pvdn->next = zone->next;
	else
	{ // zone is first entry in a tag list 
		zone_t *s = zone_head;

		if (zone != s)
		{ // traverse and update down list
			while (s->pvdn != zone) 
				s = s->pvdn;

			if (zone->next)
				s->pvdn = zone->next;
			else
				s->pvdn = zone->pvdn;
		}
	}

	if (zone == zone_head)
	{ // freeing head node so update head pointer
		if (zone->next) // move to next, pvdn should be maintained properly
			zone_head = zone->next;
		else // no more entries with this tag so move head down
			zone_head = zone->pvdn;
	}

#if 0
	fprintf(stderr, "After free:\n");
	Z_DumpTree();
	fprintf(stderr, "\n");
#endif

#ifdef MULTITHREAD
	if (zonelock)
		Sys_UnlockMutex(zonelock);
#endif

	free(zone);
}

void VARGS Z_Free(void *mem)
{
	free(mem);
}

void VARGS Z_FreeTags(int tag)
{
	zone_t *taglist;
	zone_t *t;

#ifdef MULTITHREAD
	if (zonelock)
		Sys_LockMutex(zonelock);
#endif
	if (zone_head)
	{
		if (zone_head->mh.tag == tag)
		{ // just pull off the head
			taglist = zone_head;
			zone_head = zone_head->pvdn;
		}
		else
		{ // search for tag list and isolate it
			zone_t *z;
			z = zone_head;
			while (z->pvdn != NULL && z->pvdn->mh.tag != tag)
				z = z->pvdn;

			if (z->pvdn == NULL)
				taglist = NULL;
			else
			{
				taglist = z->pvdn;
				z->pvdn = z->pvdn->pvdn;
			}
		}
	}
	else
		taglist = NULL;
#ifdef MULTITHREAD
	if (zonelock)
		Sys_UnlockMutex(zonelock);
#endif

	// actually free list
	while (taglist != NULL)
	{
		t = taglist->next;
		free(taglist);
		taglist = t;
	}
}

//close enough
#ifndef SIZE_MAX
	#define SIZE_MAX ((size_t)-1)
#endif

#ifdef USE_MSVCRT_DEBUG
qboolean ZF_ReallocElementsNamed(void **ptr, size_t *elements, size_t newelements, size_t elementsize, const char *file, int line)
#else
qboolean ZF_ReallocElements(void **ptr, size_t *elements, size_t newelements, size_t elementsize)
#endif
{
	void *n;
	size_t oldsize;
	size_t newsize;

	//protect against malicious overflows
	if (newelements > SIZE_MAX / elementsize)
		return false;

	oldsize = *elements * elementsize;
	newsize = newelements * elementsize;

#ifdef USE_MSVCRT_DEBUG
	n = BZ_ReallocNamed(*ptr, newsize, file, line);
#else
	n = BZ_Realloc(*ptr, newsize);
#endif
	if (!n)
		return false;
	if (newsize > oldsize)
		memset((char*)n+oldsize, 0, newsize - oldsize);
	*elements = newelements;
	*ptr = n;
	return true;
}

/*
void *Z_Realloc(void *data, int newsize)
{
	memheader_t *memref;

	if (!data)
		return Z_Malloc(newsize);

	memref = ((memheader_t *)data) - 1;

	if (memref[0].tag != TAGLESS)
	{ // allocate a new block and copy since we need to maintain the lists
		zone_t *zone = ((zone_t *)data) - 1;
		int size = zone->mh.size;
		if (size != newsize)
		{
			void *newdata = Z_Malloc(newsize);

			if (size > newsize)
				size = newsize;
			memcpy(newdata, data, size);

			Z_Free(data);
			data = newdata;
		}
	}
	else
	{
		int oldsize = memref[0].size;
		memref = realloc(memref, newsize + sizeof(memheader_t));
		memref->size = newsize;
		if (newsize > oldsize)
			memset((qbyte *)memref + sizeof(memheader_t) + oldsize, 0, newsize - oldsize);
		data = ((memheader_t *)memref) + 1;
	}

	return data;
}
*/

#ifdef USE_MSVCRT_DEBUG
void *BZF_MallocNamed(int size, const char *file, int line)	//BZ_MallocNamed but allowed to fail - like straight malloc.
{
	void *mem;
	mem = _malloc_dbg(size, _NORMAL_BLOCK, file, line);
	if (mem)
	{
		zmemdelta += size;
		zmemtotal += size;
	}
	return mem;
}
#else
void *BZF_Malloc(size_t size)	//BZ_Malloc but allowed to fail - like straight malloc.
{
	void *mem;
	mem = malloc(size);
	if (mem)
	{
		zmemdelta += size;
		zmemtotal += size;
	}
	return mem;
}
#endif

#ifdef USE_MSVCRT_DEBUG
void *BZ_MallocNamed(int size, const char *file, int line)	//BZ_MallocNamed but allowed to fail - like straight malloc.
{
	void *mem = BZF_MallocNamed(size, file, line);
	if (!mem)
		Sys_Error("BZ_Malloc: Failed on allocation of %i bytes", size);

	return mem;
}
#else
void *BZ_Malloc(size_t size)	//Doesn't clear. The expectation is a large file, rather than sensitive data structures.
{
	void *mem = BZF_Malloc(size);
	if (!mem)
		Sys_Error("BZ_Malloc: Failed on allocation of %"PRIuSIZE" bytes", size);

	return mem;
}
#endif

#ifdef USE_MSVCRT_DEBUG
void *BZF_ReallocNamed(void *data, int newsize, const char *file, int line)
{
	return _realloc_dbg(data, newsize, _NORMAL_BLOCK, file, line);
}

void *BZ_ReallocNamed(void *data, int newsize, const char *file, int line)
{
	void *mem = BZF_ReallocNamed(data, newsize, file, line);

	if (!mem)
		Sys_Error("BZ_Realloc: Failed on reallocation of %i bytes", newsize);

	return mem;
}
#else
void *BZF_Realloc(void *data, size_t newsize)
{
	return realloc(data, newsize);
}

void *BZ_Realloc(void *data, size_t newsize)
{
	void *mem = BZF_Realloc(data, newsize);

	if (!mem)
		Sys_Error("BZ_Realloc: Failed on reallocation of %"PRIuSIZE" bytes", newsize);

	return mem;
}
#endif

void BZ_Free(void *data)
{
	free(data);
}


typedef struct zonegroupblock_s
{
	union
	{
		struct
		{
			struct zonegroupblock_s *next;
			struct zonegroupblock_s *prev;
#ifdef _DEBUG
			#define ZONEGROUP_SENTINAL 0x709ef00d
			unsigned int sentinal;
			unsigned int size;
#endif
		};
		vec4_t align16;
	};
} zonegroupblock_t;

#ifdef USE_MSVCRT_DEBUG
#undef ZG_Malloc
void *QDECL ZG_Malloc(zonegroup_t *ctx, int size){return ZG_MallocNamed(ctx, size, "ZG_Malloc", size);}
void *ZG_MallocNamed(zonegroup_t *ctx, int size, char *file, int line)
#else
void *QDECL ZG_Malloc(zonegroup_t *ctx, size_t size)
#endif
{
	zonegroupblock_t *newm;
	size += sizeof(zonegroupblock_t);	//well, at least the memory will be pointer aligned...
#ifdef USE_MSVCRT_DEBUG
	newm = Z_MallocNamed(size, file, line);
#else
	newm = Z_Malloc(size);
#endif

	newm->prev = NULL;
	if (ctx->first)
		((zonegroupblock_t*)ctx->first)->prev = newm;
	newm->next = ctx->first;
	ctx->first = newm;
	ctx->totalbytes += size;

#ifdef _DEBUG
	newm->sentinal = ZONEGROUP_SENTINAL;
	newm->size = size;
#endif
	return(void*)(newm+1);
}
void ZG_Free(zonegroup_t *ctx, void *ptr)
{
	zonegroupblock_t *mem = (zonegroupblock_t*)ptr-1;
	if (mem->prev)
		mem->prev->next = mem->next;
	else
		ctx->first = mem->next;
	if (mem->next)
		mem->next->prev = mem->prev;

#ifdef _DEBUG
	if (mem->sentinal != ZONEGROUP_SENTINAL)
		Sys_Error("Corrupt memory");
	ctx->totalbytes -= mem->size;
#endif
	BZ_Free(mem);
}
void ZG_FreeGroup(zonegroup_t *ctx)
{
	zonegroupblock_t *old;
	while(ctx->first)
	{
		old = ctx->first;
		ctx->first = old->next;
		BZ_Free(old);
	}
	ctx->totalbytes = 0;
}

//============================================================================

/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
clears old temp.
=================
*/
typedef struct hnktemps_s {
	struct hnktemps_s *next;
#if TEMPDEBUG>0
	int len;
#endif
} hnktemps_t;
hnktemps_t *hnktemps;

void Hunk_TempFree(void)
{
	hnktemps_t *nt;

	COM_AssertMainThread("Hunk_TempFree");

	while (hnktemps)
	{
#if TEMPDEBUG>0
		int i;
		qbyte *buf;
		buf = (qbyte *)(hnktemps+1);
		for (i = 0; i < TEMPDEBUG; i++)
		{
			if (buf[i] != sentinelkey)
				Sys_Error ("Hunk_Check: corrupt sentinel");
		}
		buf+=TEMPDEBUG;
		//app data
		buf += hnktemps->len;
		for (i = 0; i < TEMPDEBUG; i++)
		{
			if (buf[i] != sentinelkey)
				Sys_Error ("Hunk_Check: corrupt sentinel");
		}
#endif

		nt = hnktemps->next;

		free(hnktemps);
		hnktemps = nt;
	}
}


//allocates without clearing previous temp.
//safer than my hack that fuh moaned about...
void *Hunk_TempAllocMore (size_t size)
{
	void	*buf;

#if TEMPDEBUG>0
	hnktemps_t *nt;
	COM_AssertMainThread("Hunk_TempAllocMore");
	nt = (hnktemps_t*)malloc(size + sizeof(hnktemps_t) + TEMPDEBUG*2);
	if (!nt)
		return NULL;
	nt->next = hnktemps;
	nt->len = size;
	hnktemps = nt;
	buf = (void *)(nt+1);
	memset(buf, sentinelkey, TEMPDEBUG);
	buf = (char *)buf + TEMPDEBUG;
	memset(buf, 0, size);
	memset((char *)buf + size, sentinelkey, TEMPDEBUG);
	return buf;
#else
	hnktemps_t *nt;
	COM_AssertMainThread("Hunk_TempAllocMore");
	nt = (hnktemps_t*)malloc(size + sizeof(hnktemps_t));
	if (!nt)
		return NULL;
	nt->next = hnktemps;
	hnktemps = nt;
	buf = (void *)(nt+1);
	memset(buf, 0, size);
	return buf;
#endif
}


void *Hunk_TempAlloc (size_t size)
{
	Hunk_TempFree();

	return Hunk_TempAllocMore(size);
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/
void Cache_Flush(void)
{
	//this generically named function is hyjacked to flush models and sounds, as well as ragdolls etc
#ifdef HAVE_CLIENT
	S_Purge(false);
#endif
#if defined(HAVE_CLIENT) || defined(HAVE_SERVER)
#ifdef RAGDOLL
	rag_flushdolls(true);
#endif
	Mod_Purge(MP_FLUSH);
#endif
#ifdef HAVE_CLIENT
	Image_Purge();
#endif

#ifdef __GLIBC__
	malloc_trim(0);
#endif
}

static void Hunk_Print_f (void)
{
	Con_Printf("Z Delta: %"PRIuSIZE"KB\n", zmemdelta/1024); zmemdelta = 0;
	Con_Printf("Z Total: %"PRIuSIZE"KB\n", zmemtotal/1024);
	//note: Zone memory isn't tracked reliably. we don't track the mem that is freed, so it'll just climb and climb
	//we don't track reallocs either.

#if 0
	{
		zone_t *zone;
		int zoneused = 0;
		int zoneblocks = 0;

		for(zone = zone_head; zone; zone=zone->next)
		{
			zoneused += zone->size + sizeof(zone_t);
			zoneblocks++;
		}
		Con_Printf("Zone: %i containing %iKB\n", zoneblocks, zoneused/1024);
	}
#endif

#ifdef USE_MSVCRT_DEBUG
	{
		static struct _CrtMemState savedstate;
		static qboolean statesaved;

		_CrtMemDumpAllObjectsSince(statesaved?&savedstate:NULL);
		_CrtMemCheckpoint(&savedstate);
		statesaved = true;
	}
#endif
}
void Cache_Init(void)
{
	Cmd_AddCommand ("flush", Cache_Flush);
	Cmd_AddCommand ("hunkprint", Hunk_Print_f);
#if 0
	Cmd_AddCommand ("zoneprint", Zone_Print_f);
#endif
#ifdef NAMEDMALLOCS
	Cmd_AddCommand ("zonegroups", Zone_Groups_f);
#endif
}

//============================================================================

/*
========================
Memory_Init
========================
*/
void Memory_Init (void)
{
#if 0 //ndef NOZONE
	int p;
	int zonesize = DYNAMIC_SIZE;
#endif

#if ZONEDEBUG>0 || HUNKDEBUG>0 || TEMPDEBUG>0||CACHEDEBUG>0
	srand(time(0));
	sentinelkey = rand() & 0xff;
#endif

	Cache_Init ();

#ifdef MULTITHREAD
	if (!zonelock)
		zonelock = Sys_CreateMutex(); // this can fail!
#endif

#if 0 //ndef NOZONE
	p = COM_CheckParm ("-zone");
	if (p)
	{
		if (p < com_argc-1)
			zonesize = Q_atoi (com_argv[p+1]) * 1024;
		else
			Sys_Error ("Memory_Init: you must specify a size in KB after -zone");
	}
	mainzone = Hunk_AllocName ( zonesize, "zone" );
	Z_ClearZone (mainzone, zonesize);
#endif

#ifdef MULTITHREAD
	Sys_ThreadsInit();
#endif
}

void Memory_DeInit(void)
{
	Hunk_TempFree();
	Cache_Flush();

#ifdef MULTITHREAD
	if (zonelock)
	{
		Sys_DestroyMutex(zonelock);
		zonelock = NULL;
	}
#endif
}

