// RenderSystemMemory.cpp
//

#include "RenderSystem_local.h"

#define	MEMORY_BLOCK_SIZE	0x200000

/*
=====================
R_ShutdownFrameData
=====================
*/
void R_ShutdownFrameData(void) {
	frameData_t* frame;
	frameMemoryBlock_t* block;

	// free any current data
	frame = frameData;
	if (!frame) {
		return;
	}

	R_FreeDeferredTriSurfs(frame);

	frameMemoryBlock_t* nextBlock;
	for (block = frame->memory; block; block = nextBlock) {
		nextBlock = block->next;
		Mem_Free(block);
	}
	Mem_Free(frame);
	frameData = NULL;
}

/*
=====================
R_InitFrameData
=====================
*/
void R_InitFrameData(void) {
	int size;
	frameData_t* frame;
	frameMemoryBlock_t* block;

	R_ShutdownFrameData();

	frameData = (frameData_t*)Mem_ClearedAlloc(sizeof(*frameData));
	frame = frameData;
	size = MEMORY_BLOCK_SIZE;
	block = (frameMemoryBlock_t*)Mem_Alloc(size + sizeof(*block));
	if (!block) {
		common->FatalError("R_InitFrameData: Mem_Alloc() failed");
	}
	block->size = size;
	block->used = 0;
	block->next = NULL;
	frame->memory = block;
	frame->memoryHighwater = 0;

	R_ToggleSmpFrame();
}

/*
================
R_CountFrameData
================
*/
int R_CountFrameData(void) {
	frameData_t* frame;
	frameMemoryBlock_t* block;
	int				count;

	count = 0;
	frame = frameData;
	for (block = frame->memory; block; block = block->next) {
		count += block->used;
		if (block == frame->alloc) {
			break;
		}
	}

	// note if this is a new highwater mark
	if (count > frame->memoryHighwater) {
		frame->memoryHighwater = count;
	}

	return count;
}

/*
=================
R_StaticAlloc
=================
*/
void* R_StaticAlloc(int bytes) {
	void* buf;

	tr.pc.c_alloc++;

	tr.staticAllocCount += bytes;

	buf = Mem_Alloc(bytes);

	// don't exit on failure on zero length allocations since the old code didn't
	if (!buf && (bytes != 0)) {
		common->FatalError("R_StaticAlloc failed on %i bytes", bytes);
	}
	return buf;
}

/*
=================
R_ClearedStaticAlloc
=================
*/
void* R_ClearedStaticAlloc(int bytes) {
	void* buf;

	buf = R_StaticAlloc(bytes);
	SIMDProcessor->Memset(buf, 0, bytes);
	return buf;
}

/*
=================
R_StaticFree
=================
*/
void R_StaticFree(void* data) {
	tr.pc.c_free++;
	Mem_Free(data);
}

/*
================
R_FrameAlloc

This data will be automatically freed when the
current frame's back end completes.

This should only be called by the front end.  The
back end shouldn't need to allocate memory.

If we passed smpFrame in, the back end could
alloc memory, because it will always be a
different frameData than the front end is using.

All temporary data, like dynamic tesselations
and local spaces are allocated here.

The memory will not move, but it may not be
contiguous with previous allocations even
from this frame.

The memory is NOT zero filled.
Should part of this be inlined in a macro?
================
*/
void* R_FrameAlloc(int bytes) {
	frameData_t* frame;
	frameMemoryBlock_t* block;
	void* buf;

	bytes = (bytes + 16) & ~15;
	// see if it can be satisfied in the current block
	frame = frameData;
	block = frame->alloc;

	if (block->size - block->used >= bytes) {
		buf = block->base + block->used;
		block->used += bytes;
		return buf;
	}

	// advance to the next memory block if available
	block = block->next;
	// create a new block if we are at the end of
	// the chain
	if (!block) {
		int		size;

		size = MEMORY_BLOCK_SIZE;
		block = (frameMemoryBlock_t*)Mem_Alloc(size + sizeof(*block));
		if (!block) {
			common->FatalError("R_FrameAlloc: Mem_Alloc() failed");
		}
		block->size = size;
		block->used = 0;
		block->next = NULL;
		frame->alloc->next = block;
	}

	// we could fix this if we needed to...
	if (bytes > block->size) {
		common->FatalError("R_FrameAlloc of %i exceeded MEMORY_BLOCK_SIZE",
			bytes);
	}

	frame->alloc = block;

	block->used = bytes;

	return block->base;
}

/*
==================
R_ClearedFrameAlloc
==================
*/
void* R_ClearedFrameAlloc(int bytes) {
	void* r;

	r = R_FrameAlloc(bytes);
	SIMDProcessor->Memset(r, 0, bytes);
	return r;
}