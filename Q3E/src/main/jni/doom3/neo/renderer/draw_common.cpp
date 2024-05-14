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

#ifdef _K_DEV //karin: debug shader pass
#define _HARM_SKIP_RENDER_SHADER_PASS
#endif
#ifdef _HARM_SKIP_RENDER_SHADER_PASS
static idCVar harm_r_skipShaderPass("harm_r_skipShaderPass", "0", CVAR_INTEGER|CVAR_RENDERER, "1. TG_EXPLICIT, 2. TG_DIFFUSE_CUBE, 3. TG_REFLECT_CUBE, 4. TG_SKYBOX_CUBE, 5. TG_WOBBLESKY_CUBE, 6. TG_SCREEN, 7. TG_SCREEN2, 8. TG_GLASSWARP, 9000. All. greater than 0: skip, less than 0: only, 0 disabled.");
#endif

/*
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float textureMatrix[16] ) //k2023
{
	float	genMatrix[16];
	float	final[16];

	genMatrix[0] = lightProject[0][0];
	genMatrix[4] = lightProject[0][1];
	genMatrix[8] = lightProject[0][2];
	genMatrix[12] = lightProject[0][3];

	genMatrix[1] = lightProject[1][0];
	genMatrix[5] = lightProject[1][1];
	genMatrix[9] = lightProject[1][2];
	genMatrix[13] = lightProject[1][3];

	genMatrix[2] = 0;
	genMatrix[6] = 0;
	genMatrix[10] = 0;
	genMatrix[14] = 0;

	genMatrix[3] = lightProject[2][0];
	genMatrix[7] = lightProject[2][1];
	genMatrix[11] = lightProject[2][2];
	genMatrix[15] = lightProject[2][3];

	myGlMultMatrix( genMatrix, textureMatrix, final );

	lightProject[0][0] = final[0];
	lightProject[0][1] = final[4];
	lightProject[0][2] = final[8];
	lightProject[0][3] = final[12];

	lightProject[1][0] = final[1];
	lightProject[1][1] = final[5];
	lightProject[1][2] = final[9];
	lightProject[1][3] = final[13];
}

/*
================
RB_PrepareStageTexturing
================
*/
void RB_PrepareStageTexturing(const shaderStage_t *pStage,  const drawSurf_t *surf, idDrawVert *ac)
{
	// set privatePolygonOffset if necessary
	if (pStage->privatePolygonOffset) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset);
	}

	// set the texture matrix if needed
	RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);

	// texgens
	if (pStage->texture.texgen == TG_DIFFUSE_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->normal.ToFloatPtr());
	}

	else if (pStage->texture.texgen == TG_SKYBOX_CUBE || pStage->texture.texgen == TG_WOBBLESKY_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, 0,
		                       vertexCache.Position(surf->dynamicTexCoords));
	}

	else if (pStage->texture.texgen == TG_SCREEN || pStage->texture.texgen == TG_SCREEN2)
	{
		RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture, true);
        float mat[16];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
        
        float plane[4];
        plane[0] = mat[0 * 4 + 0];
        plane[1] = mat[1 * 4 + 0];
        plane[2] = mat[2 * 4 + 0];
        plane[3] = mat[3 * 4 + 0];
		GL_Uniform4fv(offsetof(shaderProgram_t, texgenS), plane);
        
        plane[0] = mat[0 * 4 + 1];
        plane[1] = mat[1 * 4 + 1];
        plane[2] = mat[2 * 4 + 1];
        plane[3] = mat[3 * 4 + 1];
		GL_Uniform4fv(offsetof(shaderProgram_t, texgenT), plane);
        
        plane[0] = mat[0 * 4 + 3];
        plane[1] = mat[1 * 4 + 3];
        plane[2] = mat[2 * 4 + 3];
        plane[3] = mat[3 * 4 + 3];
		GL_Uniform4fv(offsetof(shaderProgram_t, texgenQ), plane);
	}

	else if (pStage->texture.texgen == TG_REFLECT_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewMatrix), surf->space->modelViewMatrix);

		float mat[16];
		R_TransposeGLMatrix(backEnd.viewDef->worldSpace.modelViewMatrix, mat);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat);
	}

	else { // explicit
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), reinterpret_cast<void *>(&ac->st));
	}
}

/*
================
RB_FinishStageTexturing
================
*/
void RB_FinishStageTexturing(const shaderStage_t *pStage, const drawSurf_t *surf, idDrawVert *ac)
{
	// unset privatePolygonOffset if necessary
	if (pStage->privatePolygonOffset && !surf->material->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (pStage->texture.texgen == TG_DIFFUSE_CUBE || pStage->texture.texgen == TG_SKYBOX_CUBE
			//k: reflection cubemap
	    || pStage->texture.texgen == TG_REFLECT_CUBE
	    || pStage->texture.texgen == TG_WOBBLESKY_CUBE) {
		 GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), (void *)&ac->st);
	}

	if (pStage->texture.hasMatrix) {
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
	}
}

/*
=============================================================================================

FILL DEPTH BUFFER

=============================================================================================
*/


