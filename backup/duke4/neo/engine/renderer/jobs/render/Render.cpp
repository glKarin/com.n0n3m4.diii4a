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

idRender render;

/*
=================
RB_DrawElementsImmediate

Draws with immediate mode commands, which is going to be very slow.
This should never happen if the vertex cache is operating properly.
=================
*/
void RB_DrawElementsImmediate( const srfTriangles_t *tri ) {
	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += tri->numIndexes;
	backEnd.pc.c_drawVertexes += tri->numVerts;

	if ( tri->ambientSurface != NULL  ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

#if !defined(__ANDROID__) //karin: GLES not support immediate mode rendering
	glBegin( GL_TRIANGLES );
	for ( int i = 0 ; i < tri->numIndexes ; i++ ) {
		glTexCoord2fv( tri->verts[ tri->indexes[i] ].st.ToFloatPtr() );
		glVertex3fv( tri->verts[ tri->indexes[i] ].xyz.ToFloatPtr() );
	}
	glEnd();
#else
	if(tri->numIndexes) {
		GLint arr[2];
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, arr);
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, arr + 1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glVertexAttribPointer(PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, false, sizeof(idDrawVert), tri->verts[0].xyz.ToFloatPtr());
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), tri->verts[0].st.ToFloatPtr());

		idVec4 orig_genericinfo = tr.genericShaderParam->GetVectorValue();
		idVec4 genericinfo;
		genericinfo.Zero();
		genericinfo.x = 1;
		tr.genericShaderParam->SetVectorValue(genericinfo);

		glDrawElements(GL_TRIANGLES, tri->numIndexes, GL_INDEX_TYPE, tri->indexes);

		glBindBuffer(GL_ARRAY_BUFFER, arr[0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arr[1]);
		tr.genericShaderParam->SetVectorValue(orig_genericinfo);
	}
#endif
}


/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const srfTriangles_t *tri ) {

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += tri->numIndexes;
	backEnd.pc.c_drawVertexes += tri->numVerts;

	if ( tri->ambientSurface != NULL  ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	if ( tri->indexCache && r_useIndexBuffers.GetBool() ) {
		glDrawElements( GL_TRIANGLES, 
						r_singleTriangle.GetBool() ? 3 : tri->numIndexes,
						GL_INDEX_TYPE,
                         (int *)vertexCache.Position( tri->indexCache ) );
		backEnd.pc.c_vboIndexes += tri->numIndexes;
	} else {
		if ( r_useIndexBuffers.GetBool() ) {
			vertexCache.UnbindIndex();
		}
		glDrawElements( GL_TRIANGLES, 
						r_singleTriangle.GetBool() ? 3 : tri->numIndexes,
						GL_INDEX_TYPE,
						tri->indexes );
	}
}

/*
================
RB_DrawShadowElementsWithCounters

May not use all the indexes in the surface if caps are skipped
================
*/
void RB_DrawShadowElementsWithCounters( const srfTriangles_t *tri, int numIndexes ) {
	backEnd.pc.c_shadowElements++;
	backEnd.pc.c_shadowIndexes += numIndexes;
	backEnd.pc.c_shadowVertexes += tri->numVerts;

	if ( tri->indexCache && r_useIndexBuffers.GetBool() ) {
		glDrawElements( GL_TRIANGLES, 
						r_singleTriangle.GetBool() ? 3 : numIndexes,
						GL_INDEX_TYPE,
						(int *)vertexCache.Position( tri->indexCache ) );
		backEnd.pc.c_vboIndexes += numIndexes;
	} else {
		if ( r_useIndexBuffers.GetBool() ) {
			vertexCache.UnbindIndex();
		}
		glDrawElements( GL_TRIANGLES, 
						r_singleTriangle.GetBool() ? 3 : numIndexes,
						GL_INDEX_TYPE,
						tri->indexes );
	}
}


/*
===============
RB_RenderTriangleSurface

Sets texcoord and vertex pointers
===============
*/
void RB_RenderTriangleSurface( const srfTriangles_t *tri ) {
	if ( !tri->ambientCache ) {
		RB_DrawElementsImmediate( tri );
		return;
	}


	idDrawVert *ac = (idDrawVert *)vertexCache.Position( tri->ambientCache );
#ifdef __ANDROID__ //karin: GLES is programming pipeline
	glVertexAttribPointer(PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());
#else
	glVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
	glTexCoordPointer( 2, GL_FLOAT, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
#endif

	RB_DrawElementsWithCounters( tri );
}

/*
===============
RB_T_RenderTriangleSurface

===============
*/
void RB_T_RenderTriangleSurface( const drawSurf_t *surf ) {
	RB_RenderTriangleSurface( surf->geo );
}

/*
===============
RB_EnterWeaponDepthHack
===============
*/
void RB_EnterWeaponDepthHack() {
#ifdef __ANDROID__ //karin: GLES using glDepthRangef
	glDepthRangef( 0, 0.5 );
#else
	glDepthRange( 0, 0.5 );
#endif

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] *= 0.25;

#ifdef __ANDROID__ //karin: GLES not support matrix functions
	RB_SetProjectionMatrix(matrix);
	float	mat[16];
	myGlMultMatrix(backEnd.currentSpace->modelViewMatrix, matrix, mat);
	RB_SetMVP(mat);
#else
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( matrix );
	glMatrixMode(GL_MODELVIEW);
#endif
}

/*
===============
RB_EnterModelDepthHack
===============
*/
void RB_EnterModelDepthHack( float depth ) {
#ifdef __ANDROID__ //karin: GLES using glDepthRangef
	glDepthRangef( 0.0f, 1.0f );
#else
	glDepthRange( 0.0f, 1.0f );
#endif

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] -= depth;

#ifdef __ANDROID__ //karin: GLES not support matrix functions
	RB_SetProjectionMatrix(matrix);
	float	mat[16];
	myGlMultMatrix(backEnd.currentSpace->modelViewMatrix, matrix, mat);
	RB_SetMVP(mat);
#else
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( matrix );
	glMatrixMode(GL_MODELVIEW);
#endif
}

