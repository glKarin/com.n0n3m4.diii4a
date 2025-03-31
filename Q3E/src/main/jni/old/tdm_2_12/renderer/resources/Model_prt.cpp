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



#include <algorithm>
#include "renderer/tr_local.h"
#include "renderer/resources/Model_local.h"

static const char *parametricParticle_SnapshotName = "_ParametricParticle_Snapshot_";

/*
====================
idRenderModelPrt::idRenderModelPrt
====================
*/
idRenderModelPrt::idRenderModelPrt() {
	particleSystem = NULL;
}

/*
====================
idRenderModelPrt::InitFromFile
====================
*/
void idRenderModelPrt::InitFromFile( const char *fileName ) {
	name = fileName;
	particleSystem = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, fileName ) );
	SetSofteningRadii();  // # 3878
}

/*
=================
idRenderModelPrt::TouchData
=================
*/
void idRenderModelPrt::TouchData( void ) {
	// Ensure our particle system is added to the list of referenced decls
	particleSystem = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, name ) );
}

/*
====================
idRenderModelPrt::InstantiateDynamicModel
====================
*/
idRenderModel *idRenderModelPrt::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const struct viewDef_s *viewDef, idRenderModel *cachedModel ) {
	idRenderModelStatic	*staticModel;

	if ( cachedModel && !r_useCachedDynamicModels.GetBool() ) {
		delete cachedModel;
		cachedModel = NULL;
	}

	// this may be triggered by a model trace or other non-view related source, to which we should look like an empty model
	if ( !renderEntity || !viewDef ) {
		delete cachedModel;
		return NULL;
	}
	else if ( r_skipParticles.GetBool() ) {
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

	else if ( cachedModel ) {
		assert( dynamic_cast<idRenderModelStatic *>(cachedModel) != NULL );
		assert( idStr::Icmp( cachedModel->Name(), parametricParticle_SnapshotName ) == 0 );

		staticModel = static_cast<idRenderModelStatic *>(cachedModel);
	}
	else {
		staticModel = new idRenderModelStatic;
		staticModel->InitEmpty( parametricParticle_SnapshotName );
	}

	//stgatilov #5437: this is filled lazingly to avoid slowdown for non-colliding particle systems
	idEntity *owner = nullptr;
	idPartSysEmitterSignature sign;

	idPartSysData psys;
	const renderView_t *renderView = &viewDef->renderView;
	psys.entityAxis = renderEntity->axis;
	memcpy(&psys.entityParmsColor, renderEntity->shaderParms, sizeof(psys.entityParmsColor));
	psys.viewAxis = renderView->viewaxis;

	for ( int stageNum = 0; stageNum < particleSystem->stages.Num(); stageNum++ ) {
		idParticleStage *stage = particleSystem->stages[stageNum];

		if ( !stage->material )
			continue;
		if ( !stage->cycleMsec )
			continue;
		else if ( stage->hidden ) {		// just for gui particle editor use
			staticModel->DeleteSurfaceWithId( stageNum );
			continue;
		}

		psys.totalParticles = stage->totalParticles;

		sign.particleStageIndex = stageNum;
		if (stage->collisionStatic && !owner) {
			//stgatilov #5437: this is some very stupid and slow code to find signature
			//normally, every particle system should be registered in a manager class (like renderEntities in renderWorld)
			//then the signature can be stored alongside them, instead of computing it every frame =(
			owner = gameLocal.entities[renderEntity->entityNum];
			if (stage->mapLayoutType != PML_LINEAR)
				common->Error("Particle model on entity %s uses collisionStatic without mapLayout linear", (owner ? owner->name.c_str() : "[unknown]"));
			if (owner && owner->fromMapFile) {
				sign.mainName = owner->name;
				int k = 0;
				for (const idKeyValue *kv = owner->spawnArgs.MatchPrefix("model"); kv; kv = owner->spawnArgs.MatchPrefix("model", kv)) {
					idStr name = kv->GetValue();
					name.StripFileExtension();
					if (name.Icmp(particleSystem->GetName()) == 0) {
						sign.modelSuffix = kv->GetKey().c_str() + 5;
						k++;
					}
				}
				if (k > 1)
					common->Error("Particle model on entity %s: two models with same .prt", owner->name.c_str());
				if (k == 0)
					owner = nullptr;
			}
		}
		idImageAsset *cutoffImage = nullptr;
		if (stage->collisionStatic && owner)
			idParticle_PrepareCutoffMap(stage, nullptr, sign, psys.totalParticles, cutoffImage, nullptr);

		idPartSysEmit psEmit;
		psEmit.entityParmsStopTime = renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME];
		psEmit.entityParmsTimeOffset = renderEntity->shaderParms[SHADERPARM_TIMEOFFSET];
		psEmit.randomizer = idParticle_ComputeRandomizer(sign, renderEntity->shaderParms[SHADERPARM_DIVERSITY]);
		psEmit.totalParticles = stage->totalParticles;
		psEmit.viewTimeMs = renderView->time;

		const int count = stage->totalParticles * stage->NumQuadsPerParticle();
		int surfaceNum = 0;
		modelSurface_t *surf;
		if ( staticModel->FindSurfaceWithId( stageNum, surfaceNum ) ) {
			surf = &staticModel->surfaces[surfaceNum];
			R_FreeStaticTriSurfVertexCaches( surf->geometry );
		} else {
			surf = &staticModel->surfaces.Alloc();
			surf->id = stageNum;
			surf->material = stage->material;
			surf->geometry = R_AllocStaticTriSurf();
			R_AllocStaticTriSurfVerts( surf->geometry, 4 * count );
			R_AllocStaticTriSurfIndexes( surf->geometry, 6 * count );
			R_AllocStaticTriSurfPlanes( surf->geometry, 6 * count );
		}

		int numVerts = 0;
		idDrawVert *verts = surf->geometry->verts;

		for ( int index = 0; index < stage->totalParticles; index++ ) {
			idParticleData part;
			int cycIdx;
			if (!idParticle_EmitParticle(*stage, psEmit, index, part, cycIdx))
				continue;

			if (cutoffImage) {
				float cutoff = idParticle_FetchCutoffTimeLinear(cutoffImage, psys.totalParticles, index, cycIdx);
				if (part.frac > cutoff)
					continue;
			}

			// if the particle doesn't get drawn because it is faded out or beyond a kill region, don't increment the verts
			idDrawVert *ptr = verts + numVerts;
			idParticle_CreateParticle(*stage, psys, part, ptr);
			numVerts = ptr - verts;
		}

		// numVerts must be a multiple of 4
		assert( ( numVerts & 3 ) == 0 && numVerts <= 4 * count );

		// build the indexes
		int	numIndexes = 0;
		glIndex_t *indexes = surf->geometry->indexes;
		for ( int i = 0; i < numVerts; i += 4 ) {
			indexes[numIndexes+0] = i;
			indexes[numIndexes+1] = i+2;
			indexes[numIndexes+2] = i+3;
			indexes[numIndexes+3] = i;
			indexes[numIndexes+4] = i+3;
			indexes[numIndexes+5] = i+1;
			numIndexes += 6;
		}

		surf->geometry->tangentsCalculated = false;
		surf->geometry->facePlanesCalculated = false;
		surf->geometry->numVerts = numVerts;
		surf->geometry->numIndexes = numIndexes;
		surf->geometry->bounds = particleSystem->GetStageBounds(renderEntity, stage);
	}

	return staticModel;
}

