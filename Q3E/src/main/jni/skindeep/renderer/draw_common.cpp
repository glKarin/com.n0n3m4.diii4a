/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "sys/platform.h"
#include "renderer/VertexCache.h"

#include "renderer/tr_local.h"

/*
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float *textureMatrix ) {
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

	myGlMultMatrix( genMatrix, backEnd.lightTextureMatrix, final );

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
void RB_PrepareStageTexturing( const shaderStage_t *pStage,  const drawSurf_t *surf, idDrawVert *ac ) {
	// set privatePolygonOffset if necessary
	if ( pStage->privatePolygonOffset ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
	}

	// set the texture matrix if needed
	if ( pStage->texture.hasMatrix ) {
		RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );
	}

	// texgens
	if ( pStage->texture.texgen == TG_DIFFUSE_CUBE ) {
		R_GLSL_SetActiveProgram("basicCube");
		qglVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
	}
	if ( pStage->texture.texgen == TG_SKYBOX_CUBE || pStage->texture.texgen == TG_WOBBLESKY_CUBE ) {
		R_GLSL_SetActiveProgram("basicCube");
		qglVertexAttribPointer(1, 3, GL_FLOAT, false, 0, vertexCache.Position(surf->dynamicTexCoords));
	}
	if ( pStage->texture.texgen == TG_SCREEN ) {
		R_GLSL_SetActiveProgram("basicObjectLinear");

		float	mat[16];
		idVec4 plane;
		myGlMultMatrix( surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );

		plane[0] = mat[0];
		plane[1] = mat[4];
		plane[2] = mat[8];
		plane[3] = mat[12];
		R_GLSL_SetProgramEnv(10, plane);


		plane[0] = mat[1];
		plane[1] = mat[5];
		plane[2] = mat[9];
		plane[3] = mat[13];
		R_GLSL_SetProgramEnv(11, plane);

		plane[0] = mat[3];
		plane[1] = mat[7];
		plane[2] = mat[11];
		plane[3] = mat[15];
		R_GLSL_SetProgramEnv(12, plane);
	}

	if ( pStage->texture.texgen == TG_SCREEN2 ) {
		R_GLSL_SetActiveProgram("basicObjectLinear");

		float	mat[16];
		idVec4 plane;
		myGlMultMatrix( surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );

		plane[0] = mat[0];
		plane[1] = mat[4];
		plane[2] = mat[8];
		plane[3] = mat[12];
		R_GLSL_SetProgramEnv(10, plane);

		plane[0] = mat[1];
		plane[1] = mat[5];
		plane[2] = mat[9];
		plane[3] = mat[13];
		R_GLSL_SetProgramEnv(11, plane);

		plane[0] = mat[3];
		plane[1] = mat[7];
		plane[2] = mat[11];
		plane[3] = mat[15];
		R_GLSL_SetProgramEnv(12, plane);
	}

	if ( pStage->texture.texgen == TG_GLASSWARP ) {
		R_GLSL_SetActiveProgram("glasswarp");

		GL_SelectTexture( 2 );
		globalImages->scratchImage->Bind();

		GL_SelectTexture( 1 );
		globalImages->scratchImage2->Bind();

		float	mat[16];
		idVec4 plane;
		myGlMultMatrix( surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );

		plane[0] = mat[0];
		plane[1] = mat[4];
		plane[2] = mat[8];
		plane[3] = mat[12];
		R_GLSL_SetProgramEnv(10, plane);

		plane[0] = mat[1];
		plane[1] = mat[5];
		plane[2] = mat[9];
		plane[3] = mat[13];
		R_GLSL_SetProgramEnv(11, plane);

		plane[0] = mat[3];
		plane[1] = mat[7];
		plane[2] = mat[11];
		plane[3] = mat[15];
		R_GLSL_SetProgramEnv(12, plane);

		GL_SelectTexture( 0 );
	}

	if ( pStage->texture.texgen == TG_REFLECT_CUBE ) {
		// see if there is also a bump map specified
		const shaderStage_t *bumpStage = surf->material->GetBumpStage();
		if (bumpStage) {
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture(1);
			bumpStage->texture.image->Bind();
			GL_SelectTexture(0);
			R_GLSL_SetActiveProgram("bumpyEnvironment");
		} else {
			R_GLSL_SetActiveProgram("environment");
		}
	}
}

/*
================
RB_FinishStageTexturing
================
*/
void RB_FinishStageTexturing( const shaderStage_t *pStage, const drawSurf_t *surf, idDrawVert *ac ) {
	// unset privatePolygonOffset if necessary
	if ( pStage->privatePolygonOffset && !surf->material->TestMaterialFlag(MF_POLYGONOFFSET) ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	if ( pStage->texture.texgen == TG_DIFFUSE_CUBE || pStage->texture.texgen == TG_SKYBOX_CUBE
		|| pStage->texture.texgen == TG_WOBBLESKY_CUBE ) {
		R_GLSL_SetActiveProgram("basicSurf");
		qglVertexAttribPointer(1, 2, GL_FLOAT, false, 0, ac->st.ToFloatPtr());
	}

	if ( pStage->texture.texgen == TG_SCREEN || pStage->texture.texgen == TG_SCREEN2 ) {
		R_GLSL_SetActiveProgram("basicSurf");
	}

	if ( pStage->texture.texgen == TG_GLASSWARP ) {
		GL_SelectTexture( 2 );
		globalImages->BindNull();

		GL_SelectTexture( 1 );
		if ( pStage->texture.hasMatrix ) {
			RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );
		}

		R_GLSL_SetActiveProgram("basicSurf");

		globalImages->BindNull();
		GL_SelectTexture( 0 );
	}

	if ( pStage->texture.texgen == TG_REFLECT_CUBE ) {
		// see if there is also a bump map specified
		const shaderStage_t *bumpStage = surf->material->GetBumpStage();
		if (bumpStage) {
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture(1);
			globalImages->BindNull();
			GL_SelectTexture(0);
		}
		R_GLSL_SetActiveProgram("basicSurf");
	}

	if ( pStage->texture.hasMatrix ) {
		qglMatrixMode( GL_TEXTURE );
		RB_LoadMatrixIdentity( GL_TEXTURE );
		qglMatrixMode( GL_MODELVIEW );
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
void RB_T_FillDepthBuffer( const drawSurf_t *surf ) {
	int			stage;
	const idMaterial	*shader;
	const shaderStage_t *pStage;
	const float	*regs;
	idVec4		color;
	const srfTriangles_t	*tri;

	tri = surf->geo;
	shader = surf->material;

	// update the clip plane if needed
	if ( backEnd.viewDef->numClipPlanes && surf->space != backEnd.currentSpace ) {
		GL_SelectTexture( 1 );

		idPlane	plane;

		R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.viewDef->clipPlanes[0], plane );
		plane[3] += 0.5;	// the notch is in the middle
		qglTexGenfv( GL_S, GL_OBJECT_PLANE, plane.ToFloatPtr() );
		GL_SelectTexture( 0 );
	}

	if ( !shader->IsDrawn() ) {
		return;
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if ( !tri->numIndexes ) {
		return;
	}

	// translucent surfaces don't put anything in the depth buffer and don't
	// test against it, which makes them fail the mirror clip plane operation
	// SM: We DO want to proceed with translucent materials that need to write to the color pass
	if (shader->HasCoverage(MC_TRANSLUCENT) && (!shader->TestMaterialFlag(MF_NODESATURATE) || backEnd.viewDef != tr.primaryView)) {
		return;
	}

	// blendo eric: don't depth fill on decal blends
	if (shader->TestMaterialFlag(MF_BLENDLIT)) {
		return;
	}

	if ( !tri->ambientCache ) {
		common->Printf( "RB_T_FillDepthBuffer: !tri->ambientCache\n" );
		return;
	}

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// if all stages of a material have been conditioned off, don't do anything
	for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
		pStage = shader->GetStage(stage);
		// check the stage enable condition
		if ( regs[ pStage->conditionRegister ] != 0 ) {
			break;
		}
	}
	if ( stage == shader->GetNumStages() ) {
		return;
	}

	// set polygon offset if necessary
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET) ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
	}

	// subviews will just down-modulate the color buffer by overbright
	if ( shader->GetSort() == SS_SUBVIEW ) {
		GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS );
		color[0] =
		color[1] =
		color[2] = ( 1.0 / backEnd.overBright );
		color[3] = 1;
	} else {
		// others just draw black
		color[0] = 0;
		color[1] = 0;
		color[2] = 0;
		color[3] = 1;
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position( tri->ambientCache );
	R_GLSL_EnableVertexAttribs(2);
	R_GLSL_SetVertexAttribs(ac);

	bool drawSolid = false;

	if ( shader->HasCoverage(MC_OPAQUE) && shader->GetSort() != SS_PORTAL_SKY) {
		drawSolid = true;
	}

	static idVec4 NoAlphaTest(-1.0f, 0.0f, 0.0f, 0.0f);

	// we may have multiple alpha tested stages
	if ( shader->HasCoverage(MC_PERFORATED) ) {
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool	didDraw = false;

		qglEnable( GL_ALPHA_TEST );
		// perforated surfaces may have multiple alpha tested stages
		for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
			pStage = shader->GetStage(stage);

			if ( !pStage->hasAlphaTest ) {
				continue;
			}

			// check the stage enable condition
			if ( regs[ pStage->conditionRegister ] == 0 ) {
				continue;
			}

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// set the alpha modulate
			color[3] = regs[ pStage->color.registers[3] ];

			// skip the entire stage if alpha would be black
			if ( color[3] <= 0 ) {
				continue;
			}
			qglColor4fv( color.ToFloatPtr() );

			R_GLSL_SetProgramEnv(0, color);
			R_GLSL_SetProgramEnv(1, idVec4(regs[pStage->alphaTestRegister], 0.0f, 0.0f, 0.0f));

			qglAlphaFunc( GL_GREATER, regs[ pStage->alphaTestRegister ] );

			// bind the texture
			pStage->texture.image->Bind();

			// set texture matrix and texGens
			RB_PrepareStageTexturing( pStage, surf, ac );

			// draw it
			RB_DrawElementsWithCounters( tri );

			RB_FinishStageTexturing( pStage, surf, ac );
		}
		qglDisable( GL_ALPHA_TEST );
		if ( !didDraw ) {
			drawSolid = true;
		}
	} else if ( shader->Coverage() == MC_TRANSLUCENT ) {
		// SM: We only will get here if this needs to write to the color buffer for post process info,
		// as other translucent coverages get skipped
		qglDepthMask( GL_FALSE );
		for ( stage = 0; stage < shader->GetNumStages(); stage++ ) {
			pStage = shader->GetStage( stage );

			// set the alpha modulate
			color[3] = regs[pStage->color.registers[3]];

			qglColor4fv( color.ToFloatPtr() );

			R_GLSL_SetProgramEnv(0, color);
			R_GLSL_SetProgramEnv(1, NoAlphaTest);

			GL_State( pStage->drawStateBits );

			// bind the texture
			pStage->texture.image->Bind();

			// set texture matrix and texGens
			RB_PrepareStageTexturing( pStage, surf, ac );

			// draw it
			RB_DrawElementsWithCounters( tri );

			RB_FinishStageTexturing( pStage, surf, ac );
		}
		qglDepthMask( GL_TRUE );
		GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS );
	}

	// draw the entire surface solid
	if ( drawSolid ) {
		qglColor4fv( color.ToFloatPtr() );

		R_GLSL_SetProgramEnv(0, color);
		R_GLSL_SetProgramEnv(1, NoAlphaTest);

		globalImages->whiteImage->Bind();

		// draw it
		RB_DrawElementsWithCounters( tri );
	}


	// reset polygon offset
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET) ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	// reset blending
	if ( shader->GetSort() == SS_SUBVIEW ) {
		GL_State( GLS_DEPTHFUNC_LESS );
	}

	R_GLSL_DisableVertexAttribs();
}

