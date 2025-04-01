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

#include "renderer/tr_local.h"

/*


Prelight models

"_prelight_<lightname>", ie "_prelight_light1"

Static surfaces available to dmap will be processed to optimized
shadow and lit surface geometry

Entity models are never prelighted.

Light entity can have a "noPrelight 1" key set to avoid the preprocessing
and carving of the world.  A light that will move should usually have this
set.

Prelight models will usually have multiple surfaces

Shadow volume surfaces will have the material "_shadowVolume"

The exact same vertexes as the ambient surfaces will be used for the
non-shadow surfaces, so there is opportunity to share


Reference their parent surfaces?
Reference their parent area?


If we don't track parts that are in different areas, there will be huge
losses when an areaportal closed door has a light poking slightly
through it.

There is potential benefit to splitting even the shadow volumes
at area boundaries, but it would involve the possibility of an
extra plane of shadow drawing at the area boundary.


interaction	lightName	numIndexes

Shadow volume surface

Surfaces in the world cannot have "no self shadow" properties, because all
the surfaces are considered together for the optimized shadow volume.  If
you want no self shadow on a static surface, you must still make it into an
entity so it isn't considered in the prelight.


r_hidePrelights
r_hideNonPrelights



each surface could include prelight indexes

generation procedure in dmap:

carve original surfaces into areas

for each light
	build shadow volume and beam tree
	cut all potentially lit surfaces into the beam tree
		move lit fragments into a new optimize group

optimize groups

build light models

*/

//anon beign
/*
=================
R_DeriveEntityData
=================
*/
void R_DeriveEntityData(idRenderEntityLocal* entity)
{
	R_AxisToModelMatrix(entity->parms.axis, entity->parms.origin, entity->modelMatrix);

	idRenderMatrix::CreateFromOriginAxis(entity->parms.origin, entity->parms.axis, entity->modelRenderMatrix);

	// calculate the matrix that transforms the unit cube to exactly cover the model in world space
	idRenderMatrix::OffsetScaleForBounds(entity->modelRenderMatrix, entity->referenceBounds, entity->inverseBaseModelProject);

	// calculate the global model bounds by inverse projecting the unit cube with the 'inverseBaseModelProject'
	idRenderMatrix::ProjectedBounds(entity->globalReferenceBounds, entity->inverseBaseModelProject, bounds_unitCube, false);

	entity->world->entityDefsBoundingSphere[entity->index] = R_BoundingSphereOfLocalBox(entity->referenceBounds, entity->modelMatrix);
}
//anon end

/*
=================================================================================

LIGHT TESTING

=================================================================================
*/


/*
====================
R_ModulateLights_f

Modifies the shaderParms on all the lights so the level
designers can easily test different color schemes
====================
*/
void R_ModulateLights_f( const idCmdArgs &args ) {

	if ( !tr.primaryWorld ) {
		return;
	} else if ( args.Argc() != 4 ) {
		common->Printf( "usage: modulateLights <redFloat> <greenFloat> <blueFloat>\n" );
		return;
	}
	int count = 0;
	double modulate[3] = {atof(args.Argv(1)), atof(args.Argv(2)), atof(args.Argv(3))};
	idRenderLightLocal *light;

	for (int i = 0 ; i < tr.primaryWorld->lightDefs.Num() ; i++ ) {
		light = tr.primaryWorld->lightDefs[i];
		if ( light ) {
			light->parms.shaderParms[0] *= modulate[0];
			light->parms.shaderParms[1] *= modulate[1];
			light->parms.shaderParms[2] *= modulate[2];
			count++;
		}
	}
	common->Printf( "modulated %i lights\n", count );
}



//======================================================================================


/*
===============
R_CreateEntityRefs

Creates all needed model references in portal areas,
chaining them to both the area and the entityDef.

Bumps tr.viewCount.
===============
*/
void R_CreateEntityRefs( idRenderEntityLocal *def ) {
	if ( !def->parms.hModel ) {
		def->parms.hModel = renderModelManager->DefaultModel();
	}

	// if the entity hasn't been fully specified due to expensive animation calcs
	// for md5 and particles, use the provided conservative bounds.
	if ( def->parms.callback ) {
		def->referenceBounds = def->parms.bounds;
	} else {
		def->referenceBounds = def->parms.hModel->Bounds( &def->parms );
	}

	// some models, like empty particles, may not need to be added at all
	if ( def->referenceBounds.IsCleared() ) {
		return;
	}

	if ( r_showUpdates.GetBool() && ( 
		(def->referenceBounds[1][0] - def->referenceBounds[0][0]) > 1024 ||
		(def->referenceBounds[1][1] - def->referenceBounds[0][1]) > 1024 ) ) {
		common->Printf( "big entityRef: %f,%f\n", def->referenceBounds[1][0] - def->referenceBounds[0][0], def->referenceBounds[1][1] - def->referenceBounds[0][1] );
	}

	// derive entity data
	R_DeriveEntityData(def);

	def->world->AddEntityToAreas(def);
}

