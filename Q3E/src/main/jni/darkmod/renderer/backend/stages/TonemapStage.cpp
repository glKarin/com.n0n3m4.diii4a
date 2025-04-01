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

idCVar r_postprocess_exposure(
	"r_postprocess_exposure", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Multiplies color by coefficient before range compression.\n",
	1e-3f, 1e+3f
);

idCVar r_postprocess_overbright_desaturation(
	"r_postprocess_overbright_desaturation", "0.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"How strong is the desaturation of overbright colors (0 = don't desaturate).\n",
	0.0f, 1.0f
);

idCVar r_postprocess_compress(
	"r_postprocess_compress", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL,
	"Perform range compression to map overbright colors into [0..1] output range.\n"
	"This basically turns HDR into LDR, although in our case the input is not even linear..."
);
idCVar r_postprocess_compress_switch_point(
	"r_postprocess_compress_switch_point", "1.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Range compression curve: two parts of the curve smoothly join at X = This.\n"
	"Initial part exists to make normal colors look as before, while tail part exists to squeeze overbright colors.",
	0.01f, 100.0f
);
idCVar r_postprocess_compress_switch_multiplier(
	"r_postprocess_compress_switch_multiplier", "0.7", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Range compression curve: ratio Y/X = This at X = SwitchPoint.\n"
	"Lower value sacrifices brightness of normal colors for more contrast in overbright colors.",
	0.001f, 0.999f
);
idCVar r_postprocess_compress_initial_slope(
	"r_postprocess_compress_initial_slope", "1.2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Range compression curve: curve derivative at X = 0.\n"
	"Values > 1 make dark colors more contrast.",
	0.01f, 100.0f
);
idCVar r_postprocess_compress_tail_power(
	"r_postprocess_compress_tail_power", "2.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Range compression curve: power of Reinhard-like tail function.\n"
	"Higher value makes overbright colors get close to 1 (full bright) faster.",
	0.1f, 10.0f
);

// postprocess related - J.C.Denton
idCVar r_postprocess_gamma(
	"r_postprocess_gamma", "1.2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Applies inverse power function in postprocessing.\n"
	"Higher value => brighter color.",
	0.1f, 3.0f
);
idCVar r_postprocess_brightness(
	"r_postprocess_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Multiplies final color by coefficient.\n"
	"Using this is strongly discouraged: it causes color clamping when u > 1 or does not fully utilize monitor range when u < 1.",
	0.5f, 2.0f
);
idCVar r_postprocess_desaturation(
	"r_postprocess_desaturation", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT,
	"Desaturates the scene if positive.\n"
	"Oversaturates the scene if negative (high values can cause color clamping).",
	-5.0f, 5.0f
);

idCVar r_postprocess_sharpen(
	"r_postprocess_sharpen", "1", CVAR_RENDERER|CVAR_BOOL|CVAR_ARCHIVE,
	"Use contrast-adaptive sharpening in tonemapping"
);
idCVar r_postprocess_sharpness(
	"r_postprocess_sharpness", "0.5", CVAR_RENDERER|CVAR_FLOAT|CVAR_ARCHIVE,
	"Sharpening amount",
	0.0f, 1.0f
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

	DEFINE_UNIFORM(float, exposure)

	DEFINE_UNIFORM(float, overbrightDesaturation)

	DEFINE_UNIFORM(int, compressEnable)
	DEFINE_UNIFORM(float, compressSwitchPoint)
	DEFINE_UNIFORM(float, compressSwitchMultiplier)
	DEFINE_UNIFORM(float, compressInitialSlope)
	DEFINE_UNIFORM(float, compressTailMultiplier)
	DEFINE_UNIFORM(float, compressTailShift)
	DEFINE_UNIFORM(float, compressTailPower)

	DEFINE_UNIFORM(float, gamma)
	DEFINE_UNIFORM(float, brightness)
	DEFINE_UNIFORM(float, desaturation)

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
	if ( !r_tonemapInternal.GetBool() ) {
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

	uniforms->exposure.Set( r_postprocess_exposure.GetFloat() );

	uniforms->overbrightDesaturation.Set( r_postprocess_overbright_desaturation.GetFloat() );

	uniforms->compressEnable.Set( r_postprocess_compress.GetBool() );
	if ( r_postprocess_compress.GetBool() ) {
		float SwitchPoint = r_postprocess_compress_switch_point.GetFloat();
		float SwitchMultiplier = r_postprocess_compress_switch_multiplier.GetFloat();
		float InitialSlope = r_postprocess_compress_initial_slope.GetFloat();
		float TailPower = r_postprocess_compress_tail_power.GetFloat();

		// value and derivative of curve at switch point
		float SwitchValue = SwitchMultiplier * SwitchPoint;
		float SwitchSlope = SwitchMultiplier * ( 1.0f + idMath::Log( SwitchMultiplier / InitialSlope ) );
		// select unknown parameters of tail curve to ensure C1-continuity at switch point
		float TailShift = ( 1.0f - SwitchValue ) / SwitchSlope * TailPower;
		float TailMultiplier = idMath::Pow( 1.0f - SwitchValue, 1.0f / TailPower ) * TailShift;

		uniforms->compressSwitchPoint.Set( SwitchPoint );
		uniforms->compressSwitchMultiplier.Set( SwitchMultiplier );
		uniforms->compressInitialSlope.Set( InitialSlope );
		uniforms->compressTailMultiplier.Set( TailMultiplier );
		uniforms->compressTailShift.Set( TailShift );
		uniforms->compressTailPower.Set( TailPower );
	}

	uniforms->gamma.Set( r_postprocess_gamma.GetFloat() );
	uniforms->brightness.Set( r_postprocess_brightness.GetFloat() );
	uniforms->desaturation.Set( r_postprocess_desaturation.GetFloat() );

	uniforms->sharpen.Set( r_postprocess_sharpen.GetBool() );
	uniforms->sharpness.Set( r_postprocess_sharpness.GetFloat() );

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
