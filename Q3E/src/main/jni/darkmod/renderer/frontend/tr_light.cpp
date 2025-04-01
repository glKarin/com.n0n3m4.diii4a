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
#include "renderer/resources/Model_local.h"
#include "renderer/backend/FrameBufferManager.h"

#define CHECK_BOUNDS_EPSILON			1.0f

idCVar r_maxShadowMapLight( "r_maxShadowMapLight", "1000", CVAR_ARCHIVE | CVAR_RENDERER, "lights bigger than this will be force-sent to stencil" );
idCVar r_useParallelAddModels( "r_useParallelAddModels", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "parallelize R_AddModelSurfaces in frontend using jobs" );
idCVarBool r_useClipPlaneCulling( "r_useClipPlaneCulling", "1", CVAR_RENDERER, "cull surfaces behind mirrors" );

/*
===========================================================================================

VERTEX CACHE GENERATORS

===========================================================================================
*/

/*
==================
R_CreateAmbientCache

Create it if needed
==================
*/
bool R_CreateAmbientCache( srfTriangles_t *tri, bool needsLighting ) {
	if ( vertexCache.CacheIsCurrent( tri->ambientCache ) ) {
		return true;
	}

	// we are going to use it for drawing, so make sure we have the tangents and normals
	if ( needsLighting && !tri->tangentsCalculated ) {
		R_DeriveTangents( tri );
	}
	tri->ambientCache = vertexCache.AllocVertex( tri->verts, tri->numVerts * sizeof( tri->verts[0] ) );

	return tri->ambientCache.IsValid();
}

/*
==================
R_CreatePrivateShadowCache

This is used only for a specific light
==================
*/
void R_CreatePrivateShadowCache( srfTriangles_t *tri ) {
	if ( vertexCache.CacheIsCurrent( tri->shadowCache ) ) {
		return;
	}
	tri->shadowCache = vertexCache.AllocVertex( tri->shadowVertexes, tri->numVerts * sizeof( *tri->shadowVertexes ) );
}

/*
==================
R_CreateVertexProgramShadowCache

This is constant for any number of lights, the vertex program
takes care of projecting the verts to infinity.
==================
*/
void R_CreateVertexProgramShadowCache( srfTriangles_t *tri ) {
	if ( !tri->verts ) {
		return;
	}
	if ( vertexCache.CacheIsCurrent( tri->shadowCache ) ) {
		return;
	}
	shadowCache_t *temp = (shadowCache_t *)_alloca16( tri->numVerts * 2 * sizeof( shadowCache_t ) );
	SIMDProcessor->CreateVertexProgramShadowCache( &temp->xyz, tri->verts, tri->numVerts );
	tri->shadowCache = vertexCache.AllocVertex( temp, tri->numVerts * 2 * sizeof( shadowCache_t ) );
}

/*
==================
R_SkyboxTexGen
==================
*/
//void R_SkyboxTexGen( drawSurf_t *surf, const idVec3 &viewOrg ) {
//	idVec3	localViewOrigin;
//
//	R_GlobalPointToLocal( surf->space->modelMatrix, viewOrg, localViewOrigin );
//
//	const idDrawVert *verts = surf->frontendGeo->verts;
//	const int numVerts = surf->frontendGeo->numVerts;
//	const int size = numVerts * sizeof( idVec3 );
//	idVec3 *texCoords = (idVec3 *) _alloca16( size );
//
//	for ( int i = 0; i < numVerts; i++ ) {
//		texCoords[i][0] = verts[i].xyz[0] - localViewOrigin[0];
//		texCoords[i][1] = verts[i].xyz[1] - localViewOrigin[1];
//		texCoords[i][2] = verts[i].xyz[2] - localViewOrigin[2];
//	}
//	surf->dynamicTexCoords = vertexCache.AllocVertex( texCoords, size );
//}

/*
==================
R_WobbleskyTexGen
==================
*/
//void R_WobbleskyTexGen( drawSurf_t *surf, const idVec3 &viewOrg ) {
//	idVec3	localViewOrigin;
//
//	const int *parms = surf->material->GetTexGenRegisters();
//
//	const float	wobbleDegrees = surf->shaderRegisters[ parms[0] ] * idMath::PI / 180.0f;
//	const float	wobbleSpeed   = surf->shaderRegisters[ parms[1] ] * 2.0f * idMath::PI / 60.0f;
//	const float	rotateSpeed   = surf->shaderRegisters[ parms[2] ] * 2.0f * idMath::PI / 60.0f;
//
//	// very ad-hoc "wobble" transform
//	float	transform[16];
//	const float	a = tr.viewDef->floatTime * wobbleSpeed;
//	const float	z = cos( wobbleDegrees );
//	float	s = sin( a ) * sin( wobbleDegrees );
//	float	c = cos( a ) * sin( wobbleDegrees );
//
//	idVec3	axis[3];
//
//	axis[2][0] = c;
//	axis[2][1] = s;
//	axis[2][2] = z;
//
//	axis[1][0] = -sin( a * 2.0f ) * sin( wobbleDegrees );
//	axis[1][2] = -s * sin( wobbleDegrees );
//	axis[1][1] = sqrt( 1.0f - ( axis[1][0] * axis[1][0] + axis[1][2] * axis[1][2] ) );
//
//	// make the second vector exactly perpendicular to the first
//	axis[1] -= ( axis[2] * axis[1] ) * axis[2];
//	axis[1].Normalize();
//
//	// construct the third with a cross
//	axis[0].Cross( axis[1], axis[2] );
//
//	// add the rotate
//	s = sin( rotateSpeed * tr.viewDef->floatTime );
//	c = cos( rotateSpeed * tr.viewDef->floatTime );
//
//	transform[0] = axis[0][0] * c + axis[1][0] * s;
//	transform[4] = axis[0][1] * c + axis[1][1] * s;
//	transform[8] = axis[0][2] * c + axis[1][2] * s;
//
//	transform[1] = axis[1][0] * c - axis[0][0] * s;
//	transform[5] = axis[1][1] * c - axis[0][1] * s;
//	transform[9] = axis[1][2] * c - axis[0][2] * s;
//
//	transform[2] = axis[2][0];
//	transform[6] = axis[2][1];
//	transform[10] = axis[2][2];
//
//	transform[3] = transform[7] = transform[11] = 0.0f;
//	transform[12] = transform[13] = transform[14] = 0.0f;
//
//	R_GlobalPointToLocal( surf->space->modelMatrix, viewOrg, localViewOrigin );
//
//	const int numVerts = surf->frontendGeo->numVerts;
//	const int size = numVerts * sizeof( idVec3 );
//	idVec3 *texCoords = (idVec3 *) _alloca16( size );
//	const idDrawVert *verts = surf->frontendGeo->verts;
//	idVec3 v;
//
//	for (int i = 0; i < numVerts; i++ ) {
//		v[0] = verts[i].xyz[0] - localViewOrigin[0];
//		v[1] = verts[i].xyz[1] - localViewOrigin[1];
//		v[2] = verts[i].xyz[2] - localViewOrigin[2];
//
//		R_LocalPointToGlobal( transform, v, texCoords[i] );
//	}
//	surf->dynamicTexCoords = vertexCache.AllocVertex( texCoords, size );
//}

//=======================================================================================================

/*
=============
R_SetEntityDefViewEntity

If the entityDef isn't already on the viewEntity list, create
a viewEntity and add it to the list with an empty scissor rect.

This does not instantiate dynamic models for the entity yet.
=============
*/
viewEntity_t *R_SetEntityDefViewEntity( idRenderEntityLocal *def ) {
	viewEntity_t		*vModel;

	if ( def->viewCount == tr.viewCount ) {
		return def->viewEntity;
	}
	def->viewCount = tr.viewCount;
	def->world->entityDefsInView.SetBitTrue(def->index);

	// set the model and modelview matricies
	vModel = (viewEntity_t *)R_ClearedFrameAlloc( sizeof( *vModel ) );
	vModel->entityDef = def;

	// the scissorRect will be expanded as the model bounds is accepted into visible portal chains
	vModel->scissorRect.Clear();
	vModel->scissorRect.zmin = 0.0f;
	vModel->scissorRect.zmax = 1.0f;

	// copy the model and weapon depth hack for back-end use
	vModel->modelDepthHack = def->parms.modelDepthHack;
	vModel->weaponDepthHack = def->parms.weaponDepthHack;

	R_AxisToModelMatrix( def->parms.axis, def->parms.origin, vModel->modelMatrix );

	// we may not have a viewDef if we are just creating shadows at entity creation time
	if ( tr.viewDef ) {
		myGlMultMatrix( vModel->modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, vModel->modelViewMatrix );

		vModel->next = tr.viewDef->viewEntitys;
		tr.viewDef->viewEntitys = vModel;
	}
	def->viewEntity = vModel;

	return vModel;
}

/*
====================
R_TestPointInViewLight
====================
*/
static const float INSIDE_LIGHT_FRUSTUM_SLOP = 32.0f;
// this needs to be greater than the dist from origin to corner of near clip plane
static bool R_TestPointInViewLight( const idVec3 &org, const idRenderLightLocal *light ) {

	for ( int i = 0 ; i < 6 ; i++ ) {
		if ( light->frustum[i].Distance( org ) > INSIDE_LIGHT_FRUSTUM_SLOP ) {
			return false;
		}
	}
	return true;
}

/*
===================
R_PointInFrustum

Assumes positive sides face outward
===================
*/
static bool R_PointInFrustum( idVec3 &p, idPlane *planes, int numPlanes ) {

	for ( int i = 0 ; i < numPlanes ; i++ ) {
		if ( planes[i].Distance( p ) > 0.0f ) {
			return false;
		}
	}
	return true;
}

