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

#include "renderer/backend/RenderBackend.h"

#include "renderer/backend/stages/AmbientOcclusionStage.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/FrameBuffer.h"

idCVar r_useNewRenderPasses( "r_useNewRenderPasses", "2", CVAR_INTEGER | CVAR_ARCHIVE| CVAR_RENDERER,
	"Use new refactored code for rendering surface/light material stages",
	0, 2
);

RenderBackend renderBackendImpl;
RenderBackend *renderBackend = &renderBackendImpl;

namespace {
	void CreateLightgemFbo( FrameBuffer *fbo ) {
		fbo->Init( DARKMOD_LG_RENDER_WIDTH, DARKMOD_LG_RENDER_WIDTH );
#ifdef __ANDROID__ //karin: only GL_RGBA8 for render buffer
        fbo->AddColorRenderBuffer( 0, GL_RGBA8 );
#else
        fbo->AddColorRenderBuffer( 0, GL_RGB8 );
#endif
		fbo->AddDepthStencilRenderBuffer( GL_DEPTH24_STENCIL8 );
	}
}

RenderBackend::RenderBackend() {}

void RenderBackend::Init() {
	initialized = true;

	depthStage.Init();
	interactionStage.Init();
	stencilShadowStage.Init();
	shadowMapStage.Init();
	surfacePassesStage.Init();
	lightPassesStage.Init();
	frobOutlineStage.Init();
	tonemapStage.Init();

	lightgemFbo = frameBuffers->CreateFromGenerator( "lightgem", CreateLightgemFbo );
	qglGenBuffers( 3, lightgemPbos );
	for ( int i = 0; i < 3; ++i ) {
		qglBindBuffer( GL_PIXEL_PACK_BUFFER, lightgemPbos[i] );
#ifdef __ANDROID__ //karin: RGBA
		qglBufferData( GL_PIXEL_PACK_BUFFER, DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * 4, nullptr, GL_STREAM_READ );
#else
		qglBufferData( GL_PIXEL_PACK_BUFFER, DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * 3, nullptr, GL_STREAM_READ );
#endif
	}
	qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 ); // reset to default to allow sysmem ReadPixels if LG disabled
}

void RenderBackend::Shutdown() {
	if (!initialized)
		return;
	qglDeleteBuffers( 3, lightgemPbos );
	
	tonemapStage.Shutdown();
	frobOutlineStage.Shutdown();
	lightPassesStage.Shutdown();
	surfacePassesStage.Shutdown();
	shadowMapStage.Shutdown();
	stencilShadowStage.Shutdown();
	interactionStage.Shutdown();
	depthStage.Shutdown();
}

void RenderBackend::DrawView( const viewDef_t *viewDef, bool colorIsBackground ) {
	TRACE_GL_SCOPE( "DrawView" );

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if ( viewDef->viewEntitys && r_skipRender.GetBool() ) {
		return;
	}

	// skip render context sets the wgl context to NULL,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	if ( viewDef->viewEntitys && r_skipRenderContext.GetBool() ) {
		GLimp_DeactivateContext();
	}
	backEnd.pc.c_surfaces += viewDef->numDrawSurfs;

	RB_ShowOverdraw();


	int processed;

	backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;

	// clear the framebuffer, set the projection matrix, etc
	RB_BeginDrawingView( colorIsBackground );

	backEnd.lightScale = r_lightScale.GetFloat();
	backEnd.overBright = 1.0f;

	drawSurf_t **drawSurfs = ( drawSurf_t ** )&viewDef->drawSurfs[ 0 ];
	int numDrawSurfs = viewDef->numDrawSurfs;

	// if we are just doing 2D rendering, no need to fill the depth buffer
	if ( viewDef->viewEntitys ) {
		// fill the depth buffer and clear color buffer to black except on subviews
		depthStage.DrawDepth( viewDef, drawSurfs, numDrawSurfs );
		if( ambientOcclusion->ShouldEnableForCurrentView() ) {
			ambientOcclusion->ComputeSSAOFromDepth();
		}
		DrawShadowsAndInteractions( viewDef );
	}

	if ( r_useNewRenderPasses.GetInteger() > 0 ) {
		int beforePostproc = 0;
		while ( beforePostproc < numDrawSurfs && drawSurfs[beforePostproc]->sort < SS_POST_PROCESS )
			beforePostproc++;
		const drawSurf_t **postprocSurfs = (const drawSurf_t **)drawSurfs + beforePostproc;
		int postprocCount = numDrawSurfs - beforePostproc;

		surfacePassesStage.DrawSurfaces( viewDef, (const drawSurf_t **)drawSurfs, beforePostproc );

		if ( (r_frobOutline.GetInteger() > 0 || r_newFrob.GetInteger() == 1) && !viewDef->IsLightGem() ) {
			frobOutlineStage.DrawFrobOutline( drawSurfs, numDrawSurfs );
		}

		if ( r_useNewRenderPasses.GetInteger() == 2 ) {
			LightPassesStage::DrawMask mask;
			mask.opaque = true;
			mask.translucent = false;
			lightPassesStage.DrawAllFogLights( viewDef, mask );

			lightPassesStage.DrawAllBlendLights( viewDef );
			volumetric->RenderAll( viewDef );

			if ( surfacePassesStage.NeedCurrentRenderTexture( viewDef, postprocSurfs, postprocCount ) )
				frameBuffers->UpdateCurrentRenderCopy();

			surfacePassesStage.DrawSurfaces( viewDef, postprocSurfs, postprocCount );

			// 2.08: second fog pass, translucent only
			mask.opaque = false;
			mask.translucent = true;
			lightPassesStage.DrawAllFogLights( viewDef, mask );
		}
		else {
			extern void RB_STD_FogAllLights( bool translucent );
			RB_STD_FogAllLights( false );

			if ( surfacePassesStage.NeedCurrentRenderTexture( viewDef, postprocSurfs, postprocCount ) )
				frameBuffers->UpdateCurrentRenderCopy();

			surfacePassesStage.DrawSurfaces( viewDef, postprocSurfs, postprocCount );

			RB_STD_FogAllLights( true ); // 2.08: second fog pass, translucent only
		}
	}
	else {

		// now draw any non-light dependent shading passes
		int RB_STD_DrawShaderPasses( drawSurf_t **drawSurfs, int numDrawSurfs, bool postProcessing );
		processed = RB_STD_DrawShaderPasses( drawSurfs, numDrawSurfs, false );

		if (
			(r_frobOutline.GetInteger() > 0 || r_newFrob.GetInteger() == 1) && 
			!viewDef->IsLightGem()
		) {
			frobOutlineStage.DrawFrobOutline( drawSurfs, numDrawSurfs );
		}

		// fog and blend lights
		extern void RB_STD_FogAllLights( bool translucent );
		RB_STD_FogAllLights( false );

		// now draw any post-processing effects using _currentRender
		if ( processed < numDrawSurfs ) {
			RB_STD_DrawShaderPasses( drawSurfs + processed, numDrawSurfs - processed, true );
		}

		RB_STD_FogAllLights( true ); // 2.08: second fog pass, translucent only
	}

