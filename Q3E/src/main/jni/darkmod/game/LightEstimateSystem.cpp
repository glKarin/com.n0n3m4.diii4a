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
#include "game/LightEstimateSystem.h"

#include "renderer/tr_local.h"

idCVar g_lesDefaultTrackDuration(
	"g_lesDefaultTrackDuration", "10000", CVAR_GAME | CVAR_INTEGER,
	"For how long LightEstimateSystem tracks something due to light value query (in ms)"
);
idCVar g_lesEvaluationPeriod(
	"g_lesEvaluationPeriod", "300", CVAR_GAME | CVAR_INTEGER,
	"Evaluation of all samples is spread across this period of game time (in ms)"
);

idCVar g_lesSamplingMinPerSurface(
	"g_lesSamplingMinPerSurface", "1", CVAR_GAME | CVAR_INTEGER,
	"Minimum number of samples to have on any surface"
);
idCVar g_lesSamplingMaxPerModel(
	"g_lesSamplingMaxPerModel", "10", CVAR_GAME | CVAR_INTEGER,
	"Maximum number of samples on model (as long as every surface has its min)"
);
idCVar g_lesSamplingAreaPerSample(
	"g_lesSamplingAreaPerSample", "1000.0f", CVAR_GAME | CVAR_FLOAT,
	"Number of samples is decided to have this amount of area per sample"
);
idCVar g_lesAverageMaxSamples(
	"g_lesAverageMaxSamples", "3", CVAR_GAME | CVAR_FLOAT,
	"Only K samples with max brightness are averaged into final brightness value"
);

idCVar g_lesSingleEntity(
	"g_lesSingleEntity", "", CVAR_GAME,
	"If non-empty, then only one entity with specified name is processed (debugging)",
	idGameLocal::ArgCompletion_EntityName
);

static idCVar* cvarsToClearOnChange[] = {&g_lesEvaluationPeriod, &g_lesSamplingMinPerSurface, &g_lesSamplingMaxPerModel, &g_lesSamplingAreaPerSample, &g_lesSingleEntity};

// if modelIndex == X, then use idEntity::m_lesExplicitSampling
static const int MODEL_INDEX_EXPLICIT = 1000000000;

void LightEstimateSystem::Clear() {
	for (int i = 0; i < trackedEntities.Num(); i++)
		ForgetAllQueries(trackedEntities[i]);
	// reset to default values
	*this = LightEstimateSystem();
}