/*
==================
RB_T_FillDepthBuffer
==================
*/
void RB_T_FillDepthBuffer(const drawSurf_t *surf)
{
	int			stage;
	const idMaterial	*shader;
	const shaderStage_t *pStage;
	const float	*regs;
	float		color[4];
	const srfTriangles_t	*tri;
	const float	one[1] = { 1 };

	tri = surf->geo;
	shader = surf->material;

	if (backEnd.viewDef->numClipPlanes && surf->space != backEnd.currentSpace) {
		idPlane plane;

		R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.viewDef->clipPlanes[0], plane);
		plane[3] += 0.5;  // the notch is in the middle

		GL_Uniform4fv(offsetof(shaderProgram_t, clipPlane), plane.ToFloatPtr());
	}

	if (!shader->IsDrawn()) {
		return;
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if (!tri->numIndexes) {
		return;
	}

	// translucent surfaces don't put anything in the depth buffer and don't
	// test against it, which makes them fail the mirror clip plane operation
	if (shader->Coverage() == MC_TRANSLUCENT) {
		return;
	}

	if (!tri->ambientCache) {
		common->Printf("RB_T_FillDepthBuffer: !tri->ambientCache\n");
		return;
	}

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// if all stages of a material have been conditioned off, don't do anything
	for (stage = 0; stage < shader->GetNumStages() ; stage++) {
		pStage = shader->GetStage(stage);

		// check the stage enable condition
		if (regs[ pStage->conditionRegister ] != 0) {
			break;
		}
	}

	if (stage == shader->GetNumStages()) {
		return;
	}

	// set polygon offset if necessary
	if ((shader->TestMaterialFlag(MF_POLYGONOFFSET))&&((r_offsetFactor.GetFloat()!=0)&&(r_offsetUnits.GetFloat()!=0))) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	// subviews will just down-modulate the color buffer by overbright
	if (shader->GetSort() == SS_SUBVIEW) {
		qglEnable(GL_BLEND);
		GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS);
		color[0] =
		        color[1] =
		                color[2] = (1.0 / backEnd.overBright);
		color[3] = 1;
	} else {
		// others just draw black
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
		color[3] = 1;
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position(tri->ambientCache);
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), reinterpret_cast<void *>(&ac->st));

	bool drawSolid = false;

	if (shader->Coverage() == MC_OPAQUE) {
		drawSolid = true;
	}

	// we may have multiple alpha tested stages
	if (shader->Coverage() == MC_PERFORATED) {
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool	didDraw = false;

		// perforated surfaces may have multiple alpha tested stages
		for (stage = 0; stage < shader->GetNumStages() ; stage++) {
			pStage = shader->GetStage(stage);

			if (!pStage->hasAlphaTest) {
				continue;
			}

			// check the stage enable condition
			if (regs[ pStage->conditionRegister ] == 0) {
				continue;
			}

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// set the alpha modulate
			color[3] = regs[ pStage->color.registers[3] ];

			// skip the entire stage if alpha would be black
			if (color[3] <= 0) {
				continue;
			}

			GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);
			GL_Uniform1fv(offsetof(shaderProgram_t, alphaTest), &regs[pStage->alphaTestRegister]);

			// bind the texture
			pStage->texture.image->Bind();

			// set texture matrix and texGens
			RB_PrepareStageTexturing(pStage, surf, ac);

			// draw it
			RB_DrawElementsWithCounters(tri);

			RB_FinishStageTexturing(pStage, surf, ac);
		}

		if (!didDraw) {
			drawSolid = true;
		}
	}

	// draw the entire surface solid
	if (drawSolid) {
		GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);
		GL_Uniform1fv(offsetof(shaderProgram_t, alphaTest), one);

		globalImages->whiteImage->Bind();

		// draw it
		RB_DrawElementsWithCounters(tri);
	}


	// reset polygon offset
	if ((shader->TestMaterialFlag(MF_POLYGONOFFSET))&&((r_offsetFactor.GetFloat()!=0)&&(r_offsetUnits.GetFloat()!=0))) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	// reset blending
	if (shader->GetSort() == SS_SUBVIEW) {
		GL_State(GLS_DEPTHFUNC_LESS);
		qglDisable(GL_BLEND);
	}
}

/*
=====================
RB_STD_FillDepthBuffer

If we are rendering a subview with a near clip plane, use a second texture
to force the alpha test to fail when behind that clip plane
=====================
*/
void RB_STD_FillDepthBuffer(drawSurf_t **drawSurfs, int numDrawSurfs)
{
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if (!backEnd.viewDef->viewEntitys) {
		return;
	}

	RB_LogComment("---------- RB_STD_FillDepthBuffer ----------\n");

	if (backEnd.viewDef->numClipPlanes) {
		GL_UseProgram(&depthFillClipShader);
		GL_SelectTexture(1);
		globalImages->alphaNotchImage->Bind();
		GL_SelectTexture(0);
	}
	else
		GL_UseProgram(&depthFillShader);

    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
    GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

	// the first texture will be used for alpha tested surfaces
	//GL_SelectTexture(0); //k2023
	// Texture 0 will be used for alpha tested surfaces. It should be already active.
	// Bind it to white image by default
	globalImages->whiteImage->Bind(); //k2023

	// decal surfaces may enable polygon offset
	qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());

	GL_State(GLS_DEPTHFUNC_LESS);

	// Enable stencil test if we are going to be using it for shadows.
	// If we didn't do this, it would be legal behavior to get z fighting
	// from the ambient pass and the light passes.
	if (r_shadows.GetBool())
	{
	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_ALWAYS, 1, 255);
	}
	RB_RenderDrawSurfListWithFunction(drawSurfs, numDrawSurfs, RB_T_FillDepthBuffer);

	if (backEnd.viewDef->numClipPlanes) {
		GL_SelectTexture(1);
		globalImages->BindNull();
		GL_SelectTexture(0);
	}

    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
    GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

	GL_UseProgram(NULL);
}

/*
=============================================================================================

SHADER PASSES

=============================================================================================
*/

/*
==================
RB_SetProgramEnvironment

Sets variables that can be used by all vertex programs
==================
*/
void RB_SetProgramEnvironment(void)
{
	float	parm[4];
	int		pot;

	// screen power of two correction factor, assuming the copy to _currentRender
	// also copied an extra row and column for the bilerp
	int	 w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
	pot = globalImages->currentRenderImage->uploadWidth;
	parm[0] = (float)w / pot;

	int	 h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
	pot = globalImages->currentRenderImage->uploadHeight;
	parm[1] = (float)h / pot;

	parm[2] = 0;
	parm[3] = 1;

	GL_Uniform4fv(offsetof(shaderProgram_t, nonPowerOfTwo), parm);

	// window coord to 0.0 to 1.0 conversion
	parm[0] = 1.0 / w;
	parm[1] = 1.0 / h;
	parm[2] = 0;
	parm[3] = 1;
	GL_Uniform4fv(offsetof(shaderProgram_t, windowCoords), parm);

	//
	// set eye position in global space
	//
	parm[0] = backEnd.viewDef->renderView.vieworg[0];
	parm[1] = backEnd.viewDef->renderView.vieworg[1];
	parm[2] = backEnd.viewDef->renderView.vieworg[2];
	parm[3] = 1.0;
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, eyeOrigin), parm);
}

