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

/*

  back end scene + lights rendering functions

*/
#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
// Hack brightness slider to increase lighting in the game
float RB_overbright = 1;
#endif


/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters(const srfTriangles_t *tri)
{
	HARM_CHECK_SHADER("RB_DrawElementsWithCounters");

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += tri->numIndexes;
	backEnd.pc.c_drawVertexes += tri->numVerts;

	if (tri->ambientSurface != NULL) {
		if (tri->indexes == tri->ambientSurface->indexes) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}

		if (tri->verts == tri->ambientSurface->verts) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	if (tri->indexCache) {
		qglDrawElements(GL_TRIANGLES,
		                r_singleTriangle.GetBool() ? 3 : tri->numIndexes,
		                GL_INDEX_TYPE,
		                (int *)vertexCache.Position(tri->indexCache));
		backEnd.pc.c_vboIndexes += tri->numIndexes;
	} else {
		vertexCache.UnbindIndex();

		qglDrawElements(GL_TRIANGLES,
		                r_singleTriangle.GetBool() ? 3 : tri->numIndexes,
		                GL_INDEX_TYPE,
		                tri->indexes);
	}
}

/*
================
RB_DrawShadowElementsWithCounters

May not use all the indexes in the surface if caps are skipped
================
*/
void RB_DrawShadowElementsWithCounters(const srfTriangles_t *tri, int numIndexes)
{
	HARM_CHECK_SHADER("RB_DrawShadowElementsWithCounters");

	backEnd.pc.c_shadowElements++;
	backEnd.pc.c_shadowIndexes += numIndexes;
	backEnd.pc.c_shadowVertexes += tri->numVerts;

	if (tri->indexCache) {
		qglDrawElements(GL_TRIANGLES,
		                r_singleTriangle.GetBool() ? 3 : numIndexes,
		                GL_INDEX_TYPE,
		                (int *)vertexCache.Position(tri->indexCache));
		backEnd.pc.c_vboIndexes += numIndexes;
	} else {
		vertexCache.UnbindIndex();

		qglDrawElements(GL_TRIANGLES,
		                r_singleTriangle.GetBool() ? 3 : numIndexes,
		                GL_INDEX_TYPE,
		                tri->indexes);
	}
}


/*
===============
RB_RenderTriangleSurface

Sets texcoord and vertex pointers
===============
*/
void RB_RenderTriangleSurface(const srfTriangles_t *tri)
{
	if (!tri->ambientCache) {
		return;
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position(tri->ambientCache);
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());

	RB_DrawElementsWithCounters(tri);
}

/*
===============
RB_T_RenderTriangleSurface

===============
*/
void RB_T_RenderTriangleSurface(const drawSurf_t *surf)
{
	RB_RenderTriangleSurface(surf->geo);
}

/*
===============
RB_EnterWeaponDepthHack
===============
*/
void RB_EnterWeaponDepthHack(const drawSurf_t *surf)
{
	qglDepthRangef(0, 0.5);

	float	matrix[16];

	memcpy(matrix, backEnd.viewDef->projectionMatrix, sizeof(matrix));

	matrix[14] *= 0.25;

	float	mat[16];
	myGlMultMatrix(surf->space->modelViewMatrix, matrix, mat);
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);
}

/*
===============
RB_EnterModelDepthHack
===============
*/
void RB_EnterModelDepthHack(const drawSurf_t *surf)
{
	qglDepthRangef(0.0f, 1.0f);

	float	matrix[16];

	memcpy(matrix, backEnd.viewDef->projectionMatrix, sizeof(matrix));

	matrix[14] -= surf->space->modelDepthHack;

	float	mat[16];
	myGlMultMatrix(surf->space->modelViewMatrix, matrix, mat);
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);
}

/*
===============
RB_LeaveDepthHack
===============
*/
void RB_LeaveDepthHack(const drawSurf_t *surf)
{
	qglDepthRangef(0, 1);

	float	mat[16];
	myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);
}

