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

#include "game/Entity.h"
#include "renderer/resources/Model.h"

class LightEstimateSystem {
public:
	// drops all data
	void Clear();

	// must be called once per frame to do internal bookkeeping
	void Think();
	void Save(idSaveGame *savegame) const;
	void Restore(idRestoreGame *savegame);

	// get current lighting conditions on the entity
	// also tracks the entity for the nearest future
	// note: first calls to GetLightOnEntity after not tracking entity give meaningless results!
	float GetLightOnEntity(const idEntity *entity);

	// make sure to track entity for a given time (in milliseconds) since now
	// -1 means "default duration" like in GetLightOnEntity
	// note: tracking takes CPU time for evaluating light samples
	void TrackEntity(const idEntity *entity, int duration = -1);

	void DebugVisualize();
	bool DebugIgnorePlayer(const idEntity *entity) const;
	bool DebugGetLightOnEntity(const idEntity *entity, float &result) const;

private:
	struct EvaluatedSample {
		int evalTime = -1000000;		// when sample was evaluated (game time)
		bool excluded = false;			// true for surfaces with invisible/nodraw material
		idVec3 value = idVec3(0);		// light value at sample
		idVec3 position = idVec3(0);	// world pos of sample at moment of evaluation
	};

	struct PendingSample {
		int sampleIndex = -1;			// index of sample which is being evaluated
		int queryId = -1;				// light query to renderWorld which is not complete yet
		EvaluatedSample proto;			// prototype of future sample (some members set already)
	};

	struct TrackedEntity {
		idEntityPtr<const idEntity> entity;	// entity being tracked
		int trackedUntil = -1;				// track at least until this moment (game time)
		int modelIndex = -1;				// index in modelsCache
		idList<EvaluatedSample> samples;	// light values for sample points (same-indexed as ModelCache::samples)
		idList<PendingSample> pending;		// pending light queries for samples
		int periodStartsAt = -1;			// when current period started (game time)
	};

	struct ModelCache {
		const idRenderModel *model = nullptr;	// all info corresponds to this model
		int loadVersion = -1;					// used to detect model reloads
		idList<samplePointOnModel_t> samples;	// points to be sampled for light estimation
		int period;								// game time to evaluate all samples
		idList<int> schedule;					// reminder module period where a sample should be evaluated
		float approxAreaPerSample = 0.0f;		// for debug visualization
	};

	// reduced version of TrackedEntity loaded from save
	struct LoadedEntityInfo {
		idEntityPtr<const idEntity> entity;
		int trackedUntil = -1;
	};

	void ForgetAllQueries(TrackedEntity &trackedEnt);
	int ReceiveQueryResults(TrackedEntity &trackedEnt);
	int StartNewQueries(TrackedEntity &trackedEnt);
	float ComputeAverageLightBrightness(const TrackedEntity &trackedEnt) const;

	ModelCache &FindOrAddModel(const idRenderModel *model);
	TrackedEntity &FindOrAddEntity(const idEntity *entity);
	const TrackedEntity *FindEntity(const idEntity *entity) const;

	const idRenderModel *GetModelOfEntity(const idEntity *entity) const;
	idList<qhandle_t> GetIgnoredRenderEntityList(const idEntity *entity) const;
	bool ShouldSampleBeExcluded(const idEntity *entity, const samplePointOnModel_t &sample) const;

	int lastThinkTime = -1;
	idList<TrackedEntity> trackedEntities;
	idList<ModelCache> modelsCache;

	// only valid immediately after game load
	idList<LoadedEntityInfo> loadedEntities;
};