idCVar r_volumetricEnable(
	"r_volumetricEnable", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE,
	"If set to 0, then all volumetric lights are disabled. "
);
idCVar r_volumetricForceShadowMaps(
	"r_volumetricForceShadowMaps", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE,
	"If volumetrics need shadows, then switch the light to shadow maps even if stencil shadows are preferred in general. "
);
idCVar r_volumetricDustMultiplier(
	"r_volumetricDustMultiplier", "1", CVAR_ARCHIVE | CVAR_FLOAT | CVAR_RENDERER,
	"Multiplier applied to volumetric dust parameter (makes volumetrics stronger/weaker). ",
	0.001f, 100.0f
);

/*
=============
R_SetLightDefViewLight

If the lightDef isn't already on the viewLight list, create
a viewLight and add it to the list with an empty scissor rect.
=============
*/
viewLight_t *R_SetLightDefViewLight( idRenderLightLocal *light ) {
	viewLight_t *vLight;

	if ( light->viewCount == tr.viewCount ) {
		return light->viewLight;
	}
	light->viewCount = tr.viewCount;

	// add to the view light chain
	vLight = (viewLight_t *)R_ClearedFrameAlloc( sizeof( *vLight ) );
	vLight->lightDef = light;
	vLight->pointLight = light->parms.pointLight;
	vLight->noShadows = light->parms.noShadows;
	vLight->noSpecular = light->parms.noSpecular;

	// the scissorRect will be expanded as the light bounds is accepted into visible portal chains
	vLight->scissorRect.Clear();
	vLight->scissorRect.zmin = 0.0f;
	vLight->scissorRect.zmax = 1.0f;

	// calculate the shadow cap optimization states
	vLight->viewInsideLight = R_TestPointInViewLight( tr.viewDef->renderView.vieworg, light );
	if ( !vLight->viewInsideLight ) {
		vLight->viewSeesShadowPlaneBits = 0;
		for ( int i = 0 ; i < light->numShadowFrustums ; i++ ) {
			float d = light->shadowFrustums[i].planes[5].Distance( tr.viewDef->renderView.vieworg );
			if ( d < INSIDE_LIGHT_FRUSTUM_SLOP ) {
				vLight->viewSeesShadowPlaneBits|= 1 << i;
			}
		}
	} else {
		// this should not be referenced in this case
		vLight->viewSeesShadowPlaneBits = 63;
	}

	// find maximum distance from light origin to vertices of its light frustum
	ALIGN16( frustumCorners_t corners );
	idRenderMatrix::GetFrustumCorners( corners, light->inverseBaseLightProject, bounds_zeroOneCube );
	float lightFrustumRadius = 0.0f;
	for ( int i = 0; i < NUM_FRUSTUM_CORNERS; i++ )
		lightFrustumRadius = idMath::Fmax( lightFrustumRadius, ( idVec3( corners.x[i], corners.y[i], corners.z[i] ) - light->globalLightOrigin ).LengthSqr() );
	vLight->maxLightDistance = idMath::Sqrt( lightFrustumRadius );

	// see if the light center is in view, which will allow us to cull invisible shadows
	vLight->viewSeesGlobalLightOrigin = R_PointInFrustum( light->globalLightOrigin, tr.viewDef->frustum, 4 );

	// copy data used by backend
	vLight->globalLightOrigin = light->globalLightOrigin;
	vLight->lightProject[0] = light->lightProject[0];
	vLight->lightProject[1] = light->lightProject[1];
	vLight->lightProject[2] = light->lightProject[2];
	vLight->lightProject[3] = light->lightProject[3];

	// the fog plane is the light far clip plane
	idPlane fogPlane(light->baseLightProject[2][0] - light->baseLightProject[3][0],
		light->baseLightProject[2][1] - light->baseLightProject[3][1],
		light->baseLightProject[2][2] - light->baseLightProject[3][2],
		light->baseLightProject[2][3] - light->baseLightProject[3][3]);
	const float planeScale = idMath::InvSqrt(fogPlane.Normal().LengthSqr());
	vLight->fogPlane[0] = fogPlane[0] * planeScale;
	vLight->fogPlane[1] = fogPlane[1] * planeScale;
	vLight->fogPlane[2] = fogPlane[2] * planeScale;
	vLight->fogPlane[3] = fogPlane[3] * planeScale;

	// make a copy of the frustum for backend rendering
	vLight->frustumTris = ( srfTriangles_t* )R_FrameAlloc( sizeof( srfTriangles_t ) );
	memcpy( vLight->frustumTris, light->frustumTris, sizeof( srfTriangles_t ) );
	vLight->falloffImage = light->falloffImage;
	vLight->lightShader = light->lightShader;
	vLight->shaderRegisters = NULL;		// allocated and evaluated in R_AddLightSurfaces
	vLight->noFogBoundary = light->parms.noFogBoundary; // #3664
	vLight->noPortalFog = light->parms.noPortalFog; // #6282

	auto shader = vLight->lightShader;
	auto tooBigForShadowMaps = ( (light->parms.lightRadius.Length() > r_maxShadowMapLight.GetFloat()) || ( light->parms.parallel ) );
	if ( !r_shadows.GetInteger() || vLight->noShadows || !shader->LightCastsShadows() )
		vLight->shadows = LS_NONE;
	else {
		if ( r_shadows.GetInteger() == 1 || tooBigForShadowMaps )
			vLight->shadows = LS_STENCIL;
		else
			vLight->shadows = LS_MAPS;
	}
	// stgatilov #5880: this is hacky!
	// ideally, we should determine both shadows and volumetric lights without view involved
	// used in view-independent function idInteraction::CreateInteraction to determine which implementation to generate shadow tris for
	light->shadows = vLight->shadows;

	// stgatilov #5816: copy volumetric dust settings, resolve noshadows behavior
	vLight->volumetricDust = light->parms.volumetricDust * r_volumetricDustMultiplier.GetFloat();
	if ( !r_volumetricEnable.GetBool() ) {
		// debug cvar says to remove volumetrics
		vLight->volumetricDust = 0.0f;
	}
	vLight->volumetricNoshadows = false;
	if ( vLight->lightShader->IsFogLight() ) {
		// no shadows in fog
		vLight->volumetricNoshadows = true;
	}
	else {
		if ( light->parms.volumetricNoshadows == 0 && vLight->shadows != LS_MAPS || r_volumetricSamples.GetInteger() <= 0 ) {
			if ( vLight->volumetricDust > 0.0f && r_volumetricForceShadowMaps.GetBool() && !tooBigForShadowMaps ) {
				// force shadow maps implementation
				// ATTENTION: modifies "shadows" member, which is normally defined earlier in this function
				light->shadows = vLight->shadows = LS_MAPS;
			}
			else {
				// volumetric light must never pass through walls, which can only be achieved with shadow map
				// so we have to disable the volumetric light entirely
				vLight->volumetricDust = 0.0f;
			}
		}
		if ( light->parms.volumetricNoshadows == 1 || vLight->shadows != LS_MAPS ) {
			// no shadow map available, or mapper said to ignore shadows -> disable shadows in volumetric light
			vLight->volumetricNoshadows = true;
		}
	}
	if ( tr.viewDef->isSubview && !tr.viewDef->isXray ) {
		// does not work in subviews, at least because currentDepth is not up-to-date
		// note: somehow, the engine does not like different shadows implementation in main view and lightgem,
		// so I placed this condition AFTER shadows implementation is chosen
		vLight->volumetricDust = 0.0f;
	}


	// multi-light shader stuff
	if ( shader->LightCastsShadows() && tooBigForShadowMaps ) // use stencil shadows
		vLight->singleLightOnly = true;
	if ( shader->IsAmbientLight() ) { // custom ambient projection
		vLight->singleLightOnly = !strstr( shader->GetName(), "ambientlightnfo" )
			&& !strstr( shader->GetName(), "ambient_biground" );
	}
	if ( !shader->IsAmbientLight() && !strstr( shader->GetName(), "biground" ) ) // custom point light projection
		vLight->singleLightOnly = true;
	if(vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight())
		vLight->singleLightOnly = true;

	// link the view light
	vLight->next = tr.viewDef->viewLights;
	tr.viewDef->viewLights = vLight;
	light->viewLight = vLight;

	return vLight;
}

