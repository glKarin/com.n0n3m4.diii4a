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

#include "../../RenderSystem_local.h"
#include "../../../models/Model_local.h"

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
void GL_SelectTextureNoClient( int unit ) {
	backEnd.glState.currenttmu = unit;
	glActiveTextureARB( GL_TEXTURE0_ARB + unit );
	RB_LogComment( "glActiveTextureARB( %i )\n", unit );
}

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void	RB_ARB2_DrawInteraction( const drawInteraction_t *din ) {
// jmarshall
	// load all the vertex program parameters
	//tr.lightOriginParam->SetVectorValue(din->localLightOrigin);
	tr.viewOriginParam->SetVectorValue(din->localViewOrigin);
	tr.bumpmatrixSParam->SetVectorValue(din->bumpMatrix[0]);
	tr.bumpmatrixTParam->SetVectorValue(din->bumpMatrix[1]);
	tr.diffuseMatrixSParam->SetVectorValue(din->diffuseMatrix[0]);
	tr.diffuseMatrixTParam->SetVectorValue(din->diffuseMatrix[1]);
//	tr.specularMatrixSParam->SetVectorValue(din->specularMatrix[0]);
//	tr.specularMatrixTParam->SetVectorValue(din->specularMatrix[1]);

	// load all the vertex program parameters
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_ORIGIN, din->localLightOrigin.ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_VIEW_ORIGIN, din->localViewOrigin.ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_S, din->lightProjection[0].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_T, din->lightProjection[1].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_Q, din->lightProjection[2].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_FALLOFF_S, din->lightProjection[3].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_T, din->bumpMatrix[1].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_S, din->specularMatrix[0].ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_T, din->specularMatrix[1].ToFloatPtr() );
// jmarshall end


	// testing fragment based normal mapping
	//if ( r_testARBProgram.GetBool() ) {
	//	glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 2, din->localLightOrigin.ToFloatPtr() );
	//	glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 3, din->localViewOrigin.ToFloatPtr() );
	//}

	static idVec4 zero( 0, 0, 0, 0 );
	static idVec4 one(1, 1, 1, 1 );
	static idVec4 negOne(-1, -1, -1, -1 );

	switch ( din->vertexColor ) {
	case SVC_IGNORE:		
		tr.vertexScaleModulateParam->SetVectorValue(zero);
		tr.vertexScaleAddParam->SetVectorValue(one);
		break;
	case SVC_MODULATE:
		tr.vertexScaleModulateParam->SetVectorValue(one);
		tr.vertexScaleAddParam->SetVectorValue(zero);
		break;
	case SVC_INVERSE_MODULATE:
		tr.vertexScaleModulateParam->SetVectorValue(negOne);
		tr.vertexScaleAddParam->SetVectorValue(one);		
		break;
	}

	// set the constant colors
	//glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, din->diffuseColor.ToFloatPtr() );
	//glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, din->specularColor.ToFloatPtr() );
	//tr.lightColorParam->SetVectorValue(din->diffuseColor);

	// set the textures

	tr.bumpmapTextureParam->SetImage(din->bumpImage);
	tr.albedoTextureParam->SetImage(din->diffuseImage);
	tr.specularTextureParam->SetImage(din->specularImage);
	
	// draw it
	if (din->surf->space->entityDef->parms.isSelected)
	{
		tr.interactionEditorSelectProgram[din->programVariant]->Bind();
	}
	else
	{
		if (din->surf->material->GetCustomInteractionProgram(din->programVariant))
		{
			din->surf->material->GetCustomInteractionProgram(din->programVariant)->Bind();
		}
		else
		{
			tr.interactionProgram[din->programVariant]->Bind();
		}
	}

	if (din->programVariant == PROG_VARIANT_SKINNED) {
		idDrawVert* ac = nullptr;
		idRenderModelMD5Instance* skinning = (idRenderModelMD5Instance*)din->surf->space->renderModel;
		RB_BindJointBuffer(skinning->jointBuffer, skinning->jointsInverted->ToFloatPtr(), skinning->numInvertedJoints, (void*)&ac->color, (void*)&ac->color2);
	}

	RB_DrawElementsWithCounters( din->surf->geo );
}

