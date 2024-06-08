// draw_shadow.cpp
//

#include "../../RenderSystem_local.h"
#include "../../../models/Model_local.h"

static idRenderMatrix	lightProjectionMatrix;
static float	unflippedLightMatrix[16];
static float	lightMatrix[16];
static float	viewLightAxialSize;

static const int CULL_RECEIVER = 1;	// still draw occluder, but it is out of the view
static const int CULL_OCCLUDER_AND_RECEIVER = 2;	// the surface doesn't effect the view at all

idCVar r_shadow_polyOfsFactor("r_shadow_polyOfsFactor", "2", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset factor for drawing shadow buffer");
idCVar r_shadow_polyOfsUnits("r_shadow_polyOfsUnits", "3000", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset units for drawing shadow buffer");
idCVar r_shadowOccluderFacing("r_shadowOccluderFacing", "1", CVAR_INTEGER, "0 = front side, 1 = back side culling for shadows");
idCVar r_shadowEnableCache("r_shadowEnableCache", "0", CVAR_RENDERER | CVAR_BOOL, "enable shadow map caching");

/*
==================
R_Shadow_CalcLightAxialSize

all light side projections must currently match, so non-centered
and non-cubic lights must take the largest length
==================
*/
float	R_Shadow_CalcLightAxialSize(idRenderLightCommitted* vLight) {
	float	max = 0;

	if (vLight->lightDef->parms.lightType != LIGHT_TYPE_POINT) {
		idVec3	dir = vLight->lightDef->parms.target - vLight->lightDef->parms.origin;
		max = dir.Length();
		return max;
	}

	for (int i = 0; i < 3; i++) {
		float	dist = fabs(vLight->lightDef->parms.lightCenter[i]);
		dist += vLight->lightDef->parms.lightRadius[i];
		if (dist > max) {
			max = dist;
		}
	}
	return max;
}

/*
==================
RB_Shadow_RenderOccluders
==================
*/

/*
==================
RB_Shadow_RenderOccluders
==================
*/
void RB_Shadow_RenderOccluders(idRenderLightCommitted* vLight) {
	drawSurf_t** drawSurfs;
	int			numDrawSurfs;

	drawSurfs = (drawSurf_t**)&backEnd.viewDef->drawSurfs[0];
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;


	const idRenderEntityLocal* lastEntityDef = nullptr;
	for (int i = 0; i < numDrawSurfs; i++)
	{
		const idRenderEntityLocal* entityDef = drawSurfs[i]->space->entityDef;//  vLight->litRenderEntities[i].entity;

		if (!entityDef) {
			continue;
		}
		idRenderModel* inter = entityDef->viewEntity->renderModel;
		
		idBounds geoBounds = drawSurfs[i]->geo->bounds;
		geoBounds.TranslateSelf(entityDef->parms.origin);
		if (!geoBounds.IntersectsBounds(vLight->lightDef->globalLightBounds)) {
			continue;
		}

		if (entityDef->parms.noShadow)
			continue;

		//if(inter->NumJoints() > 0 && vLight->lightDef->GetLightRenderType() == LIGHT_RENDER_STATIC)
		//	continue;

		// no need to check for current on this, because each interaction is always
		// a different space

		rvmProgramVariants_t programVariant = PROG_VARIANT_NONSKINNED;
		if (drawSurfs[i]->space->renderModel->IsSkeletalMesh()) {
			programVariant = PROG_VARIANT_SKINNED;
		}

		if (entityDef != lastEntityDef)
		{
			idRenderMatrix	matrix, transposeMatrix, mvp;
			myGlMultMatrix(entityDef->modelMatrix, lightMatrix, matrix.GetFloatPtr());

			idRenderMatrix::Transpose(matrix, transposeMatrix);
			idRenderMatrix::Multiply(lightProjectionMatrix, transposeMatrix, mvp);

			RB_SetMVP(mvp);
			RB_SetModelMatrix(entityDef->modelMatrix);

			tr.shadowMapProgram[programVariant]->Bind();
			lastEntityDef = entityDef;
		}

		// draw each surface
		{
			if (drawSurfs[i]->material && !drawSurfs[i]->material->SurfaceCastsShadow()) {
				continue;
			}

			if (drawSurfs[i]->material && drawSurfs[i]->material->GetCullType() == CT_TWO_SIDED)
			{
				GL_Cull(CT_TWO_SIDED);
			}
			else
			{
				//
				// set polygon offset for the rendering
				//
				switch (r_shadowOccluderFacing.GetInteger()) {
				case 0:	// front sides
					GL_Cull(CT_TWO_SIDED);
					break;
				case 1:	// back sides
					GL_Cull(CT_BACK_SIDED);
					break;
				}
			}

			//if(surfInt->noShadow)
			//	continue;

			// cull it
			//if (surfInt->expCulled == CULL_OCCLUDER_AND_RECEIVER) {
			//	continue;
			//}

			//if(surfInt->skinning.HasSkinning()) {
			//	renderProgManager.BindShader_ShadowSkinned();
			//}
			//else {
			//	renderProgManager.BindShader_Shadow();
			//}			

			// render it
			const srfTriangles_t* tri = drawSurfs[i]->geo;
			if (!tri->ambientCache) {
				continue;
			}

			// set the vertex pointers
			idDrawVert* ac = (idDrawVert*)vertexCache.Position(tri->ambientCache);
#ifdef __ANDROID__ //karin: GLES is programming pipeline
			glVertexAttribPointer(PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
#else
			glVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
#endif

			//if (surfInt->skinning.HasSkinning()) {
			//	const rvmSkeletalSurf_t* skinning = &surfInt->skinning;
			//	RB_BindJointBuffer(skinning->jointBuffer, skinning->jointsInverted->ToFloatPtr(), skinning->numInvertedJoints, (void*)&ac->color, (void*)&ac->color2);
			//}

			//if (surfInt->shader) {
			//	surfInt->shader->GetEditorImage()->Bind();
			//}

			const shaderStage_t* pStage = drawSurfs[i]->material->GetAlbedoStage();
			if (pStage == nullptr)
				continue;

			tr.albedoTextureParam->SetImage(pStage->texture.image[0]);

			if (pStage->texture.image[0]->IsTransparent())
			{
				tr.shadowMapAlbedoProgram[programVariant]->Bind();
			}
			else
			{
				tr.shadowMapProgram[programVariant]->Bind();
			}


			if (drawSurfs[i]->space->renderModel->IsSkeletalMesh()) {
				idDrawVert* ac = nullptr;
				idRenderModelMD5Instance* skinning = (idRenderModelMD5Instance*)drawSurfs[i]->space->renderModel;
				RB_BindJointBuffer(skinning->jointBuffer, skinning->jointsInverted->ToFloatPtr(), skinning->numInvertedJoints, (void*)&ac->color, (void*)&ac->color2);
			}

			RB_DrawElementsWithCounters(tri);
			
			if (drawSurfs[i]->space->renderModel->IsSkeletalMesh()) {
				RB_UnBindJointBuffer();
			}
		}
	}
}

/*
==================
RB_RenderShadowBuffer
==================
*/
void  RB_RenderShadowBuffer(idRenderLightCommitted* vLight, int side) {
	float	xmin, xmax, ymin, ymax;
	float	width, height;
	float	zNear;

	float	fov = 90.0f;

	//
	// set up 90 degree projection matrix
	//
	zNear = 0.5f;

	ymax = zNear * tan(fov * idMath::PI / 360.0f);
	ymin = -ymax;

	xmax = zNear * tan(fov * idMath::PI / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	float* lightProjectMatrixPtr = lightProjectionMatrix.GetFloatPtr();
	lightProjectionMatrix.Zero();

	lightProjectMatrixPtr[0] = 2 * zNear / width;
	lightProjectMatrixPtr[4] = 0;
	lightProjectMatrixPtr[8] = 0;
	lightProjectMatrixPtr[12] = 0;

	lightProjectMatrixPtr[1] = 0;
	lightProjectMatrixPtr[5] = 2 * zNear / height;
	lightProjectMatrixPtr[9] = 0;
	lightProjectMatrixPtr[13] = 0;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	lightProjectMatrixPtr[2] = 0;
	lightProjectMatrixPtr[6] = 0;
	lightProjectMatrixPtr[10] = -0.999f;
	lightProjectMatrixPtr[14] = -2.0f * zNear;

	lightProjectMatrixPtr[3] = 0;
	lightProjectMatrixPtr[7] = 0;
	lightProjectMatrixPtr[11] = -1;
	lightProjectMatrixPtr[15] = 0;


	GL_State(GLS_DEPTHFUNC_LESS);

	// draw all the occluders
	GL_SelectTexture(0);

	backEnd.currentSpace = NULL;

	static float	s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};

	float	viewMatrix[16];

	idVec3	vec;
	idVec3	origin = vLight->lightDef->globalLightOrigin;

/*
Trying to fix some math here, keeping these here as reference.

id rotation: 0.000000 -0.000000 -89.999992
id rotation: 179.999985 -0.000000 -89.999992
id rotation: -89.999992 -0.000000 0.000000
id rotation: -89.999992 -0.000000 179.999985
id rotation: -89.999992 -0.000000 -89.999992
id rotation: 89.999992 -0.000000 -89.999992

fixed rotation: 0.000000 -0.000000 89.999992
fixed rotation: 179.999985 -0.000000 -89.999992
fixed rotation: 89.999992 -0.000000 179.999985
fixed rotation: -89.999992 -0.000000 0.000000
fixed rotation: 89.999992 -89.999992 0.000000
fixed rotation: -89.999992 89.999992 0.000000

		x	72.0000000	float
		y	8.00000000	float
		z	104.000000	float

ID
		[12]	-72.0000000	float
		[13]	-104.000000	float
		[14]	8.00000000	float

Up
	yaw = 90
	pitch = 0;
	roll = -90

Down
	yaw = 90
	pitch = 0;
	roll = 90
*/
	if (side == -1) {
		// projected light
		vec = vLight->lightDef->parms.target;
		vec.Normalize();
		viewMatrix[0] = vec[0];
		viewMatrix[4] = vec[1];
		viewMatrix[8] = vec[2];

		vec = vLight->lightDef->parms.right;
		vec.Normalize();
		viewMatrix[1] = -vec[0];
		viewMatrix[5] = -vec[1];
		viewMatrix[9] = -vec[2];

		vec = vLight->lightDef->parms.up;
		vec.Normalize();
		viewMatrix[2] = vec[0];
		viewMatrix[6] = vec[1];
		viewMatrix[10] = vec[2];
	}
	else {
#if 1
		idAngles rotation;

		rotation.yaw = 0;
		rotation.pitch = 0;
		rotation.roll = 0;

		// side of a point light
		memset(viewMatrix, 0, sizeof(viewMatrix));
		switch (side) {
		case 0:
			rotation.yaw = 0;
			rotation.pitch = -0;
			rotation.roll = 90;
			break;
		case 1:
			rotation.yaw = 180;
			rotation.pitch = -0;
			rotation.roll = -90;
			break;
		case 2:
			rotation.yaw = 90;
			rotation.pitch = -0;
			rotation.roll = 0;
			break;
		case 3:
			rotation.yaw = -90;
			rotation.pitch = -0;
			rotation.roll = 180;
			break;
		case 4:
			rotation.yaw = 90;
			rotation.pitch = -90;
			rotation.roll = 0;
			break;
		case 5:
			rotation.yaw = -90;
			rotation.pitch = 90;
			rotation.roll = 0;
			break;

		default:
			return;

		}
		//static float yaw = 0;
		//static float pitch = 0;
		//static float roll = 0;
		//rotation.yaw = yaw;
		//rotation.pitch = pitch;
		//rotation.roll = roll;
#else
		idAngles rotation;

		// side of a point light
		memset(viewMatrix, 0, sizeof(viewMatrix));
		switch (side) {
		case 0:
			rotation.yaw = 0;
			rotation.pitch = -0;
			rotation.roll = -90;
			break;
		case 1:
			rotation.yaw = 180;
			rotation.pitch = -0;
			rotation.roll = -90;
			break;
		case 2:
			rotation.yaw = -90;
			rotation.pitch = -0;
			rotation.roll = 0;
			break;
		case 3:
			rotation.yaw = -90;
			rotation.pitch = -0;
			rotation.roll = 180;
			break;
		case 4:
			rotation.yaw = -90;
			rotation.pitch = -0;
			rotation.roll = -90;
			break;
		case 5:
			rotation.yaw = 90;
			rotation.pitch = -0;
			rotation.roll = -90;
			break;
		}
#endif

		memcpy(viewMatrix, rotation.ToMat4().ToFloatPtr(), sizeof(float) * 16);
	}

#if 0
	viewMatrix[12] = -origin[0];
	viewMatrix[13] = -origin[2];
	viewMatrix[14] = origin[1];
#else
	viewMatrix[12] = -origin[0] * viewMatrix[0] + -origin[1] * viewMatrix[4] + -origin[2] * viewMatrix[8];
	viewMatrix[13] = -origin[0] * viewMatrix[1] + -origin[1] * viewMatrix[5] + -origin[2] * viewMatrix[9];
	viewMatrix[14] = -origin[0] * viewMatrix[2] + -origin[1] * viewMatrix[6] + -origin[2] * viewMatrix[10];
#endif

	viewMatrix[3] = 0;
	viewMatrix[7] = 0;
	viewMatrix[11] = 0;
	viewMatrix[15] = 1;

	memcpy(unflippedLightMatrix, viewMatrix, sizeof(unflippedLightMatrix));
	myGlMultMatrix(viewMatrix, s_flipMatrix, lightMatrix);

	// Store the shadow matrix for sampling in the shader.
	idRenderMatrix inverseLightMatrix;
	idRenderMatrix::Transpose(*(idRenderMatrix *)lightMatrix, inverseLightMatrix);
	if (side == -1) {
		idRenderMatrix::Multiply(lightProjectionMatrix, inverseLightMatrix, vLight->lightDef->shadowMatrix[0]);
	}
	else {
		idRenderMatrix::Multiply(lightProjectionMatrix, inverseLightMatrix, vLight->lightDef->shadowMatrix[side]);
	}

	// create frustum planes
	idPlane	globalFrustum[6];

	// near clip
	globalFrustum[0][0] = -viewMatrix[0];
	globalFrustum[0][1] = -viewMatrix[4];
	globalFrustum[0][2] = -viewMatrix[8];
	globalFrustum[0][3] = -(origin[0] * globalFrustum[0][0] + origin[1] * globalFrustum[0][1] + origin[2] * globalFrustum[0][2]);

	// far clip
	globalFrustum[1][0] = viewMatrix[0];
	globalFrustum[1][1] = viewMatrix[4];
	globalFrustum[1][2] = viewMatrix[8];
	globalFrustum[1][3] = -globalFrustum[0][3] - viewLightAxialSize;

	// side clips
	globalFrustum[2][0] = -viewMatrix[0] + viewMatrix[1];
	globalFrustum[2][1] = -viewMatrix[4] + viewMatrix[5];
	globalFrustum[2][2] = -viewMatrix[8] + viewMatrix[9];

	globalFrustum[3][0] = -viewMatrix[0] - viewMatrix[1];
	globalFrustum[3][1] = -viewMatrix[4] - viewMatrix[5];
	globalFrustum[3][2] = -viewMatrix[8] - viewMatrix[9];

	globalFrustum[4][0] = -viewMatrix[0] + viewMatrix[2];
	globalFrustum[4][1] = -viewMatrix[4] + viewMatrix[6];
	globalFrustum[4][2] = -viewMatrix[8] + viewMatrix[10];

	globalFrustum[5][0] = -viewMatrix[0] - viewMatrix[2];
	globalFrustum[5][1] = -viewMatrix[4] - viewMatrix[6];
	globalFrustum[5][2] = -viewMatrix[8] - viewMatrix[10];

	// is this nromalization necessary?
	for (int i = 0; i < 6; i++) {
		globalFrustum[i].ToVec4().ToVec3().Normalize();
	}

	for (int i = 2; i < 6; i++) {
		globalFrustum[i][3] = -(origin * globalFrustum[i].ToVec4().ToVec3());
	}

	//RB_Shadow_CullInteractions(vLight, globalFrustum);
	if (r_shadows.GetBool())
	{
		RB_Shadow_RenderOccluders(vLight);
	}

	GL_Cull(CT_FRONT_SIDED);

	// the current modelView matrix is not valid
	backEnd.currentSpace = NULL;
}

/*
===================
RB_DrawPointlightShadow
===================
*/
void RB_DrawPointlightShadow(idRenderLightCommitted *vLight) {
	int shadowMapId = -1;
	
	// Check to see if this shadow has been cached.
	int cachedShadowMapId = renderShadowSystem.CheckShadowCache(vLight);
	if (cachedShadowMapId != -1) {
		vLight->shadowMapSlice = cachedShadowMapId;
		return;
	}

	// If this is a dynamic light and we are re-rendering, just use the same shadow map we had previously.
	if(cachedShadowMapId != -1) {
		shadowMapId = cachedShadowMapId;
	}
	else {
		shadowMapId = renderShadowSystem.FindNextAvailableShadowMap(vLight, 6);
		if (shadowMapId == -1) {
			common->DWarning("Too many realtime shadow casting lights in the scene!\n");
			return;
		}
	}

	vLight->shadowMapSlice = shadowMapId;
	for(int i = 0; i < 6; i++) {
		rvmRenderShadowAtlasEntry_t* shadowMapEntry;
		
		// Set the initial shadow map slice which is needed for rendering.
		backEnd.currentShadowMapSlice = shadowMapId + i;

		shadowMapEntry = renderShadowSystem.GetShadowAtlasEntry(backEnd.currentShadowMapSlice);

		shadowMapEntry->Mark(vLight->lightDef->parms.uniqueLightId);

		int entrySize = shadowMapEntry->sliceSizeX;
		int entryX = shadowMapEntry->x;
		int entryY = shadowMapEntry->y;

		glViewport(entryX, entryY, entrySize, entrySize);
		glScissor(entryX, entryY, entrySize, entrySize);

		glClear(GL_DEPTH_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 0, 255);

		RB_RenderShadowBuffer(vLight, i);

		//backEnd.c_numShadowMapSlices++;
	}
}

/*
===================
RB_DrawSpotlightShadow
===================
*/
void RB_DrawSpotlightShadow(idRenderLightCommitted* vLight) {
	int shadowMapId = -1;

	// Check to see if this shadow has been cached.
	int cachedShadowMapId = renderShadowSystem.CheckShadowCache(vLight);
	if (cachedShadowMapId != -1 /* && vLight->lightDef->GetLightRenderType() == LIGHT_RENDER_STATIC*/) {
		vLight->shadowMapSlice = cachedShadowMapId;
		return;
	}

	// If this is a dynamic light and we are re-rendering, just use the same shadow map we had previously.
	if (cachedShadowMapId != -1) {
		shadowMapId = cachedShadowMapId;
	}
	else {
		shadowMapId = renderShadowSystem.FindNextAvailableShadowMap(vLight, 1);
		if (shadowMapId == -1) {
			common->DWarning("Too many realtime shadow casting lights in the scene!\n");
			return;
		}
	}

	vLight->shadowMapSlice = shadowMapId;
	backEnd.currentShadowMapSlice = shadowMapId;

	rvmRenderShadowAtlasEntry_t* shadowMapEntry = renderShadowSystem.GetShadowAtlasEntry(shadowMapId);

	shadowMapEntry->Mark(vLight->lightDef->parms.uniqueLightId);

	int entrySize = shadowMapEntry->sliceSizeX;
	int entryX = shadowMapEntry->x;
	int entryY = shadowMapEntry->y;

	glViewport(entryX, entryY, entrySize, entrySize);
	glScissor(entryX, entryY, entrySize, entrySize);

	glClear(GL_DEPTH_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 0, 255);

	RB_RenderShadowBuffer(vLight, -1);

	//backEnd.c_numShadowMapSlices++;
}

/*
======================
idRender::RenderShadowMaps
======================
*/
void idRender::RenderShadowMaps(void) {
	idRenderLightCommitted		*vLight;
	//rvmDeviceDebugMarker deviceDebugMarker("ShadowMaps");

#if !defined(__ANDROID__) //karin: GLES not support ARB program
	glDisable(GL_VERTEX_PROGRAM_ARB);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif

	if(!r_shadowEnableCache.GetBool()) {
		renderShadowSystem.NukeShadowMapCache();
	}

	// Bind our shadow map atlas.
	renderShadowSystem.GetShadowMapDepthAtlasRT()->MakeCurrent();

	// ensures that depth writes are enabled for the depth clear
	GL_State(GLS_DEFAULT);

	//backEnd.c_numShadowMapSlices = 0;

	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {
		// Ambient light doesn't cast shadows.
		//if (vLight->lightDef->parms.ambientLight)
		//	continue;
		//
		if (vLight->lightDef->IsVisible()) {
			vLight->lightDef->lightRendered = false;
			continue;
		}

		vLight->lightDef->lightRendered = true;

		// If this light isn't casting shadows, skip it.
		if (vLight->lightDef->parms.noShadows)
			continue;

		// all light side projections must currently match, so non-centered
		// and non-cubic lights must take the largest length
		viewLightAxialSize = R_Shadow_CalcLightAxialSize(vLight);

		idVec4 lightOrigin(vLight->lightDef->parms.origin.x, vLight->lightDef->parms.origin.y, vLight->lightDef->parms.origin.z, 1.0);
		tr.globalLightOriginParam->SetVectorValue(lightOrigin);

		// Render the pointlight shadows
		if(vLight->lightDef->parms.lightType == LIGHT_TYPE_POINT) {
			RB_DrawPointlightShadow(vLight);
		}
		else {
			RB_DrawSpotlightShadow(vLight);
		}
	}

	// Reset the viewports.
	glViewport(tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
		backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
		backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1);

	// the scissor may be smaller than the viewport for subviews
	glScissor(tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
		backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
		backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1);

	// Reset the rendertarget.	
	idRenderTexture::BindNull();

	// Reset the bound shader.
	tr.shadowMapProgram[PROG_VARIANT_NONSKINNED]->BindNull();

	tr.atlasLookupParam->SetImage(renderShadowSystem.GetAtlasLookupImage());
	tr.shadowMapAtlasParam->SetImage(renderShadowSystem.GetShadowMapDepthAtlas());
}