/*
===============
RB_LeaveDepthHack
===============
*/
void RB_LeaveDepthHack() {
#ifdef __ANDROID__ //karin: GLES using glDepthRangef
	glDepthRangef( 0, 1 );
#else
	glDepthRange( 0, 1 );
#endif

#ifdef __ANDROID__ //karin: GLES not support matrix functions
	RB_SetProjectionMatrix(backEnd.viewDef->projectionMatrix);
	float	mat[16];
	myGlMultMatrix(backEnd.currentSpace->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
	RB_SetMVP(mat);
#else
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( backEnd.viewDef->projectionMatrix );
	glMatrixMode(GL_MODELVIEW);
#endif
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
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, 
											  void (*triFunc_)( const drawSurf_t *) ) {
	int				i;
	const drawSurf_t		*drawSurf;

	backEnd.currentSpace = NULL;

	for (i = 0  ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		// change the matrix if needed
		RB_SetMVP(drawSurf->space->mvp);
		RB_SetModelMatrix(drawSurf->space->modelMatrix);

		if ( drawSurf->space != backEnd.currentSpace ) {
#if !defined(__ANDROID__) //karin: GLES not support matrix functions
			glLoadMatrixf( drawSurf->space->modelViewMatrix );
#else
			RB_SetupMVP_DrawSurf(drawSurf);
#endif
		}

		if ( drawSurf->space->weaponDepthHack ) {
#ifdef __ANDROID__ //karin: backEnd.currentSpace not be the drawSurf
			RB_EnterWeaponDepthHack(drawSurf);
#else
			RB_EnterWeaponDepthHack();
#endif
		}

		if ( drawSurf->space->modelDepthHack != 0.0f ) {
#ifdef __ANDROID__ //karin: backEnd.currentSpace not be the drawSurf
			RB_EnterModelDepthHack( drawSurf );
#else
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
#endif
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			backEnd.currentScissor = drawSurf->scissorRect;
			glScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
				backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		// render it
		triFunc_( drawSurf );

		if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
#ifdef __ANDROID__ //karin: backEnd.currentSpace not be the drawSurf
			RB_LeaveDepthHack(drawSurf);
#else
			RB_LeaveDepthHack();
#endif
		}

		backEnd.currentSpace = drawSurf->space;
	}
}

/*
======================
RB_RenderDrawSurfChainWithFunction
======================
*/
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, 
										void (*triFunc_)( const drawSurf_t *) ) {
	const drawSurf_t		*drawSurf;

	backEnd.currentSpace = NULL;

	for ( drawSurf = drawSurfs ; drawSurf ; drawSurf = drawSurf->nextOnLight ) {
		// change the matrix if needed
		RB_SetMVP(drawSurf->space->mvp);
		RB_SetModelMatrix(drawSurf->space->modelMatrix);
		
		// change the matrix if needed
		if ( drawSurf->space != backEnd.currentSpace ) {
#if !defined(__ANDROID__) //karin: GLES not support matrix functions
			glLoadMatrixf( drawSurf->space->modelViewMatrix );
#else
			RB_SetupMVP_DrawSurf(drawSurf);
#endif
		}

		if ( drawSurf->space->weaponDepthHack ) {
#ifdef __ANDROID__ //karin: backEnd.currentSpace not be the drawSurf
			RB_EnterWeaponDepthHack(drawSurf);
#else
			RB_EnterWeaponDepthHack();
#endif
		}

		if ( drawSurf->space->modelDepthHack ) {
#ifdef __ANDROID__ //karin: backEnd.currentSpace not be the drawSurf
			RB_EnterModelDepthHack( drawSurf );
#else
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
#endif
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			backEnd.currentScissor = drawSurf->scissorRect;
			glScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
				backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		// render it
		triFunc_( drawSurf );

		if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
#ifdef __ANDROID__ //karin: backEnd.currentSpace not be the drawSurf
			RB_LeaveDepthHack(drawSurf);
#else
			RB_LeaveDepthHack();
#endif
		}

		backEnd.currentSpace = drawSurf->space;
	}
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float *shaderRegisters,
							   const textureStage_t *texture, float matrix[16] ) {
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
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture ) {
	float	matrix[16];

	RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
#if !defined(__ANDROID__) //karin: GLES not support texture matrix
	glMatrixMode( GL_TEXTURE );
	glLoadMatrixf( matrix );
	glMatrixMode( GL_MODELVIEW );
#else
	if (texture->hasMatrix)
		RB_SetTextureMatrix(matrix);
	else
		RB_SetTextureMatrix(mat4_identity.ToFloatPtr());
#endif
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters ) {
	if ( texture->cinematic ) {
		cinData_t	cin;

		if ( r_skipDynamicTextures.GetBool() ) {
			//globalImages->defaultImage->Bind();
			tr.albedoTextureParam->SetImage(globalImages->defaultImage);
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime( (int)(1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) ) );

		if ( cin.image ) {
			texture->cinematic->GetRenderImage()->UploadScratch(cin.image, cin.imageWidth, cin.imageHeight);
			tr.albedoTextureParam->SetImage(texture->cinematic->GetRenderImage());
		} else {
			//globalImages->blackImage->Bind();
			tr.albedoTextureParam->SetImage(globalImages->blackImage);
		}
	} else {		
		tr.albedoTextureParam->SetImage(texture->image[0]);
	}
}