#if !defined(__ANDROID__) //karin: TODO: crash when build release
	RB_RenderDebugTools( drawSurfs, numDrawSurfs );
#endif

	// restore the context for 2D drawing if we were stubbing it out
	if ( r_skipRenderContext.GetBool() && viewDef->viewEntitys ) {
		GLimp_ActivateContext();
		RB_SetDefaultGLState();
	}
}

void RenderBackend::DrawLightgem( const viewDef_t *viewDef, byte *lightgemData ) {
	FrameBuffer *currentFbo = frameBuffers->activeFbo;
	FrameBuffer *renderFbo = frameBuffers->currentRenderFbo;
	frameBuffers->currentRenderFbo = lightgemFbo;
	lightgemFbo->Bind();
	
	DrawView( viewDef, false );

	// asynchronously copy contents of the lightgem framebuffer to a pixel buffer
	qglBindBuffer( GL_PIXEL_PACK_BUFFER, lightgemPbos[currentLightgemPbo] );
	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits
#ifdef __ANDROID__ //karin: RGBA
	qglReadPixels( 0, 0, DARKMOD_LG_RENDER_WIDTH, DARKMOD_LG_RENDER_WIDTH, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
#else
    qglReadPixels( 0, 0, DARKMOD_LG_RENDER_WIDTH, DARKMOD_LG_RENDER_WIDTH, GL_RGB, GL_UNSIGNED_BYTE, nullptr );
#endif

	// advance PBO index and actually copy the data stored in that PBO to local memory
	// this PBO is from a previous frame, and data transfer should thus be reasonably fast
	currentLightgemPbo = ( currentLightgemPbo + 1 ) % 3;
	qglBindBuffer( GL_PIXEL_PACK_BUFFER, lightgemPbos[currentLightgemPbo] );
	qglGetBufferSubData( GL_PIXEL_PACK_BUFFER, 0, DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * 3, lightgemData );

	qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	currentFbo->Bind();
	frameBuffers->currentRenderFbo = renderFbo;
}

void RenderBackend::EndFrame() {}

void RenderBackend::DrawInteractionsWithShadowMapping( const viewDef_t *viewDef, const viewLight_t *vLight ) {
	TRACE_GL_SCOPE( "DrawLight_ShadowMap" );

	if ( vLight->globalShadows || vLight->localShadows ) {
		ShadowMapStage::DrawMask mask;

		mask.clear = true;
		mask.global = true;
		mask.local = false;
		shadowMapStage.DrawShadowMapSingleLight( viewDef, vLight, mask );

		interactionStage.DrawInteractions( viewDef, vLight, DSL_LOCAL, nullptr );

		mask.clear = false;
		mask.global = false;
		mask.local = true;
		shadowMapStage.DrawShadowMapSingleLight( viewDef, vLight, mask );

	} else {
		interactionStage.DrawInteractions( viewDef, vLight, DSL_LOCAL, nullptr );
	}

	interactionStage.DrawInteractions( viewDef, vLight, DSL_GLOBAL, nullptr );

	interactionStage.DrawInteractions( viewDef, vLight, DSL_TRANSLUCENT, nullptr );

	GLSLProgram::Deactivate();
}

void RenderBackend::DrawInteractionsWithStencilShadows( const viewDef_t *viewDef, const viewLight_t *vLight ) {
	TRACE_GL_SCOPE( "DrawLight_Stencil" );

	bool useShadowFbo = r_softShadowsQuality.GetBool() && !viewDef->IsLightGem();
	idScreenRect lightScissor = stencilShadowStage.ExpandScissorRectForSoftShadows( viewDef, vLight->scissorRect );

	// clear the stencil buffer if needed
	if ( vLight->globalShadows || vLight->localShadows ) {
		backEnd.currentScissor = lightScissor;
		FB_ApplyScissor();

		if ( useShadowFbo ) {
			frameBuffers->EnterShadowStencil();
		}
		qglClear( GL_STENCIL_BUFFER_BIT );
	} else {
		// no shadows, so no need to read or write the stencil buffer
		qglStencilFunc( GL_ALWAYS, 128, 255 );
	}

	if ( vLight->globalShadows ) {
		stencilShadowStage.DrawStencilShadows( viewDef, vLight, vLight->globalShadows );
		if ( useShadowFbo && r_multiSamples.GetInteger() > 1 ) {
			backEnd.currentScissor = lightScissor;
			FB_ApplyScissor();
			frameBuffers->ResolveShadowStencilAA();
		}
	}

	if ( useShadowFbo ) {
		frameBuffers->LeaveShadowStencil();
	}
	interactionStage.DrawInteractions( viewDef, vLight, DSL_LOCAL, nullptr );

	if ( useShadowFbo ) {
		frameBuffers->EnterShadowStencil();
	}

	if ( vLight->localShadows ) {
		stencilShadowStage.DrawStencilShadows( viewDef, vLight, vLight->localShadows );
		if ( useShadowFbo && r_multiSamples.GetInteger() > 1 ) {
			backEnd.currentScissor = lightScissor;
			FB_ApplyScissor();
			frameBuffers->ResolveShadowStencilAA();
		}
	}
	if ( vLight->globalShadows || vLight->localShadows ) {
		stencilShadowStage.FillStencilShadowMipmaps( viewDef, lightScissor );
	}

	if ( useShadowFbo ) {
		frameBuffers->LeaveShadowStencil();
	}

	interactionStage.DrawInteractions( viewDef, vLight, DSL_GLOBAL, &stencilShadowStage.stencilShadowMipmap );

	qglStencilFunc( GL_ALWAYS, 128, 255 );
	interactionStage.DrawInteractions( viewDef, vLight, DSL_TRANSLUCENT, nullptr );

	GLSLProgram::Deactivate();
}

void RenderBackend::DrawShadowsAndInteractions( const viewDef_t *viewDef ) {
	TRACE_GL_SCOPE( "LightInteractions" );

	bool singlePassShadowMaps = r_shadows.GetInteger() == 2 && r_shadowMapSinglePass.GetBool();
	if ( singlePassShadowMaps ) {
		for ( int pass = 0; pass < 2; pass++ ) {
			// draw all shadow maps: global on pass 0, add local on pass 1
			ShadowMapStage::DrawMask mask;
			mask.clear = ( pass == 0 );
			mask.global = ( pass == 0 );
			mask.local = ( pass == 1 );
			shadowMapStage.DrawShadowMapAllLights( viewDef, mask );

			// draw interactions for all shadow-mapped lights
			for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
				if ( !interactionStage.ShouldDrawLight( vLight ) || vLight->shadows != LS_MAPS )
					continue;

				if ( pass == 0 ) {
					interactionStage.DrawInteractions( viewDef, vLight, DSL_LOCAL, nullptr );
				} else {
					interactionStage.DrawInteractions( viewDef, vLight, DSL_GLOBAL, nullptr );
					interactionStage.DrawInteractions( viewDef, vLight, DSL_TRANSLUCENT, nullptr );
				}
			}
		}
	}

	// draw interactions and shadows for each light separately
	// note: even with single-pass shadow maps on, we might still have some stencil-shadowed lights to process here
	for ( viewLight_t *vLight = viewDef->viewLights; vLight; vLight = vLight->next ) {
		if ( !interactionStage.ShouldDrawLight( vLight ) )
			continue;
	
		if ( vLight->shadows == LS_MAPS ) {
			if ( !singlePassShadowMaps ) {
				DrawInteractionsWithShadowMapping( viewDef, vLight );
			}
		} else {
			DrawInteractionsWithStencilShadows( viewDef, vLight );
		}
	}

	// disable stencil shadow test
	qglStencilFunc( GL_ALWAYS, 128, 255 );
	GL_SelectTexture( 0 );
}

void RenderBackend::Tonemap() {
	tonemapStage.ApplyTonemap( frameBuffers->defaultFbo, globalImages->guiRenderImage );
}
