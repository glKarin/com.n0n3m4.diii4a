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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "../unix/linux_local.h" // bk001130

#include "../unix/unix_glw.h"

glwstate_t glw_state;
static cvar_t *r_ext_multisample;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
extern cvar_t *r_centerWindow;
cvar_t  *r_previousglDriver;
extern cvar_t *in_nograb; // this is strictly for developers

extern void Android_GrabMouseCursor(qboolean grabIt);
extern qboolean no_handle_signals;


#define Q3E_PRINTF Com_Printf
#define Q3E_ERRORF(...) Com_Error(ERR_FATAL, __VA_ARGS__)
#define Q3E_DEBUGF(...) fprintf(stderr, __VA_ARGS__)
#define Q3Ebool qboolean
#define Q3E_TRUE qtrue
#define Q3E_FALSE qfalse

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

#include "../unix/linux_qgl.c"

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


/*
* Find the first occurrence of find in s.
*/
// bk001130 - from cvs1.17 (mkv), const
// bk001130 - made first argument const
static const char *Q_stristr( const char *s, const char *find)
{
	register char c, sc;
	register size_t len;

	if ((c = *find++) != 0)
	{
		if (c >= 'a' && c <= 'z')
		{
			c -= ('a' - 'A');
		}
		len = strlen(find);
		do
		{
			do
			{
				if ((sc = *s++) == 0)
					return NULL;
				if (sc >= 'a' && sc <= 'z')
				{
					sc -= ('a' - 'A');
				}
			} while (sc != c);
		} while (Q_stricmpn(s, find, len) != 0);
		s--;
	}
	return s;
}

char *strlwr (char *s) {
	if ( s==NULL ) { // bk001204 - paranoia
		assert(0);
		return s;
	}
	while (*s) {
		*s = tolower(*s);
		s++;
	}
	return s; // bk001204 - duh
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_S3_s3tc
	if ( Q_stristr( glConfig.extensions_string, "GL_S3_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
		} else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
	} else
	{
		glConfig.textureCompression = TC_NONE;
		ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
	}

	// GL_EXT_texture_env_add
    glConfig.textureEnvAddAvailable = qtrue;
    ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;

    qglGetIntegerv( GL_MAX_TEXTURE_UNITS, &glConfig.maxActiveTextures );
    qglMultiTexCoord2fARB = myglMultiTexCoord2f;
    qglActiveTextureARB = Q3E_GET_PROC_ADDRESS( "glActiveTexture" );
    qglClientActiveTextureARB = Q3E_GET_PROC_ADDRESS( "glClientActiveTexture" );
    if ( glConfig.maxActiveTextures > 1 )
    {
        ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture (%i texture units)\n", glConfig.maxActiveTextures );
    }
    else
    {
        qglMultiTexCoord2fARB = NULL;
        qglActiveTextureARB = NULL;
        qglClientActiveTextureARB = NULL;
        ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
    }

	// GL_EXT_compiled_vertex_array
	if ( Q_stristr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
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
		} else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	} else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	textureFilterAnisotropic = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_filter_anisotropic" ) )
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
}

/*
===============
GLW_SetMode
===============
*/
static int GLW_SetMode(const char *drivername, int mode, qboolean fullscreen)
{
    const char *glstring;

    ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");

    ri.Printf (PRINT_ALL, "...setting mode %d:", mode );
    // use desktop video resolution
    glConfig.vidWidth = screen_width;
    glConfig.vidHeight = screen_height;
    glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
    ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

    Q3E_GL_CONFIG_SET(samples, r_ext_multisample->value);

    Q3E_GL_CONFIG_SET(fullscreen, 1);
    Q3E_GL_CONFIG_SET(swap_interval, r_swapInterval->integer);
    Q3E_GL_CONFIG_ES_1_1();

    Q3E_InitGL();

    glConfig.colorBits = Q3E_GL_CONFIG(red) + Q3E_GL_CONFIG(green) + Q3E_GL_CONFIG(blue);
    glConfig.stencilBits = Q3E_GL_CONFIG(stencil);
    glConfig.depthBits = Q3E_GL_CONFIG(depth);

    glConfig.isFullscreen = qtrue;
    if (glConfig.isFullscreen) {
        Android_GrabMouseCursor(qtrue);
    }
    ri.Cvar_Set( "r_fullscreen", "1" );
    glConfig.displayFrequency = refresh_rate;
    Cvar_SetValue( "r_customwidth", screen_width );
    Cvar_SetValue( "r_customheight", screen_height );
    Cvar_Set( "r_mode", "-1" );
    Cvar_SetValue( "r_customPixelAspect", glConfig.windowAspect );

    ri.Printf( PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
               glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );

    // bk001130 - from cvs1.17 (mkv)
    const GLubyte * ( * _qglGetString )(GLenum name);
    _qglGetString = (const GLubyte * ( *)(GLenum name))Q3E_GET_PROC_ADDRESS("glGetString");
    glstring = (char *) _qglGetString (GL_RENDERER);
    ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

    return RSERR_OK;
}