/*
======================
RB_BindStageTexture
======================
*/
void RB_BindStageTexture( const float *shaderRegisters, const textureStage_t *texture, const drawSurf_t *surf ) {
	// image
	RB_BindVariableStageImage( texture, shaderRegisters );

	// texgens
	if ( texture->texgen == TG_DIFFUSE_CUBE ) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 3, GL_FLOAT, false, sizeof(idDrawVert), ((idDrawVert *)vertexCache.Position( surf->geo->ambientCache ))->normal.ToFloatPtr() );
#else
		glTexCoordPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ((idDrawVert *)vertexCache.Position( surf->geo->ambientCache ))->normal.ToFloatPtr() );
#endif
	}
	if ( texture->texgen == TG_SKYBOX_CUBE || texture->texgen == TG_WOBBLESKY_CUBE ) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 3, GL_FLOAT, false, 0, vertexCache.Position( surf->dynamicTexCoords ) );
#else
		glTexCoordPointer( 3, GL_FLOAT, 0, vertexCache.Position( surf->dynamicTexCoords ) );
#endif
	}
	if ( texture->texgen == TG_REFLECT_CUBE ) {
#ifdef __ANDROID__ //karin: GLES not support glTexGen
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		glVertexAttribPointer(PC_ATTRIB_INDEX_NORMAL, 3, GL_FLOAT, false, sizeof(idDrawVert), ((idDrawVert *)vertexCache.Position( surf->geo->ambientCache ))->normal.ToFloatPtr() );
#else
		glEnable( GL_TEXTURE_GEN_S );
		glEnable( GL_TEXTURE_GEN_T );
		glEnable( GL_TEXTURE_GEN_R );
		glTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT );
		glTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT );
		glTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT );
		glEnableClientState( GL_NORMAL_ARRAY );
		glNormalPointer( GL_FLOAT, sizeof( idDrawVert ), ((idDrawVert *)vertexCache.Position( surf->geo->ambientCache ))->normal.ToFloatPtr() );

		glMatrixMode( GL_TEXTURE );
#endif
		float	mat[16];

		R_TransposeGLMatrix( backEnd.viewDef->worldSpace.modelViewMatrix, mat );

