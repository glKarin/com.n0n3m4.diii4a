// RenderWorldCommitted.cpp
//

#include "RenderSystem_local.h"

/*
===================
R_PointInFrustum

Assumes positive sides face outward
===================
*/
static bool R_PointInFrustum(idVec3& p, idPlane* planes, int numPlanes) {
	for (int i = 0; i < numPlanes; i++) {
		float d = planes[i].Distance(p);
		if (d > 0) {
			return false;
		}
	}
	return true;
}


/*
=======================
R_QsortSurfaces

=======================
*/
static int R_QsortSurfaces(const void* a, const void* b) {
	const drawSurf_t* ea, * eb;

	ea = *(drawSurf_t**)a;
	eb = *(drawSurf_t**)b;

	if (ea->sort < eb->sort) {
		return -1;
	}
	if (ea->sort > eb->sort) {
		return 1;
	}
	return 0;
}


/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs(void) {
	// sort the drawsurfs by sort type, then orientation, then shader
	qsort(tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs, sizeof(tr.viewDef->drawSurfs[0]),
		R_QsortSurfaces);
}


/*
===================
R_CheckForEntityDefsUsingModel
===================
*/
void R_CheckForEntityDefsUsingModel(idRenderModel* model) {
	int i, j;
	idRenderWorldLocal* rw;
	idRenderEntityLocal* def;

	for (j = 0; j < tr.worlds.Num(); j++) {
		rw = tr.worlds[j];

		for (i = 0; i < rw->entityDefs.Num(); i++) {
			def = rw->entityDefs[i];
			if (!def) {
				continue;
			}
			if (def->parms.hModel == model) {
				//assert( 0 );
				// this should never happen but Radiant messes it up all the time so just free the derived data
				def->FreeEntityDefDerivedData(false, false);
			}
		}
	}
}

