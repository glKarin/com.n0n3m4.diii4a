
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
#ifdef __ANDROID__
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#endif
#endif

// OpenGLES2.0 compat
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
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

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83f2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83f3
#endif

#ifndef GL_TEXTURE_CUBE_MAP_EXT
#define GL_TEXTURE_CUBE_MAP_EXT GL_TEXTURE_CUBE_MAP
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT GL_TEXTURE_CUBE_MAP_POSITIVE_X
#endif
#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB GL_TEXTURE0
#endif
#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_ARB
#define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER
#endif
#ifndef GL_DYNAMIC_DRAW_ARB
#define GL_DYNAMIC_DRAW_ARB GL_DYNAMIC_DRAW
#endif
#ifndef GL_STREAM_DRAW_ARB
#define GL_STREAM_DRAW_ARB GL_STREAM_DRAW
#endif
#ifndef GL_STATIC_DRAW_ARB
#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW
#endif
#ifndef GL_INCR_WRAP_EXT
#define GL_INCR_WRAP_EXT GL_INCR_WRAP
#endif
#ifndef GL_DECR_WRAP_EXT
#define GL_DECR_WRAP_EXT GL_DECR_WRAP
#endif
#ifndef GL_MAX_TEXTURE_IMAGE_UNITS_ARB
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB GL_MAX_TEXTURE_IMAGE_UNITS
#endif
#ifndef GL_MAX_TEXTURE_UNITS_ARB
#define GL_MAX_TEXTURE_UNITS_ARB GL_MAX_TEXTURE_IMAGE_UNITS
#endif
#ifndef GL_MAX_TEXTURE_COORDS_ARB
#define GL_MAX_TEXTURE_COORDS_ARB GL_MAX_TEXTURE_SIZE
#endif



#define qglEnableVertexAttribArrayARB qglEnableVertexAttribArray
#define qglDisableVertexAttribArrayARB qglDisableVertexAttribArray
#define qglVertexAttribPointerARB qglVertexAttribPointer
#define qglGenBuffersARB qglGenBuffers
#define qglDeleteBuffersARB qglDeleteBuffers
#define qglActiveTextureARB qglActiveTexture
#define qglBindBufferARB qglBindBuffer
#define qglBufferSubDataARB qglBufferSubData
#define qglCompressedTexSubImage2DARB qglCompressedTexSubImage2D
#define qglBufferDataARB qglBufferData
#define qglUnmapBufferARB qglUnmapBuffer
#define qglCompressedTexImage2DARB qglCompressedTexImage2D

typedef GLsizeiptr GLsizeiptrARB;

typedef void (*GLExtension_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

	GLExtension_t GLimp_ExtensionPointer(const char *name);

#ifdef __cplusplus
}
#endif

// declare qgl functions
#define _GLDBG 0
typedef void (GL_APIENTRY  *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
#if _GLDBG
#define QGLPROC(name, rettype, args) extern rettype q##name args;
#else
#define QGLPROC(name, rettype, args) extern rettype (GL_APIENTRYP q##name) args;
#endif
#include "qgl_proc.h"

#include "qgl_def.h"
#define glEnableVertexAttribArrayARB glEnableVertexAttribArray
#define glDisableVertexAttribArrayARB glDisableVertexAttribArray
#define glVertexAttribPointerARB glVertexAttribPointer
#define glGenBuffersARB glGenBuffers
#define glDeleteBuffersARB glDeleteBuffers
#define glActiveTextureARB glActiveTexture
#define glBindBufferARB glBindBuffer
#define glBufferSubDataARB glBufferSubData
#define glCompressedTexSubImage2DARB glCompressedTexSubImage2D
#define glBufferDataARB glBufferData
#define glUnmapBufferARB glUnmapBuffer
#define glCompressedTexImage2DARB glCompressedTexImage2D

#endif