/*
==================
RB_SetProgramEnvironmentSpace

Sets variables related to the current space that can be used by all vertex programs
==================
*/
void RB_SetProgramEnvironmentSpace(void)
{
	const struct viewEntity_s *space = backEnd.currentSpace;

	// set eye position in local space
	float	parm[4];
	R_GlobalPointToLocal(space->modelMatrix, backEnd.viewDef->renderView.vieworg, *(idVec3 *)parm);
	parm[3] = 1.0;
	GL_Uniform4fv(offsetof(shaderProgram_t, localEyeOrigin), parm);

	// we need the model matrix without it being combined with the view matrix
	// so we can transform local vectors to global coordinates
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelMatrix), space->modelMatrix);

	// set the modelview matrix for the viewer
	float	mat[16];
	myGlMultMatrix(space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
	GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);
}

/*
==================
RB_STD_T_RenderShaderPasses

This is also called for the generated 2D rendering
==================
*/
void RB_STD_T_RenderShaderPasses(const drawSurf_t *surf, const float mat[16])
{
	int			stage;
	const idMaterial	*shader;
	const shaderStage_t *pStage;
	const float	*regs;
	float		color[4];
	const srfTriangles_t	*tri;
	bool attrIsSet[TG_GLASSWARP - TG_EXPLICIT] = { false };
	bool uniformIsSet[TG_GLASSWARP - TG_EXPLICIT] = { false };
	bool newStageAttrIsSet[SHADER_NEW_STAGE_END - SHADER_NEW_STAGE_BEGIN + 1] = { false };
	bool newStageUniformIsSet[SHADER_NEW_STAGE_END - SHADER_NEW_STAGE_BEGIN + 1] = { false };

	tri = surf->geo;
	shader = surf->material;

#ifdef _NO_LIGHT
	if(r_noLight.GetInteger() > 1)
	{
		if(!shader->HasAmbient() && !shader->ReceivesLighting())
			return;
	}
	else
	{
		if(!shader->HasAmbient())
			return;
	}
#else
	if (!shader->HasAmbient()) {
		return;
	}
#endif

	if (shader->IsPortalSky()) {
		return;
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if (!tri->numIndexes) {
		return;
	}

	if (!tri->ambientCache) {
		common->Printf("RB_T_RenderShaderPasses: !tri->ambientCache\n");
		return;
	}

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// set face culling appropriately
	GL_Cull(shader->GetCullType());

	// set polygon offset if necessary
	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position(tri->ambientCache);

	for (stage = 0; stage < shader->GetNumStages() ; stage++) {
		pStage = shader->GetStage(stage);

#ifdef _HUMANHEAD //k: scope view support
		if(tr.IsScopeView())
		{
			if(pStage->isNotScopeView)
				continue;
		}
		else
		{
			if(pStage->isScopeView)
				continue;
		}
		if(!tr.IsShuttleView())
		{
			if(pStage->isShuttleView)
				continue;
		}
		if (backEnd.viewDef->renderView.viewSpiritEntities)
		{
			if(pStage->isNotSpiritWalk)
				continue;
		}
		else
		{
			if(pStage->isSpiritWalk)
				continue;
		}
#endif

		// check the enable condition
		if (regs[ pStage->conditionRegister ] == 0) {
			continue;
		}

		// skip the stages involved in lighting
#ifdef _NO_LIGHT
		if(r_noLight.GetInteger() > 1)
		{
			if(pStage->lighting != SL_AMBIENT && pStage->lighting != SL_DIFFUSE)
				continue;
		}
		else
		{
			if(pStage->lighting != SL_AMBIENT)
				continue;
		}
#else
		if (pStage->lighting != SL_AMBIENT) {
			continue;
		}
#endif

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE)) {
			continue;
		}
#ifdef _RAVEN //karin: GLSL newShaderStage
		// see if we are a new-style stage
		rvNewShaderStage *newShaderStage = pStage->newShaderStage;

		if (newShaderStage) {
			//--------------------------
			//
			// new style stages GLSL
			//
			//--------------------------
			if ( r_skipNewAmbient.GetBool() ) {
				continue;
			}

			if(!newShaderStage->Bind(regs))
				continue;

			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));
			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_TexCoord));
			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Color));
			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Normal));

			GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
			GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
			GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), &ac->color);
			GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());

			// set standard transformations
			GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelViewProjectionMatrix), mat);

			GL_State( pStage->drawStateBits );

			RB_DrawElementsWithCounters( tri );

			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));
			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_TexCoord));
			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Color));
			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Normal));

			newShaderStage->Unbind();

			continue;
		}
