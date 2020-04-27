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
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen(idPlane lightProject[3])
{
	float	genMatrix[16];
	float	final[16];

	genMatrix[0] = lightProject[0][0];
	genMatrix[1] = lightProject[0][1];
	genMatrix[2] = lightProject[0][2];
	genMatrix[3] = lightProject[0][3];

	genMatrix[4] = lightProject[1][0];
	genMatrix[5] = lightProject[1][1];
	genMatrix[6] = lightProject[1][2];
	genMatrix[7] = lightProject[1][3];

	genMatrix[8] = 0;
	genMatrix[9] = 0;
	genMatrix[10] = 0;
	genMatrix[11] = 0;

	genMatrix[12] = lightProject[2][0];
	genMatrix[13] = lightProject[2][1];
	genMatrix[14] = lightProject[2][2];
	genMatrix[15] = lightProject[2][3];

	myGlMultMatrix(genMatrix, backEnd.lightTextureMatrix, final);

	lightProject[0][0] = final[0];
	lightProject[0][1] = final[1];
	lightProject[0][2] = final[2];
	lightProject[0][3] = final[3];

	lightProject[1][0] = final[4];
	lightProject[1][1] = final[5];
	lightProject[1][2] = final[6];
	lightProject[1][3] = final[7];
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
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset);
	}

	// set the texture matrix if needed
	RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);

	// texgens
	if (pStage->texture.texgen == TG_DIFFUSE_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, sizeof(idDrawVert),
		                       ac->normal.ToFloatPtr());
	}

	if (pStage->texture.texgen == TG_SKYBOX_CUBE || pStage->texture.texgen == TG_WOBBLESKY_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 3, GL_FLOAT, false, 0,
		                       vertexCache.Position(surf->dynamicTexCoords));
	}

#if !defined(GL_ES_VERSION_2_0)
#if 0
	if (pStage->texture.texgen == TG_SCREEN) {
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_Q);

		float	mat[16], plane[4];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);

		plane[0] = mat[0];
		plane[1] = mat[4];
		plane[2] = mat[8];
		plane[3] = mat[12];
		glTexGenfv(GL_S, GL_OBJECT_PLANE, plane);

		plane[0] = mat[1];
		plane[1] = mat[5];
		plane[2] = mat[9];
		plane[3] = mat[13];
		glTexGenfv(GL_T, GL_OBJECT_PLANE, plane);

		plane[0] = mat[3];
		plane[1] = mat[7];
		plane[2] = mat[11];
		plane[3] = mat[15];
		glTexGenfv(GL_Q, GL_OBJECT_PLANE, plane);
	}

	if (pStage->texture.texgen == TG_SCREEN2) {
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_Q);

		float	mat[16], plane[4];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);

		plane[0] = mat[0];
		plane[1] = mat[4];
		plane[2] = mat[8];
		plane[3] = mat[12];
		glTexGenfv(GL_S, GL_OBJECT_PLANE, plane);

		plane[0] = mat[1];
		plane[1] = mat[5];
		plane[2] = mat[9];
		plane[3] = mat[13];
		glTexGenfv(GL_T, GL_OBJECT_PLANE, plane);

		plane[0] = mat[3];
		plane[1] = mat[7];
		plane[2] = mat[11];
		plane[3] = mat[15];
		glTexGenfv(GL_Q, GL_OBJECT_PLANE, plane);
	}

	if (pStage->texture.texgen == TG_GLASSWARP) {
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FPROG_GLASSWARP);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);

		GL_SelectTexture(2);
		globalImages->scratchImage->Bind();

		GL_SelectTexture(1);
		globalImages->scratchImage2->Bind();

		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_Q);

		float	mat[16], plane[4];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);

		plane[0] = mat[0];
		plane[1] = mat[4];
		plane[2] = mat[8];
		plane[3] = mat[12];
		glTexGenfv(GL_S, GL_OBJECT_PLANE, plane);

		plane[0] = mat[1];
		plane[1] = mat[5];
		plane[2] = mat[9];
		plane[3] = mat[13];
		glTexGenfv(GL_T, GL_OBJECT_PLANE, plane);

		plane[0] = mat[3];
		plane[1] = mat[7];
		plane[2] = mat[11];
		plane[3] = mat[15];
		glTexGenfv(GL_Q, GL_OBJECT_PLANE, plane);

		GL_SelectTexture(0);
	}

	if (pStage->texture.texgen == TG_REFLECT_CUBE) {
		// see if there is also a bump map specified
		const shaderStage_t *bumpStage = surf->material->GetBumpStage();

		if (bumpStage) {
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture(1);
			bumpStage->texture.image->Bind();
			GL_SelectTexture(0);

			glNormalPointer(GL_FLOAT, sizeof(idDrawVert), ac->normal.ToFloatPtr());
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());

			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));

			// Program env 5, 6, 7, 8 have been set in RB_SetProgramEnvironmentSpace

			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_BUMPY_ENVIRONMENT);
			glEnable(GL_VERTEX_PROGRAM_ARB);
		} else {
			// per-pixel reflection mapping without a normal map
			glNormalPointer(GL_FLOAT, sizeof(idDrawVert), ac->normal.ToFloatPtr());
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));

			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FPROG_ENVIRONMENT);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VPROG_ENVIRONMENT);
			glEnable(GL_VERTEX_PROGRAM_ARB);
		}
	}