#ifdef __ANDROID__ //karin: GLES not support texture matrix
		RB_SetTextureMatrix(mat);
#else
		glLoadMatrixf( mat );
		glMatrixMode( GL_MODELVIEW );
#endif
	}

	// matrix
	if ( texture->hasMatrix ) {
		RB_LoadShaderTextureMatrix( shaderRegisters, texture );
	}
}

/*
======================
RB_FinishStageTexture
======================
*/
void RB_FinishStageTexture( const textureStage_t *texture, const drawSurf_t *surf ) {
	if ( texture->texgen == TG_DIFFUSE_CUBE || texture->texgen == TG_SKYBOX_CUBE 
		|| texture->texgen == TG_WOBBLESKY_CUBE ) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), (void *)&(((idDrawVert *)vertexCache.Position( surf->geo->ambientCache ))->st) );
#else
		glTexCoordPointer( 2, GL_FLOAT, sizeof( idDrawVert ), 
			(void *)&(((idDrawVert *)vertexCache.Position( surf->geo->ambientCache ))->st) );
#endif
	}

	if ( texture->texgen == TG_REFLECT_CUBE ) {
#ifdef __ANDROID__ //karin: GLES not support texture matrix
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		RB_SetTextureMatrix(renderMatrix_identity);
#else
		glDisable( GL_TEXTURE_GEN_S );
		glDisable( GL_TEXTURE_GEN_T );
		glDisable( GL_TEXTURE_GEN_R );
		glTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		glTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		glTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		glDisableClientState( GL_NORMAL_ARRAY );

		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
#endif
	}

	if ( texture->hasMatrix ) {
#if !defined(__ANDROID__) //karin: GLES not support matrix functions
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
#else
		RB_SetTextureMatrix(renderMatrix_identity);
#endif
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
void RB_DetermineLightScale( void ) {
	backEnd.pc.maxLightValue = 1.0f;
	backEnd.lightScale = r_lightScale.GetFloat();
	backEnd.overBright = 1.0;
}


/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {
	// set the modelview matrix for the viewer
#if !defined(__ANDROID__) //karin: GLES not support matrix functions
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( backEnd.viewDef->projectionMatrix );
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#else
    RB_SetProjectionMatrix(backEnd.viewDef->projectionMatrix);
	RB_SetMVP(backEnd.viewDef->projectionMatrix);
#endif

	// set the window clipping
	glViewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1, 
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1, 
		backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
		backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	// the scissor may be smaller than the viewport for subviews
	glScissor( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1, 
		tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1, 
		backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
		backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );
	backEnd.currentScissor = backEnd.viewDef->scissor;

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if ( backEnd.viewDef->viewEntitys ) {
		glStencilMask(0xff);
		// some cards may have 7 bit stencil buffers, so don't assume this
		// should be 128
		glClearStencil(1 << (glConfig.stencilBits - 1));

		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable( GL_DEPTH_TEST );
	} else {
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_STENCIL_TEST );
	}

	backEnd.glState.faceCulling = -1;		// force face culling to set next time
	GL_Cull( CT_FRONT_SIDED );

}