#endif

		// see if we are a new-style stage
		newShaderStage_t *newStage = pStage->newStage;

		if (newStage) {
			//--------------------------
			//
			// new style stages
			//
			//--------------------------
			if ( r_skipNewAmbient.GetBool() ) {
				continue;
			}

			if(newStage->glslProgram <= 0)
				continue;

			const shaderProgram_t *shaderProgram = shaderManager->Find(newStage->glslProgram);
			if(!shaderProgram)
				continue;
			GL_UseProgram((shaderProgram_t *)shaderProgram);
			int type = shaderProgram->type;
			assert(type >= SHADER_NEW_STAGE_BEGIN && type <= SHADER_NEW_STAGE_END);
			int index = type - SHADER_NEW_STAGE_BEGIN;

			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));
			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_TexCoord));
			GL_EnableVertexAttribArray(SHADER_PARM_ADDR(attr_Color));

			if(!newStageAttrIsSet[index])
			{
				GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
				GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
				GL_VertexAttribPointer(SHADER_PARM_ADDR(attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), &ac->color);
				newStageAttrIsSet[index] = true;
			}

			if(!newStageUniformIsSet[index])
			{
				// set standard transformations
				GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelViewProjectionMatrix), mat);
				float projectionMatrix[16];
				R_TransposeGLMatrix(backEnd.viewDef->projectionMatrix, projectionMatrix);
				GL_UniformMatrix4fv(SHADER_PARM_ADDR(projectionMatrix), /*backEnd.viewDef->*/
									projectionMatrix);

				float modelViewMatrix[16];
				R_TransposeGLMatrix(surf->space->modelViewMatrix, modelViewMatrix);
				GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelViewMatrix), /*drawSurf->space->*/
									modelViewMatrix);

				// we need the model matrix without it being combined with the view matrix
				// so we can transform local vectors to global coordinates
				idMat4 modelMatrix;
				memcpy(&modelMatrix, surf->space->modelMatrix, sizeof(modelMatrix));
				modelMatrix.TransposeSelf();
				GL_UniformMatrix4fv(SHADER_PARM_ADDR(modelMatrix), modelMatrix.ToFloatPtr());

				// obsolete: screen power of two correction factor, assuming the copy to _currentRender
				// also copied an extra row and column for the bilerp
				// if is pot, glUniform4f(1, 1, 0, 1);
				// window coord to 0.0 to 1.0 conversion
				RB_SetProgramEnvironment();
				newStageUniformIsSet[index] = true;
			}

			//============================================================================

			// setting local parameters (specified in material definition)
			for ( int i = 0; i < newStage->numVertexParms; i++ ) {
				idVec4 vparm;
				for (int d = 0; d < 4; d++)
					vparm[d] = regs[ newStage->vertexParms[i][d] ];
				GL_Uniform4fv(SHADER_PARMS_ADDR(u_vertexParm, i), vparm.ToFloatPtr());
			}

			// setting textures
			// note: the textures are also bound to TUs at this moment
			for ( int i = 0; i < newStage->numFragmentProgramImages; i++ ) {
				if ( newStage->fragmentProgramImages[i] ) {
					GL_SelectTexture( i );
					newStage->fragmentProgramImages[i]->Bind();
					//GL_Uniform1i(SHADER_PARMS_ADDR(u_fragmentMap, i), i);
				}
			}

			GL_State( pStage->drawStateBits );

			RB_DrawElementsWithCounters( tri );

			for ( int i = 0; i < newStage->numFragmentProgramImages; i++ ) {
				if ( newStage->fragmentProgramImages[i] ) {
					GL_SelectTexture( i );
					globalImages->BindNull();
					//GL_Uniform1i(SHADER_PARMS_ADDR(u_fragmentMap, i), i);
				}
			}
			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Vertex));
			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_TexCoord));
			GL_DisableVertexAttribArray(SHADER_PARM_ADDR(attr_Color));

			GL_SelectTextureForce(0);
			GL_UseProgram(NULL);

			continue;
		}

		//--------------------------
		//
		// old style stages
		//
		//--------------------------

		// set the color
		color[0] = regs[ pStage->color.registers[0] ];
		color[1] = regs[ pStage->color.registers[1] ];
		color[2] = regs[ pStage->color.registers[2] ];
		color[3] = regs[ pStage->color.registers[3] ];

		// skip the entire stage if an add would be black
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE)
		    && color[0] <= 0 && color[1] <= 0 && color[2] <= 0) {
			continue;
		}

		// skip the entire stage if a blend would be completely transparent
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
		    && color[3] <= 0) {
			continue;
		}

		const int texgen = pStage->texture.texgen;
		bool usingTexCoord = true;
#ifdef _HARM_SKIP_RENDER_SHADER_PASS
		int skipShaderPass = harm_r_skipShaderPass.GetInteger();
		if(skipShaderPass == 9000 || skipShaderPass == texgen + 1)
		{
			RB_LogComment("skip\n");
			continue;
		}
		if(skipShaderPass < 0 && -skipShaderPass != texgen + 1)
		{
			RB_LogComment("skip ~\n");
			continue;
		}
#endif

		switch(texgen)
		{
			case TG_SKYBOX_CUBE:
			case TG_WOBBLESKY_CUBE:
				GL_UseProgram(&cubemapShader);
				break;
			case TG_REFLECT_CUBE:
				GL_UseProgram(&reflectionCubemapShader);
				break;
			case TG_DIFFUSE_CUBE:
				GL_UseProgram(&diffuseCubemapShader);
				break;
			//k: D3XP using TG_SCREEN in hell level, shader is texgen
			case TG_SCREEN:
			case TG_SCREEN2:
				GL_UseProgram(&texgenShader);
				usingTexCoord = false;
				break;
			case TG_GLASSWARP: // unused
				continue;
			default:
				GL_UseProgram(&defaultShader);
				break;
		}

		GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

		if(usingTexCoord)
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

		// set attr
		if(!attrIsSet[texgen])
		{
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), (void *)&ac->color);
			attrIsSet[texgen] = true;
		}
		if(!uniformIsSet[texgen])
		{
/*			RB_SetProgramEnvironment();

			RB_SetProgramEnvironmentSpace();
*/ //k2023 not used
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);
			uniformIsSet[texgen] = true;
		}

		// select the vertex color source
		if (pStage->vertexColor != SVC_IGNORE) {
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
		}

		static const float zero[4] = { 0, 0, 0, 0 };
		static const float one[4] = { 1, 1, 1, 1 };
		static const float negOne[4] = { -1, -1, -1, -1 };

		switch (pStage->vertexColor) {
			case SVC_MODULATE:
				GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), one);
				GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), zero);
				break;
			case SVC_INVERSE_MODULATE:
				GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), negOne);
				GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
				break;
			case SVC_IGNORE:
			default:
				GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), zero);
				GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
				break;
		}

		GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);

		// bind the texture
		RB_BindVariableStageImage(&pStage->texture, regs);

		// set the state
#ifdef _NO_LIGHT
		if (r_noLight.GetBool() || shader->IsNoLight())
        {
            if (r_noLight.GetInteger() > 1 && !shader->IsNoLight())
            {
                if (pStage->lighting == SL_AMBIENT)
                    GL_State(pStage->drawStateBits);
                else
                {
                    if (shader->TestMaterialFlag(MF_POLYGONOFFSET))
                        GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE|GLS_DEPTHFUNC_LESS);
                    else
                        GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE|backEnd.depthFunc);
                }
            }
            else/* if (shader->IsNoLight())*/
            {
                if (pStage->drawStateBits!=9000)
                    GL_State(pStage->drawStateBits);
                else
                {
                    if (shader->TestMaterialFlag(MF_POLYGONOFFSET))
                        GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE|GLS_DEPTHFUNC_LESS);
                    else
                        GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE|backEnd.depthFunc);
                }
            }
            // else GL_State(pStage->drawStateBits);
        }
		else
