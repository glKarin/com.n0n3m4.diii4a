/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "tr_local.h"


/*

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

typedef struct {
	int i2;
	int facing;
} edgeDef_t;

#define MAX_EDGE_DEFS   32

static edgeDef_t edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static int numEdgeDefs[SHADER_MAX_VERTEXES];
static int facing[SHADER_MAX_INDEXES / 3];
#if !defined(USE_OPENGLES) //karin: unused on OpenGLES
static vec3_t shadowXyz[SHADER_MAX_VERTEXES];
#endif

#ifdef USE_OPENGLES

extern cvar_t *harm_r_stencilShadowMask;
extern cvar_t *harm_r_stencilShadowOp;
extern cvar_t *harm_r_stencilShadowDebug;

#if 1
#define stencilIncr GL_INCR
#define stencilDecr GL_DECR
#else
GLenum stencilIncr = GL_INCR;
GLenum stencilDecr = GL_DECR;
#endif

#ifdef USE_SHADOW_INF
extern cvar_t *harm_r_stencilShadowInfinite;
#endif

#ifdef USE_SHADOW_XYZ
#ifdef USE_SHADOW_INF
typedef vec4_t shadow_vec_t;
#else
typedef vec3_t shadow_vec_t;
#endif
static	shadow_vec_t		shadowXyz[SHADER_MAX_VERTEXES * 2]; //karin: RB_EndSurface() - SHADER_MAX_INDEXES hit
typedef unsigned int shadowIndex_t;
#define GL_SHADOW_INDEX_TYPE GL_UNSIGNED_INT
#else
typedef glIndex_t shadowIndex_t;
#define GL_SHADOW_INDEX_TYPE GL_INDEX_TYPE
#endif
static shadowIndex_t indexes[6*MAX_EDGE_DEFS*SHADER_MAX_VERTEXES];
static int idx = 0;

static shadowIndex_t front_cap_indexes[SHADER_MAX_INDEXES];
static int front_cap_idx = 0;

static shadowIndex_t far_cap_indexes[SHADER_MAX_INDEXES];
static int far_cap_idx = 0;
extern cvar_t *harm_r_stencilShadowCap;

extern cvar_t *harm_r_shadowPolygonOffset;
extern cvar_t *harm_r_shadowPolygonFactor;
#endif

void R_AddEdgeDef( int i1, int i2, int facing ) {
	int c;

	c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;     // overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = facing;

	numEdgeDefs[ i1 ]++;
}

void R_RenderShadowEdges( void ) {
	int i;

#if 0
	int numTris;

	// dumb way -- render every triangle's edges
	numTris = tess.numIndexes / 3;

	for ( i = 0 ; i < numTris ; i++ ) {
		int i1, i2, i3;

		if ( !facing[i] ) {
			continue;
		}

		i1 = tess.indexes[ i * 3 + 0 ];
		i2 = tess.indexes[ i * 3 + 1 ];
		i3 = tess.indexes[ i * 3 + 2 ];

		qglBegin( GL_TRIANGLE_STRIP );
		qglVertex3fv( tess.xyz[ i1 ] );
		qglVertex3fv( shadowXyz[ i1 ] );
		qglVertex3fv( tess.xyz[ i2 ] );
		qglVertex3fv( shadowXyz[ i2 ] );
		qglVertex3fv( tess.xyz[ i3 ] );
		qglVertex3fv( shadowXyz[ i3 ] );
		qglVertex3fv( tess.xyz[ i1 ] );
		qglVertex3fv( shadowXyz[ i1 ] );
		qglEnd();
	}
#else
	int c, c2;
	int j, k;
	int i2;
	int c_edges, c_rejected;
	int hit[2];
#ifdef USE_OPENGLES
	idx = 0;
#endif

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
	c_edges = 0;
	c_rejected = 0;

	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		c = numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[ i ][ j ].i2;
			c2 = numEdgeDefs[ i2 ];
			for ( k = 0 ; k < c2 ; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i ) {
					hit[ edgeDefs[ i2 ][ k ].facing ]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if ( hit[ 1 ] == 0 ) {
#ifdef USE_OPENGLES
				// A single drawing call is better than many. So I prefer a singe TRIANGLES call than many TRAINGLE_STRIP call
				// even if it seems less efficiant, it's faster on the PANDORA
				indexes[idx++] = i;
				indexes[idx++] = i + tess.numVertexes;
				indexes[idx++] = i2;
				indexes[idx++] = i2;
				indexes[idx++] = i + tess.numVertexes;
				indexes[idx++] = i2 + tess.numVertexes;
#else
				qglBegin( GL_TRIANGLE_STRIP );
				qglVertex3fv( tess.xyz[ i ] );
				qglVertex3fv( shadowXyz[ i ] );
				qglVertex3fv( tess.xyz[ i2 ] );
				qglVertex3fv( shadowXyz[ i2 ] );
				qglEnd();
#endif
				c_edges++;
			} else {
				c_rejected++;
			}
		}
	}

#ifdef USE_OPENGLES
	qglDrawElements(GL_TRIANGLES, idx, GL_SHADOW_INDEX_TYPE, indexes);
#endif

#endif
}

#ifdef USE_OPENGLES
static void RB_ShadowDebug( void ) {
	if(!harm_r_stencilShadowDebug->integer)
		return;

	qboolean useZFail = harm_r_stencilShadowOp->integer == 2
						|| ((backEnd.currentEntity->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal) // personal
	;
	qboolean useCaps = useZFail || harm_r_stencilShadowCap->integer;
	if(harm_r_stencilShadowDebug->integer == 1 && idx == 0)
	{
		Com_Printf("No edges\n");
		return;
	}
	if(harm_r_stencilShadowDebug->integer & (2 | 4))
	{
		if(!useCaps)
			return;
		if(harm_r_stencilShadowDebug->integer == 2 && front_cap_idx == 0)
		{
			Com_Printf("No front caps\n");
			return;
		}
		if(harm_r_stencilShadowDebug->integer == 4 && far_cap_idx == 0)
		{
			Com_Printf("No far caps\n");
			return;
		}
	}
	GLfloat edgeColor[4];
	GLfloat frontCapColor[4];
	GLfloat farCapColor[4];
	GLfloat alpha = 1.0f;
	if(useZFail)
	{
		edgeColor[0] = 1.0f; edgeColor[1] = 1.0f; edgeColor[2] = 0.0f; edgeColor[3] = alpha;
		frontCapColor[0] = 0.0f; frontCapColor[1] = 1.0f; frontCapColor[2] = 1.0f; frontCapColor[3] = alpha;
		farCapColor[0] = 1.0f; farCapColor[1] = 0.0f; farCapColor[2] = 1.0f; farCapColor[3] = alpha;
	}
	else
	{
		edgeColor[0] = 1.0f; edgeColor[1] = 0.0f; edgeColor[2] = 0.0f; edgeColor[3] = alpha;
		frontCapColor[0] = 0.0f; frontCapColor[1] = 1.0f; frontCapColor[2] = 0.0f; frontCapColor[3] = alpha;
		farCapColor[0] = 0.0f; farCapColor[1] = 0.0f; farCapColor[2] = 1.0f; farCapColor[3] = alpha;
	}

#ifdef USE_SHADOW_XYZ
#ifdef USE_SHADOW_INF
	if(harm_r_stencilShadowInfinite->integer < 0)
		qglVertexPointer (4, GL_FLOAT, 0, shadowXyz);
	else
		qglVertexPointer (3, GL_FLOAT, 16, shadowXyz);
#else
	qglVertexPointer (3, GL_FLOAT, 0, shadowXyz);
#endif
#else
	#ifdef USE_SHADOW_INF
	if(harm_r_stencilShadowInfinite->integer < 0)
		qglVertexPointer (4, GL_FLOAT, 0, tess.xyz);
	else
		qglVertexPointer (3, GL_FLOAT, 16, tess.xyz);
#else
	qglVertexPointer (3, GL_FLOAT, 16, tess.xyz);
#endif
#endif

	int faceCulling = glState.faceCulling;
	unsigned long glStateBits = glState.glStateBits;
	GLfloat color[4];
	GLboolean clipPlane0;
	GLboolean depthTest;
    GLboolean stencilTest;
	GLboolean writeDepth;
	GLboolean rgba[4];
	GLboolean blend;

	depthTest = qglIsEnabled(GL_DEPTH_TEST);
	blend = qglIsEnabled(GL_BLEND);
    stencilTest = qglIsEnabled(GL_STENCIL_TEST);
	clipPlane0 = qglIsEnabled(GL_CLIP_PLANE0);
	glGetFloatv(GL_CURRENT_COLOR, color);
	qglGetBooleanv(GL_DEPTH_WRITEMASK, &writeDepth);
	qglGetBooleanv(GL_COLOR_WRITEMASK, rgba);
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	if(writeDepth)
		qglDepthMask(GL_FALSE);
//	if(depthTest)
//		qglDisable(GL_DEPTH_TEST);
    if(stencilTest)
        qglDisable(GL_STENCIL_TEST);
	if(blend)
		qglDisable(GL_BLEND);

	if(clipPlane0)
		qglDisable( GL_CLIP_PLANE0 );
	//GL_Cull( CT_TWO_SIDED );
	GL_Cull( CT_BACK_SIDED );

	GL_Bind( tr.whiteImage );

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

//	qglColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	if((harm_r_stencilShadowDebug->integer & 1) && idx != 0)
	{
		qglColor4f(edgeColor[0], edgeColor[1], edgeColor[2], edgeColor[3]);
		qglDrawElements(GL_TRIANGLES, idx, GL_SHADOW_INDEX_TYPE, indexes);
	}
	if((harm_r_stencilShadowDebug->integer & 2) && front_cap_idx != 0)
	{
		qglColor4f(frontCapColor[0], frontCapColor[1], frontCapColor[2], frontCapColor[3]);
		qglDrawElements(GL_TRIANGLES, front_cap_idx, GL_SHADOW_INDEX_TYPE, front_cap_indexes);
	}
	if((harm_r_stencilShadowDebug->integer & 4) && far_cap_idx != 0)
	{
		qglColor4f(farCapColor[0], farCapColor[1], farCapColor[2], farCapColor[3]);
		qglDrawElements(GL_TRIANGLES, far_cap_idx, GL_SHADOW_INDEX_TYPE, far_cap_indexes);
	}

	GL_State( glStateBits );
	GL_Cull( faceCulling );
	qglColor4f( color[0], color[1], color[2], color[3] );
	qglColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);
	if(writeDepth)
		qglDepthMask(GL_TRUE);
	if(clipPlane0)
		qglEnable( GL_CLIP_PLANE0 );
	if(depthTest)
		qglEnable(GL_DEPTH_TEST);
    if(stencilTest)
        qglEnable(GL_STENCIL_TEST);
	if(blend)
		qglEnable(GL_BLEND);
}

static ID_INLINE void R_RenderShadowCaps( void )
{
	if(front_cap_idx > 0)
		qglDrawElements(GL_TRIANGLES, front_cap_idx, GL_SHADOW_INDEX_TYPE, front_cap_indexes);
	if(far_cap_idx > 0)
		qglDrawElements(GL_TRIANGLES, far_cap_idx, GL_SHADOW_INDEX_TYPE, far_cap_indexes);
}

static void RB_BeginShadow( void ) {
	//qglClearStencil(1<<(glConfig.stencilBits-1)); // 128

	qglClear(GL_STENCIL_BUFFER_BIT);
}

static void RB_ShadowMask( void ) {
	int faceCulling = glState.faceCulling;
	unsigned long glStateBits = glState.glStateBits;
	GLfloat color[4];
	GLboolean clipPlane0;
	GLboolean depthTest;
	GLboolean writeDepth;
	GLboolean rgba[4];

	depthTest = qglIsEnabled(GL_DEPTH_TEST);
	clipPlane0 = qglIsEnabled(GL_CLIP_PLANE0);
	glGetFloatv(GL_CURRENT_COLOR, color);
	qglGetBooleanv(GL_DEPTH_WRITEMASK, &writeDepth);
	qglGetBooleanv(GL_COLOR_WRITEMASK, rgba);
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	if(writeDepth)
		qglDepthMask(GL_FALSE);
	if(depthTest)
		qglDisable(GL_DEPTH_TEST);

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_NOTEQUAL, 0, 255 );

	if(clipPlane0)
		qglDisable( GL_CLIP_PLANE0 );
	GL_Cull( CT_TWO_SIDED );

	GL_Bind( tr.whiteImage );

	qglPushMatrix();
	qglLoadIdentity();

	qglColor3f( 0.6f, 0.6f, 0.6f );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	qglColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
#if 0
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (text)
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		qglDisableClientState( GL_COLOR_ARRAY );
#endif
	GLfloat vtx[] = {
			-100,  100, -10,
			100,  100, -10,
			100, -100, -10,
			-100, -100, -10
	};
	qglVertexPointer  ( 3, GL_FLOAT, 0, vtx );
	qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
#if 0
	if (text)
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		qglEnableClientState( GL_COLOR_ARRAY );
#endif

	qglDisable( GL_STENCIL_TEST );

	qglPopMatrix();
	GL_State( glStateBits );
	GL_Cull( faceCulling );
	qglColor4f( color[0], color[1], color[2], color[3] );
	qglColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);
	if(writeDepth)
		qglDepthMask(GL_TRUE);
	if(clipPlane0)
		qglEnable( GL_CLIP_PLANE0 );
	if(depthTest)
		qglEnable(GL_DEPTH_TEST);
}
#endif
/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd( void ) {
	int i;
	int numTris;
	vec3_t lightDir;
	GLboolean rgba[4];

	if ( glConfig.stencilBits < 4 ) {
		return;
	}

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );

#ifdef USE_OPENGLES //karin: use shadowXyz for stencil shadow
	qboolean useZFail = harm_r_stencilShadowOp->integer == 2
			|| ((backEnd.currentEntity->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal) // personal
			;
	qboolean useCaps = useZFail || harm_r_stencilShadowCap->integer;

	front_cap_idx = 0;
	far_cap_idx = 0;

#ifdef USE_SHADOW_INF
	float volumeLength;
	if(harm_r_stencilShadowInfinite->integer > 0)
		volumeLength = harm_r_stencilShadowInfinite->integer;
	else if(harm_r_stencilShadowInfinite->integer < 0)
		volumeLength = -harm_r_stencilShadowInfinite->integer;
	else
		volumeLength = 512;
#else
#define volumeLength 512
#endif

	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
#ifdef USE_SHADOW_XYZ
		VectorCopy( tess.xyz[i], shadowXyz[i] );
#ifdef USE_SHADOW_INF
		shadowXyz[i][3] = 1.0f;
#endif
		VectorMA( tess.xyz[i], -volumeLength, lightDir, shadowXyz[i+tess.numVertexes] );
#ifdef USE_SHADOW_INF
		shadowXyz[i+tess.numVertexes][3] = 0.0f;
#endif
#else
		VectorMA( tess.xyz[i], -volumeLength, lightDir, tess.xyz[i+tess.numVertexes] );
#ifdef USE_SHADOW_INF
		tess.xyz[i][3] = 1.0f; // !!! need ???
		tess.xyz[i+tess.numVertexes][3] = 0.0f;
#endif
#endif
	}
#else
	// project vertexes away from light direction
	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		VectorMA( tess.xyz[i], -512, lightDir, shadowXyz[i] );
	}
#endif

	// decide which triangles face the light
	memset( numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ ) {
		int i1, i2, i3;
		vec3_t d1, d2, normal;
		float   *v1, *v2, *v3;
		float d;

		i1 = tess.indexes[ i * 3 + 0 ];
		i2 = tess.indexes[ i * 3 + 1 ];
		i3 = tess.indexes[ i * 3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		d = DotProduct( normal, lightDir );
		if ( d > 0 ) { // back CCW
			facing[ i ] = 1;
			if(useCaps && harm_r_stencilShadowCap->integer != 2)
			{
				far_cap_indexes[ far_cap_idx + 0 ] = tess.numVertexes + i1;
				far_cap_indexes[ far_cap_idx + 1 ] = tess.numVertexes + i3;
				far_cap_indexes[ far_cap_idx + 2 ] = tess.numVertexes + i2;
				far_cap_idx += 3;
			}
		} else { // front CW
			facing[ i ] = 0;
			if(useCaps)
			{
				front_cap_indexes[ front_cap_idx + 0 ] = i1;
				front_cap_indexes[ front_cap_idx + 1 ] = i3;
				front_cap_indexes[ front_cap_idx + 2 ] = i2;
				front_cap_idx += 3;

				if(harm_r_stencilShadowCap->integer == 2)
				{
					far_cap_indexes[ far_cap_idx + 0 ] = tess.numVertexes + i1;
					far_cap_indexes[ far_cap_idx + 1 ] = tess.numVertexes + i2;
					far_cap_indexes[ far_cap_idx + 2 ] = tess.numVertexes + i3;
					far_cap_idx += 3;
				}
			}
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	// draw the silhouette edges

	GL_Bind( tr.whiteImage );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	qglColor3f( 0.2f, 0.2f, 0.2f );

	// don't write to the color buffer
	qglGetBooleanv(GL_COLOR_WRITEMASK, rgba);
	qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

#ifdef USE_OPENGLES
	if(harm_r_stencilShadowMask->integer)
		RB_BeginShadow();
#ifdef USE_SHADOW_XYZ
#ifdef USE_SHADOW_INF
	if(harm_r_stencilShadowInfinite->integer < 0)
		qglVertexPointer (4, GL_FLOAT, 0, shadowXyz);
	else
		qglVertexPointer (3, GL_FLOAT, 16, shadowXyz);
#else
	qglVertexPointer (3, GL_FLOAT, 0, shadowXyz);
#endif
#else
#ifdef USE_SHADOW_INF
	if(harm_r_stencilShadowInfinite->integer < 0)
		qglVertexPointer (4, GL_FLOAT, 0, tess.xyz);
	else
		qglVertexPointer (3, GL_FLOAT, 16, tess.xyz);
#else
	qglVertexPointer (3, GL_FLOAT, 16, tess.xyz);
#endif
#endif
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (text)
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		qglDisableClientState( GL_COLOR_ARRAY );
#endif

	if(useZFail)
	{
		GL_Cull( CT_FRONT_SIDED );
		qglStencilOp(GL_KEEP, stencilIncr, GL_KEEP);
	}
	else
	{
		GL_Cull( CT_BACK_SIDED );
		qglStencilOp( GL_KEEP, GL_KEEP, stencilIncr );
	}

	qboolean setupPolygonOffset = harm_r_shadowPolygonOffset->value || harm_r_shadowPolygonFactor->value;
	GLboolean polygonOffset = qfalse;
	GLfloat polygonOffsetFactor = 0.0f;
	GLfloat polygonOffsetUnits = 0.0f;
	if(setupPolygonOffset)
	{
		polygonOffset = qglIsEnabled( GL_POLYGON_OFFSET_FILL );
		if(!polygonOffset)
			qglEnable( GL_POLYGON_OFFSET_FILL );
		glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &polygonOffsetFactor);
		glGetFloatv(GL_POLYGON_OFFSET_UNITS, &polygonOffsetUnits);

		qglPolygonOffset( harm_r_shadowPolygonFactor->value, -harm_r_shadowPolygonOffset->value );
	}

	if(useCaps)
		R_RenderShadowCaps();
	R_RenderShadowEdges();

	if(useZFail)
	{
		GL_Cull( CT_BACK_SIDED );
		qglStencilOp( GL_KEEP, stencilDecr, GL_KEEP );
	}
	else
	{
		GL_Cull( CT_FRONT_SIDED );
		qglStencilOp( GL_KEEP, GL_KEEP, stencilDecr );
	}

	if(useCaps)
		R_RenderShadowCaps();
#ifdef USE_OPENGLES
	qglDrawElements(GL_TRIANGLES, idx, GL_SHADOW_INDEX_TYPE, indexes);
#else
	R_RenderShadowEdges();
#endif

	if(setupPolygonOffset)
	{
		if(!polygonOffset)
			qglDisable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( polygonOffsetFactor, polygonOffsetUnits );
	}

#ifdef USE_OPENGLES
	if(harm_r_stencilShadowMask->integer)
		RB_ShadowMask();

	RB_ShadowDebug(); //debug

	if (text)
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		qglEnableClientState( GL_COLOR_ARRAY );
#endif
	// reenable writing to the color buffer
	qglColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);
}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish( void ) {
#ifdef USE_OPENGLES
	if(harm_r_stencilShadowMask->integer)
		return;
#endif
	if ( r_shadows->integer != 2 ) {
		return;
	}
	if ( glConfig.stencilBits < 4 ) {
		return;
	}
	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_NOTEQUAL, 0, 255 );

	qglDisable( GL_CLIP_PLANE0 );
	GL_Cull( CT_TWO_SIDED );

	GL_Bind( tr.whiteImage );

	qglLoadIdentity();

	qglColor3f( 0.6f, 0.6f, 0.6f );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	qglColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

#ifdef USE_OPENGLES
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (text)
		qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		qglDisableClientState( GL_COLOR_ARRAY );
	GLfloat vtx[] = {
	 -100,  100, -10,
	  100,  100, -10,
	  100, -100, -10,
	 -100, -100, -10
	};
	qglVertexPointer  ( 3, GL_FLOAT, 0, vtx );
	qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	if (text)
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		qglEnableClientState( GL_COLOR_ARRAY );
#else
	qglBegin( GL_QUADS );
	qglVertex3f( -100, 100, -10 );
	qglVertex3f( 100, 100, -10 );
	qglVertex3f( 100, -100, -10 );
	qglVertex3f( -100, -100, -10 );
	qglEnd();
#endif

	qglColor4f( 1,1,1,1 );
	qglDisable( GL_STENCIL_TEST );
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform( void ) {
	float   *xyz;
	int i;
	float h;
	vec3_t ground;
	vec3_t light;
	float groundDist;
	float d;
	vec3_t lightDir;

	xyz = ( float * ) tess.xyz;

	ground[0] = backEnd.or.axis[0][2];
	ground[1] = backEnd.or.axis[1][2];
	ground[2] = backEnd.or.axis[2][2];

	groundDist = backEnd.or.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, ( 0.5 - d ), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		h = DotProduct( xyz, ground ) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
