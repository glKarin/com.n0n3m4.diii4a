// Copyright (C) 2004 Id Software, Inc.
//
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#if defined( _WINDOWS )

#include <gl/gl.h>

#elif defined( MACOS_X )

// magic flag to keep tiger gl.h from loading glext.h
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>

#elif defined( __linux__ )

// using our local glext.h
// http://oss.sgi.com/projects/ogl-sample/ABI/
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY
#include <GL/gl.h>
#include <GL/glx.h>

// TTimo - X.h has a '#define Success 0' line..
#undef Success

#elif defined( _XENON )

#include "../sys/dx/gl_stub.h"
#include "../sys/dx/glext_stub.h"

#else

#include <gl.h>

#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef WINAPI
#define WINAPI
#endif

// only use local glext.h if we are not using the system one already
// http://oss.sgi.com/projects/ogl-sample/ABI/
#ifndef GL_GLEXT_VERSION

#include "glext.h"

#endif

typedef void (*GLExtension_t)(void);

#ifdef __cplusplus
	extern "C" {
#endif

GLExtension_t GLimp_ExtensionPointer( const char *name );

#ifdef __cplusplus
	}
#endif

// multitexture
extern	void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
extern	void ( APIENTRY * qglMultiTexCoord2fvARB )( GLenum texture, GLfloat * RESTRICT st );
extern	void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
extern	void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

// ARB_vertex_buffer_object
extern PFNGLBINDBUFFERARBPROC qglBindBufferARB;
extern PFNGLDELETEBUFFERSARBPROC qglDeleteBuffersARB;
extern PFNGLGENBUFFERSARBPROC qglGenBuffersARB;
extern PFNGLISBUFFERARBPROC qglIsBufferARB;
extern PFNGLBUFFERDATAARBPROC qglBufferDataARB;
extern PFNGLBUFFERSUBDATAARBPROC qglBufferSubDataARB;
extern PFNGLGETBUFFERSUBDATAARBPROC qglGetBufferSubDataARB;
extern PFNGLMAPBUFFERARBPROC qglMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC qglUnmapBufferARB;
extern PFNGLGETBUFFERPARAMETERIVARBPROC qglGetBufferParameterivARB;
extern PFNGLGETBUFFERPOINTERVARBPROC qglGetBufferPointervARB;


// NV_register_combiners
extern	void ( APIENTRY *qglCombinerParameterfvNV )( GLenum pname, const GLfloat * RESTRICT params );
extern	void ( APIENTRY *qglCombinerParameterivNV )( GLenum pname, const GLint * RESTRICT params );
extern	void ( APIENTRY *qglCombinerParameterfNV )( GLenum pname, const GLfloat param );
extern	void ( APIENTRY *qglCombinerParameteriNV )( GLenum pname, const GLint param );
extern	void ( APIENTRY *qglCombinerInputNV )( GLenum stage, GLenum portion, GLenum variable, GLenum input,
											  GLenum mapping, GLenum componentUsage );
extern	void ( APIENTRY *qglCombinerOutputNV )( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, 
											   GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct,
											   GLboolean cdDotProduct, GLboolean muxSum );
extern	void ( APIENTRY *qglFinalCombinerInputNV )( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );

// RAVEN BEGIN
// dluetscher: added support for NV_vertex_program and NV_fragment_program extensions
extern void ( APIENTRY *qglBindProgramNV ) (GLenum, GLuint);
extern void ( APIENTRY *qglLoadProgramNV ) (GLenum, GLuint, GLsizei, const GLubyte * RESTRICT );
extern void ( APIENTRY *qglProgramParameter4fvNV ) (GLenum, GLuint, const GLfloat * RESTRICT );
// RAVEN END

// 3D textures
extern void ( APIENTRY *qglTexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid * RESTRICT );

// shared texture palette
extern	void ( APIENTRY *qglColorTableEXT)( int, int, int, int, int, const void * RESTRICT );

