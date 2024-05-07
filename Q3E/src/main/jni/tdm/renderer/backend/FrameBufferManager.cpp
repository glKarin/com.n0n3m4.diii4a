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
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/FrameBuffer.h"

FrameBufferManager frameBuffersImpl;
FrameBufferManager *frameBuffers = &frameBuffersImpl;

namespace {
	GLenum ColorBufferFormat() {
#ifdef __ANDROID__ //karin: only GL_RGBA8 for render buffer
        return GL_RGBA8;
#else
		if ( r_fboColorBits.GetInteger() == 64 ) {
			return GL_RGBA16F;
		}
		if ( r_fboColorBits.GetInteger() == 16 ) {
			return GL_RGB5_A1;
		}
		return ( glConfig.srgb ? GL_SRGB_ALPHA : GL_RGBA );
#endif
	}

	GLenum DepthBufferFormat() {
		return ( r_fboDepthBits.GetInteger() == 32 ) ? GL_DEPTH32F_STENCIL8 : GL_DEPTH24_STENCIL8;
	}
}

FrameBufferManager::~FrameBufferManager() {
	Shutdown();
}

void FrameBufferManager::Init() {
	UpdateResolutionAndFormats();
	defaultFbo = CreateFromGenerator( "default", FrameBuffer::CreateDefaultFrameBuffer );
	primaryFbo = CreateFromGenerator( "primary", [this](FrameBuffer *fbo) { CreatePrimary( fbo ); } );
	resolveFbo = CreateFromGenerator( "resolve", [this](FrameBuffer *fbo) { CreateResolve( fbo ); } );
	guiFbo = CreateFromGenerator( "gui", [this](FrameBuffer *fbo) { CreateGui( fbo ); } );
	shadowStencilFbo = CreateFromGenerator( "shadowStencil", [this](FrameBuffer *fbo) { CreateStencilShadow( fbo ); } );
	shadowMapFbo = CreateFromGenerator( "shadowMap", [this](FrameBuffer *fbo) { CreateMapsShadow( fbo ); } );

	activeFbo = defaultFbo;
	activeDrawFbo = defaultFbo;
}

void FrameBufferManager::Shutdown() {
	for (auto fbo : fbos) {
		delete fbo;
	}
	fbos.ClearFree();

	if (pbo != 0) {
		qglDeleteBuffers(1, &pbo);
		pbo = 0;
	}
}

FrameBuffer * FrameBufferManager::CreateFromGenerator( const idStr &name, Generator generator ) {
	for (auto fbo : fbos) {
		if (name == fbo->Name()) {
			fbo->Destroy();
			fbo->SetGenerator( generator );
			return fbo;
		}
	}
	FrameBuffer *fbo = new FrameBuffer(name, generator);
	fbos.Append( fbo );
	return fbo;
}

void FrameBufferManager::PurgeAll() {
	for (auto fbo : fbos) {
		fbo->Destroy();
	}

	if (pbo != 0) {
		qglDeleteBuffers(1, &pbo);
		pbo = 0;
	}
}

void FrameBufferManager::BeginFrame() {
	if (
		r_multiSamples.IsModified() || 
		r_fboResolution.IsModified() ||
		r_fboColorBits.IsModified() ||
		r_fboDepthBits.IsModified() ||
		r_shadows.IsModified() ||
		r_shadowMapSize.IsModified()
	) {
		r_multiSamples.ClearModified();
		r_fboResolution.ClearModified();
		r_fboColorBits.ClearModified();
		r_fboDepthBits.ClearModified();
		r_shadows.ClearModified();
		r_shadowMapSize.ClearModified();

		// something FBO-related has changed, let's recreate everything from scratch
		UpdateResolutionAndFormats();
		PurgeAll();
	}

	currentRenderFbo = defaultFbo;
	defaultFbo->Bind();
}

