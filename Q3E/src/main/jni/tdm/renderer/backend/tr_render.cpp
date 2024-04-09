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
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/RenderBackend.h"

/*
  back end scene + lights rendering functions
*/


/*
=================
RB_DrawElementsImmediate

Draws with immediate mode commands, which is going to be very slow.
This should never happen if the vertex cache is operating properly.
=================
*/
void RB_DrawElementsImmediate( const srfTriangles_t *tri ) {
	if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
		common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
		return;
	}
	vertexCache.UnbindIndex();
	static vertCacheHandle_t nil;
	vertexCache.VertexPosition( nil );
	auto ac = tri->verts;
	qglDrawElements( GL_TRIANGLES, tri->numIndexes, GL_INDEX_TYPE, tri->indexes );
}

static void RB_PerfCounters( const drawSurf_t* surf, int instances, drawCallKind_t kind ) {
	if ( backEnd.viewDef->IsLightGem() )
		return;	// ignore lightgem
	if ( !backEnd.viewDef->viewEntitys )
		return;	// GUI

	if ( r_showPrimitives.GetBool() ) {
		backEnd.pc.c_drawCalls[kind]++;
		backEnd.pc.c_drawIndexes[kind] += instances * surf->numIndexes;
		backEnd.pc.c_drawVerts[kind] += instances * surf->numVerts;	// we pay for vertex shader cost per instance
	}

	if ( r_showEntityDraws && surf->space ) {
		if ( r_showEntityDraws & 4 )
			( (viewEntity_t*) surf->space )->drawCalls += surf->numIndexes / 3;
		else
			( (viewEntity_t*) surf->space )->drawCalls++;
	}
}

/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const drawSurf_t *surf, drawCallKind_t kind ) {
	RB_PerfCounters( surf, 1, kind );
	void* indexPtr;
	if ( surf->indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( surf->indexCache );
	} else {
		//note: this happens briefly when index cache is being resized
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		vertexCache.UnbindIndex();
		if ( !surf->frontendGeo ) return;
		indexPtr = surf->frontendGeo->indexes; // FIXME
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElements( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr );
	else
		qglDrawElementsBaseVertex( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, basePointer );
}

void RB_DrawTriangles( const srfTriangles_t &tri ) {
	void* indexPtr;
	if ( tri.indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( tri.indexCache );
	} else {
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		vertexCache.UnbindIndex();
		indexPtr = tri.indexes;
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElements( GL_TRIANGLES, tri.numIndexes, GL_INDEX_TYPE, indexPtr );
	else
		qglDrawElementsBaseVertex( GL_TRIANGLES, tri.numIndexes, GL_INDEX_TYPE, indexPtr, basePointer );
}

/*
================
RB_DrawElementsInstanced
================
*/
void RB_DrawElementsInstanced( const drawSurf_t *surf, int instances, drawCallKind_t kind ) {
	RB_PerfCounters( surf, instances, kind );

	void* indexPtr;
	if ( surf->indexCache.IsValid() ) {
		indexPtr = vertexCache.IndexPosition( surf->indexCache );
	} else {
		if (r_glCoreProfile.GetInteger() > 0) {
#ifdef _DEBUG
			common->Warning("Drawing without index buffer not supported in Core profile!");
#endif
			return;
		}
		//TODO: remove this code?
		indexPtr = surf->frontendGeo->indexes; // FIXME?
		vertexCache.UnbindIndex();
	}
	int basePointer = vertexCache.GetBaseVertex();
	if ( basePointer < 0 )
		qglDrawElementsInstanced( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, instances );
	else
		qglDrawElementsInstancedBaseVertex( GL_TRIANGLES, surf->numIndexes, GL_INDEX_TYPE, indexPtr, instances, basePointer );
}

/*
===============
RB_RenderTriangleSurface

Sets vertex pointers
===============
*/
void RB_RenderTriangleSurface( const drawSurf_t *surf ) {
	if ( !surf->ambientCache.IsValid() ) {
		//RB_DrawElementsImmediate( surf->frontendGeo );
		return;
	}
	vertexCache.VertexPosition( surf->ambientCache );
	RB_DrawElementsWithCounters( surf );
}

/*
===============
RB_T_RenderTriangleSurface

===============
*/
void RB_T_RenderTriangleSurface( const drawSurf_t *surf ) {
	RB_RenderTriangleSurface( surf );
}

/*
===============
RB_EnterWeaponDepthHack
===============
*/
void RB_EnterWeaponDepthHack() {
#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0.0f, 0.5f );
#else
	qglDepthRange( 0.0f, 0.5f );
#endif

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] *= 0.25f;

	// FIXME: this part is broken since storing projection matrix in uniform block
	auto prog = GLSLProgram::GetCurrentProgram();
	if ( prog ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		//transformUniforms->projectionMatrix.Set( matrix );
	}
}

/*
===============
RB_EnterModelDepthHack
===============
*/
void RB_EnterModelDepthHack( float depth ) {
	// FIXME: this is completely broken, is it even still needed?
	
#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0.0f, 1.0f );
#else
	qglDepthRange( 0.0f, 1.0f );
#endif

	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] -= depth;

	auto prog = GLSLProgram::GetCurrentProgram();
	if ( prog ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		//transformUniforms->projectionMatrix.Set( matrix );
	}
}

