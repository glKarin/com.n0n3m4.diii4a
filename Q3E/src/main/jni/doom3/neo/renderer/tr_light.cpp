/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

static const float CHECK_BOUNDS_EPSILON = 1.0f;


/*
===========================================================================================

VERTEX CACHE GENERATORS

===========================================================================================
*/

#ifdef _HUMANHEAD
static ID_INLINE float R_CalcViewAndEntityDistance(const viewDef_t *viewDef, const renderEntity_t *entity)
{
	//idVec3 origin = idVec3(space->modelMatrix[12], space->modelMatrix[13], space->modelMatrix[14]);
	return (viewDef->renderView.vieworg - entity->origin).LengthFast();
}

static ID_INLINE float R_CalcViewAndLightDistance(const viewDef_t *viewDef, const renderLight_t *light)
{
	return (viewDef->renderView.vieworg - light->origin).LengthFast();
}
#endif

/*
==================
R_CreateAmbientCache

Create it if needed
==================
*/
bool R_CreateAmbientCache(srfTriangles_t *tri, bool needsLighting)
{
	if (tri->ambientCache) {
		return true;
	}

	// we are going to use it for drawing, so make sure we have the tangents and normals
	if (needsLighting && !tri->tangentsCalculated) {
		R_DeriveTangents(tri);
	}

	vertexCache.Alloc(tri->verts, tri->numVerts * sizeof(tri->verts[0]), &tri->ambientCache);

	if (!tri->ambientCache) {
		return false;
	}

	return true;
}

/*
==================
R_CreatePrivateShadowCache

This is used only for a specific light
==================
*/
void R_CreatePrivateShadowCache(srfTriangles_t *tri)
{
	if (!tri->shadowVertexes) {
		return;
	}

	vertexCache.Alloc(tri->shadowVertexes, tri->numVerts * sizeof(*tri->shadowVertexes), &tri->shadowCache);
}

/*
==================
R_CreateVertexProgramShadowCache

This is constant for any number of lights, the vertex program
takes care of projecting the verts to infinity.
==================
*/
void R_CreateVertexProgramShadowCache(srfTriangles_t *tri)
{
	if (tri->verts == NULL) {
		return;
	}

#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP
	_DROID_ALLOC16_DEF(shadowCache_t, temp, (tri->numVerts * 2 * sizeof(shadowCache_t)));
#else
	shadowCache_t *temp = (shadowCache_t *)_alloca16(tri->numVerts * 2 * sizeof(shadowCache_t));
#endif

#if 1

	SIMDProcessor->CreateVertexProgramShadowCache(&temp->xyz, tri->verts, tri->numVerts);

#else

	int numVerts = tri->numVerts;
	const idDrawVert *verts = tri->verts;

	for (int i = 0; i < numVerts; i++) {
		const float *v = verts[i].xyz.ToFloatPtr();
		temp[i*2+0].xyz[0] = v[0];
		temp[i*2+1].xyz[0] = v[0];
		temp[i*2+0].xyz[1] = v[1];
		temp[i*2+1].xyz[1] = v[1];
		temp[i*2+0].xyz[2] = v[2];
		temp[i*2+1].xyz[2] = v[2];
		temp[i*2+0].xyz[3] = 1.0f;		// on the model surface
		temp[i*2+1].xyz[3] = 0.0f;		// will be projected to infinity
	}

#endif

	vertexCache.Alloc(temp, tri->numVerts * 2 * sizeof(shadowCache_t), &tri->shadowCache);
#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP
	_DROID_FREE(temp);
#endif
}

/*
==================
R_SkyboxTexGen
==================
*/
void R_SkyboxTexGen(drawSurf_t *surf, const idVec3 &viewOrg)
{
	int		i;
	idVec3	localViewOrigin;

	R_GlobalPointToLocal(surf->space->modelMatrix, viewOrg, localViewOrigin);

	int numVerts = surf->geo->numVerts;
	int size = numVerts * sizeof(idVec3);
	idVec3 *texCoords = (idVec3 *) _alloca16(size);

	const idDrawVert *verts = surf->geo->verts;

	for (i = 0; i < numVerts; i++) {
		texCoords[i][0] = verts[i].xyz[0] - localViewOrigin[0];
		texCoords[i][1] = verts[i].xyz[1] - localViewOrigin[1];
		texCoords[i][2] = verts[i].xyz[2] - localViewOrigin[2];
	}

	surf->dynamicTexCoords = vertexCache.AllocFrameTemp(texCoords, size);
}

/*
==================
R_WobbleskyTexGen
==================
*/
void R_WobbleskyTexGen(drawSurf_t *surf, const idVec3 &viewOrg)
{
	int		i;
	idVec3	localViewOrigin;

	const int *parms = surf->material->GetTexGenRegisters();

	float	wobbleDegrees = surf->shaderRegisters[ parms[0] ];
	float	wobbleSpeed = surf->shaderRegisters[ parms[1] ];
	float	rotateSpeed = surf->shaderRegisters[ parms[2] ];

	wobbleDegrees = wobbleDegrees * idMath::PI / 180;
	wobbleSpeed = wobbleSpeed * 2 * idMath::PI / 60;
	rotateSpeed = rotateSpeed * 2 * idMath::PI / 60;

	// very ad-hoc "wobble" transform
	float	transform[16];
	float	a = tr.viewDef->floatTime * wobbleSpeed;
	float	s = sin(a) * sin(wobbleDegrees);
	float	c = cos(a) * sin(wobbleDegrees);
	float	z = cos(wobbleDegrees);

	idVec3	axis[3];

	axis[2][0] = c;
	axis[2][1] = s;
	axis[2][2] = z;

	axis[1][0] = -sin(a * 2) * sin(wobbleDegrees);
	axis[1][2] = -s * sin(wobbleDegrees);
	axis[1][1] = sqrt(1.0f - (axis[1][0] * axis[1][0] + axis[1][2] * axis[1][2]));

	// make the second vector exactly perpendicular to the first
	axis[1] -= (axis[2] * axis[1]) * axis[2];
	axis[1].Normalize();

	// construct the third with a cross
	axis[0].Cross(axis[1], axis[2]);

	// add the rotate
	s = sin(rotateSpeed * tr.viewDef->floatTime);
	c = cos(rotateSpeed * tr.viewDef->floatTime);

	transform[0] = axis[0][0] * c + axis[1][0] * s;
	transform[4] = axis[0][1] * c + axis[1][1] * s;
	transform[8] = axis[0][2] * c + axis[1][2] * s;

	transform[1] = axis[1][0] * c - axis[0][0] * s;
	transform[5] = axis[1][1] * c - axis[0][1] * s;
	transform[9] = axis[1][2] * c - axis[0][2] * s;

	transform[2] = axis[2][0];
	transform[6] = axis[2][1];
	transform[10] = axis[2][2];

	transform[3] = transform[7] = transform[11] = 0.0f;
	transform[12] = transform[13] = transform[14] = 0.0f;

	R_GlobalPointToLocal(surf->space->modelMatrix, viewOrg, localViewOrigin);

	int numVerts = surf->geo->numVerts;
	int size = numVerts * sizeof(idVec3);
	idVec3 *texCoords = (idVec3 *) _alloca16(size);

	const idDrawVert *verts = surf->geo->verts;

	for (i = 0; i < numVerts; i++) {
		idVec3 v;

		v[0] = verts[i].xyz[0] - localViewOrigin[0];
		v[1] = verts[i].xyz[1] - localViewOrigin[1];
		v[2] = verts[i].xyz[2] - localViewOrigin[2];

		R_LocalPointToGlobal(transform, v, texCoords[i]);
	}

	surf->dynamicTexCoords = vertexCache.AllocFrameTemp(texCoords, size);
}

//=======================================================================================================