/*
====================
RB_RenderDrawSurfListWithFunction

The triangle functions can check backEnd.currentSpace != surf->space
to see if they need to perform any new matrix setup.  The modelview
matrix will already have been loaded, and backEnd.currentSpace will
be updated after the triangle function completes.
====================
*/
void RB_RenderDrawSurfListWithFunction(drawSurf_t **drawSurfs, int numDrawSurfs,
                                       void (*triFunc_)(const drawSurf_t *))
{
	int				i;
	const drawSurf_t		*drawSurf;

	backEnd.currentSpace = NULL;

	for (i = 0  ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		// change the matrix if needed
		if (drawSurf->space != backEnd.currentSpace) {
			float	mat[16];
			myGlMultMatrix(drawSurf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);

			// we need the model matrix without it being combined with the view matrix
			// so we can transform local vectors to global coordinates
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), drawSurf->space->modelMatrix);
		}

		if (drawSurf->space->weaponDepthHack) {
			RB_EnterWeaponDepthHack(drawSurf);
		}

		if (drawSurf->space->modelDepthHack != 0.0f) {
			RB_EnterModelDepthHack(drawSurf);
		}

		// change the scissor if needed
		if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(drawSurf->scissorRect)) {
			backEnd.currentScissor = drawSurf->scissorRect;
			qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
		}

		// render it
		triFunc_(drawSurf);

		if (drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f) {
			RB_LeaveDepthHack(drawSurf);
		}

		backEnd.currentSpace = drawSurf->space;
	}

	backEnd.currentSpace = NULL; //k2023
}

