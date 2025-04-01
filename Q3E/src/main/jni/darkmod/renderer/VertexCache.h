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

#ifndef __VERTEXCACHE_H__
#define __VERTEXCACHE_H__

#include "renderer/backend/GpuBuffer.h"

// vertex cache calls should only be made by the front end

const int VERTCACHE_NUM_FRAMES = 3;

static const uint32 drawVertSize = sizeof(idDrawVert);
static const uint32 shadowCacheSize = sizeof(shadowCache_t);
static const uint32 VERTCACHE_FRAME_MASK = ( 1 << VERTCACHE_FRAMENUM_BITS ) - 1;

// 240 is the least common multiple between 16-byte alignment and the size of idDrawVert and shadowCache_t.
// The vertex cache positions need to be divisible by either size for glDrawElementsBaseVertex and similar
// functions.
const int VERTEX_CACHE_ALIGN = 240;
const int INDEX_CACHE_ALIGN = 16;
const int SHADOW_CACHE_ALIGN = 16;
//#define ALIGN( x, a ) ( ( ( x ) + ((a)-1) ) - ( ( (x) + (a) - 1 ) % a ) )

enum cacheType_t {
	CACHE_VERTEX,
	CACHE_INDEX,
	CACHE_JOINT
};

struct geoBufferSet_t {
	GpuBuffer			indexBuffer;
	GpuBuffer			vertexBuffer;
	byte *				mappedVertexBase;
	byte *				mappedIndexBase;
	idSysInterlockedInteger	indexMemUsed;
	idSysInterlockedInteger	vertexMemUsed;
	int					allocations;	// number of index and vertex allocations combined

	geoBufferSet_t();
};

static const vertCacheHandle_t NO_CACHE = { 0, 0, 0, false };

enum attribBind_t {
	ATTRIB_REGULAR,
	ATTRIB_SHADOW,
};

class idVertexCache {
public:
	void			Init();
	void			Shutdown();

	// called when vertex programs are enabled or disabled, because
	// the cached data is no longer valid
	void			PurgeAll();

	// will be an int offset cast to a pointer of ARB_vertex_buffer_object
	void			VertexPosition( vertCacheHandle_t handle, attribBind_t attrib = attribBind_t::ATTRIB_REGULAR );
	void *			IndexPosition( vertCacheHandle_t handle );

	// if you need to draw something without an indexCache, this must be called to reset GL_ELEMENT_ARRAY_BUFFER
	void			UnbindIndex();

	// updates the counter for determining which temp space to use
	// and which blocks can be purged
	// Also prints debugging info when enabled
	void			EndFrame();

	// prepare a shadow buffer to fill the static cache during map load
	void			PrepareStaticCacheForUpload();

	// this data is only valid for one frame of rendering
	vertCacheHandle_t AllocVertex( const void * data, int bytes ) {
		return ActuallyAlloc( dynamicData, data, bytes, CACHE_VERTEX );
	}
	vertCacheHandle_t AllocIndex( const void * data, int bytes ) {
		return ActuallyAlloc( dynamicData, data, bytes, CACHE_INDEX );
	}

	// this data is valid until the next map load
	vertCacheHandle_t AllocStaticVertex( const void* data, int bytes );
	vertCacheHandle_t AllocStaticIndex( const void* data, int bytes );
	vertCacheHandle_t AllocStaticShadow( void* data, int bytes );	// receives ownership

	// Returns false if it's been purged
	// This can only be called by the front end, the back end should only be looking at
	// vertCacheHandle_t that are already validated.
	bool			CacheIsCurrent( const vertCacheHandle_t handle ) const {
		return handle.isStatic || ( handle.IsValid() && handle.frameNumber == ( currentFrame & VERTCACHE_FRAME_MASK ) );
	}

	int GetBaseVertex() {
		return basePointer;
	}

public:
	static idCVar	r_showVertexCache;
	static idCVar	r_staticVertexMemory;
	static idCVar	r_staticIndexMemory;
	static idCVar	r_frameVertexMemory;
	static idCVar	r_frameIndexMemory;

	drawSurf_t		screenRectSurf;

private:

	int				currentFrame;			// for purgable block tracking
	int				basePointer;

	geoBufferSet_t	dynamicData;
	GLuint			staticVertexBuffer;
	GLuint			staticIndexBuffer;
	GLuint			staticShadowBuffer;

	GLuint			currentIndexBuffer;

	int				indexAllocCount, vertexAllocCount;
	int				indexUseCount, vertexUseCount;

	int				currentVertexCacheSize;
	int				currentIndexCacheSize;

	// Try to make room for <bytes> bytes
	vertCacheHandle_t ActuallyAlloc( geoBufferSet_t & vcs, const void * data, int bytes, cacheType_t type );
};

extern idVertexCache vertexCache;

#endif