void RB_T_FillCustomMask(const drawSurf_t* surf) {
	if (surf->space->entityDef && surf->space->entityDef->parms.customPostValue != 0)
	{
		R_GLSL_SetCustomPostValue(surf->space->entityDef->parms.customPostValue);
		RB_T_FillDepthBuffer(surf);
	}
}

void RB_STD_FillCustomMask(drawSurf_t** drawSurfs, int numDrawSurfs) {
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if (!backEnd.viewDef->viewEntitys) {
		return;
	}

	// Don't do this in subviews
	if (backEnd.viewDef->isSubview) {
		return;
	}

	// SM: This means its a portal sky, so skip it
	if (backEnd.viewDef->renderView.shaderParms[11] == -42.0f) {
		return;
	}

	// enable the second texture for mirror plane clipping if needed
	if (backEnd.viewDef->numClipPlanes) {
		GL_SelectTexture(1);
		globalImages->alphaNotchImage->Bind();
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglEnable(GL_TEXTURE_GEN_S);
		qglTexCoord2f(1, 0.5);
	}

	// the first texture will be used for alpha tested surfaces
	GL_SelectTexture(0);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// decal surfaces may enable polygon offset
	qglPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat());

	GL_State(GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK);
	
	// We need logical OR writes for this pass
	if (!glConfig.isBrokenAMDR7200)
	{
		qglEnable(GL_COLOR_LOGIC_OP);
		qglLogicOp(GL_OR);
	}

	R_GLSL_SetActiveProgram("customMaskPass");

	RB_RenderDrawSurfListWithFunction(drawSurfs, numDrawSurfs, RB_T_FillCustomMask);

	R_GLSL_DisablePrograms();

	if (!glConfig.isBrokenAMDR7200)
	{
		qglDisable(GL_COLOR_LOGIC_OP);
	}

	if (backEnd.viewDef->numClipPlanes) {
		GL_SelectTexture(1);
		globalImages->BindNull();
		qglDisable(GL_TEXTURE_GEN_S);
		GL_SelectTexture(0);
	}

}

