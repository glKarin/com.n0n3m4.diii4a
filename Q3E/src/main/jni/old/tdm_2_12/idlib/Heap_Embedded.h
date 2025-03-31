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

#ifndef __HEAP_EMBEDDED__
#define __HEAP_EMBEDDED__

#include "Heap.h"
#include "containers/List.h"

//stgatilov: this allocator always serves requests from within single memory block
//it was originally implemented for storing idScriptObject data in x64 mode
class idEmbeddedAllocator {
private:
	char *bufferPtr;
	int bufferSize;
	idList<int> freePos;
	idList<int> freeSize;

public:
	idEmbeddedAllocator() {
		Clear();
	}
	void Clear() {
		bufferPtr = 0;
		bufferSize = -1;
		freePos.ClearFree();
		freeSize.ClearFree();
	}
	//size of memory required to always fit "count" alive allocations of size <= "size"
	//note: this property is critical to make sure that idScriptObject addressing works in 64-bit mode!
	static int GetSizeUpperBound(int count, int size) {
		return 2 * count * (size + sizeof(int));
	}
	void Init(void *ptr, int size) {
		Clear();
		bufferPtr = (char*)ptr;
		bufferSize = size;
		freePos.Append(0);
		freeSize.Append(bufferSize);
	}

	void *Alloc(int size) {
		assert(size >= 0);
		size += sizeof(int);

		int i;
		for (i = 0; i < freeSize.Num(); i++)
			if (freeSize[i] >= size)
				break;
		if (i == freeSize.Num())
			return NULL;

		int pos = freePos[i];
		*(int*)(bufferPtr + pos) = size;
		char *res = bufferPtr + pos + sizeof(int);

		freeSize[i] -= size;
		freePos[i] += size;
		if (freeSize[i] == 0) {
			freeSize.RemoveIndex(i);
			freePos.RemoveIndex(i);
		}

		return res;
	}

	void Free(void *pntr) {
		char *ptr = (char*)pntr;
		assert(ptr >= bufferPtr + sizeof(int) && ptr <= bufferPtr + bufferSize);
		ptr -= sizeof(int);
		int size = *(int*)ptr;
		int pos = ptr - bufferPtr;

		int i;
		for (i = 0; i < freePos.Num(); i++)
			if (freePos[i] >= pos)
				break;

		freePos.Insert(pos, i);
		freeSize.Insert(size, i);

		bool merged = CoaslesceBlocks(i-1);
		i -= int(merged);
		CoaslesceBlocks(i);
	}

	bool CoaslesceBlocks(int idx) {
		if (!(idx >= 0 && idx + 1 < freePos.Num()))
			return false;
		if (freePos[idx] + freeSize[idx] != freePos[idx+1])
			return false;
		assert(freePos[idx] + freeSize[idx] <= freePos[idx+1]);
		freeSize[idx] += freeSize[idx+1];
		freePos.RemoveIndex(idx+1);
		freeSize.RemoveIndex(idx+1);
		return true;
	}

	static void Test(int count, int size, int tries);
};

#endif
