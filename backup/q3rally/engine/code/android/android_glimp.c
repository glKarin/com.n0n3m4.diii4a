/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../client/client.h"
#define _GLES_DISABLE_COMPAT
#include "../renderergl1/tr_local.h"



#define Q3E_PRINTF Com_Printf
#define Q3E_ERRORF(...) Com_Error(ERR_FATAL, __VA_ARGS__)
#define Q3E_DEBUGF(...) fprintf(stderr, __VA_ARGS__)
#define Q3Ebool qboolean
#define Q3E_TRUE qtrue
#define Q3E_FALSE qfalse
#define Q3E_POST_GL_INIT GLES_PostInit();

static void GLES_PostInit(void);

#include "q3e/q3e_glimp.inc"

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(Q3E_NoDisplay())
		return;

	if(Q3E_RequireWindow(w))
		Q3E_RestoreGL();
}

void GLimp_AndroidQuit(void)
{
	Q3E_DestroyGL(qtrue);
}

qboolean GLimp_InitGL(qboolean fullscreen)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_SET(swap_interval, r_swapInterval->integer);
	Q3E_GL_CONFIG_ES_1_1();

	qboolean res = Q3E_InitGL();
	return res;
}

void GLES_PostInit(void)
{
	glConfig.colorBits = Q3E_GL_CONFIG(red) + Q3E_GL_CONFIG(green) + Q3E_GL_CONFIG(blue);
	glConfig.stencilBits = Q3E_GL_CONFIG(stencil);
	glConfig.depthBits = Q3E_GL_CONFIG(depth);

	glConfig.isFullscreen = Q3E_GL_CONFIG(fullscreen);

	if (glConfig.isFullscreen) {
		//Sys_GrabMouseCursor(true);
	}
}



typedef GLdouble GLfloat;

void myglMultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
	qglMultiTexCoord4f(texture, s, t, 0, 1);
}

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;

int qglMajorVersion, qglMinorVersion;
int qglesMajorVersion, qglesMinorVersion;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

#define GLE(ret, name, ...) name##proc * qgl##name = NULL;
QGL_1_1_PROCS;
QGL_1_1_FIXED_FUNCTION_PROCS;
QGL_DESKTOP_1_1_PROCS;
#if !defined(USE_OPENGLES)
QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
#endif
QGL_ES_1_1_PROCS;
QGL_ES_1_1_FIXED_FUNCTION_PROCS;
QGL_1_3_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
QGL_3_0_PROCS;
QGL_ARB_occlusion_query_PROCS;
QGL_ARB_framebuffer_object_PROCS;
QGL_ARB_vertex_array_object_PROCS;
QGL_EXT_direct_state_access_PROCS;
#undef GLE

/*
===============
OpenGL ES compatibility
===============
*/
static void APIENTRY GLimp_GLES_ClearDepth( GLclampd depth ) {
	qglClearDepthf( depth );
}

static void APIENTRY GLimp_GLES_DepthRange( GLclampd near_val, GLclampd far_val ) {
	qglDepthRangef( near_val, far_val );
}

static void APIENTRY GLimp_GLES_DrawBuffer( GLenum mode ) {
	// unsupported
}

static void APIENTRY GLimp_GLES_PolygonMode( GLenum face, GLenum mode ) {
	// unsupported
}

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();

	Q3E_ShutdownGL();
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize( void )
{
}


/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( char *comment )
{
}

