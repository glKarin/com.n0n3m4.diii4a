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
#pragma hdrstop

#include "tools/compilers/compiler_common.h"
#include "renderer/resources/Image.h"


class PrtCollision {
public:
	~PrtCollision();
	void Run(const char *mapFileName);

private:
	void ProcessMap();
	void ProcessParticleDeform(const char *modelName, const idVec3 &origin, const idMat3 &axis, bool disabled, float diversity);
	void ProcessParticleModel(const char *modelName, const idKeyValue &kvModel, const idVec3 &origin, const idMat3 &axis, bool disabled, float diversity);
	bool ProcessSurfaceEmitter(const srfTriangles_t *geom, const idVec3 &origin, const idMat3 &axis, float entDiversity, const char *outputFilename);
	bool ProcessPointEmitter(const idVec3 &origin, const idMat3 &axis, float entDiversity, const char *outputFilename);
	void ModelParticleMovement(const idVec3 &entOrigin, const idMat3 &entAxis, const idPartStageData &stg, const idPartSysData &psys, idParticleData part, uint32 &texel);
	float FindSegmentCollision(idVec3 start, idVec3 end, bool collideWorldOnly);
	idRenderWorld *RenderWorld();

	// path to .map file
	const char *mapFileName = nullptr;
	// parsed .map file
	idMapFile *mapFile = nullptr;
	// used to parse map/proc file and to compute traces
	idRenderWorld *renderWorld = nullptr;
	// for some entities, the internal filter checks are disabled
	// meaning: material contents/deform check, dynamic model exclusion
	// the array is indexed by map-entity indices
	idList<bool> entityBlockFilterOverride;

	int numSurfsProcessed = 0;
	int numSurfsDisabled = 0;
	int64 numRaysCasted = 0, numRaysHit = 0;
	double timeStarted = 0.0;
	double timeEnded = 0.0;
	double timeElapsed = 0.0;

	//temporary data to avoid passing all the trash around everywhere
	const char *prtName = nullptr;
	const idParticleStage *prtStage = nullptr;
	idPartSysEmitterSignature signature;
};

PrtCollision::~PrtCollision() {
	delete mapFile;
	if (renderWorld)
		renderSystem->FreeRenderWorld(renderWorld);
}

float PrtCollision::FindSegmentCollision(idVec3 start, idVec3 end, bool collideWorldOnly) {
	//find collision
	modelTrace_t mt;
	//hitsCount += renderWorld->Trace(mt, start, end, 0.0f);
	//hitsCount += renderWorld->FastWorldTrace(mt, start, end);
	auto traceFilter = [&](const qhandle_t *handle, const renderEntity_t *rent, const idRenderModel *model, const idMaterial *material) -> bool {
		if (collideWorldOnly)
			return false;			//note: world is included automatically (fastWorld = true)

		bool disableInternalChecks = rent && (unsigned)rent->entityNum < (unsigned)entityBlockFilterOverride.Num() && entityBlockFilterOverride[rent->entityNum];
		if (disableInternalChecks)
			return true;			//mapper asked to disable all the remaining checks

		if (material) {
			if (material->Deform() != DFRM_NONE && material->Deform() != DFRM_TURB)
				return false;		//most importantly, skip particle systems
			if (!(material->GetContentFlags() & (CONTENTS_WATER | CONTENTS_SOLID)))
				return false;		//skip light flares (but don't skip water)
		}
		else if (rent) {
			idRenderModel *hModel = rent->hModel;
			if (hModel && hModel->IsDynamicModel() != DM_STATIC)
				return false;		//we should not load dynamic models, but just in case
		}

		return true;
	};
	numRaysHit += renderWorld->TraceAll(mt, start, end, true, 0.0f, LambdaToFuncPtr(traceFilter), &traceFilter);
	numRaysCasted++;

	return mt.fraction;
}