/*
=====================
RB_STD_FillDepthBuffer

If we are rendering a subview with a near clip plane, use a second texture
to force the alpha test to fail when behind that clip plane
=====================
*/
void RB_STD_FillDepthBuffer( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if ( !backEnd.viewDef->viewEntitys ) {
		return;
	}

	// enable the second texture for mirror plane clipping if needed
	if ( backEnd.viewDef->numClipPlanes ) {
		GL_SelectTexture( 1 );
		globalImages->alphaNotchImage->Bind();
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		qglEnable( GL_TEXTURE_GEN_S );
		qglTexCoord2f( 1, 0.5 );
	}

	// the first texture will be used for alpha tested surfaces
	GL_SelectTexture( 0 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	// decal surfaces may enable polygon offset
	qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() );

	GL_State( GLS_DEPTHFUNC_LESS );

	// Enable stencil test if we are going to be using it for shadows.
	// If we didn't do this, it would be legal behavior to get z fighting
	// from the ambient pass and the light passes.
	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	R_GLSL_SetActiveProgram("depthOnly");

	RB_RenderDrawSurfListWithFunction( drawSurfs, numDrawSurfs, RB_T_FillDepthBuffer );

	R_GLSL_DisablePrograms();

	if ( backEnd.viewDef->numClipPlanes ) {
		GL_SelectTexture( 1 );
		globalImages->BindNull();
		qglDisable( GL_TEXTURE_GEN_S );
		GL_SelectTexture( 0 );
	}

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
void RB_SetProgramEnvironment( void ) {
	idVec4	parm;
	int		pot;

	if ( !glConfig.ARBVertexProgramAvailable ) {
		return;
	}

#if 0
	// screen power of two correction factor, one pixel in so we don't get a bilerp
	// of an uncopied pixel
	int	 w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
	pot = globalImages->currentRenderImage->uploadWidth;
	if ( w == pot ) {
		parm[0] = 1.0;
	} else {
		parm[0] = (float)(w-1) / pot;
	}

	int	 h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
	pot = globalImages->currentRenderImage->uploadHeight;
	if ( h == pot ) {
		parm[1] = 1.0;
	} else {
		parm[1] = (float)(h-1) / pot;
	}

	parm[2] = 0;
	parm[3] = 1;
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm );
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
	// SM: As far as I can tell, this is not used in any vertex program so not porting it to GLSL
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, parm.ToFloatPtr() );
#endif

	// SM: Turns out we still need the correction for some FX like the heat haze
	// when rendered in a remote view (where the w/h is not equal to the render image)
	RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, 0, parm );

	// window coord to 0.0 to 1.0 conversion
	parm[0] = 1.0 / w;
	parm[1] = 1.0 / h;
	parm[2] = 0;
	parm[3] = 1;
	RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, 1, parm );

	//
	// set eye position in global space
	//
	parm[0] = backEnd.viewDef->renderView.vieworg[0];
	parm[1] = backEnd.viewDef->renderView.vieworg[1];
	parm[2] = backEnd.viewDef->renderView.vieworg[2];
	parm[3] = 1.0;
	// SM: As far as I can tell, this is not used in any vertex program so not porting it to GLSL
	qglProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 1, parm.ToFloatPtr() );


}

/*
==================
RB_SetProgramEnvironmentSpace

Sets variables related to the current space that can be used by all vertex programs
==================
*/
void RB_SetProgramEnvironmentSpace( void ) {
	if ( !glConfig.ARBVertexProgramAvailable ) {
		return;
	}

	const struct viewEntity_s *space = backEnd.currentSpace;
	idVec4	parm;

	// set eye position in local space
	R_GlobalPointToLocal( space->modelMatrix, backEnd.viewDef->renderView.vieworg, *(idVec3 *)&parm );
	parm[3] = 1.0;
	RB_SetProgramEnv( GL_VERTEX_PROGRAM_ARB, 5, parm );

	// we need the model matrix without it being combined with the view matrix
	// so we can transform local vectors to global coordinates
	parm[0] = space->modelMatrix[0];
	parm[1] = space->modelMatrix[4];
	parm[2] = space->modelMatrix[8];
	parm[3] = space->modelMatrix[12];
	RB_SetProgramEnv( GL_VERTEX_PROGRAM_ARB, 6, parm );
	parm[0] = space->modelMatrix[1];
	parm[1] = space->modelMatrix[5];
	parm[2] = space->modelMatrix[9];
	parm[3] = space->modelMatrix[13];
	RB_SetProgramEnv( GL_VERTEX_PROGRAM_ARB, 7, parm );
	parm[0] = space->modelMatrix[2];
	parm[1] = space->modelMatrix[6];
	parm[2] = space->modelMatrix[10];
	parm[3] = space->modelMatrix[14];
	RB_SetProgramEnv( GL_VERTEX_PROGRAM_ARB, 8, parm );
}