#endif
#endif
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
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (pStage->texture.texgen == TG_DIFFUSE_CUBE || pStage->texture.texgen == TG_SKYBOX_CUBE
	    || pStage->texture.texgen == TG_WOBBLESKY_CUBE) {
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), (void *)&ac->st);
	}

#if !defined(GL_ES_VERSION_2_0)
#if 0
	if (pStage->texture.texgen == TG_SCREEN) {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_Q);
	}

	if (pStage->texture.texgen == TG_SCREEN2) {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_Q);
	}

	if (pStage->texture.texgen == TG_GLASSWARP) {
		GL_SelectTexture(2);
		globalImages->BindNull();

		GL_SelectTexture(1);

		RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_Q);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		globalImages->BindNull();
		GL_SelectTexture(0);
	}

	if (pStage->texture.texgen == TG_REFLECT_CUBE) {
		// see if there is also a bump map specified
		const shaderStage_t *bumpStage = surf->material->GetBumpStage();

		if (bumpStage) {
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture(1);
			globalImages->BindNull();
			GL_SelectTexture(0);

			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
		} else {
			// per-pixel reflection mapping without bump mapping
		}

		GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glDisable(GL_VERTEX_PROGRAM_ARB);
	}
#endif
#endif

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

#warning TODO
#if !defined(GL_ES_VERSION_2_0)
	// update the clip plane if needed
	if (backEnd.viewDef->numClipPlanes && surf->space != backEnd.currentSpace) {
		GL_SelectTexture(1);

		idPlane	plane;

		R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.viewDef->clipPlanes[0], plane);
		plane[3] += 0.5;	// the notch is in the middle
		glTexGenfv(GL_S, GL_OBJECT_PLANE, plane.ToFloatPtr());
		GL_SelectTexture(0);
	}
#endif

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
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	// subviews will just down-modulate the color buffer by overbright
	if (shader->GetSort() == SS_SUBVIEW) {
		glEnable(GL_BLEND);
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
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
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
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	// reset blending
	if (shader->GetSort() == SS_SUBVIEW) {
		GL_State(GLS_DEPTHFUNC_LESS);
		glDisable(GL_BLEND);
	}

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
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

	GL_UseProgram(&depthFillShader);

#warning unimplemented in GLES shaders
#if !defined(GL_ES_VERSION_2_0)
	// enable the second texture for mirror plane clipping if needed
	if (backEnd.viewDef->numClipPlanes) {
		GL_SelectTexture(1);
		globalImages->alphaNotchImage->Bind();
		GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
		glEnable(GL_TEXTURE_GEN_S);
		glTexCoord2f(1, 0.5);
	}
#endif

	// the first texture will be used for alpha tested surfaces
	GL_SelectTexture(0);
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

	// decal surfaces may enable polygon offset
	glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());

	GL_State(GLS_DEPTHFUNC_LESS);

	// Enable stencil test if we are going to be using it for shadows.
	// If we didn't do this, it would be legal behavior to get z fighting
	// from the ambient pass and the light passes.
	if (r_shadows.GetBool())
	{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 255);
	}
	RB_RenderDrawSurfListWithFunction(drawSurfs, numDrawSurfs, RB_T_FillDepthBuffer);