void PrtCollision::ModelParticleMovement(
	const idVec3 &entOrigin, const idMat3 &entAxis, const idPartStageData &stg,
	const idPartSysData &psys, idParticleData part,
	uint32 &texel
) {
	bool singleTraceOptimization = (
		stg.customPathType == prtCustomPth_t::PPATH_STANDARD &&
		stg.directionType == prtDirection_t::PDIR_CONE && stg.directionParms[0] == 0.0f &&
		stg.gravity == 0.0f &&
		stg.speed.from == stg.speed.to && !stg.speed.table &&
	true);

	int stepsCnt;
	if (singleTraceOptimization) {
		//represent whole travel path as one line segment
		stepsCnt = 1;
	}
	else {
		//approximate travel path as a polyline
		stepsCnt = stg.collisionStaticTimeSteps;
		if (stepsCnt <= 0) {
			common->Error("Particle %s stage %d: requires collisionStaticTimeSteps parameter", prtName, signature.particleStageIndex);
		}
	}

	float collisionAtFrac = 1.0f;

	float prevFrac;
	idVec3 prevPos;
	for (int smpIdx = 0; ; smpIdx++) {
		//fetch current sample
		float currFrac = part.frac = float(smpIdx) / stepsCnt;
		int random = part.randomSeed;
		idVec3 currPos = idParticle_ParticleOrigin(stg, psys, part, random);
		//convert to world coordinates
		currPos = currPos * entAxis + entOrigin;

		if (smpIdx > 0) {
			//check connecting segment for collision
			float ratio = FindSegmentCollision(prevPos, currPos, stg.collisionStaticWorldOnly);
			if (ratio < 1.0f) {
				collisionAtFrac = prevFrac * (1.0f - ratio) + currFrac * ratio;
				break;
			}
		}
		//use this sample as "previous" next iteration
		prevPos = currPos;
		prevFrac = currFrac;
	}

	//convert into color
	int digits[4] = {0, 0, 0, 255};
	float rem = collisionAtFrac;
	for (int d = 0; d < 3; d++) {
		rem *= 256.0f;
		digits[d] = idMath::ClampInt(0, 255, int(rem));
		rem -= digits[d];
	}
	texel = digits[0] + (digits[1] << 8) + (digits[2] << 16) + (digits[3] << 24);
}