// RAVEN BEGIN
// dluetscher: added the BlendEquationEXT() from the blend minmax extension
extern	void ( APIENTRY *qglBlendEquationEXT)( GLenum mode );
// RAVEN END

// RAVEN BEGIN
// dluetscher: added support for the EXT_draw_range_elements extension
extern  void ( APIENTRY * qglDrawRangeElementsEXT )(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * RESTRICT indices);
// RAVEN END

// ATI_fragment_shader
extern	PFNGLGENFRAGMENTSHADERSATIPROC	qglGenFragmentShadersATI;
extern	PFNGLBINDFRAGMENTSHADERATIPROC	qglBindFragmentShaderATI;
extern	PFNGLDELETEFRAGMENTSHADERATIPROC qglDeleteFragmentShaderATI;
extern	PFNGLBEGINFRAGMENTSHADERATIPROC qglBeginFragmentShaderATI;
extern	PFNGLENDFRAGMENTSHADERATIPROC	qglEndFragmentShaderATI;
extern	PFNGLPASSTEXCOORDATIPROC		qglPassTexCoordATI;
extern	PFNGLSAMPLEMAPATIPROC			qglSampleMapATI;
extern	PFNGLCOLORFRAGMENTOP1ATIPROC	qglColorFragmentOp1ATI;
extern	PFNGLCOLORFRAGMENTOP2ATIPROC	qglColorFragmentOp2ATI;
extern	PFNGLCOLORFRAGMENTOP3ATIPROC	qglColorFragmentOp3ATI;
extern	PFNGLALPHAFRAGMENTOP1ATIPROC	qglAlphaFragmentOp1ATI;
extern	PFNGLALPHAFRAGMENTOP2ATIPROC	qglAlphaFragmentOp2ATI;
extern	PFNGLALPHAFRAGMENTOP3ATIPROC	qglAlphaFragmentOp3ATI;
extern	PFNGLSETFRAGMENTSHADERCONSTANTATIPROC	qglSetFragmentShaderConstantATI;

// EXT_stencil_two_side
extern	PFNGLACTIVESTENCILFACEEXTPROC	qglActiveStencilFaceEXT;


// ATI_separate_stencil
extern	PFNGLSTENCILOPSEPARATEATIPROC		qglStencilOpSeparateATI;
extern	PFNGLSTENCILFUNCSEPARATEATIPROC		qglStencilFuncSeparateATI;

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

// RAVEN BEGIN
// rjohnson: new shader stage system
// GLSL fragment
extern PFNGLCREATESHADEROBJECTARBPROC		qglCreateShaderObjectARB;
extern PFNGLDELETEOBJECTARBPROC				qglDeleteObjectARB;
extern PFNGLSHADERSOURCEARBPROC				qglShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC			qglCompileShaderARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC		qglGetObjectParameterivARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC		qglCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC				qglAttachObjectARB;
extern PFNGLDETACHOBJECTARBPROC				qglDetachObjectARB;
extern PFNGLLINKPROGRAMARBPROC				qglLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC			qglUseProgramObjectARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC		qglGetUniformLocationARB;
extern PFNGLUNIFORM1FARBPROC				qglUniform1fARB;
extern PFNGLUNIFORM1IARBPROC				qglUniform1iARB;
extern PFNGLUNIFORM1FVARBPROC				qglUniform1fvARB;
extern PFNGLUNIFORM2FVARBPROC				qglUniform2fvARB;
extern PFNGLUNIFORM3FVARBPROC				qglUniform3fvARB;
extern PFNGLUNIFORM4FVARBPROC				qglUniform4fvARB;
extern PFNGLGETINFOLOGARBPROC				qglGetInfoLogARB;
// RAVEN END

// GL_EXT_depth_bounds_test
extern PFNGLDEPTHBOUNDSEXTPROC              qglDepthBoundsEXT;

//===========================================================================

// non-windows systems will just redefine qgl* to gl*
#if defined( __APPLE__ ) || defined( ID_GL_HARDLINK )

#include "qgl_linked.h"

#else

// windows systems use a function pointer for each call so we can do our log file intercepts