/*
===============
GLW_StartDriverAndSetMode
===============
*/
static qboolean GLW_StartDriverAndSetMode(const char *drivername, int mode, qboolean fullscreen)
{
    rserr_t err;

    // don't ever bother going into fullscreen with a voodoo card
    if ( 1 )
    {
        ri.Cvar_Set( "r_fullscreen", "1" );
        r_fullscreen->modified = qfalse;
        fullscreen = qtrue;
    }

    if (fullscreen && in_nograb->value)
    {
        ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
        ri.Cvar_Set( "r_fullscreen", "0" );
        r_fullscreen->modified = qfalse;
        fullscreen = qfalse;
    }

    err = GLW_SetMode( drivername, mode, fullscreen );

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

static void GLW_InitGamma( void )
{
	glConfig.deviceSupportsGamma = qfalse;

}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL( const char *name )
{
	qboolean fullscreen;

	ri.Printf( PRINT_ALL, "...loading %s:\n", name );

	fullscreen = r_fullscreen->integer;

	// create the window and set up the context
	if ( !GLW_StartDriverAndSetMode( name, r_mode->integer, fullscreen ) )
		goto fail;

	// load the QGL layer
	if ( QGL_Init( name ) )
	{
		return qtrue;
	} else {
		goto fail;
	}
fail:
	ri.Printf( PRINT_ALL, "failed\n" );

	QGL_Shutdown();

	return qfalse;
}

void GLimp_Init( void )
{
	qboolean attemptedlibGL = qfalse;
	qboolean attempted3Dfx = qfalse;
	qboolean success = qfalse;
    char  buf[1024];
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

    r_ext_multisample = ri.Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

	r_previousglDriver = ri.Cvar_Get( "r_previousglDriver", "", CVAR_ROM );

	if(!no_handle_signals)
	InitSig();

	IN_Init();   // rcg08312005 moved into glimp.

	// Hack here so that if the UI
	if ( *r_previousglDriver->string )
	{
		// The UI changed it on us, hack it back
		// This means the renderer can't be changed on the fly
		ri.Cvar_Set( "r_glDriver", r_previousglDriver->string );
	}

	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL( r_glDriver->string ) )
	{
		ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem - get help on the official website: http://www.urbanterror.info/support/manual/faq/\n" );
	}

	// Save it in case the UI stomps it
	ri.Cvar_Set( "r_previousglDriver", r_glDriver->string );

	// This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

	//
	// chipset specific configuration
	//
	strcpy( buf, glConfig.renderer_string );
	strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( 0 )
	{
		glConfig.hardwareType = GLHW_GENERIC;

		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

		// VOODOO GRAPHICS w/ 2MB
		if ( Q_stristr( buf, "voodoo graphics/1 tmu/2 mb" ) )
		{
			ri.Cvar_Set( "r_picmip", "2" );
			ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
		} else
		{
			ri.Cvar_Set( "r_picmip", "1" );

			if ( Q_stristr( buf, "rage 128" ) || Q_stristr( buf, "rage128" ) )
			{
				ri.Cvar_Set( "r_finish", "0" );
			}
				// Savage3D and Savage4 should always have trilinear enabled
			else if ( Q_stristr( buf, "savage3d" ) || Q_stristr( buf, "s3 savage4" ) )
			{
				ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			}
		}
	}

	//
	// this is where hardware specific workarounds that should be
	// detected/initialized every startup should go.
	//
    glConfig.hardwareType = GLHW_GENERIC;

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	// initialize extensions
	GLW_InitExtensions();
	GLW_InitGamma();

	if(!no_handle_signals)
	InitSig(); // not clear why this is at begin & end of function

	return;
}

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	IN_Shutdown();

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );

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
	if ( glw_state.log_fp )
	{
		fprintf( glw_state.log_fp, "%s", comment );
	}
}

#define R_MODE_FALLBACK -1 //k 3 // 640 * 480


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

	// check logging
	QGL_EnableLogging( (qboolean)r_logFile->integer ); // bk001205 - was ->value
}

void		GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
}


void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {
    ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n");
    return qfalse;
}
void *GLimp_RendererSleep( void ) {
    return NULL;
}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}