/*
=================================================================================

CREATE LIGHT REFS

=================================================================================
*/

/*
=====================
R_SetLightProject

All values are reletive to the origin
Assumes that right and up are not normalized
This is also called by dmap during map processing.
=====================
*/
void R_SetLightProject( idPlane lightProject[4], const idVec3 origin, const idVec3 target,
					   const idVec3 rightVector, const idVec3 upVector, const idVec3 start, const idVec3 stop ) {
	float		dist, scale, ofs;
	idVec3		normal, startGlobal;
	idVec4		targetGlobal;

	idVec3 right = rightVector;
	const float	rLen = right.Normalize();

	idVec3 up = upVector;
	const float	uLen = up.Normalize();

	normal = up.Cross( right );
	//normal = right.Cross( up );
	normal.Normalize();

	dist = target * normal; //  - ( origin * normal );
	if ( dist < 0 ) {
		dist = -dist;
		normal = -normal;
	}
	scale = ( 0.5f * dist ) / rLen;
	right *= scale;
	scale = -( 0.5f * dist ) / uLen;
	up *= scale;

	lightProject[2] = normal;
	lightProject[2][3] = -( origin * lightProject[2].Normal() );

	lightProject[0] = right;
	lightProject[0][3] = -( origin * lightProject[0].Normal() );

	lightProject[1] = up;
	lightProject[1][3] = -( origin * lightProject[1].Normal() );

	// now offset to center
	targetGlobal.ToVec3() = target + origin;
	targetGlobal[3] = 1;
	ofs = 0.5f - ( targetGlobal * lightProject[0].ToVec4() ) / ( targetGlobal * lightProject[2].ToVec4() );
	lightProject[0].ToVec4() += ofs * lightProject[2].ToVec4();
	ofs = 0.5f - ( targetGlobal * lightProject[1].ToVec4() ) / ( targetGlobal * lightProject[2].ToVec4() );
	lightProject[1].ToVec4() += ofs * lightProject[2].ToVec4();

	// set the falloff vector
	normal = stop - start;

	dist = normal.Normalize();

	if ( dist <= 0 ) {
		dist = 1;
	}
	lightProject[3] = normal * ( 1.0f / dist );
	startGlobal = start + origin;
	lightProject[3][3] = -( startGlobal * lightProject[3].Normal() );
}

/*
===================
R_SetLightFrustum

Creates plane equations from the light projection, positive sides
face out of the light
===================
*/
void R_SetLightFrustum( const idPlane lightProject[4], idPlane frustum[6] ) {

	// we want the planes of s=0, s=q, t=0, and t=q
	frustum[0] = -lightProject[0];
	frustum[1] = -lightProject[1];
	frustum[2] = -(lightProject[2] - lightProject[0]);
	frustum[3] = -(lightProject[2] - lightProject[1]);

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	frustum[4] = -lightProject[3];
	frustum[5] = lightProject[3];
	frustum[5][3] -= 1.0f;

	frustum[5][3] /= frustum[5].Normalize();
	frustum[4][3] /= frustum[4].Normalize();
	frustum[3][3] /= frustum[3].Normalize();
	frustum[2][3] /= frustum[2].Normalize();
	frustum[1][3] /= frustum[1].Normalize();
	frustum[0][3] /= frustum[0].Normalize();
}

/*
====================
R_FreeLightDefFrustum
====================
*/
void R_FreeLightDefFrustum( idRenderLightLocal *ldef ) {

	// free the frustum tris
	if ( ldef->frustumTris ) {
		R_FreeStaticTriSurf( ldef->frustumTris );
		ldef->frustumTris = NULL;
	}
	// free frustum windings
	for ( int i = 0; i < 6; i++ ) {
		ldef->frustumWindings[i].SetNumPoints(0);
	}
}

/*
====================
R_ConvertLightProjectionNewToOld

stgatilov: there are two conventions about how to store projection matrix:
  old = Doom 3: X and Y are divided by Z, W gives falloff parameter (scaled)
  new = D3BFG: X and Y are divided by W, Z gives falloff parameter (0..1)
====================
*/
static void R_ConvertLightProjectionNewToOld(const idRenderMatrix &newProject, idPlane oldProject[4], double zScale)
{
	// set the old style light projection where Z and W are flipped and
	// for projected lights lightProject[3] is divided by ( zNear + zFar )
	oldProject[0][0] = newProject[0][0];
	oldProject[0][1] = newProject[0][1];
	oldProject[0][2] = newProject[0][2];
	oldProject[0][3] = newProject[0][3];

	oldProject[1][0] = newProject[1][0];
	oldProject[1][1] = newProject[1][1];
	oldProject[1][2] = newProject[1][2];
	oldProject[1][3] = newProject[1][3];

	oldProject[2][0] = newProject[3][0];
	oldProject[2][1] = newProject[3][1];
	oldProject[2][2] = newProject[3][2];
	oldProject[2][3] = newProject[3][3];

	oldProject[3][0] = newProject[2][0] * zScale;
	oldProject[3][1] = newProject[2][1] * zScale;
	oldProject[3][2] = newProject[2][2] * zScale;
	oldProject[3][3] = newProject[2][3] * zScale;
}