/*
======================
RB_RenderDrawSurfChainWithFunction
======================
*/
void RB_RenderDrawSurfChainWithFunction(const drawSurf_t *drawSurfs,
                                        void (*triFunc_)(const drawSurf_t *))
{
	const drawSurf_t		*drawSurf;

	backEnd.currentSpace = NULL;

	for (drawSurf = drawSurfs ; drawSurf ; drawSurf = drawSurf->nextOnLight) {
		// change the matrix if needed
		if (drawSurf->space != backEnd.currentSpace) {
			float	mat[16];
			myGlMultMatrix(drawSurf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);

			// we need the model matrix without it being combined with the view matrix
			// so we can transform local vectors to global coordinates
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), drawSurf->space->modelMatrix);
		}

		if (drawSurf->space->weaponDepthHack) {
			RB_EnterWeaponDepthHack(drawSurf);
		}

		if (drawSurf->space->modelDepthHack) {
			RB_EnterModelDepthHack(drawSurf);
		}

		// change the scissor if needed
		if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(drawSurf->scissorRect)) {
			backEnd.currentScissor = drawSurf->scissorRect;
			qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
		}

		// render it
		triFunc_(drawSurf);

		if (drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f) {
			RB_LeaveDepthHack(drawSurf);
		}

		backEnd.currentSpace = drawSurf->space;
	}

	backEnd.currentSpace = NULL; //k2023
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix(const float *shaderRegisters,
                               const textureStage_t *texture, float matrix[16]) //k2023
{
	matrix[0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[4] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[8] = 0;
	matrix[12] = shaderRegisters[ texture->matrix[0][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if ( matrix[12] < -40 || matrix[12] > 40 ) {
		matrix[12] -= (int)matrix[12];
	}

	matrix[1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[5] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[9] = 0;
	matrix[13] = shaderRegisters[ texture->matrix[1][2] ];
	if ( matrix[13] < -40 || matrix[13] > 40 ) {
		matrix[13] -= (int)matrix[13];
	}

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = 1;
	matrix[14] = 0;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
void RB_LoadShaderTextureMatrix(const float *shaderRegisters, const textureStage_t *texture)
{
	float	matrix[16];

	if (texture->hasMatrix) {
		RB_GetShaderTextureMatrix(shaderRegisters, texture, matrix);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
	} else {
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
	}
}

void RB_LoadShaderTextureMatrix(const float *shaderRegisters, const textureStage_t *texture, bool transpose)
{
	float	matrix[16];

	if (texture->hasMatrix) {
		RB_GetShaderTextureMatrix(shaderRegisters, texture, matrix);
		if(transpose)
			R_TransposeGLMatrix(matrix);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
	} else {
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
	}
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void RB_BindVariableStageImage(const textureStage_t *texture, const float *shaderRegisters)
{
	if (texture->cinematic) {
		cinData_t	cin;

		if (r_skipDynamicTextures.GetBool()) {
			globalImages->defaultImage->Bind();
			return;
		}

#ifdef _MULTITHREAD
		if(multithreadActive)
		{
			if ( texture->image ) {
				// The first time the image will be invalid so wont bind, so bind black image
				bool res = true;
				texture->image->Bind(&res);
				if ( !res )
				{
					globalImages->blackImage->Bind();
				}
				// Save time to display
				texture->image->cinmaticNextTime = (int)(1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) );
				// Update next time
				renderThread->AddAllocList( texture->image, true, true );
			}
		}
		else
		{
#endif
		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime((int)(1000 * (backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11])));

		if (cin.image) {
			globalImages->cinematicImage->UploadScratch(cin.image, cin.imageWidth, cin.imageHeight);
		} else {
			globalImages->blackImage->Bind();
		}
#ifdef _MULTITHREAD
		}
#endif
	} else {
		//FIXME: see why image is invalid
		if (texture->image) {
			texture->image->Bind();
		}
	}
}

/*
======================
RB_FinishStageTexture
======================
*/
void RB_FinishStageTexture(const textureStage_t *texture, const drawSurf_t *surf)
{
	if (texture->texgen == TG_DIFFUSE_CUBE || texture->texgen == TG_SKYBOX_CUBE
	    || texture->texgen == TG_WOBBLESKY_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert),
		                       (void *)&(((idDrawVert *)vertexCache.Position(surf->geo->ambientCache))->st));
	}

	if (texture->hasMatrix) {
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
	}
}



//=============================================================================================


/*
=================
RB_DetermineLightScale

Sets:
backEnd.lightScale
backEnd.overBright

Find out how much we are going to need to overscale the lighting, so we
can down modulate the pre-lighting passes.

We only look at light calculations, but an argument could be made that
we should also look at surface evaluations, which would let surfaces
overbright past 1.0
=================
*/
void RB_DetermineLightScale(void)
{
	viewLight_t			*vLight;
	const idMaterial	*shader;
	float				max;
	int					i, j, numStages;
	const shaderStage_t	*stage;

	// the light scale will be based on the largest color component of any surface
	// that will be drawn.
	// should we consider separating rgb scales?

	// if there are no lights, this will remain at 1.0, so GUI-only
	// rendering will not lose any bits of precision
	max = 1.0;

	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) {
		// lights with no surfaces or shaderparms may still be present
		// for debug display
		if (!vLight->localInteractions && !vLight->globalInteractions
		    && !vLight->translucentInteractions) {
			continue;
		}

		shader = vLight->lightShader;
		numStages = shader->GetNumStages();

		for (i = 0 ; i < numStages ; i++) {
			stage = shader->GetStage(i);

			for (j = 0 ; j < 3 ; j++) {
				float	v = r_lightScale.GetFloat() * vLight->shaderRegisters[ stage->color.registers[j] ];

				if (v > max) {
					max = v;
				}
			}
		}
	}

	backEnd.pc.maxLightValue = max;

	if (max <= tr.backEndRendererMaxLight) {
		backEnd.lightScale = r_lightScale.GetFloat();
		backEnd.overBright = 1.0;
	} else {
		backEnd.lightScale = r_lightScale.GetFloat() * tr.backEndRendererMaxLight / max;
		backEnd.overBright = max / tr.backEndRendererMaxLight;
	}
}


/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView(void)
{
	// set the window clipping
	qglViewport(tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
	            tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
	            backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
	            backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1);

	// the scissor may be smaller than the viewport for subviews
	qglScissor(tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
	           tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
	           backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
	           backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1);
	backEnd.currentScissor = backEnd.viewDef->scissor;

	// ensures that depth writes are enabled for the depth clear
	GL_State(GLS_DEFAULT);

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if (backEnd.viewDef->viewEntitys) {
		qglStencilMask(0xff);
		// some cards may have 7 bit stencil buffers, so don't assume this
		// should be 128
		qglClearStencil(1<<(glConfig.stencilBits-1));
		qglClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		qglEnable(GL_DEPTH_TEST);
	} else {
		qglDisable(GL_DEPTH_TEST);
		qglDisable(GL_STENCIL_TEST);
	}

	backEnd.glState.faceCulling = -1;		// force face culling to set next time
	GL_Cull(CT_FRONT_SIDED);

}

/*
==================
R_SetDrawInteractions
==================
*/
void R_SetDrawInteraction(const shaderStage_t *surfaceStage, const float *surfaceRegs,
                          idImage **image, idVec4 matrix[2], float color[4])
{
	*image = surfaceStage->texture.image;

	if (surfaceStage->texture.hasMatrix) {
		matrix[0][0] = surfaceRegs[surfaceStage->texture.matrix[0][0]];
		matrix[0][1] = surfaceRegs[surfaceStage->texture.matrix[0][1]];
		matrix[0][2] = 0;
		matrix[0][3] = surfaceRegs[surfaceStage->texture.matrix[0][2]];

		matrix[1][0] = surfaceRegs[surfaceStage->texture.matrix[1][0]];
		matrix[1][1] = surfaceRegs[surfaceStage->texture.matrix[1][1]];
		matrix[1][2] = 0;
		matrix[1][3] = surfaceRegs[surfaceStage->texture.matrix[1][2]];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if (matrix[0][3] < -40 || matrix[0][3] > 40) {
			matrix[0][3] -= (int)matrix[0][3];
		}

		if (matrix[1][3] < -40 || matrix[1][3] > 40) {
			matrix[1][3] -= (int)matrix[1][3];
		}
	} else {
		matrix[0][0] = 1;
		matrix[0][1] = 0;
		matrix[0][2] = 0;
		matrix[0][3] = 0;

		matrix[1][0] = 0;
		matrix[1][1] = 1;
		matrix[1][2] = 0;
		matrix[1][3] = 0;
	}

	if (color) {
		for (int i = 0 ; i < 4 ; i++) {
			color[i] = surfaceRegs[surfaceStage->color.registers[i]];

			// clamp here, so card with greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			if (color[i] < 0) {
				color[i] = 0;
			} else if (color[i] > 1.0) {
				color[i] = 1.0;
			}
		}
	}
}

