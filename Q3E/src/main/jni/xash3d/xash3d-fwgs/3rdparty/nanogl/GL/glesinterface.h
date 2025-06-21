/*
Copyright (C) 2007-2009 Olli Hinkka

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __GLESINTERFACE_H__
#define __GLESINTERFACE_H__

#if !defined( __WINS__ )
#if defined( __TARGET_FPU_VFP )
#pragma softfp_linkage
#endif
#endif

#ifdef SOFTFP_LINK
#define S __attribute__( ( pcs( "aapcs" ) ) )
#else
#define S
#endif

#ifdef _WIN32
#include <windows.h> //APIENTRY
#endif
#ifndef APIENTRY
#ifdef _MSC_VER
#define APIENTRY WINAPI
#else
#define APIENTRY
#endif
#endif

typedef void ( APIENTRY *GL_DEBUG_PROC_KHR )( unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* message, void* userParam );

struct GlESInterface
{
	int ( *eglChooseConfig )( int dpy, const int *attrib_list, int *configs, int config_size, int *num_config ) S;
	int ( *eglCopyBuffers )( int dpy, int surface, void *target ) S;
	int ( *eglCreateContext )( int dpy, int config, int share_list, const int *attrib_list ) S;
	int ( *eglCreatePbufferSurface )( int dpy, int config, const int *attrib_list ) S;
	int ( *eglCreatePixmapSurface )( int dpy, int config, void *pixmap, const int *attrib_list ) S;
	int ( *eglCreateWindowSurface )( int dpy, int config, void *window, const int *attrib_list ) S;
	int ( *eglDestroyContext )( int dpy, int ctx ) S;
	int ( *eglDestroySurface )( int dpy, int surface ) S;
	int ( *eglGetConfigAttrib )( int dpy, int config, int attribute, int *value ) S;
	int ( *eglGetConfigs )( int dpy, int *configs, int config_size, int *num_config ) S;
	int ( *eglGetCurrentContext )( void ) S;
	int ( *eglGetCurrentDisplay )( void ) S;
	int ( *eglGetCurrentSurface )( int readdraw ) S;
	int ( *eglGetDisplay )( int display ) S;
	int ( *eglGetError )( void ) S;

	void ( *( *eglGetProcAddress )( const char *procname ) )( ... ) S;

	int ( *eglInitialize )( int dpy, int *major, int *minor ) S;
	int ( *eglMakeCurrent )( int dpy, int draw, int read, int ctx ) S;
	int ( *eglQueryContext )( int dpy, int ctx, int attribute, int *value ) S;
	const char *( *eglQueryString )(int dpy, int name)S;
	int ( *eglQuerySurface )( int dpy, int surface, int attribute, int *value ) S;
	int ( *eglSwapBuffers )( int dpy, int draw ) S;
	int ( *eglTerminate )( int dpy ) S;
	int ( *eglWaitGL )( void ) S;
	int ( *eglWaitNative )( int engine ) S;
	void( APIENTRY *glActiveTexture )( unsigned int texture ) S;
	void( APIENTRY *glAlphaFunc )( unsigned int func, float ref ) S;
	void( APIENTRY *glAlphaFuncx )( unsigned int func, int ref ) S;
	void( APIENTRY *glBindTexture )( unsigned int target, unsigned int texture ) S;
	void( APIENTRY *glBlendFunc )( unsigned int sfactor, unsigned int dfactor ) S;
	void( APIENTRY *glClear )( unsigned int mask ) S;
	void( APIENTRY *glClearColor )( float red, float green, float blue, float alpha ) S;
	void( APIENTRY *glClearColorx )( int red, int green, int blue, int alpha ) S;
	void( APIENTRY *glClearDepthf )( float depth ) S;
	void( APIENTRY *glClearDepthx )( int depth ) S;
	void( APIENTRY *glClearStencil )( int s ) S;
	void( APIENTRY *glClientActiveTexture )( unsigned int texture ) S;
	void( APIENTRY *glColor4f )( float red, float green, float blue, float alpha ) S;
	void( APIENTRY *glColor4x )( int red, int green, int blue, int alpha ) S;
	void( APIENTRY *glColorMask )( unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha ) S;
	void( APIENTRY *glColorPointer )( int size, unsigned int type, int stride, const void *pointer ) S;
	void( APIENTRY *glCompressedTexImage2D )( unsigned int target, int level, unsigned int internalformat, int width, int height, int border, int imageSize, const void *data ) S;
	void( APIENTRY *glCompressedTexSubImage2D )( unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, int imageSize, const void *data ) S;
	void( APIENTRY *glCopyTexImage2D )( unsigned int target, int level, unsigned int internalformat, int x, int y, int width, int height, int border ) S;
	void( APIENTRY *glCopyTexSubImage2D )( unsigned int target, int level, int xoffset, int yoffset, int x, int y, int width, int height ) S;
	void( APIENTRY *glCullFace )( unsigned int mode ) S;
	void( APIENTRY *glDeleteTextures )( int n, const unsigned int *textures ) S;
	void( APIENTRY *glDepthFunc )( unsigned int func ) S;
	void( APIENTRY *glDepthMask )( unsigned char flag ) S;
	void( APIENTRY *glDepthRangef )( float zNear, float zFar ) S;
	void( APIENTRY *glDepthRangex )( int zNear, int zFar ) S;
	void( APIENTRY *glDisable )( unsigned int cap ) S;
	void( APIENTRY *glDisableClientState )( unsigned int array ) S;
	void( APIENTRY *glDrawArrays )( unsigned int mode, int first, int count ) S;
	void( APIENTRY *glDrawElements )( unsigned int mode, int count, unsigned int type, const void *indices ) S;
	void( APIENTRY *glEnable )( unsigned int cap ) S;
	void( APIENTRY *glEnableClientState )( unsigned int array ) S;
	void( APIENTRY *glFinish )( void ) S;
	void( APIENTRY *glFlush )( void ) S;
	void( APIENTRY *glFogf )( unsigned int pname, float param ) S;
	void( APIENTRY *glFogfv )( unsigned int pname, const float *params ) S;
	void( APIENTRY *glFogx )( unsigned int pname, int param ) S;
	void( APIENTRY *glFogxv )( unsigned int pname, const int *params ) S;
	void( APIENTRY *glFrontFace )( unsigned int mode ) S;
	void( APIENTRY *glFrustumf )( float left, float right, float bottom, float top, float zNear, float zFar ) S;
	void( APIENTRY *glFrustumx )( int left, int right, int bottom, int top, int zNear, int zFar ) S;
	void( APIENTRY *glGenTextures )( int n, unsigned int *textures ) S;
	unsigned int( APIENTRY *glGetError )( void ) S;
	void( APIENTRY *glGetIntegerv )( unsigned int pname, int *params ) S;
	const unsigned char *( APIENTRY *glGetString )(unsigned int name)S;
	void( APIENTRY *glHint )( unsigned int target, unsigned int mode ) S;
	void( APIENTRY *glLightModelf )( unsigned int pname, float param ) S;
	void( APIENTRY *glLightModelfv )( unsigned int pname, const float *params ) S;
	void( APIENTRY *glLightModelx )( unsigned int pname, int param ) S;
	void( APIENTRY *glLightModelxv )( unsigned int pname, const int *params ) S;
	void( APIENTRY *glLightf )( unsigned int light, unsigned int pname, float param ) S;
	void( APIENTRY *glLightfv )( unsigned int light, unsigned int pname, const float *params ) S;
	void( APIENTRY *glLightx )( unsigned int light, unsigned int pname, int param ) S;
	void( APIENTRY *glLightxv )( unsigned int light, unsigned int pname, const int *params ) S;
	void( APIENTRY *glLineWidth )( float width ) S;
	void( APIENTRY *glLineWidthx )( int width ) S;
	void( APIENTRY *glLoadIdentity )( void ) S;
	void( APIENTRY *glLoadMatrixf )( const float *m ) S;
	void( APIENTRY *glLoadMatrixx )( const int *m ) S;
	void( APIENTRY *glLogicOp )( unsigned int opcode ) S;
	void( APIENTRY *glMaterialf )( unsigned int face, unsigned int pname, float param ) S;
	void( APIENTRY *glMaterialfv )( unsigned int face, unsigned int pname, const float *params ) S;
	void( APIENTRY *glMaterialx )( unsigned int face, unsigned int pname, int param ) S;
	void( APIENTRY *glMaterialxv )( unsigned int face, unsigned int pname, const int *params ) S;
	void( APIENTRY *glMatrixMode )( unsigned int mode ) S;
	void( APIENTRY *glMultMatrixf )( const float *m ) S;
	void( APIENTRY *glMultMatrixx )( const int *m ) S;
	void( APIENTRY *glMultiTexCoord4f )( unsigned int target, float s, float t, float r, float q ) S;
	void( APIENTRY *glMultiTexCoord4x )( unsigned int target, int s, int t, int r, int q ) S;
	void( APIENTRY *glNormal3f )( float nx, float ny, float nz ) S;
	void( APIENTRY *glNormal3x )( int nx, int ny, int nz ) S;
	void( APIENTRY *glNormalPointer )( unsigned int type, int stride, const void *pointer ) S;
	void( APIENTRY *glOrthof )( float left, float right, float bottom, float top, float zNear, float zFar ) S;
	void( APIENTRY *glOrthox )( int left, int right, int bottom, int top, int zNear, int zFar ) S;
	void( APIENTRY *glPixelStorei )( unsigned int pname, int param ) S;
	void( APIENTRY *glPointSize )( float size ) S;
	void( APIENTRY *glPointSizex )( int size ) S;
	void( APIENTRY *glPolygonOffset )( float factor, float units ) S;
	void( APIENTRY *glPolygonOffsetx )( int factor, int units ) S;
	void( APIENTRY *glPopMatrix )( void ) S;
	void( APIENTRY *glPushMatrix )( void ) S;
	unsigned int( APIENTRY *glQueryMatrixxOES )( int mantissa[16], int exponent[16] ) S;
	void( APIENTRY *glReadPixels )( int x, int y, int width, int height, unsigned int format, unsigned int type, void *pixels ) S;
	void( APIENTRY *glRotatef )( float angle, float x, float y, float z ) S;
	void( APIENTRY *glRotatex )( int angle, int x, int y, int z ) S;
	void( APIENTRY *glSampleCoverage )( float value, unsigned char invert ) S;
	void( APIENTRY *glSampleCoveragex )( int value, unsigned char invert ) S;
	void( APIENTRY *glScalef )( float x, float y, float z ) S;
	void( APIENTRY *glScalex )( int x, int y, int z ) S;
	void( APIENTRY *glScissor )( int x, int y, int width, int height ) S;
	void( APIENTRY *glShadeModel )( unsigned int mode ) S;
	void( APIENTRY *glStencilFunc )( unsigned int func, int ref, unsigned int mask ) S;
	void( APIENTRY *glStencilMask )( unsigned int mask ) S;
	void( APIENTRY *glStencilOp )( unsigned int fail, unsigned int zfail, unsigned int zpass ) S;
	void( APIENTRY *glTexCoordPointer )( int size, unsigned int type, int stride, const void *pointer ) S;
	void( APIENTRY *glTexEnvf )( unsigned int target, unsigned int pname, float param ) S;
	void( APIENTRY *glTexEnvfv )( unsigned int target, unsigned int pname, const float *params ) S;
	void( APIENTRY *glTexEnvx )( unsigned int target, unsigned int pname, int param ) S;
	void( APIENTRY *glTexEnvxv )( unsigned int target, unsigned int pname, const int *params ) S;
	void( APIENTRY *glTexImage2D )( unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void *pixels ) S;
	void( APIENTRY *glTexParameterf )( unsigned int target, unsigned int pname, float param ) S;
	void( APIENTRY *glTexParameterx )( unsigned int target, unsigned int pname, int param ) S;
	void( APIENTRY *glTexSubImage2D )( unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *pixels ) S;
	void( APIENTRY *glTranslatef )( float x, float y, float z ) S;
	void( APIENTRY *glTranslatex )( int x, int y, int z ) S;
	void( APIENTRY *glVertexPointer )( int size, unsigned int type, int stride, const void *pointer ) S;
	void( APIENTRY *glViewport )( int x, int y, int width, int height ) S;
	int ( *eglSwapInterval )( int dpy, int interval ) S;
	void( APIENTRY *glBindBuffer )( unsigned int target, unsigned int buffer ) S;
	void( APIENTRY *glBufferData )( unsigned int target, int size, const void *data, unsigned int usage ) S;
	void( APIENTRY *glBufferSubData )( unsigned int target, int offset, int size, const void *data ) S;
	void( APIENTRY *glClipPlanef )( unsigned int plane, const float *equation ) S;
	void( APIENTRY *glClipPlanex )( unsigned int plane, const int *equation ) S;
	void( APIENTRY *glColor4ub )( unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha ) S;
	void( APIENTRY *glDeleteBuffers )( int n, const unsigned int *buffers ) S;
	void( APIENTRY *glGenBuffers )( int n, unsigned int *buffers ) S;
	void( APIENTRY *glGetBooleanv )( unsigned int pname, unsigned char *params ) S;
	void( APIENTRY *glGetBufferParameteriv )( unsigned int target, unsigned int pname, int *params ) S;
	void( APIENTRY *glGetClipPlanef )( unsigned int pname, float eqn[4] ) S;
	void( APIENTRY *glGetClipPlanex )( unsigned int pname, int eqn[4] ) S;
	void( APIENTRY *glGetFixedv )( unsigned int pname, int *params ) S;
	void( APIENTRY *glGetFloatv )( unsigned int pname, float *params ) S;
	void( APIENTRY *glGetLightfv )( unsigned int light, unsigned int pname, float *params ) S;
	void( APIENTRY *glGetLightxv )( unsigned int light, unsigned int pname, int *params ) S;
	void( APIENTRY *glGetMaterialfv )( unsigned int face, unsigned int pname, float *params ) S;
	void( APIENTRY *glGetMaterialxv )( unsigned int face, unsigned int pname, int *params ) S;
	void( APIENTRY *glGetPointerv )( unsigned int pname, void **params ) S;
	void( APIENTRY *glGetTexEnvfv )( unsigned int env, unsigned int pname, float *params ) S;
	void( APIENTRY *glGetTexEnviv )( unsigned int env, unsigned int pname, int *params ) S;
	void( APIENTRY *glGetTexEnvxv )( unsigned int env, unsigned int pname, int *params ) S;
	void( APIENTRY *glGetTexParameterfv )( unsigned int target, unsigned int pname, float *params ) S;
	void( APIENTRY *glGetTexParameteriv )( unsigned int target, unsigned int pname, int *params ) S;
	void( APIENTRY *glGetTexParameterxv )( unsigned int target, unsigned int pname, int *params ) S;
	unsigned char( APIENTRY *glIsBuffer )( unsigned int buffer ) S;
	unsigned char( APIENTRY *glIsEnabled )( unsigned int cap ) S;
	unsigned char( APIENTRY *glIsTexture )( unsigned int texture ) S;
	void( APIENTRY *glPointParameterf )( unsigned int pname, float param ) S;
	void( APIENTRY *glPointParameterfv )( unsigned int pname, const float *params ) S;
	void( APIENTRY *glPointParameterx )( unsigned int pname, int param ) S;
	void( APIENTRY *glPointParameterxv )( unsigned int pname, const int *params ) S;
	void( APIENTRY *glPointSizePointerOES )( unsigned int type, int stride, const void *pointer ) S;
	void( APIENTRY *glTexEnvi )( unsigned int target, unsigned int pname, int param ) S;
	void( APIENTRY *glTexEnviv )( unsigned int target, unsigned int pname, const int *params ) S;
	void( APIENTRY *glTexParameterfv )( unsigned int target, unsigned int pname, const float *params ) S;
	void( APIENTRY *glTexParameteri )( unsigned int target, unsigned int pname, int param ) S;
	void( APIENTRY *glTexParameteriv )( unsigned int target, unsigned int pname, const int *params ) S;
	void( APIENTRY *glTexParameterxv )( unsigned int target, unsigned int pname, const int *params ) S;

	int ( *eglBindTexImage )( int dpy, int surface, int buffer ) S;
	int ( *eglReleaseTexImage )( int dpy, int surface, int buffer ) S;
	int ( *eglSurfaceAttrib )( int dpy, int surface, int attribute, int value ) S;

#ifdef USE_CORE_PROFILE
	void( APIENTRY *glOrtho )( double left, double right, double bottom, double top, double zNear, double zFar ) S;
	void( APIENTRY *glDepthRange )( double zNear, double zFar ) S;
#endif

	// Rikku2000: Light
	void( APIENTRY *glColorMaterial )( unsigned int face, unsigned int mode ) S;

	void( APIENTRY *glGenFramebuffers )( unsigned int n, unsigned int *framebuffers ) S;
	void( APIENTRY *glGenRenderbuffers )( unsigned int n, unsigned int *renderbuffers ) S;
	void( APIENTRY *glRenderbufferStorage )( unsigned int target, unsigned int internalformat, unsigned int width, unsigned int height ) S;
	void( APIENTRY *glBindFramebuffer )( unsigned int target, unsigned int framebuffer ) S;
	void( APIENTRY *glBindRenderbuffer )( unsigned int target, unsigned int renderbuffer ) S;
	void( APIENTRY *glFramebufferTexture2D )( unsigned int target, unsigned int attachment, unsigned int textarget, unsigned int texture, int level ) S;
	void( APIENTRY *glDeleteRenderbuffers )( unsigned int n, const unsigned int *renderbuffers ) S;
	void( APIENTRY *glDeleteFramebuffers )( unsigned int n, const unsigned int *framebuffers ) S;
	void( APIENTRY *glFramebufferRenderbuffer )( unsigned int target, unsigned int attachment, unsigned int renderbuffertarget, unsigned int renderbuffer ) S;
	
// GL_KHR_debug
	void ( APIENTRY *glDebugMessageControlKHR )( unsigned int source, unsigned int type, unsigned int severity, int count, const unsigned int* ids, unsigned char enabled ) S;
	void ( APIENTRY *glDebugMessageInsertKHR )( unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char* buf ) S;
	void ( APIENTRY *glDebugMessageCallbackKHR )( GL_DEBUG_PROC_KHR callback, void* userParam ) S;
	unsigned int ( APIENTRY *glGetDebugMessageLogKHR )( unsigned int count, int bufsize, unsigned int* sources, unsigned int* types, unsigned int* ids, unsigned int* severities, int* lengths, char* messageLog ) S;

	void( APIENTRY *glTexGenfvOES )(unsigned int coord, unsigned int pname, const float *params) S;
	void( APIENTRY *glTexGeniOES )(unsigned int coord, unsigned int pname, int param) S;
};
#if !defined( __WINS__ )
#if defined( __TARGET_FPU_VFP )
#pragma no_softfp_linkage
#endif
#endif

#endif
