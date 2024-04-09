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
#include "renderer/backend/qgl/qgl.h"
#include "renderer/tr_local.h"

#if defined(_WIN32)
#include "sys/win32/win_local.h"
#elif defined(__ANDROID__) //karin: Android sys
#include "sys/android/local.h"
#else
#include "sys/linux/local.h"
#endif


static idCVar r_glBlacklistExtensions("r_glBlacklistExtensions", "", CVAR_ARCHIVE | CVAR_RENDERER, "Set space-separated list of OpenGL extension to force-disable on game start");
static void GLimp_LoadExtensionsBlacklist() {
	static idStr text;
	static idList<const char*> ptrs;
	//parse extensions blacklist
	text = r_glBlacklistExtensions.GetString();
	ptrs.ClearFree();
	for (int i = 0; i < text.Length(); i++)
		if (text[i] == ' ')
			text[i] = 0;
	for (int i = 0; i < text.Length(); i++) if (text[i]) {
		ptrs.Append(&text[i]);
		i += idStr::Length(&text[i]);
	}
	ptrs.Append(nullptr);
	//set pointer to our arrays
	GLAD_GL_blacklisted_extensions = ptrs.Ptr();
}

void GLimp_LoadFunctions(bool inContext) {
	GLimp_LoadExtensionsBlacklist();
	bool GLok = gladLoadGL();
	if (inContext && !GLok) {
		common->Error("Failed to initialize OpenGL functions (glad)");
	}
#if defined(_WIN32)
	bool WGLok = gladLoadWGL(win32.hDC);
	if (inContext && !WGLok) {
		common->Error("Failed to initialize WGL functions (glad)");
	}
#endif
}

#define CHECK_FEATURE(name) GLimp_CheckExtension(#name, GLAD_##name)
bool GLimp_CheckExtension( const char *name, int available ) {
	if ( !available ) {
		common->Printf("^1X^0 - %s not found\n", name);
		return false;
	}
	else {
		common->Printf("^2v^0 - using %s\n", name);
		return true;
	}
}

void GLimp_CheckRequiredFeatures( void ) {
	common->Printf( "Checking required OpenGL features...\n" );
	bool reqs = true;
#if !defined(__ANDROID__) //karin: GLES3.2
	reqs = reqs && CHECK_FEATURE(GL_VERSION_3_3);
	reqs = reqs && CHECK_FEATURE(GL_EXT_texture_compression_s3tc);
#endif
#if defined(_WIN32)
	reqs = reqs && CHECK_FEATURE(WGL_VERSION_1_0);
	//reqs = reqs && CHECK_FEATURE(WGL_ARB_create_context);
	//reqs = reqs && CHECK_FEATURE(WGL_ARB_create_context_profile);
	reqs = reqs && CHECK_FEATURE(WGL_ARB_pixel_format);
#elif defined(__ANDROID__)
#elif defined(__linux__)
	//reqs = reqs && CHECK_FEATURE(GLX_VERSION_1_4);
	//reqs = reqs && CHECK_FEATURE(GLX_ARB_create_context);
	//reqs = reqs && CHECK_FEATURE(GLX_ARB_create_context_profile);
#endif
	if (!reqs) {
		common->Error("OpenGL minimum requirements not satisfied");
	}


	common->Printf( "Checking optional OpenGL extensions...\n" );

	glConfig.anisotropicAvailable = CHECK_FEATURE(GL_EXT_texture_filter_anisotropic);
	if ( glConfig.anisotropicAvailable ) {
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "    maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	} else {
		glConfig.maxTextureAnisotropy = 1;
	}

	glConfig.stencilTexturing = CHECK_FEATURE( GL_ARB_stencil_texturing );
	glConfig.depthBoundsTestAvailable = CHECK_FEATURE(GL_EXT_depth_bounds_test);
	glConfig.bufferStorageAvailable = CHECK_FEATURE( GL_ARB_buffer_storage );

	//it seems that these extensions are checked via GLAD_GL_xxx variables
	CHECK_FEATURE(GL_ARB_texture_storage);
	CHECK_FEATURE(GL_ARB_multi_draw_indirect);
	CHECK_FEATURE(GL_ARB_vertex_attrib_binding);
	CHECK_FEATURE(GL_ARB_compatibility);
	CHECK_FEATURE(GL_KHR_debug);

#ifdef _WIN32
	CHECK_FEATURE(WGL_EXT_swap_control);
#endif

	qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, &glConfig.maxTextureUnits );
	common->Printf( "Max active texture units in fragment shader: %d\n", glConfig.maxTextureUnits );
	qglGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glConfig.maxTextures );
	common->Printf( "Max combined texture units: %d\n", glConfig.maxTextures );
	if ( glConfig.maxTextures < MAX_MULTITEXTURE_UNITS ) {
		common->Error( "   Too few!\n" );
	}
	qglGetIntegerv( GL_MAX_SAMPLES, &glConfig.maxSamples );
	common->Printf( "Max anti-aliasing samples: %d\n", glConfig.maxSamples );

	int n;
	qglGetIntegerv( GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB, &n );
	common->Printf( "Max geometry output vertices: %d\n", n );
	qglGetIntegerv( GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB, &n );
	common->Printf( "Max geometry output components: %d\n", n );
	qglGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &n );
	common->Printf( "Max vertex attribs: %d\n", n );
}


#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __attribute__ ((visibility ("default")))
#endif
//#4953 http://forums.thedarkmod.com/topic/19979-choose-gpu/
//hint driver to use discrete GPU on a laptop having both integrated and discrete graphics
extern "C" {
	DLLEXPORT int NvOptimusEnablement = 0x00000001;
	DLLEXPORT int AmdPowerXpressRequestHighPerformance = 0x00000001;
}