#if !defined(GL_ES_VERSION_2_0)
	if (backEnd.viewDef->numClipPlanes) {
		GL_SelectTexture(1);
		globalImages->BindNull();
		glDisable(GL_TEXTURE_GEN_S);
		GL_SelectTexture(0);
	}
#endif

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

#if 0
	// screen power of two correction factor, one pixel in so we don't get a bilerp
	// of an uncopied pixel
	int	 w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
	pot = globalImages->currentRenderImage->uploadWidth;

	if (w == pot) {
		parm[0] = 1.0;
	} else {
		parm[0] = (float)(w-1) / pot;
	}

	int	 h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
	pot = globalImages->currentRenderImage->uploadHeight;

	if (h == pot) {
		parm[1] = 1.0;
	} else {
		parm[1] = (float)(h-1) / pot;
	}

	parm[2] = 0;
	parm[3] = 1;

	GL_Uniform4fv(offsetof(shaderProgram_t, nonPowerOfTwo), parm);
#else
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
#endif

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
void RB_STD_T_RenderShaderPasses(const drawSurf_t *surf)
{
	int			stage;
	const idMaterial	*shader;
	const shaderStage_t *pStage;
	const float	*regs;
	float		color[4];
	const srfTriangles_t	*tri;

	tri = surf->geo;
	shader = surf->material;

	if (!shader->HasAmbient()) {
		return;
	}

	if (shader->IsPortalSky()) {
		return;
	}

	// change the matrix if needed
	if (surf->space != backEnd.currentSpace) {
		backEnd.currentSpace = surf->space;
		RB_SetProgramEnvironmentSpace();
	}

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(surf->scissorRect)) {
		backEnd.currentScissor = surf->scissorRect;
		glScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if (!tri->numIndexes) {
		return;
	}

	if (!tri->ambientCache) {
		common->Printf("RB_T_RenderShaderPasses: !tri->ambientCache\n");
		return;
	}

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// set face culling appropriately
	GL_Cull(shader->GetCullType());

	// set polygon offset if necessary
	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset());
	}

	if (surf->space->weaponDepthHack) {
		RB_EnterWeaponDepthHack(surf);
	}

	if (surf->space->modelDepthHack != 0.0f) {
		RB_EnterModelDepthHack(surf);
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position(tri->ambientCache);
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), reinterpret_cast<void *>(&ac->st));

	for (stage = 0; stage < shader->GetNumStages() ; stage++) {
		pStage = shader->GetStage(stage);

		// check the enable condition
		if (regs[ pStage->conditionRegister ] == 0) {
			continue;
		}

		// skip the stages involved in lighting
		if (pStage->lighting != SL_AMBIENT) {
			continue;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if ((pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE)) {
			continue;
		}

		// see if we are a new-style stage
		newShaderStage_t *newStage = pStage->newStage;

		if (newStage) {
			//--------------------------
			//
			// new style stages
			//
			//--------------------------

			if (r_skipNewAmbient.GetBool()) {
				continue;
			}

			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), (void *)&ac->color);
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());

			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));

			GL_State(pStage->drawStateBits);

#if 0
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, newStage->vertexProgram);
			glEnable(GL_VERTEX_PROGRAM_ARB);

			// megaTextures bind a lot of images and set a lot of parameters
			if (newStage->megaTexture) {
				newStage->megaTexture->SetMappingForSurface(tri);
				idVec3	localViewer;
				R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewer);
				newStage->megaTexture->BindForViewOrigin(localViewer);
			}

			for (int i = 0 ; i < newStage->numVertexParms ; i++) {
				float	parm[4];
				parm[0] = regs[ newStage->vertexParms[i][0] ];
				parm[1] = regs[ newStage->vertexParms[i][1] ];
				parm[2] = regs[ newStage->vertexParms[i][2] ];
				parm[3] = regs[ newStage->vertexParms[i][3] ];
				glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, parm);
			}

			for (int i = 0 ; i < newStage->numFragmentProgramImages ; i++) {
				if (newStage->fragmentProgramImages[i]) {
					GL_SelectTexture(i);
					newStage->fragmentProgramImages[i]->Bind();
				}
			}

			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, newStage->fragmentProgram);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
