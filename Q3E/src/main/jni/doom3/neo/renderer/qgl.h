
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
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#endif

//#if defined(GL_ES_VERSION_2_0)
#ifndef GL_RGB8 //karin: !!Only for OpenGLES2.0 compat, it defined and not equals GL_RGB if in OpenGLES3.0!!
#define GL_RGB8	GL_RGB
#endif
#ifndef GL_RGBA8 //karin: !!Only for OpenGLES2.0 compat, it defined and not equals GL_RGBA if in OpenGLES3.0!!
#define GL_RGBA8	GL_RGBA
#endif
#ifndef GL_ALPHA8 //karin: !!Only for OpenGLES2.0 compat, it defined and not equals GL_ALPHA if in OpenGLES3.0!!
#define GL_ALPHA8 GL_ALPHA
#endif
#ifndef GL_RGB5 //karin: !!Only for OpenGLES2.0 compat, it defined and not equals GL_RGB if in OpenGLES3.0!!
#define GL_RGB5	GL_RGB565 // GL_RGB5_A1 // GL_RGBA
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83f2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83f3
#endif
#ifndef GL_COMPRESSED_RGB_ARB
#define GL_COMPRESSED_RGB_ARB             0x84ED
#endif
#ifndef GL_COMPRESSED_RGBA_ARB
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#endif

#ifndef GL_LUMINANCE8 //karin: !!Only for OpenGLES2.0 compat, it not same value if in OpenGL!!
#define GL_LUMINANCE8	GL_LUMINANCE
#endif
#ifndef GL_LUMINANCE8_ALPHA8 //karin: !!Only for OpenGLES2.0 compat, it not same value if in OpenGL!!
#define GL_LUMINANCE8_ALPHA8	GL_LUMINANCE_ALPHA
#endif
//#endif

// GLES3.1
#ifndef GL_DEPTH_STENCIL_TEXTURE_MODE
#define GL_DEPTH_STENCIL_TEXTURE_MODE     0x90EA
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX                  0x1901
#endif
#ifndef GL_TEXTURE_WIDTH
#define GL_TEXTURE_WIDTH                  0x1000
#endif
#ifndef GL_TEXTURE_HEIGHT
#define GL_TEXTURE_HEIGHT                 0x1001
#endif
#ifndef GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_INTERNAL_FORMAT        0x1003
#endif
#ifndef GL_TEXTURE_RED_TYPE
#define GL_TEXTURE_RED_TYPE               0x8C10
#endif
#ifndef GL_TEXTURE_GREEN_TYPE
#define GL_TEXTURE_GREEN_TYPE             0x8C11
#endif
#ifndef GL_TEXTURE_BLUE_TYPE
#define GL_TEXTURE_BLUE_TYPE              0x8C12
#endif
#ifndef GL_TEXTURE_ALPHA_TYPE
#define GL_TEXTURE_ALPHA_TYPE             0x8C13
#endif
#ifndef GL_TEXTURE_RED_SIZE
#define GL_TEXTURE_RED_SIZE               0x805C
#endif
#ifndef GL_TEXTURE_GREEN_SIZE
#define GL_TEXTURE_GREEN_SIZE             0x805D
#endif
#ifndef GL_TEXTURE_BLUE_SIZE
#define GL_TEXTURE_BLUE_SIZE              0x805E
#endif
#ifndef GL_TEXTURE_ALPHA_SIZE
#define GL_TEXTURE_ALPHA_SIZE             0x805F
#endif