/*
==================
RB_STD_T_RenderShaderPasses

This is also called for the generated 2D rendering
==================
*/
void RB_STD_T_RenderShaderPasses( const drawSurf_t *surf ) {
	int			stage;
	const idMaterial	*shader;
	const shaderStage_t *pStage;
	const float	*regs;
	idVec4		color;
	const srfTriangles_t	*tri;

	tri = surf->geo;
	shader = surf->material;

	// blendo eric: is this necessary?
	// Blendo ambience requires us to concern ourselves with shaders with non-ambient stages.
	// If it's disabled, we can skip them.
	if (!tr.UseBlendoAmbience() && !shader->HasAmbient()) {
		return;
	}

	// SM: Not sure why this exists, because as far as we can tell this was never used in BFG
	// we want to use this flag so that we aren't stuck with a hardcoded portalsky material name
	//if ( shader->IsPortalSky() ) {
	//	return;
	//}

	// change the matrix if needed
	if ( surf->space != backEnd.currentSpace ) {
		RB_LoadMatrix( surf->space->modelViewMatrix, GL_MODELVIEW );
		backEnd.currentSpace = surf->space;
		RB_SetProgramEnvironmentSpace();
	}

	// change the scissor if needed
	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( surf->scissorRect ) ) {
		backEnd.currentScissor = surf->scissorRect;
		RB_SetScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	}

	// some deforms may disable themselves by setting numIndexes = 0
	if ( !tri->numIndexes ) {
		return;
	}

	if ( !tri->ambientCache ) {
		common->Printf( "RB_T_RenderShaderPasses: !tri->ambientCache\n" );
		return;
	}

	// get the expressions for conditionals / color / texcoords
	regs = surf->shaderRegisters;

	// set face culling appropriately
	GL_Cull( shader->GetCullType() );

	// set polygon offset if necessary
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET) ) {
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
	}

	if ( surf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
	}

	if ( surf->space->modelDepthHack != 0.0f ) {
		RB_EnterModelDepthHack( surf->space->modelDepthHack );
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position( tri->ambientCache );
 	R_GLSL_EnableVertexAttribs(6);
 	R_GLSL_SetVertexAttribs(ac);
	
	// SM: HACK!! For some reason, we still need to call these GPA crashes when doing a capture
	// Really shouldn't need this as they aren't used at all when running shaders but /shrug
	qglVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	qglTexCoordPointer(2, GL_FLOAT, sizeof(idDrawVert), reinterpret_cast<void *>(&ac->st));

	for ( stage = 0; stage < shader->GetNumStages() ; stage++ ) {
		pStage = shader->GetStage(stage);

		// check the enable condition
		if ( regs[ pStage->conditionRegister ] == 0 ) {
			continue;
		}

		// skip the stages involved in lighting
		bool bBlendoAmbienceStage = tr.UseBlendoAmbience() && pStage->lighting == SL_DIFFUSE;

		if ( pStage->lighting != SL_AMBIENT && !bBlendoAmbienceStage) {
			continue;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if ( ( pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS) ) == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) ) {
			continue;
		}

		// see if we are a new-style stage
		newShaderStage_t *newStage = pStage->newStage;
		if ( newStage ) {
			//--------------------------
			//
			// new style stages
			//
			//--------------------------

			// completely skip the stage if we don't have the capability
			if ( r_skipNewAmbient.GetBool() ) {
				continue;
			}

			GL_State( pStage->drawStateBits );

			R_GLSL_SetActiveProgram(*(newStage->glslName));

			// megaTextures bind a lot of images and set a lot of parameters
			if ( newStage->megaTexture ) {
				newStage->megaTexture->SetMappingForSurface( tri );
				idVec3	localViewer;
				R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewer );
				newStage->megaTexture->BindForViewOrigin( localViewer );
			}

			for ( int i = 0 ; i < newStage->numVertexParms ; i++ ) {
				idVec4	parm;
				parm[0] = regs[ newStage->vertexParms[i][0] ];
				parm[1] = regs[ newStage->vertexParms[i][1] ];
				parm[2] = regs[ newStage->vertexParms[i][2] ];
				parm[3] = regs[ newStage->vertexParms[i][3] ];
				RB_SetProgramLocal( GL_VERTEX_PROGRAM_ARB, i, parm );
			}

			// SW: If we've been supplied a fragment map override, we have to swap it in before binding.
			if (surf->fragmentMapOverride)
			{
				if (newStage->numFragmentProgramImages < 1)
					newStage->numFragmentProgramImages = 1;

				newStage->fragmentProgramImages[0] = surf->fragmentMapOverride;
			}

			for ( int i = 0 ; i < newStage->numFragmentProgramImages ; i++ ) {
				if ( newStage->fragmentProgramImages[i] ) {
					GL_SelectTexture( i );
					newStage->fragmentProgramImages[i]->Bind();
				}
			}

			// draw it
			RB_DrawElementsWithCounters( tri );

			for ( int i = 1 ; i < newStage->numFragmentProgramImages ; i++ ) {
				if ( newStage->fragmentProgramImages[i] ) {
					GL_SelectTexture( i );
					globalImages->BindNull();
				}
			}
			if ( newStage->megaTexture ) {
				newStage->megaTexture->Unbind();
			}

			GL_SelectTexture( 0 );

			R_GLSL_SetActiveProgram("basicSurf");
			continue;
		}

		//--------------------------
		//
		// old style stages
		//
		//--------------------------

		int drawStateBits = pStage->drawStateBits;

		// set the color
		color[0] = regs[pStage->color.registers[0]];
		color[1] = regs[pStage->color.registers[1]];
		color[2] = regs[pStage->color.registers[2]];
		color[3] = regs[pStage->color.registers[3]];

		// Blendo ambience:
		// If this is a diffuse surface, we want a very particular kind of ambient draw call that just slightly increases the visibility of the surface
		// We do this by forcing an additive blend mode and changing colour values to be exceedingly dim.
		if (bBlendoAmbienceStage)
		{
			float scale = tr.GetBlendoAmbienceScale();
			color[0] *= scale;
			color[1] *= scale;
			color[2] *= scale;

			//blendo eric: we alpha blend here to darken previous lighting passes behind decals
			if (shader->TestMaterialFlag(MF_BLENDLIT)) {
				drawStateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS;
			}
			else {
				color[3] *= scale; // is this required here?
				drawStateBits |= (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
			}
		}

		// skip the entire stage if an add would be black
		if ( ( pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )
			&& color[0] <= 0 && color[1] <= 0 && color[2] <= 0 ) {
			continue;
		}

		// skip the entire stage if a blend would be completely transparent
		if ( ( pStage->drawStateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
			&& color[3] <= 0 ) {
			continue;
		}

		R_GLSL_SetVertexColorEnv(pStage->vertexColor, color);

		// bind the texture
		RB_BindVariableStageImage( &pStage->texture, regs );

		// set the state
		GL_State( drawStateBits );

		RB_PrepareStageTexturing( pStage, surf, ac );

		// draw it
		RB_DrawElementsWithCounters( tri );

		RB_FinishStageTexturing( pStage, surf, ac );
	}

	// reset polygon offset
	if ( shader->TestMaterialFlag(MF_POLYGONOFFSET) ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
	if ( surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
	}

	R_GLSL_DisableVertexAttribs();
}

/*
=====================
RB_STD_DrawShaderPasses

Draw non-light dependent passes
=====================
*/
int RB_STD_DrawShaderPasses( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int				i;

	// only obey skipAmbient if we are rendering a view
	if ( backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool() ) {
		return numDrawSurfs;
	}

	// if we are about to draw the first surface that needs
	// the rendering in a texture, copy it over
	if ( drawSurfs[0]->material->GetSort() >= SS_POST_PROCESS ) {
		if ( r_skipPostProcess.GetBool() ) {
			return 0;
		}

		// only dump if in a 3d view
		if ( backEnd.viewDef->viewEntitys ) {
			globalImages->currentRenderImage->CopyFramebuffer( backEnd.viewDef->viewport.x1,
				backEnd.viewDef->viewport.y1,  backEnd.viewDef->viewport.x2 -  backEnd.viewDef->viewport.x1 + 1,
				backEnd.viewDef->viewport.y2 -  backEnd.viewDef->viewport.y1 + 1, true );
		}
		backEnd.currentRenderCopied = true;
	}

	GL_SelectTexture( 1 );
	globalImages->BindNull();

	GL_SelectTexture( 0 );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_GLSL_SetActiveProgram("basicSurf");

	RB_SetProgramEnvironment();

	// we don't use RB_RenderDrawSurfListWithFunction()
	// because we want to defer the matrix load because many
	// surfaces won't draw any ambient passes
	backEnd.currentSpace = NULL;
	for (i = 0  ; i < numDrawSurfs ; i++ ) {
		if ( drawSurfs[i]->material->SuppressInSubview() ) {
			continue;
		}

		if ( backEnd.viewDef->isXraySubview && drawSurfs[i]->space->entityDef ) {
			if ( drawSurfs[i]->space->entityDef->parms.xrayIndex != 2 ) {
				continue;
			}
		}

		// blendo eric: cutoff for current pass
		if (drawSurfs[i]->material->GetSort() > backEnd.materialStageCutoffInclusive ) {
			break;
		}

		// we need to draw the post process shaders after we have drawn the fog lights
		if ( drawSurfs[i]->material->GetSort() >= SS_POST_PROCESS
			&& !backEnd.currentRenderCopied ) {
			break;
		}

		RB_STD_T_RenderShaderPasses( drawSurfs[i] );
	}

	GL_Cull( CT_FRONT_SIDED );
	qglColor3f( 1, 1, 1 );

	R_GLSL_DisablePrograms();

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
static void RB_T_Shadow( const drawSurf_t *surf ) {
	const srfTriangles_t	*tri;

	// set the light position if we are using a vertex program to project the rear surfaces
	if ( tr.backEndRendererHasVertexPrograms && r_useShadowVertexProgram.GetBool()
		&& surf->space != backEnd.currentSpace ) {
		idVec4 localLight;

		R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.vLight->globalLightOrigin, localLight.ToVec3() );
		localLight.w = 0.0f;
		RB_SetProgramEnv( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_ORIGIN, localLight );
	}

	tri = surf->geo;

	if ( !tri->shadowCache ) {
		return;
	}

	qglVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(shadowCache_t), vertexCache.Position(tri->shadowCache));

	// we always draw the sil planes, but we may not need to draw the front or rear caps
	int	numIndexes;
	bool external = false;

	if ( !r_useExternalShadows.GetInteger() ) {
		numIndexes = tri->numIndexes;
	} else if ( r_useExternalShadows.GetInteger() == 2 ) { // force to no caps for testing
		numIndexes = tri->numShadowIndexesNoCaps;
	} else if ( !(surf->dsFlags & DSF_VIEW_INSIDE_SHADOW) ) {
		// if we aren't inside the shadow projection, no caps are ever needed needed
		numIndexes = tri->numShadowIndexesNoCaps;
		external = true;
	} else if ( !backEnd.vLight->viewInsideLight && !(surf->geo->shadowCapPlaneBits & SHADOW_CAP_INFINITE) ) {
		// if we are inside the shadow projection, but outside the light, and drawing
		// a non-infinite shadow, we can skip some caps
		if ( backEnd.vLight->viewSeesShadowPlaneBits & surf->geo->shadowCapPlaneBits ) {
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

	// set depth bounds
	if( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglDepthBoundsEXT( surf->scissorRect.zmin, surf->scissorRect.zmax );
	}

	// debug visualization
	if ( r_showShadows.GetInteger() ) {
		if ( r_showShadows.GetInteger() == 3 ) {
			if ( external ) {
				qglColor3f( 0.1/backEnd.overBright, 1/backEnd.overBright, 0.1/backEnd.overBright );
			} else {
				// these are the surfaces that require the reverse
				qglColor3f( 1/backEnd.overBright, 0.1/backEnd.overBright, 0.1/backEnd.overBright );
			}
		} else {
			// draw different color for turboshadows
			if ( surf->geo->shadowCapPlaneBits & SHADOW_CAP_INFINITE ) {
				if ( numIndexes == tri->numIndexes ) {
					qglColor3f( 1/backEnd.overBright, 0.1/backEnd.overBright, 0.1/backEnd.overBright );
				} else {
					qglColor3f( 1/backEnd.overBright, 0.4/backEnd.overBright, 0.1/backEnd.overBright );
				}
			} else {
				if ( numIndexes == tri->numIndexes ) {
					qglColor3f( 0.1/backEnd.overBright, 1/backEnd.overBright, 0.1/backEnd.overBright );
				} else if ( numIndexes == tri->numShadowIndexesNoFrontCaps ) {
					qglColor3f( 0.1/backEnd.overBright, 1/backEnd.overBright, 0.6/backEnd.overBright );
				} else {
					qglColor3f( 0.6/backEnd.overBright, 1/backEnd.overBright, 0.1/backEnd.overBright );
				}
			}
		}

		qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		qglDisable( GL_STENCIL_TEST );
		GL_Cull( CT_TWO_SIDED );
		RB_DrawShadowElementsWithCounters( tri, numIndexes );
		GL_Cull( CT_FRONT_SIDED );
		qglEnable( GL_STENCIL_TEST );

		return;
	}

	bool useStencilOpSeperate = r_useStencilOpSeparate.GetBool() && qglStencilOpSeparate != NULL;
	if( !r_stencilReverse.GetBool() ) {
		if( useStencilOpSeperate ) {
			// not using z-fail, but using qglStencilOpSeparate()
			GLenum firstFace = backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK;
			GLenum secondFace = backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT;
			GL_Cull( CT_TWO_SIDED );
			if ( !external ) {
				qglStencilOpSeparate( firstFace, GL_KEEP, tr.stencilDecr, tr.stencilDecr );
				qglStencilOpSeparate( secondFace, GL_KEEP, tr.stencilIncr, tr.stencilIncr );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );
			}

			qglStencilOpSeparate( firstFace, GL_KEEP, GL_KEEP, tr.stencilIncr );
			qglStencilOpSeparate( secondFace, GL_KEEP, GL_KEEP, tr.stencilDecr );

			RB_DrawShadowElementsWithCounters( tri, numIndexes );

		} else { // DG: this is the original code:
			// patent-free work around
			if ( !external ) {
				// "preload" the stencil buffer with the number of volumes
				// that get clipped by the near or far clip plane
				qglStencilOp( GL_KEEP, tr.stencilDecr, tr.stencilDecr );
				GL_Cull( CT_FRONT_SIDED );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );
				qglStencilOp( GL_KEEP, tr.stencilIncr, tr.stencilIncr );
				GL_Cull( CT_BACK_SIDED );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );
			}

			// traditional depth-pass stencil shadows
			qglStencilOp( GL_KEEP, GL_KEEP, tr.stencilIncr );
			GL_Cull( CT_FRONT_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );

			qglStencilOp( GL_KEEP, GL_KEEP, tr.stencilDecr );
			GL_Cull( CT_BACK_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );
		}
	} else { // use the formerly patented "Carmack's Reverse" Z-Fail code
		if( useStencilOpSeperate ) {
			// Z-Fail with glStencilOpSeparate() which will reduce draw calls
			GLenum firstFace = backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK;
			GLenum secondFace = backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT;
			if ( !external ) { // z-fail
				qglStencilOpSeparate( firstFace, GL_KEEP, tr.stencilDecr, GL_KEEP );
				qglStencilOpSeparate( secondFace, GL_KEEP, tr.stencilIncr, GL_KEEP );
			} else { // depth-pass
				qglStencilOpSeparate( firstFace, GL_KEEP, GL_KEEP, tr.stencilIncr );
				qglStencilOpSeparate( secondFace, GL_KEEP, GL_KEEP, tr.stencilDecr );
			}
			GL_Cull( CT_TWO_SIDED );
			RB_DrawShadowElementsWithCounters( tri, numIndexes );

		} else { // Z-Fail without glStencilOpSeparate()

			// LEITH: the (formerly patented) "Carmack's Reverse" code

			// depth-fail/Z-Fail stencil shadows
			if ( !external ) {
				qglStencilOp( GL_KEEP, tr.stencilDecr, GL_KEEP );
				GL_Cull( CT_FRONT_SIDED );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );
				qglStencilOp( GL_KEEP, tr.stencilIncr, GL_KEEP );
				GL_Cull( CT_BACK_SIDED );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );
			}
			// traditional depth-pass stencil shadows
			else {
				qglStencilOp( GL_KEEP, GL_KEEP, tr.stencilIncr );
				GL_Cull( CT_FRONT_SIDED );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );

				qglStencilOp( GL_KEEP, GL_KEEP, tr.stencilDecr );
				GL_Cull( CT_BACK_SIDED );
				RB_DrawShadowElementsWithCounters( tri, numIndexes );
			}
		}
	}

}