/*
=================
RB_SubmittInteraction
=================
*/
static void RB_SubmittInteraction(drawInteraction_t *din, void (*DrawInteraction)(const drawInteraction_t *))
{
	if (!din->bumpImage) {
		return;
	}

	if (!din->diffuseImage || r_skipDiffuse.GetBool()) {
		din->diffuseImage = globalImages->blackImage;
	}

	if (!din->specularImage || r_skipSpecular.GetBool() || din->ambientLight) {
		din->specularImage = globalImages->blackImage;
	}

	if (!din->bumpImage || r_skipBump.GetBool()) {
		din->bumpImage = globalImages->flatNormalMap;
	}

	// if we wouldn't draw anything, don't call the Draw function
	if (
	        ((din->diffuseColor[0] > 0 ||
	          din->diffuseColor[1] > 0 ||
	          din->diffuseColor[2] > 0) && din->diffuseImage != globalImages->blackImage)
	        || ((din->specularColor[0] > 0 ||
	             din->specularColor[1] > 0 ||
	             din->specularColor[2] > 0) && din->specularImage != globalImages->blackImage)) {
#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
		if(RB_overbright > 1.0f)
		{
			din->diffuseColor[0] *= RB_overbright;
			din->diffuseColor[1] *= RB_overbright;
			din->diffuseColor[2] *= RB_overbright;
		}
#endif
		DrawInteraction(din);
	}
}