extern  void ( APIENTRY * qglAccum )(GLenum op, GLfloat value);
extern  void ( APIENTRY * qglAlphaFunc )(GLenum func, GLclampf ref);
extern  GLboolean ( APIENTRY * qglAreTexturesResident )(GLsizei n, const GLuint * RESTRICT textures, GLboolean * RESTRICT residences);
extern  void ( APIENTRY * qglArrayElement )(GLint i);
extern  void ( APIENTRY * qglBegin )(GLenum mode);
extern  void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
extern  void ( APIENTRY * qglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * RESTRICT bitmap);
extern  void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
extern  void ( APIENTRY * qglCallList )(GLuint list);
extern  void ( APIENTRY * qglCallLists )(GLsizei n, GLenum type, const GLvoid * RESTRICT lists);
extern  void ( APIENTRY * qglClear )(GLbitfield mask);
extern  void ( APIENTRY * qglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern  void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern  void ( APIENTRY * qglClearDepth )(GLclampd depth);
extern  void ( APIENTRY * qglClearIndex )(GLfloat c);
extern  void ( APIENTRY * qglClearStencil )(GLint s);
extern  void ( APIENTRY * qglClipPlane )(GLenum plane, const GLdouble * RESTRICT equation);
extern  void ( APIENTRY * qglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
extern  void ( APIENTRY * qglColor3bv )(const GLbyte * RESTRICT v);
extern  void ( APIENTRY * qglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
extern  void ( APIENTRY * qglColor3dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
extern  void ( APIENTRY * qglColor3fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglColor3i )(GLint red, GLint green, GLint blue);
extern  void ( APIENTRY * qglColor3iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglColor3s )(GLshort red, GLshort green, GLshort blue);
extern  void ( APIENTRY * qglColor3sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
extern  void ( APIENTRY * qglColor3ubv )(const GLubyte * RESTRICT v);
extern  void ( APIENTRY * qglColor3ui )(GLuint red, GLuint green, GLuint blue);
extern  void ( APIENTRY * qglColor3uiv )(const GLuint * RESTRICT v);
extern  void ( APIENTRY * qglColor3us )(GLushort red, GLushort green, GLushort blue);
extern  void ( APIENTRY * qglColor3usv )(const GLushort * RESTRICT v);
extern  void ( APIENTRY * qglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern  void ( APIENTRY * qglColor4bv )(const GLbyte * RESTRICT v);
extern  void ( APIENTRY * qglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern  void ( APIENTRY * qglColor4dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern  void ( APIENTRY * qglColor4fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
extern  void ( APIENTRY * qglColor4iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern  void ( APIENTRY * qglColor4sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern  void ( APIENTRY * qglColor4ubv )(const GLubyte * RESTRICT v);
extern  void ( APIENTRY * qglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern  void ( APIENTRY * qglColor4uiv )(const GLuint * RESTRICT v);
extern  void ( APIENTRY * qglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern  void ( APIENTRY * qglColor4usv )(const GLushort * RESTRICT v);
extern  void ( APIENTRY * qglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern  void ( APIENTRY * qglColorMaterial )(GLenum face, GLenum mode);
extern  void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid * RESTRICT pointer);
extern  void ( APIENTRY * qglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern  void ( APIENTRY * qglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern  void ( APIENTRY * qglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern  void ( APIENTRY * qglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern  void ( APIENTRY * qglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( APIENTRY * qglCullFace )(GLenum mode);
extern  void ( APIENTRY * qglDeleteLists )(GLuint list, GLsizei range);
extern  void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint * RESTRICT textures);
extern  void ( APIENTRY * qglDepthFunc )(GLenum func);
extern  void ( APIENTRY * qglDepthMask )(GLboolean flag);
extern  void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
extern  void ( APIENTRY * qglDisable )(GLenum cap);
extern  void ( APIENTRY * qglDisableClientState )(GLenum array);
extern  void ( APIENTRY * qglDrawArrays )(GLenum mode, GLint first, GLsizei count);
extern  void ( APIENTRY * qglDrawBuffer )(GLenum mode);
extern  void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid * RESTRICT indices);
extern  void ( APIENTRY * qglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglEdgeFlag )(GLboolean flag);
extern  void ( APIENTRY * qglEdgeFlagPointer )(GLsizei stride, const GLvoid * RESTRICT pointer);
extern  void ( APIENTRY * qglEdgeFlagv )(const GLboolean * RESTRICT flag);
extern  void ( APIENTRY * qglEnable )(GLenum cap);
extern  void ( APIENTRY * qglEnableClientState )(GLenum array);
extern  void ( APIENTRY * qglEnd )(void);
extern  void ( APIENTRY * qglEndList )(void);
extern  void ( APIENTRY * qglEvalCoord1d )(GLdouble u);
extern  void ( APIENTRY * qglEvalCoord1dv )(const GLdouble * RESTRICT u);
extern  void ( APIENTRY * qglEvalCoord1f )(GLfloat u);
extern  void ( APIENTRY * qglEvalCoord1fv )(const GLfloat * RESTRICT u);
extern  void ( APIENTRY * qglEvalCoord2d )(GLdouble u, GLdouble v);
extern  void ( APIENTRY * qglEvalCoord2dv )(const GLdouble * RESTRICT u);
extern  void ( APIENTRY * qglEvalCoord2f )(GLfloat u, GLfloat v);
extern  void ( APIENTRY * qglEvalCoord2fv )(const GLfloat * RESTRICT u);
extern  void ( APIENTRY * qglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
extern  void ( APIENTRY * qglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern  void ( APIENTRY * qglEvalPoint1 )(GLint i);
extern  void ( APIENTRY * qglEvalPoint2 )(GLint i, GLint j);
extern  void ( APIENTRY * qglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat * RESTRICT buffer);
extern  void ( APIENTRY * qglFinish )(void);
extern  void ( APIENTRY * qglFlush )(void);
extern  void ( APIENTRY * qglFogf )(GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglFogfv )(GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglFogi )(GLenum pname, GLint param);
extern  void ( APIENTRY * qglFogiv )(GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglFrontFace )(GLenum mode);
extern  void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  GLuint ( APIENTRY * qglGenLists )(GLsizei range);
extern  void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint * RESTRICT textures);
extern  void ( APIENTRY * qglGetBooleanv )(GLenum pname, GLboolean * RESTRICT params);
extern  void ( APIENTRY * qglGetClipPlane )(GLenum plane, GLdouble *RESTRICT equation);
extern  void ( APIENTRY * qglGetDoublev )(GLenum pname, GLdouble * RESTRICT params);
extern  GLenum ( APIENTRY * qglGetError )(void);
extern  void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetIntegerv )(GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglGetLightfv )(GLenum light, GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetLightiv )(GLenum light, GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglGetMapdv )(GLenum target, GLenum query, GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglGetMapfv )(GLenum target, GLenum query, GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglGetMapiv )(GLenum target, GLenum query, GLint * RESTRICT v);
extern  void ( APIENTRY * qglGetMaterialfv )(GLenum face, GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetMaterialiv )(GLenum face, GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglGetPixelMapfv )(GLenum map, GLfloat * RESTRICT values);
extern  void ( APIENTRY * qglGetPixelMapuiv )(GLenum map, GLuint * RESTRICT values);
extern  void ( APIENTRY * qglGetPixelMapusv )(GLenum map, GLushort * RESTRICT values);
extern  void ( APIENTRY * qglGetPointerv )(GLenum pname, GLvoid* * RESTRICT params);
extern  void ( APIENTRY * qglGetPolygonStipple )(GLubyte * RESTRICT mask);
extern  const GLubyte * ( APIENTRY * qglGetString )(GLenum name);
extern  void ( APIENTRY * qglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetTexEnviv )(GLenum target, GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglGetTexGendv )(GLenum coord, GLenum pname, GLdouble * RESTRICT params);
extern  void ( APIENTRY * qglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetTexGeniv )(GLenum coord, GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglGetTexParameteriv )(GLenum target, GLenum pname, GLint * RESTRICT params);
extern  void ( APIENTRY * qglHint )(GLenum target, GLenum mode);
extern  void ( APIENTRY * qglIndexMask )(GLuint mask);
extern  void ( APIENTRY * qglIndexPointer )(GLenum type, GLsizei stride, const GLvoid * RESTRICT pointer);
extern  void ( APIENTRY * qglIndexd )(GLdouble c);
extern  void ( APIENTRY * qglIndexdv )(const GLdouble * RESTRICT c);
extern  void ( APIENTRY * qglIndexf )(GLfloat c);
extern  void ( APIENTRY * qglIndexfv )(const GLfloat * RESTRICT c);
extern  void ( APIENTRY * qglIndexi )(GLint c);
extern  void ( APIENTRY * qglIndexiv )(const GLint * RESTRICT c);
extern  void ( APIENTRY * qglIndexs )(GLshort c);
extern  void ( APIENTRY * qglIndexsv )(const GLshort * RESTRICT c);
extern  void ( APIENTRY * qglIndexub )(GLubyte c);
extern  void ( APIENTRY * qglIndexubv )(const GLubyte * RESTRICT c);
extern  void ( APIENTRY * qglInitNames )(void);
extern  void ( APIENTRY * qglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid * RESTRICT pointer);
extern  GLboolean ( APIENTRY * qglIsEnabled )(GLenum cap);
extern  GLboolean ( APIENTRY * qglIsList )(GLuint list);
extern  GLboolean ( APIENTRY * qglIsTexture )(GLuint texture);
extern  void ( APIENTRY * qglLightModelf )(GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglLightModelfv )(GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglLightModeli )(GLenum pname, GLint param);
extern  void ( APIENTRY * qglLightModeliv )(GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglLightf )(GLenum light, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglLightfv )(GLenum light, GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglLighti )(GLenum light, GLenum pname, GLint param);
extern  void ( APIENTRY * qglLightiv )(GLenum light, GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglLineStipple )(GLint factor, GLushort pattern);
extern  void ( APIENTRY * qglLineWidth )(GLfloat width);
extern  void ( APIENTRY * qglListBase )(GLuint base);
extern  void ( APIENTRY * qglLoadIdentity )(void);
extern  void ( APIENTRY * qglLoadMatrixd )(const GLdouble * RESTRICT m);
extern  void ( APIENTRY * qglLoadMatrixf )(const GLfloat * RESTRICT m);
extern  void ( APIENTRY * qglLoadName )(GLuint name);
extern  void ( APIENTRY * qglLogicOp )(GLenum opcode);
extern  void ( APIENTRY * qglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * RESTRICT points);
extern  void ( APIENTRY * qglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * RESTRICT points);
extern  void ( APIENTRY * qglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * RESTRICT points);
extern  void ( APIENTRY * qglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * RESTRICT points);
extern  void ( APIENTRY * qglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
extern  void ( APIENTRY * qglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
extern  void ( APIENTRY * qglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern  void ( APIENTRY * qglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern  void ( APIENTRY * qglMaterialf )(GLenum face, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglMaterialfv )(GLenum face, GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglMateriali )(GLenum face, GLenum pname, GLint param);
extern  void ( APIENTRY * qglMaterialiv )(GLenum face, GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglMatrixMode )(GLenum mode);
extern  void ( APIENTRY * qglMultMatrixd )(const GLdouble * RESTRICT m);
extern  void ( APIENTRY * qglMultMatrixf )(const GLfloat * RESTRICT m);
extern  void ( APIENTRY * qglNewList )(GLuint list, GLenum mode);
extern  void ( APIENTRY * qglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
extern  void ( APIENTRY * qglNormal3bv )(const GLbyte * RESTRICT v);
extern  void ( APIENTRY * qglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
extern  void ( APIENTRY * qglNormal3dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
extern  void ( APIENTRY * qglNormal3fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglNormal3i )(GLint nx, GLint ny, GLint nz);
extern  void ( APIENTRY * qglNormal3iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
extern  void ( APIENTRY * qglNormal3sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglNormalPointer )(GLenum type, GLsizei stride, const GLvoid * RESTRICT pointer);
extern  void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  void ( APIENTRY * qglPassThrough )(GLfloat token);
extern  void ( APIENTRY * qglPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat * RESTRICT values);
extern  void ( APIENTRY * qglPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint * RESTRICT values);
extern  void ( APIENTRY * qglPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort * RESTRICT values);
extern  void ( APIENTRY * qglPixelStoref )(GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglPixelStorei )(GLenum pname, GLint param);
extern  void ( APIENTRY * qglPixelTransferf )(GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglPixelTransferi )(GLenum pname, GLint param);
extern  void ( APIENTRY * qglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
extern  void ( APIENTRY * qglPointSize )(GLfloat size);
extern  void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
extern  void ( APIENTRY * qglPolygonOffset )(GLfloat factor, GLfloat units);
extern  void ( APIENTRY * qglPolygonStipple )(const GLubyte * RESTRICT mask);
extern  void ( APIENTRY * qglPopAttrib )(void);
extern  void ( APIENTRY * qglPopClientAttrib )(void);
extern  void ( APIENTRY * qglPopMatrix )(void);
extern  void ( APIENTRY * qglPopName )(void);
extern  void ( APIENTRY * qglPrioritizeTextures )(GLsizei n, const GLuint * RESTRICT textures, const GLclampf * RESTRICT priorities);
extern  void ( APIENTRY * qglPushAttrib )(GLbitfield mask);
extern  void ( APIENTRY * qglPushClientAttrib )(GLbitfield mask);
extern  void ( APIENTRY * qglPushMatrix )(void);
extern  void ( APIENTRY * qglPushName )(GLuint name);
extern  void ( APIENTRY * qglRasterPos2d )(GLdouble x, GLdouble y);
extern  void ( APIENTRY * qglRasterPos2dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos2f )(GLfloat x, GLfloat y);
extern  void ( APIENTRY * qglRasterPos2fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos2i )(GLint x, GLint y);
extern  void ( APIENTRY * qglRasterPos2iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos2s )(GLshort x, GLshort y);
extern  void ( APIENTRY * qglRasterPos2sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglRasterPos3dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglRasterPos3fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos3i )(GLint x, GLint y, GLint z);
extern  void ( APIENTRY * qglRasterPos3iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos3s )(GLshort x, GLshort y, GLshort z);
extern  void ( APIENTRY * qglRasterPos3sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern  void ( APIENTRY * qglRasterPos4dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern  void ( APIENTRY * qglRasterPos4fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
extern  void ( APIENTRY * qglRasterPos4iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern  void ( APIENTRY * qglRasterPos4sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglReadBuffer )(GLenum mode);
extern  void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern  void ( APIENTRY * qglRectdv )(const GLdouble * RESTRICT v1, const GLdouble * RESTRICT v2);
extern  void ( APIENTRY * qglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern  void ( APIENTRY * qglRectfv )(const GLfloat * RESTRICT v1, const GLfloat * RESTRICT v2);
extern  void ( APIENTRY * qglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
extern  void ( APIENTRY * qglRectiv )(const GLint * RESTRICT v1, const GLint * RESTRICT v2);
extern  void ( APIENTRY * qglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern  void ( APIENTRY * qglRectsv )(const GLshort * RESTRICT v1, const GLshort * RESTRICT v2);
extern  GLint ( APIENTRY * qglRenderMode )(GLenum mode);
extern  void ( APIENTRY * qglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglScaled )(GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( APIENTRY * qglSelectBuffer )(GLsizei size, GLuint * RESTRICT buffer);
extern  void ( APIENTRY * qglShadeModel )(GLenum mode);
extern  void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
extern  void ( APIENTRY * qglStencilMask )(GLuint mask);
extern  void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
extern  void ( APIENTRY * qglTexCoord1d )(GLdouble s);
extern  void ( APIENTRY * qglTexCoord1dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord1f )(GLfloat s);
extern  void ( APIENTRY * qglTexCoord1fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord1i )(GLint s);
extern  void ( APIENTRY * qglTexCoord1iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord1s )(GLshort s);
extern  void ( APIENTRY * qglTexCoord1sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord2d )(GLdouble s, GLdouble t);
extern  void ( APIENTRY * qglTexCoord2dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord2f )(GLfloat s, GLfloat t);
extern  void ( APIENTRY * qglTexCoord2fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord2i )(GLint s, GLint t);
extern  void ( APIENTRY * qglTexCoord2iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord2s )(GLshort s, GLshort t);
extern  void ( APIENTRY * qglTexCoord2sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
extern  void ( APIENTRY * qglTexCoord3dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
extern  void ( APIENTRY * qglTexCoord3fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord3i )(GLint s, GLint t, GLint r);
extern  void ( APIENTRY * qglTexCoord3iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord3s )(GLshort s, GLshort t, GLshort r);
extern  void ( APIENTRY * qglTexCoord3sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern  void ( APIENTRY * qglTexCoord4dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern  void ( APIENTRY * qglTexCoord4fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
extern  void ( APIENTRY * qglTexCoord4iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
extern  void ( APIENTRY * qglTexCoord4sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid * RESTRICT pointer);
extern  void ( APIENTRY * qglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglTexEnvfv )(GLenum target, GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
extern  void ( APIENTRY * qglTexEnviv )(GLenum target, GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglTexGend )(GLenum coord, GLenum pname, GLdouble param);
extern  void ( APIENTRY * qglTexGendv )(GLenum coord, GLenum pname, const GLdouble * RESTRICT params);
extern  void ( APIENTRY * qglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglTexGenfv )(GLenum coord, GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglTexGeni )(GLenum coord, GLenum pname, GLint param);
extern  void ( APIENTRY * qglTexGeniv )(GLenum coord, GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglTexParameterfv )(GLenum target, GLenum pname, const GLfloat * RESTRICT params);
extern  void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
extern  void ( APIENTRY * qglTexParameteriv )(GLenum target, GLenum pname, const GLint * RESTRICT params);
extern  void ( APIENTRY * qglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * RESTRICT pixels);
extern  void ( APIENTRY * qglTranslated )(GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglVertex2d )(GLdouble x, GLdouble y);
extern  void ( APIENTRY * qglVertex2dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglVertex2f )(GLfloat x, GLfloat y);
extern  void ( APIENTRY * qglVertex2fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglVertex2i )(GLint x, GLint y);
extern  void ( APIENTRY * qglVertex2iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglVertex2s )(GLshort x, GLshort y);
extern  void ( APIENTRY * qglVertex2sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglVertex3dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglVertex3fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglVertex3i )(GLint x, GLint y, GLint z);
extern  void ( APIENTRY * qglVertex3iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglVertex3s )(GLshort x, GLshort y, GLshort z);
extern  void ( APIENTRY * qglVertex3sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern  void ( APIENTRY * qglVertex4dv )(const GLdouble * RESTRICT v);
extern  void ( APIENTRY * qglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern  void ( APIENTRY * qglVertex4fv )(const GLfloat * RESTRICT v);
extern  void ( APIENTRY * qglVertex4i )(GLint x, GLint y, GLint z, GLint w);
extern  void ( APIENTRY * qglVertex4iv )(const GLint * RESTRICT v);
extern  void ( APIENTRY * qglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern  void ( APIENTRY * qglVertex4sv )(const GLshort * RESTRICT v);
extern  void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid * RESTRICT pointer);
extern  void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

// RAVEN BEGIN
// dluetscher: added some wgl calls to the XENON version, as well as the Windows version
#if defined( _WINDOWS ) || defined( _XENON )
typedef struct tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;
// RAVEN END

extern  int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR * RESTRICT );
extern  int   ( WINAPI * qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
extern  int   ( WINAPI * qwglGetPixelFormat)(HDC);
extern  BOOL  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR * RESTRICT );
extern  BOOL  ( WINAPI * qwglSwapBuffers)(HDC);

// RAVEN BEGIN
// dluetscher: added block of functions from WGL ARB pbuffer and render texture extensions
typedef struct HPBUFFERARB__ *HPBUFFERARB;
extern BOOL  ( WINAPI * qwglBindTexImageARB)(HPBUFFERARB, int);
extern BOOL	 ( WINAPI * qwglChoosePixelFormatARB)(HDC hdc, const int * RESTRICT piAttribIList, const FLOAT * RESTRICT pfAttribFList, UINT nMaxFormats, int * RESTRICT piFormats, UINT * RESTRICT nNumFormats);
extern HPBUFFERARB ( WINAPI * qwglCreatePbufferARB)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int * RESTRICT piAttribList);
extern BOOL  ( WINAPI * qwglDestroyPbufferARB)(HPBUFFERARB);
extern HDC	 ( WINAPI * qwglGetPbufferDCARB)(HPBUFFERARB);
extern int	 ( WINAPI * qwglReleasePbufferDCARB)(HPBUFFERARB, HDC);
extern BOOL  ( WINAPI * qwglReleaseTexImageARB)(HPBUFFERARB, int);
extern BOOL  ( WINAPI * qwglSetPbufferAttribARB)(HPBUFFERARB, const int * RESTRICT );
// RAVEN END

extern BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
extern HGLRC ( WINAPI * qwglCreateContext)(HDC);
extern HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
extern BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
extern HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
extern HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
extern PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
extern BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
extern BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);

// RAVEN BEGIN
// dluetscher: removed these wgl calls from the XENON version
#if !defined( _XENON )
// RAVEN END
extern BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

extern BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

extern BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
extern int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF * RESTRICT );
extern int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF * RESTRICT );
extern BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
extern BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);
#endif

#endif	// _WINDOWS

#if defined( __linux__ )

//GLX Functions
extern XVisualInfo * (*qglXChooseVisual)( Display * RESTRICT dpy, int screen, int * RESTRICT attribList );
extern GLXContext (*qglXCreateContext)( Display * RESTRICT dpy, XVisualInfo * RESTRICT vis, GLXContext shareList, Bool direct );
extern void (*qglXDestroyContext)( Display * RESTRICT dpy, GLXContext ctx );
extern Bool (*qglXMakeCurrent)( Display * RESTRICT dpy, GLXDrawable drawable, GLXContext ctx);
extern void (*qglXSwapBuffers)( Display * RESTRICT dpy, GLXDrawable drawable );
extern GLExtension_t (*qglXGetProcAddressARB)( const GLubyte * RESTRICT procname );
extern GLXContext (*qglXGetCurrentContext)( void );

// make sure the code is correctly using qgl everywhere
// don't enable that when building glimp itself obviously..
#if !defined( GLIMP )
	#include "../sys/linux/qgl_enforce.h"
#endif

#endif // __linux__

// RAVEN BEGIN
// mwhitlock: loading from .xbr resource bundles
#if defined( _XENON )
extern void ( APIENTRY * qglSetTexCacheDefault2DImageId ) (int id);
extern void ( APIENTRY * qglSetTexCacheDefaultCubeImageId ) (int id);
extern  GLboolean ( APIENTRY * qglTexImageExistsInBundles ) ( unsigned long texNameCRC32 );
extern  void ( APIENTRY * qglTexImageFromCache )(int id, unsigned long texNameCRC32 );

// nrausch: capture backbuffer to memory
extern void xglCapture( int width, int height, void * RESTRICT pixels );

// mwhitlock: allow us to directly use the results of resolving from the EDRAM rendertarget as texture.
extern void ( APIENTRY * qglTexImageFromFrontBuffer )( void );

#endif //_XENON
// RAVEN END

#endif	// hardlinlk vs dlopen

#endif