#endif
		GL_State(pStage->drawStateBits);

		RB_PrepareStageTexturing(pStage, surf, ac);

		// draw it
		RB_DrawElementsWithCounters(tri);

		RB_FinishStageTexturing(pStage, surf, ac);

		if (pStage->vertexColor != SVC_IGNORE) {
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
			GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), zero);
			GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
		}

		if(usingTexCoord)
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

		GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	}

	// reset polygon offset
	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	GL_UseProgram(NULL);
}

/*
=====================
RB_STD_DrawShaderPasses

Draw non-light dependent passes
=====================
*/
int RB_STD_DrawShaderPasses(drawSurf_t **drawSurfs, int numDrawSurfs)
{
	int				i;

	// only obey skipAmbient if we are rendering a view
	if (backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool()) {
		return numDrawSurfs;
	}

	RB_LogComment("---------- RB_STD_DrawShaderPasses ----------\n");

	// if we are about to draw the first surface that needs
	// the rendering in a texture, copy it over
	if (drawSurfs[0]->material->GetSort() >= SS_POST_PROCESS
#ifdef _RAVEN //karin: sniper's blur is 2D and has flag `MF_NEED_CURRENT_RENDER`
		|| drawSurfs[0]->material->TestMaterialFlag(MF_NEED_CURRENT_RENDER)
#endif
	) {
		if (r_skipPostProcess.GetBool()) {
			return 0;
		}

		// only dump if in a 3d view
#if !defined(_RAVEN) //karin: sniper's blur is 2D //TODO: check it for avoid unused operation
		if (backEnd.viewDef->viewEntitys)
#endif
		{
			globalImages->currentRenderImage->CopyFramebuffer(backEnd.viewDef->viewport.x1,
			                backEnd.viewDef->viewport.y1,  backEnd.viewDef->viewport.x2 -  backEnd.viewDef->viewport.x1 + 1,
			                backEnd.viewDef->viewport.y2 -  backEnd.viewDef->viewport.y1 + 1, true);
		}

		backEnd.currentRenderCopied = true;
	}

	// we don't use RB_RenderDrawSurfListWithFunction()
	// because we want to defer the matrix load because many
	// surfaces won't draw any ambient passes
	backEnd.currentSpace = NULL;

	float	mat[16];
	for (i = 0  ; i < numDrawSurfs ; i++) {
		if (drawSurfs[i]->material->SuppressInSubview()) {
			continue;
		}

		if (backEnd.viewDef->isXraySubview && drawSurfs[i]->space->entityDef) {
			if (drawSurfs[i]->space->entityDef->parms.xrayIndex != 2) {
				continue;
			}
		}

		// we need to draw the post process shaders after we have drawn the fog lights
		if (drawSurfs[i]->material->GetSort() >= SS_POST_PROCESS
		    && !backEnd.currentRenderCopied) {
			break;
		}
/* //k2023
		GL_SelectTexture(1);
		globalImages->BindNull();

		GL_SelectTexture(0);*/

		// Change the MVP matrix if needed
		if (drawSurfs[i]->space != backEnd.currentSpace) {
			RB_ComputeMVP(drawSurfs[i], mat);
			// We can't set the uniform now, as we still don't know which shader to use
		}

		// Hack Depth Range if necessary
		bool bNeedRestoreDepthRange = false;
		if (drawSurfs[i]->space->weaponDepthHack && drawSurfs[i]->space->modelDepthHack == 0.0f) {
			qglDepthRangef(0.0f, 0.5f);
			bNeedRestoreDepthRange = true;
		}

		{
			RB_STD_T_RenderShaderPasses(drawSurfs[i], mat);
		}

		if (bNeedRestoreDepthRange) {
			qglDepthRangef(0.0f, 1.0f);
		}

		backEnd.currentSpace = drawSurfs[i]->space;
	}

	backEnd.currentSpace = NULL; //k2023

	GL_Cull(CT_FRONT_SIDED);


	return i;
}



/*
==============================================================================

BACK END RENDERING OF STENCIL SHADOWS

==============================================================================
*/