void LightEstimateSystem::Think() {
	TRACE_CPU_SCOPE("LES::Think")

	if (loadedEntities.Num() > 0) {
		// post-restore processing
		for (int i = 0; i < loadedEntities.Num(); i++) {
			TrackedEntity &trackedEnt = FindOrAddEntity(loadedEntities[i].entity.GetEntity());
			trackedEnt.trackedUntil = loadedEntities[i].trackedUntil;
		}
		loadedEntities.Clear();
	}

	bool needClear = false;
	for (idCVar *pvar : cvarsToClearOnChange) {
		if (pvar->IsModified()) {
			pvar->ClearModified();
			needClear = true;	// cvar modified
		}
	}
	for (int i = 0; i < modelsCache.Num(); i++) {
		ModelCache &mcache =  modelsCache[i];
		int oldVersion = mcache.loadVersion;
		int newVersion = mcache.model->GetLoadVersion();
		if (newVersion != oldVersion)
			needClear = true;	// model content was changed (reloadModels)
	}
	if (needClear) {
		// full restart to ensure changes take effect
		Clear();
	}

	int newNum = 0;
	int newTime = gameLocal.time;

	// special care is needed if model has changed
	idList<TrackedEntity> entitiesWithModelChange;

	for (int i = 0; i < trackedEntities.Num(); i++) {
		TRACE_CPU_SCOPE("LES::EntityThink")
		TrackedEntity &trackedEnt = trackedEntities[i];
		const idEntity *ent = trackedEnt.entity.GetEntity();

		if (!ent) {
			// tracked entity has died
			ForgetAllQueries(trackedEnt);
			continue;
		}
		TRACE_ATTACH_TEXT(ent->GetName())

		if (trackedEnt.trackedUntil < newTime) {
			// no longer needs to be tracked
			ForgetAllQueries(trackedEnt);
			continue;
		}

		if (trackedEnt.needsUpdate) {
			// marked for update explicitly
			ForgetAllQueries(trackedEnt);
			entitiesWithModelChange.AddGrow(trackedEnt);
			continue;
		}

		if (trackedEnt.modelIndex != MODEL_INDEX_EXPLICIT) {
			const idRenderModel *oldModel = trackedEnt.modelIndex >= 0 ? modelsCache[trackedEnt.modelIndex].model : nullptr;
			const idRenderModel *newModel = GetModelOfEntity(ent);
			if (oldModel != newModel) {
				// model change: extract tracked entity for special handling
				ForgetAllQueries(trackedEnt);
				entitiesWithModelChange.AddGrow(trackedEnt);
				continue;
			}
		}

		// check if any pending light queries are ready
		int received = ReceiveQueryResults(trackedEnt);
		// start new light queries if needed
		int started = StartNewQueries(trackedEnt);
		TRACE_ATTACH_FORMAT("recv %d\nstart %d\n", received, started)

		trackedEntities[newNum++] = trackedEntities[i];
	}
	trackedEntities.SetNum(newNum, false);

	for (int i = 0; i < entitiesWithModelChange.Num(); i++) {
		const idEntity *ent = entitiesWithModelChange[i].entity.GetEntity();
		TRACE_CPU_SCOPE_TEXT("LES::EntityReadd", ent->GetName())
		// readd entity back so that everything is updated from new model
		TrackedEntity &trackedEnt = FindOrAddEntity(ent);
		assert(trackedEntities.IndexOf(&trackedEnt) >= newNum);
		// restore "tracked until time moment"
		trackedEnt.trackedUntil = entitiesWithModelChange[i].trackedUntil;
	}

	lastThinkTime = newTime;
}

void LightEstimateSystem::DebugVisualize() {
	const idPlayer *player = gameLocal.GetLocalPlayer();
	idMat3 eyeAxis = player->viewAngles.ToMat3();
	idVec3 eyeOrigin = player->normalViewOrigin;

	for (int i = 0; i < trackedEntities.Num(); i++) {
		TrackedEntity &trackedEnt = trackedEntities[i];
		const idEntity *ent = trackedEnt.entity.GetEntity();
		if (DebugIgnorePlayer(ent))
			continue;

		const LesModelSampling *msampling = GetSamplingOfTrackedEntity(trackedEnt);
		if (!msampling)
			continue;
		float areaPerSample = msampling->approxAreaPerSample;
		float sampleRadius = idMath::Sqrt(areaPerSample);

		for (int s = 0; s < trackedEnt.samples.Num(); s++) {
			EvaluatedSample smp = trackedEnt.samples[s];
			float brightness = idVec3(DARKMOD_LG_RED, DARKMOD_LG_GREEN, DARKMOD_LG_BLUE) * smp.value;

			if (smp.excluded)
				continue;
			int deltaTime = lastThinkTime - smp.evalTime;
			float validity = idMath::Fmax(1.0f - float(deltaTime) / msampling->period, 0.0f);

			float boxSize = sampleRadius * 0.02f;
			float textSize = sampleRadius * 0.003f;
			idBox tmpBox(bounds_zero.Expand(boxSize), smp.position, mat3_identity);
			gameRenderWorld->DebugFilledBox(idVec4(smp.value, validity), tmpBox);
			gameRenderWorld->DebugText(va("%0.3f", brightness), smp.position, textSize, colorYellow, eyeAxis);
		}
	}
}

bool LightEstimateSystem::DebugIgnorePlayer(const idEntity *entity) const {
	if (!entity)
		return true;
	const idPlayer *player = gameLocal.GetLocalPlayer();
	if (entity == player || entity->GetTeamMaster() == player->GetTeamMaster())
		return true;
	if (entity == gameLocal.m_lightGem.m_LightgemSurface.GetEntity())
		return true;
	const char *single = g_lesSingleEntity.GetString();
	if (single[0] && idStr::Icmp(entity->GetName(), single))
		return true;
	return false;
}