/*
=====================
RB_StencilShadowPass

Stencil test should already be enabled, and the stencil buffer should have
been set to 128 on any surfaces that might receive shadows
=====================
*/
void RB_StencilShadowPass( const drawSurf_t *drawSurfs ) {
	if ( !r_shadows.GetBool() ) {
		return;
	}

	if ( !drawSurfs ) {
		return;
	}

	globalImages->BindNull();
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// for visualizing the shadows
	if ( r_showShadows.GetInteger() ) {
		if ( r_showShadows.GetInteger() == 2 ) {
			// draw filled in
			GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS  );
		} else {
			// draw as lines, filling the depth buffer
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS  );
		}
	} else {
		// don't write to the color buffer, just the stencil buffer
		GL_State( GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
	}

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglPolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
		qglEnable( GL_POLYGON_OFFSET_FILL );
	}

	qglStencilFunc( GL_ALWAYS, 1, 255 );

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
	}

	R_GLSL_EnableVertexAttribs(1);
	RB_RenderDrawSurfChainWithFunction(drawSurfs, RB_T_Shadow);
	R_GLSL_DisableVertexAttribs();

	GL_Cull( CT_FRONT_SIDED );

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}

	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	}

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	qglStencilFunc( GL_GEQUAL, 128, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
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
static void RB_T_BlendLight( const drawSurf_t *surf ) {
	const srfTriangles_t *tri;

	tri = surf->geo;

	if ( backEnd.currentSpace != surf->space ) {
		idPlane	lightProject[4];
		int		i;

		for ( i = 0 ; i < 4 ; i++ ) {
			R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i] );
		}

		GL_SelectTexture( 0 );
		qglTexGenfv( GL_S, GL_OBJECT_PLANE, lightProject[0].ToFloatPtr() );
		qglTexGenfv( GL_T, GL_OBJECT_PLANE, lightProject[1].ToFloatPtr() );
		qglTexGenfv( GL_Q, GL_OBJECT_PLANE, lightProject[2].ToFloatPtr() );

		GL_SelectTexture( 1 );
		qglTexGenfv( GL_S, GL_OBJECT_PLANE, lightProject[3].ToFloatPtr() );
	}

	// this gets used for both blend lights and shadow draws
	if ( tri->ambientCache ) {
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( tri->ambientCache );
		qglVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
	} else if ( tri->shadowCache ) {
		shadowCache_t	*sc = (shadowCache_t *)vertexCache.Position( tri->shadowCache );
		qglVertexPointer( 3, GL_FLOAT, sizeof( shadowCache_t ), sc->xyz.ToFloatPtr() );
	}

	RB_DrawElementsWithCounters( tri );
}


