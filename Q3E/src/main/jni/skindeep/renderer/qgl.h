/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#if defined( ID_DEDICATED ) && defined( _WIN32 )
// to allow stubbing gl on windows, define WINGDIAPI to nothing - it would otherwise be
// extended to __declspec(dllimport) on MSVC (our stub is no dll.)
	#ifdef WINGDIAPI
		#pragma push_macro("WINGDIAPI")
		#undef WINGDIAPI
		#define WINGDIAPI
	#endif
#endif

#include <SDL_opengl.h>

#if defined( ID_DEDICATED ) && defined( _WIN32 )
// restore WINGDIAPI
	#ifdef WINGDIAPI
		#pragma pop_macro("WINGDIAPI")
	#endif
#endif

typedef void (*GLExtension_t)(void);

#ifdef __cplusplus
	extern "C" {
#endif

GLExtension_t GLimp_ExtensionPointer( const char *name );

#ifdef __cplusplus
	}
#endif


#ifdef _GLES //karin: glClearDepthf and glDepthRangef on GLES
#define qglDepthRange qglDepthRangef
#define qglClearDepth qglClearDepthf
#endif

// declare qgl functions
#define QGLPROC(name, rettype, args) extern rettype (APIENTRYP q##name) args;
#include "renderer/qgl_proc.h"

// multitexture
extern	void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
extern	void ( APIENTRY * qglMultiTexCoord2fvARB )( GLenum texture, GLfloat *st );
extern	void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
extern	void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

// SM: Swap out ARB_vertex_buffer_object for GL_EXT_PROTOTYPES aka Core 2.0+ version
// and only the functions we're actively using
// ARB_vertex_buffer_object
// extern PFNGLBINDBUFFERARBPROC qglBindBufferARB;
// extern PFNGLDELETEBUFFERSARBPROC qglDeleteBuffersARB;
// extern PFNGLGENBUFFERSARBPROC qglGenBuffersARB;
// extern PFNGLISBUFFERARBPROC qglIsBufferARB;
// extern PFNGLBUFFERDATAARBPROC qglBufferDataARB;
// extern PFNGLBUFFERSUBDATAARBPROC qglBufferSubDataARB;
// extern PFNGLGETBUFFERSUBDATAARBPROC qglGetBufferSubDataARB;
// extern PFNGLMAPBUFFERARBPROC qglMapBufferARB;
// extern PFNGLUNMAPBUFFERARBPROC qglUnmapBufferARB;
// extern PFNGLGETBUFFERPARAMETERIVARBPROC qglGetBufferParameterivARB;
// extern PFNGLGETBUFFERPOINTERVARBPROC qglGetBufferPointervARB;
extern PFNGLBINDBUFFERPROC	qglBindBuffer;
extern PFNGLGENBUFFERSPROC	qglGenBuffers;
extern PFNGLBUFFERDATAPROC	qglBufferData;
extern PFNGLBUFFERSUBDATAPROC qglBufferSubData;
#ifdef _GLES //karin: delete VBO proc
extern PFNGLDELETEBUFFERSPROC qglDeleteBuffers;
#endif

// 3D textures
extern void ( APIENTRY *qglTexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

// shared texture palette
extern	void ( APIENTRY *qglColorTableEXT)( int, int, int, int, int, const void * );

// EXT_stencil_two_side
extern	PFNGLACTIVESTENCILFACEEXTPROC	qglActiveStencilFaceEXT;

// DG: couldn't find any extension for this, it's supported in GL2.0 and newer, incl OpenGL ES2.0
extern PFNGLSTENCILOPSEPARATEPROC qglStencilOpSeparate;

// ARB_texture_compression
extern	PFNGLCOMPRESSEDTEXIMAGE2DARBPROC	qglCompressedTexImage2DARB;
extern	PFNGLGETCOMPRESSEDTEXIMAGEARBPROC	qglGetCompressedTexImageARB;

// ARB_vertex_program / ARB_fragment_program
extern PFNGLVERTEXATTRIBPOINTERARBPROC		qglVertexAttribPointerARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC	qglEnableVertexAttribArrayARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	qglDisableVertexAttribArrayARB;
extern PFNGLPROGRAMSTRINGARBPROC			qglProgramStringARB;
extern PFNGLBINDPROGRAMARBPROC				qglBindProgramARB;
extern PFNGLGENPROGRAMSARBPROC				qglGenProgramsARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC	qglProgramEnvParameter4fvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC	qglProgramLocalParameter4fvARB;
extern PFNGLGETPROGRAMIVARBPROC				qglGetProgramivARB;

// GL_EXT_depth_bounds_test
extern PFNGLDEPTHBOUNDSEXTPROC              qglDepthBoundsEXT;

// SM: Debug extensions to help with frame captures
extern PFNGLOBJECTLABELPROC					qglObjectLabel;
extern PFNGLPUSHDEBUGGROUPPROC				qglPushDebugGroup;
extern PFNGLPOPDEBUGGROUPPROC				qglPopDebugGroup;


// blendo eric: frame/render buffers
extern PFNGLGENFRAMEBUFFERSPROC				qglGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC			qglDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC				qglBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC		qglFramebufferTexture2D;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC		qglCheckFramebufferStatus;
extern PFNGLBLITFRAMEBUFFERPROC				qglBlitFramebuffer;
extern PFNGLDRAWBUFFERSPROC					qglDrawBuffers;
extern PFNGLCLEARBUFFERUIVPROC				qglClearBufferuiv;
extern PFNGLCLEARBUFFERFVPROC				qglClearBufferfv;

// SM: GLSL functions
extern PFNGLCREATEPROGRAMPROC				qglCreateProgram;
extern PFNGLATTACHSHADERPROC				qglAttachShader;
extern PFNGLLINKPROGRAMPROC					qglLinkProgram;
extern PFNGLDELETEPROGRAMPROC				qglDeleteProgram;
extern PFNGLDELETESHADERPROC				qglDeleteShader;
extern PFNGLUSEPROGRAMPROC					qglUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC			qglGetUniformLocation;
extern PFNGLUNIFORMMATRIX4FVPROC			qglUniformMatrix4fv;
extern PFNGLUNIFORM4FVPROC					qglUniform4fv;
extern PFNGLUNIFORM3FVPROC					qglUniform3fv;
extern PFNGLUNIFORM2FVPROC					qglUniform2fv;
extern PFNGLUNIFORM1FPROC					qglUniform1f;
extern PFNGLUNIFORM1IPROC					qglUniform1i;
extern PFNGLUNIFORM1UIPROC					qglUniform1ui;
extern PFNGLCREATESHADERPROC				qglCreateShader;
extern PFNGLSHADERSOURCEPROC				qglShaderSource;
extern PFNGLCOMPILESHADERPROC				qglCompileShader;
extern PFNGLGETSHADERIVPROC					qglGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC			qglGetShaderInfoLog;
extern PFNGLGETPROGRAMIVPROC				qglGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC			qglGetProgramInfoLog;
extern PFNGLGETUNIFORMBLOCKINDEXPROC		qglGetUniformBlockIndex;
extern PFNGLUNIFORMBLOCKBINDINGPROC			qglUniformBlockBinding;
extern PFNGLBINDBUFFERRANGEPROC				qglBindBufferRange;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC		qglEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC	qglDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC			qglVertexAttribPointer;
extern PFNGLACTIVETEXTUREPROC				qglActiveTexture;
extern PFNGLBINDBUFFERBASEPROC				qglBindBufferBase;

#endif