bool LightEstimateSystem::DebugGetLightOnEntity(const idEntity *entity, float &result) const {
	if (const TrackedEntity *trackedEnt = FindEntity(entity)) {
		result = ComputeAverageLightBrightness(*trackedEnt);
		return true;
	} else {
		result = 0.0f;
		return false;
	}
}

float LightEstimateSystem::ComputeAverageLightBrightness(const TrackedEntity &trackedEnt) const {
	// collect brightness from all samples
	idFlexList<float, 128> brightnessList;
	for (int i = 0; i < trackedEnt.samples.Num(); i++) {
		const EvaluatedSample &smp = trackedEnt.samples[i];
		if (smp.excluded)
			continue;
		float brightness = smp.value * idVec3(DARKMOD_LG_RED, DARKMOD_LG_GREEN, DARKMOD_LG_BLUE);
		brightnessList.AddGrow(brightness);
	}

	// if there are many samples, drop excessive ones with minimum values
	std::sort(brightnessList.Ptr(), brightnessList.Ptr() + brightnessList.Num(), std::greater<float>());
	int maxn = g_lesAverageMaxSamples.GetInteger();
	if (brightnessList.Num() > maxn)
		brightnessList.SetNum(maxn);

	// compute average of remaining (max) samples
	float totalBrightness = 0.0f;
	for (int i = 0; i < brightnessList.Num(); i++)
		totalBrightness += brightnessList[i];
	// note: return 0.0 if there are no acceptable samples
	float avgBrightness = totalBrightness / idMath::Imax(brightnessList.Num(), 1);

	return avgBrightness;
}

float LightEstimateSystem::GetLightOnEntity(const idEntity *entity) {
	const char *single = g_lesSingleEntity.GetString();
	if (single[0] && idStr::Icmp(entity->GetName(), single))
		return 0.0f;
	int nowTime = gameLocal.time;
	TrackedEntity& trackedEnt = FindOrAddEntity(entity);
	trackedEnt.trackedUntil = idMath::Imax(trackedEnt.trackedUntil, nowTime + g_lesDefaultTrackDuration.GetInteger());
	return ComputeAverageLightBrightness(trackedEnt);
}

void LightEstimateSystem::TrackEntity(const idEntity *entity, int duration) {
	const char *single = g_lesSingleEntity.GetString();
	if (single[0] && idStr::Icmp(entity->GetName(), single))
		return;
	if (duration < 0)
		duration = g_lesDefaultTrackDuration.GetInteger();
	int nowTime = gameLocal.time;
	TrackedEntity& trackedEnt = FindOrAddEntity(entity);
	trackedEnt.trackedUntil = idMath::Imax(trackedEnt.trackedUntil, nowTime + duration);
}

void LightEstimateSystem::ForgetAllQueries(TrackedEntity &trackedEnt) {
	for (int i = 0; i < trackedEnt.pending.Num(); i++) {
		int id = trackedEnt.pending[i].queryId;
		if (id >= 0)
			gameRenderWorld->LightAtPointQuery_Forget(id);
	}
	trackedEnt.pending.Clear();
}

int LightEstimateSystem::ReceiveQueryResults(TrackedEntity &trackedEnt) {
	int newNum = 0;

	for (int i = 0; i < trackedEnt.pending.Num(); i++) {
		PendingSample pend = trackedEnt.pending[i];

		bool finished = false;
		if (pend.queryId < 0) {
			finished = true;
		} else if (gameRenderWorld->LightAtPointQuery_CheckResult(pend.queryId, pend.proto.value, pend.proto.position)) {
			gameRenderWorld->LightAtPointQuery_Forget(pend.queryId);
			finished = true;
		}

		if (finished) {
			// replace old value of sample
			trackedEnt.samples[pend.sampleIndex] = pend.proto;
		} else {
			// back to pending
			trackedEnt.pending[newNum++] = pend;
		}
	}

	int received = trackedEnt.pending.Num() - newNum;
	trackedEnt.pending.SetNum(newNum, false);
	return received;
}