/*
=====================
RB_BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
static void RB_BlendLight( const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2 ) {
	const idMaterial	*lightShader;
	const shaderStage_t	*stage;
	int					i;
	const float	*regs;

	if ( !drawSurfs ) {
		return;
	}
	if ( r_skipBlendLights.GetBool() ) {
		return;
	}

	lightShader = backEnd.vLight->lightShader;
	regs = backEnd.vLight->shaderRegisters;

	// texture 1 will get the falloff texture
	GL_SelectTexture( 1 );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglEnable( GL_TEXTURE_GEN_S );
	qglTexCoord2f( 0, 0.5 );
	backEnd.vLight->falloffImage->Bind();

	// texture 0 will get the projected texture
	GL_SelectTexture( 0 );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglEnable( GL_TEXTURE_GEN_S );
	qglEnable( GL_TEXTURE_GEN_T );
	qglEnable( GL_TEXTURE_GEN_Q );

	for ( i = 0 ; i < lightShader->GetNumStages() ; i++ ) {
		stage = lightShader->GetStage(i);

		if ( !regs[ stage->conditionRegister ] ) {
			continue;
		}

		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );

		GL_SelectTexture( 0 );
		stage->texture.image->Bind();

		if ( stage->texture.hasMatrix ) {
			RB_LoadShaderTextureMatrix( regs, &stage->texture );
		}

		// get the modulate values from the light, including alpha, unlike normal lights
		backEnd.lightColor[0] = regs[ stage->color.registers[0] ];
		backEnd.lightColor[1] = regs[ stage->color.registers[1] ];
		backEnd.lightColor[2] = regs[ stage->color.registers[2] ];
		backEnd.lightColor[3] = regs[ stage->color.registers[3] ];
		qglColor4fv( backEnd.lightColor );

		RB_RenderDrawSurfChainWithFunction( drawSurfs, RB_T_BlendLight );
		RB_RenderDrawSurfChainWithFunction( drawSurfs2, RB_T_BlendLight );

		if ( stage->texture.hasMatrix ) {
			GL_SelectTexture( 0 );
			qglMatrixMode( GL_TEXTURE );
			qglLoadIdentity();
			qglMatrixMode( GL_MODELVIEW );
		}
	}

	GL_SelectTexture( 1 );
	qglDisable( GL_TEXTURE_GEN_S );
	globalImages->BindNull();

	GL_SelectTexture( 0 );
	qglDisable( GL_TEXTURE_GEN_S );
	qglDisable( GL_TEXTURE_GEN_T );
	qglDisable( GL_TEXTURE_GEN_Q );
}


//========================================================================

static idPlane	fogPlanes[4];

/*
=====================
RB_T_BasicFog

=====================
*/
static void RB_T_BasicFog( const drawSurf_t *surf ) {
	if ( backEnd.currentSpace != surf->space ) {
		idPlane	local;

		GL_SelectTexture( 0 );

		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[0], local );
		local[3] += 0.5;
		R_GLSL_SetProgramEnv( 10, local.ToVec4() );

//		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[1], local );
//		local[3] += 0.5;
		local[0] = local[1] = local[2] = 0; local[3] = 0.5;
		R_GLSL_SetProgramEnv( 11, local.ToVec4() );

		GL_SelectTexture( 1 );

		// GL_S is constant per viewer
		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[2], local );
		local[3] += FOG_ENTER;
		R_GLSL_SetProgramEnv( 13, local.ToVec4() );

		R_GlobalPlaneToLocal( surf->space->modelMatrix, fogPlanes[3], local );
		R_GLSL_SetProgramEnv( 12, local.ToVec4() );
	}

	RB_T_RenderTriangleSurface( surf );
}



