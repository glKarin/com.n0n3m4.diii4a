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

#define LOG_TAG "nanoGL"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <cutils/log.h>

#include "nanogl.h"
#include "glesinterface.h"
#include "gl.h"

#define DEBUG_NANO 0

#ifdef __ANDROID__
#include <android/log.h>
#define LOG __android_log_print

#define LOGI( ... ) __android_log_print( ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__ )
#define LOGD( ... )   \
	if ( DEBUG_NANO ) \
	__android_log_print( ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__ )
#define LOGE( ... ) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )
#define LOGW( ... ) __android_log_print( ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__ )
#else
#ifndef _MSC_VER
#define LOGI( ... )             \
	printf( "I: " __VA_ARGS__ ); \
	printf( "\n" )
#define LOGD( ... )                 \
	if ( DEBUG_NANO )               \
	{                               \
		printf( "D: " __VA_ARGS__ ); \
		printf( "\n" );             \
	}
#define LOGE( ... )             \
	printf( "E: " __VA_ARGS__ ); \
	printf( "\n" )
#define LOGW( ... )             \
	printf( "W: " __VA_ARGS__ ); \
	printf( "\n" )
#else
#define LOGI printf
#define LOGD printf
#define LOGE printf
#define LOGW printf

#endif
#endif

#ifdef _WIN32
#include <windows.h>
#define loadDriver( x ) LoadLibraryA( x )
#define procAddress( x, y ) (( void * ) GetProcAddress( (HINSTANCE)x, y ))
#define freeDriver( x ) FreeLibrary( (HINSTANCE)x )
#define GL_LIB "opengl32.dll"
#define GLES_LIB "GLESv1_CM.dll"
#define EGL_LIB "EGL.dll"
#else
#include <dlfcn.h>
#define loadDriver( x ) dlopen( x, RTLD_NOW | RTLD_LOCAL )
#define procAddress( x, y ) dlsym( x, y )
#define freeDriver( x ) dlclose() x )
#define GL_LIB "libGL.so.1"
#define GLES_LIB "libGLESv1_CM.so"
#define EGL_LIB "libEGL.so"
#endif

//#define GL_ENTRY(_r, _api, ...) #_api,

static char const *const gl_names[] = {
#include "funcnames.h"
    NULL};

//const char * driver;

static void *glesLib = NULL;

GlESInterface *glEsImpl = NULL;

extern void InitGLStructs( );
#ifdef WIN32
void APIENTRY gl_unimplemented( GLenum none )
{
#ifndef USE_CORE_PROFILE
	LOGE( "Called unimplemented OpenGL ES API\n" );
#endif
}
#else // make glGetString not crash
const char * APIENTRY gl_unimplemented( GLenum none )
{
#ifndef USE_CORE_PROFILE
	LOGE( "Called unimplemented OpenGL ES API\n" );
#endif
	return "";
}
#endif
#ifdef REF_DLL
#define NANOGL_XASH
extern "C" void *GL_GetProcAddress( const char *name );
#elif defined XASH_SDL
#define NANOGL_SDL
#include "SDL.h"
#elif __ANDROID__
#define NANOGL_EGL
#endif

#ifdef NANOGL_EGL
void *eglLib;
#endif

void *nanoGL_GetProcAddress( const char *name )
{
	void *addr = NULL;
#ifdef NANOGL_XASH
	addr = GL_GetProcAddress( name );
#elif defined NANOGL_SDL
	addr = SDL_GL_GetProcAddress( name );
#endif
	if ( !addr )
		addr = procAddress( glesLib, name );
#ifdef NANOGL_EGL
	if( !addr )
		addr = procAddress( eglLib, name );
	if( !addr && glEsImpl->eglGetProcAddress )
		addr = (void *)glEsImpl->eglGetProcAddress( name );
#endif
	return addr;
}