// debug
#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#endif
#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT                   0x92E0
#endif
#ifndef GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#endif
#ifndef GL_DEBUG_CALLBACK_FUNCTION
#define GL_DEBUG_CALLBACK_FUNCTION        0x8244
#endif
#ifndef GL_DEBUG_CALLBACK_USER_PARAM
#define GL_DEBUG_CALLBACK_USER_PARAM      0x8245
#endif
#ifndef GL_DEBUG_SOURCE_API
#define GL_DEBUG_SOURCE_API               0x8246
#endif
#ifndef GL_DEBUG_SOURCE_WINDOW_SYSTEM
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247
#endif
#ifndef GL_DEBUG_SOURCE_SHADER_COMPILER
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#endif
#ifndef GL_DEBUG_SOURCE_THIRD_PARTY
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#endif
#ifndef GL_DEBUG_SOURCE_APPLICATION
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#endif
#ifndef GL_DEBUG_SOURCE_OTHER
#define GL_DEBUG_SOURCE_OTHER             0x824B
#endif
#ifndef GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_TYPE_ERROR               0x824C
#endif
#ifndef GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#endif
#ifndef GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E
#endif
#ifndef GL_DEBUG_TYPE_PORTABILITY
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#endif
#ifndef GL_DEBUG_TYPE_PERFORMANCE
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#endif
#ifndef GL_DEBUG_TYPE_OTHER
#define GL_DEBUG_TYPE_OTHER               0x8251
#endif
#ifndef GL_DEBUG_TYPE_MARKER
#define GL_DEBUG_TYPE_MARKER              0x8268
#endif
#ifndef GL_DEBUG_TYPE_PUSH_GROUP
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#endif
#ifndef GL_DEBUG_TYPE_POP_GROUP
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#endif
#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#endif
#ifndef GL_MAX_DEBUG_GROUP_STACK_DEPTH
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH    0x826C
#endif
#ifndef GL_DEBUG_GROUP_STACK_DEPTH
#define GL_DEBUG_GROUP_STACK_DEPTH        0x826D
#endif
#ifndef GL_MAX_LABEL_LENGTH
#define GL_MAX_LABEL_LENGTH               0x82E8
#endif
#ifndef GL_MAX_DEBUG_MESSAGE_LENGTH
#define GL_MAX_DEBUG_MESSAGE_LENGTH       0x9143
#endif
#ifndef GL_MAX_DEBUG_LOGGED_MESSAGES
#define GL_MAX_DEBUG_LOGGED_MESSAGES      0x9144
#endif
#ifndef GL_DEBUG_LOGGED_MESSAGES
#define GL_DEBUG_LOGGED_MESSAGES          0x9145
#endif
#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#endif
#ifndef GL_DEBUG_SEVERITY_MEDIUM
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#endif
#ifndef GL_DEBUG_SEVERITY_LOW
#define GL_DEBUG_SEVERITY_LOW             0x9148
#endif


//#include "matrix/esUtil.h"

typedef void (*GLExtension_t)(void);

//#ifdef __cplusplus
//extern "C" {
//#endif

	GLExtension_t GLimp_ExtensionPointer(const char *name);
	bool GLimp_ProcIsValid(const void *func);
	#define GLIMP_PROCISVALID(func) GLimp_ProcIsValid((const void *)(func))

//#ifdef __cplusplus
//}
//#endif