#endif

			// draw it
			RB_DrawElementsWithCounters(tri);

#if 0
			for (int i = 1 ; i < newStage->numFragmentProgramImages ; i++) {
				if (newStage->fragmentProgramImages[i]) {
					GL_SelectTexture(i);
					globalImages->BindNull();
				}
			}

			if (newStage->megaTexture) {
				newStage->megaTexture->Unbind();
			}

			GL_SelectTexture(0);

			glDisable(GL_VERTEX_PROGRAM_ARB);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif

			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));	// gl_Color
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Tangent));
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Bitangent));
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Normal));
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

		// select the vertex color source
		if (pStage->vertexColor != SVC_IGNORE) {
			GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), (void *)&ac->color);
			GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
		}

		static const float zero[4] = { 0, 0, 0, 0 };
		static const float one[4] = { 1, 1, 1, 1 };
		static const float negOne[4] = { -1, -1, -1, -1 };

		switch (pStage->vertexColor) {
			case SVC_IGNORE:
				GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), zero);
				GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
				break;
			case SVC_MODULATE:
				GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), one);
				GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), zero);
				break;
			case SVC_INVERSE_MODULATE:
				GL_Uniform4fv(offsetof(shaderProgram_t, colorModulate), negOne);
				GL_Uniform4fv(offsetof(shaderProgram_t, colorAdd), one);
				break;
		}

		GL_Uniform4fv(offsetof(shaderProgram_t, glColor), color);

		// bind the texture
		RB_BindVariableStageImage(&pStage->texture, regs);

		// set the state
		if (r_noLight.GetBool())
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
		else
		{
			GL_State(pStage->drawStateBits);
		}

		RB_PrepareStageTexturing(pStage, surf, ac);

		// draw it
		RB_DrawElementsWithCounters(tri);

		RB_FinishStageTexturing(pStage, surf, ac);

		if (pStage->vertexColor != SVC_IGNORE) {
			GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Color));
		}
	}

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

	// reset polygon offset
	if (shader->TestMaterialFlag(MF_POLYGONOFFSET)) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f) {
		RB_LeaveDepthHack(surf);
	}
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
	if (drawSurfs[0]->material->GetSort() >= SS_POST_PROCESS) {
		if (r_skipPostProcess.GetBool()) {
			return 0;
		}

		// only dump if in a 3d view
		if (backEnd.viewDef->viewEntitys) {
			globalImages->currentRenderImage->CopyFramebuffer(backEnd.viewDef->viewport.x1,
			                backEnd.viewDef->viewport.y1,  backEnd.viewDef->viewport.x2 -  backEnd.viewDef->viewport.x1 + 1,
			                backEnd.viewDef->viewport.y2 -  backEnd.viewDef->viewport.y1 + 1, true);
		}

		backEnd.currentRenderCopied = true;
	}

	GL_UseProgram(&defaultShader);

	GL_SelectTexture(1);
	globalImages->BindNull();

	GL_SelectTexture(0);
	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

	RB_SetProgramEnvironment();

	// we don't use RB_RenderDrawSurfListWithFunction()
	// because we want to defer the matrix load because many
	// surfaces won't draw any ambient passes
	backEnd.currentSpace = NULL;

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

		RB_STD_T_RenderShaderPasses(drawSurfs[i]);
	}

	GL_Cull(CT_FRONT_SIDED);
#if !defined(GL_ES_VERSION_2_0)
	glColor4f(1, 1, 1, 1);
#endif

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));

	GL_UseProgram(NULL);

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

	// set the light position for the vertex program to project the rear surfaces
	if (surf->space != backEnd.currentSpace) {
		idVec4 localLight;

		R_GlobalPointToLocal(surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight.ToVec3());
		localLight.w = 0.0f;
		GL_Uniform4fv(offsetof(shaderProgram_t, localLightOrigin), localLight.ToFloatPtr());

		// set the modelview matrix for the viewer
		float	mat[16];

		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
		GL_UniformMatrix4fv(offsetof(shaderProgram_t, modelViewProjectionMatrix), mat);
	}

	tri = surf->geo;

	if (!tri->shadowCache) {
		return;
	}

	GL_EnableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
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