int LightEstimateSystem::StartNewQueries(TrackedEntity &trackedEnt) {
	int nowTime = gameLocal.time;
	const idEntity *entity = trackedEnt.entity.GetEntity();

	const LesModelSampling *msampling = GetSamplingOfTrackedEntity(trackedEnt);
	if (!msampling)
		return 0;
	
	int oldNumPending = trackedEnt.pending.Num();
	for (int s = 0; s < msampling->samples.Num(); s++) {
		// evaluation moments for each sample must have prescribed reminder module period
		int maxK = (nowTime + msampling->period - msampling->schedule[s]) / msampling->period - 1;
		int maxEvalTime = msampling->schedule[s] + maxK * msampling->period;
		assert(maxEvalTime <= nowTime && maxEvalTime + msampling->period > nowTime);

		// the most recent moment when this sample was evaluated
		// important: including the pending samples!
		// this becomes an issue when several game tics happen per frame due to low FPS
		int lastEvalTime = trackedEnt.samples[s].evalTime;
		for (int p = 0; p < oldNumPending; p++) {
			const PendingSample& pold = trackedEnt.pending[p];
			if (pold.sampleIndex == s)
				lastEvalTime = idMath::Imax(lastEvalTime, pold.proto.evalTime);
		}

		if (lastEvalTime >= maxEvalTime)
			continue;	// fresh enough

		PendingSample pnew;
		pnew.sampleIndex = s;
		pnew.proto.evalTime = nowTime;
		pnew.proto.excluded = ShouldSampleBeExcluded(entity, msampling->samples[s]);
		// don't waste time on excluded sample, don't send query
		if (!pnew.proto.excluded) {
			idList<qhandle_t> ignoredHandles = GetIgnoredRenderEntityList(entity);
			pnew.queryId = gameRenderWorld->LightAtPointQuery_AddQuery(entity->GetModelDefHandle(), msampling->samples[s], ignoredHandles);
		}

		trackedEnt.pending.AddGrow(pnew);

		// note: LQS in renderer frontend processes !all! pending queries in every Think call
		// so if we already have pending queries for this sample, we can drop the older ones
		for (int p = 0; p < oldNumPending; p++) {
			PendingSample &pold = trackedEnt.pending[p];
			assert(pnew.proto.evalTime >= pold.proto.evalTime);
			if (pold.sampleIndex == s && pold.queryId >= 0) {
				gameRenderWorld->LightAtPointQuery_Forget(pold.queryId);
				pold.queryId = -1;
			}
		}
	}

	return trackedEnt.pending.Num() - oldNumPending;
}

idList<qhandle_t> LightEstimateSystem::GetIgnoredRenderEntityList(const idEntity *entity) const {
	idList<qhandle_t> ignoredHandles;

	if (entity->GetTeamMaster() == nullptr) {
		// isolated entity
		ignoredHandles.Append(entity->GetModelDefHandle());
	} else {
		// entity and its attachments are one group
		// they should be ignored from shadow casters when light is computed
		for (idEntity *ent = entity->GetTeamMaster(); ent; ent = ent->GetNextTeamEntity())
			ignoredHandles.Append(ent->GetModelDefHandle());
	}

	return ignoredHandles;
}

bool LightEstimateSystem::ShouldSampleBeExcluded(const idEntity *entity, const samplePointOnModel_t &sample) const {
	if (sample.surfaceIndex >= 0) {
		const renderEntity_t *rent = gameRenderWorld->GetRenderEntity(entity->GetModelDefHandle());
		const idRenderModel *rmodel = rent->hModel;
		const idMaterial *material = rmodel->GetSampleMaterial(rent, sample);
		if (!material->IsDrawn() && !entity->spawnArgs.GetBool("les_sample_invisible"))
			return true;
	}
	return false;
}

const idRenderModel *LightEstimateSystem::GetModelOfEntity(const idEntity *entity) const {
	if (!entity)
		return nullptr;

	qhandle_t renderHandle = entity->GetModelDefHandle();
	if (renderHandle < 0)
		return nullptr;

	const renderEntity_t *rent = gameRenderWorld->GetRenderEntity(renderHandle);
	if (!rent)
		return nullptr;

	const idRenderModel *rmodel = rent->hModel;
	if (rmodel->IsDynamicModel() && !rmodel->GetDefaultPose())
		return nullptr;		// only consider static and MD5 models

	return rmodel;
}