/*
==================
R_SetDrawInteractions
==================
*/
void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs,
						  idImage **image, idVec4 matrix[2], float color[4] ) {
	int animationId = ((int)(tr.frameShaderTime * 2.0f)) % MAX_ANIM_MAPS;


	*image = surfaceStage->texture.image[animationId];

	if (*image == nullptr)
	{
		*image = surfaceStage->texture.image[0];
	}

	if ( surfaceStage->texture.hasMatrix ) {
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
		if ( matrix[0][3] < -40 || matrix[0][3] > 40 ) {
			matrix[0][3] -= (int)matrix[0][3];
		}
		if ( matrix[1][3] < -40 || matrix[1][3] > 40 ) {
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

	if ( color ) {
		for ( int i = 0 ; i < 4 ; i++ ) {
			color[i] = surfaceRegs[surfaceStage->color.registers[i]];
			// clamp here, so card with greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			if ( color[i] < 0 ) {
				color[i] = 0;
			} else if ( color[i] > 1.0 ) {
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
static void RB_SubmittInteraction( drawInteraction_t *din, void (*DrawInteraction)(const drawInteraction_t *) ) {
	if ( !din->bumpImage ) {
		return;
	}

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}
	if ( !din->specularImage || r_skipSpecular.GetBool() || din->ambientLight ) {
		din->specularImage = globalImages->blackImage;
	}
	if ( !din->bumpImage || r_skipBump.GetBool() ) {
		din->bumpImage = globalImages->flatNormalMap;
	}

	// if we wouldn't draw anything, don't call the Draw function
	if ( 
		( ( din->diffuseColor[0] > 0 || 
		din->diffuseColor[1] > 0 || 
		din->diffuseColor[2] > 0 ) && din->diffuseImage != globalImages->blackImage )
		|| ( ( din->specularColor[0] > 0 || 
		din->specularColor[1] > 0 || 
		din->specularColor[2] > 0 ) && din->specularImage != globalImages->blackImage ) ) {
		DrawInteraction( din );
	}
}

/*
=============
RB_CreateSingleDrawInteractions

This can be used by different draw_* backends to decompose a complex light / surface
interaction into primitive interactions
=============
*/
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *), rvmProgramVariants_t programVariant) {
	const idMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	drawInteraction_t	inter;

	if ( r_skipInteractions.GetBool() || !surf->geo || !surf->geo->ambientCache ) {
		return;
	}

	if ( tr.logFile ) {
		//RB_LogComment( "---------- RB_CreateSingleDrawInteractions %s on %s ----------\n", lightShader->GetName(), surfaceShader->GetName() );
	}

	if (surf->forceTwoSided)
	{
		GL_Cull(CT_TWO_SIDED);
	}
	else
	{
		GL_Cull(surf->material->GetCullType());
	}

	// change the matrix if needed
	RB_SetMVP(surf->space->mvp);
	RB_SetModelMatrix(surf->space->modelMatrix);

	// change the matrix and light projection vectors if needed
	if ( surf->space != backEnd.currentSpace ) {
		backEnd.currentSpace = surf->space;
#if !defined(__ANDROID__) //karin: GLES not support matrix functions
		glLoadMatrixf( surf->space->modelViewMatrix );
#else
		RB_SetupMVP_DrawSurf(surf);
#endif
	}

	// change the scissor if needed
	if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( surf->scissorRect ) ) {
		backEnd.currentScissor = surf->scissorRect;
		glScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
			backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	}

	// hack depth range if needed
	if ( surf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
	}

	if ( surf->space->modelDepthHack ) {
		RB_EnterModelDepthHack( surf->space->modelDepthHack );
	}

	inter.surf = surf;
	inter.programVariant = programVariant;
//	inter.lightFalloffImage = vLight->falloffImage;

	//R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3() );
	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, inter.localViewOrigin.ToVec3() );
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	//inter.ambientLight = lightShader->IsAmbientLight();

	// the base projections may be modified by texture matrix on light stages
	//idPlane lightProject[4];
	//for ( int i = 0 ; i < 4 ; i++ ) {
	//	R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i] );
	//}

	for ( int lightStageNum = 0 ; lightStageNum < 1; lightStageNum++ ) {


		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		//float lightColor[4];
		//
		//// backEnd.lightScale is calculated so that lightColor[] will never exceed
		//// tr.backEndRendererMaxLight
		//lightColor[0] = backEnd.lightScale * vLight->lightDef->parms.lightColor.x;
		//lightColor[1] = backEnd.lightScale * vLight->lightDef->parms.lightColor.y;
		//lightColor[2] = backEnd.lightScale * vLight->lightDef->parms.lightColor.z;
		//lightColor[3] = lightRegs[ lightStage->color.registers[3] ];

		// go through the individual stages
		for ( int surfaceStageNum = 0 ; surfaceStageNum < surfaceShader->GetNumStages() ; surfaceStageNum++ ) {
			const shaderStage_t	*surfaceStage = surfaceShader->GetStage( surfaceStageNum );

			switch( surfaceStage->lighting ) {
				case SL_AMBIENT: {
					// ignore ambient stages while drawing interactions
					break;
				}
				case SL_BUMP: {
					// ignore stage that fails the condition
					if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
						break;
					}
					// draw any previous interaction
					RB_SubmittInteraction( &inter, DrawInteraction );
					inter.diffuseImage = NULL;
					inter.specularImage = NULL;
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, NULL );
					break;
				}
				case SL_DIFFUSE: {
					// ignore stage that fails the condition
					if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
						break;
					}
					if ( inter.diffuseImage ) {
						RB_SubmittInteraction( &inter, DrawInteraction );
					}
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.diffuseImage,
											inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
					//inter.diffuseColor[0] *= lightColor[0];
					//inter.diffuseColor[1] *= lightColor[1];
					//inter.diffuseColor[2] *= lightColor[2];
					//inter.diffuseColor[3] *= lightColor[3];
					inter.vertexColor = surfaceStage->vertexColor;
					break;
				}
				case SL_SPECULAR: {
					// ignore stage that fails the condition
					if ( !surfaceRegs[ surfaceStage->conditionRegister ] ) {
						break;
					}
					if ( inter.specularImage ) {
						RB_SubmittInteraction( &inter, DrawInteraction );
					}
					R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.specularImage,
											inter.specularMatrix, inter.specularColor.ToFloatPtr() );
					//inter.specularColor[0] *= lightColor[0];
					//inter.specularColor[1] *= lightColor[1];
					//inter.specularColor[2] *= lightColor[2];
					//inter.specularColor[3] *= lightColor[3];
					inter.vertexColor = surfaceStage->vertexColor;
					break;
				}
			}
		}

		// draw the final interaction
		RB_SubmittInteraction( &inter, DrawInteraction );
	}

	// unhack depth range if needed
	if ( surf->space->weaponDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
	}
}