/*
=============
RB_CreateSingleDrawInteractions

This can be used by different draw_* backends to decompose a complex light / surface
interaction into primitive interactions
=============
*/
void RB_CreateSingleDrawInteractions(const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *))
{
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const viewLight_t	*vLight = backEnd.vLight;
	const idMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;
	drawInteraction_t	inter;

	if (r_skipInteractions.GetBool() || !surf->geo || !surf->geo->ambientCache) {
		return;
	}

	if (tr.logFile) {
		RB_LogComment("---------- RB_CreateSingleDrawInteractions %s on %s ----------\n", lightShader->GetName(), surfaceShader->GetName());
	}

	// change the matrix and light projection vectors if needed
	if (surf->space != backEnd.currentSpace) {

		float	mat[16];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);

		// we need the model matrix without it being combined with the view matrix
		// so we can transform local vectors to global coordinates
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), surf->space->modelMatrix);

        GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewMatrix), surf->space->modelViewMatrix);
	}

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	// hack depth range if needed
	if (surf->space->weaponDepthHack) {
		RB_EnterWeaponDepthHack(surf);
	}

	if (surf->space->modelDepthHack) {
		RB_EnterModelDepthHack(surf);
	}

	inter.surf = surf;
	inter.lightFalloffImage = vLight->falloffImage;

	R_GlobalPointToLocal(surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3());
	R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3());
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.ambientLight = lightShader->IsAmbientLight();

	// the base projections may be modified by texture matrix on light stages
	idPlane lightProject[4];

	for (int i = 0 ; i < 4 ; i++) {
		R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i]);
	}

	for (int lightStageNum = 0 ; lightStageNum < lightShader->GetNumStages() ; lightStageNum++) {
		const shaderStage_t	*lightStage = lightShader->GetStage(lightStageNum);

		// ignore stages that fail the condition
		if (!lightRegs[ lightStage->conditionRegister ]) {
			continue;
		}

		inter.lightImage = lightStage->texture.image;

		memcpy(inter.lightProjection, lightProject, sizeof(inter.lightProjection));

		// now multiply the texgen by the light texture matrix
		if (lightStage->texture.hasMatrix) {
			RB_GetShaderTextureMatrix(lightRegs, &lightStage->texture, backEnd.lightTextureMatrix);
			RB_BakeTextureMatrixIntoTexgen(reinterpret_cast<class idPlane *>(inter.lightProjection), backEnd.lightTextureMatrix); //k2023
		}

		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		float lightColor[4];

		// backEnd.lightScale is calculated so that lightColor[] will never exceed
		// tr.backEndRendererMaxLight
		lightColor[0] = backEnd.lightScale * lightRegs[ lightStage->color.registers[0] ];
		lightColor[1] = backEnd.lightScale * lightRegs[ lightStage->color.registers[1] ];
		lightColor[2] = backEnd.lightScale * lightRegs[ lightStage->color.registers[2] ];
		lightColor[3] = lightRegs[ lightStage->color.registers[3] ];

		// go through the individual stages
		for (int surfaceStageNum = 0 ; surfaceStageNum < surfaceShader->GetNumStages() ; surfaceStageNum++) {
			const shaderStage_t	*surfaceStage = surfaceShader->GetStage(surfaceStageNum);

			switch (surfaceStage->lighting) {
				case SL_AMBIENT: {
					// ignore ambient stages while drawing interactions
					break;
				}
				case SL_BUMP: {
					// ignore stage that fails the condition
					if (!surfaceRegs[ surfaceStage->conditionRegister ]) {
						break;
					}

					// draw any previous interaction
					RB_SubmittInteraction(&inter, DrawInteraction);
					inter.diffuseImage = NULL;
					inter.specularImage = NULL;
					R_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL);
					break;
				}
				case SL_DIFFUSE: {
					// ignore stage that fails the condition
					if (!surfaceRegs[ surfaceStage->conditionRegister ]) {
						break;
					}

					if (inter.diffuseImage) {
						RB_SubmittInteraction(&inter, DrawInteraction);
					}

					R_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.diffuseImage,
					                     inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr());
					inter.diffuseColor[0] *= lightColor[0];
					inter.diffuseColor[1] *= lightColor[1];
					inter.diffuseColor[2] *= lightColor[2];
					inter.diffuseColor[3] *= lightColor[3];
					inter.vertexColor = surfaceStage->vertexColor;
					break;
				}
				case SL_SPECULAR: {
					// ignore stage that fails the condition
					if (!surfaceRegs[ surfaceStage->conditionRegister ]) {
						break;
					}

					if (inter.specularImage) {
						RB_SubmittInteraction(&inter, DrawInteraction);
					}

					R_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.specularImage,
					                     inter.specularMatrix, inter.specularColor.ToFloatPtr());
					inter.specularColor[0] *= lightColor[0];
					inter.specularColor[1] *= lightColor[1];
					inter.specularColor[2] *= lightColor[2];
					inter.specularColor[3] *= lightColor[3];
					inter.vertexColor = surfaceStage->vertexColor;
					break;
				}
			}
		}

		// draw the final interaction
		RB_SubmittInteraction(&inter, DrawInteraction);
	}

	// unhack depth range if needed
	if (surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f) {
		RB_LeaveDepthHack(surf);
	}

	backEnd.currentSpace = surf->space; //k2023
}

