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
#include "renderer/resources/ParticleSystem.h"
#include "renderer/tr_local.h"


#define PIN(type) const type &
#define POUT(type) type &
#define PINOUT(type) type &

#define SinCos16 idMath::SinCos16
#define PI idMath::PI
#define clamp(v, l, r) idMath::ClampFloat(l, r, v)
#define length(v) (v).Length()
#define normalize(v) ((v).Normalized())
#define cross(a, b) (a).Cross(b)
#define transpose(m) (m).Transpose()

//particle-specific
#define EMITTER idDrawVert* &
struct idParticleDrawVert;
static void idParticle_EmitQuad(
	PIN(idParticleDrawVert) v0, PIN(idParticleDrawVert) v1, PIN(idParticleDrawVert) v2, PIN(idParticleDrawVert) v3,
	EMITTER emitter
);

#include "renderer/resources/ParticleSystem_def.h"

static void idParticle_EmitQuad(
	PIN(idParticleDrawVert) v0, PIN(idParticleDrawVert) v1, PIN(idParticleDrawVert) v2, PIN(idParticleDrawVert) v3,
	EMITTER emitter
) {
	#define EMIT_VERTEX(v) \
		emitter->xyz = v.xyz; \
		emitter->st = v.st; \
		emitter->color[0] = idMath::FtoiTrunc(v.color[0] * 255.0f); \
		emitter->color[1] = idMath::FtoiTrunc(v.color[1] * 255.0f); \
		emitter->color[2] = idMath::FtoiTrunc(v.color[2] * 255.0f); \
		emitter->color[3] = idMath::FtoiTrunc(v.color[3] * 255.0f); \
		emitter++;
	EMIT_VERTEX(v0);
	EMIT_VERTEX(v1);
	EMIT_VERTEX(v2);
	EMIT_VERTEX(v3);
}

//===========================================================================

idBounds idParticle_EstimateBoundsStdSys(const idPartStageData &stg) {
	idBounds res;
	res.Clear();

	// this isn't absolutely guaranteed, but it should be close

	idParticleData part;
	part.origin.Zero();
	part.axis = mat3_identity;

	idRandom steppingRandom;
	steppingRandom.SetSeed( 0 );

	// just step through a lot of possible particles as a representative sampling
	for ( int i = 0 ; i < 1000 ; i++ ) {
		steppingRandom.RandomInt();	//step to next seed
		part.randomSeed = steppingRandom.GetSeed();

		int	maxMsec = stg.particleLife * 1000;

		// SteveL #4218: Speed up load time for long-lived particles.
		// Limit the sampling to 250 spread across the particle's lifetime.
		const int step_milliseconds = idMath::Imax(maxMsec / 250, 16); // 16 was the original value, meaning test every frame

		for ( int inCycleTime = 0 ; inCycleTime < maxMsec ; inCycleTime += step_milliseconds )
		{
			// make sure we get the very last tic, which may make up an extreme edge
			if ( inCycleTime + step_milliseconds > maxMsec ) {
				inCycleTime = maxMsec - 1;
			}
			part.frac = (float)inCycleTime / ( stg.particleLife * 1000 );

			int random = part.randomSeed;
			idVec3 origin = idParticle_ParticleOriginStdSys(stg, part, random);

			res.AddPoint(origin);
		}
	}

	// find the max size
	float maxSize = 0;
	for ( float f = 0; f <= 1.0f; f += 1.0f / 64 ) {
		float size = idParticleParm_Eval( stg.size, f );
		float aspect = idParticleParm_Eval( stg.aspect, f );
		float width = size;
		float height = size * aspect;
		// stgatilov #6099: the particles can be rotated arbitrarily (e.g. view-dependent)
		// so we take bounding sphere and use its radius for expansion
		size = idVec2(width, height).Length();
		if ( size > maxSize ) {
			maxSize = size;
		}
	}

	maxSize += 8;	// just for good measure
	// users can specify a per-stage bounds expansion to handle odd cases
	res.ExpandSelf( maxSize + stg.boundsExpansion );

	return res;
}

idBounds idParticle_GetStageBoundsModel(const idPartStageData &stg, const idBounds &stdBounds, const idMat3 &entityAxis) {
	//we have bounding box for idParticle_ParticleOriginStdSys positions
	idBounds bounds = stdBounds;
	//now we need to apply the remaining modifications from idParticle_ParticleOrigin
	//note: part.origin and part.axis are trivial (they are used only in particle deform)

	//apply transformation
	if ( stg.worldAxis ) { // SteveL #3950 -- allow particles to use world axis for their offset and travel direction
		bounds *= entityAxis.Transpose();
	}

	if (stg.gravity) {
		// add gravity after adjusting for axis
		idVec3 grav = idVec3( 0, 0, -stg.gravity );
		if ( stg.worldGravity ) {
			grav *= entityAxis.Transpose();
		}
		float age = stg.particleLife;
		idVec3 gravityShift = grav * age * age;
		bounds.AddBounds(bounds.Translate(gravityShift));
	}

	return bounds;
}

