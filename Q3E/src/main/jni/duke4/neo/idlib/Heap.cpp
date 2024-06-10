/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#pragma hdrstop

void* lastTempMemory = NULL;

/*
==================
Mem_ClearFrameStats
==================
*/
void Mem_ClearFrameStats(void) {

}

/*
==================
Mem_GetFrameStats
==================
*/
void Mem_GetFrameStats(memoryStats_t& allocs, memoryStats_t& frees) {

}

/*
==================
Mem_GetStats
==================
*/
void Mem_GetStats(memoryStats_t& stats) {

}

/*
==================
Mem_UpdateStats
==================
*/
void Mem_UpdateStats(memoryStats_t& stats, int size) {
	stats.num++;
	if (size < stats.minSize) {
		stats.minSize = size;
	}
	if (size > stats.maxSize) {
		stats.maxSize = size;
	}
	stats.totalSize += size;
}

/*
==================
Mem_UpdateAllocStats
==================
*/
void Mem_UpdateAllocStats(int size) {

}

/*
==================
Mem_UpdateFreeStats
==================
*/
void Mem_UpdateFreeStats(int size) {

}

/*
==================
Mem_Alloc
==================
*/
void* Mem_Alloc(const int size) {
	return malloc(size);
}

/*
==================
Mem_Free
==================
*/
void Mem_Free(void* ptr) {
	free(ptr);
}

/*
==================
Mem_Alloc16
==================
*/
void* Mem_Alloc16(const int size) {
#ifdef __ANDROID__
    void *ret = NULL;
    if(posix_memalign(&ret, 16, size) == 0)
        return ret;
    return NULL;
#else
	return _aligned_malloc(size, 16);
#endif
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16(void* ptr) {
#ifdef __ANDROID__
	free(ptr);
#else
	_aligned_free(ptr);
#endif
}

/*
==================
Mem_ClearedAlloc
==================
*/
void* Mem_ClearedAlloc(const int size) {
	void* mem = Mem_Alloc(size);
	memset(mem, 0, size);
	return mem;
}

/*
==================
Mem_ClearedAlloc
==================
*/
void Mem_AllocDefragBlock(void) {

}

/*
==================
Mem_CopyString
==================
*/
char* Mem_CopyString(const char* in) {
	char* out;

	out = (char*)Mem_Alloc(strlen(in) + 1);
	strcpy(out, in);
	return out;
}

/*
==================
Mem_Dump_f
==================
*/
void Mem_Dump_f(const idCmdArgs& args) {
}

/*
==================
Mem_DumpCompressed_f
==================
*/
void Mem_DumpCompressed_f(const idCmdArgs& args) {
}

/*
==================
Mem_Init
==================
*/
void Mem_Init(void) {

}

/*
==================
Mem_Shutdown
==================
*/
void Mem_Shutdown(void) {

}

/*
==================
Mem_EnableLeakTest
==================
*/
void Mem_EnableLeakTest(const char* name) {
}