/*
==================
idRender::DrawForwardLit
==================
*/
void idRender::DrawForwardLit( void ) {
	const idMaterial	*lightShader;

	drawSurf_t** drawSurfs;
	int			numDrawSurfs;

	drawSurfs = (drawSurf_t**)&backEnd.viewDef->drawSurfs[0];
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	GL_SelectTexture( 0 );
#if !defined(__ANDROID__) //karin: GLES is programming pipeline
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
#else
	// glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
#endif

	// enable the vertex arrays
	glEnableVertexAttribArrayARB(8);
	glEnableVertexAttribArrayARB(9);
	glEnableVertexAttribArrayARB(10);
	glEnableVertexAttribArrayARB(11);
#if !defined(__ANDROID__) //karin: GLES is programming pipeline
	glEnableClientState(GL_COLOR_ARRAY);
#else
	glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
#endif

	for (int i = 0; i < numDrawSurfs; i++)
	{
		const drawSurf_t* drawSurf = drawSurfs[i];

		//if (drawSurf->numSurfRenderLights == 0)
		//	continue;

		if (drawSurf->geo->numVerts == 0)
			continue;

		if(drawSurf->space->entityDef == nullptr)
			continue;

		if(drawSurf->space->entityDef->parms.hideInMainView)
			continue;

		R_CreateAmbientCache((srfTriangles_t*)drawSurf->geo, true);;

		if (drawSurf->geo->numVerts == 0)
			continue;

		tr.numLightsParam->SetIntValue(drawSurf->numSurfRenderLights);
		
		// Build up the forward+ lighting path info for this draw surface.
		for (int d = 0; d < drawSurf->numSurfRenderLights; d++)
		{
			const idRenderLightCommitted* vLight = drawSurf->surfRenderLights[d];

			float lightOriginAlpha = 0;

			// globalLightExtents.
			if (vLight->lightDef->parms.lightType == LIGHT_TYPE_POINT)
			{
				tr.globalLightExtentsParam->SetVectorValue(idVec4(vLight->lightDef->parms.lightRadius.x, vLight->lightDef->parms.lightRadius.y, vLight->lightDef->parms.lightRadius.z, 1.0f), d);
			}
			else if(vLight->lightDef->parms.lightType == LIGHT_TYPE_SPOTLIGHT)
			{
				float maxLightDistance = idMath::Distance(vLight->lightDef->parms.start, vLight->lightDef->parms.target);
				idVec3 lightTarget = vLight->lightDef->parms.target + vLight->lightDef->parms.origin;
				idVec3 lightRight = vLight->lightDef->parms.target + vLight->lightDef->parms.right;
				idVec3 lightUp = vLight->lightDef->parms.target - vLight->lightDef->parms.up;

				idVec3 rightDir = vLight->lightDef->parms.origin - lightRight;
				rightDir.Normalize();

				idVec3 leftUp = vLight->lightDef->parms.origin - lightUp;
				leftUp.Normalize();

				lightOriginAlpha = leftUp * rightDir; // Cosine angle.

				idVec3 spotLightDir = vLight->lightDef->parms.origin - lightTarget;

				idVec4 lightRadius(maxLightDistance, spotLightDir.x, spotLightDir.y, spotLightDir.z);
				tr.globalLightExtentsParam->SetVectorValue(lightRadius, d);
			}
			else
			{
				common->FatalError("DrawForwardLit: Unknown light type");
			}
			
			// shadowmap info
			idVec4 shadowMapInfo(vLight->shadowMapSlice, renderShadowSystem.GetAtlasSampleScale(), renderShadowSystem.GetShadowMapAtlasSize(), !vLight->lightDef->parms.noShadows);
			tr.shadowMapInfoParm->SetVectorValue(shadowMapInfo, d);

			// globalLightOrigin
			idVec4 lightOrigin(vLight->lightDef->parms.origin.x, vLight->lightDef->parms.origin.y, vLight->lightDef->parms.origin.z, vLight->lightDef->parms.noSpecular);
			if (vLight->lightDef->parms.lightType == LIGHT_TYPE_SPOTLIGHT)
			{
				lightOrigin.w = lightOriginAlpha;
			}
			tr.globalLightOriginParam->SetVectorValue(lightOrigin, d);

			// light color.
			idVec4 lightColor(vLight->lightDef->parms.lightColor.x * backEnd.lightScale, vLight->lightDef->parms.lightColor.y * backEnd.lightScale, vLight->lightDef->parms.lightColor.z * backEnd.lightScale, vLight->lightDef->parms.lightType);
			tr.lightColorParam->SetVectorValue(lightColor, d);
		}

		// perform setup here that will be constant for all interactions
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

		// set the vertex pointers
		idDrawVert* ac = (idDrawVert*)vertexCache.Position(drawSurf->geo->ambientCache);
#if !defined(__ANDROID__) //karin: GLES is programming pipeline
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(idDrawVert), ac->color);
#else
		glVertexAttribPointer(PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, false, sizeof(idDrawVert), ac->color);
#endif
		glVertexAttribPointerARB(11, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
		glVertexAttribPointerARB(10, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[1].ToFloatPtr());
		glVertexAttribPointerARB(9, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->tangents[0].ToFloatPtr());
		glVertexAttribPointerARB(8, 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
#if !defined(__ANDROID__) //karin: GLES is programming pipeline
		glVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
#else
		glVertexAttribPointer(PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
#endif

		rvmProgramVariants_t programVariant = PROG_VARIANT_NONSKINNED;
		if (drawSurf->space->renderModel->IsSkeletalMesh()) {			
			programVariant = PROG_VARIANT_SKINNED;
		}

		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions(drawSurf, RB_ARB2_DrawInteraction, programVariant);

		// disable features
		if (drawSurf->space->renderModel->IsSkeletalMesh()) {
			RB_UnBindJointBuffer();
		}

		tr.interactionProgram[programVariant]->BindNull();

		backEnd.glState.currenttmu = -1;
		GL_SelectTexture(0);
	}

	glDisableVertexAttribArrayARB(8);
	glDisableVertexAttribArrayARB(9);
	glDisableVertexAttribArrayARB(10);
	glDisableVertexAttribArrayARB(11);
#if !defined(__ANDROID__) //karin: GLES is programming pipeline
	glDisableClientState(GL_COLOR_ARRAY);
#else
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
#endif

	// disable stencil shadow test
	glStencilFunc( GL_ALWAYS, 128, 255 );

	GL_SelectTexture( 0 );
#if !defined(__ANDROID__) //karin: GLES is programming pipeline
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
#else
	glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
#endif
}