/*
===============
GLimp_GetProcAddresses

Get addresses for OpenGL functions.
===============
*/
static qboolean GLimp_GetProcAddresses( qboolean fixedFunction ) {
	qboolean success = qtrue;
	const char *version;

#define GLE( ret, name, ... ) qgl##name = (name##proc *) Q3E_GET_PROC_ADDRESS("gl" #name); \
	if ( qgl##name == NULL ) { \
		ri.Printf( PRINT_ALL, "ERROR: Missing OpenGL function %s\n", "gl" #name ); \
		success = qfalse; \
	}

	// OpenGL 1.0 and OpenGL ES 1.0
	GLE(const GLubyte *, GetString, GLenum name)

	if ( !qglGetString ) {
		Com_Error( ERR_FATAL, "glGetString is NULL" );
	}

	version = (const char *)qglGetString( GL_VERSION );

	if ( !version ) {
		Com_Error( ERR_FATAL, "GL_VERSION is NULL" );
	}

	if ( Q_stricmpn( "OpenGL ES", version, 9 ) == 0 ) {
		char profile[6]; // ES, ES-CM, or ES-CL
		sscanf( version, "OpenGL %5s %d.%d", profile, &qglesMajorVersion, &qglesMinorVersion );
		// common lite profile (no floating point) is not supported
		if ( Q_stricmp( profile, "ES-CL" ) == 0 ) {
			qglesMajorVersion = 0;
			qglesMinorVersion = 0;
		}
	} else {
		sscanf( version, "%d.%d", &qglMajorVersion, &qglMinorVersion );
	}

	if ( fixedFunction ) {
		if ( QGL_VERSION_ATLEAST( 1, 1 ) ) {
			QGL_1_1_PROCS;
			QGL_1_1_FIXED_FUNCTION_PROCS;
#if !defined(USE_OPENGLES)
			QGL_DESKTOP_1_1_PROCS;
			QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
#else
			qglClearDepth = GLimp_GLES_ClearDepth;
			qglDepthRange = GLimp_GLES_DepthRange;
			qglDrawBuffer = GLimp_GLES_DrawBuffer;
			qglPolygonMode = GLimp_GLES_PolygonMode;
#endif
		} else if ( qglesMajorVersion == 1 && qglesMinorVersion >= 1 ) {
			// OpenGL ES 1.1 (2.0 is not backward compatible)
			QGL_1_1_PROCS;
			QGL_1_1_FIXED_FUNCTION_PROCS;
			QGL_ES_1_1_PROCS;
			QGL_ES_1_1_FIXED_FUNCTION_PROCS;
			
			qglClearDepth = GLimp_GLES_ClearDepth;
			qglDepthRange = GLimp_GLES_DepthRange;
			qglDrawBuffer = GLimp_GLES_DrawBuffer;
			qglPolygonMode = GLimp_GLES_PolygonMode;

		} else {
			Com_Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 1.1 is required", version );
		}
	} else {
		if ( QGL_VERSION_ATLEAST( 2, 0 ) ) {
			QGL_1_1_PROCS;
#if !defined(USE_OPENGLES)
			QGL_DESKTOP_1_1_PROCS;
#endif
			QGL_1_3_PROCS;
			QGL_1_5_PROCS;
			QGL_2_0_PROCS;
		} else if ( QGLES_VERSION_ATLEAST( 2, 0 ) ) {
			QGL_1_1_PROCS;
			QGL_ES_1_1_PROCS;
			QGL_1_3_PROCS;
			QGL_1_5_PROCS;
			QGL_2_0_PROCS;
			// error so this doesn't segfault due to NULL desktop GL functions being used
			Com_Error( ERR_FATAL, "Unsupported OpenGL Version: %s", version );
		} else {
			Com_Error( ERR_FATAL, "Unsupported OpenGL Version (%s), OpenGL 2.0 is required", version );
		}
	}

	if ( QGL_VERSION_ATLEAST( 3, 0 ) || QGLES_VERSION_ATLEAST( 3, 0 ) ) {
		QGL_3_0_PROCS;
	}

#undef GLE

	return success;
}

/*
===============
GLimp_ClearProcAddresses

Clear addresses for OpenGL functions.
===============
*/
static void GLimp_ClearProcAddresses( void ) {
#define GLE( ret, name, ... ) qgl##name = NULL;

	qglMajorVersion = 0;
	qglMinorVersion = 0;
	qglesMajorVersion = 0;
	qglesMinorVersion = 0;

	QGL_1_1_PROCS;
	QGL_1_1_FIXED_FUNCTION_PROCS;
#if !defined(USE_OPENGLES)
	QGL_DESKTOP_1_1_PROCS;
	QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
#endif
	QGL_ES_1_1_PROCS;
	QGL_ES_1_1_FIXED_FUNCTION_PROCS;
	QGL_1_3_PROCS;
	QGL_1_5_PROCS;
	QGL_2_0_PROCS;
	QGL_3_0_PROCS;
	QGL_ARB_occlusion_query_PROCS;
	QGL_ARB_framebuffer_object_PROCS;
	QGL_ARB_vertex_array_object_PROCS;
	QGL_EXT_direct_state_access_PROCS;

	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	qglMultiTexCoord2fARB = NULL;

	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;

#undef GLE
}

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder, qboolean fixedFunction)
{
	const char *glstring;

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");

	ri.Printf (PRINT_ALL, "...setting mode %d:", mode );
	// use desktop video resolution
	glConfig.vidWidth = screen_width;
	glConfig.vidHeight = screen_height;
	glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;

	Q3E_GL_CONFIG_SET(samples, r_ext_multisample->value);

	GLimp_InitGL(qtrue);

	if( fullscreen )
	{
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		glConfig.isFullscreen = qfalse;
	}
	glConfig.isFullscreen = qtrue;
    ri.Cvar_Set( "r_fullscreen", "1" );
    glConfig.displayFrequency = refresh_rate;
    Cvar_SetValue( "r_customwidth", screen_width );
    Cvar_SetValue( "r_customheight", screen_height );
    Cvar_Set( "r_mode", "-1" );
	Cvar_SetValue( "r_customPixelAspect", glConfig.windowAspect );

	GLimp_GetProcAddresses( fixedFunction );

	qglClearColor( 0, 0, 0, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );
	Q3E_SwapBuffers();

	ri.Printf( PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
			   glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );

	glstring = (char *) qglGetString (GL_RENDERER);
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder, qboolean gl3Core)
{
	rserr_t err;

	if (fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}
	
	err = GLimp_SetMode(mode, fullscreen, noborder, gl3Core);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;
		case RSERR_INVALID_MODE:
			ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
			return qfalse;
		default:
			break;
	}

	return qtrue;
}

