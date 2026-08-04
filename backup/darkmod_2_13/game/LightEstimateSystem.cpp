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

		const idRenderModel *oldModel = trackedEnt.modelIndex >= 0 ? modelsCache[trackedEnt.modelIndex].model : nullptr;
		const idRenderModel *newModel = GetModelOfEntity(ent);
		if (oldModel != newModel) {
			// model change: extract tracked entity for special handling
			ForgetAllQueries(trackedEnt);
			entitiesWithModelChange.AddGrow(trackedEnt);
			continue;
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

		if (trackedEnt.modelIndex < 0)
			continue;
		ModelCache &mcache = modelsCache[trackedEnt.modelIndex];
		float areaPerSample = mcache.approxAreaPerSample;
		float sampleRadius = idMath::Sqrt(areaPerSample);

		for (int s = 0; s < trackedEnt.samples.Num(); s++) {
			EvaluatedSample smp = trackedEnt.samples[s];
			float brightness = idVec3(DARKMOD_LG_RED, DARKMOD_LG_GREEN, DARKMOD_LG_BLUE) * smp.value;

			if (smp.excluded)
				continue;
			int deltaTime = lastThinkTime - smp.evalTime;
			float validity = idMath::Fmax(1.0f - float(deltaTime) / mcache.period, 0.0f);

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
		assert((pend.queryId < 0) == pend.proto.excluded);

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

	if (trackedEnt.modelIndex < 0)
		return 0;		// no model?
	const ModelCache &mcache = modelsCache[trackedEnt.modelIndex];
	assert(GetModelOfEntity(entity) == mcache.model);

	int oldNumPending = trackedEnt.pending.Num();
	for (int s = 0; s < mcache.samples.Num(); s++) {
		// evaluation moments for each sample must have prescribed reminder module period
		int maxK = (nowTime + mcache.period - mcache.schedule[s]) / mcache.period - 1;
		int maxEvalTime = mcache.schedule[s] + maxK * mcache.period;
		assert(maxEvalTime <= nowTime && maxEvalTime + mcache.period > nowTime);
		if (trackedEnt.samples[s].evalTime >= maxEvalTime)
			continue;	// fresh enough

		PendingSample pend;
		pend.sampleIndex = s;
		pend.proto.evalTime = nowTime;
		pend.proto.excluded = ShouldSampleBeExcluded(entity, mcache.samples[s]);
		// don't waste time on excluded sample, don't send query
		if (!pend.proto.excluded) {
			idList<qhandle_t> ignoredHandles = GetIgnoredRenderEntityList(entity);
			pend.queryId = gameRenderWorld->LightAtPointQuery_AddQuery(entity->GetModelDefHandle(), mcache.samples[s], ignoredHandles);
		}

		trackedEnt.pending.AddGrow(pend);
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
	const renderEntity_t *rent = gameRenderWorld->GetRenderEntity(entity->GetModelDefHandle());
	const idRenderModel *rmodel = rent->hModel;
	const idMaterial *material = rmodel->GetSampleMaterial(rent, sample);
	if (!material->IsDrawn())
		return true;
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

	if (const idRenderModel *rmodel = GetModelOfEntity(entity)) {
		const ModelCache &mcache = FindOrAddModel(rmodel);
		trackedEnt.modelIndex = modelsCache.IndexOf(&mcache);

		int n = mcache.samples.Num();
		trackedEnt.samples.SetNum(n);
		for (int i = 0; i < n; i++)
			trackedEnt.samples[i] = EvaluatedSample();	// reset to defaults
	}

	return trackedEnt;
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
	int n = mcache.samples.Num();

	mcache.period = g_lesEvaluationPeriod.GetInteger();
	mcache.schedule.SetNum(n);
	for (int i = 0; i < n; i++)
		mcache.schedule[i] = mcache.period * i / n;

	idVec3 boxSize = model->Bounds().GetSize();
	mcache.approxAreaPerSample = (boxSize.x * boxSize.y + boxSize.y * boxSize.z + boxSize.z * boxSize.x);
	mcache.approxAreaPerSample /= idMath::Imax(n, 1);

	return mcache;
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
