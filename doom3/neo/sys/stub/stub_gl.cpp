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
#include "sys/platform.h"

#include "renderer/tr_local.h"

#ifdef _MSC_VER
#pragma warning(push)
// for each gl function we get an inconsistent dll linkage warning, because SDL_OpenGL.h says they're dllimport
// showing one warning is enough and it doesn't matter anyway (these stubs are for the dedicated server)
#pragma warning( once : 4273 )
#else
#endif

void qglActiveTexture (GLenum texture) {}
void qglAttachShader (GLuint program, GLuint shader) {}
void qglBindAttribLocation (GLuint program, GLuint index, const GLchar *name) {}
void qglBindBuffer (GLenum target, GLuint buffer) {}
void qglBindFramebuffer (GLenum target, GLuint framebuffer) {}
void qglBindRenderbuffer (GLenum target, GLuint renderbuffer) {}
void qglBindTexture (GLenum target, GLuint texture) {}
void qglBlendColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
void qglBlendEquation (GLenum mode) {}
void qglBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha) {}
void qglBlendFunc (GLenum sfactor, GLenum dfactor) {}
void qglBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {}
void qglBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage) {}
void qglBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {}
GLenum qglCheckFramebufferStatus (GLenum target) { return GL_FRAMEBUFFER_COMPLETE; }
void qglClear (GLbitfield mask) {}
void qglClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
void qglClearDepthf (GLfloat d) {}
void qglClearStencil (GLint s) {}
void qglColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
void qglCompileShader (GLuint shader) {}
void qglCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {}
void qglCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {}
void qglCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {}
void qglCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
GLuint qglCreateProgram (void) { return 0; }
GLuint qglCreateShader (GLenum type) { return 0; }
void qglCullFace (GLenum mode) {}
void qglDeleteBuffers (GLsizei n, const GLuint *buffers) {}
void qglDeleteFramebuffers (GLsizei n, const GLuint *framebuffers) {}
void qglDeleteProgram (GLuint program) {}
void qglDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers) {}
void qglDeleteShader (GLuint shader) {}
void qglDeleteTextures (GLsizei n, const GLuint *textures) {}
void qglDepthFunc (GLenum func) {}
void qglDepthMask (GLboolean flag) {}
void qglDepthRangef (GLfloat n, GLfloat f) {}
void qglDetachShader (GLuint program, GLuint shader) {}
void qglDisable (GLenum cap) {}
void qglDisableVertexAttribArray (GLuint index) {}
void qglDrawArrays (GLenum mode, GLint first, GLsizei count) {}
void qglDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices) {}
void qglEnable (GLenum cap) {}
void qglEnableVertexAttribArray (GLuint index) {}
void qglFinish (void) {}
void qglFlush (void) {}
void qglFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void qglFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void qglFrontFace (GLenum mode) {}
void qglGenBuffers (GLsizei n, GLuint *buffers) {}
void qglGenerateMipmap (GLenum target) {}
void qglGenFramebuffers (GLsizei n, GLuint *framebuffers) {}
void qglGenRenderbuffers (GLsizei n, GLuint *renderbuffers) {}
void qglGenTextures (GLsizei n, GLuint *textures) {}
void qglGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {}
void qglGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {}
void qglGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {}
GLint qglGetAttribLocation (GLuint program, const GLchar *name) { return -1; }
void qglGetBooleanv (GLenum pname, GLboolean *data) {}
void qglGetBufferParameteriv (GLenum target, GLenum pname, GLint *params) {}
GLenum qglGetError (void) { return GL_NO_ERROR; }
void qglGetFloatv (GLenum pname, GLfloat *data) {}
void qglGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params) {}
void qglGetIntegerv (GLenum pname, GLint *data) {}
void qglGetProgramiv (GLuint program, GLenum pname, GLint *params) {}
void qglGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
void qglGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint *params) {}
void qglGetShaderiv (GLuint shader, GLenum pname, GLint *params) {}
void qglGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
void qglGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {}
void qglGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {}
const GLubyte * qglGetString (GLenum name) { return (const GLubyte *)""; }
void qglGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) {}
void qglGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {}
void qglGetUniformfv (GLuint program, GLint location, GLfloat *params) {}
void qglGetUniformiv (GLuint program, GLint location, GLint *params) {}
GLint qglGetUniformLocation (GLuint program, const GLchar *name) { return -1; }
void qglGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params) {}
void qglGetVertexAttribiv (GLuint index, GLenum pname, GLint *params) {}
void qglGetVertexAttribPointerv (GLuint index, GLenum pname, void **pointer) {}
void qglHint (GLenum target, GLenum mode) {}
GLboolean qglIsBuffer (GLuint buffer) { return GL_FALSE; }
GLboolean qglIsEnabled (GLenum cap) { return GL_FALSE; }
GLboolean qglIsFramebuffer (GLuint framebuffer) { return GL_FALSE; }
GLboolean qglIsProgram (GLuint program) { return GL_FALSE; }
GLboolean qglIsRenderbuffer (GLuint renderbuffer) { return GL_FALSE; }
GLboolean qglIsShader (GLuint shader) { return GL_FALSE; }
GLboolean qglIsTexture (GLuint texture) { return GL_FALSE; }
void qglLineWidth (GLfloat width) {}
void qglLinkProgram (GLuint program) {}
void qglPixelStorei (GLenum pname, GLint param) {}
void qglPolygonOffset (GLfloat factor, GLfloat units) {}
void qglReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {}
void qglReleaseShaderCompiler (void) {}
void qglRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
void qglSampleCoverage (GLfloat value, GLboolean invert) {}
void qglScissor (GLint x, GLint y, GLsizei width, GLsizei height) {}
void qglShaderBinary (GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) {}
void qglShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {}
void qglStencilFunc (GLenum func, GLint ref, GLuint mask) {}
void qglStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask) {}
void qglStencilMask (GLuint mask) {}
void qglStencilMaskSeparate (GLenum face, GLuint mask) {}
void qglStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {}
void qglStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {}
void qglTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {}
void qglTexParameterf (GLenum target, GLenum pname, GLfloat param) {}
void qglTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {}
void qglTexParameteri (GLenum target, GLenum pname, GLint param) {}
void qglTexParameteriv (GLenum target, GLenum pname, const GLint *params) {}
void qglTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {}
void qglUniform1f (GLint location, GLfloat v0) {}
void qglUniform1fv (GLint location, GLsizei count, const GLfloat *value) {}
void qglUniform1i (GLint location, GLint v0) {}
void qglUniform1iv (GLint location, GLsizei count, const GLint *value) {}
void qglUniform2f (GLint location, GLfloat v0, GLfloat v1) {}
void qglUniform2fv (GLint location, GLsizei count, const GLfloat *value) {}
void qglUniform2i (GLint location, GLint v0, GLint v1) {}
void qglUniform2iv (GLint location, GLsizei count, const GLint *value) {}
void qglUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void qglUniform3fv (GLint location, GLsizei count, const GLfloat *value) {}
void qglUniform3i (GLint location, GLint v0, GLint v1, GLint v2) {}
void qglUniform3iv (GLint location, GLsizei count, const GLint *value) {}
void qglUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void qglUniform4fv (GLint location, GLsizei count, const GLfloat *value) {}
void qglUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
void qglUniform4iv (GLint location, GLsizei count, const GLint *value) {}
void qglUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
void qglUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
void qglUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
void qglUseProgram (GLuint program) {}
void qglValidateProgram (GLuint program) {}
void qglVertexAttrib1f (GLuint index, GLfloat x) {}
void qglVertexAttrib1fv (GLuint index, const GLfloat *v) {}
void qglVertexAttrib2f (GLuint index, GLfloat x, GLfloat y) {}
void qglVertexAttrib2fv (GLuint index, const GLfloat *v) {}
void qglVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z) {}
void qglVertexAttrib3fv (GLuint index, const GLfloat *v) {}
void qglVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void qglVertexAttrib4fv (GLuint index, const GLfloat *v) {}
void qglVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {}
void qglViewport (GLint x, GLint y, GLsizei width, GLsizei height) {}