/*
========================
R_ComputePointLightProjectionMatrix

Computes the light projection matrix for a point light.
========================
*/
static void R_ComputePointLightProjectionMatrix(idRenderLightLocal* light, idRenderMatrix& localProject, idPlane oldProject[4])
{
	assert(light->parms.pointLight);

	// A point light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / idMath::Fmax(light->parms.lightRadius[0], 1e-10f);
	localProject[1][1] = 0.5f / idMath::Fmax(light->parms.lightRadius[1], 1e-10f);
	localProject[2][2] = 0.5f / idMath::Fmax(light->parms.lightRadius[2], 1e-10f);
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	R_ConvertLightProjectionNewToOld(localProject, oldProject, 1.0f);
}

idCVar r_spotlightBehavior(
	"r_spotlightBehavior", "0", CVAR_INTEGER | CVAR_RENDERER,
	"How to compute light volume & falloff for projected/spot lights (#5815):\n"
	"  Based on Doom 3 code:\n"
	"    0 --- as in Doom 3 and TDM from 2.00 to 2.07\n"
	"  Based on Doom 3 BFG code:\n"
	"    1 --- as in Doom 3 BFG\n"
	"      falloff is 0.0 at distance = N\n"
	"      falloff is 1.0 at distance = F + N\n"
	"    2 --- (not matching any released version)\n"
	"      falloff is 0.0 at distance = N\n"
	"      falloff is 1.0 at distance = F\n"
	"    3 --- as in TDM 2.08 and 2.09\n"
	"      falloff is 0.0 at distance = N\n"
	"      falloff is 1.0 at distance = F + N*N/(F+N)",
0, 3);