void idParticle_AnalyzeSurfaceEmitter(const srfTriangles_t *tri, idBounds csysBounds[4]) {
	//get bounds for every axis of particle's coordinate system
	//note that each particle gets some interpolation of normal/tangents and xyz as its coordinate system
	for (int l = 0; l < 4; l++)
		csysBounds[l].Clear();
	for (int v = 0; v < tri->numVerts; v++) {
		csysBounds[0].AddPoint(tri->verts[v].tangents[0]);
		csysBounds[1].AddPoint(tri->verts[v].tangents[1]);
		csysBounds[2].AddPoint(tri->verts[v].normal);
		csysBounds[3].AddPoint(tri->verts[v].xyz);
	}
}

idBounds idParticle_GetStageBoundsDeform(const idPartStageData &stg, const idBounds &stdBounds, const idBounds csysBounds[4]) {
	//compute bounds of particle movement
	float radius = stdBounds.GetRadius(vec3_zero);
	//these are pessimistic bounds, considering orientation of movement to be arbitrary
	idBounds bounds = bounds_zero.Expand(radius);

	if (!stg.worldAxis) {
		//movement is in particle space (not in world space)
		//so we can estimate bounds much better
		idBounds sum = bounds_zero;
		for (int l = 0; l < 3; l++) {
			//consider possible shift from l-th local coordinates
			float minCoeff = stdBounds[0][l];
			float maxCoeff = stdBounds[1][l];
			idBounds scaled;
			scaled.Clear();
			//use bounds on particle's axis
			scaled.AddPoint(minCoeff * csysBounds[l][0]);
			scaled.AddPoint(maxCoeff * csysBounds[l][0]);
			scaled.AddPoint(minCoeff * csysBounds[l][1]);
			scaled.AddPoint(maxCoeff * csysBounds[l][1]);
			//add the shift to total sum
			sum[0] += scaled[0];
			sum[1] += scaled[1];
		}
		//we got another bounds on movement, so take their intersection s final estimate
		bounds.IntersectSelf(sum);
	}
	//add bounds of particle origin to movement bounds
	bounds[0] += csysBounds[3][0];
	bounds[1] += csysBounds[3][1];

	float maxGravityMovement = stg.gravity * stg.particleLife * stg.particleLife;
	if (stg.worldGravity) {
		//cannot predict how entity would be oriented, so expand in all directions
		bounds.ExpandSelf(maxGravityMovement);
	}
	else {
		//apply shift from gravity along Z
		bounds.AddBounds(bounds.Translate(idVec3(0.0f, 0.0f, -maxGravityMovement)));
	}

	return bounds;
}

int idParticle_PrepareDistributionOnSurface(const srfTriangles_s *tri, float *areas, float *totalArea) {
	int triNum = tri->numIndexes / 3;
	if (areas) {
		float sumArea = 0.0f;
		for (int i = 0; i < triNum; i++) {
			int a = tri->indexes[3 * i + 0];
			int b = tri->indexes[3 * i + 1];
			int c = tri->indexes[3 * i + 2];
			float area = idWinding::TriangleArea( tri->verts[a].xyz, tri->verts[b].xyz,  tri->verts[c].xyz );
			areas[i] = sumArea;
			sumArea += area;
		}
		areas[triNum] = sumArea;
		if (totalArea)
			*totalArea = sumArea;
	}
	return triNum + 1;
}

int idParticle_GetParticleCountOnSurface(const idPartStageData &stg, const srfTriangles_t *tri, float totalArea, int &particleCountPerCycle) {
	if (totalArea < 0) {
		//every triangle should work as independent emitter with fixed number of particles
		int triCount = (tri->numIndexes / 3);
		particleCountPerCycle = stg.totalParticles;
		return particleCountPerCycle * triCount;
	}
	else {
		//we interpret stage->totalParticles as "particles per map square area"
		//so the systems look the same on different size surfaces
		particleCountPerCycle = stg.totalParticles * totalArea / 4096.0f;
		return particleCountPerCycle;
	}
}

