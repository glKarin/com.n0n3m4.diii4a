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
#include "renderer/backend/glsl.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/stages/AmbientOcclusionStage.h"
#include "renderer/backend/FrameBufferManager.h"

//=============================================================================
// Below goes the suggested new way of handling GLSL parameters.
// TODO: move it to glsl.cpp

void Attributes::Default::Bind(GLSLProgram *program) {
	using namespace Attributes::Default;
	program->BindAttribLocation(Position, "attr_Position");
	program->BindAttribLocation(Normal, "attr_Normal");
	program->BindAttribLocation(Color, "attr_Color");
	program->BindAttribLocation(TexCoord, "attr_TexCoord");
	program->BindAttribLocation(Tangent, "attr_Tangent");
	program->BindAttribLocation(Bitangent, "attr_Bitangent");
	program->BindAttribLocation(DrawId, "attr_DrawId");
}

void Uniforms::Global::Set(const viewEntity_t *space) {
	modelMatrix.Set( space->modelMatrix );
	//projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	modelViewMatrix.Set( space->modelViewMatrix );
	if ( viewOriginLocal.IsPresent() ) {
		idVec4 vol;
		R_GlobalPointToLocal( space->modelMatrix, backEnd.viewDef->renderView.vieworg, vol.ToVec3() );
		vol[3] = 1.0;
		viewOriginLocal.Set( vol );
	}
}

void Uniforms::MaterialStage::Set(const shaderStage_t *pStage, const drawSurf_t *surf) {
	//============================================================================
	//note: copied from RB_SetProgramEnvironment and RB_SetProgramEnvironmentSpace
	//============================================================================

	idVec4 parm;
	// screen power of two correction factor, assuming the copy to _currentRender
	// also copied an extra row and column for the bilerp
	parm[0] = 1;
	parm[1] = 1;
	parm[2] = 0;
	parm[3] = 1;
 	scalePotToWindow.Set( parm );

	// window coord to 0.0 to 1.0 conversion
	parm[0] = 1.0 / frameBuffers->activeFbo->Width();
	parm[1] = 1.0 / frameBuffers->activeFbo->Height();
	parm[2] = 0;
	parm[3] = 1;
 	scaleWindowToUnit.Set( parm );

	// #3877: Allow shaders to access depth buffer.
	// Two useful ratios are packed into this parm: [0] and [1] hold the x,y multipliers you need to map a screen
	// coordinate (fragment position) to the depth image: those are simply the reciprocal of the depth
	// image size, which has been rounded up to a power of two. Slots [3] and [4] hold the ratio of the depth image
	// size to the current render image size. These sizes can differ if the game crops the render viewport temporarily
	// during post-processing effects. The depth render is smaller during the effect too, but the depth image doesn't
	// need to be downsized, whereas the current render image does get downsized when it's captured by the game after
	// the skybox render pass. The ratio is needed to map between the two render images.
	parm[0] = 1.0f / globalImages->currentDepthImage->uploadWidth;
	parm[1] = 1.0f / globalImages->currentDepthImage->uploadHeight;
	parm[2] = static_cast<float>( globalImages->currentRenderImage->uploadWidth ) / globalImages->currentDepthImage->uploadWidth;
	parm[3] = static_cast<float>( globalImages->currentRenderImage->uploadHeight ) / globalImages->currentDepthImage->uploadHeight;
	scaleDepthCoords.Set( parm );

	//
	// set eye position in global space
	//
	parm[0] = backEnd.viewDef->renderView.vieworg[0];
	parm[1] = backEnd.viewDef->renderView.vieworg[1];
	parm[2] = backEnd.viewDef->renderView.vieworg[2];
	parm[3] = 1.0;
	viewOriginGlobal.Set( parm );

	const struct viewEntity_s *space = surf->space;
	// set eye position in local space
	R_GlobalPointToLocal( space->modelMatrix, backEnd.viewDef->renderView.vieworg, parm.ToVec3() );
	parm[3] = 1.0;
	viewOriginLocal.Set( parm );

	// we need the model matrix without it being combined with the view matrix
	// so we can transform local vectors to global coordinates
	parm[0] = space->modelMatrix[0];
	parm[1] = space->modelMatrix[4];
	parm[2] = space->modelMatrix[8];
	parm[3] = space->modelMatrix[12];
 	modelMatrixRow0.Set( parm );
	parm[0] = space->modelMatrix[1];
	parm[1] = space->modelMatrix[5];
	parm[2] = space->modelMatrix[9];
	parm[3] = space->modelMatrix[13];
 	modelMatrixRow1.Set( parm );
	parm[0] = space->modelMatrix[2];
	parm[1] = space->modelMatrix[6];
	parm[2] = space->modelMatrix[10];
	parm[3] = space->modelMatrix[14];
 	modelMatrixRow2.Set( parm );

	//============================================================================

	const newShaderStage_t *newStage = pStage->newStage;
	if (newStage) {
		//setting local parameters (specified in material definition)
		const float	*regs = surf->shaderRegisters;
		for ( int i = 0; i < newStage->numVertexParms; i++ ) {
			parm[0] = regs[newStage->vertexParms[i][0]];
			parm[1] = regs[newStage->vertexParms[i][1]];
			parm[2] = regs[newStage->vertexParms[i][2]];
			parm[3] = regs[newStage->vertexParms[i][3]];
	 		localParams[ i ]->Set( parm );
		}

		//setting textures
		//note: the textures are also bound to TUs at this moment
		for ( int i = 0; i < newStage->numFragmentProgramImages; i++ ) {
			if ( newStage->fragmentProgramImages[i] ) {
				GL_SelectTexture( i );
				newStage->fragmentProgramImages[i]->Bind();
	 			textures[ i ]->Set( i );
			}
		}
	}

	GL_CheckErrors();
}