/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( qboolean fixedFunction )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// GL_EXT_texture_compression_s3tc
	if ( Q3E_GL_ExtensionSupported( "GL_ARB_texture_compression" ) &&
	     Q3E_GL_ExtensionSupported( "GL_EXT_texture_compression_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC_ARB;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
	}

	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE)
	{
		if ( Q3E_GL_ExtensionSupported( "GL_S3_s3tc" ) )
		{
			if ( r_ext_compressed_textures->value )
			{
				glConfig.textureCompression = TC_S3TC;
				ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
		}
	}

	// OpenGL 1 fixed function pipeline
	if ( fixedFunction )
	{
		// GL_EXT_texture_env_add
#ifdef USE_OPENGLES
		glConfig.textureEnvAddAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
#else
		glConfig.textureEnvAddAvailable = qfalse;
		if ( Q3E_GL_ExtensionSupported( "GL_EXT_texture_env_add" ) )
		{
			if ( r_ext_texture_env_add->integer )
			{
				glConfig.textureEnvAddAvailable = qtrue;
				ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
			}
			else
			{
				glConfig.textureEnvAddAvailable = qfalse;
				ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
		}
#endif

		// GL_ARB_multitexture
		qglMultiTexCoord2fARB = NULL;
		qglActiveTextureARB = NULL;
		qglClientActiveTextureARB = NULL;
#ifdef USE_OPENGLES
		qglGetIntegerv( GL_MAX_TEXTURE_UNITS, &glConfig.numTextureUnits );
		//ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, %i texture units\n", glConfig.maxActiveTextures );
		//glConfig.maxActiveTextures=4;
		qglMultiTexCoord2fARB = myglMultiTexCoord2f;
		qglActiveTextureARB = Q3E_GET_PROC_ADDRESS( "glActiveTexture" );
		qglClientActiveTextureARB = Q3E_GET_PROC_ADDRESS( "glClientActiveTexture" );
		if ( glConfig.numTextureUnits > 1 )
		{
			ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture (%i texture units)\n", glConfig.numTextureUnits );
		}
		else
		{
			qglMultiTexCoord2fARB = NULL;
			qglActiveTextureARB = NULL;
			qglClientActiveTextureARB = NULL;
			ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
		}
#else
		if ( Q3E_GL_ExtensionSupported( "GL_ARB_multitexture" ) )
		{
			if ( r_ext_multitexture->value )
			{
				qglMultiTexCoord2fARB = Q3E_GET_PROC_ADDRESS( "glMultiTexCoord2fARB" );
				qglActiveTextureARB = Q3E_GET_PROC_ADDRESS( "glActiveTextureARB" );
				qglClientActiveTextureARB = Q3E_GET_PROC_ADDRESS( "glClientActiveTextureARB" );

				if ( qglActiveTextureARB )
				{
					GLint glint = 0;
					qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
					glConfig.numTextureUnits = (int) glint;
					if ( glConfig.numTextureUnits > 1 )
					{
						ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
					}
					else
					{
						qglMultiTexCoord2fARB = NULL;
						qglActiveTextureARB = NULL;
						qglClientActiveTextureARB = NULL;
						ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
					}
				}
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
		}
#endif

		// GL_EXT_compiled_vertex_array
		if ( Q3E_GL_ExtensionSupported( "GL_EXT_compiled_vertex_array" ) )
		{
			if ( r_ext_compiled_vertex_array->value )
			{
				ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
				qglLockArraysEXT = ( void ( APIENTRY * )( GLint, GLint ) ) Q3E_GET_PROC_ADDRESS( "glLockArraysEXT" );
				qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) Q3E_GET_PROC_ADDRESS( "glUnlockArraysEXT" );
				if (!qglLockArraysEXT || !qglUnlockArraysEXT)
				{
					ri.Error (ERR_FATAL, "bad getprocaddress");
				}
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
		}
	}

	textureFilterAnisotropic = qfalse;
	if ( Q3E_GL_ExtensionSupported( "GL_EXT_texture_filter_anisotropic" ) )
	{
		if ( r_ext_texture_filter_anisotropic->integer ) {
			qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&maxAnisotropy );
			if ( maxAnisotropy <= 0 ) {
				ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
				maxAnisotropy = 0;
			}
			else
			{
				ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic (max: %i)\n", maxAnisotropy );
				textureFilterAnisotropic = qtrue;
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}

	haveClampToEdge = qfalse;
	if ( QGL_VERSION_ATLEAST( 1, 2 ) || QGLES_VERSION_ATLEAST( 1, 0 ) || Q3E_GL_ExtensionSupported( "GL_SGIS_texture_edge_clamp" ) )
	{
		ri.Printf( PRINT_ALL, "...using GL_SGIS_texture_edge_clamp\n" );
		haveClampToEdge = qtrue;
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_SGIS_texture_edge_clamp not found\n" );
	}
}

#define R_MODE_FALLBACK -1 //k 3 // 640 * 480

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( qboolean fixedFunction )
{
	ri.Printf( PRINT_DEVELOPER, "Glimp_Init( )\n" );

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );

	if( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		ri.Cvar_Set( "r_fullscreen", "1" ); //k 0
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}

	ri.Sys_GLimpInit( );

	// Create the window and set up the context
	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer, fixedFunction))
		goto success;

	// Try again, this time in a platform specific "safe mode"
	ri.Sys_GLimpSafeInit( );

	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, qfalse, fixedFunction))
		goto success;

	// Finally, try the default screen resolution
	if( r_mode->integer != R_MODE_FALLBACK )
	{
		ri.Printf( PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n",
				r_mode->integer, R_MODE_FALLBACK );

		if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, qfalse, qfalse, fixedFunction))
			goto success;
	}

	// Nothing worked, give up
	ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );

success:
	// These values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	// if hardware gamma is supported
	glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer &&
		qfalse;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );

#ifndef USE_OPENGLES
	// manually create extension list if using OpenGL 3
	if ( qglGetStringi )
	{
		int i, numExtensions, extensionLength, listLength;
		const char *extension;

		qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		listLength = 0;

		for ( i = 0; i < numExtensions; i++ )
		{
			extension = (char *) qglGetStringi( GL_EXTENSIONS, i );
			extensionLength = strlen( extension );

			if ( ( listLength + extensionLength + 1 ) >= sizeof( glConfig.extensions_string ) )
				break;

			if ( i > 0 ) {
				Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), " " );
				listLength++;
			}

			Q_strcat( glConfig.extensions_string, sizeof( glConfig.extensions_string ), extension );
			listLength += extensionLength;
		}
	}
	else
#endif
	{
		Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );
	}

	// initialize extensions
	GLimp_InitExtensions( fixedFunction );

	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	ri.IN_Init( NULL );
}


/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		Q3E_SwapBuffers();
	}

	if( r_fullscreen->modified )
	{
		r_fullscreen->modified = qfalse;
	}
}

void		GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
}
