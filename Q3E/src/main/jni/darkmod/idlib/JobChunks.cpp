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
#include "JobChunks.h"

#include "ParallelJobList.h"

void JobsInChunks::Clear() {
	jobs.Clear();
	chunks.Clear();
}

void JobsInChunks::AddJob(void *param, float jobTime) {
	assert(jobTime >= 0.0f);
	chjJob_t desc;
	desc.param = param;
	desc.time = jobTime;
	jobs.AddGrow(desc);
}

void JobsInChunks::MakeChunks(float mainChunkTime, int workersNum, float smoothFactor) {
	TRACE_CPU_SCOPE_FORMAT("MakeChunks", "%d jobs / %d threads", jobs.Num(), workersNum);
	// make sure: long jobs run first, short jobs are at tail
	// this naturally reduces partially occupied workers at the end
	std::sort(jobs.begin(), jobs.end(), [](const chjJob_t &a, const chjJob_t &b) {
		return a.time > b.time;
	});

	double remainsTime = 0.0;
	for (int i = 0; i < jobs.Num(); i++)
		remainsTime += jobs[i].time;

	chunks.Clear();

	float chunkSize = mainChunkTime;
	int done = 0;
	while (done < jobs.Num()) {
		// if there is enough total work remaining, better split the rest into smaller chunks
		float chunkLimit = remainsTime / (2.0f * smoothFactor * workersNum);
		chunkSize = idMath::Fmin(chunkSize, chunkLimit);

		// always put at least one job into a chunk
		int taken = done + 1;
		double takenTime = jobs[done].time;
		// add as many more jobs as possible
		while (taken < jobs.Num() && takenTime + jobs[taken].time <= chunkSize) {
			takenTime += jobs[taken].time;
			taken++;
		}

		chjChunk_t chunk;
		chunk.start = done;
		chunk.end = taken;
		chunk.time = takenTime;
		chunk.jobsPtr = nullptr;
		chunks.AddGrow(chunk);

		done = taken;
		remainsTime -= takenTime;
	}
}

void JobsInChunks::AddToJobList(idParallelJobList *jobList, void (*jobFunc)(const chjChunk_t *)) {
	for (int i = 0; i < chunks.Num(); i++) {
		chunks[i].jobsPtr = jobs.Ptr();
		jobList->AddJob((jobRun_t)jobFunc, &chunks[i]);
	}
}
