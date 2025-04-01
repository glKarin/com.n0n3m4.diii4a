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

#ifndef __ID_DEBUG__
#define __ID_DEBUG__

#include <stdint.h>

const char *CleanupSourceCodeFileName(const char *fileName);

// some utilities for getting debug information in runtime
class idDebugSystem {
public:
	// captures call stack of function calling this one into specified binary blob
	// len is: size of data buffer refore call, size of written data after call
	// returns nonzero hash code of the call stack (same call stacks have equal hash codes)
	static uint32_t GetStack(uint8_t *data, int &len);

	// decodes previously captured call stack as an array of stack frames
	// most recent stack frame goes first in the array
	static void DecodeStack(uint8_t *data, int len, idList<debugStackFrame_t> &info);

	// cleans the stack trace according to default TDM rules:
	//   cutWinMain: remove all stack frames above CRT "main" inclusive
	//   shortenPath: remove path prefix ending at "darkmod_src" to make path relative to source code root
	static void CleanStack(idList<debugStackFrame_t> &info, bool cutWinMain = true, bool shortenPath = true);

	// converts given call stack (array of stack frames) into readable string
	// str and maxLen specify the receiving string buffer
	static void StringifyStack(uint32_t hash, const debugStackFrame_t *frames, int framesCnt, char *str, int maxLen);

	// moved from Heap.cpp (Mem_CleanupFileName)
	// caller must copy the returned string immediately!
	// note: does not allocate any memory in anyway
	static const char *CleanupFileName(const char *fileName) { return CleanupSourceCodeFileName(fileName); }
};

#endif
