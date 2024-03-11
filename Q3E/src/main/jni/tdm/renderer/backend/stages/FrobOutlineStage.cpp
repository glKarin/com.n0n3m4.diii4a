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
#include "renderer/backend/stages/FrobOutlineStage.h"

#include "renderer/backend/glsl.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/tr_local.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"

idCVar r_newFrob( "r_newFrob", "0", CVAR_RENDERER | CVAR_ARCHIVE,
	"Controls how objects are frob-highlighted:\n"
	"  0 = use material stages by parm11\n"
	"  1 = use the frob shader\n"
	"  2 = use nothing (no highlight)\n"
	"Note: outline is controlled by r_frobOutline"
);

idCVar r_frobIgnoreDepth( "r_frobIgnoreDepth", "0", CVAR_BOOL|CVAR_RENDERER|CVAR_ARCHIVE, "Ignore depth when drawing frob outline" );
idCVar r_frobDepthOffset( "r_frobDepthOffset", "0.0005", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "Extra depth offset for frob outline" );
idCVar r_frobOutline( "r_frobOutline", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Work-in-progress outline around highlighted objects: 1 = image-based, 2 = geometric" );
idCVar r_frobOutlineColorR( "r_frobOutlineColorR", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - red component" );
idCVar r_frobOutlineColorG( "r_frobOutlineColorG", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - green component" );
idCVar r_frobOutlineColorB( "r_frobOutlineColorB", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - blue component" );
idCVar r_frobOutlineColorA( "r_frobOutlineColorA", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Color of the frob outline - alpha component" );
idCVar r_frobOutlineExtrusion( "r_frobOutlineExtrusion", "-3.0", CVAR_FLOAT | CVAR_RENDERER | CVAR_ARCHIVE, "Thickness of geometric outline in pixels (negative = hard, positive = soft)" );
idCVar r_frobHighlightColorMulR( "r_frobHighlightColorMulR", "0.3", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Diffuse color of the frob highlight - red component" );
idCVar r_frobHighlightColorMulG( "r_frobHighlightColorMulG", "0.3", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Diffuse color of the frob highlight - green component" );
idCVar r_frobHighlightColorMulB( "r_frobHighlightColorMulB", "0.3", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Diffuse color of the frob highlight - blue component" );
idCVar r_frobHighlightColorAddR( "r_frobHighlightColorAddR", "0.02", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Added color of the frob highlight - red component" );
idCVar r_frobHighlightColorAddG( "r_frobHighlightColorAddG", "0.02", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Added color of the frob highlight - green component" );
idCVar r_frobHighlightColorAddB( "r_frobHighlightColorAddB", "0.02", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE , "Added color of the frob highlight - blue component" );
idCVar r_frobOutlineBlurPasses( "r_frobOutlineBlurPasses", "2", CVAR_RENDERER|CVAR_FLOAT|CVAR_ARCHIVE, "Thickness of the new frob outline" );

namespace {
	struct FrobOutlineUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( FrobOutlineUniforms )

		DEFINE_UNIFORM( vec2, extrusion )
		DEFINE_UNIFORM( int, hard )
		DEFINE_UNIFORM( float, depth )
		DEFINE_UNIFORM( vec4, color )
		DEFINE_UNIFORM( vec4, colorAdd )
		DEFINE_UNIFORM( vec4, texMatrix )
		DEFINE_UNIFORM( sampler, diffuse )
		DEFINE_UNIFORM( float, alphaTest )
	};

	struct BlurUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( BlurUniforms )

		DEFINE_UNIFORM( sampler, source )
		DEFINE_UNIFORM( vec2, axis )
	};

	struct ApplyUniforms : GLSLUniformGroup {
		UNIFORM_GROUP_DEF( ApplyUniforms )

		DEFINE_UNIFORM( sampler, source )
		DEFINE_UNIFORM( vec4, color )
	};
}

static void FrobOutlinePreset( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		common->Printf( "Pass number of preset to use:\n" );
		common->Printf( "  1 - geometric hard\n" );
		common->Printf( "  2 - geometric soft\n" );
		common->Printf( "  3 - image-based\n" );
		common->Printf( "  4 - image-based weak (Kingsal)\n" );
		common->Printf( "  5 - geometric hard black (Nbohr1more)\n" );
		common->Printf( "  6 - image-based black (Nbohr1more)\n" );
		return;
	}
	int preset = atoi( args.Argv( 1 ) );
	r_frobOutlineColorR.SetFloat( 1.0f );
	r_frobOutlineColorG.SetFloat( 1.0f );
	r_frobOutlineColorB.SetFloat( 1.0f );
	if ( preset == 1 ) {
		//geometric hard
		r_frobOutline.SetInteger( 2 );
		r_frobOutlineColorA.SetFloat( 1.0f );
		r_frobOutlineExtrusion.SetFloat( -3.0f );
	}
	else if ( preset == 2 ) {
		//geometric soft
		r_frobOutline.SetInteger( 2 );
		r_frobOutlineColorA.SetFloat( 0.5f );
		r_frobOutlineExtrusion.SetFloat( 10.0f );
	}
	else if ( preset == 3 ) {
		//image-based depth-aware
		r_frobOutline.SetInteger( 1 );
		r_frobOutlineColorA.SetFloat( 1.2f );
		r_frobIgnoreDepth.SetBool( false );
		r_frobOutlineBlurPasses.SetInteger( 2 );
	}
	else if ( preset == 4 ) {
		//image-based depth-aware (tweaked by Kingsal)
		r_frobOutline.SetInteger( 1 );
		r_frobOutlineColorA.SetFloat( 0.7f );
		r_frobIgnoreDepth.SetBool( false );
		r_frobOutlineBlurPasses.SetInteger( 1 );
	}
	else if ( preset == 5 ) {
		//geometric hard black (tweaked by Nbohr1more)
		r_frobOutline.SetInteger( 2 );
		r_frobOutlineColorA.SetFloat( 1.0f );
		r_frobOutlineExtrusion.SetFloat( -8.0f );
		r_frobOutlineColorR.SetFloat( 0.0f );
		r_frobOutlineColorG.SetFloat( 0.0f );
		r_frobOutlineColorB.SetFloat( 0.0f );
	}
	else if ( preset == 6 ) {
		//image-based depth-aware black (tweaked by Nbohr1more)
		r_frobOutline.SetInteger( 1 );
		r_frobOutlineColorA.SetFloat( 2.5f );
		r_frobIgnoreDepth.SetBool( false );
		r_frobOutlineBlurPasses.SetInteger( 2 );
		r_frobOutlineColorR.SetFloat( 0.0f );
		r_frobOutlineColorG.SetFloat( 0.0f );
		r_frobOutlineColorB.SetFloat( 0.0f );
	}
	else {
		common->Printf( "Unknown preset number\n" );
	}
}

void FrobOutlineStage::Init() {
	silhouetteShader = programManager->LoadFromFiles( "frob_silhouette", "stages/frob/frob.vert.glsl", "stages/frob/frob_flat.frag.glsl" );
	highlightShader = programManager->LoadFromFiles( "frob_highlight", "stages/frob/frob.vert.glsl", "stages/frob/frob_highlight.frag.glsl" );
    extrudeShader = programManager->LoadFromFiles( "frob_extrude", "stages/frob/frob.vert.glsl", "stages/frob/frob_modalpha.frag.glsl", "stages/frob/frob_extrude.geom.glsl" );
	applyShader = programManager->LoadFromFiles( "frob_apply", "fullscreen_tri.vert.glsl", "stages/frob/frob_apply.frag.glsl" );
	colorTex[0] = globalImages->ImageScratch( "frob_color_0" );
	colorTex[1] = globalImages->ImageScratch( "frob_color_1" );
	depthTex = globalImages->ImageScratch( "frob_depth" );
	fbo[0] = frameBuffers->CreateFromGenerator( "frob_0", [this](FrameBuffer *) { this->CreateFbo( 0 ); } );
	fbo[1] = frameBuffers->CreateFromGenerator( "frob_1", [this](FrameBuffer *) { this->CreateFbo( 1 ); } );
	drawFbo = frameBuffers->CreateFromGenerator( "frob_draw", [this](FrameBuffer *) { this->CreateDrawFbo(); } );

	cmdSystem->AddCommand(
		"r_frobOutlinePreset", FrobOutlinePreset,
		CMD_FL_RENDERER, "Change frob outline cvars according to specified preset",
		idCmdSystem::ArgCompletion_Integer<1, 3>
	);
}

void FrobOutlineStage::Shutdown() {}

void FrobOutlineStage::CreateFbo( int idx ) {
	int width = frameBuffers->renderWidth / 4;
	int height = frameBuffers->renderHeight / 4;
#ifdef __ANDROID__ //karin: only GL_RGBA if texture
	colorTex[idx]->GenerateAttachment( width, height, GL_RGBA );
#else
    colorTex[idx]->GenerateAttachment( width, height, GL_R8 );
#endif
	fbo[idx]->Init( width, height );
	fbo[idx]->AddColorRenderTexture( 0, colorTex[idx] );
}

void FrobOutlineStage::CreateDrawFbo() {
	int width = frameBuffers->renderWidth / 4;
	int height = frameBuffers->renderHeight / 4;
	drawFbo->Init( width, height, 4 );
#ifdef __ANDROID__ //karin: only GL_RGBA8 if render buffer
	drawFbo->AddColorRenderBuffer( 0, GL_RGBA8 );
#else
    drawFbo->AddColorRenderBuffer( 0, GL_R8 );
#endif
}

void FrobOutlineStage::DrawFrobOutline( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	// find any surfaces that should be frob-highlighted
	idList<drawSurf_t *> frobSurfs;
	for ( int i = 0; i < numDrawSurfs; ++i ) {
		drawSurf_t *surf = drawSurfs[i];

		if ( !surf->shaderRegisters[EXP_REG_PARM11] )
			continue;
		if ( !surf->material->HasAmbient() || !surf->numIndexes || !surf->ambientCache.IsValid() || !surf->space
			//stgatilov: some objects are fully transparent
			//I want to at least draw outline around them!
			/* || surf->material->GetSort() >= SS_PORTAL_SKY*/
		) {
			continue;
		}

		frobSurfs.AddGrow( surf );
	}

	if ( frobSurfs.Num() > 0 ) {
		TRACE_GL_SCOPE( "DrawFrobOutline" )

		GL_ScissorRelative( 0, 0, 1, 1 );

		if ( r_frobOutline.GetInteger() == 1 ) {
			if ( r_frobIgnoreDepth.GetBool() ) {
				DrawFrobImageBasedIgnoreDepth( frobSurfs );
			}
			else {
				DrawFrobImageBased( frobSurfs );
			}
		}
		else if ( r_frobOutline.GetInteger() == 2 ) {
			DrawFrobGeometric( frobSurfs );
		}
		else {
			DrawSurfaces( frobSurfs, r_newFrob.GetBool(), true );
		}
	}

	GL_State( GLS_DEPTHFUNC_EQUAL );
	qglStencilFunc( GL_ALWAYS, 255, 255 );
	qglStencilMask( 255 );
	GL_SelectTexture( 0 );
}

void FrobOutlineStage::DrawFrobImageBasedIgnoreDepth( idList<drawSurf_t*> &surfs ) {
	qglClearStencil( 255 );
	qglClear( GL_STENCIL_BUFFER_BIT );

	// highlight and mark surfaces in stencil buffer
	qglStencilFunc( GL_ALWAYS, 0, 0 );
	qglStencilOp( GL_KEEP, GL_REPLACE, GL_REPLACE );
	DrawSurfaces( surfs, r_newFrob.GetInteger() == 1, true );

	// create outline image and blend it where there were no surfaces
	DrawImageBasedOutline( surfs, 255 );
}

void FrobOutlineStage::DrawFrobImageBased( idList<drawSurf_t*> &surfs ) {
	qglClearStencil( 0 );
	qglClear( GL_STENCIL_BUFFER_BIT );

	// highlight and mark surfaces in stencil buffer
	qglStencilFunc( GL_ALWAYS, 128, 0 );	// 128 - highlighted object
	qglStencilOp( GL_KEEP, GL_REPLACE, GL_REPLACE );
	DrawSurfaces( surfs, r_newFrob.GetInteger() == 1, true );

	// consider pixels in surfaces triangles which failed alpha test
	// mark depth-passing ones as "we allow rendering outline here"
	qglStencilFunc( GL_NOTEQUAL, 255, 128 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
	qglStencilMask( 64 );					// 64 - rendering is allowed
	DrawSurfaces( surfs, false, false );

	// consider pixels around object edges
	// mark depth-passing ones as "we allow rendering outline here"
	qglStencilFunc( GL_NOTEQUAL, 255, 128 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
	qglStencilMask( 64 );
	MarkOutline( surfs );

	// create outline image and blend it where allowed
	DrawImageBasedOutline( surfs, 64 );
}

void FrobOutlineStage::DrawFrobGeometric( idList<drawSurf_t*> &surfs ) {
	qglClearStencil( 255 );
	qglClear( GL_STENCIL_BUFFER_BIT );

	// highlight and mark surfaces in stencil buffer
	// note: mark surfaces triangle completely, regardless of alpha test
	qglStencilFunc( GL_ALWAYS, 0, 0 );
	qglStencilOp( GL_KEEP, GL_REPLACE, GL_REPLACE );
	DrawSurfaces( surfs, r_newFrob.GetInteger() == 1, false );

	// draw extruded object edges where there were no surfaces
	qglStencilFunc( GL_EQUAL, 255, 255 );
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	DrawGeometricOutline( surfs );
}

void FrobOutlineStage::DrawSurfaces( idList<drawSurf_t*> &surfs, bool highlight, bool enableAlphaTest ) {
	int state = GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE;
	if ( !highlight )
		state |= GLS_COLORMASK;
	GL_State( state );

	highlightShader->Activate();
	FrobOutlineUniforms *frobUniforms = highlightShader->GetUniformGroup<FrobOutlineUniforms>();
	frobUniforms->extrusion.Set( 0.f, 0.f );
	frobUniforms->depth.Set( 0.f );
	frobUniforms->color.Set( r_frobHighlightColorMulR.GetFloat(), r_frobHighlightColorMulG.GetFloat(), r_frobHighlightColorMulB.GetFloat(), 1 );
	frobUniforms->colorAdd.Set( r_frobHighlightColorAddR.GetFloat(), r_frobHighlightColorAddG.GetFloat(), r_frobHighlightColorAddB.GetFloat(), 1 );

	DrawElements( surfs, highlightShader, enableAlphaTest );
}

void FrobOutlineStage::MarkOutline( idList<drawSurf_t *> &surfs ) {
	GL_State( GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK | GLS_COLORMASK );

	extrudeShader->Activate();
	auto *uniforms = extrudeShader->GetUniformGroup<FrobOutlineUniforms>();
	//according to ssao_blur.frag.glsl source,
	//information can travel by SCALE * R = 8 pixels along each direction in one pass
	static const float BlurRadiusInPixels = 12.0f;	//a bit greater than 8 * sqrt(2)
	idVec2 extr = idVec2(
		1.0f / idMath::Fmax( drawFbo->Width(), 4.0f ),
		1.0f / idMath::Fmax( drawFbo->Height(), 3.0f )
	) * r_frobOutlineBlurPasses.GetFloat() * BlurRadiusInPixels;
	uniforms->extrusion.Set( extr );
	uniforms->hard.Set( 1 );
	uniforms->depth.Set( r_frobDepthOffset.GetFloat() );

	DrawElements( surfs, extrudeShader, false );
}

void FrobOutlineStage::DrawGeometricOutline( idList<drawSurf_t*> &surfs ) {
	// outline 1) can be occluded by objects in the front, 2) is translucent
	GL_State( GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	// enable geometry shader which produces extruded geometry
	extrudeShader->Activate();
	auto *uniforms = extrudeShader->GetUniformGroup<FrobOutlineUniforms>();
	idVec2 extr = idVec2(
		1.0f / idMath::Fmax( frameBuffers->defaultFbo->Width(), 800.0f ),
		1.0f / idMath::Fmax( frameBuffers->defaultFbo->Height(), 600.0f )
	) * idMath::Fabs( r_frobOutlineExtrusion.GetFloat() );
	uniforms->extrusion.Set( extr );
	uniforms->hard.Set( r_frobOutlineExtrusion.GetFloat() < 0.0f ? 1 : 0 );
	uniforms->depth.Set( r_frobDepthOffset.GetFloat() );
	uniforms->color.Set( r_frobOutlineColorR.GetFloat(), r_frobOutlineColorG.GetFloat(), r_frobOutlineColorB.GetFloat(), r_frobOutlineColorA.GetFloat() );

	DrawElements( surfs, extrudeShader, true );
}

void FrobOutlineStage::DrawImageBasedOutline( idList<drawSurf_t *> &surfs, int stencilMask ) {
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	// draw to small anti-aliased color buffer
	FrameBuffer *previousFbo = frameBuffers->activeFbo;
	drawFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );
	qglClearColor( 0, 0, 0, 0 );
	qglClear( GL_COLOR_BUFFER_BIT );
	// draw surfaces with flat white color
	silhouetteShader->Activate();
	auto *silhouetteUniforms = silhouetteShader->GetUniformGroup<FrobOutlineUniforms>();
	silhouetteUniforms->color.Set( 1, 1, 1, 1 );
	DrawElements( surfs, silhouetteShader, true );

	// resolve color buffer
	drawFbo->BlitTo( fbo[0], GL_COLOR_BUFFER_BIT );
	// apply blur
	for ( int i = 0; i < r_frobOutlineBlurPasses.GetFloat(); ++i )
		ApplyBlur();

	// return back to original framebuffer
	previousFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );
	// blend blurred image to pixels where specified stencil bit is set
	qglStencilFunc( GL_EQUAL, stencilMask, stencilMask );
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	GL_Cull( CT_FRONT_SIDED );
	// set up shader for blending the image
	applyShader->Activate();
	ApplyUniforms *applyUniforms = applyShader->GetUniformGroup<ApplyUniforms>();
	applyUniforms->source.Set( 0 );
	GL_SelectTexture( 0 );
	colorTex[0]->Bind();
	applyUniforms->color.Set( r_frobOutlineColorR.GetFloat(), r_frobOutlineColorG.GetFloat(), r_frobOutlineColorB.GetFloat(), r_frobOutlineColorA.GetFloat() );
	// perform blend
	RB_DrawFullScreenTri();
}

extern void R_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs,
                           idImage **image, idVec4 matrix[2], float color[4] );

void FrobOutlineStage::DrawElements( idList<drawSurf_t *> &surfs, GLSLProgram  *shader, bool enableAlphaTest ) {
	GL_SelectTexture( 1 );
	shader->GetUniformGroup<FrobOutlineUniforms>()->diffuse.Set( 1 );

	for ( drawSurf_t *surf : surfs ) {
		GL_Cull( surf->material->GetCullType() );
		shader->GetUniformGroup<Uniforms::Global>()->Set( surf->space );
		vertexCache.VertexPosition( surf->ambientCache );

		//stgatilov: some transparent objects have no diffuse map
		//then using white results in very strong surface highlighting
		//better stay conservative and don't highlight them (almost)
		idImage *diffuse = globalImages->blackImage;

		const idMaterial *material = surf->material;
		for ( int i = 0; i < material->GetNumStages(); ++i ) {
			const shaderStage_t *stage = material->GetStage( i );
			if ( stage->lighting == SL_DIFFUSE && stage->texture.image ) {
				idVec4 textureMatrix[2];
				R_SetDrawInteraction( stage, surf->shaderRegisters, &diffuse, textureMatrix, nullptr );
				auto *uniforms = shader->GetUniformGroup<FrobOutlineUniforms>();
				uniforms->texMatrix.SetArray( 2, textureMatrix[0].ToFloatPtr() );
				uniforms->alphaTest.Set( stage->hasAlphaTest && enableAlphaTest ? surf->shaderRegisters[stage->alphaTestRegister] : -1.0f );
				break;
			}
		}
		diffuse->Bind();

		RB_DrawElementsWithCounters( surf );
	}
}

void FrobOutlineStage::ApplyBlur() {
	programManager->gaussianBlurShader->Activate();
	BlurUniforms *uniforms = programManager->gaussianBlurShader->GetUniformGroup<BlurUniforms>();
	uniforms->source.Set( 0 );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_SelectTexture( 0 );
	colorTex[0]->Bind();
	uniforms->axis.Set( 1, 0 );
	fbo[1]->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();

	uniforms->axis.Set( 0, 1 );
	fbo[0]->Bind();
	colorTex[1]->Bind();
	qglClear(GL_COLOR_BUFFER_BIT);
	RB_DrawFullScreenTri();
}