#if 1
int nanoGL_Init( void)
{
	// load GL API calls
	char const *const *api;
	api = gl_names;
	int count = 0;

	// alloc space
	if ( !glEsImpl )
		glEsImpl = (GlESInterface *)malloc( sizeof( GlESInterface ) );
	memset( glEsImpl, 0, sizeof( GlESInterface ) );

#ifdef NANOGL_EGL
	eglLib = loadDriver( EGL_LIB );
#endif
#ifdef USE_CORE_PROFILE
	glesLib = loadDriver( GL_LIB );
#else
	glesLib = loadDriver( GLES_LIB );
#endif

	// nanoGL interface pointer
	void **ptr = (void **)( glEsImpl );

	while ( *api )
	{
		void *f;

		f = nanoGL_GetProcAddress( *api);

	#ifdef USE_CORE_PROFILE
		// Hack: try ARB and EXT suffix
		if ( f == NULL )
		{
			char namearb[256];
			snprintf( namearb, 256, "%sARB", *api );
			f = nanoGL_GetProcAddress( namearb );
		}
		if ( f == NULL )
		{
			char namearb[256];
			snprintf( namearb, 256, "%sEXT", *api );
			f = nanoGL_GetProcAddress( namearb );
		}
	#endif
		if ( f == NULL )
		{
			LOGW( "<%s> not found.", *api);
			f = (void*)gl_unimplemented;
		}
		else
		{
			LOGD( "<%s> @ 0x%p\n", *api, f );
			count++;
		}

		*ptr++ = f;
		api++;
	}

	InitGLStructs();

	// it has loaded something, maybe it will work
	if( count > 10 )
		return 1;
	else
		return 0;
}
#else

static int CreateGlEsInterface( const char *name, void *lib, void *lib1, void *default_func )
{
	// alloc space
	if ( !glEsImpl )
		glEsImpl = (GlESInterface *)malloc( sizeof( GlESInterface ) );

	if ( !glEsImpl )
	{
		return 0;
	}

	// load GL API calls
	char const *const *api;
	api = gl_names;

	// nanoGL interface pointer
	void **ptr = (void **)( glEsImpl );

	while ( *api )
	{
		void *f;

		f = dlsym( lib, *api ); // try libGLESxx_CM.so

#ifdef USE_CORE_PROFILE
		// Hack: try ARB and EXT suffix
		if ( f == NULL )
		{
			char namearb[256];
			snprintf( namearb, 256, "%sARB", *api );
			f = dlsym( lib, namearb );
		}
		if ( f == NULL )
		{
			char namearb[256];
			snprintf( namearb, 256, "%sEXT", *api );
			f = dlsym( lib, namearb );
		}
#endif
		if ( f == NULL )
		{
			LOGW( "<%s> not found in %s. Trying libEGL.so.", *api, name ); //driver);

			// try lib1
			if ( lib1 )
			{
				f = dlsym( lib1, *api ); // libEGL.so

				if ( f == NULL )
				{
					LOGE( "<%s> not found in libEGL.so", *api );
					if ( glEsImpl->eglGetProcAddress && ( (void *)glEsImpl->eglGetProcAddress != (void *)gl_unimplemented ) )
						f = (void *)glEsImpl->eglGetProcAddress( *api );
					if ( f == NULL )
						f = (void *)default_func; //(void*)gl_unimplemented;
				}
				else
				{
					LOGD( "<%s> @ 0x%p\n", *api, f );
				}
			}
			else
			{
				LOGE( "libEGL.so not loaded!" );
				if ( glEsImpl->eglGetProcAddress && ( (void *)glEsImpl->eglGetProcAddress != (void *)gl_unimplemented ) )
					f = (void *)glEsImpl->eglGetProcAddress( *api );
				if ( !f )
					f = (void *)default_func;
			}
		}
		else
		{
			LOGD( "<%s> @ 0x%p\n", *api, f );
		}

		*ptr++ = f;
		api++;
	}

	return 1;
}

// Load using the dynamic loader
static int loadDriver( const char *name )
{
	glesLib = dlopen( name, RTLD_NOW | RTLD_LOCAL );
	int rc  = ( glesLib ) ? 1 : 0;
	return rc;
}

/**
 * Init
 */
