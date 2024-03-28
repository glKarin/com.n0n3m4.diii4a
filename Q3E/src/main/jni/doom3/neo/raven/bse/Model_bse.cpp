// copy from Model_prt.cpp
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../renderer/tr_local.h"
#include "../../renderer/Model_local.h"
#include "BSE.h"

#ifdef _RAVEN_FX
extern rvBSEParticle * BSE_GetDeclParticle(const char *name);

static const char *parametricParticle_SnapshotName = "_ParametricParticle_Snapshot_";

/*
====================
rvRenderModelBSE::rvRenderModelBSE
====================
*/
rvRenderModelBSE::rvRenderModelBSE()
{
	particleSystem = NULL;
}

/*
====================
rvRenderModelBSE::InitFromFile
====================
*/
void rvRenderModelBSE::InitFromFile(const char *fileName)
{
	name = fileName;
	particleSystem = BSE_GetDeclParticle(name);
}

/*
=================
rvRenderModelBSE::TouchData
=================
*/
void rvRenderModelBSE::TouchData(void)
{
	// Ensure our particle system is added to the list of referenced decls
	particleSystem = BSE_GetDeclParticle(name);
}

/*
====================
rvRenderModelBSE::InstantiateDynamicModel
====================
*/
idRenderModel *rvRenderModelBSE::InstantiateDynamicModel(const struct renderEntity_s *renderEntity, const struct viewDef_s *viewDef, idRenderModel *cachedModel)
{
	idRenderModelStatic	*staticModel;

	if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
		delete cachedModel;
		cachedModel = NULL;
	}

	// this may be triggered by a model trace or other non-view related source, to which we should look like an empty model
	if (renderEntity == NULL || viewDef == NULL) {
		delete cachedModel;
		return NULL;
	}

	if (r_skipParticles.GetBool()) {
		delete cachedModel;
		return NULL;
	}

	/*
	// if the entire system has faded out
	if ( renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME] && viewDef->renderView.time * 0.001f >= renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME] ) {
		delete cachedModel;
		return NULL;
	}
	*/

	if (cachedModel != NULL) {

		assert(dynamic_cast<idRenderModelStatic *>(cachedModel) != NULL);
		assert(idStr::Icmp(cachedModel->Name(), parametricParticle_SnapshotName) == 0);

		staticModel = static_cast<idRenderModelStatic *>(cachedModel);

	} else {

		staticModel = new idRenderModelStatic;
		staticModel->InitEmpty(parametricParticle_SnapshotName);
	}

	rvBSE_particleGen_t g;

	g.renderEnt = renderEntity;
	g.renderView = &viewDef->renderView;
	g.origin.Zero();
	g.axis.Identity();

	for (int stageNum = 0; stageNum < particleSystem->stages.Num(); stageNum++) {
		rvBSEParticleStage *stage = particleSystem->stages[stageNum];

		if (!stage->material) {
			continue;
		}

		if (!stage->cycleMsec) {
			continue;
		}

		if (stage->hidden) {		// just for gui particle editor use
			staticModel->DeleteSurfaceWithId(stageNum);
			continue;
		}

		//LOGI(")))) %p %d %d %d %d", particleSystem, stageNum, stage->totalParticles, stage->NumQuadsPerParticle(), stage->rvptype);
		if(stage->rvptype == PTYPE_MODEL)
		{
			if(!stage->model)
				continue;
			if(!stage->model->NumSurfaces())
				continue;
			if(!stage->model->Surface(0))
				continue;
		}
		idRandom steppingRandom, steppingRandom2;

		int stageAge = g.renderView->time + renderEntity->shaderParms[SHADERPARM_TIMEOFFSET] * 1000 - stage->timeOffset * 1000;
		int	stageCycle = stageAge / stage->cycleMsec;
		int	inCycleTime = stageAge - stageCycle * stage->cycleMsec;

		// some particles will be in this cycle, some will be in the previous cycle
		steppingRandom.SetSeed(((stageCycle << 10) & idRandom::MAX_RAND) ^(int)(renderEntity->shaderParms[SHADERPARM_DIVERSITY] * idRandom::MAX_RAND));
		steppingRandom2.SetSeed((((stageCycle-1) << 10) & idRandom::MAX_RAND) ^(int)(renderEntity->shaderParms[SHADERPARM_DIVERSITY] * idRandom::MAX_RAND));

		int	count = stage->totalParticles * stage->NumQuadsPerParticle();

		int surfaceNum;
		modelSurface_t *surf;

		if (staticModel->FindSurfaceWithId(stageNum, surfaceNum)) {
			surf = &staticModel->surfaces[surfaceNum];
			R_FreeStaticTriSurfVertexCaches(surf->geometry);
		} else {
			surf = &staticModel->surfaces.Alloc();
			surf->id = stageNum;
			surf->shader = stage->material;
			surf->geometry = R_AllocStaticTriSurf();
			if(stage->rvptype == PTYPE_MODEL)
			{
				const modelSurface_t *msurf = stage->model->Surface(0);
				R_AllocStaticTriSurfVerts(surf->geometry, msurf->geometry->numVerts * count);
				R_AllocStaticTriSurfIndexes(surf->geometry, msurf->geometry->numIndexes * count);
			}
			else
			{
				R_AllocStaticTriSurfVerts(surf->geometry, 4 * count);
				R_AllocStaticTriSurfIndexes(surf->geometry, 6 * count);
			}
		}

		int numVerts = 0;
		idDrawVert *verts = surf->geometry->verts;

		for (int index = 0; index < stage->totalParticles; index++) {
			g.index = index;

			// bump the random
			steppingRandom.RandomInt();
			steppingRandom2.RandomInt();

			// calculate local age for this index
			int	bunchOffset = stage->particleLife * 1000 * stage->spawnBunching * index / stage->totalParticles;

			int particleAge = stageAge - bunchOffset;
			int	particleCycle = particleAge / stage->cycleMsec;

			if (particleCycle < 0) {
				// before the particleSystem spawned
				continue;
			}

			if (stage->cycles && particleCycle >= stage->cycles) {
				// cycled systems will only run cycle times
				continue;
			}

			if (particleCycle == stageCycle) {
				g.random = steppingRandom;
			} else {
				g.random = steppingRandom2;
			}

			int	inCycleTime = particleAge - particleCycle * stage->cycleMsec;

			if (renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME] &&
			    g.renderView->time - inCycleTime >= renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME]*1000) {
				// don't fire any more particles
				continue;
			}

			// supress particles before or after the age clamp
			g.frac = (float)inCycleTime / (stage->particleLife * 1000);

			if (g.frac < 0.0f) {
				// yet to be spawned
				continue;
			}

			if (g.frac > 1.0f) {
				// this particle is in the deadTime band
				continue;
			}

			// this is needed so aimed particles can calculate origins at different times
			g.originalRandom = g.random;

			g.age = g.frac * stage->particleLife;

			// if the particle doesn't get drawn because it is faded out or beyond a kill region, don't increment the verts
			numVerts += stage->CreateParticle(&g, verts + numVerts);
		}

		// numVerts must be a multiple of 4
		assert((numVerts & 3) == 0 && numVerts <= 4 * count);

		// build the indexes
		int	numIndexes = 0;
		if(stage->rvptype == PTYPE_MODEL)
		{
			const modelSurface_t *msurf = stage->model->Surface(0);

			glIndex_t *indexes = surf->geometry->indexes;
			int c = numVerts / msurf->geometry->numVerts;
			numIndexes = msurf->geometry->numIndexes * c;

			for (int i = 0; i < c; i++) {
				int start = i * msurf->geometry->numIndexes;
				int vertsStart = i * msurf->geometry->numVerts;
				for (int j = 0; j < msurf->geometry->numIndexes; j++) {
					indexes[start + j] = msurf->geometry->indexes[j];
				}
			}
		}
		else
		{
			glIndex_t *indexes = surf->geometry->indexes;

			for (int i = 0; i < numVerts; i += 4) {
				indexes[numIndexes+0] = i;
				indexes[numIndexes+1] = i+2;
				indexes[numIndexes+2] = i+3;
				indexes[numIndexes+3] = i;
				indexes[numIndexes+4] = i+3;
				indexes[numIndexes+5] = i+1;
				numIndexes += 6;
			}
		}
		surf->geometry->numVerts = numVerts;
		surf->geometry->numIndexes = numIndexes;
		surf->geometry->bounds = stage->bounds;		// just always draw the particles

    	R_BoundTriSurf(surf->geometry);
	}

	return staticModel;
}

