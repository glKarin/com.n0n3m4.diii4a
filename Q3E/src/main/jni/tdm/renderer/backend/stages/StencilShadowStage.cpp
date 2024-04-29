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

#include "renderer/backend/stages/StencilShadowStage.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLUniforms.h"

struct StencilShadowStage::Uniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( Uniforms )

	DEFINE_UNIFORM( mat4, modelViewMatrix )
	DEFINE_UNIFORM( mat4, projectionMatrix )
	DEFINE_UNIFORM( vec4, localLightOrigin )
};

void StencilShadowStage::Init() {
	stencilShadowShader = programManager->LoadFromFiles(
		"stencil_shadow",
		"stages/stencil/stencil_shadow.vert.glsl",
		"stages/stencil/stencil_shadow.frag.glsl"
	);
}

void StencilShadowStage::Shutdown() {
	ShutdownMipmaps();
}

void StencilShadowStage::DrawStencilShadows( const viewDef_t *viewDef, const viewLight_t *vLight, const drawSurf_t *shadowSurfs ) {
	if ( !shadowSurfs || !r_shadows.GetInteger() ) {
		return;
	}
	TRACE_GL_SCOPE( "StencilShadowPass" );

	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglPolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
		qglEnable( GL_POLYGON_OFFSET_FILL );
	}
	GL_State( GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	GL_Cull( CT_TWO_SIDED );
	stencilShadowShader->Activate();
	uniforms = stencilShadowShader->GetUniformGroup<Uniforms>();

	uniforms->projectionMatrix.Set( viewDef->projectionMatrix );

	idList<const drawSurf_t*> depthFailSurfs;
	idList<const drawSurf_t*> depthPassSurfs;
	for (const drawSurf_t *surf = shadowSurfs; surf; surf = surf->nextOnLight) {
		if ( !surf->shadowCache.IsValid() ) {
			continue;
		}
		if ( surf->scissorRect.IsEmptyWithZ() ) {
			continue;
		}

		bool external = r_useExternalShadows.GetInteger() && !(surf->dsFlags & DSF_VIEW_INSIDE_SHADOW);
		if (external) {
			depthPassSurfs.AddGrow( surf );
		} else {
			depthFailSurfs.AddGrow( surf );
		}
	}

	// draw depth-fail stencil shadows
	qglStencilOpSeparate( viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP );
	qglStencilOpSeparate( viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
	DrawSurfs( viewDef, vLight, depthFailSurfs.Ptr(), depthFailSurfs.Num() );
	// draw traditional depth-pass stencil shadows
	qglStencilOpSeparate( viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP );
	qglStencilOpSeparate( viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP );
	DrawSurfs( viewDef, vLight, depthPassSurfs.Ptr(), depthPassSurfs.Num() );

	// reset state
	GL_Cull( CT_FRONT_SIDED );
	if ( r_shadowPolygonFactor.GetFloat() || r_shadowPolygonOffset.GetFloat() ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	// FIXME: move to interaction stage
	if ( !r_softShadowsQuality.GetBool() || viewDef->IsLightGem() )
		qglStencilFunc( GL_GEQUAL, 128, 255 ); // enable stencil test - the shadow volume path
	
}

void StencilShadowStage::DrawSurfs( const viewDef_t *viewDef, const viewLight_t *vLight, const drawSurf_t **surfs, size_t count ) {
	if (count == 0) {
		return;
	}

	idScreenRect lightScissor = ExpandScissorRectForSoftShadows( viewDef, vLight->scissorRect );
	backEnd.currentScissor = lightScissor;
	FB_ApplyScissor();
	DepthBoundsTest depthBoundsTest( lightScissor );

	for (size_t i = 0; i < count; ++i) {
		const drawSurf_t *surf = surfs[i];

		// note: already checked for validity in parent scope
		vertexCache.VertexPosition( surf->shadowCache, ATTRIB_SHADOW );

		idScreenRect surfScissor = ExpandScissorRectForSoftShadows( viewDef, surf->scissorRect );
		if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals( surfScissor )) {
			backEnd.currentScissor = surfScissor;
			FB_ApplyScissor();
		}
		depthBoundsTest.Update( surfScissor );

		uniforms->modelViewMatrix.Set( surf->space->modelViewMatrix );
		idVec3 localLightOrigin;
		R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, localLightOrigin );
		uniforms->localLightOrigin.Set( idVec4( localLightOrigin, 0.0f ) );

		RB_DrawElementsWithCounters( surf, DCK_SHADOW );
	}
}