/*
=============
RB_DrawView
=============
*/
void RB_DrawView(const void *data)
{
	const drawSurfsCommand_t	*cmd;

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.viewDef = cmd->viewDef;

	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if (!backEnd.viewDef->numDrawSurfs) {
		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if (r_skipRender.GetBool() && backEnd.viewDef->viewEntitys) {
		return;
	}

	// skip render context sets the context to NULL,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	if (r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys) {
		GLimp_DeactivateContext();
	}

	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;

	RB_ShowOverdraw();

	// render the scene, jumping to the hardware specific interaction renderers
	RB_STD_DrawView();

	// restore the context for 2D drawing if we were stubbing it out
	if (r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys) {
		GLimp_ActivateContext();
		RB_SetDefaultGLState();
	}
}

#ifdef _GLOBAL_ILLUMINATION

#define BLEND_NORMALS 1

void RB_CreateSingleDrawGlobalIllumination(const drawSurf_t *drawSurf, void (*DrawInteraction)(const drawInteraction_t *))
{
    if(!drawSurf)
        return;

    if (!drawSurf->geo || !drawSurf->geo->ambientCache) {
        return;
    }

    const idMaterial* surfaceMaterial = drawSurf->material;

    // translucent surfaces don't put anything in the depth buffer and don't
    // test against it, which makes them fail the mirror clip plane operation
    if( surfaceMaterial->Coverage() == MC_TRANSLUCENT )
    {
        return;
    }

    // get the expressions for conditionals / color / texcoords
    const float* surfaceRegs = drawSurf->shaderRegisters;

    // if all stages of a material have been conditioned off, don't do anything
    int stage = 0;
    for( ; stage < surfaceMaterial->GetNumStages(); stage++ )
    {
        const shaderStage_t* pStage = surfaceMaterial->GetStage( stage );
        // check the stage enable condition
        if( surfaceRegs[ pStage->conditionRegister ] != 0 )
        {
            break;
        }
    }
    if( stage == surfaceMaterial->GetNumStages() )
    {
        return;
    }

    // change the matrix if needed
    if( drawSurf->space != backEnd.currentSpace )
    {
        float	mat[16];
        myGlMultMatrix(drawSurf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
        GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);

        // we need the model matrix without it being combined with the view matrix
        // so we can transform local vectors to global coordinates
        GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), drawSurf->space->modelMatrix);

        GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewMatrix), drawSurf->space->modelViewMatrix);
    }

    // change the scissor if needed
    if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(drawSurf->scissorRect)) {
        backEnd.currentScissor = drawSurf->scissorRect;
        qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
                   backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
                   backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
                   backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
    }

    // hack depth range if needed
    if (drawSurf->space->weaponDepthHack) {
        RB_EnterWeaponDepthHack(drawSurf);
    }

    if (drawSurf->space->modelDepthHack) {
        RB_EnterModelDepthHack(drawSurf);
    }

    drawInteraction_t inter;
    memset(&inter, 0, sizeof(inter));
    inter.surf = drawSurf;

    // tranform the view origin into model local space
    R_GlobalPointToLocal( drawSurf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3() );
    inter.localViewOrigin[3] = 1.0f;
    inter.ambientLight = true;

    inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 1.0f;
    inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0.0f;

    inter.bumpImage = NULL;
    inter.specularImage = NULL;
    inter.diffuseImage = NULL;

    // perforated surfaces may have multiple alpha tested stages
    for( stage = 0; stage < surfaceMaterial->GetNumStages(); stage++ )
    {
        const shaderStage_t* surfaceStage = surfaceMaterial->GetStage( stage );

        switch( surfaceStage->lighting )
        {
            case SL_AMBIENT: {
                // ignore ambient stages while drawing interactions
                break;
            }

            case SL_BUMP: {
                // ignore stage that fails the condition
                if( !surfaceRegs[ surfaceStage->conditionRegister ] )
                {
                    break;
                }
                // draw any previous interaction
                if( inter.bumpImage != NULL )
                {
#if BLEND_NORMALS
                    if( inter.vertexColor == SVC_IGNORE )
                    {
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
                    }
                    else
                    {
                        // RB: this is a bit hacky: use additive blending to blend the normals
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
                    }
#endif
                }

                // draw any previous interaction
                RB_SubmittInteraction(&inter, DrawInteraction);
                inter.diffuseImage = NULL;
                inter.specularImage = NULL;
                R_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL);
                break;
            }

            case SL_DIFFUSE: {
                // ignore stage that fails the condition
                if( !surfaceRegs[ surfaceStage->conditionRegister ] )
                {
                    break;
                }

                // draw any previous interaction
                if( inter.diffuseImage != NULL )
                {
#if BLEND_NORMALS
                    if( inter.vertexColor == SVC_IGNORE )
                    {
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
                    }
                    else
                    {
                        // RB: this is a bit hacky: use additive blending to blend the normals
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
                    }
#endif

                    RB_SubmittInteraction(&inter, DrawInteraction);
                }
                R_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.diffuseImage,
                                     inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr());
                inter.vertexColor = surfaceStage->vertexColor;
                break;
            }

            case SL_SPECULAR: {
                // ignore stage that fails the condition
                if( !surfaceRegs[ surfaceStage->conditionRegister ] )
                {
                    break;
                }
                // draw any previous interaction
                if( inter.specularImage != NULL )
                {
#if BLEND_NORMALS
                    if( inter.vertexColor == SVC_IGNORE )
                    {
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
                    }
                    else
                    {
                        // RB: this is a bit hacky: use additive blending to blend the normals
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
                    }
#endif

                    RB_SubmittInteraction(&inter, DrawInteraction);
                }
                R_SetDrawInteraction(surfaceStage, surfaceRegs, &inter.specularImage, inter.specularMatrix, inter.specularColor.ToFloatPtr());
                inter.vertexColor = surfaceStage->vertexColor;
                break;
            }
        }
    }

    // draw the final interaction
    RB_SubmittInteraction(&inter, DrawInteraction);

    // draw the final interaction
#if BLEND_NORMALS
    if( inter.vertexColor == SVC_IGNORE )
    {
        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
    }
    else
    {
        // RB: this is a bit hacky: use additive blending to blend the normals
        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
    }
#endif

    // unhack depth range if needed
    if (drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f) {
        RB_LeaveDepthHack(drawSurf);
    }

    backEnd.currentSpace = drawSurf->space; //k2023
}
#endif
