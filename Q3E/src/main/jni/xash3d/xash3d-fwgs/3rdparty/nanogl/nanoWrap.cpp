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
/*
#include <e32def.h>
#include <e32std.h>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl.h"
#include "glesinterface.h"
#include "nanogl.h"

#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1

struct nanoState
{
	GLboolean alpha_test;
	GLboolean blend;
	GLboolean clip_planei;
	GLboolean color_logic_op;
	GLboolean color_material;
	GLboolean cull_face;
	GLboolean depth_test;
	GLboolean dither;
	GLboolean fog;
	GLboolean light0;
	GLboolean light1;
	GLboolean light2;
	GLboolean light3;
	GLboolean light4;
	GLboolean light5;
	GLboolean light6;
	GLboolean light7;
	GLboolean lighting;
	GLboolean line_smooth;
	GLboolean matrix_palette_oes;
	GLboolean multisample;
	GLboolean normalize;
	GLboolean point_smooth;
	GLboolean point_sprite_oes;
	GLboolean polygon_offset_fill;
	GLboolean rescale_normal;
	GLboolean sample_alpha_to_coverage;
	GLboolean sample_alpha_to_one;
	GLboolean sample_coverage;
	GLboolean scissor_test;
	GLboolean stencil_test;
	GLboolean depthmask;
	GLclampd depth_range_near;
	GLclampd depth_range_far;
	GLenum depth_func;
	GLenum cullface;
	GLenum shademodel;
	GLenum sfactor;
	GLenum dfactor;
	GLenum matrixmode;
};

static struct nanoState nanoglState;

static struct nanoState nanoglInitState =
{
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_TRUE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_TRUE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_FALSE,
        GL_TRUE,
        0.0f,
        1.0f,
        GL_LESS,
        GL_BACK,
        GL_SMOOTH,
        GL_ONE,
        GL_ZERO,
        GL_MODELVIEW,
};

struct booleanstate
{
	GLboolean value;
	GLboolean changed;
};

struct floatstate
{
	GLfloat value;
	GLboolean changed;
};

struct uintstate
{
	GLuint value;
	GLboolean changed;
};

struct ptrstate
{
	GLint size;
	GLenum type;
	GLsizei stride;
	GLvoid *ptr;
	GLboolean changed;
	GLboolean enabled;
};

struct nanotmuState
{
	struct booleanstate texture_2d;
	struct floatstate texture_env_mode;
	struct uintstate boundtexture;
	struct ptrstate vertex_array;
	struct ptrstate color_array;
	struct ptrstate texture_coord_array;
	struct ptrstate normal_array;
};

static struct nanotmuState tmuState0;
static struct nanotmuState tmuState1;

static struct nanotmuState tmuInitState =
    {
        {GL_FALSE, GL_FALSE},
        {GL_MODULATE, GL_FALSE},
        {0x7fffffff, GL_FALSE},
        {4, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE},
        {4, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE},
        {4, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE},
        {3, GL_FLOAT, 0, NULL, GL_FALSE, GL_FALSE},
};

static struct nanotmuState *activetmuState = &tmuState0;

extern GlESInterface *glEsImpl;

static GLenum wrapperPrimitiveMode = GL_QUADS;
GLboolean useTexCoordArray         = GL_FALSE;
static GLenum activetmu            = GL_TEXTURE0;
static GLenum clientactivetmu      = GL_TEXTURE0;

#if defined( __MULTITEXTURE_SUPPORT__ )
GLboolean useMultiTexCoordArray = GL_FALSE;
#endif

#if !defined( __WINS__ )
//#define __FORCEINLINE __forceinline
#define __FORCEINLINE inline
#else
#define __FORCEINLINE
#endif

static GLboolean delayedttmuchange = GL_FALSE;
static GLenum delayedtmutarget     = GL_TEXTURE0;

struct VertexAttrib
{
	float x;
	float y;
	float z;
#if !defined( __MULTITEXTURE_SUPPORT__ )
	float padding;
#endif
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;

	float s;
	float t;
#if defined( __MULTITEXTURE_SUPPORT__ )
	float s_multi;
	float t_multi;
#endif
};

static VertexAttrib vertexattribs[60000];

static GLushort indexArray[50000];

static GLuint vertexCount = 0;
static GLuint indexCount  = 0;
static GLuint vertexMark  = 0;
static int indexbase      = 0;

static VertexAttrib *ptrVertexAttribArray     = NULL;
static VertexAttrib *ptrVertexAttribArrayMark = NULL;

static VertexAttrib currentVertexAttrib;
#if defined( __MULTITEXTURE_SUPPORT__ )
static VertexAttrib currentVertexAttribInit = {0.0f, 0.0f, 0.0f, 255, 255, 255, 255, 0.0f, 0.0f, 0.0f, 0.0f};
#else
static VertexAttrib currentVertexAttribInit = {
    0.0f, 0.0f, 0.0f, 0.0f, 255, 255, 255, 255, 0.0f, 0.0f,
};
#endif
static GLushort *ptrIndexArray = NULL;

static GLboolean arraysValid = GL_FALSE;

static GLboolean skipnanogl;

void InitGLStructs( )
{
	ptrVertexAttribArray     = vertexattribs;
	ptrVertexAttribArrayMark = ptrVertexAttribArray;
	ptrIndexArray            = indexArray;

	memcpy( &nanoglState, &nanoglInitState, sizeof( struct nanoState ) );
	memcpy( &tmuState0, &tmuInitState, sizeof( struct nanotmuState ) );
	memcpy( &tmuState1, &tmuInitState, sizeof( struct nanotmuState ) );
	memcpy( &currentVertexAttrib, &currentVertexAttribInit, sizeof( struct VertexAttrib ) );

	activetmuState       = &tmuState0;
	wrapperPrimitiveMode = GL_QUADS;
	useTexCoordArray     = GL_FALSE;
	activetmu            = GL_TEXTURE0;
	clientactivetmu      = GL_TEXTURE0;
	delayedttmuchange    = GL_FALSE;
	delayedtmutarget     = GL_TEXTURE0;
	vertexCount          = 0;
	indexCount           = 0;
	vertexMark           = 0;
	indexbase            = 0;
	arraysValid          = GL_FALSE;
}

void ResetNanoState( )
{

	if ( tmuState0.color_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
	}

	if ( tmuState0.vertex_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_VERTEX_ARRAY );
	}

	if ( tmuState0.texture_coord_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if ( tmuState0.normal_array.enabled )
	{
		glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
	}
	else
	{
		glEsImpl->glDisableClientState( GL_NORMAL_ARRAY );
	}
	glEsImpl->glVertexPointer( tmuState0.vertex_array.size,
	                           tmuState0.vertex_array.type,
	                           tmuState0.vertex_array.stride,
	                           tmuState0.vertex_array.ptr );

	glEsImpl->glTexCoordPointer( tmuState0.texture_coord_array.size,
	                             tmuState0.texture_coord_array.type,
	                             tmuState0.texture_coord_array.stride,
	                             tmuState0.texture_coord_array.ptr );

	glEsImpl->glColorPointer( tmuState0.color_array.size,
	                          tmuState0.color_array.type,
	                          tmuState0.color_array.stride,
	                          tmuState0.color_array.ptr );

	glEsImpl->glNormalPointer(
	    tmuState0.normal_array.type,
	    tmuState0.normal_array.stride,
	    tmuState0.normal_array.ptr );

	glEsImpl->glMatrixMode( nanoglState.matrixmode );

	glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
		 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );

	glEsImpl->glBlendFunc( nanoglState.sfactor, nanoglState.dfactor );

	//glEsImpl->glBindTexture(GL_TEXTURE_2D, stackTextureState);

	glEsImpl->glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, activetmuState->texture_env_mode.value );

	arraysValid = GL_FALSE;
	skipnanogl = GL_FALSE;
}

void FlushOnStateChange( )
{
	if( skipnanogl )
		return;
	if ( delayedttmuchange )
	{
		delayedttmuchange = GL_FALSE;
#ifndef USE_CORE_PROFILE
		glEsImpl->glActiveTexture( delayedtmutarget );
#endif
	}

	if ( !vertexCount )
		return;

	if ( !arraysValid )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
		glEsImpl->glVertexPointer( 3, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].x );
		glEsImpl->glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( VertexAttrib ), &vertexattribs[0].red );
		glEsImpl->glTexCoordPointer( 2, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].s );
		glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
		glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
#if defined( __MULTITEXTURE_SUPPORT__ )
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		glEsImpl->glTexCoordPointer( 2, GL_FLOAT, sizeof( VertexAttrib ), &vertexattribs[0].s_multi );
		glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
#endif
		arraysValid = GL_TRUE;
	}

	glEsImpl->glDrawElements( GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, indexArray );

#if defined( __MULTITEXTURE_SUPPORT__ )
	useMultiTexCoordArray = GL_FALSE;
#endif
	vertexMark               = vertexCount          = 0;
	indexbase                = indexCount           = 0;
	ptrVertexAttribArrayMark = ptrVertexAttribArray = vertexattribs;

	ptrIndexArray            = indexArray;

	useTexCoordArray         = GL_FALSE;
}
void nanoGL_Flush( )
{
	FlushOnStateChange( );
}
void nanoGL_Reset( )
{
	ResetNanoState( );
}
void GL_MANGLE(glBegin)( GLenum mode )
{
	wrapperPrimitiveMode     = mode;
	vertexMark               = vertexCount;
	ptrVertexAttribArrayMark = ptrVertexAttribArray;
	indexbase                = indexCount;
}

void GL_MANGLE(glEnd)( void )
{
	vertexCount += ( (unsigned char *)ptrVertexAttribArray - (unsigned char *)ptrVertexAttribArrayMark ) / sizeof( VertexAttrib );
	if ( vertexCount < 3 )
	{
		return;
	}
	switch ( wrapperPrimitiveMode )
	{
	case GL_QUADS:
	{
		int qcount = ( vertexCount - vertexMark ) / 4;

		for ( int count = 0; count < qcount; count++ )
		{
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 1;
			*ptrIndexArray++ = indexCount + 2;
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 2;
			*ptrIndexArray++ = indexCount + 3;
			indexCount += 4;
			vertexCount += 2;
		}
	}
	break;
	case GL_TRIANGLES:
	{
		int vcount = ( vertexCount - vertexMark ) / 3;
		for ( int count = 0; count < vcount; count++ )
		{
			*ptrIndexArray++ = indexCount;
			*ptrIndexArray++ = indexCount + 1;
			*ptrIndexArray++ = indexCount + 2;
			indexCount += 3;
		}
	}
	break;
	case GL_TRIANGLE_STRIP:
	{
		*ptrIndexArray++ = indexCount;
		*ptrIndexArray++ = indexCount + 1;
		*ptrIndexArray++ = indexCount + 2;
		indexCount += 3;
		int vcount = ( ( vertexCount - vertexMark ) - 3 );
		if ( vcount && ( (long)ptrIndexArray & 0x02 ) )
		{
			*ptrIndexArray++ = indexCount - 1; // 2
			*ptrIndexArray++ = indexCount - 2; // 1
			*ptrIndexArray++ = indexCount;     // 3
			indexCount++;
			vcount -= 1;
			int odd = vcount & 1;
			vcount /= 2;
			unsigned int *longptr = (unsigned int *)ptrIndexArray;

			for ( int count = 0; count < vcount; count++ )
			{
				*( longptr++ ) = ( indexCount - 2 ) | ( ( indexCount - 1 ) << 16 );
				*( longptr++ ) = ( indexCount ) | ( ( indexCount ) << 16 );
				*( longptr++ ) = ( indexCount - 1 ) | ( ( indexCount + 1 ) << 16 );
				indexCount += 2;
			}
			ptrIndexArray = (unsigned short *)( longptr );
			if ( odd )
			{
				*ptrIndexArray++ = indexCount - 2; // 2
				*ptrIndexArray++ = indexCount - 1; // 1
				*ptrIndexArray++ = indexCount;     // 3
				indexCount++;
			}
		}
		else
		{
			//already aligned
			int odd = vcount & 1;
			vcount /= 2;
			unsigned int *longptr = (unsigned int *)ptrIndexArray;

			for ( int count = 0; count < vcount; count++ )
			{
				*( longptr++ ) = ( indexCount - 1 ) | ( ( indexCount - 2 ) << 16 );
				*( longptr++ ) = ( indexCount ) | ( ( indexCount - 1 ) << 16 );
				*( longptr++ ) = ( indexCount ) | ( ( indexCount + 1 ) << 16 );
				indexCount += 2;
			}
			ptrIndexArray = (unsigned short *)( longptr );
			if ( odd )
			{

				*ptrIndexArray++ = indexCount - 1; // 2
				*ptrIndexArray++ = indexCount - 2; // 1
				*ptrIndexArray++ = indexCount;     // 3
				indexCount++;
			}
		}
		vertexCount += ( vertexCount - vertexMark - 3 ) * 2;
	}
	break;
	case GL_POLYGON:
	case GL_TRIANGLE_FAN:
	{
		*ptrIndexArray++ = indexCount++;
		*ptrIndexArray++ = indexCount++;
		*ptrIndexArray++ = indexCount++;
		int vcount       = ( ( vertexCount - vertexMark ) - 3 );
		for ( int count = 0; count < vcount; count++ )
		{
			*ptrIndexArray++ = indexbase;
			*ptrIndexArray++ = indexCount - 1;
			*ptrIndexArray++ = indexCount++;
			vertexCount += 2;
		}
	}
	break;

	default:
		break;
	}
	if ( ptrVertexAttribArray - vertexattribs > 20000 * sizeof( VertexAttrib ) ||
	     ptrIndexArray - indexArray > 15000 * sizeof( GLushort ) )
		FlushOnStateChange( );
}

void GL_MANGLE(glEnable)( GLenum cap )
{
	if( skipnanogl )
	{
		glEsImpl->glEnable( cap );
		return;
	}
	GLboolean statechanged = GL_FALSE;
	switch ( cap )
	{
	case GL_ALPHA_TEST:
	{
		if ( !nanoglState.alpha_test )
		{
			nanoglState.alpha_test = GL_TRUE;
			statechanged           = GL_TRUE;
		}
		break;
	}
	case GL_BLEND:
	{
		if ( !nanoglState.blend )
		{
			nanoglState.blend = GL_TRUE;
			statechanged      = GL_TRUE;
		}
		break;
	}
	//case GL_CLIP_PLANEi
	case GL_COLOR_LOGIC_OP:
	{
		if ( !nanoglState.color_logic_op )
		{
			nanoglState.color_logic_op = GL_TRUE;
			statechanged               = GL_TRUE;
		}
		break;
	}
	case GL_COLOR_MATERIAL:
	{
		if ( !nanoglState.color_material )
		{
			nanoglState.color_material = GL_TRUE;
			statechanged               = GL_TRUE;
		}
		break;
	}
	case GL_CULL_FACE:
	{
		if ( !nanoglState.cull_face )
		{
			nanoglState.cull_face = GL_TRUE;
			statechanged          = GL_TRUE;
		}
		break;
	}
	case GL_DEPTH_TEST:
	{
		if ( !nanoglState.depth_test )
		{
			nanoglState.depth_test = GL_TRUE;
			statechanged           = GL_TRUE;
		}
		break;
	}
	case GL_DITHER:
	{
		if ( !nanoglState.dither )
		{
			nanoglState.dither = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_FOG:
	{
		if ( !nanoglState.fog )
		{
			nanoglState.fog = GL_TRUE;
			statechanged    = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT0:
	{
		if ( !nanoglState.light0 )
		{
			nanoglState.light0 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT1:
	{
		if ( !nanoglState.light1 )
		{
			nanoglState.light1 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT2:
	{
		if ( !nanoglState.light2 )
		{
			nanoglState.light2 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT3:
	{
		if ( !nanoglState.light3 )
		{
			nanoglState.light3 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT4:
	{
		if ( !nanoglState.light4 )
		{
			nanoglState.light4 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT5:
	{
		if ( !nanoglState.light5 )
		{
			nanoglState.light5 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT6:
	{
		if ( !nanoglState.light6 )
		{
			nanoglState.light6 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT7:
	{
		if ( !nanoglState.light7 )
		{
			nanoglState.light7 = GL_TRUE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHTING:
	{
		if ( !nanoglState.lighting )
		{
			nanoglState.lighting = GL_TRUE;
			statechanged         = GL_TRUE;
		}
		break;
	}
	case GL_LINE_SMOOTH:
	{
		if ( !nanoglState.line_smooth )
		{
			nanoglState.line_smooth = GL_TRUE;
			statechanged            = GL_TRUE;
		}
		break;
	}
	/*        case GL_MATRIX_PALETTE_OES:
            {
            if (!nanoglState.matrix_palette_oes)
                {
                nanoglState.matrix_palette_oes = GL_TRUE;
                statechanged = GL_TRUE;
                }
            break;
            }*/
	case GL_MULTISAMPLE:
	{
		if ( !nanoglState.multisample )
		{
			nanoglState.multisample = GL_TRUE;
			statechanged            = GL_TRUE;
		}
		break;
	}
	case GL_NORMALIZE:
	{
		if ( !nanoglState.normalize )
		{
			nanoglState.normalize = GL_TRUE;
			statechanged          = GL_TRUE;
		}
		break;
	}
	/*        case GL_POINT_SPRITE_OES:
            {
            if (!nanoglState.point_sprite_oes)
                {
                nanoglState.point_sprite_oes = GL_TRUE;
                statechanged = GL_TRUE;
                }
            break;
            }*/
	case GL_POLYGON_OFFSET_FILL:
	{
		if ( !nanoglState.polygon_offset_fill )
		{
			nanoglState.polygon_offset_fill = GL_TRUE;
			statechanged                    = GL_TRUE;
		}
		break;
	}
	case GL_RESCALE_NORMAL:
	{
		if ( !nanoglState.rescale_normal )
		{
			nanoglState.rescale_normal = GL_TRUE;
			statechanged               = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
	{
		if ( !nanoglState.sample_alpha_to_coverage )
		{
			nanoglState.sample_alpha_to_coverage = GL_TRUE;
			statechanged                         = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_ONE:
	{
		if ( !nanoglState.sample_alpha_to_one )
		{
			nanoglState.sample_alpha_to_one = GL_TRUE;
			statechanged                    = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_COVERAGE:
	{
		if ( !nanoglState.sample_coverage )
		{
			nanoglState.sample_coverage = GL_TRUE;
			statechanged                = GL_TRUE;
		}
		break;
	}
	case GL_SCISSOR_TEST:
	{
		if ( !nanoglState.scissor_test )
		{
			nanoglState.scissor_test = GL_TRUE;
			statechanged             = GL_TRUE;
		}
		break;
	}
	case GL_STENCIL_TEST:
	{
		if (!nanoglState.stencil_test)
		{
			nanoglState.stencil_test = GL_TRUE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_TEXTURE_2D:
	{
		if ( !activetmuState->texture_2d.value )
		{
			FlushOnStateChange( );
			glEsImpl->glEnable( cap );
			activetmuState->texture_2d.value = GL_TRUE;
			return;
		}
		break;
	}
#if 0 // todo: implement cubemap texgen
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
	{
		FlushOnStateChange( );
		nanoglState.texgen = true;
		return;
	}
#endif
	default:
		break;
	}

	if ( statechanged )
	{
		FlushOnStateChange( );
		glEsImpl->glEnable( cap );
	}
}

void GL_MANGLE(glDisable)( GLenum cap )
{
	if( skipnanogl )
	{
		glEsImpl->glDisable( cap );
		return;
	}
	GLboolean statechanged = GL_FALSE;
	switch ( cap )
	{
	case GL_ALPHA_TEST:
	{
		if ( nanoglState.alpha_test )
		{
			nanoglState.alpha_test = GL_FALSE;
			statechanged           = GL_TRUE;
		}
		break;
	}
	case GL_BLEND:
	{
		if ( nanoglState.blend )
		{
			nanoglState.blend = GL_FALSE;
			statechanged      = GL_TRUE;
		}
		break;
	}
	//case GL_CLIP_PLANEi
	case GL_COLOR_LOGIC_OP:
	{
		if ( nanoglState.color_logic_op )
		{
			nanoglState.color_logic_op = GL_FALSE;
			statechanged               = GL_TRUE;
		}
		break;
	}
	case GL_COLOR_MATERIAL:
	{
		if ( nanoglState.color_material )
		{
			nanoglState.color_material = GL_FALSE;
			statechanged               = GL_TRUE;
		}
		break;
	}
	case GL_CULL_FACE:
	{
		if ( nanoglState.cull_face )
		{
			nanoglState.cull_face = GL_FALSE;
			statechanged          = GL_TRUE;
		}
		break;
	}
	case GL_DEPTH_TEST:
	{
		if ( nanoglState.depth_test )
		{
			nanoglState.depth_test = GL_FALSE;
			statechanged           = GL_TRUE;
		}
		break;
	}
	case GL_DITHER:
	{
		if ( nanoglState.dither )
		{
			nanoglState.dither = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_FOG:
	{
		if ( nanoglState.fog )
		{
			nanoglState.fog = GL_FALSE;
			statechanged    = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT0:
	{
		if ( !nanoglState.light0 )
		{
			nanoglState.light0 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT1:
	{
		if ( !nanoglState.light1 )
		{
			nanoglState.light1 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT2:
	{
		if ( !nanoglState.light2 )
		{
			nanoglState.light2 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT3:
	{
		if ( !nanoglState.light3 )
		{
			nanoglState.light3 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT4:
	{
		if ( !nanoglState.light4 )
		{
			nanoglState.light4 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT5:
	{
		if ( !nanoglState.light5 )
		{
			nanoglState.light5 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT6:
	{
		if ( !nanoglState.light6 )
		{
			nanoglState.light6 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHT7:
	{
		if ( !nanoglState.light7 )
		{
			nanoglState.light7 = GL_FALSE;
			statechanged       = GL_TRUE;
		}
		break;
	}
	case GL_LIGHTING:
	{
		if ( nanoglState.lighting )
		{
			nanoglState.lighting = GL_FALSE;
			statechanged         = GL_TRUE;
		}
		break;
	}
	case GL_LINE_SMOOTH:
	{
		if ( nanoglState.line_smooth )
		{
			nanoglState.line_smooth = GL_FALSE;
			statechanged            = GL_TRUE;
		}
		break;
	}
	/*        case GL_MATRIX_PALETTE_OES:
            {
            if (nanoglState.matrix_palette_oes)
                {
                nanoglState.matrix_palette_oes = GL_FALSE;
                statechanged = GL_TRUE;
                }
            break;
            }*/
	case GL_MULTISAMPLE:
	{
		if ( nanoglState.multisample )
		{
			nanoglState.multisample = GL_FALSE;
			statechanged            = GL_TRUE;
		}
		break;
	}
	case GL_NORMALIZE:
	{
		if ( nanoglState.normalize )
		{
			nanoglState.normalize = GL_FALSE;
			statechanged          = GL_TRUE;
		}
		break;
	}
	/*        case GL_POINT_SPRITE_OES:
            {
            if (nanoglState.point_sprite_oes)
                {
                nanoglState.point_sprite_oes = GL_FALSE;
                statechanged = GL_TRUE;
                }
            break;
            }*/
	case GL_POLYGON_OFFSET_FILL:
	{
		if ( nanoglState.polygon_offset_fill )
		{
			nanoglState.polygon_offset_fill = GL_FALSE;
			statechanged                    = GL_TRUE;
		}
		break;
	}
	case GL_RESCALE_NORMAL:
	{
		if ( nanoglState.rescale_normal )
		{
			nanoglState.rescale_normal = GL_FALSE;
			statechanged               = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
	{
		if ( nanoglState.sample_alpha_to_coverage )
		{
			nanoglState.sample_alpha_to_coverage = GL_FALSE;
			statechanged                         = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_ALPHA_TO_ONE:
	{
		if ( nanoglState.sample_alpha_to_one )
		{
			nanoglState.sample_alpha_to_one = GL_FALSE;
			statechanged                    = GL_TRUE;
		}
		break;
	}
	case GL_SAMPLE_COVERAGE:
	{
		if ( nanoglState.sample_coverage )
		{
			nanoglState.sample_coverage = GL_FALSE;
			statechanged                = GL_TRUE;
		}
		break;
	}
	case GL_SCISSOR_TEST:
	{
		if ( nanoglState.scissor_test )
		{
			nanoglState.scissor_test = GL_FALSE;
			statechanged             = GL_TRUE;
		}
		break;
	}
	case GL_STENCIL_TEST:
	{
		if (nanoglState.stencil_test)
		{
			nanoglState.stencil_test = GL_FALSE;
			statechanged = GL_TRUE;
		}
		break;
	}
	case GL_TEXTURE_2D:
	{
		if ( activetmuState->texture_2d.value )
		{
			FlushOnStateChange( );
			glEsImpl->glDisable( cap );
			activetmuState->texture_2d.value = GL_FALSE;
			return;
		}
		break;
	}
#if 0
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
	{
		FlushOnStateChange( );
		nanoglState.texgen = false;
		return;
	}
#endif
	default:
		break;
	}

	if ( statechanged )
	{
		FlushOnStateChange( );
		glEsImpl->glDisable( cap );
	}
}

void GL_MANGLE(glVertex2f)( GLfloat x, GLfloat y )
{
	GL_MANGLE(glVertex3f)( x, y, 0.0f );
}

__FORCEINLINE unsigned int ClampTo255( float value )
{
	unsigned int retval = (unsigned int)( value );
	if ( retval > 255 )
	{
		retval = 255;
	}
	return retval;
}

void GL_MANGLE(glColor3f)( GLfloat red, GLfloat green, GLfloat blue )
{
	currentVertexAttrib.red   = (unsigned char)ClampTo255( red * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( green * 255.0f );
	currentVertexAttrib.blue  = (unsigned char)ClampTo255( blue * 255.0f );
	currentVertexAttrib.alpha = 255;
}

void GL_MANGLE(glTexCoord2fv)( const GLfloat *v )
{
	memcpy( &currentVertexAttrib.s, v, 2 * sizeof( float ) );
}

void GL_MANGLE(glTexCoord2f)( GLfloat s, GLfloat t )
{
	currentVertexAttrib.s = s;
	currentVertexAttrib.t = t;
}

void GL_MANGLE(glViewport)( GLint x, GLint y, GLsizei width, GLsizei height )
{
	FlushOnStateChange( );
	glEsImpl->glViewport( x, y, width, height );
}

void GL_MANGLE(glLoadIdentity)( void )
{
	FlushOnStateChange( );
	glEsImpl->glLoadIdentity( );
}

void GL_MANGLE(glColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
	currentVertexAttrib.red   = (unsigned char)ClampTo255( red * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( green * 255.0f );
	currentVertexAttrib.blue  = (unsigned char)ClampTo255( blue * 255.0f );
	currentVertexAttrib.alpha = (unsigned char)ClampTo255( alpha * 255.0f );
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );

}

void GL_MANGLE(glOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	FlushOnStateChange( );
#ifdef USE_CORE_PROFILE
	glEsImpl->glOrtho( left, right, bottom, top, zNear, zFar );
#else
	glEsImpl->glOrthof( left, right, bottom, top, zNear, zFar );
#endif
}

// Rikku2000: Light
void GL_MANGLE(glLightf)( GLenum light, GLenum pname, GLfloat param )
{
	FlushOnStateChange( );

	glEsImpl->glLightf( light, pname, param );
}
void GL_MANGLE(glLightfv)( GLenum light, GLenum pname, const GLfloat *params )
{
	FlushOnStateChange( );

	glEsImpl->glLightfv( light, pname, params );
}
void GL_MANGLE(glLightModelf)( GLenum pname, GLfloat param )
{
	FlushOnStateChange( );

	glEsImpl->glLightModelf( pname, param );
}
void GL_MANGLE(glLightModelfv)( GLenum pname, const GLfloat *params )
{
	FlushOnStateChange( );

	glEsImpl->glLightModelfv( pname, params );
}
void GL_MANGLE(glMaterialf)( GLenum face, GLenum pname, GLfloat param )
{
	FlushOnStateChange( );

	glEsImpl->glMaterialf( face, pname, param );
}
void GL_MANGLE(glMaterialfv)( GLenum face, GLenum pname, const GLfloat *params )
{
	FlushOnStateChange( );

	glEsImpl->glMaterialfv( face, pname, params );
}
void GL_MANGLE(glColorMaterial)( GLenum face, GLenum mode )
{
	FlushOnStateChange( );

	glEsImpl->glColorMaterial( face, mode );
}

void GL_MANGLE(glMatrixMode)( GLenum mode )
{
	if ( nanoglState.matrixmode == mode )
	{
		return;
	}
	nanoglState.matrixmode = mode;
	FlushOnStateChange( );
	glEsImpl->glMatrixMode( mode );
}

void GL_MANGLE(glTexParameterf)( GLenum target, GLenum pname, GLfloat param )
{
	if ( pname == GL_TEXTURE_BORDER_COLOR )
	{
		return; // not supported by opengl es
	}
	if ( ( pname == GL_TEXTURE_WRAP_S ||
	       pname == GL_TEXTURE_WRAP_T ) &&
	     param == GL_CLAMP )
	{
		param = 0x812F;
	}

	FlushOnStateChange( );
	glEsImpl->glTexParameterf( target, pname, param );
}

void GL_MANGLE(glTexParameterfv)( GLenum target, GLenum pname, const GLfloat *params )
{
	GL_MANGLE(glTexParameterf)( target, pname, params[0] );
}

void GL_MANGLE(glTexImage2D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	unsigned char *data = (unsigned char*)pixels;

	if( pixels && format == GL_RGBA && (
		internalformat == GL_RGB ||
		internalformat == GL_RGB8 ||
		internalformat == GL_RGB5 ||
		internalformat == GL_LUMINANCE ||
		internalformat == GL_LUMINANCE8 ||
		internalformat == GL_LUMINANCE4 )) // strip alpha from texture
	{
		unsigned char *in = data, *out;
		int i = 0, size = width * height * 4;

		data = out = (unsigned char*)malloc( size );
	
		for( i = 0; i < size; i += 4, in += 4, out += 4 )
		{
			memcpy( out, in, 3 );
			out[3] = 255;
		}
	}
		
	internalformat = format;
	glEsImpl->glTexImage2D( target, level, internalformat, width, height, border, format, type, data );

	if( data != pixels )
		free(data);
}

void GL_MANGLE(glDrawBuffer)( GLenum /*mode*/ )
{
}

void GL_MANGLE(glTranslatef)( GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange( );
	glEsImpl->glTranslatef( x, y, z );
}

void GL_MANGLE(glRotatef)( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange( );
	glEsImpl->glRotatef( angle, x, y, z );
}

void GL_MANGLE(glScalef)( GLfloat x, GLfloat y, GLfloat z )
{
	FlushOnStateChange( );
	glEsImpl->glScalef( x, y, z );
}

void GL_MANGLE(glDepthRange)( GLclampd zNear, GLclampd zFar )
{
	if ( ( nanoglState.depth_range_near == zNear ) && ( nanoglState.depth_range_far == zFar ) )
	{
		return;
	}
	else
	{
		nanoglState.depth_range_near = zNear;
		nanoglState.depth_range_far  = zFar;
	}
	FlushOnStateChange( );
#ifdef USE_CORE_PROFILE
	glEsImpl->glDepthRange( zNear, zFar );
#else
	glEsImpl->glDepthRangef( zNear, zFar );
#endif
}

void GL_MANGLE(glDepthFunc)( GLenum func )
{
	if ( nanoglState.depth_func == func )
	{
		return;
	}
	else
	{
		nanoglState.depth_func = func;
	}
	FlushOnStateChange( );
	glEsImpl->glDepthFunc( func );
}

void GL_MANGLE(glFinish)( void )
{
	FlushOnStateChange( );
	glEsImpl->glFinish( );
}

void GL_MANGLE(glGetFloatv)( GLenum pname, GLfloat *params )
{
	FlushOnStateChange( );
	glEsImpl->glGetFloatv( pname, params );
}

void GL_MANGLE(glCullFace)( GLenum mode )
{
	if ( nanoglState.cullface == mode )
	{
		return;
	}
	else
	{
		nanoglState.cullface = mode;
	}
	FlushOnStateChange( );
	glEsImpl->glCullFace( mode );
}

void GL_MANGLE(glFrustum)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	FlushOnStateChange( );
	glEsImpl->glFrustumf( left, right, bottom, top, zNear, zFar );
}

void GL_MANGLE(glClear)( GLbitfield mask )
{
	FlushOnStateChange( );
	glEsImpl->glClear( mask );
}

void GL_MANGLE(glVertex3f)( GLfloat x, GLfloat y, GLfloat z )
{
	GLfloat *vert = (GLfloat *)ptrVertexAttribArray++;
	*vert++       = x;
	*vert++       = y;
	*vert++       = z;
#if defined( __MULTITEXTURE_SUPPORT__ )
	memcpy( vert, &currentVertexAttrib.red, 5 * sizeof( GLfloat ) );
#else
	memcpy( vert + 1, &currentVertexAttrib.red, 3 * sizeof( GLfloat ) );
#endif
}

void GL_MANGLE(glColor4fv)( const GLfloat *v )
{
	currentVertexAttrib.red   = (unsigned char)ClampTo255( v[0] * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( v[1] * 255.0f );
	currentVertexAttrib.blue  = (unsigned char)ClampTo255( v[2] * 255.0f );
	currentVertexAttrib.alpha = (unsigned char)ClampTo255( v[3] * 255.0f );
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );
}

void GL_MANGLE(glColor3ubv)( const GLubyte *v )
{
	currentVertexAttrib.red   = v[0];
	currentVertexAttrib.green = v[1];
	currentVertexAttrib.blue  = v[2];
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );
}

void GL_MANGLE(glColor4ubv)( const GLubyte *v )
{
	//*((unsigned int*)(&currentVertexAttrib.red)) = *((unsigned int*)(v));
	currentVertexAttrib.red   = v[0];
	currentVertexAttrib.green = v[1];
	currentVertexAttrib.blue  = v[2];
	currentVertexAttrib.alpha = v[3];
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );
}

void GL_MANGLE(glColor3fv)( const GLfloat *v )
{
	currentVertexAttrib.red   = (unsigned char)ClampTo255( v[0] * 255.0f );
	currentVertexAttrib.green = (unsigned char)ClampTo255( v[1] * 255.0f );
	currentVertexAttrib.blue  = (unsigned char)ClampTo255( v[2] * 255.0f );
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );
}

//-- nicknekit: xash3d funcs --

void GL_MANGLE(glColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
	currentVertexAttrib.red   = red;
	currentVertexAttrib.green = green;
	currentVertexAttrib.blue  = blue;
	currentVertexAttrib.alpha = alpha;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );
}

void GL_MANGLE(glColor3ub)( GLubyte red, GLubyte green, GLubyte blue )
{
	currentVertexAttrib.red   = red;
	currentVertexAttrib.green = green;
	currentVertexAttrib.blue  = blue;
	currentVertexAttrib.alpha = 255;
	if( skipnanogl )
		glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
			 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );
}

void GL_MANGLE(glNormal3fv)( const GLfloat *v )
{
	FlushOnStateChange( );
	glEsImpl->glNormal3f( v[0], v[1], v[2] );
}

void GL_MANGLE(glCopyTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
	FlushOnStateChange( );
	glEsImpl->glCopyTexImage2D( target, level, internalformat, x, y, width, height, border );
}

void GL_MANGLE(glTexImage1D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	GL_MANGLE(glTexImage2D)( GL_TEXTURE_2D, level, internalformat, width, 1, border, format, type, pixels );
}

void GL_MANGLE(glTexImage3D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	GL_MANGLE(glTexImage2D)( GL_TEXTURE_2D, level, internalformat, width, height, border, format, type, pixels );
}

void GL_MANGLE(glTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels )
{
	GL_MANGLE(glTexSubImage2D)( target, level, xoffset, 0, width, 1, format, type, pixels );
}

void GL_MANGLE(glTexSubImage3D)( GLenum target, GLint level,
                      GLint xoffset, GLint yoffset,
                      GLint zoffset, GLsizei width,
                      GLsizei height, GLsizei depth,
                      GLenum format,
                      GLenum type, const GLvoid *pixels )
{
	GL_MANGLE(glTexSubImage2D)( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

GLboolean GL_MANGLE(glIsTexture)( GLuint texture )
{
	FlushOnStateChange( );
	return glEsImpl->glIsTexture( texture );
}

// TODO: add native normal/reflection map texgen support

void GL_MANGLE(glTexGeni)( GLenum coord, GLenum pname, GLint param )
{
	FlushOnStateChange();
	//glEsImpl->glTexGeniOES( coord, pname, param );
}

void GL_MANGLE(glTexGenfv)( GLenum coord, GLenum pname, const GLfloat *params )
{
	FlushOnStateChange();
	//glEsImpl->glTexGenfvOES( coord, pname, params );
}

//-- --//

void GL_MANGLE(glHint)( GLenum target, GLenum mode )
{
	FlushOnStateChange( );
	glEsImpl->glHint( target, mode );
}

void GL_MANGLE(glBlendFunc)( GLenum sfactor, GLenum dfactor )
{
	if( skipnanogl )
	{
		glEsImpl->glBlendFunc( sfactor, dfactor );
		return;
	}
	if ( ( nanoglState.sfactor == sfactor ) && ( nanoglState.dfactor == dfactor ) )
	{
		return;
	}

	nanoglState.sfactor = sfactor;
	nanoglState.dfactor = dfactor;
	FlushOnStateChange( );
	glEsImpl->glBlendFunc( sfactor, dfactor );
}

void GL_MANGLE(glPopMatrix)( void )
{
	FlushOnStateChange( );
	glEsImpl->glPopMatrix( );
}

void GL_MANGLE(glShadeModel)( GLenum mode )
{
	if ( nanoglState.shademodel == mode )
	{
		return;
	}
	nanoglState.shademodel = mode;
	FlushOnStateChange( );
	glEsImpl->glShadeModel( mode );
}

void GL_MANGLE(glPushMatrix)( void )
{
	FlushOnStateChange( );
	glEsImpl->glPushMatrix( );
}

void GL_MANGLE(glTexEnvf)( GLenum target, GLenum pname, GLfloat param )
{
	if( skipnanogl )
	{
		glEsImpl->glTexEnvf( target, pname, param );
		return;
	}
	if ( target == GL_TEXTURE_ENV )
	{
		if ( pname == GL_TEXTURE_ENV_MODE )
		{
			if ( param == activetmuState->texture_env_mode.value )
			{
				return;
			}
			else
			{
				FlushOnStateChange( );
				glEsImpl->glTexEnvf( target, pname, param );
				activetmuState->texture_env_mode.value = param;
				return;
			}
		}
	}
	FlushOnStateChange( );
	glEsImpl->glTexEnvf( target, pname, param );
}

void GL_MANGLE(glVertex3fv)( const GLfloat *v )
{
	GLfloat *vert = (GLfloat *)ptrVertexAttribArray++;
	memcpy( vert, v, 3 * sizeof( GLfloat ) );
#if defined( __MULTITEXTURE_SUPPORT__ )
	memcpy( vert + 3, &currentVertexAttrib.red, 5 * sizeof( GLfloat ) );
#else
	memcpy( vert + 4, &currentVertexAttrib.red, 3 * sizeof( GLfloat ) );
#endif
}

void GL_MANGLE(glDepthMask)( GLboolean flag )
{
	if( !skipnanogl )
	{
	if ( nanoglState.depthmask == flag )
	{
		return;
	}
	nanoglState.depthmask = flag;
	FlushOnStateChange( );
	}
	glEsImpl->glDepthMask( flag );
}

void GL_MANGLE(glBindTexture)( GLenum target, GLuint texture )
{
	if( skipnanogl )
	{
		glEsImpl->glBindTexture( target,texture );
		activetmuState->boundtexture.value = texture;
		return;
	}
	if ( activetmuState->boundtexture.value == texture )
	{
		return;
	}
	FlushOnStateChange( );
	activetmuState->boundtexture.value = texture;
	glEsImpl->glBindTexture( target, texture );
}

void GL_MANGLE(glGetIntegerv)( GLenum pname, GLint *params )
{
	FlushOnStateChange( );
	glEsImpl->glGetIntegerv( pname, params );
}

GLubyte nano_extensions_string[4096];
const GLubyte *GL_MANGLE(glGetString)( GLenum name )
{

	if ( name == GL_EXTENSIONS )
	{
#if defined( __MULTITEXTURE_SUPPORT__ )
		sprintf( (char *)nano_extensions_string, "%s %s", glEsImpl->glGetString( name ), "GL_ARB_multitexture EXT_texture_env_add" );
#else
		sprintf( (char *)nano_extensions_string, "%s %s", glEsImpl->glGetString( name ), "EXT_texture_env_add" );
#endif
		return nano_extensions_string;
	}
	return glEsImpl->glGetString( name );
}

void GL_MANGLE(glAlphaFunc)( GLenum func, GLclampf ref )
{
	FlushOnStateChange( );
	glEsImpl->glAlphaFunc( func, ref );
}

void GL_MANGLE(glFlush)( void )
{
	FlushOnStateChange( );
	glEsImpl->glFlush( );
}

void GL_MANGLE(glReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels )
{
	if ( format == GL_DEPTH_COMPONENT )
	{
		// OpenglEs 1.1 does not support reading depth buffer without an extension
		memset( pixels, 0xff, 4 );
		return;
	}
	FlushOnStateChange( );
	glEsImpl->glReadPixels( x, y, width, height, format, type, pixels );
}

void GL_MANGLE(glReadBuffer)( GLenum /*mode*/ )
{
}

void GL_MANGLE(glLoadMatrixf)( const GLfloat *m )
{
	FlushOnStateChange( );
	glEsImpl->glLoadMatrixf( m );
}

void GL_MANGLE(glTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
	FlushOnStateChange( );
	glEsImpl->glTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

void GL_MANGLE(glClearColor)( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
	FlushOnStateChange( );
	glEsImpl->glClearColor( red, green, blue, alpha );
}

GLenum GL_MANGLE(glGetError)( void )
{
	//FlushOnStateChange();
	return GL_NO_ERROR; //glEsImpl->glGetError();
}

void GL_MANGLE(glActiveTexture)( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glActiveTexture( texture );
		return;
	}
	if ( activetmu == texture )
	{
		return;
	}
	if ( delayedttmuchange )
	{
		delayedttmuchange = GL_FALSE;
	}
	else
	{
		delayedttmuchange = GL_TRUE;
		delayedtmutarget  = texture;
	}
	if ( texture == GL_TEXTURE0 )
	{
		activetmuState = &tmuState0;
	}
	else
	{
		activetmuState = &tmuState1;
	}
	activetmu = texture;
}

void GL_MANGLE(glActiveTextureARB)( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glActiveTexture( texture );
		return;
	}
	if ( activetmu == texture )
	{
		return;
	}
	if ( delayedttmuchange )
	{
		delayedttmuchange = GL_FALSE;
	}
	else
	{
		delayedttmuchange = GL_TRUE;
		delayedtmutarget  = texture;
	}
	if ( texture == GL_TEXTURE0 )
	{
		activetmuState = &tmuState0;
	}
	else
	{
		activetmuState = &tmuState1;
	}
	activetmu = texture;
}

void GL_MANGLE(glClientActiveTexture)( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glClientActiveTexture( texture );
		return;
	}
	clientactivetmu = texture;
}
void GL_MANGLE(glClientActiveTextureARB)( GLenum texture )
{
	if( skipnanogl )
	{
		glEsImpl->glClientActiveTexture( texture );
		return;
	}
	clientactivetmu = texture;
}


void GL_MANGLE(glPolygonMode)( GLenum face, GLenum mode )
{
}

void GL_MANGLE(glDeleteTextures)( GLsizei n, const GLuint *textures )
{
	FlushOnStateChange( );
	glEsImpl->glDeleteTextures( n, textures );
}

void GL_MANGLE(glClearDepth)( GLclampd depth )
{
	FlushOnStateChange( );
	glEsImpl->glClearDepthf( depth );
}

void GL_MANGLE(glClipPlane)( GLenum plane, const GLdouble *equation )
{
	FlushOnStateChange( );
	float array[4];
	array[0] = ( GLfloat )( equation[0] );
	array[1] = ( GLfloat )( equation[1] );
	array[2] = ( GLfloat )( equation[2] );
	array[3] = ( GLfloat )( equation[3] );
	glEsImpl->glClipPlanef( plane, array );
}

void GL_MANGLE(glScissor)( GLint x, GLint y, GLsizei width, GLsizei height )
{
	FlushOnStateChange( );
	glEsImpl->glScissor( x, y, width, height );
}

void GL_MANGLE(glPointSize)( GLfloat size )
{
	FlushOnStateChange( );
	glEsImpl->glPointSize( size );
}

void GL_MANGLE(glArrayElement)( GLint i )
{
}
void GL_MANGLE(glLineWidth)( GLfloat width )
{
}
void GL_MANGLE(glCallList)( GLuint list )
{
}
void GL_MANGLE(glColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
	FlushOnStateChange( );
	glEsImpl->glColorMask( red, green, blue, alpha );	
}
void GL_MANGLE(glStencilFunc)( GLenum func, GLint ref, GLuint mask )
{
	FlushOnStateChange( );
	glEsImpl->glStencilFunc( func, ref, mask );	
}
void GL_MANGLE(glStencilOp)( GLenum fail, GLenum zfail, GLenum zpass )
{
	FlushOnStateChange( );
	glEsImpl->glStencilOp( fail, zfail, zpass );
}

struct ptrstate vertex_array;
struct ptrstate color_array;
struct ptrstate texture_coord_array;

void GL_MANGLE(glDrawElements)( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	if( skipnanogl )
	{
		glEsImpl->glDrawElements( mode, count, type, indices );
		return;
	}
	// ensure that all primitives specified between glBegin/glEnd pairs
	// are rendered first, and that we have correct tmu in use..
	FlushOnStateChange( );
	// setup correct vertex/color/texcoord pointers
	if ( arraysValid ||
	     tmuState0.vertex_array.changed ||
	     tmuState0.color_array.changed ||
	     tmuState0.texture_coord_array.changed || tmuState0.normal_array.changed )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
	}
	if ( arraysValid || tmuState0.vertex_array.changed )
	{
		if ( tmuState0.vertex_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_VERTEX_ARRAY );
		}
		glEsImpl->glVertexPointer( tmuState0.vertex_array.size,
		                           tmuState0.vertex_array.type,
		                           tmuState0.vertex_array.stride,
		                           tmuState0.vertex_array.ptr );
		tmuState0.vertex_array.changed = GL_FALSE;
	}
	if ( arraysValid || tmuState0.color_array.changed )
	{
		if ( tmuState0.color_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
			glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
					currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );

		}
		glEsImpl->glColorPointer( tmuState0.color_array.size,
		                          tmuState0.color_array.type,
		                          tmuState0.color_array.stride,
		                          tmuState0.color_array.ptr );
		tmuState0.color_array.changed = GL_FALSE;
	}
	if ( arraysValid || tmuState0.normal_array.changed )
	{
		if ( tmuState0.normal_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_NORMAL_ARRAY );
		}
		glEsImpl->glNormalPointer( tmuState0.normal_array.type,
		                           tmuState0.normal_array.stride,
		                           tmuState0.normal_array.ptr );
		tmuState0.normal_array.changed = GL_FALSE;
	}
	if ( arraysValid || tmuState0.texture_coord_array.changed )
	{
		tmuState0.texture_coord_array.changed = GL_FALSE;
		if ( tmuState0.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState0.texture_coord_array.size,
		                             tmuState0.texture_coord_array.type,
		                             tmuState0.texture_coord_array.stride,
		                             tmuState0.texture_coord_array.ptr );
	}

	if ( arraysValid || tmuState1.texture_coord_array.changed )
	{
		tmuState1.texture_coord_array.changed = GL_FALSE;
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		if ( tmuState1.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState1.texture_coord_array.size,
		                             tmuState1.texture_coord_array.type,
		                             tmuState1.texture_coord_array.stride,
		                             tmuState1.texture_coord_array.ptr );
	}

	arraysValid = GL_FALSE;
	glEsImpl->glDrawElements( mode, count, type, indices );
}

bool vboarray;

void GL_MANGLE(glEnableClientState)( GLenum array )
{
	if( skipnanogl )
	{
		glEsImpl->glEnableClientState( array );
		if( array == GL_VERTEX_ARRAY )
			vboarray = true;
		return;
	}
	struct nanotmuState *clientstate = NULL;
	if ( clientactivetmu == GL_TEXTURE0 )
	{
		clientstate = &tmuState0;
	}
	else if ( clientactivetmu == GL_TEXTURE1 )
	{
		clientstate = &tmuState1;
	}
	else
	{
		return;
	}
	switch ( array )
	{
	case GL_VERTEX_ARRAY:
		if ( clientstate->vertex_array.enabled )
		{
			return;
		}
		clientstate->vertex_array.enabled = GL_TRUE;
		clientstate->vertex_array.changed = GL_TRUE;
		break;
	case GL_COLOR_ARRAY:
		if ( clientstate->color_array.enabled )
		{
			return;
		}
		clientstate->color_array.enabled = GL_TRUE;
		clientstate->color_array.changed = GL_TRUE;

		break;
	case GL_NORMAL_ARRAY:
		if ( clientstate->normal_array.enabled )
		{
			return;
		}
		clientstate->normal_array.enabled = GL_TRUE;
		clientstate->normal_array.changed = GL_TRUE;

		break;
	case GL_TEXTURE_COORD_ARRAY:
		if ( clientstate->texture_coord_array.enabled )
		{
			return;
		}
		clientstate->texture_coord_array.enabled = GL_TRUE;
		clientstate->texture_coord_array.changed = GL_TRUE;
		break;
	default:
		break;
	}
}
void GL_MANGLE(glDisableClientState)( GLenum array )
{
	if( skipnanogl )
	{
		glEsImpl->glDisableClientState( array );
		if( array == GL_VERTEX_ARRAY )
			vboarray = false;
		return;
	}
	struct nanotmuState *clientstate = NULL;
	if ( clientactivetmu == GL_TEXTURE0 )
	{
		clientstate = &tmuState0;
	}
	else if ( clientactivetmu == GL_TEXTURE1 )
	{
		clientstate = &tmuState1;
	}
	else
	{
		return;
	}
	switch ( array )
	{
	case GL_VERTEX_ARRAY:
		if ( !clientstate->vertex_array.enabled )
		{
			return;
		}
		clientstate->vertex_array.enabled = GL_FALSE;
		clientstate->vertex_array.changed = GL_TRUE;
		break;
	case GL_COLOR_ARRAY:
		if ( !clientstate->color_array.enabled )
		{
			return;
		}
		clientstate->color_array.enabled = GL_FALSE;
		clientstate->color_array.changed = GL_TRUE;

		break;
	case GL_NORMAL_ARRAY:
		if ( !clientstate->normal_array.enabled )
		{
			return;
		}
		clientstate->normal_array.enabled = GL_FALSE;
		clientstate->normal_array.changed = GL_TRUE;

		break;
	case GL_TEXTURE_COORD_ARRAY:
		if ( !clientstate->texture_coord_array.enabled )
		{
			return;
		}
		clientstate->texture_coord_array.enabled = GL_FALSE;
		clientstate->texture_coord_array.changed = GL_TRUE;
		break;
	default:
		break;
	}
}
void GL_MANGLE(glVertexPointer)( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{

	if( skipnanogl )
	{
		glEsImpl->glVertexPointer( size, type, stride, pointer );
		return;
	}
	if ( tmuState0.vertex_array.size == size &&
	     tmuState0.vertex_array.stride == stride &&
	     tmuState0.vertex_array.type == type &&
	     tmuState0.vertex_array.ptr == pointer )
	{
		return;
	}
	tmuState0.vertex_array.size    = size;
	tmuState0.vertex_array.stride  = stride;
	tmuState0.vertex_array.type    = type;
	tmuState0.vertex_array.ptr     = (GLvoid *)pointer;
	tmuState0.vertex_array.changed = GL_TRUE;
}
void GL_MANGLE(glTexCoordPointer)( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	if( skipnanogl )
	{
		glEsImpl->glTexCoordPointer( size, type, stride, pointer );
		return;
	}
	struct nanotmuState *clientstate = NULL;
	if ( clientactivetmu == GL_TEXTURE0 )
	{
		clientstate = &tmuState0;
	}
	else if ( clientactivetmu == GL_TEXTURE1 )
	{
		clientstate = &tmuState1;
	}
	if ( clientstate->texture_coord_array.size == size &&
	     clientstate->texture_coord_array.stride == stride &&
	     clientstate->texture_coord_array.type == type &&
	     clientstate->texture_coord_array.ptr == pointer )
	{
		return;
	}
	clientstate->texture_coord_array.size    = size;
	clientstate->texture_coord_array.stride  = stride;
	clientstate->texture_coord_array.type    = type;
	clientstate->texture_coord_array.ptr     = (GLvoid *)pointer;
	clientstate->texture_coord_array.changed = GL_TRUE;
}
void GL_MANGLE(glColorPointer)( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	if ( tmuState0.color_array.size == size &&
	     tmuState0.color_array.stride == stride &&
	     tmuState0.color_array.type == type &&
	     tmuState0.color_array.ptr == pointer )
	{
		return;
	}
	tmuState0.color_array.size    = size;
	tmuState0.color_array.stride  = stride;
	tmuState0.color_array.type    = type;
	tmuState0.color_array.ptr     = (GLvoid *)pointer;
	tmuState0.color_array.changed = GL_TRUE;
}

void GL_MANGLE(glNormalPointer)( GLenum type, GLsizei stride, const GLvoid *pointer )
{
	int size = 0;
	if ( tmuState0.normal_array.size == size &&
	     tmuState0.normal_array.stride == stride &&
	     tmuState0.normal_array.type == type &&
	     tmuState0.normal_array.ptr == pointer )
	{
		return;
	}
	tmuState0.normal_array.size    = size;
	tmuState0.normal_array.stride  = stride;
	tmuState0.normal_array.type    = type;
	tmuState0.normal_array.ptr     = (GLvoid *)pointer;
	tmuState0.normal_array.changed = GL_TRUE;
}
void GL_MANGLE(glPolygonOffset)( GLfloat factor, GLfloat units )
{
	FlushOnStateChange( );
	glEsImpl->glPolygonOffset( factor, units );
}
void GL_MANGLE(glStencilMask)( GLuint mask )
{
	FlushOnStateChange( );
	glEsImpl->glStencilMask( mask );
}
void GL_MANGLE(glClearStencil)( GLint s )
{
	FlushOnStateChange( );
	glEsImpl->glClearStencil( s );
}

#if defined( __MULTITEXTURE_SUPPORT__ )

extern "C" void GL_MANGLE(glMultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t );

void GL_MANGLE(glMultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t )
{
	if ( target == GL_TEXTURE0 )
	{
		GL_MANGLE(glTexCoord2f)( s, t );
	}
	else
	{
		currentVertexAttrib.s_multi = s;
		currentVertexAttrib.t_multi = t;
	}
}

void GL_MANGLE(glMultiTexCoord3fARB)( GLenum a, GLfloat b, GLfloat c, GLfloat )
{
	return GL_MANGLE(glMultiTexCoord2fARB)( a, b, c );
}

void GL_MANGLE(glMultiTexCoord2f)( GLenum a, GLfloat b, GLfloat c )
{
	GL_MANGLE(glMultiTexCoord2fARB)(a,b,c);
}

#endif

/* Vladimir  */
/*void GL_MANGLE(glDrawArrays)( GLenum mode, int first, int count)
{
    FlushOnStateChange();
    glEsImpl->glDrawArrays(mode, first , count);
}*/
void GL_MANGLE(glMultMatrixf)( const GLfloat *m )
{
	FlushOnStateChange( );
	glEsImpl->glMultMatrixf( m );
}

void GL_MANGLE(glPixelStorei)( GLenum pname, GLint param )
{
	FlushOnStateChange( );
	glEsImpl->glPixelStorei( pname, param );
}

void GL_MANGLE(glFogi)( GLenum pname, GLint param )
{
	FlushOnStateChange( );
	glEsImpl->glFogf( pname, param );
}

void GL_MANGLE(glFogf)( GLenum pname, GLfloat param )
{
	FlushOnStateChange( );
	glEsImpl->glFogf( pname, param );
}

void GL_MANGLE(glFogfv)( GLenum pname, const GLfloat *params )
{
	FlushOnStateChange( );
	glEsImpl->glFogfv( pname, params );
}

void GL_MANGLE(glGetTexParameteriv)( GLenum target, GLenum pname, GLint *params )
{
	FlushOnStateChange( );
	glEsImpl->glGetTexParameteriv( target, pname, params );
}

// This gives: called unimplemented OpenGL ES API (Android)
void GL_MANGLE(glTexParameteri)( GLenum target, GLenum pname, GLint param )
{
	if ( pname == GL_TEXTURE_BORDER_COLOR )
	{
		return; // not supported by opengl es
	}
	if ( ( pname == GL_TEXTURE_WRAP_S ||
	       pname == GL_TEXTURE_WRAP_T ) &&
	     param == GL_CLAMP )
	{
		param = 0x812F;
	}

	FlushOnStateChange( );
	glEsImpl->glTexParameteri( target, pname, param );
}

void GL_MANGLE(glTexParameterx)( GLenum target, GLenum pname, GLfixed param )
{
	if ( pname == GL_TEXTURE_BORDER_COLOR )
	{
		return; // not supported by opengl es
	}
	if ( ( pname == GL_TEXTURE_WRAP_S ||
	       pname == GL_TEXTURE_WRAP_T ) &&
	     param == GL_CLAMP )
	{
		param = 0x812F;
	}
	FlushOnStateChange( );
	glEsImpl->glTexParameterx( target, pname, param );
}

void GL_MANGLE(glGenTextures)( GLsizei n, GLuint *textures )
{
	FlushOnStateChange( );
	glEsImpl->glGenTextures( n, textures );
}

void GL_MANGLE(glFrontFace)( GLenum mode )
{
	FlushOnStateChange( );
	glEsImpl->glFrontFace( mode );
}
// End Vladimir

void GL_MANGLE(glTexEnvi)( GLenum target, GLenum pname, GLint param )
{
	if( skipnanogl )
	{
		glEsImpl->glTexEnvi( target, pname, param );
		return;
	}
	if ( target == GL_TEXTURE_ENV )
	{
		if ( pname == GL_TEXTURE_ENV_MODE )
		{
			if ( param == activetmuState->texture_env_mode.value )
			{
				return;
			}
			else
			{
				FlushOnStateChange( );
				glEsImpl->glTexEnvi( target, pname, param );
				activetmuState->texture_env_mode.value = param;
				return;
			}
		}
	}
	FlushOnStateChange( );
	glEsImpl->glTexEnvi( target, pname, param );
}

void GL_MANGLE(glTexEnvfv)( GLenum target, GLenum pname, const GLfloat *param )
{
	if( skipnanogl )
	{
		glEsImpl->glTexEnvfv( target, pname, param );
		return;
	}

	FlushOnStateChange( );
	glEsImpl->glTexEnvfv( target, pname, param );
}

void GL_MANGLE(glDrawArrays)( GLenum mode, GLint first, GLsizei count )
{
	if( skipnanogl )
	{
		glEsImpl->glDrawArrays( mode, first, count );
		return;
	}
	// ensure that all primitives specified between glBegin/glEnd pairs
	// are rendered first, and that we have correct tmu in use..
	if ( mode == GL_QUADS )
		mode = GL_TRIANGLE_FAN;
	FlushOnStateChange( );
	// setup correct vertex/color/texcoord pointers
	if ( arraysValid ||
	     tmuState0.vertex_array.changed ||
	     tmuState0.color_array.changed ||
	     tmuState0.texture_coord_array.changed || tmuState0.normal_array.changed )
	{
		glEsImpl->glClientActiveTexture( GL_TEXTURE0 );
	}
	if ( arraysValid || tmuState0.vertex_array.changed )
	{
		if ( tmuState0.vertex_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_VERTEX_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_VERTEX_ARRAY );
		}
		glEsImpl->glVertexPointer( tmuState0.vertex_array.size,
		                           tmuState0.vertex_array.type,
		                           tmuState0.vertex_array.stride,
		                           tmuState0.vertex_array.ptr );
		tmuState0.vertex_array.changed = GL_FALSE;
	}
	if ( arraysValid || tmuState0.color_array.changed )
	{
		if ( tmuState0.color_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
			glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
					currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );

		}
		glEsImpl->glColorPointer( tmuState0.color_array.size,
		                          tmuState0.color_array.type,
		                          tmuState0.color_array.stride,
		                          tmuState0.color_array.ptr );
		tmuState0.color_array.changed = GL_FALSE;
	}
	if ( arraysValid || tmuState0.normal_array.changed )
	{
		if ( tmuState0.normal_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_NORMAL_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_NORMAL_ARRAY );
		}
		glEsImpl->glNormalPointer( tmuState0.normal_array.type,
		                           tmuState0.normal_array.stride,
		                           tmuState0.normal_array.ptr );
		tmuState0.normal_array.changed = GL_FALSE;
	}
	if ( arraysValid || tmuState0.texture_coord_array.changed )
	{
		tmuState0.texture_coord_array.changed = GL_FALSE;
		if ( tmuState0.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState0.texture_coord_array.size,
		                             tmuState0.texture_coord_array.type,
		                             tmuState0.texture_coord_array.stride,
		                             tmuState0.texture_coord_array.ptr );
	}

	if ( arraysValid || tmuState1.texture_coord_array.changed )
	{
		tmuState1.texture_coord_array.changed = GL_FALSE;
		glEsImpl->glClientActiveTexture( GL_TEXTURE1 );
		if ( tmuState1.texture_coord_array.enabled )
		{
			glEsImpl->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		else
		{
			glEsImpl->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glEsImpl->glTexCoordPointer( tmuState1.texture_coord_array.size,
		                             tmuState1.texture_coord_array.type,
		                             tmuState1.texture_coord_array.stride,
		                             tmuState1.texture_coord_array.ptr );
	}

	arraysValid = GL_FALSE;
	glEsImpl->glDrawArrays( mode, first, count );
}
/*void GL_MANGLE(glNormalPointer)(GLenum type, GLsizei stride, const void *ptr)
{
    glEsImpl->glNormalPointer( type, stride, ptr );
}*/

void GL_MANGLE(glCopyTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height )
{
	FlushOnStateChange( );
	glEsImpl->glCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

void GL_MANGLE(glGenFramebuffers)( GLsizei n, GLuint *framebuffers )
{
	FlushOnStateChange( );
	glEsImpl->glGenFramebuffers( n, framebuffers );
}

void GL_MANGLE(glGenRenderbuffers)( GLsizei n, GLuint *renderbuffers )
{
	FlushOnStateChange( );
	glEsImpl->glGenRenderbuffers( n, renderbuffers );
}

void GL_MANGLE(glBindRenderbuffer)( GLenum target, GLuint renderbuffer )
{
	FlushOnStateChange( );
	glEsImpl->glBindRenderbuffer( target, renderbuffer );
}

void GL_MANGLE(glBindFramebuffer)( GLenum target, GLuint framebuffer )
{
	FlushOnStateChange( );
	glEsImpl->glBindFramebuffer( target, framebuffer );
}

void GL_MANGLE(glFramebufferRenderbuffer)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer )
{
	FlushOnStateChange( );
	glEsImpl->glFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
}

void GL_MANGLE(glDeleteFramebuffers)( GLsizei n, const GLuint *framebuffers )
{
	FlushOnStateChange( );
	glEsImpl->glDeleteFramebuffers( n, framebuffers );
}

void GL_MANGLE(glDeleteRenderbuffers)( GLsizei n, const GLuint *renderbuffers )
{
	FlushOnStateChange( );
	glEsImpl->glDeleteRenderbuffers( n, renderbuffers );
}
void GL_MANGLE(glFramebufferTexture2D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level )
{
	FlushOnStateChange( );
	glEsImpl->glFramebufferTexture2D( target, attachment, textarget, texture, level );
}

void GL_MANGLE(glRenderbufferStorage)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height )
{
	FlushOnStateChange( );
	glEsImpl->glRenderbufferStorage( target, internalformat, width, height );
}

void GL_MANGLE(glBindBufferARB)( GLuint target, GLuint index )
{
	static int sindex;

	if( index && !sindex && !skipnanogl )
		FlushOnStateChange();
	glEsImpl->glDisableClientState( GL_COLOR_ARRAY );
	glEsImpl->glColor4f( currentVertexAttrib.red/255.0f, currentVertexAttrib.green/255.0f,
		 currentVertexAttrib.blue/255.0f, currentVertexAttrib.alpha/255.0f );

	if( target == 0x8892 )
	{
		skipnanogl = (!!index) || vboarray;

	}
	glEsImpl->glBindBuffer( target, index );
	if( sindex && !index )
	{
		arraysValid = GL_FALSE;
		if(!skipnanogl)
		{
			glEsImpl->glEnableClientState( GL_COLOR_ARRAY );
			tmuState0.color_array.changed = 1;
			tmuState1.color_array.changed = 1;
			tmuState0.texture_coord_array.changed = 1;
			tmuState1.texture_coord_array.changed = 1;
			arraysValid = 0;
			ResetNanoState( );
		}
	}
	sindex = index;
}

void GL_MANGLE(glGenBuffersARB)( GLuint count, GLuint *indexes )
{
	glEsImpl->glGenBuffers( count, indexes );
}

void GL_MANGLE(glDeleteBuffersARB)( GLuint count, GLuint *indexes )
{
	glEsImpl->glDeleteBuffers( count, indexes );
}

void GL_MANGLE(glBufferDataARB)( GLuint target, GLuint size, void *buffer, GLuint type )
{
	if(type == 0x88E0)
		type = 0x88E8;
	glEsImpl->glBufferData( target, size, buffer, type );
}

void GL_MANGLE(glBufferSubDataARB)( GLuint target, GLsizei offset, GLsizei size, void *buffer )
{
	glEsImpl->glBufferSubData( target, offset, size, buffer );
}

GLboolean GL_MANGLE(glIsEnabled)( GLenum cap )
{
	return glEsImpl->glIsEnabled( cap );
}

void GL_MANGLE(glDebugMessageControlARB)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled )
{
	if( glEsImpl->glDebugMessageControlKHR )
		glEsImpl->glDebugMessageControlKHR( source, type, severity, count, ids, enabled );
}

void GL_MANGLE(glDebugMessageInsertARB)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf )
{
	if( glEsImpl->glDebugMessageInsertKHR )
		glEsImpl->glDebugMessageInsertKHR( source, type, id, severity, length, buf );
}

void GL_MANGLE(glDebugMessageCallbackARB)( GL_DEBUG_PROC_ARB callback, void* userParam )
{
	if( glEsImpl->glDebugMessageCallbackKHR )
		glEsImpl->glDebugMessageCallbackKHR( callback, userParam );
}

GLuint GL_MANGLE(glGetDebugMessageLogARB)( GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLuint* severities, GLsizei* lengths, char* messageLog )
{
	if( glEsImpl->glGetDebugMessageLogKHR )
		return glEsImpl->glGetDebugMessageLogKHR( count, bufsize, sources, types, ids, severities, lengths, messageLog );
	return 0;
}