/*
====================
idRenderModelPrt::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelPrt::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

/*
====================
idRenderModelPrt::Bounds
====================
*/
idBounds idRenderModelPrt::Bounds( const struct renderEntity_s *ent ) const {
	return particleSystem->GetFullBounds(ent);
}

/*
====================
idRenderModelPrt::DepthHack
====================
*/
float idRenderModelPrt::DepthHack() const {
	return particleSystem->depthHack;
}

/*
====================
idRenderModelPrt::Memory
====================
*/
int idRenderModelPrt::Memory() const {
	int total = idRenderModelStatic::Memory();

	if ( particleSystem ) {
		total += sizeof( *particleSystem );

		for ( int i = 0; i < particleSystem->stages.Num(); i++ ) {
			total += sizeof( particleSystem->stages[i] );
		}
	}

	return total;
}

/*
====================
idRenderModelPrt::SetSofteningRadii

Calculate "depth" of each particle stage that represents a 3d volume, so the particle can
be allowed to overdraw solid geometry by the right amount, and the particle "thickness" (visibility) 
can be adjusted by how much of it is visible in front of the background surface.

"depth" is by default 0.8 of the particle radius, and particles less than 2 units in size won't be softened. 
The particles that represent 3d volumes are the view-aligned ones. Others have depth set to 0.

Cache these values rather than calculate them for each stage every frame.
Added for soft particles -- SteveL #3878.
====================
*/
void idRenderModelPrt::SetSofteningRadii()
{
	softeningRadii.AssureSize( particleSystem->stages.Num() );
	
	for ( int i = 0; i < particleSystem->stages.Num(); ++i )
	{
		const idParticleStage* ps = particleSystem->stages[i];
		if ( ps->softeningRadius > -2.0f )	// User has specified a setting
		{
			softeningRadii[i] = ps->softeningRadius;
		}
		else if ( ps->orientation == POR_VIEW ) // Only view-aligned particle stages qualify for softening
		{
			float diameter = ( std::max )( ps->size.from, ps->size.to );
			float scale = ( std::max )( ps->aspect.from, ps->aspect.to );
			diameter *= ( std::max )( scale, 1.0f ); // aspect applies to 1 axis only. If it's < 1, the other axis will still be at scale 1
			if ( diameter > 2.0f )	// Particle is big enough to soften
			{
				softeningRadii[i] = diameter * 0.8f / 2.0f;
			}
			else // Particle is small. Disable both softening and modelDepthHack
			{
				softeningRadii[i] = 0.0f;
			}
		}
		else // Particle isn't view-aligned, and no user setting. Don't change anything.
		{
			softeningRadii[i] = -1;
		}
	}
}

/*
====================
idRenderModelPrt::SofteningRadius

Return the max radius of the individual quads that make up this stage.
Added for soft particles #3878.
====================
*/
float idRenderModelPrt::SofteningRadius( const int stage ) const {
	assert( particleSystem );
	assert( stage > -1 && stage < softeningRadii.Num() );
	return softeningRadii[stage];
}