/*
====================
rvRenderModelBSE::IsDynamicModel
====================
*/
dynamicModel_t rvRenderModelBSE::IsDynamicModel() const
{
	return DM_CONTINUOUS;
}

/*
====================
rvRenderModelBSE::Bounds
====================
*/
idBounds rvRenderModelBSE::Bounds(const struct renderEntity_s *ent) const
{
	return particleSystem->bounds;
}

/*
====================
rvRenderModelBSE::DepthHack
====================
*/
float rvRenderModelBSE::DepthHack() const
{
	return particleSystem->depthHack;
}

/*
====================
rvRenderModelBSE::Memory
====================
*/
int rvRenderModelBSE::Memory() const
{
	int total = 0;

	total += idRenderModelStatic::Memory();

	if (particleSystem) {
		total += sizeof(*particleSystem);

		for (int i = 0; i < particleSystem->stages.Num(); i++) {
			total += sizeof(particleSystem->stages[i]);
		}
	}

	return total;
}

#else

/*
=================
idRenderModelStatic::InitFromFile
=================
*/
void rvRenderModelBSE::InitFromFile(const char* fileName) {
	name = fileName;
}

/*
=================
idRenderModelStatic::FinishSurfaces
=================
*/
void rvRenderModelBSE::FinishSurfaces(bool useMikktspace) {
	int i; // ebp
	int surfId; // ebx
	srfTriangles_t* tri; // eax

	this->bounds[1].z = 0.0;
	this->bounds[1].y = 0.0;
	this->bounds[1].x = 0.0;
	i = 0;
	this->bounds[0].z = 0.0;
	this->bounds[0].y = 0.0;
	this->bounds[0].x = 0.0;
	if (this->surfaces.Num() > 0)
	{
		surfId = 0;
		do
		{
			tri = this->surfaces[surfId].geometry;
			if (tri)
			{
				this->bounds[0].x = fminf(this->bounds[0].x, tri->bounds[0].x);
				this->bounds[0].y = fminf(this->bounds[0].y, tri->bounds[0].y);
				this->bounds[0].z = fminf(this->bounds[0].z, tri->bounds[0].z);
				this->bounds[1].x = fmaxf(this->bounds[1].x, tri->bounds[1].x);
				this->bounds[1].y = fmaxf(this->bounds[1].y, tri->bounds[1].y);
				this->bounds[1].z = fmaxf(this->bounds[1].z, tri->bounds[1].z);
			}
			++i;
			++surfId;
		} while (i < this->surfaces.Num());
	}
}
#endif