/*
=========================
idRenderWorldCommitted::CommitRenderLight
=========================
*/
idRenderLightCommitted* idRenderWorldCommitted::CommitRenderLight(idRenderLightLocal* light) {
	idRenderLightCommitted* vLight;

	if (light->viewCount == tr.viewCount) {
		return light->viewLight;
	}
	light->viewCount = tr.viewCount;

	// add to the view light chain
	vLight = (idRenderLightCommitted*)R_ClearedFrameAlloc(sizeof(*vLight));
	vLight->lightDef = light;

	// the scissorRect will be expanded as the light bounds is accepted into visible portal chains
	vLight->scissorRect.Clear();

	// calculate the shadow cap optimization states
	vLight->viewInsideLight = vLight->TestPointInViewLight(tr.viewDef->renderView.vieworg, light);
	if (!vLight->viewInsideLight) {
		vLight->viewSeesShadowPlaneBits = 0;
		for (int i = 0; i < light->numShadowFrustums; i++) {
			float d = light->shadowFrustums[i].planes[5].Distance(tr.viewDef->renderView.vieworg);
			if (d < INSIDE_LIGHT_FRUSTUM_SLOP) {
				vLight->viewSeesShadowPlaneBits |= 1 << i;
			}
		}
	}
	else {
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
	vLight->fogPlane = light->frustum[5];
	vLight->frustumTris = light->frustumTris;
	vLight->falloffImage = light->falloffImage;
	vLight->lightShader = light->lightShader;
	vLight->shaderRegisters = NULL;		// allocated and evaluated in R_AddLightSurfaces

	// link the view light
	vLight->next = viewLights;
	viewLights = vLight;

	light->viewLight = vLight;

	return vLight;
}


/*
=============
idRenderWorldCommitted::CommitRenderModel

If the entityDef isn't already on the viewEntity list, create
a viewEntity and add it to the list with an empty scissor rect.

This does not instantiate dynamic models for the entity yet.
=============
*/
idRenderModelCommitted* idRenderWorldCommitted::CommitRenderModel(idRenderEntityLocal* def) {
	idRenderModelCommitted* vModel;

	if (def->viewCount == tr.viewCount) {
		return def->viewEntity;
	}
	def->viewCount = tr.viewCount;

	// set the model and modelview matricies
	vModel = (idRenderModelCommitted*)R_ClearedFrameAlloc(sizeof(*vModel));
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

int sortLightEntities(idRenderLightAttachedEntity* a, idRenderLightAttachedEntity* b)
{
	return a->distance - b->distance;
}

/*
===================
idRenderWorldCommitted::AddModelAndLightRefs
===================
*/
void idRenderWorldCommitted::AddModelAndLightRefs(void) {
	idRenderLightCommitted* vLight;
	idRenderModelCommitted* vEnt;
	idRenderWorldLocal* renderWorldFrontEnd = static_cast<idRenderWorldLocal*>(renderWorld);

	for (int i = 0; i < renderWorldFrontEnd->lightDefs.Num(); i++)
	{
		idRenderLightLocal* lightDef = renderWorldFrontEnd->lightDefs[i];

		if (lightDef == nullptr)
			continue;

		// debug tool to allow viewing of only one light at a time
		if (r_singleLight.GetInteger() >= 0 && r_singleLight.GetInteger() != renderWorldFrontEnd->lightDefs[i]->index) {
			continue;
		}

		vLight = tr.viewDef->CommitRenderLight(renderWorldFrontEnd->lightDefs[i]);
		vLight->scissorRect = vLight->CalcLightScissorRectangle();		

		const idMaterial* lightShader = lightDef->lightShader;
		if (!lightShader) {
			common->Error("NULL lightShader");
		}

		// evaluate the light shader registers
		float* lightRegs = (float*)R_FrameAlloc(lightShader->GetNumRegisters() * sizeof(float));
		vLight->shaderRegisters = lightRegs;
		lightShader->EvaluateRegisters(lightRegs, lightDef->parms.shaderParms, tr.viewDef, lightDef->parms.referenceSound);
	}

	for (int i = 0; i < renderWorldFrontEnd->entityDefs.Num(); i++)
	{
		// debug tool to allow viewing of only one entity at a time
		if (r_singleEntity.GetInteger() >= 0 && r_singleEntity.GetInteger() != renderWorldFrontEnd->entityDefs[i]->index) {
			continue;
		}

		if (renderWorldFrontEnd->entityDefs[i] == nullptr)
			continue;

		if(renderWorldFrontEnd->entityDefs[i]->parms.hModel->IsDefaultModel())
			continue;

		// remove decals that are completely faded away
		renderWorldFrontEnd->entityDefs[i]->FreeEntityDefFadedDecals(tr.viewDef->renderView.time);

		vEnt = tr.viewDef->CommitRenderModel(renderWorldFrontEnd->entityDefs[i]);
		vEnt->scissorRect = vEnt->CalcEntityScissorRectangle();

		vEnt->renderModel = vEnt->CreateDynamicModel();
		if (vEnt->renderModel == NULL || vEnt->renderModel->NumSurfaces() <= 0) {
			continue;
		}

		//---------------------------
		// make a tech5 renderMatrix
		//---------------------------
		R_MatrixMultiply(vEnt->modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, vEnt->modelViewMatrix);

		idRenderMatrix viewMat;

		idRenderMatrix::Transpose(*(idRenderMatrix*)vEnt->modelViewMatrix, viewMat);

		if (vEnt->entityDef->parms.customFOV == 0)
		{
			idRenderMatrix::Multiply(tr.viewDef->projectionRenderMatrix, viewMat, vEnt->mvp);
		}
		else
		{
			float customProjectionMatrix[16];
			idRenderMatrix customProjectionRenderMatrix;

			R_SetupProjectionMatrix(this, vEnt->entityDef->parms.fov_x, vEnt->entityDef->parms.fov_y, customProjectionMatrix);

			idRenderMatrix::Transpose(*(idRenderMatrix*)customProjectionMatrix, customProjectionRenderMatrix);
			idRenderMatrix::Multiply(customProjectionRenderMatrix, viewMat, vEnt->mvp);
		}

		vEnt->AddDrawsurfs(i, tr.viewDef->viewLights);
	}


	// We have already consumed the lightRendered states, so mark them as not rendered. 
	// This has to be done at the end so CommitRenderModel can determine what lights are visible.
	vLight = tr.viewDef->viewLights;
	while (vLight != nullptr)
	{
		vLight->lightDef->lightRendered = false;
		vLight = vLight->next;
	}
}

/*
================
R_RenderView

A view may be either the actual camera view,
a mirror / remote location, or a 3D view on a gui surface.

Parms will typically be allocated with R_FrameAlloc
================
*/
void idRenderWorldCommitted::RenderView(void) {
	idRenderWorldCommitted* oldView;

	if (renderView.width <= 0 || renderView.height <= 0) {
		return;
	}

	tr.viewCount++;

	// save view in case we are a subview
	oldView = tr.viewDef;

	tr.viewDef = this;

	tr.sortOffset = 0;

	// set the matrix for world space to eye space
	SetViewMatrix();

	// the four sides of the view frustum are needed
	// for culling and portal visibility
	SetupViewFrustum();

	// we need to set the projection matrix before doing
	// portal-to-screen scissor box calculations
	R_SetupProjectionMatrix(this);

	// setup render matrices for faster culling
	idRenderMatrix::Transpose(*(idRenderMatrix*)tr.viewDef->projectionMatrix, tr.viewDef->projectionRenderMatrix);
	idRenderMatrix::Transpose(*(idRenderMatrix*)tr.viewDef->worldSpace.modelViewMatrix, *(idRenderMatrix*)tr.viewDef->worldSpace.transposedModelViewMatrix);
	idRenderMatrix::Multiply(tr.viewDef->projectionRenderMatrix, *(idRenderMatrix*)tr.viewDef->worldSpace.transposedModelViewMatrix, tr.viewDef->worldSpace.mvp);

	// identify all the visible portalAreas, and the entityDefs and
	// lightDefs that are in them and pass culling.
	AddModelAndLightRefs();

	// constrain the view frustum to the view lights and entities
	ConstrainViewFrustum();

	// sort all the ambient surfaces for translucency ordering
	R_SortDrawSurfs();

	// generate any subviews (mirrors, cameras, etc) before adding this view
	if (R_GenerateSubViews()) {
		// if we are debugging subviews, allow the skipping of the
		// main view draw
		if (r_subviewOnly.GetBool()) {
			return;
		}
	}

	// write everything needed to the demo file
	if (session->writeDemo) {
		static_cast<idRenderWorldLocal*>(renderWorld)->WriteVisibleDefs(tr.viewDef);
	}

	// add the rendering commands for this viewDef
	AddDrawViewCmd();

	// restore view in case we are a subview
	tr.viewDef = oldView;
}


/*
=================
R_SetupViewFrustum

Setup that culling frustum planes for the current view
FIXME: derive from modelview matrix times projection matrix
=================
*/
void idRenderWorldCommitted::SetupViewFrustum(void) {
	int		i;
	float	xs, xc;
	float	ang;

	ang = DEG2RAD(tr.viewDef->renderView.fov_x) * 0.5f;
	idMath::SinCos(ang, xs, xc);

	tr.viewDef->frustum[0] = xs * tr.viewDef->renderView.viewaxis[0] + xc * tr.viewDef->renderView.viewaxis[1];
	tr.viewDef->frustum[1] = xs * tr.viewDef->renderView.viewaxis[0] - xc * tr.viewDef->renderView.viewaxis[1];

	ang = DEG2RAD(tr.viewDef->renderView.fov_y) * 0.5f;
	idMath::SinCos(ang, xs, xc);

	tr.viewDef->frustum[2] = xs * tr.viewDef->renderView.viewaxis[0] + xc * tr.viewDef->renderView.viewaxis[2];
	tr.viewDef->frustum[3] = xs * tr.viewDef->renderView.viewaxis[0] - xc * tr.viewDef->renderView.viewaxis[2];

	// plane four is the front clipping plane
	tr.viewDef->frustum[4] = /* vec3_origin - */ tr.viewDef->renderView.viewaxis[0];

	for (i = 0; i < 5; i++) {
		// flip direction so positive side faces out (FIXME: globally unify this)
		tr.viewDef->frustum[i] = -tr.viewDef->frustum[i].Normal();
		tr.viewDef->frustum[i][3] = -(tr.viewDef->renderView.vieworg * tr.viewDef->frustum[i].Normal());
	}

	// eventually, plane five will be the rear clipping plane for fog

	float dNear, dFar, dLeft, dUp;

	dNear = r_znear.GetFloat();
	if (tr.viewDef->renderView.cramZNear) {
		dNear *= 0.25f;
	}

	dFar = MAX_WORLD_SIZE;
	dLeft = dFar * tan(DEG2RAD(tr.viewDef->renderView.fov_x * 0.5f));
	dUp = dFar * tan(DEG2RAD(tr.viewDef->renderView.fov_y * 0.5f));
	tr.viewDef->viewFrustum.SetOrigin(tr.viewDef->renderView.vieworg);
	tr.viewDef->viewFrustum.SetAxis(tr.viewDef->renderView.viewaxis);
	tr.viewDef->viewFrustum.SetSize(dNear, dFar, dLeft, dUp);
}

/*
=================
idRenderWorldCommitted::ConstrainViewFrustum

Setup that culling frustum planes for the current view
FIXME: derive from modelview matrix times projection matrix
=================
*/
void idRenderWorldCommitted::ConstrainViewFrustum(void) {
	idBounds bounds;

	// constrain the view frustum to the total bounds of all visible lights and visible entities
	bounds.Clear();
	for (idRenderLightCommitted* vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next) {
		bounds.AddBounds(vLight->lightDef->frustumTris->bounds);
	}
	for (idRenderModelCommitted* vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next) {
		bounds.AddBounds(vEntity->entityDef->referenceBounds);
	}
	tr.viewDef->viewFrustum.ConstrainToBounds(bounds);

	if (r_useFrustumFarDistance.GetFloat() > 0.0f) {
		tr.viewDef->viewFrustum.MoveFarDistance(r_useFrustumFarDistance.GetFloat());
	}
}


/*
=============
R_AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
void idRenderWorldCommitted::AddDrawViewCmd(void) {
	drawSurfsCommand_t* cmd;

	cmd = (drawSurfsCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	cmd->commandId = RC_DRAW_VIEW;

	cmd->viewDef = this;

	if (viewEntitys) {
		// save the command for r_lockSurfaces debugging
		tr.lockSurfacesCmd = *cmd;
	}

	tr.pc.c_numViews++;
}


/*
=================
R_SetViewMatrix

Sets up the world to view matrix for a given viewParm
=================
*/
void idRenderWorldCommitted::SetViewMatrix(void) {
	idVec3	origin;
	idRenderModelCommitted* world;
	float	viewerMatrix[16];
	static float	s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};

	world = &worldSpace;

	memset(world, 0, sizeof(*world));

	// the model matrix is an identity
	world->modelMatrix[0 * 4 + 0] = 1;
	world->modelMatrix[1 * 4 + 1] = 1;
	world->modelMatrix[2 * 4 + 2] = 1;

	// transform by the camera placement
	origin = renderView.vieworg;

	viewerMatrix[0] = renderView.viewaxis[0][0];
	viewerMatrix[4] = renderView.viewaxis[0][1];
	viewerMatrix[8] = renderView.viewaxis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = renderView.viewaxis[1][0];
	viewerMatrix[5] = renderView.viewaxis[1][1];
	viewerMatrix[9] = renderView.viewaxis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = renderView.viewaxis[2][0];
	viewerMatrix[6] = renderView.viewaxis[2][1];
	viewerMatrix[10] = renderView.viewaxis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix(viewerMatrix, s_flipMatrix, world->modelViewMatrix);
}


/*
===================
R_FreeDerivedData

ReloadModels and RegenerateWorld call this
// FIXME: need to do this for all worlds
===================
*/
void R_FreeDerivedData(void) {
	int i, j;
	idRenderWorldLocal* rw;
	idRenderEntityLocal* def;
	idRenderLightLocal* light;

	for (j = 0; j < tr.worlds.Num(); j++) {
		rw = tr.worlds[j];

		for (i = 0; i < rw->entityDefs.Num(); i++) {
			def = rw->entityDefs[i];
			if (!def) {
				continue;
			}
			def->FreeEntityDefDerivedData(false, false);
		}

		for (i = 0; i < rw->lightDefs.Num(); i++) {
			light = rw->lightDefs[i];
			if (!light) {
				continue;
			}
			light->FreeLightDefDerivedData();
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
void R_ReCreateWorldReferences(void) {
	int i, j;
	idRenderWorldLocal* rw;
	idRenderEntityLocal* def;
	idRenderLightLocal* light;

	// let the interaction generation code know this shouldn't be optimized for
	// a particular view
	tr.viewDef = NULL;

	for (j = 0; j < tr.worlds.Num(); j++) {
		rw = tr.worlds[j];

		for (i = 0; i < rw->entityDefs.Num(); i++) {
			def = rw->entityDefs[i];
			if (!def) {
				continue;
			}
			// the world model entities are put specifically in a single
			// area, instead of just pushing their bounds into the tree
			def->CreateEntityRefs();
		}

		for (i = 0; i < rw->lightDefs.Num(); i++) {
			light = rw->lightDefs[i];
			if (!light) {
				continue;
			}
			renderLight_t parms = light->parms;

			light->world->FreeLightDef(i);
			rw->UpdateLightDef(i, &parms);
		}
	}
}