#if !defined(GL_ES_VERSION_2_0)
	// set depth bounds
	if (glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool()) {
		qglDepthBoundsEXT(surf->scissorRect.zmin, surf->scissorRect.zmax);
	}
#endif

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

		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glDisable(GL_STENCIL_TEST);
		GL_Cull(CT_TWO_SIDED);
		RB_DrawShadowElementsWithCounters(tri, numIndexes);
		GL_Cull(CT_FRONT_SIDED);
		if (r_shadows.GetBool())
		glEnable(GL_STENCIL_TEST);

		return;
	}

	// depth-fail stencil shadows
	if (!external) {
		glStencilOpSeparate(backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, tr.stencilDecr, GL_KEEP);
		glStencilOpSeparate(backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, tr.stencilIncr, GL_KEEP);
	} else {
		// traditional depth-pass stencil shadows
		glStencilOpSeparate(backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, tr.stencilIncr);
		glStencilOpSeparate(backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, tr.stencilDecr);
	}
	GL_Cull(CT_TWO_SIDED);
	RB_DrawShadowElementsWithCounters(tri, numIndexes);

	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_Vertex));
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
		glPolygonOffset(r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat());
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	glStencilFunc(GL_ALWAYS, 1, 255);

#if !defined(GL_ES_VERSION_2_0)
	if (glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool()) {
		glEnable(GL_DEPTH_BOUNDS_TEST_EXT);
	}
#endif

	RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_Shadow);

	GL_Cull(CT_FRONT_SIDED);

	if (r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat()) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

#if !defined(GL_ES_VERSION_2_0)
	if (glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool()) {
		glDisable(GL_DEPTH_BOUNDS_TEST_EXT);
	}
#endif

	glStencilFunc(GL_GEQUAL, 128, 255);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
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
#warning RB_T_BlendLight
#if 0	//!defined(GL_ES_VERSION_2_0)
	const srfTriangles_t *tri;

	tri = surf->geo;

	if (backEnd.currentSpace != surf->space) {
		idPlane	lightProject[4];
		int		i;

		for (i = 0 ; i < 4 ; i++) {
			R_GlobalPlaneToLocal(surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i]);
		}

		GL_SelectTexture(0);
		glTexGenfv(GL_S, GL_OBJECT_PLANE, lightProject[0].ToFloatPtr());
		glTexGenfv(GL_T, GL_OBJECT_PLANE, lightProject[1].ToFloatPtr());
		glTexGenfv(GL_Q, GL_OBJECT_PLANE, lightProject[2].ToFloatPtr());

		GL_SelectTexture(1);
		glTexGenfv(GL_S, GL_OBJECT_PLANE, lightProject[3].ToFloatPtr());
	}

	// this gets used for both blend lights and shadow draws
	if (tri->ambientCache) {
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position(tri->ambientCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	} else if (tri->shadowCache) {
		shadowCache_t	*sc = (shadowCache_t *)vertexCache.Position(tri->shadowCache);
		GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(shadowCache_t), sc->xyz.ToFloatPtr());
	}

	RB_DrawElementsWithCounters(tri);