const LightEstimateSystem::TrackedEntity *LightEstimateSystem::FindEntity(const idEntity *entity) const {
	assert(entity);
	for (int i = 0; i < trackedEntities.Num(); i++) {
		if (trackedEntities[i].entity.GetEntity() == entity)
			return &trackedEntities[i];
	}
	return nullptr;
}

LightEstimateSystem::TrackedEntity &LightEstimateSystem::FindOrAddEntity(const idEntity *entity) {
	if (auto found = FindEntity(entity)) {
		return *const_cast<TrackedEntity*>(found);
	}

	TrackedEntity& trackedEnt = trackedEntities.Alloc();
	trackedEnt = TrackedEntity();	// reset members to defaults
	trackedEnt.entity = entity;
	trackedEnt.periodStartsAt = gameLocal.time;

	const LesModelSampling *sampling = nullptr;
	if (entity->m_lesExplicitSampling) {
		// override rendermodel with sampling stored in the entity
		trackedEnt.modelIndex = MODEL_INDEX_EXPLICIT;
		sampling = entity->m_lesExplicitSampling;
	}
	else if (const idRenderModel *rmodel = GetModelOfEntity(entity)) {
		const ModelCache &mcache = FindOrAddModel(rmodel);
		trackedEnt.modelIndex = modelsCache.IndexOf(&mcache);
		sampling = &mcache;
	}

	if (sampling) {
		int n = sampling->samples.Num();
		trackedEnt.samples.SetNum(n);
		for (int i = 0; i < n; i++)
			trackedEnt.samples[i] = EvaluatedSample();	// reset to defaults
	}

	return trackedEnt;
}

const LesModelSampling *LightEstimateSystem::GetSamplingOfTrackedEntity(const TrackedEntity &trackedEnt) const {
	const idEntity *entity = trackedEnt.entity.GetEntity();

	if (trackedEnt.modelIndex < 0)
		return nullptr;		// no model
	if (trackedEnt.modelIndex == MODEL_INDEX_EXPLICIT)
		return entity->m_lesExplicitSampling;

	const ModelCache &mcache = modelsCache[trackedEnt.modelIndex];
	assert(GetModelOfEntity(entity) == mcache.model);
	return &mcache;
}

LightEstimateSystem::ModelCache &LightEstimateSystem::FindOrAddModel(const idRenderModel *model) {
	assert(model);

	for (int i = 0; i < modelsCache.Num(); i++) {
		if (modelsCache[i].model == model)
			return modelsCache[i];
	}

	ModelCache& mcache = modelsCache.Alloc();
	mcache.model = model;
	mcache.loadVersion = model->GetLoadVersion();

	modelSamplingParameters_t params;
	params.poisson = true;
	params.minSamplesPerSurface = g_lesSamplingMinPerSurface.GetInteger();
	params.maxSamplesTotal = g_lesSamplingMaxPerModel.GetInteger();
	params.areaPerSample = g_lesSamplingAreaPerSample.GetFloat();
	idRandom rnd(123456789);	// samples are deterministic for now
	model->GenerateSamples(mcache.samples, params, rnd);

	FinishModelSampling(mcache, model->Bounds());
	return mcache;
}

void LightEstimateSystem::FinishModelSampling(LesModelSampling &msampling, const idBounds &bbox) {
	int n = msampling.samples.Num();

	msampling.period = g_lesEvaluationPeriod.GetInteger();
	msampling.schedule.SetNum(n);
	for (int i = 0; i < n; i++)
		msampling.schedule[i] = msampling.period * i / n;

	idVec3 boxSize = bbox.GetSize();
	msampling.approxAreaPerSample = (boxSize.x * boxSize.y + boxSize.y * boxSize.z + boxSize.z * boxSize.x);
	msampling.approxAreaPerSample /= idMath::Imax(n, 1);
}