/*
=====================
RB_T_Shadow

the shadow volumes face INSIDE
=====================
*/
static void RB_T_Shadow(const drawSurf_t *surf)
{
	const srfTriangles_t	*tri;

	tri = surf->geo;

	if (!tri->shadowCache) {
		return;
	}

	// set the light position for the vertex program to project the rear surfaces
	if (surf->space != backEnd.currentSpace) {
		idVec4 localLight;

		R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight.ToVec3());
		localLight.w = 0.0f;
		GL_Uniform4fv(offsetof(shaderProgram_t, localLightOrigin), localLight.ToFloatPtr());
/* //k2023
		// set the modelview matrix for the viewer
		float	mat[16];

		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);*/
	}

	tri = surf->geo;

	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 4, GL_FLOAT, false, sizeof(shadowCache_t),
			vertexCache.Position(tri->shadowCache));

	// we always draw the sil planes, but we may not need to draw the front or rear caps
	int	numIndexes;
	bool external = false;

	if (!r_useExternalShadows.GetInteger()) {
		numIndexes = tri->numIndexes;
	} else if (r_useExternalShadows.GetInteger() == 2) {   // force to no caps for testing
		numIndexes = tri->numShadowIndexesNoCaps;
	} else if (!(surf->dsFlags & DSF_VIEW_INSIDE_SHADOW)) {
		// if we aren't inside the shadow projection, no caps are ever needed needed
		numIndexes = tri->numShadowIndexesNoCaps;
		external = true;
	} else if (!backEnd.vLight->viewInsideLight && !(surf->geo->shadowCapPlaneBits & SHADOW_CAP_INFINITE)) {
		// if we are inside the shadow projection, but outside the light, and drawing
		// a non-infinite shadow, we can skip some caps
		if (backEnd.vLight->viewSeesShadowPlaneBits & surf->geo->shadowCapPlaneBits) {
			// we can see through a rear cap, so we need to draw it, but we can skip the
			// caps on the actual surface
			numIndexes = tri->numShadowIndexesNoFrontCaps;
		} else {
			// we don't need to draw any caps
			numIndexes = tri->numShadowIndexesNoCaps;
		}

		external = true;
	} else {
		// must draw everything
		numIndexes = tri->numIndexes;
	}

	// debug visualization
	if (r_showShadows.GetInteger()) {
		float color[4];

		if (r_showShadows.GetInteger() == 3) {
			if (external) {
				color[0] = 0.1;
				color[1] = 1;
				color[2] = 0.1;
			} else {
				// these are the surfaces that require the reverse
				color[0] = 1;
				color[1] = 0.1;
				color[2] = 0.1;
			}
		} else {
			// draw different color for turboshadows
			if (surf->geo->shadowCapPlaneBits & SHADOW_CAP_INFINITE) {
				if (numIndexes == tri->numIndexes) {
					color[0] = 1;
					color[1] = 0.1;
					color[2] = 0.1;
				} else {
					color[0] = 1;
					color[1] = 0.4;
					color[2] = 0.1;
				}
			} else {
				if (numIndexes == tri->numIndexes) {
					color[0] = 0.1;
					color[1] = 1;
					color[2] = 0.1;
				} else if (numIndexes == tri->numShadowIndexesNoFrontCaps) {
					color[0] = 0.1;
					color[1] = 1;
					color[2] = 0.6;
				} else {
					color[0] = 0.6;
					color[1] = 1;
					color[2] = 0.1;
				}
			}
		}

		color[0] /= backEnd.overBright;
		color[1] /= backEnd.overBright;
		color[2] /= backEnd.overBright;
		color[3] = 1;
		GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);

		qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		qglDisable(GL_STENCIL_TEST);
		GL_Cull(CT_TWO_SIDED);
		RB_DrawShadowElementsWithCounters(tri, numIndexes);
		GL_Cull(CT_FRONT_SIDED);
		if (r_shadows.GetBool())
		qglEnable(GL_STENCIL_TEST);

		return;
	}

	// depth-fail stencil shadows
	if(!harm_r_shadowCarmackInverse.GetBool())
	{
		GLenum firstFace = backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK;
		GLenum secondFace = backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT;
		if ( !external ) {
			qglStencilOpSeparate( firstFace, GL_KEEP, tr.stencilDecr, tr.stencilDecr );
			qglStencilOpSeparate( secondFace, GL_KEEP, tr.stencilIncr, tr.stencilIncr );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );
		}

		qglStencilOpSeparate( firstFace, GL_KEEP, GL_KEEP, tr.stencilIncr );
		qglStencilOpSeparate( secondFace, GL_KEEP, GL_KEEP, tr.stencilDecr );

		RB_DrawShadowElementsWithCounters( tri, numIndexes );
	}
	else
	{
		if (!external) {
			qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, tr.stencilDecr, GL_KEEP);
			qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, tr.stencilIncr, GL_KEEP);
		} else {
			// traditional depth-pass stencil shadows
			qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, tr.stencilIncr);
			qglStencilOpSeparate(backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, tr.stencilDecr);
		}
		RB_DrawShadowElementsWithCounters(tri, numIndexes);
	}
}

/*
=====================
RB_StencilShadowPass

Stencil test should already be enabled, and the stencil buffer should have
been set to 128 on any surfaces that might receive shadows
=====================
*/
void RB_StencilShadowPass(const drawSurf_t *drawSurfs)
{
	if (!r_shadows.GetBool()) {
		return;
	}

	if (!drawSurfs) {
		return;
	}

	RB_LogComment("---------- RB_StencilShadowPass ----------\n");

	// Use the stencil shadow shader
	GL_UseProgram(&shadowShader);

	// Setup attributes arrays
	// Vertex attribute is always enabled
	// Disable Color attribute (as it is enabled by default)
	// Disable TexCoord attribute (as it is enabled by default)
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	globalImages->BindNull();

	// for visualizing the shadows
	if (r_showShadows.GetInteger()) {
		if (r_showShadows.GetInteger() == 2) {
			// draw filled in
			GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS);
		} else {
			// draw as lines, filling the depth buffer
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS);
		}
	} else {
		// don't write to the color buffer, just the stencil buffer
		GL_State(GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS);
	}

	if (r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat()) {
		qglPolygonOffset(r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat());
		qglEnable(GL_POLYGON_OFFSET_FILL);
	}

	qglStencilFunc(GL_ALWAYS, 1, 255);

	GL_Cull(CT_TWO_SIDED);

	RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_Shadow);

	GL_Cull(CT_FRONT_SIDED);

	if (r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat()) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}

	qglStencilFunc(GL_GEQUAL, 128, 255);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	GL_UseProgram(NULL);
}



/*
=============================================================================================

BLEND LIGHT PROJECTION

=============================================================================================
*/

/*
=====================
RB_T_BlendLight

=====================
*/
static void RB_T_BlendLight(const drawSurf_t *surf)
{
	const srfTriangles_t *tri;
	const viewLight_t *vLight = backEnd.vLight;

	tri = surf->geo;

	// Setup the fogMatrix as being the local Light Projection
	// Only do this once per space
	if (backEnd.currentSpace != surf->space) {
		idPlane lightProject[4];

		int i;
		for (i = 0; i < 4; i++) {
			R_GlobalPlaneToLocal(surf->space->modelMatrix, vLight->lightProject[i], lightProject[i]);
		}

		idMat4 fogMatrix;
		fogMatrix[0] = lightProject[0].ToVec4();
		fogMatrix[1] = lightProject[1].ToVec4();
		fogMatrix[2] = lightProject[2].ToVec4();
		fogMatrix[3] = lightProject[3].ToVec4();
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, fogMatrix), fogMatrix.ToFloatPtr());
	}

	// This gets used for both blend lights and shadow draws
	if (tri->ambientCache) {
		idDrawVert *ac = (idDrawVert *) vertexCache.Position(tri->ambientCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	} else if (tri->shadowCache) {
		shadowCache_t *sc = (shadowCache_t *) vertexCache.Position(tri->shadowCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), sc->xyz.ToFloatPtr());
	}

	RB_DrawElementsWithCounters(tri);
}


