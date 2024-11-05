/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file rendererGLES/tr_shadows.c
 */

#include "tr_local.h"

/*
  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]
*/

typedef struct
{
	int i2;
	int facing;
} edgeDef_t;

#define MAX_EDGE_DEFS   32

static edgeDef_t      edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static int            numEdgeDefs[SHADER_MAX_VERTEXES];
static int            facing[SHADER_MAX_INDEXES / 3];
static unsigned short indexes[6 * MAX_EDGE_DEFS * SHADER_MAX_VERTEXES];
static int            idx = 0;

#ifdef STENCIL_SHADOW_IMPROVE
#define FLOAT_ZERO 1e-6

#define SHADOW_CAP_NEAR_BACK_AND_FAR_BACK 1
#define SHADOW_CAP_NEAR_FRONT_AND_FAR_BACK 2
#define SHADOW_CAP_NEAR_FRONT_AND_FAR_FRONT 3

#define SHADOW_CAP_DEFAULT_IMPL SHADOW_CAP_NEAR_BACK_AND_FAR_BACK

extern cvar_t *harm_r_stencilShadowMask;
extern cvar_t *harm_r_stencilShadowOp;
extern cvar_t *harm_r_stencilShadowDebug;
extern cvar_t *harm_r_stencilShadowMaxAngle;

#if 1
#define stencilIncr GL_INCR
#define stencilDecr GL_DECR
#else
GLenum stencilIncr = GL_INCR;
GLenum stencilDecr = GL_DECR;
#endif

typedef vec4_t shadow_vec_t;
static	shadow_vec_t		shadowXyz[SHADER_MAX_VERTEXES * 2]; //karin: RB_EndSurface() - SHADER_MAX_INDEXES hit
typedef glIndex_t shadowIndex_t;
#define GL_SHADOW_INDEX_TYPE GL_INDEX_TYPE

static shadowIndex_t front_cap_indexes[SHADER_MAX_INDEXES];
static int front_cap_idx = 0;

static shadowIndex_t far_cap_indexes[SHADER_MAX_INDEXES];
static int far_cap_idx = 0;
extern cvar_t *harm_r_stencilShadowCap;

extern cvar_t *harm_r_shadowPolygonOffset;
extern cvar_t *harm_r_shadowPolygonFactor;