void idParticle_EmitLocationOnSurface(const idPartStageData &stg, const srfTriangles_s *tri, idParticleData &part, idVec2 &texCoord, float *areas) {
	//---------------
	// locate the particle origin and axis somewhere on the surface
	//---------------

	int triIdx;
	if (areas) {
		int triNum = tri->numIndexes / 3;
		// select a triangle based on an even area distribution
		triIdx = idBinSearch_LessEqual<float>( areas, triNum, idRandom_RandomFloat(part.randomSeed) * areas[triNum] );
	}
	else {
		// each triangle gets same "particleCount" number of particles 
		triIdx = part.index / stg.totalParticles;
	}

	// now pick a random point inside pointTri
	const idDrawVert *v1 = &tri->verts[ tri->indexes[ triIdx * 3 + 0 ] ];
	const idDrawVert *v2 = &tri->verts[ tri->indexes[ triIdx * 3 + 1 ] ];
	const idDrawVert *v3 = &tri->verts[ tri->indexes[ triIdx * 3 + 2 ] ];

	// create random barycentric coordinates
	float f1 = idRandom_RandomFloat(part.randomSeed);
	float f2 = idRandom_RandomFloat(part.randomSeed);
	float f3 = idRandom_RandomFloat(part.randomSeed);
	float ft = 1.0f / ( f1 + f2 + f3 + 0.0001f );
	f1 *= ft;
	f2 *= ft;
	f3 = 1.0f - f1 - f2;

	// interpolate all attributes across triangle
	part.origin = v1->xyz * f1 + v2->xyz * f2 + v3->xyz * f3;
	part.axis[0] = v1->tangents[0] * f1 + v2->tangents[0] * f2 + v3->tangents[0] * f3;
	part.axis[1] = v1->tangents[1] * f1 + v2->tangents[1] * f2 + v3->tangents[1] * f3;
	part.axis[2] = v1->normal * f1 + v2->normal * f2 + v3->normal * f3;
	texCoord = v1->st * f1 + v2->st * f2 + v3->st * f3;
}

float idParticle_ComputeRandomizer(const idPartSysEmitterSignature &sign, float diversity) {
	uint64 hash = idStr::HashPoly64(sign.mainName) * 77777;
	hash ^= idStr::HashPoly64(sign.modelSuffix) * 12345;
	hash -= idStr::HashPoly64("", 0);	//TODO: attachSuffix
	hash += sign.surfaceIndex * 1920767767;
	hash ^= sign.particleStageIndex * 1367130551;
	int reduced = hash % 999983;
	//note: divider equals the multiplier in idParticle_EmitParticle (for best results)
	return reduced * (1.0f / 46341.0f) + diversity;
}


bool idParticle_FindCutoffTextureSubregion(const idPartStageData &stg, const srfTriangles_s *tri, idPartSysCutoffTextureInfo &region) {
	if (!stg.useCutoffTimeMap)
		return false;	//subregion not used

	int w = stg.mapLayoutSizes[0], h = stg.mapLayoutSizes[1];
	if (stg.collisionStatic) {
		//evaluate bounding box for texture coords
		idBounds texBounds;
		texBounds.Clear();
		for (int i = 0; i < tri->numIndexes; i++) {
			//VC 2022 optimizer breaks this code, so we use volatile as workaround
			//See bug report: https://developercommunity.visualstudio.com/t/Optimization-bug-with-scalar-SSE/10707188
			volatile idVec2 tc = tri->verts[tri->indexes[i]].st;
			texBounds.AddPoint(idVec3(tc.x, tc.y, 0.0));
		}
		texBounds.IntersectsBounds(bounds_zeroOneCube);
		bool empty = false;
		if (texBounds.IsBackwards()) {
			empty = true;
			texBounds.Zero();
		}

		//find minimal subrectangle to be computed and saved
		int xBeg = (int)idMath::Floor(texBounds[0].x * w);
		int xEnd = (int)idMath::Ceil (texBounds[1].x * w);
		int yBeg = (int)idMath::Floor(texBounds[0].y * h);
		int yEnd = (int)idMath::Ceil (texBounds[1].y * h);
		//ensure at least one texel in the region
		if (xEnd == xBeg)
			(xEnd < w ? xEnd++ : xBeg--);
		if (yEnd == yBeg)
			(yEnd < h ? yEnd++ : yBeg--);

		region.baseX = xBeg;
		region.baseY = yBeg;
		region.resX = w;
		region.resY = h;
		region.sizeX = xEnd - xBeg;
		region.sizeY = yEnd - yBeg;
		return !empty;
	}
	else {
		//cutoff map set explicitly: subregion is full
		region.baseX = 0;
		region.baseY = 0;
		region.resX = w;
		region.resY = h;
		region.sizeX = w;
		region.sizeY = h;
		return true;
	}
}