/*
================
idRender::PrepareStageTexturing
================
*/
void idRender::PrepareStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf, idDrawVert* ac) {
	// set privatePolygonOffset if necessary
	if (pStage->privatePolygonOffset) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset);
	}

	// set the texture matrix if needed
	if (pStage->texture.hasMatrix) {
		RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);
	}
#ifdef __ANDROID__ //karin: GLES not support texture matrix
	else
		RB_SetTextureMatrix(mat4_identity.ToFloatPtr());
#endif

	// texgens
	if (pStage->texture.texgen == TG_DIFFUSE_CUBE) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 3, GL_FLOAT, false, sizeof(idDrawVert), ac->normal.ToFloatPtr());
#else
		glTexCoordPointer(3, GL_FLOAT, sizeof(idDrawVert), ac->normal.ToFloatPtr());
#endif
	}
	else if (pStage->texture.texgen == TG_SKYBOX_CUBE || pStage->texture.texgen == TG_WOBBLESKY_CUBE) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 3, GL_FLOAT, false, 0, vertexCache.Position(surf->dynamicTexCoords));
#else
		glTexCoordPointer(3, GL_FLOAT, 0, vertexCache.Position(surf->dynamicTexCoords));
#endif
	}
	else if (pStage->texture.texgen == TG_SCREEN) {
#if !defined(__ANDROID__) //karin: GLES not support glTexGen
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
#else
		RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture/*, true*/);
        float mat[16];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);

        float plane[4];
        plane[0] = mat[0 * 4 + 0];
        plane[1] = mat[1 * 4 + 0];
        plane[2] = mat[2 * 4 + 0];
        plane[3] = mat[3 * 4 + 0];
		tr.texgenS->SetVectorValue(plane);

        plane[0] = mat[0 * 4 + 1];
        plane[1] = mat[1 * 4 + 1];
        plane[2] = mat[2 * 4 + 1];
        plane[3] = mat[3 * 4 + 1];
		tr.texgenT->SetVectorValue(plane);

        plane[0] = mat[0 * 4 + 3];
        plane[1] = mat[1 * 4 + 3];
        plane[2] = mat[2 * 4 + 3];
        plane[3] = mat[3 * 4 + 3];
		tr.texgenQ->SetVectorValue(plane);
#endif
	}

	else if (pStage->texture.texgen == TG_SCREEN2) {
#if !defined(__ANDROID__) //karin: GLES not support glTexGen
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
#else
		RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture/*, true*/);
        float mat[16];
		myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);

        float plane[4];
        plane[0] = mat[0 * 4 + 0];
        plane[1] = mat[1 * 4 + 0];
        plane[2] = mat[2 * 4 + 0];
        plane[3] = mat[3 * 4 + 0];
		tr.texgenS->SetVectorValue(plane);

        plane[0] = mat[0 * 4 + 1];
        plane[1] = mat[1 * 4 + 1];
        plane[2] = mat[2 * 4 + 1];
        plane[3] = mat[3 * 4 + 1];
		tr.texgenT->SetVectorValue(plane);

        plane[0] = mat[0 * 4 + 3];
        plane[1] = mat[1 * 4 + 3];
        plane[2] = mat[2 * 4 + 3];
        plane[3] = mat[3 * 4 + 3];
		tr.texgenQ->SetVectorValue(plane);
#endif
	}
#ifdef __ANDROID__ //karin: default pass texcoord
	else { // explicit
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), reinterpret_cast<void *>(&ac->st));
	}
#endif
}