/*
========================
R_ComputeSpotLightProjectionMatrix

Computes the light projection matrix for a spot light.
========================
*/
static void R_ComputeSpotLightProjectionMatrix(idRenderLightLocal* light, idRenderMatrix& localProject, idPlane oldProject[4])
{
	static const float SPOT_LIGHT_MIN_Z_NEAR = 8.0f;
	static const float SPOT_LIGHT_MIN_Z_FAR = 16.0f;

	if (r_spotlightBehavior.GetInteger() == 0) {
		// stgatilov: that's how R_DeriveLightData calls R_SetLightProject in Doom 3
		const idVec3 rightVector = light->parms.right;
		const idVec3 upVector = light->parms.up;
		const idVec3 target = light->parms.target;
		const idVec3 origin = vec3_origin;
		const idVec3 start = light->parms.start;
		const idVec3 stop = light->parms.end;
		idPlane *lightProject = oldProject;

		// stgatilov: original function R_SetLightProject from Doom 3
		float		dist, distSE;
		float		scale;
		float		rLen, uLen;
		idVec3		normal, startToEnd;
		float		ofs;
		idVec3		right, up;
		idVec3		startGlobal;
		idVec4		targetGlobal;

		right = rightVector;
		rLen = right.Normalize();
		up = upVector;
		uLen = up.Normalize();
		normal = up.Cross( right );
		//normal = right.Cross( up );
		normal.Normalize();

		dist = target * normal; //  - ( origin * normal );
		if ( dist < 0 ) {
			dist = -dist;
			normal = -normal;
		}

		scale = ( 0.5f * dist ) / rLen;
		right *= scale;
		scale = -( 0.5f * dist ) / uLen;
		up *= scale;

		lightProject[2] = normal;
		lightProject[2][3] = -( origin * lightProject[2].Normal() );

		lightProject[0] = right;
		lightProject[0][3] = -( origin * lightProject[0].Normal() );

		lightProject[1] = up;
		lightProject[1][3] = -( origin * lightProject[1].Normal() );

		// now offset to center
		targetGlobal.ToVec3() = target + origin;
		targetGlobal[3] = 1;
		ofs = 0.5f - ( targetGlobal * lightProject[0].ToVec4() ) / ( targetGlobal * lightProject[2].ToVec4() );
		lightProject[0].ToVec4() += ofs * lightProject[2].ToVec4();
		ofs = 0.5f - ( targetGlobal * lightProject[1].ToVec4() ) / ( targetGlobal * lightProject[2].ToVec4() );
		lightProject[1].ToVec4() += ofs * lightProject[2].ToVec4();

		// set the falloff vector
		startToEnd = stop - start;
		distSE = startToEnd.Normalize();
		if ( distSE <= 0 ) {
			distSE = 1;
		}
		lightProject[3] = startToEnd * ( 1.0f / distSE );
		startGlobal = start + origin;
		lightProject[3][3] = -( startGlobal * lightProject[3].Normal() );

		// stgatilov: build frustum planes and polytope
		idPlane polytopePlanes[6];
		R_SetLightFrustum( lightProject, polytopePlanes );
		idVec3 verts[8];
		if ( !R_PolytopeSurfaceFrustumLike( polytopePlanes, verts, NULL ) ) {
			// start/end vectors are bad: they don't bound the frustum!
			// replace start = 0, end = normal and rebuild
			lightProject[3] = normal * ( 1.0f / dist );
			lightProject[3][3] = 0.0f;
			R_SetLightFrustum( lightProject, polytopePlanes );
			bool ok = R_PolytopeSurfaceFrustumLike( polytopePlanes, verts, NULL );
			assert(ok);
		}

		// stgatilov: find near/far planes bounding the polytope
		float zNear = idMath::INFINITY;
		float zFar = -idMath::INFINITY;
		for (int v = 0; v < 8; v++) {
			float dot = verts[v] * normal;
			zNear = idMath::Fmin(zNear, dot);
			zFar = idMath::Fmax(zFar, dot);
		}
		zNear = idMath::Fmax(zNear, SPOT_LIGHT_MIN_Z_NEAR);
		zFar -= zNear;	//zFar is actually (far - near) distance in BFG code
		zFar = idMath::Fmax(zFar, SPOT_LIGHT_MIN_Z_FAR);

		// stgatilov: build BFG-style projection matrix defining a bounding frustum
		// note: the lit polytope may be smaller in general case
		const float zScale = (zNear + zFar) / zFar;
		localProject[0][0] = lightProject[0][0];
		localProject[0][1] = lightProject[0][1];
		localProject[0][2] = lightProject[0][2];
		localProject[0][3] = lightProject[0][3];

		localProject[1][0] = lightProject[1][0];
		localProject[1][1] = lightProject[1][1];
		localProject[1][2] = lightProject[1][2];
		localProject[1][3] = lightProject[1][3];

		localProject[3][0] = lightProject[2][0];
		localProject[3][1] = lightProject[2][1];
		localProject[3][2] = lightProject[2][2];
		localProject[3][3] = lightProject[2][3];

		localProject[2][0] = normal[0] * zScale;
		localProject[2][1] = normal[1] * zScale;
		localProject[2][2] = normal[2] * zScale;
		localProject[2][3] = -zNear * zScale;
	}
	else {
		const float targetDistSqr = light->parms.target.LengthSqr();
		const float invTargetDist = idMath::InvSqrt(targetDistSqr);
		const float targetDist = invTargetDist * targetDistSqr;

		const idVec3 normalizedTarget = light->parms.target * invTargetDist;
		const idVec3 normalizedRight = light->parms.right * (0.5f * targetDist / light->parms.right.LengthSqr());
		const idVec3 normalizedUp = light->parms.up * (-0.5f * targetDist / light->parms.up.LengthSqr());

		localProject[0][0] = normalizedRight[0];
		localProject[0][1] = normalizedRight[1];
		localProject[0][2] = normalizedRight[2];
		localProject[0][3] = 0.0f;

		localProject[1][0] = normalizedUp[0];
		localProject[1][1] = normalizedUp[1];
		localProject[1][2] = normalizedUp[2];
		localProject[1][3] = 0.0f;

		localProject[3][0] = normalizedTarget[0];
		localProject[3][1] = normalizedTarget[1];
		localProject[3][2] = normalizedTarget[2];
		localProject[3][3] = 0.0f;

		// Set the falloff vector.
		// This is similar to the Z calculation for depth buffering, which means that the
		// mapped texture is going to be perspective distorted heavily towards the zero end.
		const float zNear = Max(light->parms.start * normalizedTarget, SPOT_LIGHT_MIN_Z_NEAR);
		/*const*/ float zFar = Max(light->parms.end * normalizedTarget, SPOT_LIGHT_MIN_Z_FAR);
		if (r_spotlightBehavior.GetInteger() == 2)
			zFar = Max(zFar - zNear, SPOT_LIGHT_MIN_Z_FAR);
		const float zScale = (zNear + zFar) / zFar;

		localProject[2][0] = normalizedTarget[0] * zScale;
		localProject[2][1] = normalizedTarget[1] * zScale;
		localProject[2][2] = normalizedTarget[2] * zScale;
		localProject[2][3] = -zNear * zScale;

		// now offset to the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range
		idVec4 projectedTarget;
		localProject.TransformPoint(light->parms.target, projectedTarget);

		const float ofs0 = 0.5f - projectedTarget[0] / projectedTarget[3];
		localProject[0][0] += ofs0 * localProject[3][0];
		localProject[0][1] += ofs0 * localProject[3][1];
		localProject[0][2] += ofs0 * localProject[3][2];
		localProject[0][3] += ofs0 * localProject[3][3];

		const float ofs1 = 0.5f - projectedTarget[1] / projectedTarget[3];
		localProject[1][0] += ofs1 * localProject[3][0];
		localProject[1][1] += ofs1 * localProject[3][1];
		localProject[1][2] += ofs1 * localProject[3][2];
		localProject[1][3] += ofs1 * localProject[3][3];

		float conversionScale = 1.0f / ( zNear + zFar );
		if (r_spotlightBehavior.GetInteger() == 3)
			conversionScale = 1.0f / zFar;
		R_ConvertLightProjectionNewToOld(localProject, oldProject, conversionScale);
	}
}