/*
=================
idRenderWorldLocal::CreateLightDefInteractions

When a lightDef is determined to effect the view (contact the frustum and non-0 light), it will check to
make sure that it has interactions for all the entityDefs that it might possibly contact.

This does not guarantee that all possible interactions for this light are generated, only that
the ones that may effect the current view are generated. so it does need to be called every view.

This does not cause entityDefs to create dynamic models, all work is done on the referenceBounds.

All entities that have non-empty interactions with viewLights will
have viewEntities made for them and be put on the viewEntity list,
even if their surfaces aren't visible, because they may need to cast shadows.

Interactions are usually removed when a entityDef or lightDef is modified, unless the change
is known to not effect them, so there is no danger of getting a stale interaction, we just need to
check that needed ones are created.

An interaction can be at several levels:

Don't interact (but share an area) (numSurfaces = 0)
Entity reference bounds touches light frustum, but surfaces haven't been generated (numSurfaces = -1)
Shadow surfaces have been generated, but light surfaces have not.  The shadow surface may still be empty due to bounds being conservative.
Both shadow and light surfaces have been generated.  Either or both surfaces may still be empty due to conservative bounds.

=================
*/
void idRenderWorldLocal::CreateLightDefInteractions( idRenderLightLocal *ldef ) {
	TRACE_CPU_SCOPE_TEXT( "CreateLightDefInteractions", GetTraceLabel( ldef->parms ) );

	bool lightCastsShadows = !ldef->parms.noShadows && ldef->lightShader->LightCastsShadows();
	idRenderMatrix::CullSixPlanes2 lightCuller;
	lightCuller.Prepare(ldef->frustum);

	for ( areaReference_t *lref = ldef->references ; lref ; lref = lref->next ) {
		int areaIdx = lref->areaIdx;
		portalArea_t *area = &portalAreas[areaIdx];

		// stgatilov #6296: for noshadows light, skip areas outside view
		// this is valid because we can skip all entities outside view
		// and if entity is in view, at least one of its areas must be in view too
		if ( tr.viewDef && !lightCastsShadows && area->areaViewCount != tr.viewCount )
			continue;

		// check all the models in this area
		for ( int entityIdx : area->entityRefs ) {
			int edefInView = entityDefsInView.GetBit(entityIdx);
			assert(edefInView == (entityDefs[entityIdx]->viewCount == tr.viewCount));

			// if the entity doesn't have any light-interacting surfaces, we could skip this,
			// but we don't want to instantiate dynamic models yet, so we can't check that on
			// most things
			if ( tr.viewDef && !lightCastsShadows && !edefInView ) {
				// if the entity isn't viewed and light has now shadows, skip
				continue;
			}

			// stgatilov #6296: do very fast light-entity culling
			// bounding sphere of entity is compact and stored outside entityDef
			// we use frustum planes as exact representation of light volume: bounding sphere would sweep much more space
			if ( lightCuller.CullSphere( ldef->frustum, entityDefsBoundingSphere[entityIdx] ) )
				continue;

			// if any of the edef's interaction match this light, we don't
			// need to consider it. 
			idInteraction *inter = interactionTable.Find(this, ldef->index, entityIdx);
			if ( inter ) {
				// if this entity wasn't in view already, the scissor rect will be empty,
				// so it will only be used for shadow casting
				if ( !edefInView && !inter->IsEmpty() ) {
					R_SetEntityDefViewEntity( entityDefs[entityIdx] );
				}
				continue;
			}

			CreateNewLightDefInteraction( ldef, entityDefs[entityIdx] );
		}
	}

	// stgatilov #5172: add interactions with world geometry only in some areas
	// this is necessary for areas were light flow does not reach but wall shadows should be present
	for ( int areaIdx : ldef->areasForAdditionalWorldShadows ) {
		portalArea_t *area = &portalAreas[areaIdx];

		for ( int entityIdx : area->forceShadowsBehindOpaqueEntityRefs ) {
			idRenderEntityLocal *edef = entityDefs[entityIdx];

			// if any of the edef's interaction match this light, we don't
			// need to consider it. 
			idInteraction *inter = interactionTable.Find(this, ldef->index, entityIdx);
			if ( inter ) {
				// if this entity wasn't in view already, the scissor rect will be empty,
				// so it will only be used for shadow casting
				if ( !inter->IsEmpty() ) {
					R_SetEntityDefViewEntity( entityDefs[entityIdx] );
				}
				continue;
			}

			CreateNewLightDefInteraction( ldef, edef );
		}
	}
}

/*
=================
idRenderWorldLocal::CreateNewLightDefInteraction

stgatilov #5172: Extracted path of CreateLightDefInteractions to call it several times.
It assumes an interaction is not yet present and must be created, and does all the necessary processing.
=================
*/
void idRenderWorldLocal::CreateNewLightDefInteraction( idRenderLightLocal *ldef, idRenderEntityLocal *edef ) {
	// create a new interaction, but don't do any work other than bbox to frustum culling
	idInteraction *inter = idInteraction::AllocAndLink( edef, ldef );

	bool skipInteraction = false;
	if ( tr.viewDef && edef->viewCount != tr.viewCount ) {
		// if the entity isn't viewed and shadow is suppressed, skip
		if ( !r_skipSuppress.GetBool() ) {
			if ( edef->parms.suppressShadowInViewID && edef->parms.suppressShadowInViewID == tr.viewDef->renderView.viewID )
				skipInteraction = true;
			if ( edef->parms.suppressShadowInLightID && edef->parms.suppressShadowInLightID == ldef->parms.lightId )
				skipInteraction = true;
		}
	}
	// some big outdoor meshes are flagged to not create any dynamic interactions
	// when the level designer knows that nearby moving lights shouldn't actually hit them
	if ( edef->parms.noDynamicInteractions && generateAllInteractionsCalled ) {
		skipInteraction = true;
	}
	if ( r_singleEntity.GetInteger() >= 0 && r_singleEntity.GetInteger() != edef->index ) {
		skipInteraction = true;
	}
	if ( skipInteraction ) {
		inter->MakeEmpty();
		return;
	}

	// do a check of the entity reference bounds against the light frustum,
	// trying to avoid creating a viewEntity if it hasn't been already
	float	modelMatrix[16];
	float	*m;
	if ( edef->viewCount == tr.viewCount ) {
		m = edef->viewEntity->modelMatrix;
	} else {
		R_AxisToModelMatrix( edef->parms.axis, edef->parms.origin, modelMatrix );
		m = modelMatrix;
	}

	if ( R_CornerCullLocalBox( edef->referenceBounds, m, 6, ldef->frustum ) ) {
		inter->MakeEmpty();
		return;
	}

	extern idCVar r_useLightPortalFlowCulling;
	if ( r_useLightPortalFlowCulling.GetBool() ) {
		bool forceShadowsBehindOpaque = ( edef->parms.hModel->IsStaticWorldModel() || edef->parms.forceShadowBehindOpaque );
		if ( !forceShadowsBehindOpaque ) {
			// stgatilov #5172: check if entity bounds are visible through saved portal windings
			if ( CullInteractionByLightFlow( ldef, edef) ) {
				inter->MakeEmpty();
				return;
			}
		}
	}

	// we will do a more precise per-surface check when we are checking the entity
	// if this entity wasn't in view already, the scissor rect will be empty,
	// so it will only be used for shadow casting
	R_SetEntityDefViewEntity( edef );
}

bool idRenderWorldLocal::CullInteractionByLightFlow( idRenderLightLocal *ldef, idRenderEntityLocal *edef ) const {
	// check if light flow information was generated
	if ( ldef->lightPortalFlow.areaRefs.Num() == 0 )
		return false;

	const lightPortalFlow_t &flow = ldef->lightPortalFlow;
	idBox entityBox( edef->referenceBounds, edef->modelMatrix );

	for ( areaReference_t *eref = edef->entityRefs; eref; eref = eref->next ) {

		// find how light enters into particular area with the entity
		auto iterPair = std::equal_range(flow.areaRefs.begin(), flow.areaRefs.end(), lightPortalFlow_t::areaRef_t{eref->areaIdx, -1, -1});

		for ( const lightPortalFlow_t::areaRef_t *portalEntry = iterPair.first; portalEntry < iterPair.second; portalEntry++ ) {

			// for each entry, check if whole entity is outside light cone
			bool culled = false;
			for ( int planeIdx = portalEntry->planeBeg; planeIdx < portalEntry->planeEnd; planeIdx++ ) {
				const idPlane &plane = flow.planeStorage[planeIdx];

				int side = entityBox.PlaneSide( plane );
				if ( side == PLANESIDE_FRONT ) {
					culled = true;
					break;
				}
			}

			if ( !culled ) {
				// reached in at least one area by at least one portal entry
				return false;
			}
		}
	}

	return true;
}