/*
================
idRender::FinishStageTexturing
================
*/
void idRender::FinishStageTexturing(const shaderStage_t* pStage, const drawSurf_t* surf, idDrawVert* ac) {
	// unset privatePolygonOffset if necessary
	if (pStage->privatePolygonOffset && !surf->material->TestMaterialFlag(MF_POLYGONOFFSET)) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (pStage->texture.texgen == TG_DIFFUSE_CUBE || pStage->texture.texgen == TG_SKYBOX_CUBE
		|| pStage->texture.texgen == TG_WOBBLESKY_CUBE) {
#ifdef __ANDROID__ //karin: GLES is programming pipeline
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), (void*)&ac->st);
#else
		glTexCoordPointer(2, GL_FLOAT, sizeof(idDrawVert), (void*)&ac->st);
#endif
	}

	if (pStage->texture.texgen == TG_SCREEN) {
#if !defined(__ANDROID__) //karin: GLES not support glTexGen
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_Q);
#else
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), (void*)&ac->st);
#endif
	}
	if (pStage->texture.texgen == TG_SCREEN2) {
#if !defined(__ANDROID__) //karin: GLES not support glTexGen
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_Q);
#else
		glVertexAttribPointer(PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, false, sizeof(idDrawVert), (void*)&ac->st);
#endif
	}

	if (pStage->texture.texgen == TG_GLASSWARP) {
		if (tr.backEndRenderer == BE_ARB2 /*|| tr.backEndRenderer == BE_NV30*/) {
			GL_SelectTexture(2);
			globalImages->BindNull();

			GL_SelectTexture(1);
			if (pStage->texture.hasMatrix) {
				RB_LoadShaderTextureMatrix(surf->shaderRegisters, &pStage->texture);
			}
#if !defined(__ANDROID__) //karin: GLES not support glTexGen
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_Q);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif
			globalImages->BindNull();
			GL_SelectTexture(0);
		}
	}

	if (pStage->texture.texgen == TG_REFLECT_CUBE) {
		if (tr.backEndRenderer == BE_ARB2) {
			// see if there is also a bump map specified
			const shaderStage_t* bumpStage = surf->material->GetBumpStage();
			if (bumpStage) {
				// per-pixel reflection mapping with bump mapping
				GL_SelectTexture(1);
				globalImages->BindNull();
				GL_SelectTexture(0);

				glDisableVertexAttribArrayARB(9);
				glDisableVertexAttribArrayARB(10);
			}
			else {
				// per-pixel reflection mapping without bump mapping
			}

#ifdef __ANDROID__ //karin: GLES is programming pipeline
			glDisableVertexAttribArray(PC_ATTRIB_INDEX_NORMAL);
#else
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			glDisable(GL_VERTEX_PROGRAM_ARB);
			// Fixme: Hack to get around an apparent bug in ATI drivers.  Should remove as soon as it gets fixed.
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
#endif
		}
		else {
#ifdef __ANDROID__ //karin: GLES not support glTexGen
			glDisableVertexAttribArray(PC_ATTRIB_INDEX_NORMAL);
			RB_SetTextureMatrix(renderMatrix_identity);
#else
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
			glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glDisableClientState(GL_NORMAL_ARRAY);

			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
#endif
		}
	}

	if (pStage->texture.hasMatrix) {
#if !defined(__ANDROID__) //karin: GLES not support texture matrix
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
#else
		RB_SetTextureMatrix(renderMatrix_identity);
#endif
	}
}