/*
========================
R_ComputeParallelLightProjectionMatrix

Computes the light projection matrix for a parallel light.
========================
*/
static void R_ComputeParallelLightProjectionMatrix(idRenderLightLocal* light, idRenderMatrix& localProject, idPlane oldProject[4])
{
	assert(light->parms.parallel);

	// A parallel light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / idMath::Fmax(light->parms.lightRadius[0], 1e-10f);
	localProject[1][1] = 0.5f / idMath::Fmax(light->parms.lightRadius[1], 1e-10f);
	localProject[2][2] = 0.5f / idMath::Fmax(light->parms.lightRadius[2], 1e-10f);
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	R_ConvertLightProjectionNewToOld(localProject, oldProject, 1.0f);
}

/*
=================
R_DeriveLightData

Fills everything in based on light->parms
=================
*/
void R_DeriveLightData( idRenderLightLocal *light ) {
	int i;

	// decide which light shader we are going to use
	if ( light->parms.shader ) {
		light->lightShader = light->parms.shader;
	}
	if ( !light->lightShader ) {
		if ( light->parms.pointLight ) {
			light->lightShader = declManager->FindMaterial( "lights/defaultPointLight" );
		} else {
			light->lightShader = declManager->FindMaterial( "lights/defaultProjectedLight" );
		}
	}

	// get the falloff image
	light->falloffImage = light->lightShader->LightFalloffImage();
	if ( !light->falloffImage ) {
		// use the falloff from the default shader of the correct type
		const idMaterial	*defaultShader;

		if ( light->parms.pointLight ) {
			defaultShader = declManager->FindMaterial( "lights/defaultPointLight" );
			light->falloffImage = defaultShader->LightFalloffImage();
		} else {
			// projected lights by default don't diminish with distance
			defaultShader = declManager->FindMaterial( "lights/defaultProjectedLight" );
			light->falloffImage = defaultShader->LightFalloffImage();
		}
	}
	// ------------------------------------
	// compute the light projection matrix
	// ------------------------------------

	idRenderMatrix localProject;	// new=BFG style projection matrix
	if ( light->parms.parallel ) {
		R_ComputeParallelLightProjectionMatrix( light, localProject, light->lightProject );
	} else if ( light->parms.pointLight ) {
		R_ComputePointLightProjectionMatrix( light, localProject, light->lightProject );
	} else {
		 R_ComputeSpotLightProjectionMatrix( light, localProject, light->lightProject );
	}

	// set the frustum planes
	R_SetLightFrustum( light->lightProject, light->frustum );

	// transform the lightProject
	float lightTransform[16];
	R_AxisToModelMatrix( light->parms.axis, light->parms.origin, lightTransform );
	for ( i = 0; i < 6; i++ ) {
		idPlane		temp;
		temp = light->frustum[i];
		R_LocalPlaneToGlobal( lightTransform, temp, light->frustum[i] );
	}
	for ( i = 0; i < 4; i++ ) {
		idPlane		temp;
		temp = light->lightProject[i];
		R_LocalPlaneToGlobal( lightTransform, temp, light->lightProject[i] );
	}

	// adjust global light origin for off center projections and parallel projections
	// we are just faking parallel by making it a very far off center for now
	if ( light->parms.parallel ) {
		idVec3 dir = light->parms.lightCenter;
		if ( dir.Normalize() == 0.0f ) {
			// make point straight up if not specified
			dir[2] = 1.0f;
		}
		light->globalLightOrigin = light->parms.origin + dir * 100000.0f;
	} else {
		light->globalLightOrigin = light->parms.origin + light->parms.axis * light->parms.lightCenter;
	}

	R_FreeLightDefFrustum( light );

	//light->frustumTris = R_PolytopeSurface( 6, light->frustum, light->frustumWindings );
	//stgatilov: specialized implementation gives perfectly precise watertight triangulation
	bool polytopeValid = R_PolytopeSurfaceFrustumLike( light->frustum, NULL, light->frustumWindings, &light->frustumTris );
	assert(polytopeValid);

	// a projected light will have one shadowFrustum, a point light will have
	// six unless the light center is outside the box
	R_MakeShadowFrustums( light );

	// Rotate and translate the light projection by the light matrix.
	// 99% of lights remain axis aligned in world space.
	idRenderMatrix lightMatrix;
	idRenderMatrix::CreateFromOriginAxis( light->parms.origin, light->parms.axis, lightMatrix );

	idRenderMatrix inverseLightMatrix;
	if ( !idRenderMatrix::Inverse( lightMatrix, inverseLightMatrix ) ) {
		idLib::Warning( "lightMatrix invert failed" );
	}

	// 'baseLightProject' goes from global space -> light local space -> light projective space
	idRenderMatrix::Multiply( localProject, inverseLightMatrix, light->baseLightProject );

	// Invert the light projection so we can deform zero-to-one cubes into
	// the light model and calculate global bounds.
	if ( !idRenderMatrix::Inverse( light->baseLightProject, light->inverseBaseLightProject ) ) {
		idLib::Warning( "baseLightProject invert failed" );
	}

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idRenderMatrix::ProjectedBounds( light->globalLightBounds, light->inverseBaseLightProject, bounds_zeroOneCube, false );
}