void idParticle_PrepareCutoffMap(
	const idParticleStage *stage, const srfTriangles_t *tri, const idPartSysEmitterSignature &sign, int totalParticles,
	idImageAsset *&image, idPartSysCutoffTextureInfo *texinfo
) {
	image = nullptr;
	if (!stage->useCutoffTimeMap)
		return;	//nothing to prepare

	if ( stage->collisionStatic ) {
		//collisionStatic: individual texture is used for every combination of surface index and particle stage index
		if ( sign.surfaceIndex >= 0 && sign.particleStageIndex >= 0 ) {
			idStr imagePath = idParticleStage::GetCollisionStaticImagePath( sign );
			if ( image = idParticleStage::LoadCutoffTimeMap( imagePath, true ) ) {
				if ( image->defaulted )
					image = nullptr;	//image not found
				else if ( !image->cpuData.pic ) {
					assert(false);
					image = nullptr;	//some SMP weirdness: do not crash at least
				}
				else {
					const imageBlock_t &data = image->cpuData;
					const byte *pic = data.GetPic(0);
					if ( data.GetSizeInBytes() == 4 && pic[0] == 255 && pic[1] == 255 && pic[2] == 255 )
						image = nullptr;	//collisionStatic was disabled for this emitter
				}
			}
		}
		else {
			//something failed really bad, ignore cutoffTimeMap
			assert(false);
		}
	}
	else {
		//cutoffTimeMap set explicitly
		image = stage->cutoffTimeMap;
	}

	if (image) {
		int w = image->cpuData.width, h = image->cpuData.height;
		if ( stage->mapLayoutType == PML_TEXTURE ) {
			assert(texinfo);
			//set up the subregion (especially important for collisionStatic)
			idParticle_FindCutoffTextureSubregion(*stage, tri, *texinfo);

			if ( w != texinfo->sizeX || h != texinfo->sizeY ) {
				//dimensions mismatch: drop the image
				image = nullptr;
			}
		}
		else if ( stage->mapLayoutType == PML_LINEAR ) {
			int casesCnt = stage->diversityPeriod * totalParticles;
			//number of texels can be slightly more than number of cases
			//but at most by one row
			if ( stage->diversityPeriod <= 0 || casesCnt > w * h ) {
				//total size mismatch: drop the image
				image = nullptr;
			}
		}
	}
}

float idParticle_FetchCutoffTimeTexture(const idImageAsset *image, const idPartSysCutoffTextureInfo &texinfo, idVec2 texcoord) {
	assert(image);
	//take the image
	const byte *pic = image->cpuData.GetPic(0);
	int w = image->cpuData.width, h = image->cpuData.height;

	//clamp texcoords (GL_CLAMP_TO_EDGE)
	texcoord.Clamp( idVec2(0.0f, 0.0f), idVec2(1.0f - FLT_EPSILON, 1.0f - FLT_EPSILON) );
	//multiply by virtual texture resolution
	texcoord.MulCW( idVec2(texinfo.resX, texinfo.resY) );
	//determine texel in virtual grid (GL_NEAREST)
	int x = int(texcoord.x), y = int(texcoord.y);
	assert(x >= texinfo.baseX && y >= texinfo.baseY && x < texinfo.baseX + w && y < texinfo.baseY + h);

	//find texel in actual image
	const byte *texel = &pic[4 * ((y - texinfo.baseY) * w + (x - texinfo.baseX))];
	//convert from 8-bit RGB into 24-bit time ratio
	//note: 8-bit grayscale image turns into ratio = (val/255)
	static const float TWO_POWER_MINUS_24 = 1.0f / (1<<24);
	float ratio = ((texel[0] * 256 + texel[1]) * 256 + texel[2]) * TWO_POWER_MINUS_24;
	return ratio;
}

float idParticle_FetchCutoffTimeLinear(const idImageAsset *image, int totalParticles, int index, int cycIdx) {
	assert(image);
	const byte *pic = image->cpuData.GetPic(0);

	//find texel in actual image
	int texelIdx = cycIdx * totalParticles + index;
	const byte *texel = &pic[4 * texelIdx];
	//convert from 8-bit RGB into 24-bit time ratio
	//note: 8-bit grayscale image turns into ratio = (val/255)
	static const float TWO_POWER_MINUS_24 = 1.0f / (1<<24);
	float ratio = ((texel[0] * 256 + texel[1]) * 256 + texel[2]) * TWO_POWER_MINUS_24;
	return ratio;
}