bool PrtCollision::ProcessSurfaceEmitter(
	const srfTriangles_t *geom,
	const idVec3 &entOrigin, const idMat3 &entAxis, float entDiversity,
	const char *outputFilename
) {
	assert(prtStage->collisionStatic);
	int stageIdx = signature.particleStageIndex;

	bool supportedWithTextureLayout = (
		((idVec3*)prtStage->distributionParms)->LengthSqr() == 0.0f &&
	true);

	if (prtStage->mapLayoutType == PML_TEXTURE) {
		if (!supportedWithTextureLayout) {
			common->Error("Particle %s stage %d: nonzero distribution not supported with texture mapLayout", prtName, stageIdx);
		}

		idPartSysCutoffTextureInfo texinfo;
		if (!idParticle_FindCutoffTextureSubregion(*prtStage, geom, texinfo)) {
			common->Warning("Particle %s collisionStatic %s: texture coordinates out of [0..1] x [0..1] domain", prtName, outputFilename);
		}

		int bytes = texinfo.sizeX * texinfo.sizeY * 4;
		uint32 *texels = (uint32*)Mem_Alloc(bytes);
		memset(texels, 0, bytes);

		for (int y = texinfo.baseY; y < texinfo.baseY + texinfo.sizeY; y++)
			for (int x = texinfo.baseX; x < texinfo.baseX + texinfo.sizeX; x++) {
				idVec2 texCoord((x + 0.5) / texinfo.resX, (y + 0.5) / texinfo.resY);

				int usedTriNum = 0;
				int insideTriNum = 0;
				int triNum = geom->numIndexes/3;
				for (int t = 0; t < triNum; t++) {
					const idDrawVert &v0 = geom->verts[geom->indexes[3 * t + 0]];
					const idDrawVert &v1 = geom->verts[geom->indexes[3 * t + 1]];
					const idDrawVert &v2 = geom->verts[geom->indexes[3 * t + 2]];
					const idVec2 &st0 = v0.st, &st1 = v1.st, st2 = v2.st;

					static const float TEXAREA_SINGULAR_EPS = 1e-5f;
					static const float BARY_BOUNDARY_EPS = 1e-5f;
					static const float BARY_OVERLAP_EPS = 1e-3f;

					//compute barycentric coordinates for texture
					float bary0 = (texCoord - st1).Cross(st2 - st1);
					float bary1 = (texCoord - st2).Cross(st0 - st2);
					float bary2 = (texCoord - st0).Cross(st1 - st0);
					float totalArea = bary0 + bary1 + bary2;
					if (idMath::Fabs(totalArea) <= TEXAREA_SINGULAR_EPS)
						continue;
					totalArea = 1.0f / totalArea;
					bary0 *= totalArea;
					bary1 *= totalArea;
					bary2 = 1.0f - bary0 - bary1;

					float minBary = idMath::Fmin(idMath::Fmin(bary0, bary1), bary2);
					if (minBary < -BARY_BOUNDARY_EPS)
						continue;
					usedTriNum++;
					if (minBary >= BARY_OVERLAP_EPS)
						insideTriNum++;

					//compute position and axis (exactly as in R_ParticleDeform)
					idParticleData part;
					idPartSysData psys;
					memset(&psys, 0, sizeof(psys));
					memset(&part, 0, sizeof(part));
					psys.entityAxis = entAxis;
					part.origin  = bary0 * v0.xyz         + bary1 * v1.xyz         + bary2 * v2.xyz        ;
					part.axis[0] = bary0 * v0.tangents[0] + bary1 * v1.tangents[0] + bary2 * v2.tangents[0];
					part.axis[1] = bary0 * v0.tangents[1] + bary1 * v1.tangents[1] + bary2 * v2.tangents[1];
					part.axis[2] = bary0 * v0.normal      + bary1 * v1.normal      + bary2 * v2.normal     ;

					//trace particle movement, find and store collision
					uint32 *pTexel = &texels[(y - texinfo.baseY) * texinfo.sizeX + (x - texinfo.baseX)];
					ModelParticleMovement(entOrigin, entAxis, *prtStage, psys, part, *pTexel);
				}

				if (insideTriNum > 1) {
					common->Warning("Particle %s collisionStatic %s: texture coords wrap over themselves at (%s)", prtName, outputFilename, texCoord.ToString());
					return false;
				}
			}

		R_WriteTGA(outputFilename, (byte*)texels, texinfo.sizeX, texinfo.sizeY);
		Mem_Free(texels);
	}
	else {
		assert(prtStage->mapLayoutType == PML_LINEAR);
		if (prtStage->diversityPeriod <= 0) {
			common->Error("Particle %s stage %d: mapLayout linear requires diversityPeriod set", prtName, stageIdx);
		}

		float totalArea = -1.0f;
		int areaCnt = idParticle_PrepareDistributionOnSurface(geom);
		float *areas = (float*)alloca(areaCnt * sizeof(float));
		idParticle_PrepareDistributionOnSurface(geom, areas, &totalArea);

		idPartSysData psys;
		memset(&psys, -1, sizeof(psys));
		int totalParticles = idParticle_GetParticleCountOnSurface(*prtStage, geom, totalArea, psys.totalParticles);
		psys.entityAxis = entAxis;

		int period = prtStage->diversityPeriod;
		int totalTexels = period * totalParticles;
		static const int MAXDIM = 4<<10;
		if (totalTexels > MAXDIM * MAXDIM) {
			common->Error(
				"Particle %s stage %d: number of texels is too large: %d = %d x %d",
				prtName, stageIdx, totalTexels, period, totalParticles
			);
		}

		int w, h;
		if (totalParticles <= MAXDIM && period <= MAXDIM) {
			w = totalParticles;
			h = period;
		}
		else {
			w = MAXDIM;
			h = (totalTexels + MAXDIM-1) / MAXDIM;
		}
		int bytes = w * h * 4;
		uint32 *texels = (uint32*)Mem_Alloc(bytes);
		memset(texels, 0, bytes);

		idPartSysEmit psEmit;
		memset(&psEmit, -1, sizeof(psEmit));
		psEmit.totalParticles = psys.totalParticles;
		psEmit.randomizer = idParticle_ComputeRandomizer(signature, entDiversity);

		for (int cycIdx = 0; cycIdx < period; cycIdx++) {
			for (int index = 0; index < totalParticles; index++) {
				idParticleData part;
				part.index = index;
				part.randomSeed = idParticle_GetRandomSeed(index, cycIdx, psEmit.randomizer);

				idVec2 texcoord;
				idParticle_EmitLocationOnSurface(*prtStage, geom, part, texcoord, areas);

				uint32 *pTexel = &texels[cycIdx * totalParticles + index];
				ModelParticleMovement(entOrigin, entAxis, *prtStage, psys, part, *pTexel);
			}
		}

		R_WriteTGA(outputFilename, (byte*)texels, w, h);
		Mem_Free(texels);
	}

	return true;
}