/*
==================
RB_FogPass
==================
*/
static void RB_FogPass( const drawSurf_t *drawSurfs,  const drawSurf_t *drawSurfs2 ) {
	const srfTriangles_t*frustumTris;
	drawSurf_t			ds;
	const idMaterial	*lightShader;
	const shaderStage_t	*stage;
	const float			*regs;

	// create a surface for the light frustom triangles, which are oriented drawn side out
	frustumTris = backEnd.vLight->frustumTris;

	// if we ran out of vertex cache memory, skip it
	if ( !frustumTris->ambientCache ) {
		return;
	}
	memset( &ds, 0, sizeof( ds ) );
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

	R_GLSL_EnableVertexAttribs( 2 );
	R_GLSL_SetActiveProgram( "fogLight" );
	idVec4 lightColor;
	memcpy( &lightColor, backEnd.lightColor, sizeof( float ) * 4 );
	R_GLSL_SetProgramEnv( PP_COLOR_GLSL, lightColor );

	// calculate the falloff planes
	float	a;

	// if they left the default value on, set a fog distance of 500
	if ( backEnd.lightColor[3] <= 1.0 ) {
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	} else {
		// otherwise, distance = alpha color
		a = -0.5f / backEnd.lightColor[3];
	}

	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

	// texture 0 is the falloff image
	GL_SelectTexture( 0 );
	globalImages->fogImage->Bind();
	//GL_Bind( tr.whiteImage );

	fogPlanes[0][0] = a * backEnd.viewDef->worldSpace.modelViewMatrix[2];
	fogPlanes[0][1] = a * backEnd.viewDef->worldSpace.modelViewMatrix[6];
	fogPlanes[0][2] = a * backEnd.viewDef->worldSpace.modelViewMatrix[10];
	fogPlanes[0][3] = a * backEnd.viewDef->worldSpace.modelViewMatrix[14];

	fogPlanes[1][0] = a * backEnd.viewDef->worldSpace.modelViewMatrix[0];
	fogPlanes[1][1] = a * backEnd.viewDef->worldSpace.modelViewMatrix[4];
	fogPlanes[1][2] = a * backEnd.viewDef->worldSpace.modelViewMatrix[8];
	fogPlanes[1][3] = a * backEnd.viewDef->worldSpace.modelViewMatrix[12];


	// texture 1 is the entering plane fade correction
	GL_SelectTexture( 1 );
	globalImages->fogEnterImage->Bind();

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

	// draw it
	RB_RenderDrawSurfChainWithFunction( drawSurfs, RB_T_BasicFog );
	RB_RenderDrawSurfChainWithFunction( drawSurfs2, RB_T_BasicFog );

	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
	GL_Cull( CT_BACK_SIDED );
	RB_RenderDrawSurfChainWithFunction( &ds, RB_T_BasicFog );
	GL_Cull( CT_FRONT_SIDED );

	GL_SelectTexture( 1 );
	globalImages->BindNull();

	GL_SelectTexture( 0 );

	R_GLSL_DisableVertexAttribs();
	R_GLSL_DisablePrograms();
}


/*
==================
RB_STD_FogAllLights
==================
*/
void RB_STD_FogAllLights( void ) {
	viewLight_t	*vLight;

	if ( r_skipFogLights.GetBool() || r_showOverDraw.GetInteger() != 0
		 || backEnd.viewDef->isXraySubview /* dont fog in xray mode*/
		 ) {
		return;
	}

	qglDisable( GL_STENCIL_TEST );

	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;

		if ( !vLight->lightShader->IsFogLight() && !vLight->lightShader->IsBlendLight() ) {
			continue;
		}

#if 0 // _D3XP disabled that
		if ( r_ignore.GetInteger() ) {
			// we use the stencil buffer to guarantee that no pixels will be
			// double fogged, which happens in some areas that are thousands of
			// units from the origin
			backEnd.currentScissor = vLight->scissorRect;
			if ( r_useScissor.GetBool() ) {
				RB_SetScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			qglClear( GL_STENCIL_BUFFER_BIT );

			qglEnable( GL_STENCIL_TEST );

			// only pass on the cleared stencil values
			qglStencilFunc( GL_EQUAL, 128, 255 );

			// when we pass the stencil test and depth test and are going to draw,
			// increment the stencil buffer so we don't ever draw on that pixel again
			qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
		}
#endif

		if ( vLight->lightShader->IsFogLight() ) {
			RB_FogPass( vLight->globalInteractions, vLight->localInteractions );
		} else if ( vLight->lightShader->IsBlendLight() ) {
			RB_BlendLight( vLight->globalInteractions, vLight->localInteractions );
		}
		qglDisable( GL_STENCIL_TEST );
	}

	qglEnable( GL_STENCIL_TEST );
}

//=========================================================================================

/*
==================
RB_STD_LightScale

Perform extra blending passes to multiply the entire buffer by
a floating point value
==================
*/
void RB_STD_LightScale( void ) {
#if !defined(_GLES) //karin: always skip light scale
	float	v, f;

	if ( backEnd.overBright == 1.0f ) {
		return;
	}

	if ( r_skipLightScale.GetBool() ) {
		return;
	}

	// the scissor may be smaller than the viewport for subviews
	if ( r_useScissor.GetBool() ) {
		RB_SetScissor( backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
			backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
			backEnd.viewDef->scissor.x2 - backEnd.viewDef->scissor.x1 + 1,
			backEnd.viewDef->scissor.y2 - backEnd.viewDef->scissor.y1 + 1 );
		backEnd.currentScissor = backEnd.viewDef->scissor;
	}

	// SM: For now don't glsl-ify this because it's some full on fixed function stuff

	// full screen blends
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, 1, 0, 1, -1, 1 );

	GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR );
	GL_Cull( CT_TWO_SIDED );	// so mirror views also get it
	globalImages->BindNull();
	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_STENCIL_TEST );

	v = 1;
	while ( idMath::Fabs( v - backEnd.overBright ) > 0.01 ) {	// a little extra slop
		f = backEnd.overBright / v;
		f /= 2;
		if ( f > 1 ) {
			f = 1;
		}
		qglColor3f( f, f, f );
		v = v * f * 2;

		qglBegin( GL_QUADS );
		qglVertex2f( 0,0 );
		qglVertex2f( 0,1 );
		qglVertex2f( 1,1 );
		qglVertex2f( 1,0 );
		qglEnd();
	}


	qglPopMatrix();
	qglEnable( GL_DEPTH_TEST );
	qglMatrixMode( GL_MODELVIEW );
	GL_Cull( CT_FRONT_SIDED );