/*
===============
RB_LeaveDepthHack
===============
*/
void RB_LeaveDepthHack() {
#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0.0f, 1.0f );
#else
	qglDepthRange( 0.0f, 1.0f );
#endif

	if ( auto prog = GLSLProgram::GetCurrentProgram() ) {
		Uniforms::Global* transformUniforms = prog->GetUniformGroup<Uniforms::Global>();
		//transformUniforms->projectionMatrix.Set( backEnd.viewDef->projectionMatrix );
	}
}


static void RB_RenderDrawSurfWithFunction( const drawSurf_t *drawSurf, void ( *triFunc_ )( const drawSurf_t * ) ) {
	if ( drawSurf->space != backEnd.currentSpace ) {
		if( GLSLProgram::GetCurrentProgram() != nullptr ) {
			Uniforms::Global *transformUniforms = GLSLProgram::GetCurrentProgram()->GetUniformGroup<Uniforms::Global>();
			transformUniforms->Set( drawSurf->space );
		}
		GL_CheckErrors();
	}

	if ( drawSurf->space->weaponDepthHack ) {
		RB_EnterWeaponDepthHack();
		GL_CheckErrors();
	}

	if ( drawSurf->space->modelDepthHack != 0.0f ) {
		RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
		GL_CheckErrors();
	}

#if 1 // duzenko: this is needed for portal fogging e.g. in Lone Salvation
	backEnd.currentScissor = drawSurf->scissorRect;
	FB_ApplyScissor();
	GL_CheckErrors();
#endif

	// render it
	triFunc_( drawSurf );
	GL_CheckErrors();

	if ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
		GL_CheckErrors();
	}

	// mark currentSpace if we have drawn.
	backEnd.currentSpace = drawSurf->space;
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
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, void ( *triFunc_ )( const drawSurf_t * ) ) {
	GL_CheckErrors();
	backEnd.currentSpace = nullptr;
	for ( int i = 0; i < numDrawSurfs ; i++ ) {
		RB_RenderDrawSurfWithFunction( drawSurfs[i], triFunc_ );
	}
	GL_CheckErrors();
}
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, void ( *triFunc_ )( const drawSurf_t * ) ) {
	GL_CheckErrors();
	backEnd.currentSpace = nullptr;
	for ( const drawSurf_t *drawSurf = drawSurfs; drawSurf; drawSurf = drawSurf->nextOnLight ) {
		RB_RenderDrawSurfWithFunction( drawSurf, triFunc_ );
	}
	GL_CheckErrors();
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
idMat4 RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture ) {
	idMat4 result = mat4_identity;

	if ( texture->hasMatrix ) {
		// fetch register values into 2 x 3 matrix
		idVec3 texmat[2];
		for ( int r = 0; r < 2; r++ )
			for ( int c = 0; c < 3; c++ )
				texmat[r][c] = shaderRegisters[ texture->matrix[r][c] ];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		for ( int r = 0; r < 2; r++ ) {
			float &x = texmat[r][2];
			if ( idMath::Fabs(x) > 40.0f )
				x -= ( int )x;
		}

		// expand to 4 x 4 matrix
		for ( int r = 0; r < 2; r++ )
			result[r].Set( texmat[r][0], texmat[r][1], 0, texmat[r][2] );
	}

	return result;
}
void RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] ) {
	RB_GetShaderTextureMatrix( shaderRegisters, texture ).ToGL( matrix );
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const shaderStage_t* pStage ) {
	const auto *texture = &pStage->texture;
	if ( !texture->hasMatrix )
		return;

	auto prog = GLSLProgram::GetCurrentProgram();
	if ( !prog )
		return;

	if ( shaderRegisters ) {
		float	matrix[16];
		RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
		prog->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( matrix );
	} else 
		prog->GetUniformGroup<Uniforms::Global>()->textureMatrix.Set( mat4_identity );

	/*qglMatrixMode( GL_TEXTURE );
	qglLoadMatrixf( matrix );
	qglMatrixMode( GL_MODELVIEW );*/
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters ) {
	if ( texture->cinematic ) {
		if ( r_skipDynamicTextures.GetBool() ) {
			globalImages->defaultImage->Bind();
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		const cinData_t cin = texture->cinematic->ImageForTime( ( int )( 1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) ) );

		if ( cin.image ) {
			globalImages->cinematicImage->UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
		} else {
			globalImages->blackImage->Bind();
		}
	} else if ( texture->image ) {
		texture->image->Bind();
	}
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView( bool colorIsBackground ) {
	auto& viewDef = backEnd.viewDef;
	// set the modelview matrix for the viewer
	GL_SetProjection( (float *)backEnd.viewDef->projectionMatrix );

	// set the window clipping
	FB_ApplyViewport();
	// the scissor may be smaller than the viewport for subviews
	backEnd.currentScissor = backEnd.viewDef->scissor;
	FB_ApplyScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if ( backEnd.viewDef->viewEntitys ) {
		qglStencilMask( 0xff );
		// we use framebuffers with one of attachments: GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8 or GL_STENCIL_INDEX8
		// all of them are exactly 8-bit, so middle value 128 serves as "zero" for us
		qglClearStencil( 128 );
		qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		qglEnable( GL_DEPTH_TEST );
	} else {
		qglDisable( GL_DEPTH_TEST );
		qglDisable( GL_STENCIL_TEST );
	}

	if ( !colorIsBackground && !backEnd.viewDef->renderView.isOverlay ) {
		// clear screen for debugging and to ensure well-defined behavior
		// automatically enable this with several other debug tools
		// that might leave unrendered portions of the screen
		if ( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool() ) {
			float c[3];
			if ( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 ) {
				qglClearColor( c[0], c[1], c[2], 1 );
			} else if ( r_clear.GetInteger() == 2 ) {
				qglClearColor( 0.0f, 0.0f,  0.0f, 1.0f );
			} else if ( r_showOverDraw.GetBool() ) {
				qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
			} else {
				qglClearColor( 0.4f, 0.0f, 0.25f, 1.0f );
			}
			qglClear( GL_COLOR_BUFFER_BIT );
		}
	}

	backEnd.glState.faceCulling = -1;		// force face culling to set next time

	GL_Cull( CT_FRONT_SIDED );
	GL_CheckErrors();
}

/*
==================
R_SetDrawInteractions
==================
*/
void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs,
                           idImage **image, idVec4 matrix[2], float color[4] ) {
	*image = surfaceStage->texture.image;

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
		if ( matrix[0][3] < -40.0f || matrix[0][3] > 40.0f ) {
			matrix[0][3] -= ( int )matrix[0][3];
		}

		if ( matrix[1][3] < -40.0f || matrix[1][3] > 40.0f ) {
			matrix[1][3] -= ( int )matrix[1][3];
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
		color[0] = surfaceRegs[surfaceStage->color.registers[0]];
		color[1] = surfaceRegs[surfaceStage->color.registers[1]];
		color[2] = surfaceRegs[surfaceStage->color.registers[2]];
		color[3] = surfaceRegs[surfaceStage->color.registers[3]];
	}
}