/*
=================
R_CreateLightRefs
=================
*/
void R_CreateLightRefs( idRenderLightLocal *light ) {
	light->world->AddLightToAreas( light );
}

/*
===============
R_RenderLightFrustum

Called by the editor and dmap to operate on light volumes
===============
*/
void R_RenderLightFrustum( const renderLight_t &renderLight, idPlane lightFrustum[6] ) {
	idRenderLightLocal	fakeLight;

	//memset( &fakeLight, 0, sizeof( fakeLight ) );
	fakeLight.parms = renderLight;

	R_DeriveLightData( &fakeLight );
	
	R_FreeStaticTriSurf( fakeLight.frustumTris );

	lightFrustum[0] = fakeLight.frustum[0];
	lightFrustum[1] = fakeLight.frustum[1];
	lightFrustum[2] = fakeLight.frustum[2];
	lightFrustum[3] = fakeLight.frustum[3];
	lightFrustum[4] = fakeLight.frustum[4];
	lightFrustum[5] = fakeLight.frustum[5];

}


//=================================================================================

/*
===============
WindingCompletelyInsideLight
===============
*/
bool WindingCompletelyInsideLight( const idWinding *w, const idRenderLightLocal *ldef ) {
	for ( int i = 0; i < w->GetNumPoints(); i++ ) {
		if ( idRenderMatrix::CullPointToMVP( ldef->baseLightProject, ( *w )[i].ToVec3(), true ) ) {
			return false;
		}
	}
	return true;
}

/*
======================
R_CreateLightDefFogPortals

When a fog light is created or moved, see if it completely
encloses any portals, which may allow them to be fogged closed.
======================
*/
void R_CreateLightDefFogPortals( idRenderLightLocal *ldef ) {
	areaReference_t		*lref;

	ldef->foggedPortals = NULL;
	if ( !ldef->lightShader->IsFogLight() ||
		// some fog lights will explicitly disallow portal fogging
		ldef->lightShader->TestMaterialFlag( MF_NOPORTALFOG) || 
	  ldef->parms.noPortalFog ) {
		return;
	} // #6282 added noPortalFog entity flag

	//const portal_t	*prt;
	portalArea_t	*area;
	doublePortal_t	*dp;

	for ( lref = ldef->references ; lref ; lref = lref->next ) {
		// check all the models in this area
		int areaIdx = lref->areaIdx;
		area = &ldef->world->portalAreas[areaIdx];

		for ( auto const &prt: area->areaPortals ) {
			dp = prt->doublePortal;

			// we only handle a single fog volume covering a portal
			// this will never cause incorrect drawing, but it may
			// fail to cull a portal 
			if ( dp->fogLight ) {
				continue;
			}
			
			if ( WindingCompletelyInsideLight( &prt->w, ldef ) ) {
				dp->fogLight = ldef;
				dp->nextFoggedPortal = ldef->foggedPortals;
				ldef->foggedPortals = dp;
			}
		}
	}
}