#endif
}

//=========================================================================================

/*
=============
RB_STD_DrawView

=============
*/
void	RB_STD_DrawView( void ) {
	// SM: We get to RB_STD_DrawView on level load when it's the default buffer,
	// but that's an error so don't render then
	if (backEnd.frameBufferId == FRAME_DEFAULT_BACKBUFFER)
		return;

	drawSurf_t	 **drawSurfs;
	int			numDrawSurfs;

	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

	drawSurfs = (drawSurf_t **)&backEnd.viewDef->drawSurfs[0];
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	// clear the z buffer, set the projection matrix, etc
	RB_BeginDrawingView();

	// decide how much overbrighting we are going to do
	RB_DetermineLightScale();

	// Do the custom mask pass first
	// Only draw to color attachment 2 (mask texture)
#ifdef _GLES //karin: must fill GL_NONE if discard channels on GLES
	GLenum customBuffers[] = { GL_NONE, GL_NONE, GL_COLOR_ATTACHMENT2 };
	qglDrawBuffers(3, customBuffers);
#else
	GLenum customBuffers[] = { GL_COLOR_ATTACHMENT2 };
	qglDrawBuffers(1, customBuffers);
#endif
	RB_STD_FillCustomMask( drawSurfs, numDrawSurfs );

	// fill the depth buffer and clear color buffer to black except on
	// subviews
	// Only draw to color attachmenmt 0 (main color)
	GLenum mainBuffers[] = { GL_COLOR_ATTACHMENT0 };
	qglDrawBuffers(1, mainBuffers);
	RB_STD_FillDepthBuffer( drawSurfs, numDrawSurfs );

	// main light renderer
	// For interactions, we want color attachment 0 (main color) and 1 (light)
	GLenum interactionBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	qglDrawBuffers(2, interactionBuffers);
	switch( tr.backEndRenderer ) {
	case BE_GLSL:
		RB_GLSL_DrawInteractions();
		break;
	}

	// blendo eric: probably don't need stencilfunc or lightscale here
	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );

	// uplight the entire screen to crutch up not having better blending range
	qglDrawBuffers(1, mainBuffers);
	RB_STD_LightScale();

	// blendo eric: blend lit
	// alpha blended with lighting is done by darkening previous light passes using alpha blended geo on the blendo ambient pass,
	// and then running an exclusively interaction lighting pass on blend lit geo
	//
	// first draw non-interaction ambient and blend lit ambient (w alpha that darkens behind it) until SS_BLENDLIT,
    // do a second interaction light pass for blend lit decals (RB_ARB2_DrawInteractionsBlend)
	// which were excluded from the original RB_ARB2_DrawInteractions,
	// then finally draw the rest of the effect passes
	backEnd.materialStageCutoffInclusive = SS_BLENDLIT;
	int	processed = RB_STD_DrawShaderPasses(drawSurfs, numDrawSurfs); // alpha darken
	backEnd.materialStageCutoffInclusive = SS_MAX;
	
	qglDrawBuffers(2, interactionBuffers);
	RB_GLSL_DrawInteractionsBlend();

	// Back to main buffer only
	qglDrawBuffers(1, mainBuffers);
	// now draw any non-light dependent shading passes
	if (processed < numDrawSurfs) {
		processed += RB_STD_DrawShaderPasses(drawSurfs + processed, numDrawSurfs - processed);
	}

	// fob and blend lights
    RB_STD_FogAllLights();

	// Calculate view-projection matrix
	float	mat[16];
	myGlMultMatrix( backEnd.viewDef->worldSpace.modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );
	idMat4 viewProj( mat );

	// SM: Set environment parameters for invViewProj and prevViewProj
	if ( backEnd.viewDef == tr.primaryView ) {
		// Calculate invViewProj
		// Occasionally, we may get a view-projection that's not invertible, in which case don't update
		idMat4 invViewProj = viewProj;
		bool inverted = invViewProj.InverseSelf();
		if ( inverted ) {
			backEnd.invViewProj = invViewProj;
			backEnd.invViewProj.TransposeSelf();
		}

		// Load up invViewProj and prevViewProj
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_INV_VIEWPROJ_X, backEnd.invViewProj[0] );
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_INV_VIEWPROJ_Y, backEnd.invViewProj[1] );
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_INV_VIEWPROJ_Z, backEnd.invViewProj[2] );
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_INV_VIEWPROJ_W, backEnd.invViewProj[3] );
		
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_PREV_VIEWPROJ_X, backEnd.prevViewProj[0] );
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_PREV_VIEWPROJ_Y, backEnd.prevViewProj[1] );
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_PREV_VIEWPROJ_Z, backEnd.prevViewProj[2] );
		RB_SetProgramEnv( GL_FRAGMENT_PROGRAM_ARB, PP_PREV_VIEWPROJ_W, backEnd.prevViewProj[3] );
	}

	// now draw any post-processing effects using _currentRender
	if ( processed < numDrawSurfs ) {
		RB_STD_DrawShaderPasses( drawSurfs+processed, numDrawSurfs-processed );
	}

	RB_RenderDebugTools( drawSurfs, numDrawSurfs );

	// SM: Now update prev view proj for next frame
	if ( backEnd.viewDef == tr.primaryView ) {
		backEnd.prevViewProj = viewProj;
		backEnd.prevViewProj.TransposeSelf();
	}
}

void RB_LoadMatrix(const float* matrix, GLenum matrixMode)
{
	R_GLSL_LoadMatrix(matrix, matrixMode);
	// NOTE: We actually ignore the mode for this qglLoadMatrix call b/c there's all the other mode stuff
	qglLoadMatrixf(matrix);
}

void RB_LoadMatrixIdentity(GLenum matrixMode)
{
	R_GLSL_LoadMatrix(mat4_identity.ToFloatPtr(), matrixMode);
	// NOTE: We actually ignore the mode for this qglLoadMatrix call b/c there's all the other mode stuff
	qglLoadIdentity();
}

void RB_SetProgramEnv(GLenum target, GLuint index, const idVec4& params)
{
	R_GLSL_SetProgramEnv(index, params);
}

void RB_SetProgramLocal(GLenum target, GLuint index, const idVec4& params)
{
	R_GLSL_SetProgramLocal(index, params);
}

void RB_SetScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	qglScissor(x, y, Max(width, 0), Max(height, 0));
}