#endif
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
#warning RB_BlendLight
#if 0	//!defined(GL_ES_VERSION_2_0)
	const idMaterial	*lightShader;
	const shaderStage_t	*stage;
	int					i;
	const float	*regs;

	if (!drawSurfs) {
		return;
	}

	if (r_skipBlendLights.GetBool()) {
		return;
	}

	RB_LogComment("---------- RB_BlendLight ----------\n");

	lightShader = backEnd.vLight->lightShader;
	regs = backEnd.vLight->shaderRegisters;

	// texture 1 will get the falloff texture
	GL_SelectTexture(1);
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	glEnable(GL_TEXTURE_GEN_S);
	glTexCoord2f(0, 0.5);
	backEnd.vLight->falloffImage->Bind();

	// texture 0 will get the projected texture
	GL_SelectTexture(0);
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_Q);

	for (i = 0 ; i < lightShader->GetNumStages() ; i++) {
		stage = lightShader->GetStage(i);

		if (!regs[ stage->conditionRegister ]) {
			continue;
		}

		GL_State(GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL);

		GL_SelectTexture(0);
		stage->texture.image->Bind();

		RB_LoadShaderTextureMatrix(regs, &stage->texture);

		// get the modulate values from the light, including alpha, unlike normal lights
		backEnd.lightColor[0] = regs[ stage->color.registers[0] ];
		backEnd.lightColor[1] = regs[ stage->color.registers[1] ];
		backEnd.lightColor[2] = regs[ stage->color.registers[2] ];
		backEnd.lightColor[3] = regs[ stage->color.registers[3] ];
		glColor4f(backEnd.lightColor[0], backEnd.lightColor[1], backEnd.lightColor[2], backEnd.lightColor[3]);

		RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BlendLight);
		RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BlendLight);

		if (stage->texture.hasMatrix) {
			GL_UniformMatrix4fv(offsetof(shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr());
		}
	}

	GL_SelectTexture(1);
	glDisable(GL_TEXTURE_GEN_S);
	globalImages->BindNull();

	GL_SelectTexture(0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_Q);
#endif
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
#warning
#if 0	//!defined(GL_ES_VERSION_2_0)
	if (backEnd.currentSpace != surf->space) {
		idPlane	local;

		GL_SelectTexture(0);

		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[0], local);
		local[3] += 0.5;
		glTexGenfv(GL_S, GL_OBJECT_PLANE, local.ToFloatPtr());

//		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[1], local );
//		local[3] += 0.5;
		local[0] = local[1] = local[2] = 0;
		local[3] = 0.5;
		glTexGenfv(GL_T, GL_OBJECT_PLANE, local.ToFloatPtr());

		GL_SelectTexture(1);

		// GL_S is constant per viewer
		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[2], local);
		local[3] += FOG_ENTER;
		glTexGenfv(GL_T, GL_OBJECT_PLANE, local.ToFloatPtr());

		R_GlobalPlaneToLocal(surf->space->modelMatrix, fogPlanes[3], local);
		glTexGenfv(GL_S, GL_OBJECT_PLANE, local.ToFloatPtr());
	}

	RB_T_RenderTriangleSurface(surf);
#endif
}



/*
==================
RB_FogPass
==================
*/
static void RB_FogPass(const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2)
{
#warning
#if 0	//!defined(GL_ES_VERSION_2_0)
	const srfTriangles_t *frustumTris;
	drawSurf_t			ds;
	const idMaterial	*lightShader;
	const shaderStage_t	*stage;
	const float			*regs;

	RB_LogComment("---------- RB_FogPass ----------\n");

	// create a surface for the light frustom triangles, which are oriented drawn side out
	frustumTris = backEnd.vLight->frustumTris;

	// if we ran out of vertex cache memory, skip it
	if (!frustumTris->ambientCache) {
		return;
	}

	memset(&ds, 0, sizeof(ds));
	ds.space = &backEnd.viewDef->worldSpace;
	ds.geo = frustumTris;
	ds.scissorRect = backEnd.viewDef->scissor;

	// find the current color and density of the fog
	lightShader = backEnd.vLight->lightShader;
	regs = backEnd.vLight->shaderRegisters;
	// assume fog shaders have only a single stage
	stage = lightShader->GetStage(0);

	backEnd.lightColor[0] = regs[ stage->color.registers[0] ];
	backEnd.lightColor[1] = regs[ stage->color.registers[1] ];
	backEnd.lightColor[2] = regs[ stage->color.registers[2] ];
	backEnd.lightColor[3] = regs[ stage->color.registers[3] ];

	glColor3fv(backEnd.lightColor);

	// calculate the falloff planes
	float	a;

	// if they left the default value on, set a fog distance of 500
	if (backEnd.lightColor[3] <= 1.0) {
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	} else {
		// otherwise, distance = alpha color
		a = -0.5f / backEnd.lightColor[3];
	}

	GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);

	// texture 0 is the falloff image
	GL_SelectTexture(0);
	globalImages->fogImage->Bind();
	//GL_Bind( tr.whiteImage );
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glTexCoord2f(0.5f, 0.5f);		// make sure Q is set

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
	GL_DisableVertexAttribArray(offsetof(shaderProgram_t, attr_TexCoord));
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	// T will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2][0] = 0.001f * backEnd.vLight->fogPlane[0];
	fogPlanes[2][1] = 0.001f * backEnd.vLight->fogPlane[1];
	fogPlanes[2][2] = 0.001f * backEnd.vLight->fogPlane[2];
	fogPlanes[2][3] = 0.001f * backEnd.vLight->fogPlane[3];

	// S is based on the view origin
	float s = backEnd.viewDef->renderView.vieworg * fogPlanes[2].Normal() + fogPlanes[2][3];

	fogPlanes[3][0] = 0;
	fogPlanes[3][1] = 0;
	fogPlanes[3][2] = 0;
	fogPlanes[3][3] = FOG_ENTER + s;

	glTexCoord2f(FOG_ENTER + s, FOG_ENTER);


	// draw it
	RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_BasicFog);
	RB_RenderDrawSurfChainWithFunction(drawSurfs2, RB_T_BasicFog);

	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State(GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS);
	GL_Cull(CT_BACK_SIDED);
	RB_RenderDrawSurfChainWithFunction(&ds, RB_T_BasicFog);
	GL_Cull(CT_FRONT_SIDED);

	GL_SelectTexture(1);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	globalImages->BindNull();

	GL_SelectTexture(0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
#endif
}