static const int INTERACTION_TABLE_MAX_LIGHTS = 4096;
static const int INTERACTION_TABLE_MAX_ENTITYS = 8192/*MAX_GENTITIES*/;	//stgatilov: cannot allocate 2GB table =(
idInteractionTable::idInteractionTable() {
	SM_matrix = nullptr;
}
idInteractionTable::~idInteractionTable() {
	Shutdown();
}
void idInteractionTable::Init() {
	useInteractionTable = r_useInteractionTable.GetInteger();
	if (useInteractionTable == 1) {
		delete[] SM_matrix;
		SM_matrix = (idInteraction**)R_ClearedStaticAlloc(INTERACTION_TABLE_MAX_LIGHTS * INTERACTION_TABLE_MAX_ENTITYS * sizeof(SM_matrix[0]));
		if (!SM_matrix) {
			common->Error("Failed to allocate interaction table");
		}
	}
	if (useInteractionTable == 2) {
		SHT_table.Reserve(256, true);
	}
}
void idInteractionTable::Shutdown() {
	if (useInteractionTable == 1) {
		delete[] SM_matrix;
		SM_matrix = nullptr;
	}
	if (useInteractionTable == 2) {
		SHT_table.ClearFree();
	}
	useInteractionTable = -1;
}
DEBUG_OPTIMIZE_ON
idInteraction *idInteractionTable::Find(idRenderWorldLocal *world, int lightIdx, int entityIdx) const {
	if (useInteractionTable < 0)
		common->Error("Interaction table not initialized");
	if (useInteractionTable == 1) {
		int idx = lightIdx * INTERACTION_TABLE_MAX_ENTITYS + entityIdx;
		return SM_matrix[idx];
	}
	if (useInteractionTable == 2) {
		int key = (lightIdx << 16) + entityIdx;
		return SHT_table.Get(key, nullptr);
	}
	idRenderEntityLocal *edef = world->entityDefs[entityIdx];
	idRenderLightLocal *ldef = world->lightDefs[lightIdx];
	for ( idInteraction *inter = edef->lastInteraction; inter; inter = inter->entityPrev ) {
		if ( inter->lightDef == ldef ) {
			return inter;
		}
	}
	return nullptr;
}
DEBUG_OPTIMIZE_OFF
bool idInteractionTable::Add(idInteraction *interaction) {
	if (useInteractionTable < 0)
		common->Error("Interaction table not initialized");
	if (useInteractionTable == 1) {
		assert(interaction->lightDef->index < INTERACTION_TABLE_MAX_LIGHTS);
		assert(interaction->entityDef->index < INTERACTION_TABLE_MAX_ENTITYS);
		int idx = interaction->lightDef->index * INTERACTION_TABLE_MAX_ENTITYS + interaction->entityDef->index;
		idInteraction *&cell = SM_matrix[idx];
		if (cell) {
			return false;
		}
		cell = interaction;
		return true;
	}
	if (useInteractionTable == 2) {
		int key = (interaction->lightDef->index << 16 ) + interaction->entityDef->index;
		return SHT_table.AddIfNew(key, interaction);
	}

	return true;	//don't care
}
bool idInteractionTable::Remove(idInteraction *interaction) {
	if (useInteractionTable < 0)
		common->Error("Interaction table not initialized");
	if (useInteractionTable == 1) {
		int idx = interaction->lightDef->index * INTERACTION_TABLE_MAX_ENTITYS + interaction->entityDef->index;
		idInteraction *&cell = SM_matrix[idx];
		if (cell) {
			assert(cell == interaction);
			cell = nullptr;
			return true;
		}
		return false;
	}
	if (useInteractionTable == 2) {
		int key = (interaction->lightDef->index << 16 ) + interaction->entityDef->index;
		return SHT_table.Remove(key);
	}
	return true;	//don't care
}
idStr idInteractionTable::Stats() const {
	char buff[256];
	if (useInteractionTable == 1) {
		idStr::snPrintf(buff, sizeof(buff), "size = L%d x E%d x %dB = %d MB",
			INTERACTION_TABLE_MAX_LIGHTS, INTERACTION_TABLE_MAX_ENTITYS, int(sizeof(SM_matrix[0])),
			int( (INTERACTION_TABLE_MAX_LIGHTS * INTERACTION_TABLE_MAX_ENTITYS * sizeof(SM_matrix[0])) >> 20 )
		);
	}
	if (useInteractionTable == 2) {
		idStr::snPrintf(buff, sizeof(buff), "size = %d/%d", SHT_table.Num(), SHT_table.CellsNum());
	}
	return buff;
}

//===============================================================================================================

/*
=================
R_PrepareLightSurf
=================
*/
drawSurf_t *R_PrepareLightSurf( const srfTriangles_t *tri, const viewEntity_t *space,
		const idMaterial *material, const idScreenRect &scissor, bool viewInsideShadow, bool blockSelfShadows ) {
	if ( !space ) {
		space = &tr.viewDef->worldSpace;
	}
	drawSurf_t *drawSurf = (drawSurf_t *)R_FrameAlloc( sizeof( *drawSurf ) );

	drawSurf->CopyGeo( tri );
	drawSurf->space = space;
	drawSurf->material = material;
	drawSurf->scissorRect = scissor;
	drawSurf->dsFlags = 0;
	
	drawSurf->particle_radius = 0.0f; // #3878

	if ( viewInsideShadow ) {
		drawSurf->dsFlags |= DSF_VIEW_INSIDE_SHADOW;
	}
	if ( blockSelfShadows ) {	// #6571
		drawSurf->dsFlags |= DSF_BLOCK_SELF_SHADOWS;
	}

	if ( !material ) {
		// shadows won't have a shader
		drawSurf->shaderRegisters = NULL;
		if ( !(drawSurf->dsFlags & DSF_VIEW_INSIDE_SHADOW) )
			drawSurf->numIndexes = tri->numShadowIndexesNoCaps;
	} else {
		// process the shader expressions for conditionals / color / texcoords
		const float *constRegs = material->ConstantRegisters();
		if ( constRegs ) {
			// this shader has only constants for parameters
			drawSurf->shaderRegisters = constRegs;
		} else {
			// FIXME: share with the ambient surface?
			float *regs = (float *)R_FrameAlloc( material->GetNumRegisters() * sizeof( float ) );
			drawSurf->shaderRegisters = regs;
			material->EvaluateRegisters( regs, space->entityDef->parms.shaderParms, tr.viewDef, space->entityDef->parms.referenceSound );
		}
	}

	return drawSurf;
}

/*
======================
R_ClippedLightScissorRectangle
======================
*/
idScreenRect R_ClippedLightScissorRectangle( viewLight_t *vLight ) {
	const idRenderLightLocal *light = vLight->lightDef;
	idScreenRect r;
	idFixedWinding w;

	r.Clear();

	for ( int i = 0 ; i < 6 ; i++ ) {
		const idWinding &ow = light->frustumWindings[i];

		// !ow - projected lights may have one of the frustums degenerated
		// OR
		// light->frustum[i].Distance( tr.viewDef->renderView.vieworg ) >= 0
		// the light frustum planes face out from the light,
		// so the planes that have the view origin on the negative
		// side will be the "back" faces of the light, which must have
		// some fragment inside the portalStack to be visible
		if ( !ow.GetNumPoints() || light->frustum[i].Distance( tr.viewDef->renderView.vieworg ) >= 0 ) {
			continue;
		}
		w = ow;

		// now check the winding against each of the frustum planes
		for ( int j = 0; j < 5; j++ ) {
			if ( !w.ClipInPlace( -tr.viewDef->frustum[j] ) ) {
				break;
			}
		}

		// project these points to the screen and add to bounds
		for ( int j = 0 ; j < w.GetNumPoints(); j++ ) {
			idPlane		eye, clip;
			idVec3		ndc;

			R_TransformModelToClip( w[j].ToVec3(), tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );

			if ( clip[3] <= 0.01f ) {
				clip[3] = 0.01f;
			}
			R_TransformClipToDevice( clip, tr.viewDef, ndc );

			float windowX = 0.5f * ( 1.0f + ndc[0] ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 );
			float windowY = 0.5f * ( 1.0f + ndc[1] ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 );

			if ( windowX > tr.viewDef->scissor.x2 ) {
				windowX = tr.viewDef->scissor.x2;
			} else if ( windowX < tr.viewDef->scissor.x1 ) {
				windowX = tr.viewDef->scissor.x1;
			}
			if ( windowY > tr.viewDef->scissor.y2 ) {
				windowY = tr.viewDef->scissor.y2;
			} else if ( windowY < tr.viewDef->scissor.y1 ) {
				windowY = tr.viewDef->scissor.y1;
			}
			r.AddPoint( windowX, windowY );
		}
	}

	// add the fudge boundary
	r.Expand();

	return r;
}

/*
==================
R_CalcLightScissorRectangle

The light screen bounds will be used to crop the scissor rect during
stencil clears and interaction drawing
==================
*/
static int c_clippedLight = 0, c_unclippedLight = 0;

idScreenRect R_CalcLightScissorRectangle( viewLight_t *vLight ) {
	idScreenRect	r;
	idPlane			eye, clip;
	idVec3			ndc;

	if ( vLight->lightDef->parms.pointLight ) {
		idBounds bounds;
		idRenderLightLocal *lightDef = vLight->lightDef;
		if ( tr.viewDef->viewFrustum.ProjectionBounds( idBox( lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis ), bounds ) )
			r = R_ScreenRectFromViewFrustumBounds( bounds );
		else
			r.ClearWithZ();
		return r;
	}

	if ( r_useClippedLightScissors.GetInteger() == 2 ) {
		return R_ClippedLightScissorRectangle( vLight );
	}
	r.Clear();

	const srfTriangles_t *tri = vLight->lightDef->frustumTris;

	for ( int i = 0 ; i < tri->numVerts ; i++ ) {
		R_TransformModelToClip( tri->verts[i].xyz, tr.viewDef->worldSpace.modelViewMatrix,
			tr.viewDef->projectionMatrix, eye, clip );

		// if it is near clipped, clip the winding polygons to the view frustum
		if ( clip[3] <= 1 ) {
			c_clippedLight++;
			if ( r_useClippedLightScissors.GetInteger() ) {
				return R_ClippedLightScissorRectangle( vLight );
			} else {
				r.x1 = r.y1 = 0;
				r.x2 = ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) - 1;
				r.y2 = ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) - 1;
				return r;
			}
		}
		R_TransformClipToDevice( clip, tr.viewDef, ndc );

		float windowX = 0.5f * ( 1.0f + ndc[0] ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 );
		float windowY = 0.5f * ( 1.0f + ndc[1] ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 );

		if ( windowX > tr.viewDef->scissor.x2 ) {
			windowX = tr.viewDef->scissor.x2;
		} else if ( windowX < tr.viewDef->scissor.x1 ) {
			windowX = tr.viewDef->scissor.x1;
		}
		if ( windowY > tr.viewDef->scissor.y2 ) {
			windowY = tr.viewDef->scissor.y2;
		} else if ( windowY < tr.viewDef->scissor.y1 ) {
			windowY = tr.viewDef->scissor.y1;
		}
		r.AddPoint( windowX, windowY );
	}

	// add the fudge boundary
	r.Expand();

	c_unclippedLight++;

	return r;
}

