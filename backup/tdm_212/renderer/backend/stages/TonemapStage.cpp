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
#include "renderer/backend/stages/TonemapStage.h"

#include "renderer/backend/GLSLUniforms.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/tr_local.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/FrameBufferManager.h"


// postprocess related - J.C.Denton
idCVar r_postprocess_gamma(
	"r_postprocess_gamma", "1.2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Applies inverse power function in postprocessing",
	0.1f, 3.0f
);
idCVar r_postprocess_brightness(
	"r_postprocess_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Multiplies color by coefficient",
	0.5f, 2.0f
);
idCVar r_postprocess_colorCurveBias(
	"r_postprocess_colorCurveBias", "0.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Applies Exponential Color Curve to final pass (range 0 to 1):\n"
	"1 = color curve fully applied\n"
	"0 = No color curve"
);
idCVar r_postprocess_colorCorrection(
	"r_postprocess_colorCorrection", "5", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Applies an exponential color correction function to final scene"
);
idCVar r_postprocess_colorCorrectBias(
	"r_postprocess_colorCorrectBias", "0.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Applies an exponential color correction function to final scene with this bias.\n"
	"E.g. value ranges between 0-1. A blend is performed between scene render and color corrected image based on this value"
);
idCVar r_postprocess_desaturation(
	"r_postprocess_desaturation", "0.00", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Desaturates the scene"
);


idCVar r_postprocess_sharpen(
	"r_postprocess_sharpen", "1", CVAR_RENDERER|CVAR_BOOL|CVAR_ARCHIVE,
	"Use contrast-adaptive sharpening in tonemapping"
);
idCVar r_postprocess_sharpness(
	"r_postprocess_sharpness", "0.5", CVAR_RENDERER|CVAR_FLOAT|CVAR_ARCHIVE,
	"Sharpening amount"
);

idCVar r_postprocess_dither( "r_postprocess_dither", "1",
	CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE,
	"Should blue noise dithering be used in tonemapping to reduce color banding issues?\n"
	"Note that dithering is force-disabled for low color precision, r_fboColorBits = 64 is needed."
);
idCVar r_postprocess_dither_input( "r_postprocess_dither_input", "2",
	CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE,
	"Dithering up to X/2 is added to integer color value BEFORE tonemapping"
);
idCVar r_postprocess_dither_output( "r_postprocess_dither_output", "2",
	CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE,
	"Dithering up to X/2 is added to integer color value AFTER tonemapping"
);


struct TonemapStage::Uniforms : GLSLUniformGroup {
	UNIFORM_GROUP_DEF( Uniforms )
	DEFINE_UNIFORM(sampler, texture)
	DEFINE_UNIFORM(float, gamma)
	DEFINE_UNIFORM(float, brightness)
	DEFINE_UNIFORM(float, desaturation)
	DEFINE_UNIFORM(float, colorCurveBias)
	DEFINE_UNIFORM(float, colorCorrection)
	DEFINE_UNIFORM(float, colorCorrectBias)
	DEFINE_UNIFORM(int, sharpen)
	DEFINE_UNIFORM(float, sharpness)
	DEFINE_UNIFORM(sampler, noiseImage)
	DEFINE_UNIFORM(float, ditherInput)
	DEFINE_UNIFORM(float, ditherOutput)
};

void TonemapStage::Init() {
	tonemapShader = programManager->LoadFromFiles( "tonemap", "fullscreen_tri.vert.glsl", "stages/tonemap/tonemap.frag.glsl" );
}

void TonemapStage::Shutdown() {}

void TonemapStage::ApplyTonemap( FrameBuffer *destinationFbo, idImage *sourceTexture ) {
	if ( !r_tonemap ) {
		return;
	}
	TRACE_GL_SCOPE("Tonemap")

	destinationFbo->Bind();
	GL_ViewportRelative( 0, 0, 1, 1 );
	GL_ScissorRelative( 0, 0, 1, 1 );

	GL_State( GLS_DEPTHMASK );
	qglDisable( GL_DEPTH_TEST );

	tonemapShader->Activate();
	Uniforms *uniforms = tonemapShader->GetUniformGroup<Uniforms>();
	uniforms->gamma.Set( idMath::ClampFloat( 1e-3f, 1e+3f, r_postprocess_gamma.GetFloat() ) );
	uniforms->brightness.Set( r_postprocess_brightness.GetFloat() );
	uniforms->desaturation.Set(idMath::ClampFloat( -1.0f, 1.0f, r_postprocess_desaturation.GetFloat() ) );
	uniforms->colorCurveBias.Set(r_postprocess_colorCurveBias.GetFloat() );
	uniforms->colorCorrection.Set(r_postprocess_colorCorrection.GetFloat() );
	uniforms->colorCorrectBias.Set(idMath::ClampFloat( 0.0f, 1.0f, r_postprocess_colorCorrectBias.GetFloat() ) );
	uniforms->sharpen.Set( r_postprocess_sharpen.GetBool() );
	uniforms->sharpness.Set( idMath::ClampFloat( 0.0f, 1.0f, r_postprocess_sharpness.GetFloat() ) );

	// note: dithering helps only when internal color precision is greater than output precision
	// otherwise, we lose necessary information before tonemapping, and dithering does not help at all
	if ( r_postprocess_dither.GetBool() && r_fboColorBits.GetInteger() > 32 ) {
		// r_fboColorBits = 64 means half-floats, which have 10 bits of precision
		uniforms->ditherInput.Set( r_postprocess_dither_input.GetFloat() / (1 << 10) );
		uniforms->ditherOutput.Set( r_postprocess_dither_output.GetFloat() / (1 << 8) );
	}
	else {
		uniforms->ditherInput.Set( 0.0f );
		uniforms->ditherOutput.Set( 0.0f );
	}

	GL_SelectTexture( 0 );
	sourceTexture->Bind();
	uniforms->texture.Set( 0 );

	GL_SelectTexture( 1 );
	globalImages->blueNoise1024rgbaImage->Bind();
	uniforms->noiseImage.Set( 1 );

	RB_DrawFullScreenTri();
}
