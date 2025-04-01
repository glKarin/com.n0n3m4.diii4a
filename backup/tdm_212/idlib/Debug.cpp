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

#include "Debug.h"


uint32_t idDebugSystem::GetStack(uint8_t *data, int &len) {
	Sys_CaptureStackTrace(2, data, len);
	uint32_t hash = idStr::Hash((char*)data, len);
	if (hash == 0) hash = (uint32_t)-1;
	return hash;
}

void idDebugSystem::DecodeStack(uint8_t *data, int len, idList<debugStackFrame_t> &info) {
	int cnt = Sys_GetStackTraceFramesCount(data, len);
	info.SetNum(cnt);
	Sys_DecodeStackTrace(data, len, info.Ptr());
}

void idDebugSystem::CleanStack(idList<debugStackFrame_t> &info, bool cutWinMain, bool shortenPath) {
	if (shortenPath) {
		for (int i = 0; i < info.Num(); i++) {
			const char *filename = idDebugSystem::CleanupFileName(info[i].fileName);
			strcpy(info[i].fileName, filename);
		}
	}
	if (cutWinMain) {
		int k = 0;
		for (k = 0; k < info.Num(); k++) {
			const auto &fr = info[k];
			if (strcmp(fr.functionName, "main") == 0 ||
				strcmp(fr.functionName, "WinMain") == 0 ||
				strcmp(fr.functionName, "WinMainCRTStartup") == 0 ||
				strcmp(fr.functionName, "__tmainCRTStartup") == 0
			) {
				break;
			}
		}
		info.Resize(k);
	}
}

void idDebugSystem::StringifyStack(uint32_t hash, const debugStackFrame_t *frames, int framesCnt, char *str, int maxLen) {
	maxLen--;

	if (hash) {
		idStr::snPrintf(str, maxLen, "Stack trace (hash = %08X):\n", hash);
		int l = idStr::Length(str);
		maxLen -= l;
		str += l;
	}

	static const int ALIGN_LENGTH = 40;
	for (int i = 0; i < framesCnt; i++) {
		const auto &fr = frames[i];

		idStr::snPrintf(str, maxLen, "  %s", fr.functionName);
		int l = idStr::Length(str);
		while (l < maxLen && l < ALIGN_LENGTH)
			str[l++] = ' ';
		maxLen -= l;
		str += l;

		idStr::snPrintf(str, maxLen, "  %s:%d\n", fr.fileName, fr.lineNumber);
		l = idStr::Length(str);
		maxLen -= l;
		str += l;
	}

	str[0] = 0;
}

const char *CleanupSourceCodeFileName(const char *fileName) {
	static char newFileNames[4][MAX_STRING_CHARS];
	static int index;

	index = (index + 1) & 3;
	char *path = newFileNames[index];
	strcpy(path, fileName);

	for (int i = 0; path[i]; i++)
		if (path[i] == '\\')
			path[i] = '/';
	char *tdm = strstr(path, SOURCE_CODE_BASE_FOLDER);
	if (tdm)
		path = tdm + strlen(SOURCE_CODE_BASE_FOLDER);
	while (char *topar = strstr(path, "/../")) {
		char *ptr;
		for (ptr = topar; ptr > path && *(ptr-1) != '/'; ptr--);
		topar += 4;
		memmove(ptr, topar, strlen(topar) + 1);
	}
	return path;
}