void PrtCollision::ProcessParticleDeform(const char *modelName, const idVec3 &origin, const idMat3 &axis, bool disabled, float diversity) {
	idRenderModel *model = renderModelManager->CheckModel(modelName);
	if (!model)
		return;
	assert(strcmp(modelName, model->Name()) == 0);

	// look for surfaces with "deform particle" materials
	int surfNum = model->NumSurfaces();
	for (int s = 0; s < surfNum; s++) {
		const modelSurface_t *surf = model->Surface(s);
		const idMaterial *material = surf->material;

		if (material->Deform() == DFRM_PARTICLE || material->Deform() == DFRM_PARTICLE2) {
			const idDeclParticle *particleDecl = (idDeclParticle *)material->GetDeformDecl();
			const auto &prtStages = particleDecl->stages;

			// look for particle stages with "collisionStatic"
			for (int g = 0; g < prtStages.Num(); g++) {
				prtStage = prtStages[g];
				prtName = particleDecl->GetName();

				if (prtStage->collisionStatic) {
					signature.mainName = modelName;
					signature.modelSuffix = "";
					signature.surfaceIndex = s;
					signature.particleStageIndex = g;
					idStr imageName = idParticleStage::GetCollisionStaticImagePath(signature);

					numSurfsProcessed++;
					if (!disabled)
						ProcessSurfaceEmitter(surf->geometry, origin, axis, diversity, imageName.c_str());
					else {
						numSurfsDisabled++;
						byte white[4] = {255, 255, 255, 0};
						R_WriteTGA(imageName, white, 1, 1);
					}
				}

				prtStage = nullptr;
				prtName = nullptr;
			}
		}
	}
}

bool PrtCollision::ProcessPointEmitter(
	const idVec3 &entOrigin, const idMat3 &entAxis, float entDiversity,
	const char *outputFilename
) {
	assert(prtStage->collisionStatic);

	if (prtStage->mapLayoutType == PML_TEXTURE) {
		common->Error("Particle %s on emitter %s: texture mapLayout not allowed on models", prtName, outputFilename);
	}
	assert(prtStage->mapLayoutType == PML_LINEAR);

	if (prtStage->diversityPeriod <= 0) {
		common->Error("Particle %s on emitter %s: mapLayout linear requires diversityPeriod set", prtName, outputFilename);
	}

	idPartSysData psys;
	memset(&psys, -1, sizeof(psys));
	int totalParticles = prtStage->totalParticles;
	psys.entityAxis = entAxis;

	int period = prtStage->diversityPeriod;
	int totalTexels = period * totalParticles;
	static const int MAXDIM = 4<<10;
	if (totalTexels > MAXDIM * MAXDIM) {
		common->Error(
			"Particle %s on emitter %s: number of texels is too large: %d = %d x %d",
			prtName, outputFilename, totalTexels, period, totalParticles
		);
	}

	int w, h;
	if (totalParticles <= MAXDIM && period <= MAXDIM) {
		w = totalParticles;
		h = period;
	}
	else {
		w = MAXDIM;
		h = (totalTexels + MAXDIM-1) / MAXDIM;
	}
	int bytes = w * h * 4;
	uint32 *texels = (uint32*)Mem_Alloc(bytes);
	memset(texels, 0, bytes);

	idPartSysEmit psEmit;
	memset(&psEmit, -1, sizeof(psEmit));
	psEmit.totalParticles = psys.totalParticles;
	psEmit.randomizer = idParticle_ComputeRandomizer(signature, entDiversity);

	for (int cycIdx = 0; cycIdx < period; cycIdx++) {
		for (int index = 0; index < totalParticles; index++) {
			idParticleData part;
			part.index = index;
			part.randomSeed = idParticle_GetRandomSeed(index, cycIdx, psEmit.randomizer);
			part.axis.Identity();
			part.origin.Zero();

			uint32 *pTexel = &texels[cycIdx * totalParticles + index];
			ModelParticleMovement(entOrigin, entAxis, *prtStage, psys, part, *pTexel);
		}
	}

	R_WriteTGA(outputFilename, (byte*)texels, w, h);
	Mem_Free(texels);

	return true;
}