// declare qgl functions
#ifdef GL_ES_VERSION_3_0 // GLES3.1
typedef void (GL_APIENTRY  *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
#endif
#define QGLPROC(name, rettype, args) extern rettype (GL_APIENTRYP q##name) args;
#include "qgl_proc.h"
#endif

//#define QGL_ALIAS
#ifdef QGL_ALIAS

#define glActiveTexture qglActiveTexture
#define glAttachShader qglAttachShader
#define glBindAttribLocation qglBindAttribLocation
#define glBindBuffer qglBindBuffer
#define glBindFramebuffer qglBindFramebuffer
#define glBindRenderbuffer qglBindRenderbuffer
#define glBindTexture qglBindTexture
#define glBlendColor qglBlendColor
#define glBlendEquation qglBlendEquation
#define glBlendEquationSeparate qglBlendEquationSeparate
#define glBlendFunc qglBlendFunc
#define glBlendFuncSeparate qglBlendFuncSeparate
#define glBufferData qglBufferData
#define glBufferSubData qglBufferSubData
#define glCheckFramebufferStatus qglCheckFramebufferStatus
#define glClear qglClear
#define glClearColor qglClearColor
#define glClearDepthf qglClearDepthf
#define glClearStencil qglClearStencil
#define glColorMask qglColorMask
#define glCompileShader qglCompileShader
#define glCompressedTexImage2D qglCompressedTexImage2D
#define glCompressedTexSubImage2D qglCompressedTexSubImage2D
#define glCopyTexImage2D qglCopyTexImage2D
#define glCopyTexSubImage2D qglCopyTexSubImage2D
#define glCreateProgram qglCreateProgram
#define glCreateShader qglCreateShader
#define glCullFace qglCullFace
#define glDeleteBuffers qglDeleteBuffers
#define glDeleteFramebuffers qglDeleteFramebuffers
#define glDeleteProgram qglDeleteProgram
#define glDeleteRenderbuffers qglDeleteRenderbuffers
#define glDeleteShader qglDeleteShader
#define glDeleteTextures qglDeleteTextures
#define glDepthFunc qglDepthFunc
#define glDepthMask qglDepthMask
#define glDepthRangef qglDepthRangef
#define glDetachShader qglDetachShader
#define glDisable qglDisable
#define glDisableVertexAttribArray qglDisableVertexAttribArray
#define glDrawArrays qglDrawArrays
#define glDrawElements qglDrawElements
#define glEnable qglEnable
#define glEnableVertexAttribArray qglEnableVertexAttribArray
#define glFinish qglFinish
#define glFlush qglFlush
#define glFramebufferRenderbuffer qglFramebufferRenderbuffer
#define glFramebufferTexture2D qglFramebufferTexture2D
#define glFrontFace qglFrontFace
#define glGenBuffers qglGenBuffers
#define glGenerateMipmap qglGenerateMipmap
#define glGenFramebuffers qglGenFramebuffers
#define glGenRenderbuffers qglGenRenderbuffers
#define glGenTextures qglGenTextures
#define glGetActiveAttrib qglGetActiveAttrib
#define glGetActiveUniform qglGetActiveUniform
#define glGetAttachedShaders qglGetAttachedShaders
#define glGetAttribLocation qglGetAttribLocation
#define glGetBooleanv qglGetBooleanv
#define glGetBufferParameteriv qglGetBufferParameteriv
#define glGetError qglGetError
#define glGetFloatv qglGetFloatv
#define glGetFramebufferAttachmentParameteriv qglGetFramebufferAttachmentParameteriv
#define glGetIntegerv qglGetIntegerv
#define glGetProgramiv qglGetProgramiv
#define glGetProgramInfoLog qglGetProgramInfoLog
#define glGetRenderbufferParameteriv qglGetRenderbufferParameteriv
#define glGetShaderiv qglGetShaderiv
#define glGetShaderInfoLog qglGetShaderInfoLog
#define glGetShaderPrecisionFormat qglGetShaderPrecisionFormat
#define glGetShaderSource qglGetShaderSource
#define glGetString qglGetString
#define glGetTexParameterfv qglGetTexParameterfv
#define glGetTexParameteriv qglGetTexParameteriv
#define glGetUniformfv qglGetUniformfv
#define glGetUniformiv qglGetUniformiv
#define glGetUniformLocation qglGetUniformLocation
#define glGetVertexAttribfv qglGetVertexAttribfv
#define glGetVertexAttribiv qglGetVertexAttribiv
#define glGetVertexAttribPointerv qglGetVertexAttribPointerv
#define glHint qglHint
#define glIsBuffer qglIsBuffer
#define glIsEnabled qglIsEnabled
#define glIsFramebuffer qglIsFramebuffer
#define glIsProgram qglIsProgram
#define glIsRenderbuffer qglIsRenderbuffer
#define glIsShader qglIsShader
#define glIsTexture qglIsTexture
#define glLineWidth qglLineWidth
#define glLinkProgram qglLinkProgram
#define glPixelStorei qglPixelStorei
#define glPolygonOffset qglPolygonOffset
#define glReadPixels qglReadPixels
#define glReleaseShaderCompiler qglReleaseShaderCompiler
#define glRenderbufferStorage qglRenderbufferStorage
#define glSampleCoverage qglSampleCoverage
#define glScissor qglScissor
#define glShaderBinary qglShaderBinary
#define glShaderSource qglShaderSource
#define glStencilFunc qglStencilFunc
#define glStencilFuncSeparate qglStencilFuncSeparate
#define glStencilMask qglStencilMask
#define glStencilMaskSeparate qglStencilMaskSeparate
#define glStencilOp qglStencilOp
#define glStencilOpSeparate qglStencilOpSeparate
#define glTexImage2D qglTexImage2D
#define glTexParameterf qglTexParameterf
#define glTexParameterfv qglTexParameterfv
#define glTexParameteri qglTexParameteri
#define glTexParameteriv qglTexParameteriv
#define glTexSubImage2D qglTexSubImage2D
#define glUniform1f qglUniform1f
#define glUniform1fv qglUniform1fv
#define glUniform1i qglUniform1i
#define glUniform1iv qglUniform1iv
#define glUniform2f qglUniform2f
#define glUniform2fv qglUniform2fv
#define glUniform2i qglUniform2i
#define glUniform2iv qglUniform2iv
#define glUniform3f qglUniform3f
#define glUniform3fv qglUniform3fv
#define glUniform3i qglUniform3i
#define glUniform3iv qglUniform3iv
#define glUniform4f qglUniform4f
#define glUniform4fv qglUniform4fv
#define glUniform4i qglUniform4i
#define glUniform4iv qglUniform4iv
#define glUniformMatrix2fv qglUniformMatrix2fv
#define glUniformMatrix3fv qglUniformMatrix3fv
#define glUniformMatrix4fv qglUniformMatrix4fv
#define glUseProgram qglUseProgram
#define glValidateProgram qglValidateProgram
#define glVertexAttrib1f qglVertexAttrib1f
#define glVertexAttrib1fv qglVertexAttrib1fv
#define glVertexAttrib2f qglVertexAttrib2f
#define glVertexAttrib2fv qglVertexAttrib2fv
#define glVertexAttrib3f qglVertexAttrib3f
#define glVertexAttrib3fv qglVertexAttrib3fv
#define glVertexAttrib4f qglVertexAttrib4f
#define glVertexAttrib4fv qglVertexAttrib4fv
#define glVertexAttribPointer qglVertexAttribPointer
#define glViewport qglViewport


#if USE_MAP
#define glMapBufferRange qglMapBufferRange
#define glUnmapBuffer qglUnmapBuffer
#endif

#ifdef _OPENGLES3
// GLES3.0
#define glTexImage3D qglTexImage3D
#define glReadBuffer qglReadBuffer
#define glDrawBuffers qglDrawBuffers
#define glFramebufferTextureLayer qglFramebufferTextureLayer
#define glBlitFramebuffer qglBlitFramebuffer
#define glProgramBinary qglProgramBinary
#define glGetProgramBinary qglGetProgramBinary

// GLES3.1
#define glDebugMessageControl qglDebugMessageControl
#define glDebugMessageCallback qglDebugMessageCallback
#define glGetDebugMessageLog qglGetDebugMessageLog
#define glGetTexLevelParameteriv qglGetTexLevelParameteriv
#define glGetInternalformativ qglGetInternalformativ
#endif

#endif

#if defined( _WIN32 ) && defined(ID_ALLOW_TOOLS)

extern  BOOL(WINAPI * qwglSwapBuffers)(HDC);
extern int Win_ChoosePixelFormat(HDC hdc);

extern BOOL(WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
extern HGLRC(WINAPI * qwglCreateContext)(HDC);
extern HGLRC(WINAPI * qwglCreateLayerContext)(HDC, int);
extern BOOL(WINAPI * qwglDeleteContext)(HGLRC);
extern HGLRC(WINAPI * qwglGetCurrentContext)(VOID);
extern HDC(WINAPI * qwglGetCurrentDC)(VOID);
extern PROC(WINAPI * qwglGetProcAddress)(LPCSTR);
extern BOOL(WINAPI * qwglMakeCurrent)(HDC, HGLRC);
extern BOOL(WINAPI * qwglShareLists)(HGLRC, HGLRC);
extern BOOL(WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

extern BOOL(WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                          FLOAT, int, LPGLYPHMETRICSFLOAT);

extern BOOL(WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                             LPLAYERPLANEDESCRIPTOR);
extern int  (WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                  CONST COLORREF *);
extern int  (WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                  COLORREF *);
extern BOOL(WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
extern BOOL(WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

#include "glsl/gles2_compat.h"

#define qglBegin glBegin
#define qglEnd glEnd
#define qglColor4f glColor4f
#define qglColor3f glColor3f
#define qglVertex2f glVertex2f
#define qglVertex3f glVertex3f
#define qglVertex3fv glVertex3fv
#define qglMatrixMode glMatrixMode
#define qglLoadIdentity glLoadIdentity
#define qglOrtho glOrtho
#define qglPushMatrix glPushMatrix
#define qglPopMatrix glPopMatrix
#define qglColor3fv glColor3fv
#define qglColor4fv glColor4fv
#define qglRotatef glRotatef
#define qglTranslatef glTranslatef
#define qglRectf glRectf
#define qglTexCoord2f glTexCoord2f
#define qglTexCoord2fv glTexCoord2fv
#define qglEnableClientState glEnableClientState
#define qglPushAttrib glPushAttrib
#define qglPopAttrib glPopAttrib
#define qglLoadMatrixf glLoadMatrixf
#define qglPolygonMode glPolygonMode

#define GL_COMPILE_AND_EXECUTE 0x1301
#define qglPointSize(x)
#define qglRasterPos2f(x, y)
#define qglRasterPos3f(x, y, z)
#define qglRasterPos3fv(x)
#define qglCallLists(x, y, z)
#define qglCallList(x)
#define qglEndList()
#define qglNewList(x, y)
#define qglGenLists(x) 0
#define qglListBase(x)
#define qglDeleteLists(x, y)
#define qgluNewQuadric() NULL
#define qgluDeleteQuadric()
#define qgluSphere gluSphere

#endif	// _WIN32 && ID_ALLOW_TOOLS