void LightEstimateSystem::SetExplicitSamplingForEntity(idEntity *entity, const idList<idVec3> *samples) {
	LesModelSampling *msampling = nullptr;

	if (samples) {
		int n = samples->Num();

		msampling = new LesModelSampling();
		msampling->samples.SetNum(n);
		idBounds bbox;
		bbox.Clear();
		for (int i = 0; i < n; i++) {
			samplePointOnModel_t &smp = msampling->samples[i];
			smp.staticPosition = (*samples)[i];
			smp.surfaceIndex = -1;
			smp.triangleIndex = -1;
			smp.baryCoords = idVec3(0.0f);
			bbox.AddPoint(smp.staticPosition);
		}
		bbox.ExpandSelf(1.0f);
		FinishModelSampling(*msampling, bbox);
	}

	delete entity->m_lesExplicitSampling;
	entity->m_lesExplicitSampling = msampling;

	if (const TrackedEntity *found = FindEntity(entity)) {
		// mark this entity so that it's completely readded during next Think
		const_cast<TrackedEntity*>(found)->needsUpdate = true;
	}
}

void LightEstimateSystem::Save(idSaveGame *savegame) const {
	savegame->WriteInt(lastThinkTime);

	savegame->WriteInt(trackedEntities.Num());

	for (int i = 0; i < trackedEntities.Num(); i++) {
		const TrackedEntity &trent = trackedEntities[i];
		trent.entity.Save(savegame);
		savegame->WriteInt(trent.trackedUntil);
		// we don't save other information
		// it is model-dependent and will be recomputed on load
	}
}

void LightEstimateSystem::Restore(idRestoreGame *savegame) {
	Clear();

	savegame->ReadInt(lastThinkTime);

	int trackedNum;
	savegame->ReadInt(trackedNum);
	loadedEntities.SetNum(trackedNum);

	for (int i = 0; i < trackedNum; i++) {
		// this info will get applied on next Think
		loadedEntities[i].entity.Restore(savegame);
		savegame->ReadInt(loadedEntities[i].trackedUntil);
		// other model-dependent information will get recomputed in one period
	}
}

void LightEstimateSystem::SaveExplicitSampling(idSaveGame *savefile, const idEntity *entity) {
	if (const LesModelSampling *msampling = entity->m_lesExplicitSampling) {
		int n = msampling->samples.Num();
		savefile->WriteInt(n);
		for (int i = 0; i < n; i++) {
			const samplePointOnModel_t &smp = msampling->samples[i];
			savefile->WriteVec3(smp.staticPosition);
			savefile->WriteInt(smp.surfaceIndex);
			savefile->WriteInt(smp.triangleIndex);
			savefile->WriteVec3(smp.baryCoords);
		}
		savefile->WriteInt(msampling->period);
		savefile->WriteFloat(msampling->approxAreaPerSample);
		for (int i = 0; i < n; i++)
			savefile->WriteInt(msampling->schedule[i]);
	}
	else {
		savefile->WriteInt(-1);
	}
}

void LightEstimateSystem::RestoreExplicitSampling(idRestoreGame *savefile, idEntity *entity) {
	int n;
	savefile->ReadInt(n);

	LesModelSampling *msampling = nullptr;
	if (n >= 0) {
		msampling = new LesModelSampling();
		msampling->samples.SetNum(n);
		for (int i = 0; i < n; i++) {
			samplePointOnModel_t &smp = msampling->samples[i];
			savefile->ReadVec3(smp.staticPosition);
			savefile->ReadInt(smp.surfaceIndex);
			savefile->ReadInt(smp.triangleIndex);
			savefile->ReadVec3(smp.baryCoords);
		}
		savefile->ReadInt(msampling->period);
		savefile->ReadFloat(msampling->approxAreaPerSample);
		msampling->schedule.SetNum(n);
		for (int i = 0; i < n; i++)
			savefile->ReadInt(msampling->schedule[i]);
	}

	delete entity->m_lesExplicitSampling;
	entity->m_lesExplicitSampling = msampling;
	// during restore, LES is cleared completely, so update will happen immediately afterwards
}