/*
====================
R_FreeLightDefDerivedData

Frees all references and lit surfaces from the light
====================
*/
void R_FreeLightDefDerivedData( idRenderLightLocal *def ) {

	// remove any portal fog references
	doublePortal_t *dp = def->foggedPortals;
	while ( dp ) {
		dp->fogLight = NULL;
		dp = dp->nextFoggedPortal;
	}

	// free all the interactions
	while ( def->firstInteraction ) {
		def->firstInteraction->UnlinkAndFree();
	}

	// free all the references to the light
	for ( areaReference_t *ref = def->references; ref; ref = ref->next ) {
		// look at the area with reference
		int areaIdx = ref->areaIdx;
		portalArea_t *area = &def->world->portalAreas[areaIdx];
		assert(area->lightBackRefs[ref->idxInArea] == ref);
		assert(area->lightRefs[ref->idxInArea] == def->index);

		// overwrite our deleted ref with the last ref in the area
		int last = area->lightRefs.Num() - 1;
		assert(ref->idxInArea == last || def->index != area->lightRefs[last]);
		assert(area->lightBackRefs[last]->areaIdx == areaIdx);
		assert(area->lightBackRefs[last]->idxInArea == last);
		area->lightRefs[ref->idxInArea] = area->lightRefs[last];
		area->lightBackRefs[ref->idxInArea] = area->lightBackRefs[last];
		area->lightBackRefs[ref->idxInArea]->idxInArea = ref->idxInArea;
		assert(area->lightBackRefs[ref->idxInArea]->areaIdx == areaIdx);
		// delete last ref
		area->lightRefs.Pop();
		area->lightBackRefs.Pop();
		
		// put it back on the free list for reuse
		def->world->areaReferenceAllocator.Free( ref );
	}
	def->references = NULL;

	def->areasForAdditionalWorldShadows.ClearFree();
	def->lightPortalFlow.Clear();

	R_FreeLightDefFrustum( def );
}

/*
===================
R_FreeEntityDefDerivedData

Used by both RE_FreeEntityDef and RE_UpdateEntityDef
Does not actually free the entityDef.
===================
*/
void R_FreeEntityDefDerivedData( idRenderEntityLocal *def, bool keepDecals, bool keepCachedDynamicModel ) {
	// demo playback needs to free the joints, while normal play
	// leaves them in the control of the game
	if ( session->readDemo ) {
		if ( def->parms.joints ) {
			Mem_Free16( def->parms.joints );
			def->parms.joints = NULL;
		}
		if ( def->parms.callbackData ) {
			Mem_Free( def->parms.callbackData );
			def->parms.callbackData = NULL;
		}
		for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
			if ( def->parms.gui[ i ] ) {
				delete def->parms.gui[ i ];
				def->parms.gui[ i ] = NULL;
			}
		}
	}

	// free all the interactions
	while ( def->firstInteraction != NULL ) {
		def->firstInteraction->UnlinkAndFree();
	}

	// clear the dynamic model if present
	if ( def->dynamicModel ) {
		def->dynamicModel = NULL;

		if ( !keepCachedDynamicModel ) {
			delete def->cachedDynamicModel;
			def->cachedDynamicModel = NULL;
		}
	}

	if ( !keepDecals ) {
		R_FreeEntityDefDecals( def );
		R_FreeEntityDefOverlay( def );
	}

	bool forceShadowsBehindOpaque = ( def->parms.hModel->IsStaticWorldModel() || def->parms.forceShadowBehindOpaque );

	// free the entityRefs from the areas
	for ( areaReference_t *ref = def->entityRefs; ref; ref = ref->next ) {
		// look at the area with reference
		int areaIdx = ref->areaIdx;
		portalArea_t *area = &def->world->portalAreas[areaIdx];
		assert(area->entityBackRefs[ref->idxInArea] == ref);
		assert(area->entityRefs[ref->idxInArea] == def->index);

		// overwrite our deleted ref with the last ref in the area
		int last = area->entityRefs.Num() - 1;
		assert(ref->idxInArea == last || def->index != area->entityRefs[last]);
		assert(area->entityBackRefs[last]->areaIdx == areaIdx);
		assert(area->entityBackRefs[last]->idxInArea == last);
		area->entityRefs[ref->idxInArea] = area->entityRefs[last];
		area->entityBackRefs[ref->idxInArea] = area->entityBackRefs[last];
		area->entityBackRefs[ref->idxInArea]->idxInArea = ref->idxInArea;
		assert(area->entityBackRefs[ref->idxInArea]->areaIdx == areaIdx);
		// delete last ref
		area->entityRefs.Pop();
		area->entityBackRefs.Pop();

		// put it back on the free list for reuse
		def->world->areaReferenceAllocator.Free( ref );

		if ( forceShadowsBehindOpaque ) {
			// remove from special list of "hacked" entities which cast shadows behind walls
			for ( int i = 0; i < area->forceShadowsBehindOpaqueEntityRefs.Num(); i++ ) {
				if ( area->forceShadowsBehindOpaqueEntityRefs[i] == def->index ) {
					idSwap( area->forceShadowsBehindOpaqueEntityRefs[i], area->forceShadowsBehindOpaqueEntityRefs.Last() );
					area->forceShadowsBehindOpaqueEntityRefs.Pop();
					break;
				}
			}
		}
	}
	def->entityRefs = NULL;
}

