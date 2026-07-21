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
#pragma once

#include "containers/List.h"

typedef struct chjJob_s {
	float time;
	void *param;
} chjJob_t;

typedef struct chjChunk_s {
	chjJob_t *jobsPtr;
	float time;
	int start, end;
} chjChunk_t;

class JobsInChunks {
public:
	void Clear();
	void AddJob(void *param, float jobTime);
	void MakeChunks(float chunkTime, int workersNum, float smoothFactor = 1.0f);

	// does AddJob for chunks
	// don't forget to Submit and Wait afterwards
	void AddToJobList(idParallelJobList *jobList, void (*jobFunc)(const chjChunk_t *));

private:
	idList<chjJob_t> jobs;
	idList<chjChunk_t> chunks;
};