void PrtCollision::ProcessParticleModel(const char *modelName, const idKeyValue &kvModel, const idVec3 &origin, const idMat3 &axis, bool disabled, float diversity) {
	const idDeclParticle *particleDecl = (idDeclParticle*)declManager->FindType(DECL_PARTICLE, kvModel.GetValue());
	const auto &prtStages = particleDecl->stages;

	// look for particle stages with "collisionStatic"
	for (int g = 0; g < prtStages.Num(); g++) {
		prtStage = prtStages[g];
		prtName = particleDecl->GetName();

		if (prtStage->collisionStatic) {
			signature.mainName = modelName;
			signature.modelSuffix = kvModel.GetKey().c_str() + 5;
			signature.surfaceIndex = 0;
			signature.particleStageIndex = g;
			idStr imageName = idParticleStage::GetCollisionStaticImagePath(signature);

			numSurfsProcessed++;
			if (!disabled)
				ProcessPointEmitter(origin, axis, diversity, imageName.c_str());
			else {
				numSurfsDisabled++;
				byte white[4] = {255, 255, 255, 0};
				R_WriteTGA(imageName, white, 1, 1);
			}
		}

		prtStage = nullptr;
		prtName = nullptr;
	}
}

static idDict GetSpawnArgsOfMapEntity(idMapEntity *ent) {
	const char *classname = ent->epairs.GetString("classname", "");
	auto def = (const idDeclEntityDef*)declManager->FindType(DECL_ENTITYDEF, classname, false);
	if (!def)
		return ent->epairs;
	idDict args = def->dict;
	args.Copy(ent->epairs);
	return args;
}

void PrtCollision::ProcessMap() {
	// create render world
	RenderWorld();

	common->Printf("Processing collisionStatic particle systems...\n");
	// find all particle-emitting surfaces with "collisionStatic" particle stages
	// only brushes/patches are looked as candidates, models are ignored completely
	int numEnts = mapFile->GetNumEntities();
	for (int e = 0; e < numEnts; e++) {
		idMapEntity *ent = mapFile->GetEntity(e);
		const char *name = ent->epairs.GetString("name", NULL);
		idDict spawnArgs = GetSpawnArgsOfMapEntity(ent);
		bool isWorldspawn = strcmp(spawnArgs.GetString("classname"), "worldspawn") == 0;

		if (isWorldspawn) {
			int areasNum = renderWorld->NumAreas();
			// check worldspawn geometry of every portal-area
			for (int a = 0; a < areasNum; a++) {
				idStr modelName;
				sprintf(modelName, "_area%d", a);
				ProcessParticleDeform(modelName.c_str(), idVec3(), mat3_identity, false, 0.0f);
			}
		}
		else {
			if (!name)
				continue;	// not an entity?...
			int isEmitter = spawnArgs.GetInt("particle_collision_static_emitter", "-1");
			idVec3 origin = spawnArgs.GetVector("origin");
			idMat3 axis;
			gameEdit->ParseSpawnArgsToAxis( &spawnArgs, axis );
			float diversity = spawnArgs.GetFloat("shaderParm5");
			//TODO: search attachments recursively too
			for (const idKeyValue *kv = spawnArgs.MatchPrefix("model"); kv; kv = spawnArgs.MatchPrefix("model", kv))
				if (idStr::CheckExtension(kv->GetValue(), ".prt"))
					ProcessParticleModel(name, *kv, origin, axis, isEmitter == 0, diversity);
			if (ent->GetNumPrimitives() > 0) {
				// use model from .proc file, i.e. surfaces compiled from brushes/patches
				// idRenderWorld::InitFromMap has already loaded them with entity name = model name
				ProcessParticleDeform(name, origin, axis, isEmitter == 0, diversity);
			}
		}
	}
}

