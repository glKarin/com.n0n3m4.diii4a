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

#define PIN(type) const type &
#define POUT(type) type &
#define PINOUT(type) type &

//particle-specific
#define EMITTER idDrawVert* &

#include "renderer/resources/ParticleSystem_decl.h"

#undef PIN
#undef POUT
#undef PINOUT

#undef EMITTER

//===========================================================================

typedef struct srfTriangles_s srfTriangles_t;
typedef struct drawSurf_s drawSurf_t;
typedef struct renderEntity_s renderEntity_t;
class idParticleStage;
class idImageAsset;

//---------------------------------------------------------------------------

//compute bounding box of particles in "standard" coordinate system --- transformation and world gravity not applied yet
//it is computed by sampling many particles with idParticle_ParticleOriginStdSys function
idBounds idParticle_EstimateBoundsStdSys(const idPartStageData &stg);

//compute bounding box of particles for smoke or particle model, with all transformations applied
//"entityAxis" must be set to renderEntity_t::axis: world-space effects like gravity depend on this
//"stdBounds" must be set to returned value of idParticle_EstimateBoundsStdSys (usually precomputed on load)
idBounds idParticle_GetStageBoundsModel(const idPartStageData &stg, const idBounds &stdBounds, const idMat3 &entityAxis);

//analyze surface (triangular mesh) which can be used as emitter with particle deform
//produces bounds for X axis, Y axis, Z axis, and origin of emitted particles (from idParticleData::axis and idParticleData::origin)
//TODO: put csysBounds into some sort of temporary structure?
void idParticle_AnalyzeSurfaceEmitter(const srfTriangles_t *tri, idBounds csysBounds[4]);

//compute bounding box of particles emitted from surface via "particle deform"
//"csysBounds" must contain information about surface, computed by idParticle_AnalyzeSurfaceEmitter
//"stdBounds" must be set to returned value of idParticle_EstimateBoundsStdSys (usually precomputed on load)
idBounds idParticle_GetStageBoundsDeform(const idPartStageData &stg, const idBounds &stdBounds, const idBounds csysBounds[4]);

//---------------------------------------------------------------------------

//prepares auxilliary data for sampling a random triangle out of the surface, area-wise uniformly (particle deform)
//it computes array of prefix sums of triangle areas, used later to binary search a random value
//to learn the size of "areas" buffer, call the function with areas = NULL first
int idParticle_PrepareDistributionOnSurface(const srfTriangles_t *tri, float *areas = NULL, float *totalArea = NULL);

//see how many particles the surface emitter generates
//set "totalArea" to -1 if every triangle should behave as separate emitter with same number of particles (particle2 deform)
//the value "particleCountPerCycle" must be put into idPartSysEmit::totalParticles
int idParticle_GetParticleCountOnSurface(const idPartStageData &stg, const srfTriangles_t *tri, float totalArea, int &particleCountPerCycle);

//computes emit location of particle on surface for particle deform systems
//"part" is usually a result of idParticle_EmitParticle function, and must contain: "index" and "randomSeed"
//the function fills "part.origin", "part.axis", and "texCoord", and also changes randomSeed as side effect
//"areas" must be NULL for "particle2 deform", and result of idParticle_PrepareDistributionOnSurface for "particle deform"
void idParticle_EmitLocationOnSurface(
	const idPartStageData &stg, const srfTriangles_t *tri,
	idParticleData &part, idVec2 &texCoord,
	float *areas = NULL
);

//identifies particle system (particle deform and particle model)
//used to:
// 1) find generated cutoff map (collisionStatic)
// 2) randomize rand seed of emitters
struct idPartSysEmitterSignature {
	//for surface emitter: name of renderModel (e.g. "_area9" or "func_static_173")
	//for model emitter: entity name as in .map file (e.g. "atdm_moveable_candle1_4")
	idStr mainName;
	//model emitter only: suffix in model spawnarg (e.g. "_lit" or "_extinguished")
	idStr modelSuffix;
	//model emitter only: def_attach path (e.g. "_flame2" or "_candle_flame")
	//TODO: idStr attachSuffix;
	//surface emitter only: index of surface in model
	int surfaceIndex = 0;
	//index of particle stage in particle system
	int particleStageIndex = 0;

};
//computes "randomizer" for particle deform particle system
//it affects random seed of each particle, so that same particledecl-s applied to nearby surfaces look different
float idParticle_ComputeRandomizer(const idPartSysEmitterSignature &sign, float diversity);

//---------------------------------------------------------------------------

//finds correspondence between physical image and virtual texture in cutoff map
//the region is trivial if cutoffTimeMap is set explicitly, but thorough analysis of the surface is needed in collisionStatic case
//returns false if there is no cutoffTimeMap or if the sought-for region is empty
//note: this is exposed for collisionStatic preprocessing, normally you can call idParticle_PrepareCutoffTexture instead
bool idParticle_FindCutoffTextureSubregion(const idPartStageData &stg, const srfTriangles_t *tri, idPartSysCutoffTextureInfo &region);

//finds image and auxilliary data for cutoffTimeMap feature
//"image" is set to valid texture if cutoff should be done, and to NULL if not
//"texinfo" is generated using idParticle_FindCutoffTextureSubregion (makes sense only for mapLayout "texture")
//"totalParticles" must be the number of particles the whole system has, i.e. return value of idParticle_GetParticleCountOnSurface
void idParticle_PrepareCutoffMap(
	const idParticleStage *stage, const srfTriangles_t *tri, const idPartSysEmitterSignature &signature, int totalParticles,
	idImageAsset *&image, idPartSysCutoffTextureInfo *texinfo
);

//fetches cutoffTime from the image (with "mapLayout texture") using texcoords of emit location
float idParticle_FetchCutoffTimeTexture(const idImageAsset *image, const idPartSysCutoffTextureInfo &texinfo, idVec2 texcoord);
//fetches cutoffTime from the image (with "mapLayout linear") using index of particle and its current cycle
float idParticle_FetchCutoffTimeLinear(const idImageAsset *image, int totalParticles, int index, int cycIdx);
