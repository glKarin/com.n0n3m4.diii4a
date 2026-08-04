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

#include "precompiled.h"
#include "Heap_Embedded.h"
#include "Lib.h"
#include <random>
#pragma hdrstop


static void MyAssert(bool cond) {
	if (!cond)
		idLib::common->Error("idEmbeddedAllocator test failed!\n");
}

void idEmbeddedAllocator::Test(int count, int size, int tries) {
	int totalSize = GetSizeUpperBound(count, size);
	void *buffer = Mem_Alloc(totalSize);
	idEmbeddedAllocator alloc;
	alloc.Init(buffer, totalSize);

	std::mt19937 rnd;
	std::uniform_int_distribution<int> distSize(0, size);
	std::uniform_int_distribution<int> percent(0, 99);

	int done = 0;
	idList<char*> usedPtr;
	idList<int> usedSize;
	while (done < tries) {
		if (percent(rnd) < 51 && usedPtr.Num() < count) {
			int newSize = distSize(rnd);
			char *ptr = (char*)alloc.Alloc(newSize);
			MyAssert(ptr != NULL);
			for (int i = 0; i < usedPtr.Num(); i++)
				MyAssert(usedPtr[i] >= ptr + newSize || usedPtr[i] + usedSize[i] <= ptr);
			usedPtr.Append(ptr);
			usedSize.Append(newSize);
			done++;
		}
		else if (usedPtr.Num() > 0) {
			int idx = std::uniform_int_distribution<int>(0, usedPtr.Num() - 1)(rnd);
			char *ptr = usedPtr[idx];
			alloc.Free(ptr);
			usedPtr.RemoveIndex(idx);
			usedSize.RemoveIndex(idx);
			done++;
		}
	}

	Mem_Free(buffer);
}