/*
=============
RB_DrawView
=============
*/
void idRender::RenderSingleView( const void *data ) {
	const drawSurfsCommand_t	*cmd;
	drawSurf_t** drawSurfs;
	int			numDrawSurfs;

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.viewDef = cmd->viewDef;
	
	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if ( !backEnd.viewDef->numDrawSurfs ) {
		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if ( r_skipRender.GetBool() && backEnd.viewDef->viewEntitys ) {
		return;
	}

	// skip render context sets the wgl context to NULL,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	if ( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys ) {
		GLimp_DeactivateContext();
	}

	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;

	RB_ShowOverdraw();

	RB_LogComment("---------- RB_STD_DrawView ----------\n");

	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

	drawSurfs = (drawSurf_t**)&backEnd.viewDef->drawSurfs[0];
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	RenderShadowMaps();

	RB_SetViewMatrix(backEnd.viewDef->worldSpace.modelViewMatrix); // Despite this being called "modelview" its just "view".
	RB_SetProjectionMatrix(backEnd.viewDef->projectionMatrix);

	// If we have a backend rendertexture, assign it here.
	if (backEnd.renderTexture)
	{
		backEnd.renderTexture->MakeCurrent();

		idVec4 value;
		value.Zero();
		value.x = backEnd.renderTexture->GetWidth();
		value.y = backEnd.renderTexture->GetHeight();
		value.z = tr.frameCount;

		tr.screenInfoParam->SetVectorValue(value);
	}
	else if(backEnd.viewDef->renderView.isEditor) // Editor can have full post processing disabled. 
	{
		idVec4 value;
		value.Zero();		
		value.z = tr.frameCount;

		tr.screenInfoParam->SetVectorValue(value);
	}

	// clear the z buffer, set the projection matrix, etc
	RB_BeginDrawingView();

	if (backEnd.clearColor || backEnd.clearDepth)
	{
		if (backEnd.clearDepth) {
			glStencilMask(0xff);
			// some cards may have 7 bit stencil buffers, so don't assume this
			// should be 128
			glClearStencil(1 << (glConfig.stencilBits - 1));
#ifdef __ANDROID__ //karin: GLES using glClearDepthf
			glClearDepthf(backEnd.clearDepthValue);
#else
			glClearDepth(backEnd.clearDepthValue);
#endif
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		if (backEnd.clearColor) {
			glClearColor(backEnd.clearColorValue[0], backEnd.clearColorValue[1], backEnd.clearColorValue[2], backEnd.clearColorValue[3]);
			glClear(GL_COLOR_BUFFER_BIT);
		}

#ifdef __ANDROID__ //karin: GLES using glClearDepthf
		glClearDepthf(1.0f);
#else
		glClearDepth(1.0f);
#endif

		backEnd.clearColor = false;
		backEnd.clearDepth = false;
	}


	// decide how much overbrighting we are going to do
	RB_DetermineLightScale();

	// fill the depth buffer and clear color buffer to black except on
	// subviews
	FillDepthBuffer(drawSurfs, numDrawSurfs);

	// Run Occlusion Queries.
	DrawTestOcclusion();

	// main light renderer
	DrawForwardLit();

	// disable stencil shadow test
	glStencilFunc(GL_ALWAYS, 128, 255);

	// now draw any non-light dependent shading passes
	int	processed = DrawShaderPasses(drawSurfs, numDrawSurfs);

	// now draw any post-processing effects using _currentRender
	if (processed < numDrawSurfs) {
		DrawShaderPasses(drawSurfs + processed, numDrawSurfs - processed);
	}

	RB_RenderDebugTools(drawSurfs, numDrawSurfs);

	// restore the context for 2D drawing if we were stubbing it out
	if ( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys ) {
		GLimp_ActivateContext();
		RB_SetDefaultGLState();
	}
}

void RB_EnterWeaponDepthHack(const drawSurf_t *surf)
{
#ifdef __ANDROID__ //karin: GLES using glDepthRangef
	glDepthRangef( 0, 0.5 );
#else
	glDepthRange( 0, 0.5 );
#endif

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] *= 0.25;

#ifdef __ANDROID__ //karin: GLES not support matrix functions
	RB_SetProjectionMatrix(matrix);
	float	mat[16];
	myGlMultMatrix(surf->space->modelViewMatrix, matrix, mat);
	RB_SetMVP(mat);
#else
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( matrix );
	glMatrixMode(GL_MODELVIEW);
#endif
}

void RB_EnterModelDepthHack(const drawSurf_t *surf )
{
#ifdef __ANDROID__ //karin: GLES using glDepthRangef
	glDepthRangef( 0.0f, 1.0f );
#else
	glDepthRange( 0.0f, 1.0f );
#endif

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] -= surf->space->modelDepthHack;

#ifdef __ANDROID__ //karin: GLES not support matrix functions
	RB_SetProjectionMatrix(matrix);
	float	mat[16];
	myGlMultMatrix(surf->space->modelViewMatrix, matrix, mat);
	RB_SetMVP(mat);
#else
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( matrix );
	glMatrixMode(GL_MODELVIEW);
#endif
}

void RB_LeaveDepthHack(const drawSurf_t *surf)
{
#ifdef __ANDROID__ //karin: GLES using glDepthRangef
	glDepthRangef( 0, 1 );
#else
	glDepthRange( 0, 1 );
#endif

#ifdef __ANDROID__ //karin: GLES not support matrix functions
	RB_SetProjectionMatrix(backEnd.viewDef->projectionMatrix);
	float	mat[16];
	myGlMultMatrix(surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat);
	RB_SetMVP(mat);
#else
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( backEnd.viewDef->projectionMatrix );
	glMatrixMode(GL_MODELVIEW);
#endif
}