/*
==================
RB_STD_FogAllLights
==================
*/
void RB_STD_FogAllLights(void)
{
#warning
#if 0	//!defined(GL_ES_VERSION_2_0)
	viewLight_t	*vLight;

	if (r_skipFogLights.GetBool() || r_showOverDraw.GetInteger() != 0
	    || backEnd.viewDef->isXraySubview /* dont fog in xray mode*/
	   ) {
		return;
	}

	RB_LogComment("---------- RB_STD_FogAllLights ----------\n");

	glDisable(GL_STENCIL_TEST);

	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) {
		backEnd.vLight = vLight;

		if (!vLight->lightShader->IsFogLight() && !vLight->lightShader->IsBlendLight()) {
			continue;
		}

#if 0 // _D3XP disabled that

		if (r_ignore.GetInteger()) {
			// we use the stencil buffer to guarantee that no pixels will be
			// double fogged, which happens in some areas that are thousands of
			// units from the origin
			backEnd.currentScissor = vLight->scissorRect;

			if (r_useScissor.GetBool()) {
				glScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
				           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
			}

			glClear(GL_STENCIL_BUFFER_BIT);

			glEnable(GL_STENCIL_TEST);

			// only pass on the cleared stencil values
			glStencilFunc(GL_EQUAL, 128, 255);

			// when we pass the stencil test and depth test and are going to draw,
			// increment the stencil buffer so we don't ever draw on that pixel again
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		}

#endif

		if (vLight->lightShader->IsFogLight()) {
			RB_FogPass(vLight->globalInteractions, vLight->localInteractions);
		} else if (vLight->lightShader->IsBlendLight()) {
			RB_BlendLight(vLight->globalInteractions, vLight->localInteractions);
		}

		glDisable(GL_STENCIL_TEST);
	}

	glEnable(GL_STENCIL_TEST);
#endif
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
#warning
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
		glScissor(backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
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
#if !defined(GL_ES_VERSION_2_0)
	glOrtho(0, 1, 0, 1, -1, 1);
#else
	glOrthof(0, 1, 0, 1, -1, 1);
#endif

	GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR);
	GL_Cull(CT_TWO_SIDED);	// so mirror views also get it
	globalImages->BindNull();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

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
	glEnable(GL_DEPTH_TEST);
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
	glDisable(GL_BLEND);
	RB_STD_FillDepthBuffer(drawSurfs, numDrawSurfs);
	// main light renderer
	glEnable(GL_BLEND);
	switch (tr.backEndRenderer) {
		case BE_GLSL:
			if (!r_noLight.GetBool())
			RB_GLSL_DrawInteractions();
			break;
	}

	// disable stencil shadow test
	glStencilFunc(GL_ALWAYS, 128, 255);

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