#define max(x, y) idMath::Fmax(x, y)
#ifdef __ANDROID__ //karin: using glslprops/ for avoid override
#include "glslprogs/tdm_shadowstencilsoft_shared.glsl"
#else
#include "glprogs/tdm_shadowstencilsoft_shared.glsl"
#endif
#undef max

idScreenRect StencilShadowStage::ExpandScissorRectForSoftShadows( const viewDef_t *viewDef, const idScreenRect &scissor ) const {
	int stencilBufferHeight = frameBuffers->shadowStencilFbo->Height();
	if ( stencilBufferHeight <= 0 )
		return scissor;

	// compute maximum blur radius, measured in texels of stencil texture
	float maxBlurAxisLength = computeMaxBlurAxisLength( stencilBufferHeight, r_softShadowsQuality.GetInteger() );

	// support RenderScale < 1: stencil texture is at render resolution, but scissors are in VidSize
	maxBlurAxisLength += 1.0f;
	maxBlurAxisLength *= ( glConfig.vidHeight / stencilBufferHeight );
	int addedWidth = int(ceil(maxBlurAxisLength));
	assert( addedWidth >= 0 );

	idScreenRect res = scissor;
	res.Expand( addedWidth );
	res.Intersect( viewDef->scissor );	// clamp to viewport
	return res;
}

//=======================================================================================

idCVar r_softShadowsMipmaps(
	"r_softShadowsMipmaps", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE,
	"Use mipmap tiles to avoid sampling far away from penumbra"
);

// if any of these properties change, then we need to recreate mipmaps
struct MipmapsInitProps {
	int renderWidth = -1;
	int renderHeight = -1;
	int softShadowQuality = -1;

	bool operator== (const MipmapsInitProps &p) const {
		return memcmp(this, &p, sizeof(p)) == 0;
	}
};
static MipmapsInitProps currentMipmapProps;

void StencilShadowStage::ShutdownMipmaps() {
	stencilShadowMipmap.Shutdown();
	currentMipmapProps = MipmapsInitProps();
}

void StencilShadowStage::FillStencilShadowMipmaps( const viewDef_t *viewDef, const idScreenRect &lightScissor ) {
	if ( viewDef->IsLightGem() )
		return;

	MipmapsInitProps newProps = {
		frameBuffers->shadowStencilFbo->Width(),
		frameBuffers->shadowStencilFbo->Height(),
		r_softShadowsMipmaps.GetBool() ? r_softShadowsQuality.GetInteger() : 0
	};
	if ( !(newProps == currentMipmapProps) ) {
		// some important property has changed: everything should be recreated
		stencilShadowMipmap.Shutdown();

		float maxBlurAxisLength = computeMaxBlurAxisLength( newProps.renderHeight, r_softShadowsQuality.GetInteger() );
		float lodLevelFloat = log2( idMath::Fmax(maxBlurAxisLength * 2, 1.0f) );
		int lodLevel = int( idMath::Ceil(lodLevelFloat) );
		static const int BASE_LEVEL = 2;
		lodLevel = idMath::Imax(lodLevel, BASE_LEVEL);

		if ( newProps.softShadowQuality > 0 ) {
			stencilShadowMipmap.Init(
				TiledCustomMipmapStage::MM_STENCIL_SHADOW,
#ifdef __ANDROID__ //karin: only GL_RGBA if texture
                GL_RGBA,
#else
                GL_R8,
#endif
				newProps.renderWidth,
				newProps.renderHeight,
				lodLevel, BASE_LEVEL
			);
		}
		currentMipmapProps = newProps;
	}

	if ( newProps.softShadowQuality > 0 ) {
		// TODO #6214: how to round up properly for low renderscale?
		int x = lightScissor.x1 * newProps.renderWidth  / glConfig.vidWidth ;
		int y = lightScissor.y1 * newProps.renderHeight / glConfig.vidHeight;
		int w = lightScissor.GetWidth()  * newProps.renderWidth  / glConfig.vidWidth ;
		int h = lightScissor.GetHeight() * newProps.renderHeight / glConfig.vidHeight;

		stencilShadowMipmap.FillFrom( globalImages->shadowDepthFbo, x, y, w, h );
		// restore current viewport and scissor
		FB_ApplyViewport();
		FB_ApplyScissor();
	}
}