/*
=====================
RB_BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
static void RB_BlendLight(const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2)
{
	const viewLight_t *vLight = backEnd.vLight;
	const idMaterial * const lightShader = vLight->lightShader;
	const float * const regs = vLight->shaderRegisters;

	if (!drawSurfs) {
		return;
	}

	if (r_skipBlendLights.GetBool()) {
		return;
	}

	RB_LogComment("---------- RB_BlendLight ----------\n");

	// Use blendLight shader
	GL_UseProgram(&blendLightShader);
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	// Texture 1 will get the falloff texture
	GL_SelectTexture(1);
	vLight->falloffImage->Bind();

	// Texture 0 will get the projected texture
	GL_SelectTexture(0);

	int i;
	for (i = 0; i < lightShader->GetNumStages(); i++) {
		const shaderStage_t *stage = lightShader->GetStage(i);

		if (!regs[stage->conditionRegister]) {
			continue;
		}

		// Setup the drawState
		GL_State(GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL);

		// Bind the projected texture
		stage->texture.image->Bind();

		// Setup the texture matrix
		if ( stage->texture.hasMatrix ) {
			float matrix[16];
			RB_GetShaderTextureMatrix(regs, &stage->texture, matrix);
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), matrix);
		}

		// Setup the Fog Color
		float lightColor[4];
		lightColor[0] = regs[stage->color.registers[0]];
		lightColor[1] = regs[stage->color.registers[1]];
		lightColor[2] = regs[stage->color.registers[2]];
		lightColor[3] = regs[stage->color.registers[3]];
		GL_Uniform4fv(offsetof(shaderProgram_t, fogColor), lightColor);

		RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BlendLight);
		RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BlendLight);

		// Restore texture matrix to identity
		if (stage->texture.hasMatrix) {
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
		}
	}

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	GL_UseProgram(NULL);
}


//========================================================================

static idPlane	fogPlanes[4];

/*
=====================
RB_T_BasicFog

=====================
*/
static void RB_T_BasicFog(const drawSurf_t *surf)
{
	if ( backEnd.currentSpace != surf->space ) {
		idPlane transfoFogPlane[4];

		//S
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[0], transfoFogPlane[0]);
		transfoFogPlane[0][3] += 0.5;
		//T
		transfoFogPlane[1][0] = transfoFogPlane[1][1] = transfoFogPlane[1][2] = 0;
		transfoFogPlane[1][3] = 0.5;
		//T
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[2], transfoFogPlane[2]);
		transfoFogPlane[2][3] += FOG_ENTER;
		//S
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[3], transfoFogPlane[3]);

		idMat4 fogMatrix;
		fogMatrix[0] = transfoFogPlane[0].ToVec4();
		fogMatrix[1] = transfoFogPlane[1].ToVec4();
		fogMatrix[2] = transfoFogPlane[2].ToVec4();
		fogMatrix[3] = transfoFogPlane[3].ToVec4();

		GL_UniformMatrix4fv(offsetof(shaderProgram_t, fogMatrix), fogMatrix.ToFloatPtr());
	}

	const srfTriangles_t *tri = surf->geo;
	idDrawVert* ac = (idDrawVert*) vertexCache.Position(tri->ambientCache);

	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());

	RB_DrawElementsWithCounters(tri);
}



/*
==================
RB_FogPass
==================
*/
static void RB_FogPass(const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2)
{
	const viewLight_t* vLight = backEnd.vLight;

	drawSurf_t ds;
	const idMaterial* lightShader;
	const shaderStage_t* stage;
	const float* regs;

	// create a surface for the light frustom triangles, which are oriented drawn side out
	const srfTriangles_t* frustumTris = vLight->frustumTris;

	RB_LogComment("---------- RB_FogPass ----------\n");

	// if we ran out of vertex cache memory, skip it
	if ( !frustumTris->ambientCache ) {
		return;
	}

	// Initial expected GL state:
	// Texture 0 is active, and bound to NULL
	// Vertex attribute array is enabled
	// All other attributes array are disabled
	// No shaders active

	GL_UseProgram(&fogShader);

	// Setup attributes arrays
	// Vertex attribute is always enabled
	// Disable Color attribute (as it is enabled by default)
	// Disable TexCoord attribute (as it is enabled by default)
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	memset(&ds, 0, sizeof(ds));
	ds.space = &backEnd.viewDef->worldSpace;
	ds.geo = frustumTris;
	ds.scissorRect = backEnd.viewDef->scissor;

	// find the current color and density of the fog
	lightShader = vLight->lightShader;
	regs = vLight->shaderRegisters;
	// assume fog shaders have only a single stage
	stage = lightShader->GetStage(0);

	float lightColor[4];

	lightColor[0] = regs[stage->color.registers[0]];
	lightColor[1] = regs[stage->color.registers[1]];
	lightColor[2] = regs[stage->color.registers[2]];
	lightColor[3] = regs[stage->color.registers[3]];

	// FogColor
	GL_Uniform4fv(offsetof(shaderProgram_t, fogColor), lightColor);

	// calculate the falloff planes
	const float a = ( lightColor[3] <= 1.0 ) ? -0.5f / DEFAULT_FOG_DISTANCE : -0.5f / lightColor[3];

	// texture 0 is the falloff image
	// It is expected to be already active
	globalImages->fogImage->Bind();

	fogPlanes[0][0] = a * backEnd.viewDef->worldSpace.modelViewMatrix[2];
	fogPlanes[0][1] = a * backEnd.viewDef->worldSpace.modelViewMatrix[6];
	fogPlanes[0][2] = a * backEnd.viewDef->worldSpace.modelViewMatrix[10];
	fogPlanes[0][3] = a * backEnd.viewDef->worldSpace.modelViewMatrix[14];

	fogPlanes[1][0] = a * backEnd.viewDef->worldSpace.modelViewMatrix[0];
	fogPlanes[1][1] = a * backEnd.viewDef->worldSpace.modelViewMatrix[4];
	fogPlanes[1][2] = a * backEnd.viewDef->worldSpace.modelViewMatrix[8];
	fogPlanes[1][3] = a * backEnd.viewDef->worldSpace.modelViewMatrix[12];

	// texture 1 is the entering plane fade correction
	GL_SelectTexture(1);
	globalImages->fogEnterImage->Bind();
	// reactive texture 0 for next passes
	GL_SelectTexture(0);

	// T will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2][0] = 0.001f * vLight->fogPlane[0];
	fogPlanes[2][1] = 0.001f * vLight->fogPlane[1];
	fogPlanes[2][2] = 0.001f * vLight->fogPlane[2];
	fogPlanes[2][3] = 0.001f * vLight->fogPlane[3];

	// S is based on the view origin
	const float s = backEnd.viewDef->renderView.vieworg * fogPlanes[2].Normal() + fogPlanes[2][3];
	fogPlanes[3][0] = 0;
	fogPlanes[3][1] = 0;
	fogPlanes[3][2] = 0;
	fogPlanes[3][3] = FOG_ENTER + s;

	// draw it
	GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
	RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BasicFog);
	RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BasicFog);

	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS);
	GL_Cull(CT_BACK_SIDED);
	RB_RenderDrawSurfChainWithFunction(&ds, RB_T_BasicFog);
	// Restore culling
	GL_Cull(CT_FRONT_SIDED);
	GL_State(GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL); // Restore DepthFunc

	// Restore attributes arrays
	// Vertex attribute is always enabled
	// Re-enable Color attribute (as it is enabled by default)
	// Re-enable TexCoord attribute (as it is enabled by default)
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));

	GL_UseProgram(NULL);
}