#ifdef _OPENGLES3
void qglTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {}
void qglReadBuffer (GLenum src) {};
void qglDrawBuffers (GLsizei n, const GLenum *bufs) {};
void qglFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {};
void qglBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void qglProgramBinary (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) {}
void qglGetProgramBinary (GLuint program, GLsizei bufsize, GLsizei *length, GLenum *binaryFormat, void *binary) {}


void qglDebugMessageControl (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) {}

void qglDebugMessageCallback (GLDEBUGPROC callback, const void *userParam) {}
GLuint qglGetDebugMessageLog (GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) { return 0; }
void qglGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint * params) {}
void qglGetInternalformativ (GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params) {}

GLsync qglFenceSync (GLenum condition, GLbitfield flags) { return 0; }
GLboolean qglIsSync (GLsync sync) { return GL_FALSE; }
void qglDeleteSync (GLsync sync) {}
GLenum qglClientWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout) { return GL_ALREADY_SIGNALED; }
#endif


static void StubFunction( void ) {};
GLExtension_t GLimp_ExtensionPointer( const char *a) { return StubFunction; };

bool GLimp_Init(glimpParms_t a) {return true;};
void GLimp_SetGamma(unsigned short*a, unsigned short*b, unsigned short*c) {};
void GLimp_ResetGamma() {}
bool GLimp_SetScreenParms(glimpParms_t parms) { return true; };
void GLimp_Shutdown() {};
void GLimp_SwapBuffers() {};
void GLimp_ActivateContext() {};
void GLimp_DeactivateContext() {};
void GLimp_GrabInput(int flags) {};

void GLimp_EnableLogging(bool log) {}
int Sys_GetVideoRam(void) { return 0; }
void GLimp_Startup(void) {}
bool GLimp_ProcIsValid(const void *func) { return false; }
bool GLimp_CheckGLInitialized(void) { return true; }

#ifdef _IMGUI
void GLimp_ImGui_Init(void) {}
void GLimp_ImGui_NewFrame(void) {}
void GLimp_ImGui_EndFrame(void) {}
void GLimp_ImGui_Shutdown(void) {}
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
