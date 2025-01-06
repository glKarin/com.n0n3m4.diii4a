// RenderModelCommitted.cpp
//

#include "RenderSystem_local.h"

static const float CHECK_BOUNDS_EPSILON = 1.0f;
#define MAX_POLYTOPE_PLANES		6

/*
==================
idRenderModelCommitted::CalcEntityScissorRectangle
==================
*/
idScreenRect idRenderModelCommitted::CalcEntityScissorRectangle(void) {
	idBounds bounds;
	idRenderEntityLocal* def = entityDef;

	tr.viewDef->viewFrustum.ProjectionBounds(idBox(def->referenceBounds, def->parms.origin, def->parms.axis), bounds);

	return R_ScreenRectFromViewFrustumBounds(bounds);
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
idRenderModel* idRenderModelCommitted::CreateDynamicModel(void) {
	bool callbackUpdate;
	idRenderEntityLocal* def = entityDef;

	// allow deferred entities to construct themselves
	if (def->parms.callback) {
		callbackUpdate = R_IssueEntityDefCallback(def);
	}
	else {
		callbackUpdate = false;
	}

	idRenderModel* model = def->parms.hModel;

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
		def->ClearEntityDefDynamicModel();
	}

	// if we don't have a snapshot of the dynamic model, generate it now
	if (!def->dynamicModel) {

		// instantiate the snapshot of the dynamic model, possibly reusing memory from the cached snapshot
		def->cachedDynamicModel = model->InstantiateDynamicModel(&def->parms, tr.viewDef, def->cachedDynamicModel);

		if (def->cachedDynamicModel) {

			// add any overlays to the snapshot of the dynamic model
			//if ( def->overlay && !r_skipOverlays.GetBool() ) {
			//	def->overlay->AddOverlaySurfacesToModel( def->cachedDynamicModel );
			//} else {
			//	idRenderModelOverlay::RemoveOverlaySurfacesFromModel( def->cachedDynamicModel );
			//}

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
==================
R_SkyboxTexGen
==================
*/
void R_SkyboxTexGen(drawSurf_t* surf, const idVec3& viewOrg) {
	int		i;
	idVec3	localViewOrigin;

	R_GlobalPointToLocal(surf->space->modelMatrix, viewOrg, localViewOrigin);

	int numVerts = surf->geo->numVerts;
	int size = numVerts * sizeof(idVec3);
	idVec3* texCoords = (idVec3*)_alloca16(size);

	const idDrawVert* verts = surf->geo->verts;
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
void R_WobbleskyTexGen(drawSurf_t* surf, const idVec3& viewOrg) {
	int		i;
	idVec3	localViewOrigin;

	const int* parms = surf->material->GetTexGenRegisters();

	float	wobbleDegrees = surf->shaderRegisters[parms[0]];
	float	wobbleSpeed = surf->shaderRegisters[parms[1]];
	float	rotateSpeed = surf->shaderRegisters[parms[2]];

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
	idVec3* texCoords = (idVec3*)_alloca16(size);

	const idDrawVert* verts = surf->geo->verts;
	for (i = 0; i < numVerts; i++) {
		idVec3 v;

		v[0] = verts[i].xyz[0] - localViewOrigin[0];
		v[1] = verts[i].xyz[1] - localViewOrigin[1];
		v[2] = verts[i].xyz[2] - localViewOrigin[2];

		R_LocalPointToGlobal(transform, v, texCoords[i]);
	}

	surf->dynamicTexCoords = vertexCache.AllocFrameTemp(texCoords, size);
}

/*
=================
R_SpecularTexGen

Calculates the specular coordinates for cards without vertex programs.
=================
*/
static void R_SpecularTexGen(drawSurf_t* surf, const idVec3& globalLightOrigin, const idVec3& viewOrg) {
	const srfTriangles_t* tri;
	idVec3	localLightOrigin;
	idVec3	localViewOrigin;

	R_GlobalPointToLocal(surf->space->modelMatrix, globalLightOrigin, localLightOrigin);
	R_GlobalPointToLocal(surf->space->modelMatrix, viewOrg, localViewOrigin);

	tri = surf->geo;

	// FIXME: change to 3 component?
	int	size = tri->numVerts * sizeof(idVec4);
	idVec4* texCoords = (idVec4*)_alloca16(size);

#if 1

	SIMDProcessor->CreateSpecularTextureCoords(texCoords, localLightOrigin, localViewOrigin,
		tri->verts, tri->numVerts, tri->indexes, tri->numIndexes);

#else

	bool* used = (bool*)_alloca16(tri->numVerts * sizeof(used[0]));
	memset(used, 0, tri->numVerts * sizeof(used[0]));

	// because the interaction may be a very small subset of the full surface,
	// it makes sense to only deal with the verts used
	for (int j = 0; j < tri->numIndexes; j++) {
		int i = tri->indexes[j];
		if (used[i]) {
			continue;
		}
		used[i] = true;

		float ilength;

		const idDrawVert* v = &tri->verts[i];

		idVec3 lightDir = localLightOrigin - v->xyz;
		idVec3 viewDir = localViewOrigin - v->xyz;

		ilength = idMath::RSqrt(lightDir * lightDir);
		lightDir[0] *= ilength;
		lightDir[1] *= ilength;
		lightDir[2] *= ilength;

		ilength = idMath::RSqrt(viewDir * viewDir);
		viewDir[0] *= ilength;
		viewDir[1] *= ilength;
		viewDir[2] *= ilength;

		lightDir += viewDir;

		texCoords[i][0] = lightDir * v->tangents[0];
		texCoords[i][1] = lightDir * v->tangents[1];
		texCoords[i][2] = lightDir * v->normal;
		texCoords[i][3] = 1;
	}

#endif

	surf->dynamicTexCoords = vertexCache.AllocFrameTemp(texCoords, size);
}

/*
==================
R_IssueEntityDefCallback
==================
*/
bool R_IssueEntityDefCallback(idRenderEntityLocal* def) {
	bool update;
	idBounds	oldBounds;

	if (r_checkBounds.GetBool()) {
		oldBounds = def->referenceBounds;
	}

	def->archived = false;		// will need to be written to the demo file
	tr.pc.c_entityDefCallbacks++;
	if (tr.viewDef) {
		update = def->parms.callback(&def->parms, &tr.viewDef->renderView);
	}
	else {
		update = def->parms.callback(&def->parms, NULL);
	}

	if (!def->parms.hModel) {
		common->Error("R_IssueEntityDefCallback: dynamic entity callback didn't set model");
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
=================
R_AddDrawSurf
=================
*/
drawSurf_t* R_AddDrawSurf(const srfTriangles_t* tri, const idRenderModelCommitted* space, const renderEntity_t* renderEntity,
	const idMaterial* shader, const idScreenRect& scissor, idVec4 forceColor) {
	drawSurf_t* drawSurf;
	const float* shaderParms;
	static float	refRegs[MAX_EXPRESSION_REGISTERS];	// don't put on stack, or VC++ will do a page touch
	float			generatedShaderParms[MAX_ENTITY_SHADER_PARMS];

	drawSurf = (drawSurf_t*)R_FrameAlloc(sizeof(*drawSurf));
	drawSurf->geo = tri;
	drawSurf->space = space;
	drawSurf->material = shader;
	drawSurf->scissorRect = scissor;
	drawSurf->sort = shader->GetSort() + tr.sortOffset;
	drawSurf->dsFlags = 0;
	drawSurf->forceColor = forceColor;
	drawSurf->forceTwoSided = renderEntity->forceTwoSided;
	drawSurf->numSurfRenderLights = 0;
	drawSurf->hideInMainView = renderEntity->hideInMainView;

	// bumping this offset each time causes surfaces with equal sort orders to still
	// deterministically draw in the order they are added
	tr.sortOffset += 0.000001f;

	// if it doesn't fit, resize the list
	if (tr.viewDef->numDrawSurfs == tr.viewDef->maxDrawSurfs) {
		drawSurf_t** old = tr.viewDef->drawSurfs;
		int			count;

		if (tr.viewDef->maxDrawSurfs == 0) {
			tr.viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
			count = 0;
		}
		else {
			count = tr.viewDef->maxDrawSurfs * sizeof(tr.viewDef->drawSurfs[0]);
			tr.viewDef->maxDrawSurfs *= 2;
		}
		tr.viewDef->drawSurfs = (drawSurf_t**)R_FrameAlloc(tr.viewDef->maxDrawSurfs * sizeof(tr.viewDef->drawSurfs[0]));
		memcpy(tr.viewDef->drawSurfs, old, count);
	}
	tr.viewDef->drawSurfs[tr.viewDef->numDrawSurfs] = drawSurf;
	tr.viewDef->numDrawSurfs++;

	// process the shader expressions for conditionals / color / texcoords
	const float* constRegs = shader->ConstantRegisters();
	if (constRegs) {
		// shader only uses constant values
		drawSurf->shaderRegisters = constRegs;
	}
	else {
		float* regs = (float*)R_FrameAlloc(shader->GetNumRegisters() * sizeof(float));
		drawSurf->shaderRegisters = regs;

		// a reference shader will take the calculated stage color value from another shader
		// and use that for the parm0-parm3 of the current shader, which allows a stage of
		// a light model and light flares to pick up different flashing tables from
		// different light shaders
		if (renderEntity->referenceShader) {
			// evaluate the reference shader to find our shader parms
			const shaderStage_t* pStage;

			renderEntity->referenceShader->EvaluateRegisters(refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->referenceSound);
			pStage = renderEntity->referenceShader->GetStage(0);

			memcpy(generatedShaderParms, renderEntity->shaderParms, sizeof(generatedShaderParms));
			generatedShaderParms[0] = refRegs[pStage->color.registers[0]];
			generatedShaderParms[1] = refRegs[pStage->color.registers[1]];
			generatedShaderParms[2] = refRegs[pStage->color.registers[2]];

			shaderParms = generatedShaderParms;
		}
		else {
			// evaluate with the entityDef's shader parms
			shaderParms = renderEntity->shaderParms;
		}

		float oldFloatTime;
		int oldTime;

		if (space->entityDef && space->entityDef->parms.timeGroup) {
			oldFloatTime = tr.viewDef->floatTime;
			oldTime = tr.viewDef->renderView.time;

			tr.viewDef->floatTime = game->GetTimeGroupTime(space->entityDef->parms.timeGroup) * 0.001;
			tr.viewDef->renderView.time = game->GetTimeGroupTime(space->entityDef->parms.timeGroup);
		}

		shader->EvaluateRegisters(regs, shaderParms, tr.viewDef, renderEntity->referenceSound);

		if (space->entityDef && space->entityDef->parms.timeGroup) {
			tr.viewDef->floatTime = oldFloatTime;
			tr.viewDef->renderView.time = oldTime;
		}
	}

	// check for deformations
	space->DeformDrawSurf(drawSurf);

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
	idUserInterface* gui = NULL;

	if (!space->entityDef) {
		gui = shader->GlobalGui();
	}
	else {
		int guiNum = shader->GetEntityGui() - 1;
		if (guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI) {
			gui = renderEntity->gui[guiNum];
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

		tr.viewDef->floatTime = game->GetTimeGroupTime(1) * 0.001;
		tr.viewDef->renderView.time = game->GetTimeGroupTime(1);

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

	return drawSurf;
}

/*
===============
idRenderModelCommitted::GenerateSurfaceLights
===============
*/
void idRenderModelCommitted::GenerateSurfaceLights(int committedRenderModelId, drawSurf_t* newDrawSurf, const idRenderLightCommitted* lightDefs)
{
	const idRenderLightCommitted* vLight = lightDefs;

	while (vLight != nullptr)
	{
		idBounds drawSurfBounds = newDrawSurf->geo->bounds;
		drawSurfBounds.TranslateSelf(entityDef->parms.origin);
		
		idRenderLightLocal* renderLight = vLight->lightDef;

		if (!renderLight->lightRendered)
		{
			vLight = vLight->next;
			continue;
		}
		
		if (drawSurfBounds.IntersectsBounds(renderLight->globalLightBounds))
		{
			if (newDrawSurf->numSurfRenderLights >= MAX_RENDERLIGHTS_PER_SURFACE)
			{
			//	common->Error("Too many realtime shadow casting lights hitting surface.");
			}
			else
			{
				newDrawSurf->surfRenderLights[newDrawSurf->numSurfRenderLights++] = vLight;
			}
		}

		vLight = vLight->next;
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
void idRenderModelCommitted::AddDrawsurfs(int committedRenderModelId, const idRenderLightCommitted* lightDefs) {
	int					i, total;
	idRenderEntityLocal* def;
	srfTriangles_t* tri;
	idRenderModel* model;
	const idMaterial* shader;

	def = entityDef;

	if (def->dynamicModel) {
		model = def->dynamicModel;
	}
	else {
		model = def->parms.hModel;
	}

	// add all the surfaces
	total = model->NumSurfaces();
	for (i = 0; i < total; i++) {
		const idModelSurface* surf = model->Surface(i);

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
			for (j = 0; j < tri->numVerts; j++) {
				for (k = 0; k < 3; k++) {
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

		if (def->parms.skipFrustumCulling || !R_CullLocalBox(tri->bounds, modelMatrix, 5, tr.viewDef->frustum)) {

			def->visibleCount = tr.viewCount;

			// make sure we have an ambient cache
			if (!R_CreateAmbientCache(tri, shader->ReceivesLighting())) {
				// don't add anything if the vertex cache was too full to give us an ambient cache
				return;
			}
			// touch it so it won't get purged
			vertexCache.Touch(tri->ambientCache);

			if (r_useIndexBuffers.GetBool() && !tri->indexCache) {
				vertexCache.Alloc(tri->indexes, tri->numIndexes * sizeof(tri->indexes[0]), &tri->indexCache, true);
			}
			if (tri->indexCache) {
				vertexCache.Touch(tri->indexCache);
			}

			// add the surface for drawing
			drawSurf_t *newDrawSurf = R_AddDrawSurf(tri, this, &entityDef->parms, shader, scissorRect);

			// Generate lighting for this surface.
			GenerateSurfaceLights(committedRenderModelId, newDrawSurf, lightDefs);

			// ambientViewCount is used to allow light interactions to be rejected
			// if the ambient surface isn't visible at all
			tri->ambientViewCount = tr.viewCount;
		}
	}

	// add the lightweight decal surfaces
	for (idRenderModelDecal* decal = def->decals; decal; decal = decal->Next()) {
		decal->AddDecalDrawSurf(this);
	}
}


/*
=====================
R_PolytopeSurface

Generate vertexes and indexes for a polytope, and optionally returns the polygon windings.
The positive sides of the planes will be visible.
=====================
*/
srfTriangles_t* R_PolytopeSurface(int numPlanes, const idPlane* planes, idWinding** windings) {
	int i, j;
	srfTriangles_t* tri;
	idFixedWinding planeWindings[MAX_POLYTOPE_PLANES];
	int numVerts, numIndexes;

	if (numPlanes > MAX_POLYTOPE_PLANES) {
		common->Error("R_PolytopeSurface: more than %d planes", MAX_POLYTOPE_PLANES);
	}

	numVerts = 0;
	numIndexes = 0;
	for (i = 0; i < numPlanes; i++) {
		const idPlane& plane = planes[i];
		idFixedWinding& w = planeWindings[i];

		w.BaseForPlane(plane);
		for (j = 0; j < numPlanes; j++) {
			const idPlane& plane2 = planes[j];
			if (j == i) {
				continue;
			}
			if (!w.ClipInPlace(-plane2, ON_EPSILON)) {
				break;
			}
		}
		if (w.GetNumPoints() <= 2) {
			continue;
		}
		numVerts += w.GetNumPoints();
		numIndexes += (w.GetNumPoints() - 2) * 3;
	}

	// allocate the surface
	tri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts(tri, numVerts);
	R_AllocStaticTriSurfIndexes(tri, numIndexes);

	// copy the data from the windings
	for (i = 0; i < numPlanes; i++) {
		idFixedWinding& w = planeWindings[i];
		if (!w.GetNumPoints()) {
			continue;
		}
		for (j = 0; j < w.GetNumPoints(); j++) {
			tri->verts[tri->numVerts + j].Clear();
			tri->verts[tri->numVerts + j].xyz = w[j].ToVec3();
		}

		for (j = 1; j < w.GetNumPoints() - 1; j++) {
			tri->indexes[tri->numIndexes + 0] = tri->numVerts;
			tri->indexes[tri->numIndexes + 1] = tri->numVerts + j;
			tri->indexes[tri->numIndexes + 2] = tri->numVerts + j + 1;
			tri->numIndexes += 3;
		}
		tri->numVerts += w.GetNumPoints();

		// optionally save the winding
		if (windings) {
			windings[i] = new idWinding(w.GetNumPoints());
			*windings[i] = w;
		}
	}

	R_BoundTriSurf(tri);

	return tri;
}

/*
=====================
idRenderSystemLocal::PolytopeSurface
=====================
*/
srfTriangles_t* idRenderSystemLocal::PolytopeSurface(int numPlanes, const idPlane* planes, idWinding** windings) {
	return R_PolytopeSurface(numPlanes, planes, windings);
}