/*
==================
R_ClearEntityDefDynamicModel

If we know the reference bounds stays the same, we
only need to do this on entity update, not the full
R_FreeEntityDefDerivedData
==================
*/
void R_ClearEntityDefDynamicModel( idRenderEntityLocal *def ) {
	idInteraction *inter = def->firstInteraction;

	// free all the interaction surfaces
	while ( inter != NULL && !inter->IsEmpty() ) {
		inter->FreeSurfaces();
		inter = inter->entityNext;
	}

	// clear the dynamic model if present
	if ( def->dynamicModel ) {
		def->dynamicModel = NULL;
	}
}

/*
===================
R_FreeEntityDefDecals
===================
*/
void R_FreeEntityDefDecals( idRenderEntityLocal *def ) {
	while( def->decals ) {
		idRenderModelDecal *next = def->decals->Next();
		idRenderModelDecal::Free( def->decals );
		def->decals = next;
	}
}

/*
===================
R_FreeEntityDefFadedDecals
===================
*/
void R_FreeEntityDefFadedDecals( idRenderEntityLocal *def, int time ) {
	def->decals = idRenderModelDecal::RemoveFadedDecals( def->decals, time );
}

/*
===================
R_FreeEntityDefOverlay
===================
*/
void R_FreeEntityDefOverlay( idRenderEntityLocal *def ) {
	if ( def->overlay ) {
		idRenderModelOverlay::Free( def->overlay );
		def->overlay = NULL;
	}
}

/*
===================
R_FreeDerivedData

ReloadModels and RegenerateWorld call this
// FIXME: need to do this for all worlds
===================
*/
void R_FreeDerivedData( void ) {
	int i;
	idRenderWorldLocal *rw;
	idRenderEntityLocal *def;
	idRenderLightLocal *light;

	for ( int j = 0; j < tr.worlds.Num(); j++ ) {
		rw = tr.worlds[j];

		for ( i = 0; i < rw->entityDefs.Num(); i++ ) {
			def = rw->entityDefs[i];
			if ( !def ) {
				continue;
			}
			R_FreeEntityDefDerivedData( def, false, false );
		}

		for ( i = 0; i < rw->lightDefs.Num(); i++ ) {
			light = rw->lightDefs[i];
			if ( !light ) {
				continue;
			}
			R_FreeLightDefDerivedData( light );
		}
	}
}

/*
===================
R_CheckForEntityDefsUsingModel
===================
*/
void R_CheckForEntityDefsUsingModel( idRenderModel *model ) {
	idRenderWorldLocal *rw;
	idRenderEntityLocal	*def;

	for ( int j = 0; j < tr.worlds.Num(); j++ ) {
		rw = tr.worlds[j];

		for (int i = 0 ; i < rw->entityDefs.Num(); i++ ) {
			def = rw->entityDefs[i];
			if ( def && def->parms.hModel == model ) {
				//assert( 0 );
				// this should never happen but Radiant messes it up all the time so just free the derived data
				R_FreeEntityDefDerivedData( def, false, false );
			}
		}
	}
}

/*
===================
R_ReCreateWorldReferences

ReloadModels and RegenerateWorld call this
// FIXME: need to do this for all worlds
===================
*/
void R_ReCreateWorldReferences( void ) {
	int i;
	idRenderWorldLocal *rw;
	idRenderEntityLocal *def;
	idRenderLightLocal *light;

	// let the interaction generation code know this shouldn't be optimized for
	// a particular view
	tr.viewDef = NULL;

	for ( int j = 0; j < tr.worlds.Num(); j++ ) {
		rw = tr.worlds[j];

		for ( i = 0 ; i < rw->entityDefs.Num() ; i++ ) {
			def = rw->entityDefs[i];
			if ( !def ) {
				continue;
			}

			// the world model entities are put specifically in a single
			// area, instead of just pushing their bounds into the tree
			if ( i < rw->portalAreas.Num() ) {
				rw->AddEntityRefToArea( def, &rw->portalAreas[i] );
			} else {
				R_CreateEntityRefs( def );
			}
		}

		for ( i = 0 ; i < rw->lightDefs.Num() ; i++ ) {
			light = rw->lightDefs[i];
			if ( !light ) {
				continue;
			}

			renderLight_t parms = light->parms;
			light->world->FreeLightDef( i );
			rw->UpdateLightDef( i, &parms );
		}
	}
}

/*
===================
R_RegenerateWorld_f

Frees and regenerates all references and interactions, which
must be done when switching between display list mode and immediate mode
===================
*/
void R_RegenerateWorld_f( const idCmdArgs &args ) {
	R_FreeDerivedData();

	// watch how much memory we allocate
	tr.staticAllocCount = 0;

	R_ReCreateWorldReferences();

	renderModelManager->EndLevelLoad();

	common->Printf( "Regenerated world, staticAllocCount = %i.\n", tr.staticAllocCount );
}
