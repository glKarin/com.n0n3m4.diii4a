
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
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#define GL_GLEXT_PROTOTYPES

#ifdef ID_TARGET_OPENGL
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#define GL_APIENTRY	GLAPIENTRY
#else
#ifdef _OPENGLES3
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#endif

// OpenGLES2.0 compat
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
#ifndef GL_MAX_COLOR_ATTACHMENTS
#define GL_MAX_COLOR_ATTACHMENTS GL_MAX_COLOR_ATTACHMENTS_EXT
#endif
#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8 GL_STENCIL_INDEX8_OES
#endif
#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#endif
#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES                  0x8D64
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24  GL_DEPTH_COMPONENT24_OES
#endif

#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F GL_DEPTH_COMPONENT24_OES
#endif

//#if defined(GL_ES_VERSION_2_0)
#ifndef GL_RGB8
#define GL_RGB8	GL_RGB
#endif
#ifndef GL_RGBA8
#define GL_RGBA8	GL_RGBA
#endif
#ifndef GL_ALPHA8
#define GL_ALPHA8 GL_ALPHA
#endif
#ifndef GL_RGB5
#define GL_RGB5	GL_RGB5_A1 // GL_RGBA
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83f2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83f3
#endif
//#endif

#include "matrix/esUtil.h"

typedef void (*GLExtension_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

	GLExtension_t GLimp_ExtensionPointer(const char *name);

#ifdef __cplusplus
}
#endif

// declare qgl functions
#define QGLPROC(name, rettype, args) extern rettype (GL_APIENTRYP q##name) args;
#include "qgl_proc.h"

#endif