idRenderWorld *PrtCollision::RenderWorld() {
	if (renderWorld)
		return renderWorld;

	common->Printf("Loading .proc file...\n");
	renderWorld = renderSystem->AllocRenderWorld();
	// add static world models (aka "_areaN")
	renderWorld->InitFromMap(mapFileName);

	common->Printf("Loading map entities...\n");
	int numEnts = mapFile->GetNumEntities();
	entityBlockFilterOverride.SetNum(numEnts);
	memset(entityBlockFilterOverride.Ptr(), 0, entityBlockFilterOverride.MemoryUsed());
	for (int e = 0; e < numEnts; e++) {
		idMapEntity *ent = mapFile->GetEntity(e);
		const char *name = ent->epairs.GetString("name", NULL);
		idDict spawnArgs = GetSpawnArgsOfMapEntity(ent);
		const char *classname = spawnArgs.GetString("classname", "");
		if (!name)
			continue;	//includes worldspawn

		//check for special spawnarg which can override our decision
		int isBlocker = spawnArgs.GetInt("particle_collision_static_blocker", "-1");
		if (isBlocker >= 0) {
			if (isBlocker == 2) {
				//value 2: override internal material/model checks
				entityBlockFilterOverride[e] = true;
			}
			//value 0 or 1: override decision
			isBlocker = (isBlocker != 0);
		}
		else {
			//by default, decide if this is a blocker ourselves
			isBlocker = true;
			if (spawnArgs.FindKey("frobable")) {
				//frobbing entity often makes it disappear
				//examples: moveable_base, atdm:frobable_base
				isBlocker = false;
			}
			if (spawnArgs.FindKey("movetype")) {
				//how entity is moved (e.g. animate)
				//examples: atdm:ai_base
				isBlocker = false;
			}
			if (spawnArgs.FindKey("bind")) {
				//the master which controls movement of this object
				isBlocker = false;
			}
			if (spawnArgs.FindKey("speed")) {
				//how fast object moves
				//examples: func_rotating, func_mover
				isBlocker = false;
			}
		}
		if (!isBlocker)
			continue;

		renderEntity_t rent;
		memset(&rent, 0, sizeof(rent));
		gameEdit->ParseSpawnArgsToRenderEntity(&spawnArgs, &rent);
		if (rent.hModel) {
			//BEWARE: entityNum means index in .map file here, no game is running!
			rent.entityNum = e;
			renderWorld->AddEntityDef(&rent);
		}
	}

	return renderWorld;
}

void PrtCollision::Run(const char *mapFileName) {
	this->mapFileName = mapFileName;
	timeStarted = sys->GetClockTicks();

	common->Printf("Loading mapfile %s...\n", mapFileName);
	mapFile = new idMapFile;
	if (!mapFile->Parse(mapFileName))
		common->Error( "Couldn't load map file: '%s'", mapFileName );

	const char *prtGenDir = idParticleStage::GetCollisionStaticDirectory();
	common->Printf("Cleaning %s...\n", prtGenDir);
	//clear _prt_gen directory
	idFileList *allPrtGenFiles = fileSystem->ListFiles(prtGenDir, "", false, true);
	for (int i = 0; i < allPrtGenFiles->GetNumFiles(); i++) {
		idStr fn = allPrtGenFiles->GetFile(i);
		fileSystem->RemoveFile(fn);
	}
	fileSystem->FreeFileList(allPrtGenFiles);

	ProcessMap();

	timeEnded = sys->GetClockTicks();
	timeElapsed = (timeEnded - timeStarted) / sys->ClockTicksPerSecond();
	common->Printf("Finished in %0.3lf seconds (%d emitters, %d disabled, %lld rays)\n", timeElapsed, numSurfsProcessed, numSurfsDisabled, numRaysCasted);
	common->Printf("\n");
}



//==============================================================================
//==============================================================================
//==============================================================================


void RunParticle_f(const idCmdArgs &args) {
	if (args.Argc() < 2) {
		common->Printf(
			"Usage: runParticle mapfile\n"
		);
		return;
	}

	idStr mapfn = args.Argv(1);
	FindMapFile(mapfn);


	common->ClearWarnings( "running runparticle" );

	// refresh the screen each time we print so it doesn't look
	// like it is hung
	common->SetRefreshOnPrint( true );
	com_editors |= EDITOR_RUNPARTICLE;
	{
		PrtCollision processor;
		processor.Run(mapfn);
	}
	com_editors &= ~EDITOR_RUNPARTICLE;
	common->SetRefreshOnPrint( false );

	common->PrintWarnings();
}