/*
=================
R_AddLightSurfaces

Calc the light shader values, removing any light from the viewLight list
if it is determined to not have any visible effect due to being flashed off or turned off.

Adds entities to the viewEntity list if they are needed for shadow casting.

Add any precomputed shadow volumes.

Removes lights from the viewLights list if they are completely
turned off, or completely off screen.

Create any new interactions needed between the viewLights
and the viewEntitys due to game movement
=================
*/
void R_AddLightSurfaces( void ) {
	TRACE_CPU_SCOPE( "R_AddLightSurfaces" );
	
	viewLight_t			*vLight;
	idRenderLightLocal	*light;
	viewLight_t			**ptr;

	// go through each visible light, possibly removing some from the list
	ptr = &tr.viewDef->viewLights;
	while ( *ptr ) {
		vLight = *ptr;
		light = vLight->lightDef;
		const idMaterial *lightShader = light->lightShader;

		if ( !lightShader ) {
			common->Error( "R_AddLightSurfaces: NULL lightShader" );
			return;
		}

		// see if we are suppressing the light in this view
		if ( !r_skipSuppress.GetBool() ) {
			bool suppress = light->parms.suppressLightInViewID && light->parms.suppressLightInViewID == tr.viewDef->renderView.viewID;
			suppress |= light->parms.allowLightInViewID && light->parms.allowLightInViewID != tr.viewDef->renderView.viewID;
			suppress |= (bool) ( light->parms.suppressInSubview & ( 1 << ( tr.viewDef->isSubview ? 0 : 1 ) ) );
			if ( suppress ) {
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
		}

		// evaluate the light shader registers
		float *lightRegs =(float *)R_FrameAlloc( lightShader->GetNumRegisters() * sizeof( float ) );
		vLight->shaderRegisters = lightRegs;
		lightShader->EvaluateRegisters( lightRegs, light->parms.shaderParms, tr.viewDef, light->parms.referenceSound );

		// if this is a purely additive light and no stage in the light shader evaluates
		// to a positive light value, we can completely skip the light
		if ( !lightShader->IsFogLight() && !lightShader->IsBlendLight() ) {
			int lightStageNum;
			for ( lightStageNum = 0 ; lightStageNum < lightShader->GetNumStages() ; lightStageNum++ ) {
				const shaderStage_t	*lightStage = lightShader->GetStage( lightStageNum );

				// ignore stages that fail the condition
				if ( !lightRegs[ lightStage->conditionRegister ] ) {
					continue;
				}
				const int *registers = lightStage->color.registers;

				// snap tiny values to zero to avoid lights showing up with the wrong color
				if ( lightRegs[ registers[0] ] < 0.001f ) {
					lightRegs[ registers[0] ] = 0.0f;
				}
				if ( lightRegs[ registers[1] ] < 0.001f ) {
					lightRegs[ registers[1] ] = 0.0f;
				}
				if ( lightRegs[ registers[2] ] < 0.001f ) {
					lightRegs[ registers[2] ] = 0.0f;
				}

				// FIXME:	when using the following values the light shows up bright red when using nvidia drivers/hardware
				//			this seems to have been fixed ?
				//lightRegs[ registers[0] ] = 1.5143074e-005f;
				//lightRegs[ registers[1] ] = 1.5483369e-005f;
				//lightRegs[ registers[2] ] = 1.7014690e-005f;

				if (lightRegs[ registers[0] ] > 0.0f ||
					lightRegs[ registers[1] ] > 0.0f ||
					lightRegs[ registers[2] ] > 0.0f ) {
					break;
				}
			}
			if ( lightStageNum == lightShader->GetNumStages() ) {
				// we went through all the stages and didn't find one that adds anything
				// remove the light from the viewLights list, and change its frame marker
				// so interaction generation doesn't think the light is visible and
				// create a shadow for it
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
		}

		if ( r_useLightScissors.GetBool() ) {
			// calculate the screen area covered by the light frustum
			// which will be used to crop the stencil cull
			idScreenRect scissorRect = R_CalcLightScissorRectangle( vLight );
			// intersect with the portal crossing scissor rectangle
			vLight->scissorRect.IntersectWithZ( scissorRect );

			if ( r_showLightScissors.GetBool() ) {
				R_ShowColoredScreenRect( vLight->scissorRect, light->index );
			}
		}

		// this one stays on the list
		ptr = &vLight->next;

		// create interactions with all entities the light may touch, and add viewEntities
		// that may cast shadows, even if they aren't directly visible.  Any real work
		// will be deferred until we walk through the viewEntities
		tr.viewDef->renderWorld->CreateLightDefInteractions( light );
		tr.pc.c_viewLights++;

		// fog lights will need to draw the light frustum triangles, so make sure they
		// are in the vertex cache
		if ( lightShader->IsFogLight() || light->parms.volumetricDust > 0.0f || r_showLights > 1 ) {
			if ( !vertexCache.CacheIsCurrent(vLight->frustumTris->ambientCache) ) {
				R_CreateAmbientCache( vLight->frustumTris, false );
			}
			if ( !vertexCache.CacheIsCurrent(vLight->frustumTris->indexCache) ) {
				vLight->frustumTris->indexCache = vertexCache.AllocIndex( vLight->frustumTris->indexes, vLight->frustumTris->numIndexes * sizeof( glIndex_t ) );
			}
		}

		// add the prelight shadows for the static world geometry
		if ( light->parms.prelightModel && r_useOptimizedShadows.GetBool() && vLight->shadows == LS_STENCIL ) {
			srfTriangles_t	*tri = light->parms.prelightModel->Surface( 0 )->geometry;

			// these shadows will all have valid bounds, and can be culled normally
			if ( r_useShadowCulling.GetBool() && R_CullLocalBox( tri->bounds, tr.viewDef->worldSpace.modelMatrix, 5, tr.viewDef->frustum ) ) {
					continue;
			}

			// if we have been purged, re-upload the shadowVertexes
			if ( !vertexCache.CacheIsCurrent( tri->shadowCache ) ) {
				R_CreatePrivateShadowCache( tri );
			}

			if ( !vertexCache.CacheIsCurrent( tri->indexCache ) ) {
				tri->indexCache = vertexCache.AllocIndex( tri->indexes, tri->numIndexes * sizeof( tri->indexes[0] ) );
			}
			drawSurf_t *surf = R_PrepareLightSurf( tri, NULL, NULL, vLight->scissorRect, true /* FIXME ? */, false );
			// actually link it in
			surf->nextOnLight = vLight->globalShadows;
			vLight->globalShadows = surf;
		}
	}
}

//===============================================================================================================

/*
==================
R_IssueEntityDefCallback
==================
*/
bool R_IssueEntityDefCallback( idRenderEntityLocal *def ) {
	bool update;
	idBounds	oldBounds;

	if ( r_checkBounds.GetBool() ) {
		oldBounds = def->referenceBounds;
	}
	def->archived = false;		// will need to be written to the demo file

	tr.pc.c_entityDefCallbacks++;

	if ( tr.viewDef ) {
		update = def->parms.callback( &def->parms, &tr.viewDef->renderView );
	} else {
		update = def->parms.callback( &def->parms, NULL );
	}

	if ( !def->parms.hModel ) {
		common->Error( "R_IssueEntityDefCallback: dynamic entity callback didn't set model" );
	}

	if ( r_checkBounds.GetBool() ) {
		if (	oldBounds[0][0] > def->referenceBounds[0][0] + CHECK_BOUNDS_EPSILON ||
				oldBounds[0][1] > def->referenceBounds[0][1] + CHECK_BOUNDS_EPSILON ||
				oldBounds[0][2] > def->referenceBounds[0][2] + CHECK_BOUNDS_EPSILON ||
				oldBounds[1][0] < def->referenceBounds[1][0] - CHECK_BOUNDS_EPSILON ||
				oldBounds[1][1] < def->referenceBounds[1][1] - CHECK_BOUNDS_EPSILON ||
				oldBounds[1][2] < def->referenceBounds[1][2] - CHECK_BOUNDS_EPSILON ) {
			common->Printf( "entity %i callback extended reference bounds\n", def->index );
		}
	}
	return update;
}

/*
===================
R_EntityDefDynamicModel

Issues a deferred entity callback if necessary.
If the model isn't dynamic, it returns the original.
Returns the cached dynamic model if present, otherwise creates
it and any necessary overlays
===================
*/
idRenderModel *R_EntityDefDynamicModel( idRenderEntityLocal *def ) {
	idScopedCriticalSection lock (def->mutex);

	bool callbackUpdate = false;

	// allow deferred entities to construct themselves
	if ( def->parms.callback ) {
		callbackUpdate = R_IssueEntityDefCallback( def );
	}
	idRenderModel *model = def->parms.hModel;

	if ( !model ) {
		common->Error( "R_EntityDefDynamicModel: NULL model" );
		return renderModelManager->DefaultModel();
	}

	else if ( model->IsDynamicModel() == DM_STATIC ) {
		def->dynamicModel = NULL;
		def->dynamicModelFrameCount = 0;
		return model;
	}

	// continously animating models (particle systems, etc) will have their snapshot updated every single view
	if ( callbackUpdate || ( model->IsDynamicModel() == DM_CONTINUOUS && def->dynamicModelFrameCount != tr.frameCount ) ) {
		R_ClearEntityDefDynamicModel( def );
	}

	// if we don't have a snapshot of the dynamic model, generate it now
	if ( !def->dynamicModel ) {

		// instantiate the snapshot of the dynamic model, possibly reusing memory from the cached snapshot
		def->cachedDynamicModel = model->InstantiateDynamicModel( &def->parms, tr.viewDef, def->cachedDynamicModel );

		if ( def->cachedDynamicModel ) {

			// add any overlays to the snapshot of the dynamic model
			if ( def->overlay && !r_skipOverlays.GetBool() ) {
				def->overlay->AddOverlaySurfacesToModel( def->cachedDynamicModel );
			} else {
				idRenderModelOverlay::RemoveOverlaySurfacesFromModel( def->cachedDynamicModel );
			}

			if ( r_checkBounds.GetBool() ) {
				idBounds b = def->cachedDynamicModel->Bounds();
				if (	b[0][0] < def->referenceBounds[0][0] - CHECK_BOUNDS_EPSILON ||
						b[0][1] < def->referenceBounds[0][1] - CHECK_BOUNDS_EPSILON ||
						b[0][2] < def->referenceBounds[0][2] - CHECK_BOUNDS_EPSILON ||
						b[1][0] > def->referenceBounds[1][0] + CHECK_BOUNDS_EPSILON ||
						b[1][1] > def->referenceBounds[1][1] + CHECK_BOUNDS_EPSILON ||
						b[1][2] > def->referenceBounds[1][2] + CHECK_BOUNDS_EPSILON ) {
					common->Printf( "entity %i dynamic model exceeded reference bounds\n", def->index );
				}
			}
		}
		def->dynamicModel = def->cachedDynamicModel;
		def->dynamicModelFrameCount = tr.frameCount;
	}

	// set model depth hack value
	if ( def->dynamicModel && model->DepthHack() != 0.0f && tr.viewDef ) {
		idPlane eye, clip;
		idVec3 ndc;
		R_TransformModelToClip( def->parms.origin, tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );
		R_TransformClipToDevice( clip, tr.viewDef, ndc );
		def->parms.modelDepthHack = model->DepthHack() * ( 1.0f - ndc.z );
	}

	// FIXME: if any of the surfaces have deforms, create a frame-temporary model with references to the
	// undeformed surfaces.  This would allow deforms to be light interacting.
	return def->dynamicModel;
}

void R_AddSurfaceToView( drawSurf_t *drawSurf ) {
	// bumping this offset each time causes surfaces with equal sort orders to still
	// deterministically draw in the order they are added
	drawSurf->sort += tr.sortOffset;
	tr.sortOffset += 0.000001f;
	// if it doesn't fit, resize the list
	if ( tr.viewDef->numDrawSurfs == tr.viewDef->maxDrawSurfs ) {
		drawSurf_t	**old = tr.viewDef->drawSurfs;
		int			count;

		if ( tr.viewDef->maxDrawSurfs == 0 ) {
			tr.viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
			count = 0;
		} else {
			count = tr.viewDef->maxDrawSurfs * sizeof( tr.viewDef->drawSurfs[0] );
			tr.viewDef->maxDrawSurfs *= 2;
		}
		int newSize = tr.viewDef->maxDrawSurfs * sizeof( tr.viewDef->drawSurfs[0] );
		tr.viewDef->drawSurfs = (drawSurf_t **)R_FrameAlloc( newSize );
		//memset( tr.viewDef->drawSurfs, -1, newSize );
		memcpy( tr.viewDef->drawSurfs, old, count );
	}
	tr.viewDef->drawSurfs[tr.viewDef->numDrawSurfs] = drawSurf;
	tr.viewDef->numDrawSurfs++;
}

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf( const srfTriangles_t *tri, const viewEntity_t *space, const renderEntity_t *renderEntity,
					const idMaterial *material, const idScreenRect &scissor, const float soft_particle_radius, bool deferred )
{
	drawSurf_t		*drawSurf;
	const float		*shaderParms;
	float			generatedShaderParms[MAX_ENTITY_SHADER_PARMS];

	drawSurf = (drawSurf_t *)R_FrameAlloc( sizeof( *drawSurf ) );
	drawSurf->CopyGeo( tri );
	drawSurf->space = space;
	drawSurf->material = material;
	drawSurf->dynamicImageOverride = nullptr;
	drawSurf->scissorRect = scissor;
	drawSurf->sort = material->GetSort();
	drawSurf->dsFlags = 0;
	if ( soft_particle_radius != -1.0f ) {	// #3878
		drawSurf->dsFlags |= DSF_SOFT_PARTICLE;
		drawSurf->particle_radius = soft_particle_radius;
	} else {
		drawSurf->particle_radius = 0.0f;
	}
	if ( auto eDef = space->entityDef ) {
		if ( eDef->parms.sortOffset )
			drawSurf->sort += eDef->parms.sortOffset;
	}

	if (!deferred) {
		R_AddSurfaceToView( drawSurf );
	}

	// process the shader expressions for conditionals / color / texcoords
	const float	*constRegs = material->ConstantRegisters();
	if ( constRegs ) {
		// shader only uses constant values
		drawSurf->shaderRegisters = constRegs;
	} else {
		float *regs = (float *)R_FrameAlloc( material->GetNumRegisters() * sizeof( float ) );
		drawSurf->shaderRegisters = regs;

		// a reference shader will take the calculated stage color value from another shader
		// and use that for the parm0-parm3 of the current shader, which allows a stage of
		// a light model and light flares to pick up different flashing tables from
		// different light shaders
		if ( renderEntity->referenceShader ) {
			// evaluate the reference shader to find our shader parms
			const shaderStage_t *pStage;

			float *refRegs = (float *)R_FrameAlloc( renderEntity->referenceShader->GetNumRegisters() * sizeof( float ) );
			renderEntity->referenceShader->EvaluateRegisters( refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->referenceSound );
			pStage = renderEntity->referenceShader->GetStage(0);

			memcpy( generatedShaderParms, renderEntity->shaderParms, sizeof( generatedShaderParms ) );
			generatedShaderParms[0] = refRegs[ pStage->color.registers[0] ];
			generatedShaderParms[1] = refRegs[ pStage->color.registers[1] ];
			generatedShaderParms[2] = refRegs[ pStage->color.registers[2] ];

			shaderParms = generatedShaderParms;
		} else {
			// evaluate with the entityDef's shader parms
			shaderParms = renderEntity->shaderParms;
		}

		if ( space->entityDef && space->entityDef->parms.timeGroup ) {
			const float oldFloatTime = tr.viewDef->floatTime;
			const int oldTime = tr.viewDef->renderView.time;

			tr.viewDef->floatTime = game->GetTimeGroupTime( space->entityDef->parms.timeGroup ) * 0.001f;
			tr.viewDef->renderView.time = game->GetTimeGroupTime( space->entityDef->parms.timeGroup );

			material->EvaluateRegisters( regs, shaderParms, tr.viewDef, renderEntity->referenceSound );

			tr.viewDef->floatTime = oldFloatTime;
			tr.viewDef->renderView.time = oldTime;
		} else {
			material->EvaluateRegisters( regs, shaderParms, tr.viewDef, renderEntity->referenceSound );
		}
	}

	// check for deformations
	R_DeformDrawSurf( drawSurf );

	// check for gui surfaces
	idUserInterface	*gui = NULL;

	if ( !space->entityDef ) {
		gui = material->GlobalGui();
	} else {
		int guiNum = material->GetEntityGui() - 1;
		if ( guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI ) {
			gui = renderEntity->gui[ guiNum ];
		}
		if ( !gui ) {
			gui = material->GlobalGui();
		}
	}

	if ( gui ) {
		idBounds ndcBounds;

		if ( !R_PreciseCullSurface( drawSurf, ndcBounds ) ) {
			if (!deferred) {
				R_RenderGuiSurf( gui, drawSurf );
			}
		} else {
			gui = nullptr;
		}
	}

	if (deferred) {
		preparedSurf_t *preparedSurf = (preparedSurf_t*)R_FrameAlloc( sizeof(preparedSurf_t) );
		preparedSurf->surf = drawSurf;
		preparedSurf->gui = gui;
		preparedSurf->next = space->preparedSurfs;
		const_cast<viewEntity_t *>(space)->preparedSurfs = preparedSurf;
	}

	// we can't add subviews at this point, because that would
	// increment tr.viewCount, messing up the rest of the surface
	// adds for this view
}

/*
============================
R_ShadowBounds

Even though the extruded shadows are drawn projected to infinity, their effects are limited
to a fraction of the light's volume.  An extruded box would require 12 faces to specify and
be a lot of trouble, but an axial bounding box is quick and easy to determine.

If the light is completely contained in the view, there is no value in trying to cull the
shadows, as they will all pass.

Pure function.
============================
*/
void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds )
{
	for ( int i = 0; i < 3; i++ )
	{
		shadowBounds[0][i] = ( modelBounds[0][i] >= lightOrigin[i] ? modelBounds[0][i] : lightBounds[0][i] );
		shadowBounds[1][i] = ( lightOrigin[i] >= modelBounds[1][i] ? modelBounds[1][i] : lightBounds[1][i] );
	}
}

/*
===============
R_AddAmbientDrawsurfs

Adds surfaces for the given viewEntity
Walks through the viewEntitys list and creates drawSurf_t for each surface of
each viewEntity that has a non-empty scissorRect
===============
*/
static void R_AddAmbientDrawsurfs( viewEntity_t *vEntity ) {
	int					i, total;
	srfTriangles_t		*tri;
	idRenderModel		*model;
	const idMaterial	*shader;

	idRenderEntityLocal &def = *vEntity->entityDef;
	if ( def.dynamicModel ) 
		model = def.dynamicModel;
	else
		model = def.parms.hModel;
	
	// add all the surfaces
	total = model->NumSurfaces();
	for ( i = 0 ; i < total ; i++ ) {
		const modelSurface_t *surf = model->Surface( i );

		// for debugging, only show a single surface at a time
		if ( r_singleSurface.GetInteger() >= 0 && i != r_singleSurface.GetInteger() ) {
			continue;
		}
		tri = surf->geometry;

		if ( !tri ) {
			continue;
		}

		if ( !tri->numIndexes ) {
			continue;
		}
		shader = surf->material;
		shader = R_RemapShaderBySkin( shader, def.parms.customSkin, def.parms.customShader );

		R_GlobalShaderOverride( &shader );

		if ( !shader ) {
			continue;
		}

		if ( !shader->IsDrawn() ) {
			continue;
		}

		// Don't put worldspawn particle textures (weather patches, mostly) on the drawSurf list for non-visible 
		// views (in TDM, the light gem render). Important to block it before their verts are calculated -- SteveL #3970
		if ( tr.viewDef->IsLightGem() && ( shader->Deform() == DFRM_PARTICLE || shader->Deform() == DFRM_PARTICLE2 ) ) {
			continue;
		}

		// debugging tool to make sure we are have the correct pre-calculated bounds
		if ( r_checkBounds.GetBool() ) {
			int j, k;
			for ( j = 0 ; j < tri->numVerts ; j++ ) {
				for ( k = 0 ; k < 3 ; k++ ) {
					if ( tri->verts[j].xyz[k] > tri->bounds[1][k] + CHECK_BOUNDS_EPSILON || 
						 tri->verts[j].xyz[k] < tri->bounds[0][k] - CHECK_BOUNDS_EPSILON ) {
						 common->Printf( "bad tri->bounds on %s:%s\n", def.parms.hModel->Name(), shader->GetName() );
						 break;
					}
					if ( tri->verts[j].xyz[k] > def.referenceBounds[1][k] + CHECK_BOUNDS_EPSILON || 
						 tri->verts[j].xyz[k] < def.referenceBounds[0][k] - CHECK_BOUNDS_EPSILON ) {
						 common->Printf( "bad referenceBounds on %s:%s\n", def.parms.hModel->Name(), shader->GetName() );
						 break;
					}
				}

				if ( k != 3 ) {
					break;
				}
			}
		}

		if ( !R_CullLocalBox( tri->bounds, vEntity->modelMatrix, 5, tr.viewDef->frustum ) ) {

			// #4946: try to cull transparent objects behind mirrors, that are ignored by clip plane during depth pass
			// maybe save a couple draw calls for solid objects too
			if ( r_useClipPlaneCulling && tr.viewDef->numClipPlanes ) {
				// function assumes positive orientation outside frustum
				idPlane plane = -tr.viewDef->clipPlane[0];
				if ( R_CullLocalBox( tri->bounds, vEntity->modelMatrix, 1, &plane ) ) {
					continue;
				}
			}

			def.visibleCount = tr.viewCount;

			// make sure we have an ambient cache
			if ( !R_CreateAmbientCache( tri, shader->ReceivesLighting() ) )
				// don't add anything if the vertex cache was too full to give us an ambient cache
				return;

			if ( !vertexCache.CacheIsCurrent( tri->indexCache ) ) {
				tri->indexCache = vertexCache.AllocIndex( tri->indexes, tri->numIndexes * sizeof( tri->indexes[0] ) );
			}

			// Soft Particles -- SteveL #3878
			float particle_radius = -1.0f;		// Default = disallow softening, but allow modelDepthHack if specified in the decl.
			if ( r_useSoftParticles.GetBool() && 
				!shader->ReceivesLighting()	&&	// don't soften surfaces that are meant to be solid
				!tr.viewDef->IsLightGem() )	{	// Skip during "invisible" rendering passes (e.g. lightgem)
				const idRenderModelPrt* prt = dynamic_cast<const idRenderModelPrt*>( def.parms.hModel );
				if ( prt ) {
					particle_radius = prt->SofteningRadius( surf->id );
				}
			}

			// add the surface for drawing
			R_AddDrawSurf( tri, vEntity, &vEntity->entityDef->parms, shader, vEntity->scissorRect, particle_radius, true );

			// ambientViewCount is used to allow light interactions to be rejected
			// if the ambient surface isn't visible at all
			tri->ambientViewCount = tr.viewCount;
		}
	}

	// add the lightweight decal surfaces
	for ( idRenderModelDecal *decal = def.decals; decal; decal = decal->Next() ) {
		decal->AddDecalDrawSurf( vEntity );
	}
}

/*
==================
R_CalcEntityScissorRectangle
==================
*/
idScreenRect R_CalcEntityScissorRectangle( viewEntity_t *vEntity ) {
	idBounds bounds;
	idRenderEntityLocal *def = vEntity->entityDef;

	idScreenRect rect;
	if ( tr.viewDef->viewFrustum.ProjectionBounds( idBox( def->referenceBounds, def->parms.origin, def->parms.axis ), bounds ) )
		rect = R_ScreenRectFromViewFrustumBounds( bounds );
	else
		rect.ClearWithZ();

	return rect;
}

bool R_CullXray( idRenderEntityLocal& def ) {
	int entXrayMask = def.parms.xrayIndex;
	switch ( tr.viewDef->xrayEntityMask ) {
	case XR_ONLY:
		return !( entXrayMask & 2 );					// only substitutes show
	case XR_SUBSTITUTE:
	{
		if ( tr.viewDef->subviewSurface ) {
			auto viewEnt = tr.viewDef->subviewSurface->space;
			if ( viewEnt->entityDef->index == def.index )	// this would overlap everything else
				return true;
		}
		return entXrayMask & 4;							// substitutes show instead of their counterparts, everything else as usual
	}
	case XR_IGNORE:
	default:
		return entXrayMask && !(entXrayMask & 5);		// only regulars (including having a substitute), xrayIndex = 0 for non-spawned entities
	}
}

void R_AddSingleModel( viewEntity_t *vEntity ) {
	idInteraction* inter, * next;
	idRenderModel* model;

	idRenderEntityLocal& def = *vEntity->entityDef;
	TRACE_CPU_SCOPE_TEXT( "R_AddSingleModel", GetTraceLabel(def.parms) )
		
	if ( ( r_skipModels.GetInteger() == 1 || tr.viewDef->areaNum < 0 ) && ( def.dynamicModel || def.cachedDynamicModel ) ) { // debug filters
		return;
	}

	if ( r_skipModels.GetInteger() == 2 && !( def.dynamicModel || def.cachedDynamicModel ) ) {
		return;
	}

	if ( r_skipEntities.GetBool() && !def.parms.hModel->IsStaticWorldModel() ) {
		return;
	}

	if ( r_useEntityScissors.GetBool() ) {
		// calculate the screen area covered by the entity
		idScreenRect scissorRect = R_CalcEntityScissorRectangle( vEntity );

		// intersect with the portal crossing scissor rectangle
		vEntity->scissorRect.IntersectWithZ( scissorRect );

		if ( r_showEntityScissors.GetBool() && !r_useParallelAddModels.GetBool() ) {
			R_ShowColoredScreenRect( vEntity->scissorRect, def.index );
		}
	}

	/* this time stuff is inherently not thread-safe, but apparently also not used in TDM
	float oldFloatTime = 0.0f;
	int oldTime = 0;

	game->SelectTimeGroup( def.parms.timeGroup );

	if ( def.parms.timeGroup ) {
		oldFloatTime = tr.viewDef->floatTime;
		oldTime = tr.viewDef->renderView.time;

		tr.viewDef->floatTime = game->GetTimeGroupTime( def.parms.timeGroup ) * 0.001;
		tr.viewDef->renderView.time = game->GetTimeGroupTime( def.parms.timeGroup );
	}*/

	if ( R_CullXray( def) ) 
		return;

	// Don't let particle entities re-instantiate their dynamic model during non-visible views (in TDM, the light gem render) -- SteveL #3970
	if ( tr.viewDef->IsLightGem() && dynamic_cast<const idRenderModelPrt*>( def.parms.hModel ) != NULL ) {
		return;
	}

	// add the ambient surface if it has a visible rectangle
	if ( !vEntity->scissorRect.IsEmpty() ) {
		model = R_EntityDefDynamicModel( &def );
		if ( model == NULL || model->NumSurfaces() <= 0 ) {
			/*if ( def.parms.timeGroup ) {
				tr.viewDef->floatTime = oldFloatTime;
				tr.viewDef->renderView.time = oldTime;
			}*/
			return;
		}
		R_AddAmbientDrawsurfs( vEntity );
		tr.pc.c_visibleViewEntities++;
	} else {
		tr.pc.c_shadowViewEntities++;
	}

	// all empty interactions are at the end of the list so once the
	// first is encountered all the remaining interactions are empty
	for ( inter = def.firstInteraction; inter != NULL && !inter->IsEmpty(); inter = next ) {
		next = inter->entityNext;

		// skip any lights that aren't currently visible
		// this is run after any lights that are turned off have already
		// been removed from the viewLights list, and had their viewCount cleared
		if ( inter->lightDef->viewCount != tr.viewCount ) {
			continue;
		}
		inter->AddActiveInteraction();
	}

	/*if ( def.parms.timeGroup ) {
		tr.viewDef->floatTime = oldFloatTime;
		tr.viewDef->renderView.time = oldTime;
	}*/
}

void R_AddPreparedSurfaces( viewEntity_t *vEntity ) {
	idRenderEntityLocal& def = *vEntity->entityDef;
	if ( R_CullXray( def ) )
		return;
	for (preparedSurf_t *it = vEntity->preparedSurfs; it; it = it->next) {
		R_AddSurfaceToView( it->surf );

		if (it->gui) {
			R_RenderGuiSurf( it->gui, it->surf );
		}
	}
	vEntity->preparedSurfs = nullptr;
	
	idInteraction *inter, *next;
	for ( inter = vEntity->entityDef->firstInteraction; inter != NULL && !inter->IsEmpty(); inter = next ) {
		next = inter->entityNext;
		if (inter->flagMakeEmpty) {
			inter->MakeEmpty();
		}
		inter->LinkPreparedSurfaces();
	}
}

/*
===================
R_AddModelSurfaces

Here is where dynamic models actually get instantiated, and necessary
interactions get created.  This is all done on a sort-by-model basis
to keep source data in cache (most likely L2) as any interactions and
shadows are generated, since dynamic models will typically be lit by
two or more lights.
===================
*/
void R_AddModelSurfaces( void ) {
	TRACE_CPU_SCOPE( "R_AddModelSurfaces ")
	
	// clear the ambient surface list
	tr.viewDef->numDrawSurfs = 0;
	tr.viewDef->maxDrawSurfs = 0;	// will be set to INITIAL_DRAWSURFS on R_AddDrawSurf

	if ( r_useParallelAddModels.GetBool() && r_materialOverride.GetString()[0] == '\0' ) {
		for ( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
			tr.frontEndJobList->AddJob( (jobRun_t)R_AddSingleModel, vEntity );
		}
		tr.frontEndJobList->Submit();
		tr.frontEndJobList->Wait();
	} else {
		for ( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
			R_AddSingleModel( vEntity );
		}
	}
	// actually add prepared surfaces in single-threaded mode since this can't be parallelized due to shared state
	for ( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
		R_AddPreparedSurfaces( vEntity );
	}
}

REGISTER_PARALLEL_JOB( R_AddSingleModel, "R_AddSingleModel" );

/*
=====================
R_RemoveUnecessaryViewLights
=====================
*/
void R_RemoveUnecessaryViewLights( void ) {
	TRACE_CPU_SCOPE( "R_RemoveUnnecessaryViewLights" )

	viewLight_t		*vLight;

	// go through each visible light
	int numViewLights = 0;
	for (vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next) {
		numViewLights++;
		// if the light didn't have any lit surfaces visible, there is no need to
		// draw any of the shadows.  We still keep the vLight for debugging
		// draws
		if ( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions ) {
			vLight->localShadows = NULL;
			vLight->globalShadows = NULL;
		}
		else
		{
			// stgatilov: at least some drawsurfs remain, and they are passed to backend
			vLight->lightDef->viewCountGenBackendSurfs = tr.viewCount;
		}
	}

	if ( r_useShadowSurfaceScissor.GetBool() ) {
		// shrink the light scissor rect to only intersect the surfaces that will actually be drawn.
		// This doesn't seem to actually help, perhaps because the surface scissor
		// rects aren't actually the surface, but only the portal clippings.
		for ( vLight = tr.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
			if ( vLight->volumetricDust != 0.0f )
				continue;

			idScreenRect	surfRect;
			surfRect.ClearWithZ();

			for ( const drawSurf_t *surf = vLight->globalInteractions ; surf ; surf = surf->nextOnLight ) {
				surfRect.UnionWithZ( surf->scissorRect );
			}
			for ( const drawSurf_t *surf = vLight->localInteractions ; surf ; surf = surf->nextOnLight ) {
				surfRect.UnionWithZ( surf->scissorRect );
			}

			for ( drawSurf_t *surf = vLight->localShadows ; surf ; surf = surf->nextOnLight ) {
				surf->scissorRect.IntersectWithZ( surfRect );
			}
			for ( drawSurf_t *surf = vLight->globalShadows ; surf ; surf = surf->nextOnLight ) {
				surf->scissorRect.IntersectWithZ( surfRect );
			}

			for ( const drawSurf_t *surf = vLight->translucentInteractions ; surf ; surf = surf->nextOnLight ) {
				surfRect.UnionWithZ( surf->scissorRect );
			}

			vLight->scissorRect.IntersectWithZ( surfRect );
		}
	}
	// sort the viewLights list so the largest lights come first, which will reduce the chance of GPU pipeline bubbles
	LinkedListBubbleSort( &tr.viewDef->viewLights, &viewLight_s::next, [](const viewLight_t &a, const viewLight_t &b) -> bool {
		return a.scissorRect.GetArea() > b.scissorRect.GetArea();
	});
}

/*
=====================
R_AssignShadowMapAtlasPages
=====================
*/
void R_AssignShadowMapAtlasPages( void ) {
	// ask how large atlas is
	int numAtlasTiles, tileSize;
	frameBuffers->GetShadowMapBudget( numAtlasTiles, tileSize );

	// resolutions are represented as fixed-point ratios of tile size
	// so number X means resolution: floor(tileSize * X / TILE_RATIO)
	static const int TILE_RATIO = (1 << 20);
	// give at least this resolution to every light
	// it theoretically allows:
	//   1) 1024 lights rendered simultaneously
	//   2) r_shadowMapSize can be reduced down to 32 x 32
	static const int MIN_RATIO = TILE_RATIO / 32;

	struct Candidate {
		int ratio;				// resolution ratio
		float distance;			// light origin - view origin distance
		viewLight_t *vLight;
	};
	idList<Candidate> shadowMappers;

	// collect shadow map users
	for ( viewLight_t *vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( vLight->shadows == LS_MAPS ) {
			// get unified bounding scissor for all shadow volumes
			idScreenRect shadowsScissor;
			shadowsScissor.ClearWithZ();
			for ( drawSurf_t *surf = vLight->globalShadows; surf; surf = surf->nextOnLight )
				shadowsScissor.UnionWithZ( surf->scissorRect );
			for ( drawSurf_t *surf = vLight->localShadows; surf; surf = surf->nextOnLight )
				shadowsScissor.UnionWithZ( surf->scissorRect );
			// can't be larger than scissor of the whole light
			// note: this scissor is already reduced to union of all interaction surfaces (see R_RemoveUnecessaryViewLights)
			shadowsScissor.IntersectWithZ( vLight->scissorRect );

			// get distance along view direction (from depth)
			float stretch = 1e-10f;
			if ( !shadowsScissor.IsEmpty() || shadowsScissor.zmin <= shadowsScissor.zmax ) {
				float minViewDistance = tr.viewDef->projectionRenderMatrix.DepthToZ( shadowsScissor.zmin );
				float lightFrustumRadius = vLight->maxLightDistance;
				// this is rather approximate...
				stretch = lightFrustumRadius / minViewDistance;
			}

			// stretch factor can easily be huge, e.g. when view origin is within light volume
			// we need to put it into sane limits
			float score = idMath::Fmin( stretch, 1.0f );
			assert( score > 0.0f );
			// get desired resolution ratio as power-of-two
			int ratio = (int) idMath::Ceil( TILE_RATIO * score );
			ratio = 1 << ( idMath::ILog2( ratio - 1 ) + 1 );

			float distance = ( vLight->globalLightOrigin - tr.viewDef->renderView.vieworg ).Length();

			shadowMappers.Append( { ratio, distance, vLight } );
		}
	}

	if ( shadowMappers.Num() == 0 )
		return;

	// force lower precision for shadow maps in subviews & lightgem
	int downscaleShift = 0;
	if ( tr.viewDef->renderView.viewID == VID_LIGHTGEM )
		downscaleShift = 2;		// 25 %
	else if ( tr.viewDef->renderView.viewID == VID_SUBVIEW )
		downscaleShift = 1;		// 50 %

	// postprocess resolutions, estimate total budget needed
	int64 maxTotalRatio = int64(numAtlasTiles) * TILE_RATIO * TILE_RATIO;
	int64 totalRatio = 0;
	for ( Candidate &c : shadowMappers ) {
		c.ratio >>= downscaleShift;
		c.ratio = idMath::Imax( c.ratio, MIN_RATIO );
		totalRatio += int64(c.ratio) * c.ratio;
	}

	if ( totalRatio > maxTotalRatio ) {
		// can't afford giving everyone as much resolution as they want

		// sort candidate lights by distance
		idList<Candidate*> sortedByDistance;
		for ( Candidate &c : shadowMappers )
			sortedByDistance.Append( &c );
		std::sort( sortedByDistance.begin(), sortedByDistance.end(), [](Candidate *a, Candidate *b) -> bool {
			return a->distance > b->distance;
		} );

		// downscale lights in round robin fashion, starting from the most distance ones
		for ( int phase = 0; totalRatio > maxTotalRatio; phase++ ) {
			bool hasReduced = false;

			for ( Candidate *c : sortedByDistance ) {
				int newRatio = idMath::Imax( c->ratio >> 1, MIN_RATIO );
				if ( newRatio >= c->ratio )
					continue;

				hasReduced = true;
				// update resolution and total sum
				totalRatio -= int64(c->ratio) * c->ratio;
				c->ratio = newRatio;
				totalRatio += int64(c->ratio) * c->ratio;

				if ( totalRatio <= maxTotalRatio )
					break;
			}

			// none reduced -> every light already at min resolution
			assert( hasReduced );
			if ( !hasReduced )
				break;
		}

		// now try to bump back some lights if possible
		for ( Candidate *c : sortedByDistance ) {
			int newRatio = idMath::Imin( c->ratio << 1, TILE_RATIO );
			if ( newRatio <= c->ratio )
				continue;

			int64 newTotalRatio = totalRatio - int64(c->ratio) * c->ratio + int64(newRatio) * newRatio;
			if ( newTotalRatio <= maxTotalRatio ) {
				c->ratio = newRatio;
				totalRatio = newTotalRatio;
			}
		}
	}

	// let framebuffer management assign pages inside texture
	idList<int> ratios;
	for ( Candidate &c : shadowMappers )
		ratios.Append( c.ratio );

	idList<renderCrop_t> pages = frameBuffers->CreateShadowMapPages( ratios, TILE_RATIO );

	for ( int i = 0; i < pages.Num(); i++ )
		shadowMappers[i].vLight->shadowMapPage = pages[i];
}