void FrameBufferManager::EnterPrimary() {
	if ( r_frontBuffer.GetBool() ) return;
	depthCopiedThisView = false;
	if (currentRenderFbo == primaryFbo) return;

	currentRenderFbo = primaryFbo;
	primaryFbo->Bind();

	GL_ViewportVidSize( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
	             tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
	             backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
	             backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	GL_ScissorVidSize( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
	            tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
	            backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
	            backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );
}

idCVar r_fboScaling( "r_fboScaling", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "nearest/linear FBO scaling" );

void FrameBufferManager::LeavePrimary(bool copyToDefault) {
	if ( r_frontBuffer.GetBool() ) return;
	// if we want to do tonemapping later, we need to continue to render to a texture,
	// otherwise we can render the remaining UI views straight to the back buffer
	FrameBuffer *targetFbo = r_tonemap ? guiFbo : defaultFbo;
	if (currentRenderFbo == targetFbo) return;

	currentRenderFbo = targetFbo;

	if (copyToDefault) {
		if ( r_multiSamples.GetInteger() > 1 ) {
			ResolvePrimary();
			resolveFbo->BlitTo( targetFbo, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		} else {
			primaryFbo->BlitTo( targetFbo, GL_COLOR_BUFFER_BIT, r_fboScaling.GetBool() ? GL_LINEAR : GL_NEAREST );
			backEnd.pc.c_copyFrameBuffer++;
		}

		if ( r_frontBuffer.GetBool() && !r_tonemap ) {
			qglFinish();
		}
	}

	targetFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );
}

void FrameBufferManager::EnterShadowStencil() {
	if( r_multiSamples.GetInteger() > 1 ) {
		// with MSAA on, we need to render against the multisampled primary buffer, otherwise stencil is drawn
		// against a lower-quality depth map which may cause render errors with shadows
		primaryFbo->Bind();
	} else {
		// most vendors can't do separate stencil so we need to copy depth from the main/default FBO
		if ( !depthCopiedThisView ) {
			currentRenderFbo->BlitTo( shadowStencilFbo, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
			depthCopiedThisView = true;
		}

		shadowStencilFbo->Bind();
	}
}

void FrameBufferManager::LeaveShadowStencil() {
	currentRenderFbo->Bind();
}

void FrameBufferManager::ResolveShadowStencilAA() {
	if ( r_useScissor.GetBool() ) {
		// copy only the region selected by light scissor
		primaryFbo->BlitToVidSize(
			shadowStencilFbo, GL_STENCIL_BUFFER_BIT, GL_NEAREST,
			backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
			backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.GetWidth(),
			backEnd.currentScissor.GetHeight()
		);
	} else {
		// copy whole buffer
		primaryFbo->BlitTo( shadowStencilFbo, GL_STENCIL_BUFFER_BIT, GL_NEAREST );
	}
}

void FrameBufferManager::EnterShadowMap() {
	shadowMapFbo->Bind();
	qglDepthMask( true );
	GL_State( GLS_DEPTHFUNC_LESS ); // reset in RB_GLSL_CreateDrawInteractions
	// the caller is now responsible for proper setup of viewport/scissor
}

void FrameBufferManager::LeaveShadowMap() {
	currentRenderFbo->Bind();

	const idScreenRect &r = backEnd.viewDef->viewport;
	GL_ViewportVidSize( r.x1, r.y1, r.x2 - r.x1 + 1, r.y2 - r.y1 + 1 );
	GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
}

void FrameBufferManager::ResolvePrimary( GLbitfield mask, GLenum filter ) {
	primaryFbo->BlitTo( resolveFbo, mask, filter );
}

void FrameBufferManager::UpdateCurrentRenderCopy() {
	TRACE_GL_SCOPE( "UpdateCurrentRenderCopy" );
	currentRenderFbo->BlitTo( resolveFbo, GL_COLOR_BUFFER_BIT, GL_NEAREST );
	backEnd.pc.c_copyFrameBuffer++;
}

void FrameBufferManager::UpdateCurrentDepthCopy() {
	TRACE_GL_SCOPE( "UpdateCurrentDepthCopy" );
	currentRenderFbo->BlitTo( resolveFbo, GL_DEPTH_BUFFER_BIT, GL_NEAREST );
	backEnd.pc.c_copyDepthBuffer++;
}

void FrameBufferManager::CopyRender( const copyRenderCommand_t &cmd ) {
	//stgatilov #4754: this happens during lightgem calculating in minimized windowed TDM
	if ( cmd.imageWidth * cmd.imageHeight == 0 ) {
		return;	//no pixels to be read
	}
	int backEndStartTime = Sys_Milliseconds();

	if ( activeFbo == defaultFbo ) { // #4425: not applicable, raises gl errors
#ifdef _OPENGLES3 //karin: OpenGLES3.0
		qglReadBuffer( GL_BACK );
#endif
	}

	if ( activeFbo == primaryFbo && r_multiSamples.GetInteger() > 1 ) {
		ResolvePrimary( GL_COLOR_BUFFER_BIT );
		resolveFbo->Bind();
	}

	if ( cmd.buffer ) {
		CopyRender( cmd.buffer, cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight, cmd.usePBO );
	}

	if ( cmd.image )
		CopyRender( cmd.image, cmd.x, cmd.y, cmd.imageWidth, cmd.imageHeight );

	currentRenderFbo->Bind();

	int backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.msec += backEndFinishTime - backEndStartTime;
}

void FrameBufferManager::UpdateResolutionAndFormats() {
	float scale = r_fboResolution.GetFloat();
	renderWidth = static_cast< int >( glConfig.vidWidth * scale );
	renderHeight = static_cast< int >( glConfig.vidHeight * scale );
	shadowAtlasSize = 6 * r_shadowMapSize;
	colorFormat = ColorBufferFormat();
	depthStencilFormat = DepthBufferFormat();
}

void FrameBufferManager::CreatePrimary( FrameBuffer *primary ) {
	int msaa = r_multiSamples.GetInteger();
	primary->Init( renderWidth, renderHeight, msaa );
    primary->AddColorRenderBuffer( 0, colorFormat );
	primary->AddDepthStencilRenderBuffer( depthStencilFormat );
}

void FrameBufferManager::CreateResolve( FrameBuffer *resolve ) {
	resolve->Init( renderWidth, renderHeight );
	globalImages->currentRenderImage->GenerateAttachment( renderWidth, renderHeight, colorFormat, GL_LINEAR );
	resolve->AddColorRenderTexture( 0, globalImages->currentRenderImage );
	globalImages->currentDepthImage->GenerateAttachment( renderWidth, renderHeight, depthStencilFormat, GL_NEAREST );
	globalImages->currentDepthImage->Bind();
	GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
	qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	resolve->AddDepthRenderTexture( globalImages->currentDepthImage );
}

void FrameBufferManager::CreateGui( FrameBuffer *gui ) {
	gui->Init( glConfig.vidWidth, glConfig.vidHeight );
	globalImages->guiRenderImage->GenerateAttachment( glConfig.vidWidth, glConfig.vidHeight, colorFormat, GL_NEAREST );
	gui->AddColorRenderTexture( 0, globalImages->guiRenderImage );
}

bool FrameBufferManager::EnsureScratchImageCreated( idImageScratch *image, int width, int height ) {
	// #6300: normally, scratch images are generated as FBO attachments when FBO is generated
	// however, some images (_scratch) is never attached to FBO, it is filled with texture copy command instead
	// so we need to generate these images when we first fill them --- and CopyRender method is the only method which fills them
	// #6424: changing r_fboResolution also changes their resolution, in which case we need to recreate them
	if ( image->texnum == idImage::TEXTURE_NOT_LOADED || image->uploadWidth != width || image->uploadHeight != height ) {
		image->PurgeImage();
		image->GenerateAttachment( width, height, colorFormat, GL_LINEAR, GL_CLAMP_TO_EDGE );
		image->Bind();

		// framebuffer has alpha channel (and some materials seemingly use it for interpass communication)
		// however, alpha contents of the full image are pretty undefined, and they are copied to scratch images
		// some postprocessing effects alpha-blend scratch image with specified "alpha" and standard blending mode (e.g. textures/water_source/water_reflective2)
		// leftover alpha content affect these effects in undesirable way --- especially bad if alpha of scratch image can exceed 1.0
		GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ONE };
		qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}

	if ( image->texnum == idImage::TEXTURE_NOT_LOADED || image->uploadWidth != width || image->uploadHeight != height ) {
		return false;	// should not happen
	} else {
		return true;
	}
}

void FrameBufferManager::CopyRender( idImageScratch* image, int x, int y, int imageWidth, int imageHeight ) {
	if ( activeFbo == primaryFbo || activeFbo == resolveFbo ) {
		x *= r_fboResolution.GetFloat();
		y *= r_fboResolution.GetFloat();
		imageWidth *= r_fboResolution.GetFloat();
		imageHeight *= r_fboResolution.GetFloat();
	}

	// if copying into _scratch or _xray, then make sure it is generated and has proper size
	if ( !EnsureScratchImageCreated( image, imageWidth, imageHeight ) ) {
		common->Warning( "FrameBufferManager::CopyRender failed to create texture" );
		assert(false);
		return;
	}
	image->Bind();

	// otherwise, just subimage upload it so that drivers can tell we are going to be changing
	// it and don't try and do a texture compression or some other silliness
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
}

void FrameBufferManager::CopyRender( unsigned char *buffer, int x, int y, int imageWidth, int imageHeight, bool usePBO ) {
	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits

	// #4395 lightem pixel pack buffer optimization
	if ( usePBO ) {
		static int pboSize = -1;

		if ( !pbo ) {
#ifdef __ANDROID__ //karin: RGBA
			pboSize = imageWidth * imageHeight * 4;
#else
			pboSize = imageWidth * imageHeight * 3;
#endif
			qglGenBuffers( 1, &pbo );
			qglBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );
			qglBufferData( GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ );
			qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
		}

#ifdef __ANDROID__ //karin: RGBA
		if ( imageWidth * imageHeight * 4 != pboSize ) {
#else
		if ( imageWidth * imageHeight * 3 != pboSize ) {
#endif
			common->Error( "CaptureRenderToBuffer: wrong PBO size %dx%d/%d", imageWidth, imageHeight, pboSize );
		}
		qglBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );

		byte *ptr = reinterpret_cast< byte * >( qglMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY ) );

		// #4395 moved to initializer
		if ( ptr ) {
			memcpy( buffer, ptr, pboSize );
			qglUnmapBuffer( GL_PIXEL_PACK_BUFFER );
		}

		// revelator: added c++11 nullptr
#ifdef __ANDROID__ //karin: RGBA
		qglReadPixels( x, y, imageWidth, imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
#else
        qglReadPixels( x, y, imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE, nullptr );
#endif
		qglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	} else {
#ifdef __ANDROID__ //karin: RGBA
		qglReadPixels( x, y, imageWidth, imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
#else
        qglReadPixels( x, y, imageWidth, imageHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer );
#endif
	}
}

void FrameBufferManager::CreateStencilShadow( FrameBuffer *shadow ) {
	shadow->Init( renderWidth, renderHeight );
	globalImages->shadowDepthFbo->GenerateAttachment( renderWidth, renderHeight, depthStencilFormat, GL_NEAREST );
	shadow->AddDepthStencilRenderTexture( globalImages->shadowDepthFbo );
}

void FrameBufferManager::CreateMapsShadow( FrameBuffer *shadow ) {
	shadow->Init( shadowAtlasSize, shadowAtlasSize );
	globalImages->shadowAtlas->GenerateAttachment( shadowAtlasSize, shadowAtlasSize, GL_DEPTH_COMPONENT32F, GL_NEAREST );
	shadow->AddDepthRenderTexture( globalImages->shadowAtlas );
}

void FrameBufferManager::GetShadowMapBudget( int &numAtlasTiles, int &tileSize ) {
	tileSize = r_shadowMapSize.GetInteger();
	numAtlasTiles = shadowAtlasSize / tileSize;
	assert( tileSize * numAtlasTiles <= shadowAtlasSize );
}

idList<renderCrop_t> FrameBufferManager::CreateShadowMapPages( const idList<int> &ratios, int denominator ) {
	int n = ratios.Num();
	int minRatio = denominator;

	assert( idMath::IsPowerOfTwo( denominator ) );
	for ( int r : ratios ) {
		assert( idMath::IsPowerOfTwo( r ) );
		minRatio = idMath::Imin( minRatio, r );
	}

	// add initial full tiles
	// note: we allocate first row with x in range [0..sz)
	// this would be OK if K-th layer is at offset K * sz
	// but it is not... see the end of the function
	idList<renderCrop_t> freeTiles, subTiles;
	for ( int i = 0; i < shadowAtlasSize; i += r_shadowMapSize.GetInteger() ) {
		freeTiles.AddGrow( renderCrop_t{
			0, i, r_shadowMapSize.GetInteger(), r_shadowMapSize.GetInteger()
		} );
	}
	freeTiles.Reverse();

	// initially, all lights have no window (i.e. no shadows)
	idList<renderCrop_t> result;
	result.SetNum( n );
	memset( result.Ptr(), 0, result.Allocated() );

	for ( int currentRatio = denominator; currentRatio >= minRatio; currentRatio >>= 1 ) {
		for ( int i = 0; i < n; i++ ) {
			if ( ratios[i] != currentRatio )
				continue;
			if ( freeTiles.Num() == 0 )
				break;	// budget overflow: should never happen on correct input data

			// assign first available tile of matching size
			result[i] = freeTiles.Pop();
		}

		// split available tiles into 4 subtiles
		subTiles.SetNum( freeTiles.Num() * 4 );
		for ( int i = 0; i < freeTiles.Num(); i++ ) {
			renderCrop_t crop = freeTiles[i];
			int sw = crop.width / 2;
			int sh = crop.height / 2;
			subTiles[4 * i + 3] = { crop.x + 0 , crop.y + 0 , sw, sh };
			subTiles[4 * i + 2] = { crop.x + sw, crop.y + 0 , sw, sh };
			subTiles[4 * i + 1] = { crop.x + 0 , crop.y + sh, sw, sh };
			subTiles[4 * i + 0] = { crop.x + sw, crop.y + sh, sw, sh };
		}
		freeTiles.Swap( subTiles );
	}

	// the engine actually expects all layers of cubemap contiguous by X
	// so we need to convert our pages
	for ( int i = 0; i < result.Num(); i++ )
		result[i].x *= 6;

	return result;
}

