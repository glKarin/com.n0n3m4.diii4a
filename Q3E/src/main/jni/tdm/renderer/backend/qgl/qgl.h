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
#ifndef _QGL_H_
#define _QGL_H_

// All GL/WGL/GLX features available at compile time must be enumerated here.
// Versions:
#define GL_VERSION_1_0 1
#define GL_VERSION_1_1 1
#define GL_VERSION_1_2 1
#define GL_VERSION_1_3 1
#define GL_VERSION_1_4 1
#define GL_VERSION_1_5 1
#define GL_VERSION_2_0 1
#define GL_VERSION_2_1 1
#define GL_VERSION_3_0 1
#define GL_VERSION_3_1 1
#define GL_VERSION_3_2 1
#define GL_VERSION_3_3 1
// Mandatory extensions:
#define GL_EXT_texture_compression_s3tc			1
// Optional extensions:
#define GL_EXT_texture_filter_anisotropic		1	//core since 4.6
#define GL_EXT_depth_bounds_test				1
#define GL_ARB_compatibility					1	//check context profile, ??3.1 only??
#define GL_KHR_debug							1	//core since 4.3
#define GL_ARB_stencil_texturing				1	//core since 4.3
#define GL_ARB_buffer_storage					1	//core since 4.4
#define GL_ARB_texture_storage					1	//core since 4.2
#define GL_ARB_multi_draw_indirect				1	//core since 4.3
#define GL_ARB_vertex_attrib_binding			1	//core since 4.3
#ifdef __ANDROID__ //karin: GLES3.2
#define GL_ES_VERSION_2_0 1
#define GL_ES_VERSION_3_0 1
#define GL_ES_VERSION_3_1 1
#define GL_ES_VERSION_3_2 1
#endif
#include "glad.h"

#ifdef _WIN32
// Versions:
#define WGL_VERSION_1_0 1
// Mandatory extensions:
#define WGL_ARB_create_context					1
#define WGL_ARB_create_context_profile			1
#define WGL_ARB_pixel_format					1
// Optional extensions:
#define WGL_EXT_swap_control					1
#include "glad_wgl.h"
#endif

#ifdef __linux__
#if !defined(__ANDROID__) //karin: for Android
// Versions:
#define GLX_VERSION_1_0 1
#define GLX_VERSION_1_1 1
#define GLX_VERSION_1_2 1
#define GLX_VERSION_1_3 1
#define GLX_VERSION_1_4 1
// Mandatory extensions:
#define GLX_ARB_create_context					1
#define GLX_ARB_create_context_profile			1
#include "glad_glx.h"
#endif
#endif

#define QGL_REQUIRED_VERSION_MAJOR 3
#define QGL_REQUIRED_VERSION_MINOR 3

// Loads all GL/WGL/GLX functions from OpenGL library (using glad-generated loader).
// Note: no requirements are checked here, run GLimp_CheckRequiredFeatures afterwards.
void GLimp_LoadFunctions(bool inContext = true);

// Check and load all optional extensions.
// Fills extensions flags in glConfig and loads optional function pointers.
void GLimp_CheckRequiredFeatures();

// Unloads OpenGL library (does NOTHING in glad loader).
inline void GLimp_UnloadFunctions() {}

#endif