/*
=============
R_SetEntityDefViewEntity

If the entityDef isn't already on the viewEntity list, create
a viewEntity and add it to the list with an empty scissor rect.

This does not instantiate dynamic models for the entity yet.
=============
*/
viewEntity_t *R_SetEntityDefViewEntity(idRenderEntityLocal *def)
{
	viewEntity_t		*vModel;

	if (def->viewCount == tr.viewCount) {
		return def->viewEntity;
	}

	def->viewCount = tr.viewCount;

	// set the model and modelview matricies
	vModel = (viewEntity_t *)R_ClearedFrameAlloc(sizeof(*vModel));
	vModel->entityDef = def;

	// the scissorRect will be expanded as the model bounds is accepted into visible portal chains
	vModel->scissorRect.Clear();

	// copy the model and weapon depth hack for back-end use
	vModel->modelDepthHack = def->parms.modelDepthHack;

	vModel->weaponDepthHack = def->parms.weaponDepthHack;

	R_AxisToModelMatrix(def->parms.axis, def->parms.origin, vModel->modelMatrix);

	// we may not have a viewDef if we are just creating shadows at entity creation time
	if (tr.viewDef) {
		myGlMultMatrix(vModel->modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, vModel->modelViewMatrix);

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
static const float INSIDE_LIGHT_FRUSTUM_SLOP = 32;
// this needs to be greater than the dist from origin to corner of near clip plane
static bool R_TestPointInViewLight(const idVec3 &org, const idRenderLightLocal *light)
{
	int		i;
	idVec3	local;

	for (i = 0 ; i < 6 ; i++) {
		float d = light->frustum[i].Distance(org);

		if (d > INSIDE_LIGHT_FRUSTUM_SLOP) {
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
static bool R_PointInFrustum(idVec3 &p, idPlane *planes, int numPlanes)
{
	for (int i = 0 ; i < numPlanes ; i++) {
		float d = planes[i].Distance(p);

		if (d > 0) {
			return false;
		}
	}

	return true;
}

/*
=============
R_SetLightDefViewLight

If the lightDef isn't already on the viewLight list, create
a viewLight and add it to the list with an empty scissor rect.
=============
*/
viewLight_t *R_SetLightDefViewLight(idRenderLightLocal *light)
{
	viewLight_t *vLight;

	if (light->viewCount == tr.viewCount) {
		return light->viewLight;
	}

	light->viewCount = tr.viewCount;

	// add to the view light chain
	vLight = (viewLight_t *)R_ClearedFrameAlloc(sizeof(*vLight));
	vLight->lightDef = light;

	// the scissorRect will be expanded as the light bounds is accepted into visible portal chains
	vLight->scissorRect.Clear();

	// calculate the shadow cap optimization states
	vLight->viewInsideLight = R_TestPointInViewLight(tr.viewDef->renderView.vieworg, light);

	if (!vLight->viewInsideLight) {
		vLight->viewSeesShadowPlaneBits = 0;

		for (int i = 0 ; i < light->numShadowFrustums ; i++) {
			float d = light->shadowFrustums[i].planes[5].Distance(tr.viewDef->renderView.vieworg);

			if (d < INSIDE_LIGHT_FRUSTUM_SLOP) {
				vLight->viewSeesShadowPlaneBits|= 1 << i;
			}
		}
	} else {
		// this should not be referenced in this case
		vLight->viewSeesShadowPlaneBits = 63;
	}

	// see if the light center is in view, which will allow us to cull invisible shadows
	vLight->viewSeesGlobalLightOrigin = R_PointInFrustum(light->globalLightOrigin, tr.viewDef->frustum, 4);

	// copy data used by backend
	vLight->globalLightOrigin = light->globalLightOrigin;
	vLight->lightProject[0] = light->lightProject[0];
	vLight->lightProject[1] = light->lightProject[1];
	vLight->lightProject[2] = light->lightProject[2];
	vLight->lightProject[3] = light->lightProject[3];
#ifdef _D3BFG_CULLING
    if(harm_r_occlusionCulling.GetBool())
    {
        //anon begin
        // the fog plane is the light far clip plane
        idPlane fogPlane((ID_RENDER_MATRIX light->baseLightProject)[2][0] - (ID_RENDER_MATRIX light->baseLightProject)[3][0],
                         (ID_RENDER_MATRIX light->baseLightProject)[2][1] - (ID_RENDER_MATRIX light->baseLightProject)[3][1],
                         (ID_RENDER_MATRIX light->baseLightProject)[2][2] - (ID_RENDER_MATRIX light->baseLightProject)[3][2],
                         (ID_RENDER_MATRIX light->baseLightProject)[2][3] - (ID_RENDER_MATRIX light->baseLightProject)[3][3]);
        const float planeScale = idMath::InvSqrt(fogPlane.Normal().LengthSqr());
        vLight->fogPlane[0] = fogPlane[0] * planeScale;
        vLight->fogPlane[1] = fogPlane[1] * planeScale;
        vLight->fogPlane[2] = fogPlane[2] * planeScale;
        vLight->fogPlane[3] = fogPlane[3] * planeScale;
        //anon end
    }
    else
#endif
	vLight->fogPlane = light->frustum[5];
	vLight->frustumTris = light->frustumTris;
	vLight->falloffImage = light->falloffImage;
	vLight->lightShader = light->lightShader;
	vLight->shaderRegisters = NULL;		// allocated and evaluated in R_AddLightSurfaces

	// link the view light
	vLight->next = tr.viewDef->viewLights;
	tr.viewDef->viewLights = vLight;
#ifdef _SHADOW_MAPPING
    ID_RENDER_MATRIX_ASSIGN(vLight->baseLightProject, light->baseLightProject);
	ID_RENDER_MATRIX_ASSIGN(vLight->inverseBaseLightProject, light->inverseBaseLightProject);
    vLight->pointLight = light->parms.pointLight;
    vLight->parallel = light->parms.parallel;
    vLight->lightCenter = light->parms.lightCenter;
	vLight->lightRadius = light->parms.lightRadius;
	vLight->shadowLOD = 0;
	R_SetupShadowMappingLOD(light, vLight);
#endif

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
void idRenderWorldLocal::CreateLightDefInteractions(idRenderLightLocal *ldef)
{
	areaReference_t		*eref;
	areaReference_t		*lref;
	idRenderEntityLocal		*edef;
	portalArea_t	*area;
	idInteraction	*inter;

	for (lref = ldef->references ; lref ; lref = lref->ownerNext) {
		area = lref->area;

		// check all the models in this area
		for (eref = area->entityRefs.areaNext ; eref != &area->entityRefs ; eref = eref->areaNext) {
			edef = eref->entity;

			// if the entity doesn't have any light-interacting surfaces, we could skip this,
			// but we don't want to instantiate dynamic models yet, so we can't check that on
			// most things

			// if the entity isn't viewed
			if (tr.viewDef && edef->viewCount != tr.viewCount) {
				// if the light doesn't cast shadows, skip
				if (!ldef->lightShader->LightCastsShadows()) {
					continue;
				}

				// if we are suppressing its shadow in this view, skip
				if (!r_skipSuppress.GetBool()) {
					if (edef->parms.suppressShadowInViewID && edef->parms.suppressShadowInViewID == tr.viewDef->renderView.viewID) {
						continue;
					}

					if (edef->parms.suppressShadowInLightID && edef->parms.suppressShadowInLightID == ldef->parms.lightId) {
						continue;
					}
				}
			}

			// some big outdoor meshes are flagged to not create any dynamic interactions
			// when the level designer knows that nearby moving lights shouldn't actually hit them
			if (edef->parms.noDynamicInteractions && edef->world->generateAllInteractionsCalled) {
				continue;
			}

			// if any of the edef's interaction match this light, we don't
			// need to consider it.
			if (r_useInteractionTable.GetBool() && this->interactionTable) {
				// allocating these tables may take several megs on big maps, but it saves 3% to 5% of
				// the CPU time.  The table is updated at interaction::AllocAndLink() and interaction::UnlinkAndFree()
				int index = ldef->index * this->interactionTableWidth + edef->index;
				inter = this->interactionTable[ index ];

				if (inter) {
					// if this entity wasn't in view already, the scissor rect will be empty,
					// so it will only be used for shadow casting
					if (!inter->IsEmpty()) {
						R_SetEntityDefViewEntity(edef);
					}

					continue;
				}
			} else {
				// scan the doubly linked lists, which may have several dozen entries

				// we could check either model refs or light refs for matches, but it is
				// assumed that there will be less lights in an area than models
				// so the entity chains should be somewhat shorter (they tend to be fairly close).
				for (inter = edef->firstInteraction; inter != NULL; inter = inter->entityNext) {
					if (inter->lightDef == ldef) {
						break;
					}
				}

				// if we already have an interaction, we don't need to do anything
				if (inter != NULL) {
					// if this entity wasn't in view already, the scissor rect will be empty,
					// so it will only be used for shadow casting
					if (!inter->IsEmpty()) {
						R_SetEntityDefViewEntity(edef);
					}

					continue;
				}
			}

			//
			// create a new interaction, but don't do any work other than bbox to frustum culling
			//
			idInteraction *inter = idInteraction::AllocAndLink(edef, ldef);

			// do a check of the entity reference bounds against the light frustum,
			// trying to avoid creating a viewEntity if it hasn't been already
			float	modelMatrix[16];
			float	*m;

			if (edef->viewCount == tr.viewCount) {
				m = edef->viewEntity->modelMatrix;
			} else {
				R_AxisToModelMatrix(edef->parms.axis, edef->parms.origin, modelMatrix);
				m = modelMatrix;
			}

			if (R_CullLocalBox(edef->referenceBounds, m, 6, ldef->frustum)) {
				inter->MakeEmpty();
				continue;
			}

			// we will do a more precise per-surface check when we are checking the entity

			// if this entity wasn't in view already, the scissor rect will be empty,
			// so it will only be used for shadow casting
			R_SetEntityDefViewEntity(edef);
		}
	}
}

//===============================================================================================================

/*
=================
R_LinkLightSurf
=================
*/
void R_LinkLightSurf(const drawSurf_t **link, const srfTriangles_t *tri, const viewEntity_t *space,
                     const idRenderLightLocal *light, const idMaterial *shader, const idScreenRect &scissor, bool viewInsideShadow)
{
	drawSurf_t		*drawSurf;

	if (!space) {
		space = &tr.viewDef->worldSpace;
	}

	drawSurf = (drawSurf_t *)R_FrameAlloc(sizeof(*drawSurf));

#ifdef _MULTITHREAD
	drawSurf->geoOrig = tri;
	if(multithreadActive) //k: alloc a frame copy, because Model::geometry will free in next frame on CPU
	{
		srfTriangles_t *newTri = (srfTriangles_t *)R_FrameAlloc(sizeof(srfTriangles_t));
		*newTri = *tri;
		drawSurf->geo = newTri;
	}
	else
#endif
	drawSurf->geo = tri;
	drawSurf->space = space;
	drawSurf->material = shader;
	drawSurf->scissorRect = scissor;
	drawSurf->dsFlags = 0;

	if (viewInsideShadow) {
		drawSurf->dsFlags |= DSF_VIEW_INSIDE_SHADOW;
	}

	if (!shader) {
		// shadows won't have a shader
		drawSurf->shaderRegisters = NULL;
	} else {
		// process the shader expressions for conditionals / color / texcoords
		const float *constRegs = shader->ConstantRegisters();

		if (constRegs) {
			// this shader has only constants for parameters
			drawSurf->shaderRegisters = constRegs;
		} else {
			// FIXME: share with the ambient surface?
			float *regs = (float *)R_FrameAlloc(shader->GetNumRegisters() * sizeof(float));
			drawSurf->shaderRegisters = regs;
#ifdef _HUMANHEAD
			space->entityDef->parms.shaderParms[SHADERPARM_DISTANCE] = R_CalcViewAndEntityDistance(tr.viewDef, &space->entityDef->parms);
#endif
#ifdef _RAVEN //karin: quake4 using handle
			shader->EvaluateRegisters(regs, space->entityDef->parms.shaderParms, tr.viewDef, space->entityDef->parms.referenceSoundHandle);
#else
			shader->EvaluateRegisters(regs, space->entityDef->parms.shaderParms, tr.viewDef, space->entityDef->parms.referenceSound);
#endif
		}
	}

	// actually link it in
	drawSurf->nextOnLight = *link;
	*link = drawSurf;
}

/*
======================
R_ClippedLightScissorRectangle
======================
*/
idScreenRect R_ClippedLightScissorRectangle(viewLight_t *vLight)
{
	int i, j;
	const idRenderLightLocal *light = vLight->lightDef;
	idScreenRect r;
	idFixedWinding w;

	r.Clear();

	for (i = 0 ; i < 6 ; i++) {
		const idWinding *ow = light->frustumWindings[i];

		// projected lights may have one of the frustums degenerated
		if (!ow) {
			continue;
		}

		// the light frustum planes face out from the light,
		// so the planes that have the view origin on the negative
		// side will be the "back" faces of the light, which must have
		// some fragment inside the portalStack to be visible
		if (light->frustum[i].Distance(tr.viewDef->renderView.vieworg) >= 0) {
			continue;
		}

		w = *ow;

		// now check the winding against each of the frustum planes
		for (j = 0; j < 5; j++) {
			if (!w.ClipInPlace(-tr.viewDef->frustum[j])) {
				break;
			}
		}

		// project these points to the screen and add to bounds
		for (j = 0; j < w.GetNumPoints(); j++) {
			idPlane		eye, clip;
			idVec3		ndc;

			R_TransformModelToClip(w[j].ToVec3(), tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip);

			if (clip[3] <= 0.01f) {
				clip[3] = 0.01f;
			}

			R_TransformClipToDevice(clip, tr.viewDef, ndc);

			float windowX = 0.5f * (1.0f + ndc[0]) * (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1);
			float windowY = 0.5f * (1.0f + ndc[1]) * (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1);

			if (windowX > tr.viewDef->scissor.x2) {
				windowX = tr.viewDef->scissor.x2;
			} else if (windowX < tr.viewDef->scissor.x1) {
				windowX = tr.viewDef->scissor.x1;
			}

			if (windowY > tr.viewDef->scissor.y2) {
				windowY = tr.viewDef->scissor.y2;
			} else if (windowY < tr.viewDef->scissor.y1) {
				windowY = tr.viewDef->scissor.y1;
			}

			r.AddPoint(windowX, windowY);
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
int	c_clippedLight, c_unclippedLight;

idScreenRect	R_CalcLightScissorRectangle(viewLight_t *vLight)
{
	idScreenRect	r;
	srfTriangles_t *tri;
	idPlane			eye, clip;
	idVec3			ndc;

	if (vLight->lightDef->parms.pointLight) {
		idBounds bounds;
		idRenderLightLocal *lightDef = vLight->lightDef;
		tr.viewDef->viewFrustum.ProjectionBounds(idBox(lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis), bounds);
		return R_ScreenRectFromViewFrustumBounds(bounds);
	}

	if (r_useClippedLightScissors.GetInteger() == 2) {
		return R_ClippedLightScissorRectangle(vLight);
	}

	r.Clear();

	tri = vLight->lightDef->frustumTris;

	for (int i = 0 ; i < tri->numVerts ; i++) {
		R_TransformModelToClip(tri->verts[i].xyz, tr.viewDef->worldSpace.modelViewMatrix,
		                       tr.viewDef->projectionMatrix, eye, clip);

		// if it is near clipped, clip the winding polygons to the view frustum
		if (clip[3] <= 1) {
			c_clippedLight++;

			if (r_useClippedLightScissors.GetInteger()) {
				return R_ClippedLightScissorRectangle(vLight);
			} else {
				r.x1 = r.y1 = 0;
				r.x2 = (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1) - 1;
				r.y2 = (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1) - 1;
				return r;
			}
		}

		R_TransformClipToDevice(clip, tr.viewDef, ndc);

		float windowX = 0.5f * (1.0f + ndc[0]) * (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1);
		float windowY = 0.5f * (1.0f + ndc[1]) * (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1);

		if (windowX > tr.viewDef->scissor.x2) {
			windowX = tr.viewDef->scissor.x2;
		} else if (windowX < tr.viewDef->scissor.x1) {
			windowX = tr.viewDef->scissor.x1;
		}

		if (windowY > tr.viewDef->scissor.y2) {
			windowY = tr.viewDef->scissor.y2;
		} else if (windowY < tr.viewDef->scissor.y1) {
			windowY = tr.viewDef->scissor.y1;
		}

		r.AddPoint(windowX, windowY);
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
void R_AddLightSurfaces(void)
{
	viewLight_t		*vLight;
	idRenderLightLocal *light;
	viewLight_t		**ptr;

	// go through each visible light, possibly removing some from the list
	ptr = &tr.viewDef->viewLights;

	while (*ptr) {
		vLight = *ptr;
		light = vLight->lightDef;

		const idMaterial	*lightShader = light->lightShader;

		if (!lightShader) {
			common->Error("R_AddLightSurfaces: NULL lightShader");
		}

		// see if we are suppressing the light in this view
		if (!r_skipSuppress.GetBool()) {
			if (light->parms.suppressLightInViewID
			    && light->parms.suppressLightInViewID == tr.viewDef->renderView.viewID) {
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}

			if (light->parms.allowLightInViewID
			    && light->parms.allowLightInViewID != tr.viewDef->renderView.viewID) {
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
		}

		// evaluate the light shader registers
		float *lightRegs =(float *)R_FrameAlloc(lightShader->GetNumRegisters() * sizeof(float));
		vLight->shaderRegisters = lightRegs;
#ifdef _HUMANHEAD
		light->parms.shaderParms[SHADERPARM_DISTANCE] = R_CalcViewAndLightDistance(tr.viewDef, &light->parms);
#endif
#ifdef _RAVEN //karin: quake4 using handle
		lightShader->EvaluateRegisters(lightRegs, light->parms.shaderParms, tr.viewDef, light->parms.referenceSoundHandle);
#else
		lightShader->EvaluateRegisters(lightRegs, light->parms.shaderParms, tr.viewDef, light->parms.referenceSound);
#endif

		// if this is a purely additive light and no stage in the light shader evaluates
		// to a positive light value, we can completely skip the light
		if (!lightShader->IsFogLight() && !lightShader->IsBlendLight()) {
			int lightStageNum;

			for (lightStageNum = 0 ; lightStageNum < lightShader->GetNumStages() ; lightStageNum++) {
				const shaderStage_t	*lightStage = lightShader->GetStage(lightStageNum);

				// ignore stages that fail the condition
				if (!lightRegs[ lightStage->conditionRegister ]) {
					continue;
				}

				const int *registers = lightStage->color.registers;

				// snap tiny values to zero to avoid lights showing up with the wrong color
				if (lightRegs[ registers[0] ] < 0.001f) {
					lightRegs[ registers[0] ] = 0.0f;
				}

				if (lightRegs[ registers[1] ] < 0.001f) {
					lightRegs[ registers[1] ] = 0.0f;
				}

				if (lightRegs[ registers[2] ] < 0.001f) {
					lightRegs[ registers[2] ] = 0.0f;
				}

				// FIXME:	when using the following values the light shows up bright red when using nvidia drivers/hardware
				//			this seems to have been fixed ?
				//lightRegs[ registers[0] ] = 1.5143074e-005f;
				//lightRegs[ registers[1] ] = 1.5483369e-005f;
				//lightRegs[ registers[2] ] = 1.7014690e-005f;

				if (lightRegs[ registers[0] ] > 0.0f ||
				    lightRegs[ registers[1] ] > 0.0f ||
				    lightRegs[ registers[2] ] > 0.0f) {
					break;
				}
			}

			if (lightStageNum == lightShader->GetNumStages()) {
				// we went through all the stages and didn't find one that adds anything
				// remove the light from the viewLights list, and change its frame marker
				// so interaction generation doesn't think the light is visible and
				// create a shadow for it
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
		}

		if (r_useLightScissors.GetBool()) {
			// calculate the screen area covered by the light frustum
			// which will be used to crop the stencil cull
			idScreenRect scissorRect = R_CalcLightScissorRectangle(vLight);
			// intersect with the portal crossing scissor rectangle
			vLight->scissorRect.Intersect(scissorRect);

			if (r_showLightScissors.GetBool()) {
				R_ShowColoredScreenRect(vLight->scissorRect, light->index);
			}
		}

#if 0

		// this never happens, because CullLightByPortals() does a more precise job
		if (vLight->scissorRect.IsEmpty()) {
			// this light doesn't touch anything on screen, so remove it from the list
			*ptr = vLight->next;
			continue;
		}

#endif

		// this one stays on the list
		ptr = &vLight->next;

		// if we are doing a soft-shadow novelty test, regenerate the light with
		// a random offset every time
		if (r_lightSourceRadius.GetFloat() != 0.0f) {
			for (int i = 0 ; i < 3 ; i++) {
				light->globalLightOrigin[i] += r_lightSourceRadius.GetFloat() * (-1 + 2 * (rand()&0xfff)/(float)0xfff);
			}
		}

		// create interactions with all entities the light may touch, and add viewEntities
		// that may cast shadows, even if they aren't directly visible.  Any real work
		// will be deferred until we walk through the viewEntities
		tr.viewDef->renderWorld->CreateLightDefInteractions(light);
		tr.pc.c_viewLights++;

		// fog lights will need to draw the light frustum triangles, so make sure they
		// are in the vertex cache
		if (lightShader->IsFogLight()) {
			if (!light->frustumTris->ambientCache) {
				if (!R_CreateAmbientCache(light->frustumTris, false)) {
					// skip if we are out of vertex memory
					continue;
				}
			}

			// touch the surface so it won't get purged
			vertexCache.Touch(light->frustumTris->ambientCache);
		}

		// add the prelight shadows for the static world geometry
		if (light->parms.prelightModel && r_useOptimizedShadows.GetBool()) {

			if (!light->parms.prelightModel->NumSurfaces()) {
				common->Error("no surfs in prelight model '%s'", light->parms.prelightModel->Name());
			}

			srfTriangles_t	*tri = light->parms.prelightModel->Surface(0)->geometry;

			if (!tri->shadowVertexes) {
				common->Error("R_AddLightSurfaces: prelight model '%s' without shadowVertexes", light->parms.prelightModel->Name());
			}

			// these shadows will all have valid bounds, and can be culled normally
			if (r_useShadowCulling.GetBool()) {
				if (R_CullLocalBox(tri->bounds, tr.viewDef->worldSpace.modelMatrix, 5, tr.viewDef->frustum)) {
					continue;
				}
			}

			// if we have been purged, re-upload the shadowVertexes
			if (!tri->shadowCache) {
				R_CreatePrivateShadowCache(tri);

				if (!tri->shadowCache) {
					continue;
				}
			}

			// touch the shadow surface so it won't get purged
			vertexCache.Touch(tri->shadowCache);

			if (!tri->indexCache) {
				vertexCache.Alloc(tri->indexes, tri->numIndexes * sizeof(tri->indexes[0]), &tri->indexCache, true);
			}

			if (tri->indexCache) {
				vertexCache.Touch(tri->indexCache);
			}

			R_LinkLightSurf(&vLight->globalShadows, tri, NULL, light, NULL, vLight->scissorRect, true /* FIXME? */);
		}
	}
}

//===============================================================================================================

/*
==================
R_IssueEntityDefCallback
==================
*/
bool R_IssueEntityDefCallback(idRenderEntityLocal *def)
{
	bool update;
	idBounds	oldBounds;

	if (r_checkBounds.GetBool()) {
		oldBounds = def->referenceBounds;
	}

	def->archived = false;		// will need to be written to the demo file
	tr.pc.c_entityDefCallbacks++;

	if (tr.viewDef) {
		update = def->parms.callback(&def->parms, &tr.viewDef->renderView);
	} else {
		update = def->parms.callback(&def->parms, NULL);
	}

	if (!def->parms.hModel) {
		common->Error("R_IssueEntityDefCallback: dynamic entity callback didn't set model");
        return false;
	}

	if (r_checkBounds.GetBool()) {
		if (oldBounds[0][0] > def->referenceBounds[0][0] + CHECK_BOUNDS_EPSILON ||
		    oldBounds[0][1] > def->referenceBounds[0][1] + CHECK_BOUNDS_EPSILON ||
		    oldBounds[0][2] > def->referenceBounds[0][2] + CHECK_BOUNDS_EPSILON ||
		    oldBounds[1][0] < def->referenceBounds[1][0] - CHECK_BOUNDS_EPSILON ||
		    oldBounds[1][1] < def->referenceBounds[1][1] - CHECK_BOUNDS_EPSILON ||
		    oldBounds[1][2] < def->referenceBounds[1][2] - CHECK_BOUNDS_EPSILON) {
			common->Printf("entity %i callback extended reference bounds\n", def->index);
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
idRenderModel *R_EntityDefDynamicModel(idRenderEntityLocal *def)
{
	bool callbackUpdate;

	// allow deferred entities to construct themselves
	if (def->parms.callback) {
		callbackUpdate = R_IssueEntityDefCallback(def);
	} else {
		callbackUpdate = false;
	}

	idRenderModel *model = def->parms.hModel;

	if (!model) {
		common->Error("R_EntityDefDynamicModel: NULL model");
	}

	if (model->IsDynamicModel() == DM_STATIC) {
		def->dynamicModel = NULL;
		def->dynamicModelFrameCount = 0;
		return model;
	}

	// continously animating models (particle systems, etc) will have their snapshot updated every single view
	if (callbackUpdate || (model->IsDynamicModel() == DM_CONTINUOUS && def->dynamicModelFrameCount != tr.frameCount)) {
		R_ClearEntityDefDynamicModel(def);
	}

	// if we don't have a snapshot of the dynamic model, generate it now
	if (!def->dynamicModel) {

		// instantiate the snapshot of the dynamic model, possibly reusing memory from the cached snapshot
		def->cachedDynamicModel = model->InstantiateDynamicModel(&def->parms, tr.viewDef, def->cachedDynamicModel);

		if (def->cachedDynamicModel) {

			// add any overlays to the snapshot of the dynamic model
			if (def->overlay && !r_skipOverlays.GetBool()) {
				def->overlay->AddOverlaySurfacesToModel(def->cachedDynamicModel);
			} else {
				idRenderModelOverlay::RemoveOverlaySurfacesFromModel(def->cachedDynamicModel);
			}

			if (r_checkBounds.GetBool()) {
				idBounds b = def->cachedDynamicModel->Bounds();

				if (b[0][0] < def->referenceBounds[0][0] - CHECK_BOUNDS_EPSILON ||
				    b[0][1] < def->referenceBounds[0][1] - CHECK_BOUNDS_EPSILON ||
				    b[0][2] < def->referenceBounds[0][2] - CHECK_BOUNDS_EPSILON ||
				    b[1][0] > def->referenceBounds[1][0] + CHECK_BOUNDS_EPSILON ||
				    b[1][1] > def->referenceBounds[1][1] + CHECK_BOUNDS_EPSILON ||
				    b[1][2] > def->referenceBounds[1][2] + CHECK_BOUNDS_EPSILON) {
					common->Printf("entity %i dynamic model exceeded reference bounds\n", def->index);
				}
			}
		}

		def->dynamicModel = def->cachedDynamicModel;
		def->dynamicModelFrameCount = tr.frameCount;
	}

	// set model depth hack value
	if (def->dynamicModel && model->DepthHack() != 0.0f && tr.viewDef) {
		idPlane eye, clip;
		idVec3 ndc;
		R_TransformModelToClip(def->parms.origin, tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip);
		R_TransformClipToDevice(clip, tr.viewDef, ndc);
		def->parms.modelDepthHack = model->DepthHack() * (1.0f - ndc.z);
	}

	// FIXME: if any of the surfaces have deforms, create a frame-temporary model with references to the
	// undeformed surfaces.  This would allow deforms to be light interacting.

	return def->dynamicModel;
}

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf(const srfTriangles_t *tri, const viewEntity_t *space, const renderEntity_t *renderEntity,
                   const idMaterial *shader, const idScreenRect &scissor)
{
	drawSurf_t		*drawSurf;
	const float		*shaderParms;
	static float	refRegs[MAX_EXPRESSION_REGISTERS];	// don't put on stack, or VC++ will do a page touch
	float			generatedShaderParms[MAX_ENTITY_SHADER_PARMS];

	drawSurf = (drawSurf_t *)R_FrameAlloc(sizeof(*drawSurf));
#ifdef _MULTITHREAD
	drawSurf->geoOrig = tri;
	if(multithreadActive) //k: alloc a frame copy, because Model::geometry will free in next frame on CPU
	{
		srfTriangles_t *newTri = (srfTriangles_t *)R_FrameAlloc(sizeof(srfTriangles_t));
		*newTri = *tri;
		drawSurf->geo = newTri;
	}
	else
#endif
	drawSurf->geo = tri;
	drawSurf->space = space;
	drawSurf->material = shader;
	drawSurf->scissorRect = scissor;
	drawSurf->sort = shader->GetSort() + tr.sortOffset;
	drawSurf->dsFlags = 0;

	// bumping this offset each time causes surfaces with equal sort orders to still
	// deterministically draw in the order they are added
	tr.sortOffset += 0.000001f;

	// if it doesn't fit, resize the list
	if (tr.viewDef->numDrawSurfs == tr.viewDef->maxDrawSurfs) {
		drawSurf_t	**old = tr.viewDef->drawSurfs;
		int			count;

		if (tr.viewDef->maxDrawSurfs == 0) {
			tr.viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
			count = 0;
		} else {
			count = tr.viewDef->maxDrawSurfs * sizeof(tr.viewDef->drawSurfs[0]);
			tr.viewDef->maxDrawSurfs *= 2;
		}

		tr.viewDef->drawSurfs = (drawSurf_t **)R_FrameAlloc(tr.viewDef->maxDrawSurfs * sizeof(tr.viewDef->drawSurfs[0]));
		memcpy(tr.viewDef->drawSurfs, old, count);
	}

	tr.viewDef->drawSurfs[tr.viewDef->numDrawSurfs] = drawSurf;
	tr.viewDef->numDrawSurfs++;

	// process the shader expressions for conditionals / color / texcoords
	const float	*constRegs = shader->ConstantRegisters();

	if (constRegs) {
		// shader only uses constant values
		drawSurf->shaderRegisters = constRegs;
	} else {
		float *regs = (float *)R_FrameAlloc(shader->GetNumRegisters() * sizeof(float));
		drawSurf->shaderRegisters = regs;

		// a reference shader will take the calculated stage color value from another shader
		// and use that for the parm0-parm3 of the current shader, which allows a stage of
		// a light model and light flares to pick up different flashing tables from
		// different light shaders
		if (renderEntity->referenceShader) {
			// evaluate the reference shader to find our shader parms
			const shaderStage_t *pStage;

#ifdef _RAVEN //karin: quake4 using handle
			renderEntity->referenceShader->EvaluateRegisters(refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->referenceSoundHandle);
#else
			renderEntity->referenceShader->EvaluateRegisters(refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->referenceSound);
#endif
			pStage = renderEntity->referenceShader->GetStage(0);

			memcpy(generatedShaderParms, renderEntity->shaderParms, sizeof(generatedShaderParms));
			generatedShaderParms[0] = refRegs[ pStage->color.registers[0] ];
			generatedShaderParms[1] = refRegs[ pStage->color.registers[1] ];
			generatedShaderParms[2] = refRegs[ pStage->color.registers[2] ];

			shaderParms = generatedShaderParms;
		} else {
			// evaluate with the entityDef's shader parms
			shaderParms = renderEntity->shaderParms;
		}

		float oldFloatTime = 0.0f;
		int oldTime = 0.0f;

		
		if (space->entityDef && space->entityDef->parms.timeGroup) {
			oldFloatTime = tr.viewDef->floatTime;
			oldTime = tr.viewDef->renderView.time;

#ifdef _RAVEN
			tr.viewDef->floatTime = tr.viewDef->renderView.time * 0.001;
#else
			tr.viewDef->floatTime = game->GetTimeGroupTime(space->entityDef->parms.timeGroup) * 0.001;
			tr.viewDef->renderView.time = game->GetTimeGroupTime(space->entityDef->parms.timeGroup);
#endif
		}

#ifdef _HUMANHEAD
		((float *)shaderParms)[SHADERPARM_DISTANCE] = R_CalcViewAndEntityDistance(tr.viewDef, renderEntity);
#endif
#ifdef _RAVEN //karin: quake4 using handle
		shader->EvaluateRegisters(regs, shaderParms, tr.viewDef, renderEntity->referenceSoundHandle);
#else
		shader->EvaluateRegisters(regs, shaderParms, tr.viewDef, renderEntity->referenceSound);
#endif

		if (space->entityDef && space->entityDef->parms.timeGroup) {
			tr.viewDef->floatTime = oldFloatTime;
			tr.viewDef->renderView.time = oldTime;
		}
	}

	// check for deformations
	R_DeformDrawSurf(drawSurf);

	// skybox surfaces need a dynamic texgen
	switch (shader->Texgen()) {
		case TG_SKYBOX_CUBE:
			R_SkyboxTexGen(drawSurf, tr.viewDef->renderView.vieworg);
			break;
		case TG_WOBBLESKY_CUBE:
			R_WobbleskyTexGen(drawSurf, tr.viewDef->renderView.vieworg);
			break;
	}

	// check for gui surfaces
	idUserInterface	*gui = NULL;

	if (!space->entityDef) {
		gui = shader->GlobalGui();
	} else {
		int guiNum = shader->GetEntityGui() - 1;

		if (guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI) {
			gui = renderEntity->gui[ guiNum ];
		}

		if (gui == NULL) {
			gui = shader->GlobalGui();
		}
	}

	if (gui) {
		// force guis on the fast time
		float oldFloatTime;
		int oldTime;

		oldFloatTime = tr.viewDef->floatTime;
		oldTime = tr.viewDef->renderView.time;

#ifdef _RAVEN
		tr.viewDef->floatTime = tr.viewDef->renderView.time * 0.001;
#else
		tr.viewDef->floatTime = game->GetTimeGroupTime(1) * 0.001;
		tr.viewDef->renderView.time = game->GetTimeGroupTime(1);
#endif

		idBounds ndcBounds;

		if (!R_PreciseCullSurface(drawSurf, ndcBounds)) {
			// did we ever use this to forward an entity color to a gui that didn't set color?
//			memcpy( tr.guiShaderParms, shaderParms, sizeof( tr.guiShaderParms ) );
			R_RenderGuiSurf(gui, drawSurf);
		}

		tr.viewDef->floatTime = oldFloatTime;
		tr.viewDef->renderView.time = oldTime;
	}

	// we can't add subviews at this point, because that would
	// increment tr.viewCount, messing up the rest of the surface
	// adds for this view
}

/*
===============
R_AddAmbientDrawsurfs

Adds surfaces for the given viewEntity
Walks through the viewEntitys list and creates drawSurf_t for each surface of
each viewEntity that has a non-empty scissorRect
===============
*/
static void R_AddAmbientDrawsurfs(viewEntity_t *vEntity)
{
	int					i, total;
	idRenderEntityLocal	*def;
	srfTriangles_t		*tri;
	idRenderModel		*model;
	const idMaterial	*shader;

	def = vEntity->entityDef;

	if (def->dynamicModel) {
		model = def->dynamicModel;
	} else {
		model = def->parms.hModel;
	}

	// add all the surfaces
	total = model->NumSurfaces();

	for (i = 0 ; i < total ; i++) {
		const modelSurface_t	*surf = model->Surface(i);

#ifdef _RAVEN //k: for ShowSurface/HideSurface, shader mask is not 0 will skip render
		if(SUPPRESS_SURFACE_MASK_CHECK(def->parms.suppressSurfaceMask, i))
			continue;
#endif

		// for debugging, only show a single surface at a time
		if (r_singleSurface.GetInteger() >= 0 && i != r_singleSurface.GetInteger()) {
			continue;
		}

		tri = surf->geometry;

		if (!tri) {
			continue;
		}

		if (!tri->numIndexes) {
			continue;
		}

		shader = surf->shader;
		shader = R_RemapShaderBySkin(shader, def->parms.customSkin, def->parms.customShader);

		R_GlobalShaderOverride(&shader);

		if (!shader) {
			continue;
		}

		if (!shader->IsDrawn()) {
			continue;
		}

		// debugging tool to make sure we are have the correct pre-calculated bounds
		if (r_checkBounds.GetBool()) {
			int j, k;

			for (j = 0 ; j < tri->numVerts ; j++) {
				for (k = 0 ; k < 3 ; k++) {
					if (tri->verts[j].xyz[k] > tri->bounds[1][k] + CHECK_BOUNDS_EPSILON
					    || tri->verts[j].xyz[k] < tri->bounds[0][k] - CHECK_BOUNDS_EPSILON) {
						common->Printf("bad tri->bounds on %s:%s\n", def->parms.hModel->Name(), shader->GetName());
						break;
					}

					if (tri->verts[j].xyz[k] > def->referenceBounds[1][k] + CHECK_BOUNDS_EPSILON
					    || tri->verts[j].xyz[k] < def->referenceBounds[0][k] - CHECK_BOUNDS_EPSILON) {
						common->Printf("bad referenceBounds on %s:%s\n", def->parms.hModel->Name(), shader->GetName());
						break;
					}
				}

				if (k != 3) {
					break;
				}
			}
		}

		if (!R_CullLocalBox(tri->bounds, vEntity->modelMatrix, 5, tr.viewDef->frustum)) {

			def->visibleCount = tr.viewCount;

			// make sure we have an ambient cache
			if (!R_CreateAmbientCache(tri, shader->ReceivesLighting())) {
				// don't add anything if the vertex cache was too full to give us an ambient cache
				return;
			}

			// touch it so it won't get purged
			vertexCache.Touch(tri->ambientCache);

			if (!tri->indexCache) {
				vertexCache.Alloc(tri->indexes, tri->numIndexes * sizeof(tri->indexes[0]), &tri->indexCache, true);
			}

			if (tri->indexCache) {
				vertexCache.Touch(tri->indexCache);
			}

			// add the surface for drawing
			R_AddDrawSurf(tri, vEntity, &vEntity->entityDef->parms, shader, vEntity->scissorRect);

			// ambientViewCount is used to allow light interactions to be rejected
			// if the ambient surface isn't visible at all
			tri->ambientViewCount = tr.viewCount;
		}
	}

	// add the lightweight decal surfaces
	for (idRenderModelDecal *decal = def->decals; decal; decal = decal->Next()) {
		decal->AddDecalDrawSurf(vEntity);
	}
}

/*
==================
R_CalcEntityScissorRectangle
==================
*/
idScreenRect R_CalcEntityScissorRectangle(viewEntity_t *vEntity)
{
	idBounds bounds;
	idRenderEntityLocal *def = vEntity->entityDef;

	tr.viewDef->viewFrustum.ProjectionBounds(idBox(def->referenceBounds, def->parms.origin, def->parms.axis), bounds);

	return R_ScreenRectFromViewFrustumBounds(bounds);
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
void R_AddModelSurfaces(void)
{
	viewEntity_t		*vEntity;
	idInteraction		*inter, *next;
	idRenderModel		*model;

	// clear the ambient surface list
	tr.viewDef->numDrawSurfs = 0;
	tr.viewDef->maxDrawSurfs = 0;	// will be set to INITIAL_DRAWSURFS on R_AddDrawSurf

	// go through each entity that is either visible to the view, or to
	// any light that intersects the view (for shadows)
	for (vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next) {
		if (r_useEntityScissors.GetBool()) {
			// calculate the screen area covered by the entity
			idScreenRect scissorRect = R_CalcEntityScissorRectangle(vEntity);
			// intersect with the portal crossing scissor rectangle
			vEntity->scissorRect.Intersect(scissorRect);

			if (r_showEntityScissors.GetBool()) {
				R_ShowColoredScreenRect(vEntity->scissorRect, vEntity->entityDef->index);
			}
		}

		float oldFloatTime = 0.0f;
		int oldTime = 0.0f;

#if !defined(_RAVEN)
		game->SelectTimeGroup(vEntity->entityDef->parms.timeGroup);
#endif

		if (vEntity->entityDef->parms.timeGroup) {
			oldFloatTime = tr.viewDef->floatTime;
			oldTime = tr.viewDef->renderView.time;

#ifdef _RAVEN
			tr.viewDef->floatTime = tr.viewDef->renderView.time * 0.001;
#else
			tr.viewDef->floatTime = game->GetTimeGroupTime(vEntity->entityDef->parms.timeGroup) * 0.001;
			tr.viewDef->renderView.time = game->GetTimeGroupTime(vEntity->entityDef->parms.timeGroup);
#endif
		}

		if (tr.viewDef->isXraySubview && vEntity->entityDef->parms.xrayIndex == 1) {
			if (vEntity->entityDef->parms.timeGroup) {
				tr.viewDef->floatTime = oldFloatTime;
				tr.viewDef->renderView.time = oldTime;
			}

			continue;
		} else if (!tr.viewDef->isXraySubview && vEntity->entityDef->parms.xrayIndex == 2) {
			if (vEntity->entityDef->parms.timeGroup) {
				tr.viewDef->floatTime = oldFloatTime;
				tr.viewDef->renderView.time = oldTime;
			}

			continue;
		}

		// add the ambient surface if it has a visible rectangle
		if (!vEntity->scissorRect.IsEmpty()) {
			model = R_EntityDefDynamicModel(vEntity->entityDef);

			if (model == NULL || model->NumSurfaces() <= 0) {
				if (vEntity->entityDef->parms.timeGroup) {
					tr.viewDef->floatTime = oldFloatTime;
					tr.viewDef->renderView.time = oldTime;
				}

				continue;
			}

			R_AddAmbientDrawsurfs(vEntity);
			tr.pc.c_visibleViewEntities++;
		} else {
			tr.pc.c_shadowViewEntities++;
		}

		//
		// for all the entity / light interactions on this entity, add them to the view
		//
		if (tr.viewDef->isXraySubview) {
			if (vEntity->entityDef->parms.xrayIndex == 2) {
				for (inter = vEntity->entityDef->firstInteraction; inter != NULL && !inter->IsEmpty(); inter = next) {
					next = inter->entityNext;

					if (inter->lightDef->viewCount != tr.viewCount) {
						continue;
					}

					inter->AddActiveInteraction();
				}
			}
		} else {
			// all empty interactions are at the end of the list so once the
			// first is encountered all the remaining interactions are empty
			for (inter = vEntity->entityDef->firstInteraction; inter != NULL && !inter->IsEmpty(); inter = next) {
				next = inter->entityNext;

				// skip any lights that aren't currently visible
				// this is run after any lights that are turned off have already
				// been removed from the viewLights list, and had their viewCount cleared
				if (inter->lightDef->viewCount != tr.viewCount) {
					continue;
				}

				inter->AddActiveInteraction();
			}
		}

		if (vEntity->entityDef->parms.timeGroup) {
			tr.viewDef->floatTime = oldFloatTime;
			tr.viewDef->renderView.time = oldTime;
		}

	}
}

/*
=====================
R_RemoveUnecessaryViewLights
=====================
*/
void R_RemoveUnecessaryViewLights(void)
{
	viewLight_t		*vLight;

	// go through each visible light
#ifdef _D3BFG_CULLING
    int numViewLights = 0;
#endif
	for (vLight = tr.viewDef->viewLights ; vLight ; vLight = vLight->next) {
#ifdef _D3BFG_CULLING
        numViewLights++;
#endif
		// if the light didn't have any lit surfaces visible, there is no need to
		// draw any of the shadows.  We still keep the vLight for debugging
		// draws
		if (!vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions) {
			vLight->localShadows = NULL;
			vLight->globalShadows = NULL;
		}
	}

	if (r_useShadowSurfaceScissor.GetBool()) {
		// shrink the light scissor rect to only intersect the surfaces that will actually be drawn.
		// This doesn't seem to actually help, perhaps because the surface scissor
		// rects aren't actually the surface, but only the portal clippings.
		for (vLight = tr.viewDef->viewLights ; vLight ; vLight = vLight->next) {
			const drawSurf_t	*surf;
			idScreenRect	surfRect;

			if (!vLight->lightShader->LightCastsShadows()) {
				continue;
			}

			surfRect.Clear();

			for (surf = vLight->globalInteractions ; surf ; surf = surf->nextOnLight) {
				surfRect.Union(surf->scissorRect);
			}

			for (surf = vLight->localShadows ; surf ; surf = surf->nextOnLight) {
				const_cast<drawSurf_t *>(surf)->scissorRect.Intersect(surfRect);
			}

			for (surf = vLight->localInteractions ; surf ; surf = surf->nextOnLight) {
				surfRect.Union(surf->scissorRect);
			}

			for (surf = vLight->globalShadows ; surf ; surf = surf->nextOnLight) {
				const_cast<drawSurf_t *>(surf)->scissorRect.Intersect(surfRect);
			}

			for (surf = vLight->translucentInteractions ; surf ; surf = surf->nextOnLight) {
				surfRect.Union(surf->scissorRect);
			}

			vLight->scissorRect.Intersect(surfRect);
		}
	}
#ifdef _D3BFG_CULLING
    if (harm_r_occlusionCulling.GetBool()) {
        // sort the viewLights list so the largest lights come first, which will reduce
        // the chance of GPU pipeline bubbles
        struct sortLight_t
        {
            viewLight_t* 	vLight;
            int				screenArea;
            static int sort(const void* a, const void* b)
            {
                return ((sortLight_t*)a)->screenArea - ((sortLight_t*)b)->screenArea;
            }
        };
        sortLight_t* sortLights = (sortLight_t*)_alloca(sizeof(sortLight_t)* numViewLights);
        int	numSortLightsFilled = 0;
        for (viewLight_t* vLight = tr.viewDef->viewLights; vLight != NULL; vLight = vLight->next)
        {
            sortLights[numSortLightsFilled].vLight = vLight;
            sortLights[numSortLightsFilled].screenArea = vLight->scissorRect.GetArea();
            numSortLightsFilled++;
        }

        qsort(sortLights, numSortLightsFilled, sizeof(sortLights[0]), sortLight_t::sort);

        // rebuild the linked list in order
        tr.viewDef->viewLights = NULL;
        for (int i = 0; i < numSortLightsFilled; i++)
        {
            sortLights[i].vLight->next = tr.viewDef->viewLights;
            tr.viewDef->viewLights = sortLights[i].vLight;
        }
    }
#endif
}

#ifdef _RAVEN // particle

#ifdef _RAVEN_BSE
idRenderModel * R_EffectDefDynamicModel(rvRenderEffectLocal *def)
{
    rvBSEManager *v1; // ebx
    const char *v2; // eax
    idRenderModel *dynamicModel; // ecx
    idRenderModel *v5; // ecx
    float v6; // st7
    idVec3 ndc; // [esp+Ch] [ebp-2Ch] BYREF
    idPlane clip; // [esp+18h] [ebp-20h] BYREF
    idPlane eye; // [esp+28h] [ebp-10h] BYREF

    if ( !tr.viewDef || !def->effect )
        return NULL;
    v1 = bse;
    v2 = def->parms.declEffect->GetName();
    if ( bse->Filtered(v2) )
        return NULL;
    if ( def->dynamicModelFrameCount != tr.frameCount )
    {
        dynamicModel = def->dynamicModel;
        if ( dynamicModel )
            delete dynamicModel;
        def->dynamicModel = def->effect->Render(&def->parms, tr.viewDef);
        def->dynamicModelFrameCount = tr.frameCount;
    }
    v5 = def->dynamicModel;
    if ( v5 && 0.0f != v5->DepthHack() )
    {
        if ( tr.viewDef )
        {
            R_TransformModelToClip(
                    def->parms.origin,
                    tr.viewDef->worldSpace.modelViewMatrix,
                    tr.viewDef->projectionMatrix,
                    eye,
                    clip);
            R_TransformClipToDevice(clip, tr.viewDef, ndc);
            v6 = def->dynamicModel->DepthHack();
            def->parms.modelDepthHack = v6 * (1.0f - ndc.z);
        }
    }
    return def->dynamicModel;
}

static int compareEffect(const void *a, const void *b)
{
    const viewEffect_s *veA = (viewEffect_s *)a;
    const viewEffect_s *veB = (viewEffect_s *)b;
    return 0; //k??? TODO no source code for sort effects comparor
}

viewEffect_s * R_SetEffectDefViewEntity(rvRenderEffectLocal *def)
{
    viewEffect_s *v2; // esi

    if ( def->viewCount == tr.viewCount )
        return def->viewEffect;
    def->viewCount = tr.viewCount;
    v2 = (viewEffect_s *)R_ClearedFrameAlloc(sizeof(viewEffect_s));
    v2->effectDef = def;
    v2->scissorRect.Clear();
	v2->weaponDepthHack = def->parms.weaponDepthHackInViewID == tr.viewDef->renderView.viewID; // effect in world
    v2->modelDepthHack = def->parms.modelDepthHack;
    v2->weaponDepthHackInViewID = def->parms.weaponDepthHackInViewID;
    R_AxisToModelMatrix(def->parms.axis, def->parms.origin, v2->modelMatrix);
    if ( tr.viewDef )
    {
        myGlMultMatrix(v2->modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, v2->modelViewMatrix);
        idVec3 dis = v2->effectDef->parms.origin - tr.viewDef->renderView.vieworg;
        v2->distanceToCamera = dis.LengthSqr();
        v2->next = tr.viewDef->viewEffects;
        tr.viewDef->viewEffects = v2;
    }
    def->viewEffect = v2;
    return v2;
}

int R_SortViewEffects(viewEffect_s ***array)
{
    viewEffect_s *viewEffects; // eax
    int i; // esi
    viewEffect_s **v3; // eax
    viewEffect_s **v4; // ecx
    viewEffect_s *j; // eax

    viewEffects = tr.viewDef->viewEffects;
    if(!viewEffects)
        return 0;
    int size = sizeof(viewEffect_s *);
    for ( i = 0; viewEffects; ++i )
        viewEffects = viewEffects->next;
    v3 = (viewEffect_s **)R_FrameAlloc(size * i);
    *array = v3;
    v4 = v3;
    for ( j = tr.viewDef->viewEffects; j; ++v4 )
    {
        *v4 = j;
        j = j->next;
    }
    //qsort(*array, i, size, compareEffect); //k??? TODO no sort effects
    return i;
}

void R_AddDrawSurf(const srfTriangles_t *tri, const viewEffect_s *space, const renderEffect_s *renderEntity,
                   const idMaterial *shader, const idScreenRect &scissor)
{
    drawSurf_t		*drawSurf;
    const float		*shaderParms;
    static float	refRegs[MAX_EXPRESSION_REGISTERS];	// don't put on stack, or VC++ will do a page touch
    //float			generatedShaderParms[MAX_ENTITY_SHADER_PARMS];

    drawSurf = (drawSurf_t *)R_FrameAlloc(sizeof(*drawSurf));
#ifdef _MULTITHREAD
    drawSurf->geoOrig = tri;
    if(multithreadActive) //k: alloc a frame copy, because Model::geometry will free in next frame on CPU
    {
        srfTriangles_t *newTri = (srfTriangles_t *)R_FrameAlloc(sizeof(srfTriangles_t));
        *newTri = *tri;
        drawSurf->geo = newTri;
    }
    else
#endif
        drawSurf->geo = tri;
#if 1 // create viewEntity_s from viewEffect_s
    viewEntity_t *espace = (viewEntity_t *)R_ClearedFrameAlloc(sizeof(*espace));
	espace->scissorRect = space->scissorRect;
	espace->weaponDepthHack = space->weaponDepthHack;
	espace->modelDepthHack = space->modelDepthHack;
	memcpy(&espace->modelMatrix[0], &space->modelMatrix[0], sizeof(espace->modelMatrix));
	memcpy(&espace->modelViewMatrix[0], &space->modelViewMatrix[0], sizeof(espace->modelViewMatrix));

    espace->entityDef = (idRenderEntityLocal *)R_ClearedFrameAlloc(sizeof(*espace->entityDef));
    *espace->entityDef = idRenderEntityLocal();
    memcpy(&espace->entityDef->modelMatrix[0], &space->effectDef->modelMatrix[0], sizeof(espace->entityDef->modelMatrix));
    espace->entityDef->world = space->effectDef->world;
    espace->entityDef->index = space->effectDef->index;
    espace->entityDef->lastModifiedFrameNum = space->effectDef->lastModifiedFrameNum;
    espace->entityDef->archived = space->effectDef->archived;
    espace->entityDef->dynamicModel = space->effectDef->dynamicModel;
    espace->entityDef->referenceBounds = space->effectDef->referenceBounds;
    espace->entityDef->viewCount = space->effectDef->viewCount;
    espace->entityDef->viewEntity = espace;
    espace->entityDef->visibleCount = space->effectDef->visibleCount;
    espace->entityDef->entityRefs = space->effectDef->effectRefs;

	espace->entityDef->parms.suppressSurfaceInViewID = space->effectDef->parms.suppressSurfaceInViewID;
	espace->entityDef->parms.allowSurfaceInViewID = space->effectDef->parms.allowSurfaceInViewID;
    espace->entityDef->parms.origin = space->effectDef->parms.origin;
    espace->entityDef->parms.axis = space->effectDef->parms.axis;
    espace->entityDef->parms.referenceSoundHandle = space->effectDef->parms.referenceSoundHandle;
    espace->entityDef->parms.weaponDepthHackInViewID = space->effectDef->parms.weaponDepthHackInViewID;
    espace->entityDef->parms.modelDepthHack = space->effectDef->parms.modelDepthHack;
    espace->entityDef->parms.weaponDepthHack = space->effectDef->parms.weaponDepthHackInViewID == tr.viewDef->renderView.viewID;
    memcpy(espace->entityDef->parms.shaderParms, space->effectDef->parms.shaderParms, sizeof(espace->entityDef->parms.shaderParms));
#endif
    drawSurf->space = (viewEntity_s *)espace;
    drawSurf->material = shader;
    drawSurf->scissorRect = scissor;
    drawSurf->sort = shader->GetSort() + tr.sortOffset;
    drawSurf->dsFlags = 0;

    // bumping this offset each time causes surfaces with equal sort orders to still
    // deterministically draw in the order they are added
    tr.sortOffset += 0.000001f;

    // if it doesn't fit, resize the list
    if (tr.viewDef->numDrawSurfs == tr.viewDef->maxDrawSurfs) {
        drawSurf_t	**old = tr.viewDef->drawSurfs;
        int			count;

        if (tr.viewDef->maxDrawSurfs == 0) {
            tr.viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
            count = 0;
        } else {
            count = tr.viewDef->maxDrawSurfs * sizeof(tr.viewDef->drawSurfs[0]);
            tr.viewDef->maxDrawSurfs *= 2;
        }

        tr.viewDef->drawSurfs = (drawSurf_t **)R_FrameAlloc(tr.viewDef->maxDrawSurfs * sizeof(tr.viewDef->drawSurfs[0]));
        memcpy(tr.viewDef->drawSurfs, old, count);
    }

    tr.viewDef->drawSurfs[tr.viewDef->numDrawSurfs] = drawSurf;
    tr.viewDef->numDrawSurfs++;

    // process the shader expressions for conditionals / color / texcoords
    const float	*constRegs = shader->ConstantRegisters();

    if (constRegs) {
        // shader only uses constant values
        drawSurf->shaderRegisters = constRegs;
    } else {
        float *regs = (float *)R_FrameAlloc(shader->GetNumRegisters() * sizeof(float));
        drawSurf->shaderRegisters = regs;

        // a reference shader will take the calculated stage color value from another shader
        // and use that for the parm0-parm3 of the current shader, which allows a stage of
        // a light model and light flares to pick up different flashing tables from
        // different light shaders
#if 0
        if (renderEntity->referenceShader) {
            // evaluate the reference shader to find our shader parms
            const shaderStage_t *pStage;

#ifdef _RAVEN //karin: quake4 using handle
            renderEntity->referenceShader->EvaluateRegisters(refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->referenceSoundHandle);
#else
            renderEntity->referenceShader->EvaluateRegisters(refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->referenceSound);
#endif
            pStage = renderEntity->referenceShader->GetStage(0);

            memcpy(generatedShaderParms, renderEntity->shaderParms, sizeof(generatedShaderParms));
            generatedShaderParms[0] = refRegs[ pStage->color.registers[0] ];
            generatedShaderParms[1] = refRegs[ pStage->color.registers[1] ];
            generatedShaderParms[2] = refRegs[ pStage->color.registers[2] ];

            shaderParms = generatedShaderParms;
        } else
#endif
        {
            // evaluate with the entityDef's shader parms
            shaderParms = renderEntity->shaderParms;
        }

        float oldFloatTime = 0.0f;
        int oldTime = 0.0f;


#if 0
        if (space->entityDef && space->entityDef->parms.timeGroup) {
            oldFloatTime = tr.viewDef->floatTime;
            oldTime = tr.viewDef->renderView.time;

            tr.viewDef->floatTime = tr.viewDef->renderView.time * 0.001;
        }
#endif

        shader->EvaluateRegisters(regs, shaderParms, tr.viewDef, renderEntity->referenceSoundHandle);

#if 0
        if (space->entityDef && space->entityDef->parms.timeGroup) {
            tr.viewDef->floatTime = oldFloatTime;
            tr.viewDef->renderView.time = oldTime;
        }
#endif
    }

    // check for deformations
    R_DeformDrawSurf(drawSurf);

#if 0
    // skybox surfaces need a dynamic texgen
    switch (shader->Texgen()) {
        case TG_SKYBOX_CUBE:
            R_SkyboxTexGen(drawSurf, tr.viewDef->renderView.vieworg);
            break;
        case TG_WOBBLESKY_CUBE:
            R_WobbleskyTexGen(drawSurf, tr.viewDef->renderView.vieworg);
            break;
    }
#endif

#if 0
    // check for gui surfaces
    idUserInterface	*gui = NULL;

    if (!space->entityDef) {
        gui = shader->GlobalGui();
    } else {
        int guiNum = shader->GetEntityGui() - 1;

        if (guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI) {
            gui = renderEntity->gui[ guiNum ];
        }

        if (gui == NULL) {
            gui = shader->GlobalGui();
        }
    }

    if (gui) {
        // force guis on the fast time
        float oldFloatTime;
        int oldTime;

        oldFloatTime = tr.viewDef->floatTime;
        oldTime = tr.viewDef->renderView.time;

#ifdef _RAVEN
        tr.viewDef->floatTime = tr.viewDef->renderView.time * 0.001;
#else
        tr.viewDef->floatTime = game->GetTimeGroupTime(1) * 0.001;
		tr.viewDef->renderView.time = game->GetTimeGroupTime(1);
#endif

        idBounds ndcBounds;

        if (!R_PreciseCullSurface(drawSurf, ndcBounds)) {
            // did we ever use this to forward an entity color to a gui that didn't set color?
//			memcpy( tr.guiShaderParms, shaderParms, sizeof( tr.guiShaderParms ) );
            R_RenderGuiSurf(gui, drawSurf);
        }

        tr.viewDef->floatTime = oldFloatTime;
        tr.viewDef->renderView.time = oldTime;
    }
#endif

    // we can't add subviews at this point, because that would
    // increment tr.viewCount, messing up the rest of the surface
    // adds for this view
}

void R_AddAmbientEffectDrawsurfs(viewEffect_s *vEffect) {
    idRenderModel *dynamicModel; // ebp
    const modelSurface_t *v2; // eax
    srfTriangles_s *v4; // esi
    const idMaterial *v5; // eax
    int i; // [esp+4h] [ebp-Ch]
    rvRenderEffectLocal *def; // [esp+8h] [ebp-8h]
    int total; // [esp+Ch] [ebp-4h]

    dynamicModel = vEffect->effectDef->dynamicModel;
    def = vEffect->effectDef;
    total = dynamicModel->NumSurfaces();
    for (i = 0; i < total; ++i) {
        v2 = dynamicModel->Surface(i);
        v4 = v2->geometry;
        if (v4) {
            if (v4->numIndexes) {
                v5 = v2->shader;
                if (v5) {
                    if (/*((int) v5[66] > 0 || v5[19] || v5[20])
                        && //k??? TODO some idMaterial property as condition, need get property name by memory offset in 32bits */ !R_CullLocalBox(v4->bounds, vEffect->modelMatrix, 5, tr.viewDef->frustum)) {
                        def->visibleCount = tr.viewCount;
                        // if (!v4->primBatchMesh)  //k??? TODO no the property, same as DOOM3 if true
                        {
                            if (!R_CreateAmbientCache(v4, false))
                                return;
                            vertexCache.Touch(v4->ambientCache);
                            if (!v4->indexCache)
                                vertexCache.Alloc(v4->indexes, sizeof(v4->indexes[0]) * v4->numIndexes, &v4->indexCache,
                                                  true);
                            if (v4->indexCache)
                                vertexCache.Touch(v4->indexCache);
                        }
                        R_AddDrawSurf(
                                v4,
                                vEffect,
                                &vEffect->effectDef->parms,
                                v5,
                                vEffect->scissorRect/*,
                                0 */);
                        v4->ambientViewCount = tr.viewCount;
                    }
                }
            }
        }
    }
}

/*
===============
R_AddEffectSurfaces
===============
*/
void R_AddEffectSurfaces(void) {
    int v0; // eax
    viewEffect_s **v1; // esi
    int v2; // edi
    viewEffect_s *v3; // ebx
    idScreenRect *p_scissorRect; // ecx
    idRenderModel *v5; // eax
    viewEffect_s **array; // [esp+4h] [ebp-4h] BYREF

    v0 = R_SortViewEffects(&array);
    v1 = array;
    for(v2 = v0 - 1; v2 >= 0; --v2)
    {
        v3 = *v1;
        p_scissorRect = &(*v1++)->scissorRect;
        if ( p_scissorRect->IsEmpty() )
        {
            //++tr.pc.c_culledViewEffects;
        }
        else
        {
            v5 = R_EffectDefDynamicModel(v3->effectDef);
            if ( v5 )
            {
                if ( v5->NumSurfaces() > 0 )
                {
                    R_AddAmbientEffectDrawsurfs(v3);
                    //++tr.pc.c_visibleViewEffects;
                }
            }
        }
    }
}
#endif
#endif
