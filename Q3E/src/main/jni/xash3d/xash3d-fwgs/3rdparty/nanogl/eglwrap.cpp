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

#include "egl.h"
#include "gl.h"
#include "glesinterface.h"

#include <string.h>

extern "C++" GlESInterface *glEsImpl;
extern "C++" void FlushOnStateChange( );
void APIENTRY gl_unimplemented( GLenum none );

EGLint eglGetError( void )
{
	return glEsImpl->eglGetError( );
}

EGLDisplay eglGetDisplay( NativeDisplayType display )
{
	return glEsImpl->eglGetDisplay( display );
}

EGLBoolean eglInitialize( EGLDisplay dpy, EGLint *major, EGLint *minor )
{
	return glEsImpl->eglInitialize( dpy, major, minor );
}

EGLBoolean eglTerminate( EGLDisplay dpy )
{
	return glEsImpl->eglTerminate( dpy );
}
const char *eglQueryString( EGLDisplay dpy, EGLint name )
{
	return glEsImpl->eglQueryString( dpy, name );
}

#if defined( __MULTITEXTURE_SUPPORT__ )
extern "C" void glMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t );
#endif

void *eglGetProcAddress( const char *procname )
{
#if defined( __MULTITEXTURE_SUPPORT__ )
	if ( !strcmp( procname, "glMultiTexCoord2fARB" ) )
	{
		return (void *)&GL_MANGLE_NAME(glMultiTexCoord2fARB);
	}
	else if ( !strcmp( procname, "glActiveTextureARB" ) )
	{
		return (void *)&GL_MANGLE_NAME(glActiveTexture);
	}
	else if ( !strcmp( procname, "glClientActiveTextureARB" ) )
	{
		return (void *)&GL_MANGLE_NAME(glClientActiveTexture);
	}

#endif
	return (void *)glEsImpl->eglGetProcAddress( procname );
}

EGLBoolean eglGetConfigs( EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config )
{
	return glEsImpl->eglGetConfigs( dpy, configs, config_size, num_config );
}

EGLBoolean eglChooseConfig( EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config )
{
	return glEsImpl->eglChooseConfig( dpy, attrib_list, configs, config_size, num_config );
}

EGLBoolean eglGetConfigAttrib( EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value )
{
	return glEsImpl->eglGetConfigAttrib( dpy, config, attribute, value );
}

EGLSurface eglCreateWindowSurface( EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list )
{
	return glEsImpl->eglCreateWindowSurface( dpy, config, window, attrib_list );
}

EGLSurface eglCreatePixmapSurface( EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list )
{
	return glEsImpl->eglCreatePixmapSurface( dpy, config, pixmap, attrib_list );
}

EGLSurface eglCreatePbufferSurface( EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list )
{
	return glEsImpl->eglCreatePbufferSurface( dpy, config, attrib_list );
}

EGLBoolean eglDestroySurface( EGLDisplay dpy, EGLSurface surface )
{
	return glEsImpl->eglDestroySurface( dpy, surface );
}
EGLBoolean eglQuerySurface( EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value )
{
	return glEsImpl->eglQuerySurface( dpy, surface, attribute, value );
}
/* EGL 1.1 render-to-texture APIs */
EGLBoolean eglSurfaceAttrib( EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value )
{
	return glEsImpl->eglSurfaceAttrib( dpy, surface, attribute, value );
}

EGLBoolean eglBindTexImage( EGLDisplay dpy, EGLSurface surface, EGLint buffer )
{
	return glEsImpl->eglBindTexImage( dpy, surface, buffer );
}
EGLBoolean eglReleaseTexImage( EGLDisplay dpy, EGLSurface surface, EGLint buffer )
{
	return glEsImpl->eglReleaseTexImage( dpy, surface, buffer );
}

/* EGL 1.1 swap control API */
EGLBoolean eglSwapInterval( EGLDisplay dpy, EGLint interval )
{
	return glEsImpl->eglSwapInterval( dpy, interval );
}

EGLContext eglCreateContext( EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list )
{
	return glEsImpl->eglCreateContext( dpy, config, share_list, attrib_list );
}

EGLBoolean eglDestroyContext( EGLDisplay dpy, EGLContext ctx )
{
	return glEsImpl->eglDestroyContext( dpy, ctx );
}

EGLBoolean eglMakeCurrent( EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx )
{
	FlushOnStateChange( );
	return glEsImpl->eglMakeCurrent( dpy, draw, read, ctx );
}

EGLContext eglGetCurrentContext( void )
{
	return glEsImpl->eglGetCurrentContext( );
}
EGLSurface eglGetCurrentSurface( EGLint readdraw )
{
	if ( (void *)glEsImpl->eglGetCurrentSurface == (void *)gl_unimplemented )
		return EGL_NO_SURFACE;
	return glEsImpl->eglGetCurrentSurface( readdraw );
}
EGLDisplay eglGetCurrentDisplay( void )
{
	if ( (void *)glEsImpl->eglGetCurrentDisplay == (void *)gl_unimplemented )
		return EGL_NO_DISPLAY;
	return glEsImpl->eglGetCurrentDisplay( );
}

EGLBoolean eglQueryContext( EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value )
{
	return glEsImpl->eglQueryContext( dpy, ctx, attribute, value );
}

EGLBoolean eglWaitGL( void )
{
	FlushOnStateChange( );
	return glEsImpl->eglWaitGL( );
}

EGLBoolean eglWaitNative( EGLint engine )
{
	FlushOnStateChange( );
	return glEsImpl->eglWaitNative( engine );
}

EGLBoolean eglSwapBuffers( EGLDisplay dpy, EGLSurface draw )
{
	FlushOnStateChange( );
	return glEsImpl->eglSwapBuffers( dpy, draw );
}

EGLBoolean eglCopyBuffers( EGLDisplay dpy, EGLSurface surface, NativePixmapType target )
{
	FlushOnStateChange( );
	return glEsImpl->eglCopyBuffers( dpy, surface, target );
}