/*
==================
RB_STD_FogAllLights
==================
*/
void RB_STD_FogAllLights(void)
{
	if (r_skipFogLights.GetBool() || backEnd.viewDef->isXraySubview /* dont fog in xray mode*/ ) {
		return;
	}

	RB_LogComment("---------- RB_STD_FogAllLights ----------\n");

	// Disable Stencil Test
	qglDisable(GL_STENCIL_TEST);

	// Disable TexCoord array
	// Disable Color array
	// GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	// GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));

	//////////////////
	// For each Light
	//////////////////

	viewLight_t *vLight;
	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {
		backEnd.vLight = vLight;

		// We are only interested in Fog and Blend lights
		if (!vLight->lightShader->IsFogLight() && !vLight->lightShader->IsBlendLight()) {
			continue;
		}

		if (vLight->lightShader->IsFogLight()) {
			RB_FogPass(vLight->globalInteractions, vLight->localInteractions);
		} else if (vLight->lightShader->IsBlendLight()) {
			RB_BlendLight(vLight->globalInteractions, vLight->localInteractions);
		}
	}

	// Re-enable TexCoord array
	// Re-enable Color array
	// GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	// GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
	// Re-enable Stencil Test
	qglEnable(GL_STENCIL_TEST);
}

//=========================================================================================

/*
==================
RB_STD_LightScale

Perform extra blending passes to multiply the entire buffer by
a floating point value
==================
*/
void RB_STD_LightScale(void)
{
//#warning RB_STD_LightScale
#if 0	//!defined(GL_ES_VERSION_2_0)
	float	v, f;

	if (backEnd.overBright == 1.0f) {
		return;
	}

	if (r_skipLightScale.GetBool()) {
		return;
	}

	RB_LogComment("---------- RB_STD_LightScale ----------\n");

	// the scissor may be smaller than the viewport for subviews
	if (r_useScissor.GetBool()) {
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
		           backEnd.viewDef->scissor.x2 - backEnd.viewDef->scissor.x1 + 1,
		           backEnd.viewDef->scissor.y2 - backEnd.viewDef->scissor.y1 + 1);
		backEnd.currentScissor = backEnd.viewDef->scissor;
	}

	// full screen blends
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR);
	GL_Cull(CT_TWO_SIDED);	// so mirror views also get it
	globalImages->BindNull();
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_STENCIL_TEST);

	v = 1;

	while (idMath::Fabs(v - backEnd.overBright) > 0.01) {	// a little extra slop
		f = backEnd.overBright / v;
		f /= 2;

		if (f > 1) {
			f = 1;
		}

		glColor4f(f, f, f, 1);
		v = v * f * 2;

		glBegin(GL_QUADS);
		glVertex2f(0,0);
		glVertex2f(0,1);
		glVertex2f(1,1);
		glVertex2f(1,0);
		glEnd();
	}


	glPopMatrix();
	qglEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	GL_Cull(CT_FRONT_SIDED);
#endif
}

//=========================================================================================

/*
=============
RB_STD_DrawView

=============
*/
void	RB_STD_DrawView(void)
{
	drawSurf_t	 **drawSurfs;
	int			numDrawSurfs;

	RB_LogComment("---------- RB_STD_DrawView ----------\n");

	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

	drawSurfs = (drawSurf_t **)&backEnd.viewDef->drawSurfs[0];
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	// clear the z buffer, set the projection matrix, etc
	RB_BeginDrawingView();

	// decide how much overbrighting we are going to do
	RB_DetermineLightScale();

	// fill the depth buffer and clear color buffer to black except on
	// subviews
	qglDisable(GL_BLEND);
	RB_STD_FillDepthBuffer(drawSurfs, numDrawSurfs);
	// main light renderer
	qglEnable(GL_BLEND);

#ifdef _NO_LIGHT
	if (!r_noLight.GetBool())
#endif
		RB_GLSL_DrawInteractions();

	// disable stencil shadow test
	qglStencilFunc(GL_ALWAYS, 128, 255);

	// uplight the entire screen to crutch up not having better blending range
	RB_STD_LightScale();

	// now draw any non-light dependent shading passes
	int	processed = RB_STD_DrawShaderPasses(drawSurfs, numDrawSurfs);

	// fob and blend lights
	RB_STD_FogAllLights();

	// now draw any post-processing effects using _currentRender
	if (processed < numDrawSurfs) {
		RB_STD_DrawShaderPasses(drawSurfs+processed, numDrawSurfs-processed);
	}
	#ifndef ANDROID_NOGLERRHACK
	RB_RenderDebugTools(drawSurfs, numDrawSurfs);
	#endif
}