#ifdef _WIN32
int nanoGL_Init( )
{
	const char *lib1 = "opengl32.dll"; // Has both gl* & egl* funcs SDK < 1.5
	const char *lib2 = "opengl32.dll"; // Only gl* funcs SDK >= 1.5
	const char *lib3 = "opengl32.dll"; // Only egl* funcs SDK >= 1.5
	const char *driver;

	// load lib
	LOGI( "nanoGL: Init loading driver %s\n", lib1 );
	//LOG (ANDROID_LOG_DEBUG, LOG_TAG, "nanoGL: Init loading driver %s\n", lib1);

	if ( !loadDriver( lib1 ) )
	{
		LOGE( "Failed to load driver %s. Trying %s\n", lib1, lib2 );

		if ( !loadDriver( lib2 ) )
		{
			LOGE( "Failed to load  %s.\n", lib2 );
			return 0;
		}
		else
			driver = lib2;
	}
	else
		driver = lib1;

	void *eglLib;

	//if ( strcmp(driver, lib2) == 0 ) {
	LOGD( "**** Will Load EGL subs from %s ****", lib3 );

	eglLib = dlopen( lib3, RTLD_NOW | RTLD_LOCAL );

	if ( !eglLib )
	{
		LOGE( "Failed to load %s", lib3 );
	}
	//}

	// Load API gl* for 1.5+  else egl* gl*
	//if (CreateGlEsInterface(driver, glesLib, eglLib, NULL) == -1)
	if ( !CreateGlEsInterface( driver, glesLib, eglLib, (void *)gl_unimplemented ) == -1 )
	{
		// release lib
		LOGE( "CreateGlEsInterface failed." );

		dlclose( glesLib );
		return 0;
	}

	// Init nanoGL
	InitGLStructs( );
	return 1;
}
#else
int nanoGL_Init( )
{
	const char *lib1 = "libGLESv1_CM.so"; // Has both gl* & egl* funcs SDK < 1.5
	const char *lib2 = "libGLESv2.so";    // Only gl* funcs SDK >= 1.5
	const char *lib3 = "libEGL.so";       // Only egl* funcs SDK >= 1.5
	const char *driver;

	// load lib
	LOGI( "nanoGL: Init loading driver %s\n", lib1 );
	//LOG (ANDROID_LOG_DEBUG, LOG_TAG, "nanoGL: Init loading driver %s\n", lib1);

	if ( !loadDriver( lib1 ) )
	{
		LOGE( "Failed to load driver %s. Trying %s\n", lib1, lib2 );

		if ( !loadDriver( lib2 ) )
		{
			LOGE( "Failed to load  %s.\n", lib2 );
			return 0;
		}
		else
			driver = lib2;
	}
	else
		driver = lib1;

	void *eglLib;

	//if ( strcmp(driver, lib2) == 0 ) {
	LOGD( "**** Will Load EGL subs from %s ****", lib3 );

	eglLib = dlopen( lib3, RTLD_NOW | RTLD_LOCAL );

	if ( !eglLib )
	{
		LOGE( "Failed to load %s", lib3 );
	}
	//}

	// Load API gl* for 1.5+  else egl* gl*
	//if (CreateGlEsInterface(driver, glesLib, eglLib, NULL) == -1)
	if ( !CreateGlEsInterface( driver, glesLib, eglLib, (void *)gl_unimplemented ) == -1 )
	{
		// release lib
		LOGE( "CreateGlEsInterface failed." );

		dlclose( glesLib );
		return 0;
	}

#ifdef __ANDROID__
	// somewhy it does not initialize correctly
	*( (void **)&glEsImpl->glGenFramebuffers ) = (void *)glEsImpl->eglGetProcAddress( "glGenFramebuffersOES" );
	*( (void **)&glEsImpl->glGenRenderbuffers ) = (void *)glEsImpl->eglGetProcAddress( "glGenRenderbuffersOES" );
	*( (void **)&glEsImpl->glRenderbufferStorage ) = (void *)glEsImpl->eglGetProcAddress( "glRenderbufferStorageOES" );
	*( (void **)&glEsImpl->glBindFramebuffer ) = (void *)glEsImpl->eglGetProcAddress( "glBindFramebufferOES" );
	*( (void **)&glEsImpl->glBindRenderbuffer ) = (void *)glEsImpl->eglGetProcAddress( "glBindRenderbufferOES" );
	*( (void **)&glEsImpl->glFramebufferTexture2D ) = (void *)glEsImpl->eglGetProcAddress( "glFramebufferTexture2DOES" );
	*( (void **)&glEsImpl->glDeleteRenderbuffers ) = (void *)glEsImpl->eglGetProcAddress( "glDeleteRenderbuffersOES" );
	*( (void **)&glEsImpl->glDeleteFramebuffers ) = (void *)glEsImpl->eglGetProcAddress( "glDeleteFramebuffersOES" );
	*( (void **)&glEsImpl->glFramebufferRenderbuffer ) = (void *)glEsImpl->eglGetProcAddress( "glFramebufferRenderbufferOES" );
	*( (void **)&glEsImpl->glTexGeniOES ) = (void *)glEsImpl->eglGetProcAddress( "glTexGeniOES" );
	*( (void **)&glEsImpl->glTexGenfvOES ) = (void *)glEsImpl->eglGetProcAddress( "glTexGenfv" );


#endif

	// Init nanoGL
	InitGLStructs( );
	return 1;
}
#endif

#endif
void nanoGL_Destroy( )
{
	LOGD( "nanoGL_Destroy" );

	if ( glEsImpl )
	{
		free( glEsImpl );
		glEsImpl = NULL;
	}

	// release lib
	dlclose( glesLib );
}