qboolean R_HasAlphaTest(const shader_t *shader)
{
	int mask = GLS_ATEST_BITS;
	int m;
	for(m = 0; m < MAX_SHADER_STAGES; m++)
	{
		const shaderStage_t *stage = shader->stages[m];
		if(!stage)
			break;
		if(stage->stateBits & mask)
		{
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/**
 * @brief R_AddEdgeDef
 * @param[in] i1
 * @param[in] i2
 * @param[in] facing
 */
void R_AddEdgeDef(int i1, int i2, int facing)
{
	int c = numEdgeDefs[i1];

	if (c == MAX_EDGE_DEFS)
	{
		return;     // overflow
	}
	edgeDefs[i1][c].i2     = i2;
	edgeDefs[i1][c].facing = facing;

	numEdgeDefs[i1]++;
}

/**
 * @brief R_RenderShadowEdges
 */
void R_RenderShadowEdges(void)
{
	int i;
	int c, c2;
	int j, k;
	int i2;
	// int c_edges = 0, c_rejected = 0;  // TODO: remove ?
	int hit[2];
    idx = 0; //karin: add reset idx

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges

	for (i = 0 ; i < tess.numVertexes ; i++)
	{
		c = numEdgeDefs[i];
		for (j = 0 ; j < c ; j++)
		{
			if (!edgeDefs[i][j].facing)
			{
				continue;
			}

			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[i][j].i2;
			c2 = numEdgeDefs[i2];
			for (k = 0 ; k < c2 ; k++)
			{
				if (edgeDefs[i2][k].i2 == i)
				{
					hit[edgeDefs[i2][k].facing]++;
#ifdef STENCIL_SHADOW_IMPROVE //karin: optmize
                    if( edgeDefs[ i2 ][ k ].facing )
						break;
#endif
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if (hit[1] == 0)
			{
				// OpenGLES implementation
				// A single drawing call is better than many. So I prefer a singe TRIANGLES call than many TRAINGLE_STRIP call
				// even if it seems less efficiant, it's faster on the PANDORA
				indexes[idx++] = i;
				indexes[idx++] = i + tess.numVertexes;
				indexes[idx++] = i2;
				indexes[idx++] = i2;
				indexes[idx++] = i + tess.numVertexes;
				indexes[idx++] = i2 + tess.numVertexes;
				// c_edges++;
			}
			/*
			else
			{
			    c_rejected++;
			}
			*/
		}
	}
	glDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_SHORT, indexes);
}

#ifdef STENCIL_SHADOW_IMPROVE
static void RB_ShadowDebug( void ) {
	if(!harm_r_stencilShadowDebug->integer)
		return;

	qboolean personalModel = ((backEnd.currentEntity->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal); // personal
	qboolean infinite = personalModel || harm_r_stencilShadowInfinite->integer < 0;
	qboolean useZFail = harm_r_stencilShadowOp->integer == 2 || personalModel;
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

	if(infinite)
		glVertexPointer (4, GL_FLOAT, 0, shadowXyz);
	else
		glVertexPointer (3, GL_FLOAT, 16, shadowXyz);

	int faceCulling = glState.faceCulling;
	unsigned long glStateBits = glState.glStateBits;
	GLfloat color[4];
	GLboolean clipPlane0;
	GLboolean depthTest;
    GLboolean stencilTest;
	GLboolean writeDepth;
	GLboolean rgba[4];
	GLboolean blend;

	depthTest = glIsEnabled(GL_DEPTH_TEST);
	blend = glIsEnabled(GL_BLEND);
    stencilTest = glIsEnabled(GL_STENCIL_TEST);
	clipPlane0 = glIsEnabled(GL_CLIP_PLANE0);
	glGetFloatv(GL_CURRENT_COLOR, color);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &writeDepth);
	glGetBooleanv(GL_COLOR_WRITEMASK, rgba);
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	if(writeDepth)
		glDepthMask(GL_FALSE);
//	if(depthTest)
//		glDisable(GL_DEPTH_TEST);
    if(stencilTest)
        glDisable(GL_STENCIL_TEST);
	if(blend)
		glDisable(GL_BLEND);

	if(clipPlane0)
		glDisable( GL_CLIP_PLANE0 );
	//GL_Cull( CT_TWO_SIDED );
	GL_Cull( CT_BACK_SIDED );

	GL_Bind( tr.whiteImage );

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

//	glColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	if((harm_r_stencilShadowDebug->integer & 1) && idx != 0)
	{
		glColor4f(edgeColor[0], edgeColor[1], edgeColor[2], edgeColor[3]);
		glDrawElements(GL_TRIANGLES, idx, GL_SHADOW_INDEX_TYPE, indexes);
	}
	if((harm_r_stencilShadowDebug->integer & 2) && front_cap_idx != 0)
	{
		glColor4f(frontCapColor[0], frontCapColor[1], frontCapColor[2], frontCapColor[3]);
		glDrawElements(GL_TRIANGLES, front_cap_idx, GL_SHADOW_INDEX_TYPE, front_cap_indexes);
	}
	if((harm_r_stencilShadowDebug->integer & 4) && far_cap_idx != 0)
	{
		glColor4f(farCapColor[0], farCapColor[1], farCapColor[2], farCapColor[3]);
		glDrawElements(GL_TRIANGLES, far_cap_idx, GL_SHADOW_INDEX_TYPE, far_cap_indexes);
	}

	GL_State( glStateBits );
	GL_Cull( faceCulling );
	glColor4f( color[0], color[1], color[2], color[3] );
	glColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);
	if(writeDepth)
		glDepthMask(GL_TRUE);
	if(clipPlane0)
		glEnable( GL_CLIP_PLANE0 );
	if(depthTest)
		glEnable(GL_DEPTH_TEST);
    if(stencilTest)
        glEnable(GL_STENCIL_TEST);
	if(blend)
		glEnable(GL_BLEND);
}

static ID_INLINE void R_RenderShadowCaps( void )
{
	if(front_cap_idx > 0)
		glDrawElements(GL_TRIANGLES, front_cap_idx, GL_SHADOW_INDEX_TYPE, front_cap_indexes);
	if(far_cap_idx > 0)
		glDrawElements(GL_TRIANGLES, far_cap_idx, GL_SHADOW_INDEX_TYPE, far_cap_indexes);
}

static void RB_BeginShadow( void ) {
	glClear(GL_STENCIL_BUFFER_BIT);
}

static void RB_ShadowMask( void ) {
	int faceCulling = glState.faceCulling;
	unsigned long glStateBits = glState.glStateBits;
	GLfloat color[4];
	GLboolean clipPlane0;
	GLboolean depthTest;
	GLboolean writeDepth;
	GLboolean rgba[4];

	depthTest = glIsEnabled(GL_DEPTH_TEST);
	clipPlane0 = glIsEnabled(GL_CLIP_PLANE0);
	glGetFloatv(GL_CURRENT_COLOR, color);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &writeDepth);
	glGetBooleanv(GL_COLOR_WRITEMASK, rgba);
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	if(writeDepth)
		glDepthMask(GL_FALSE);
	if(depthTest)
		glDisable(GL_DEPTH_TEST);

	glEnable( GL_STENCIL_TEST );
	glStencilFunc( GL_NOTEQUAL, 0, 255 );

	if(clipPlane0)
		glDisable( GL_CLIP_PLANE0 );
	GL_Cull( CT_TWO_SIDED );

	GL_Bind( tr.whiteImage );

	glPushMatrix();
	glLoadIdentity();

	glColor4f( 0.6f, 0.6f, 0.6f, 1.0f );
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	glColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
#if 0
	GLboolean text = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = glIsEnabled(GL_COLOR_ARRAY);
	if (text)
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		glDisableClientState( GL_COLOR_ARRAY );
#endif
	GLfloat vtx[] = {
			-100,  100, -10,
			100,  100, -10,
			100, -100, -10,
			-100, -100, -10
	};
	glVertexPointer  ( 3, GL_FLOAT, 0, vtx );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
#if 0
	if (text)
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		glEnableClientState( GL_COLOR_ARRAY );
#endif

	glDisable( GL_STENCIL_TEST );

	glPopMatrix();
	GL_State( glStateBits );
	GL_Cull( faceCulling );
	glColor4f( color[0], color[1], color[2], color[3] );
	glColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);
	if(writeDepth)
		glDepthMask(GL_TRUE);
	if(clipPlane0)
		glEnable( GL_CLIP_PLANE0 );
	if(depthTest)
		glEnable(GL_DEPTH_TEST);
}
#endif

/**
 * @brief RB_ShadowTessEnd
 *
 * @note triangleFromEdge[ v1 ][ v2 ]
 *
 * set triangle from edge( v1, v2, tri )
 * if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
 * }
 */
void RB_ShadowTessEnd(void)
{
	int    i;
	int    numTris;
	vec3_t lightDir;

	// we can only do this if we have enough space in the vertex buffers
#if !defined(STENCIL_SHADOW_IMPROVE) //karin: always enough
	if (tess.numVertexes >= SHADER_MAX_VERTEXES / 2)
	{
		return;
	}
#endif

	if (glConfig.stencilBits < 4)
	{
		return;
	}

#ifdef STENCIL_SHADOW_IMPROVE //karin: check invalid light direction
	if(
	    (backEnd.currentEntity->lightDir[0] < FLOAT_ZERO && backEnd.currentEntity->lightDir[0] > -FLOAT_ZERO)
	    && (backEnd.currentEntity->lightDir[1] < FLOAT_ZERO && backEnd.currentEntity->lightDir[1] > -FLOAT_ZERO)
	    && (backEnd.currentEntity->lightDir[2] < FLOAT_ZERO && backEnd.currentEntity->lightDir[2] > -FLOAT_ZERO)
	)
	{
        return;
	}
#endif

	VectorCopy(backEnd.currentEntity->lightDir, lightDir);

#ifdef STENCIL_SHADOW_IMPROVE //karin: use shadowXyz for stencil shadow
    static float r_stencilShadowDotP = -2.0;
    static int r_stencilShadowDeg = -1;
    if(r_stencilShadowDeg != harm_r_stencilShadowMaxAngle->integer)
    {
        r_stencilShadowDeg = harm_r_stencilShadowMaxAngle->integer;
        if(harm_r_stencilShadowMaxAngle->integer < 0)
        {
            r_stencilShadowDotP = -2.0;
        }
        else
        {
            r_stencilShadowDotP = cos(DEG2RAD(( (float) ( /*360 - */r_stencilShadowDeg % 360 ) )));
        }
    }
	//karin: check light is under model
	if(r_stencilShadowDeg >= 0)
	{
        vec3_t upz = { 0.0f, 0.0f, 1.0f };
        float zd = DotProduct( upz, lightDir ); // lightDir is model position to light source
        if(zd <= r_stencilShadowDotP)
            return;
	}

	qboolean personalModel = ((backEnd.currentEntity->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal); // personal
	qboolean infinite = personalModel || harm_r_stencilShadowInfinite->integer < 0;
	qboolean useZFail = harm_r_stencilShadowOp->integer == 2 || personalModel;
	qboolean useCaps = useZFail || harm_r_stencilShadowCap->integer;

	front_cap_idx = 0;
	far_cap_idx = 0;

	float volumeLength;
	if(harm_r_stencilShadowInfinite->integer > 0)
		volumeLength = harm_r_stencilShadowInfinite->integer;
	else if(harm_r_stencilShadowInfinite->integer < 0)
		volumeLength = -harm_r_stencilShadowInfinite->integer;
	else
		volumeLength = 512;

	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		VectorCopy( tess.xyz[i], shadowXyz[i] );
		shadowXyz[i][3] = 1.0f;
		VectorMA( tess.xyz[i], -volumeLength, lightDir, shadowXyz[i+tess.numVertexes] );
		shadowXyz[i+tess.numVertexes][3] = 0.0f;
	}
#else
	// project vertexes away from light direction
	for (i = 0 ; i < tess.numVertexes ; i++)
	{
		VectorMA(tess.xyz[i], -512, lightDir, tess.xyz[i + tess.numVertexes]);
	}
#endif

	// decide which triangles face the light
	Com_Memset(numEdgeDefs, 0, 4 * tess.numVertexes);

	numTris = tess.numIndexes / 3;

	{
		int    i1, i2, i3;
		vec3_t d1, d2, normal;
		float  *v1, *v2, *v3;
		float  d;

		for (i = 0 ; i < numTris ; i++)
		{
			i1 = tess.indexes[i * 3 + 0];
			i2 = tess.indexes[i * 3 + 1];
			i3 = tess.indexes[i * 3 + 2];

			v1 = tess.xyz[i1];
			v2 = tess.xyz[i2];
			v3 = tess.xyz[i3];

			VectorSubtract(v2, v1, d1);
			VectorSubtract(v3, v1, d2);
			CrossProduct(d1, d2, normal);

			d = DotProduct(normal, lightDir);
			if (d > 0)
			{
				facing[i] = 1;
#ifdef STENCIL_SHADOW_IMPROVE //karin: make cap for stencil shadow
                if(useCaps)
                {
                    // back as far cap
                    if(harm_r_stencilShadowCap->integer != 3)
                    {
                        far_cap_indexes[ far_cap_idx + 0 ] = tess.numVertexes + i1;
                        far_cap_indexes[ far_cap_idx + 1 ] = tess.numVertexes + i3;
                        far_cap_indexes[ far_cap_idx + 2 ] = tess.numVertexes + i2;
                        far_cap_idx += 3;
                    }
                    // back as near cap
                    if(harm_r_stencilShadowCap->integer != 2 && harm_r_stencilShadowCap->integer != 3)
                    {
                        front_cap_indexes[ front_cap_idx + 0 ] = i1;
                        front_cap_indexes[ front_cap_idx + 1 ] = i2;
                        front_cap_indexes[ front_cap_idx + 2 ] = i3;
                        front_cap_idx += 3;
                    }
                }
#endif
			}
			else
			{
				facing[i] = 0;
#ifdef STENCIL_SHADOW_IMPROVE //karin: make cap for stencil shadow
                if(useCaps)
                {
                    // front as near cap
                    if(harm_r_stencilShadowCap->integer == 2 || harm_r_stencilShadowCap->integer == 3)
                    {
                        front_cap_indexes[ front_cap_idx + 0 ] = i1;
                        front_cap_indexes[ front_cap_idx + 1 ] = i3;
                        front_cap_indexes[ front_cap_idx + 2 ] = i2;
                        front_cap_idx += 3;
                    }
                    // front as far cap
                    if(harm_r_stencilShadowCap->integer == 3)
                    {
                        far_cap_indexes[ far_cap_idx + 0 ] = tess.numVertexes + i1;
                        far_cap_indexes[ far_cap_idx + 1 ] = tess.numVertexes + i2;
                        far_cap_indexes[ far_cap_idx + 2 ] = tess.numVertexes + i3;
                        far_cap_idx += 3;
                    }
                }
#endif
			}

			// create the edges
			R_AddEdgeDef(i1, i2, facing[i]);
			R_AddEdgeDef(i2, i3, facing[i]);
			R_AddEdgeDef(i3, i1, facing[i]);
		}
	}


	// draw the silhouette edges

	GL_Bind(tr.whiteImage);
	glEnable(GL_CULL_FACE);
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);
	glColor4f(0.2f, 0.2f, 0.2f, 1.0f);

	// don't write to the color buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 255);

#ifdef STENCIL_SHADOW_IMPROVE
    if(harm_r_stencilShadowMask->integer)
		RB_BeginShadow();
	if(infinite)
		glVertexPointer (4, GL_FLOAT, 0, shadowXyz);
	else
		glVertexPointer (3, GL_FLOAT, 16, shadowXyz);
	GLboolean text = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = glIsEnabled(GL_COLOR_ARRAY);
	if (text)
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		glDisableClientState( GL_COLOR_ARRAY );

	// mirrors have the culling order reversed
	if (backEnd.viewParms.isMirror)
	{
        if(useZFail)
        {
            glCullFace(GL_BACK);
            glStencilOp(GL_KEEP, stencilIncr, GL_KEEP);
        }
        else
        {
            glCullFace(GL_FRONT);
            glStencilOp( GL_KEEP, GL_KEEP, stencilIncr );
        }

        if(useCaps)
            R_RenderShadowCaps();
		R_RenderShadowEdges();

        if(useZFail)
        {
            glCullFace(GL_FRONT);
            glStencilOp( GL_KEEP, stencilDecr, GL_KEEP );
        }
        else
        {
            glCullFace(GL_BACK);
            glStencilOp( GL_KEEP, GL_KEEP, stencilDecr );
        }

        if(useCaps)
            R_RenderShadowCaps();
        glDrawElements(GL_TRIANGLES, idx, GL_SHADOW_INDEX_TYPE, indexes);
	}
	else
	{
        if(useZFail)
        {
            glCullFace(GL_FRONT);
            glStencilOp(GL_KEEP, stencilIncr, GL_KEEP);
        }
        else
        {
            glCullFace(GL_BACK);
            glStencilOp( GL_KEEP, GL_KEEP, stencilIncr );
        }

        if(useCaps)
            R_RenderShadowCaps();
		R_RenderShadowEdges();

        if(useZFail)
        {
            glCullFace(GL_BACK);
            glStencilOp( GL_KEEP, stencilDecr, GL_KEEP );
        }
        else
        {
            glCullFace(GL_FRONT);
            glStencilOp( GL_KEEP, GL_KEEP, stencilDecr );
        }

        if(useCaps)
            R_RenderShadowCaps();
        glDrawElements(GL_TRIANGLES, idx, GL_SHADOW_INDEX_TYPE, indexes);
	}

    if(harm_r_stencilShadowMask->integer)
		RB_ShadowMask();

	RB_ShadowDebug(); //debug

	if (text)
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	if (glcol)
		glEnableClientState( GL_COLOR_ARRAY );
#else
    // mirrors have the culling order reversed
	if (backEnd.viewParms.isMirror)
	{
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		R_RenderShadowEdges();

		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		R_RenderShadowEdges();
	}
	else
	{
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		R_RenderShadowEdges();

		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		R_RenderShadowEdges();
	}
#endif

	// reenable writing to the color buffer
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

/**
 * @brief Darken everything that is is a shadow volume.
 *
 * @details We have to delay this until everything has been shadowed,
 * because otherwise shadows from different body parts would
 * overlap and double darken.
 */
void RB_ShadowFinish(void)
{
	if (r_shadows->integer != 2)
	{
		return;
	}
	if (glConfig.stencilBits < 4)
	{
		return;
	}
#ifdef STENCIL_SHADOW_IMPROVE
    if(harm_r_stencilShadowMask->integer)
        return;
#endif
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, 255);

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CULL_FACE);

	GL_Bind(tr.whiteImage);

	glLoadIdentity();

	glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);

	// OpenGLES implementation
	GLboolean text  = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = glIsEnabled(GL_COLOR_ARRAY);
	if (text)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (glcol)
	{
		glDisableClientState(GL_COLOR_ARRAY);
	}
	GLfloat vtx[] =
	{
		-100, 100,  -10,
		100,  100,  -10,
		100,  -100, -10,
		-100, -100, -10
	};
	glVertexPointer(3, GL_FLOAT, 0, vtx);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	if (text)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (glcol)
	{
		glEnableClientState(GL_COLOR_ARRAY);
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_STENCIL_TEST);
}

/**
 * @brief RB_ProjectionShadowDeform
 */
void RB_ProjectionShadowDeform(void)
{
	float  *xyz = (float *)tess.xyz;
	int    i;
	float  h;
	vec3_t ground;
	vec3_t light;
	float  groundDist;
	float  d;
	vec3_t lightDir;

	ground[0] = backEnd.orientation.axis[0][2];
	ground[1] = backEnd.orientation.axis[1][2];
	ground[2] = backEnd.orientation.axis[2][2];

	groundDist = backEnd.orientation.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy(backEnd.currentEntity->lightDir, lightDir);
	d = DotProduct(lightDir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5f)
	{
		VectorMA(lightDir, (0.5f - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}
	d = 1.0f / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for (i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		h = DotProduct(xyz, ground) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
