/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"
#include "renderer/backend/simplex.h"	// line font definition
#include "renderer/backend/glsl.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/ImmediateRendering.h"

typedef struct debugBase_s {
	idVec4		rgb;
	int			lifeTime;
	bool		depthTest;
	void ClearFree() {}
} debugBase_t;

template<class DebugObject, int MaxN> struct DebugObjectSet {
	DebugObject arr[MaxN];
	int num;
	int time;

	int Num() const { return num; }
	bool GetDepthTest(int idx) const { return arr[idx].depthTest; }
	DebugObject *Draw(int idx) { return &arr[idx]; }
	DebugObject *Append(int lifeTime, idVec4 rgb, bool depthTest);
	void Clear(int time);
	void CopyFrom(const DebugObjectSet<DebugObject, MaxN> &src);
};


#define MAX_DEBUG_LINES			16384
#define MAX_DEBUG_TEXT			512
#define MAX_DEBUG_POLYGONS		8192

typedef struct debugLine_s : debugBase_t {
	idVec3		start;
	idVec3		end;
} debugLine_t;

typedef struct debugText_s : debugBase_t {
	idStr		text;
	idVec3		origin;
	float		scale;
	idMat3		viewAxis;
	int			align;
	void ClearFree() { text.ClearFree(); }
} debugText_t;

typedef struct debugPolygon_s : debugBase_t {
	idWinding	winding;
	void ClearFree() { winding.ClearFree(); }
} debugPolygon_t;

typedef struct debugPrimitives_s {
	DebugObjectSet<debugLine_s, MAX_DEBUG_LINES> lines;
	DebugObjectSet<debugText_s, MAX_DEBUG_TEXT> text;
	DebugObjectSet<debugPolygon_s, MAX_DEBUG_POLYGONS> polygons;
} debugPrimitives_t;

debugPrimitives_t r_debug, rb_debug;

static void RB_DrawText( ImmediateRendering &ir, const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align );


template<class DebugObject, int MaxN> DebugObject *DebugObjectSet<DebugObject, MaxN>::Append(int lifeTime, idVec4 rgb, bool depthTest) {
	if (num < MaxN) {
		DebugObject &obj = arr[num++];
		obj.lifeTime = time + lifeTime;
		obj.rgb = rgb;
		obj.depthTest = depthTest;
		return &obj;
	}
	return nullptr;
}

template<class DebugObject, int MaxN> void DebugObjectSet<DebugObject, MaxN>::Clear(int newTime) {
	time = newTime;

	if ( !time ) {
		// free resources
		for ( int i = 0 ; i < MaxN; i++ ) {
			arr[i].ClearFree();
		}
		num = 0;
		return;
	}

	// copy any objects that still needs to be drawn
	int rem = 0;
	for ( int i = 0 ; i < num; i++ ) {
		if ( arr[i].lifeTime > time ) {
			if ( rem != i ) {
				arr[rem] = arr[i];
			}
			rem++;
		}
	}
	num = rem;
}

template<class DebugObject, int MaxN> void DebugObjectSet<DebugObject, MaxN>::CopyFrom(const DebugObjectSet<DebugObject, MaxN> &src) {
	time = src.time;
	num = src.num;
	for ( int i = 0; i < num; ++i )
		arr[i] = src.arr[i];
}

/*
================
RB_CopyDebugPrimitivesToBackend
================
*/
void RB_CopyDebugPrimitivesToBackend( void ) {
	rb_debug.lines.CopyFrom(r_debug.lines);
	rb_debug.polygons.CopyFrom(r_debug.polygons);
	rb_debug.text.CopyFrom(r_debug.text);
}

/*
================
RB_DrawBounds
================
*/
void RB_DrawBounds( ImmediateRendering &ir, const idBounds &bounds ) {
	if ( bounds.IsCleared() ) {
		return;
	}
	ir.glBegin( GL_LINE_LOOP );
	ir.glVertex3f( bounds[0][0], bounds[0][1], bounds[0][2] );
	ir.glVertex3f( bounds[0][0], bounds[1][1], bounds[0][2] );
	ir.glVertex3f( bounds[1][0], bounds[1][1], bounds[0][2] );
	ir.glVertex3f( bounds[1][0], bounds[0][1], bounds[0][2] );
	ir.glEnd();

	ir.glBegin( GL_LINE_LOOP );
	ir.glVertex3f( bounds[0][0], bounds[0][1], bounds[1][2] );
	ir.glVertex3f( bounds[0][0], bounds[1][1], bounds[1][2] );
	ir.glVertex3f( bounds[1][0], bounds[1][1], bounds[1][2] );
	ir.glVertex3f( bounds[1][0], bounds[0][1], bounds[1][2] );
	ir.glEnd();

	ir.glBegin( GL_LINES );
	ir.glVertex3f( bounds[0][0], bounds[0][1], bounds[0][2] );
	ir.glVertex3f( bounds[0][0], bounds[0][1], bounds[1][2] );

	ir.glVertex3f( bounds[0][0], bounds[1][1], bounds[0][2] );
	ir.glVertex3f( bounds[0][0], bounds[1][1], bounds[1][2] );

	ir.glVertex3f( bounds[1][0], bounds[0][1], bounds[0][2] );
	ir.glVertex3f( bounds[1][0], bounds[0][1], bounds[1][2] );

	ir.glVertex3f( bounds[1][0], bounds[1][1], bounds[0][2] );
	ir.glVertex3f( bounds[1][0], bounds[1][1], bounds[1][2] );
	ir.glEnd();
}


/*
================
RB_SimpleSpaceSetup
================
*/
void RB_SimpleSpaceSetup( const viewEntity_t *space ) {
	// change the matrix if needed
	if ( space != backEnd.currentSpace ) {
		Uniforms::Global* globalUniforms = programManager->oldStageShader->GetUniformGroup<Uniforms::Global>();
		globalUniforms->Set( space );
		backEnd.currentSpace = space;
	}
}

/*
================
RB_SimpleSurfaceSetup
================
*/
void RB_SimpleSurfaceSetup( const drawSurf_t *drawSurf ) {
	RB_SimpleSpaceSetup(drawSurf->space);

	backEnd.currentScissor = drawSurf->scissorRect;
	GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
}


/*
================
RB_SimpleScreenSetup
================
*/
void RB_SimpleScreenSetup( void ) {
	GL_CheckErrors();
	backEnd.currentSpace = nullptr;
	Uniforms::Global* globalUniforms = programManager->oldStageShader->GetUniformGroup<Uniforms::Global>();
	globalUniforms->modelMatrix.Set( mat4_identity.ToFloatPtr() );			//not used, actually
	globalUniforms->modelViewMatrix.Set( mat4_identity.ToFloatPtr() );
	//specify coordinates in [0..1] x [0..1] instead of [-1..1] x [-1..1]
	idMat4 proj = mat4_identity;
	proj[0][0] = proj[1][1] = 2.0;
	proj[3][0] = proj[3][1] = -1.0;
	GL_SetProjection( proj.ToFloatPtr() );
	GL_CheckErrors();
}

/*
================
RB_SimpleWorldSetup
================
*/
void RB_SimpleWorldSetup( void ) {
	GL_CheckErrors();
	backEnd.currentSpace = &backEnd.viewDef->worldSpace;
	Uniforms::Global* globalUniforms = programManager->oldStageShader->GetUniformGroup<Uniforms::Global>();
	globalUniforms->Set( backEnd.currentSpace );

	backEnd.currentScissor = backEnd.viewDef->scissor;
	GL_CheckErrors();
	GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	GL_CheckErrors();
}

/*
=================
RB_PolygonClear

This will cover the entire screen with normal rasterization.
Texturing is disabled, but the existing glColor, glDepthMask,
glColorMask, and the enabled state of depth buffering and
stenciling will matter.
=================
*/
void RB_PolygonClear( ImmediateRendering &ir ) {
	//qglPushMatrix();
	//qglPushAttrib( GL_ALL_ATTRIB_BITS  );
	RB_SimpleScreenSetup();
	//qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_SCISSOR_TEST );
	ir.glBegin( GL_POLYGON );
	ir.glVertex3f( -20, -20, 0/*-10*/ );
	ir.glVertex3f( 20, -20, 0/*-10*/ );
	ir.glVertex3f( 20, 20, 0/*-10*/ );
	ir.glVertex3f( -20, 20, 0/*-10*/ );
	ir.glEnd();
	ir.Flush();
	//qglPopAttrib();
	//qglPopMatrix();
}

/*
====================
RB_ShowDestinationAlpha
====================
*/
void RB_ShowDestinationAlpha( void ) {
	GL_State( GLS_SRCBLEND_DST_ALPHA | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	ImmediateRendering ir;
	ir.glColor3f( 1, 1, 1 );
	RB_PolygonClear(ir);
}

/*
===================
RB_ScanStencilBuffer

Debugging tool to see what values are in the stencil buffer
===================
*/
void RB_ScanStencilBuffer( void ) {
	int		counts[256];
	int		i;
	byte	*stencilReadback;

	memset( counts, 0, sizeof( counts ) );

	stencilReadback = (byte *)R_StaticAlloc( glConfig.vidWidth * glConfig.vidHeight );
	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

	for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
		counts[ stencilReadback[i] ]++;
	}
	R_StaticFree( stencilReadback );

	// print some stats (not supposed to do from back end in SMP...)
	common->Printf( "stencil values:\n" );
	for ( i = 0 ; i < 255 ; i++ ) {
		if ( counts[i] ) {
			common->Printf( "%i: %i\n", i, counts[i] );
		}
	}
}

/*
===================
RB_CountStencilBuffer

Print an overdraw count based on stencil index values
===================
*/
void RB_CountStencilBuffer( void ) {
	int		count;
	int		i;
	byte	*stencilReadback;

	stencilReadback = (byte *)R_StaticAlloc( glConfig.vidWidth * glConfig.vidHeight );
	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

	count = 0;
	for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
		count += stencilReadback[i];
	}
	R_StaticFree( stencilReadback );

	// print some stats (not supposed to do from back end in SMP...)
	common->Printf( "overdraw: %5.1f\n", (float)count/(glConfig.vidWidth * glConfig.vidHeight)  );
}

/*
===================
RB_ColorByStencilBuffer

Sets the screen colors based on the contents of the
stencil buffer.  Stencil of 0 = black, 1 = red, 2 = green,
3 = blue, ..., 7+ = white
===================
*/
static void RB_ColorByStencilBuffer( void ) {
	static float colors[8][3] = {
		{0,0,0},
		{1,0,0},
		{0,1,0},
		{0,0,1},
		{0,1,1},
		{1,0,1},
		{1,1,0},
		{1,1,1},
	};

	// clear color buffer to white (>6 passes)
	qglClearColor( 1, 1, 1, 1 );
	qglDisable( GL_SCISSOR_TEST );
	qglClear( GL_COLOR_BUFFER_BIT );

	// now draw color for each stencil value
	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	ImmediateRendering ir;
	for ( int i = 0 ; i < 6 ; i++ ) {
		ir.glColor3fv( colors[i] );
		qglStencilFunc( GL_EQUAL, i, 255 );
		RB_PolygonClear(ir);
	}
	qglStencilFunc( GL_ALWAYS, 0, 255 );
}

//======================================================================

/*
==================
RB_ShowOverdraw
==================
*/
void RB_ShowOverdraw( void ) {
	const idMaterial	*material;
	int					i;
	drawSurf_t			**drawSurfs;
	const drawSurf_t	*surf;
	int					numDrawSurfs;
	viewLight_t			*vLight;

	if ( r_showOverDraw.GetInteger() == 0 ) {
		return;
	}
	material = declManager->FindMaterial( "textures/common/overdrawtest", false );

	if ( material == NULL ) {
		return;
	}
	drawSurfs = backEnd.viewDef->drawSurfs;
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	int interactions = 0;

	for ( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {
		for ( surf = vLight->localInteractions; surf; surf = surf->nextOnLight ) {
			interactions++;
		}
		for ( surf = vLight->globalInteractions; surf; surf = surf->nextOnLight ) {
			interactions++;
		}
	}
	drawSurf_t **newDrawSurfs = (drawSurf_t **)R_FrameAlloc( numDrawSurfs + interactions * sizeof( newDrawSurfs[0] ) );

	for ( i = 0; i < numDrawSurfs; i++ ) {
		surf = drawSurfs[i];
		if ( surf->material ) {
			const_cast<drawSurf_t *>(surf)->material = material;
		}
		newDrawSurfs[i] = const_cast<drawSurf_t *>(surf);
	}

	for ( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {
		for ( surf = vLight->localInteractions; surf; surf = surf->nextOnLight ) {
			const_cast<drawSurf_t *>(surf)->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t *>(surf);
		}
		for ( surf = vLight->globalInteractions; surf; surf = surf->nextOnLight ) {
			const_cast<drawSurf_t *>(surf)->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t *>(surf);
		}
		vLight->localInteractions = NULL;
		vLight->globalInteractions = NULL;
	}

	switch( r_showOverDraw.GetInteger() ) {
		case 1: // geometry overdraw
			const_cast<viewDef_t *>(backEnd.viewDef)->drawSurfs = newDrawSurfs;
			const_cast<viewDef_t *>(backEnd.viewDef)->numDrawSurfs = numDrawSurfs;
			break;
		case 2: // light interaction overdraw
			const_cast<viewDef_t *>(backEnd.viewDef)->drawSurfs = &newDrawSurfs[numDrawSurfs];
			const_cast<viewDef_t *>(backEnd.viewDef)->numDrawSurfs = interactions;
			break;
		case 3: // geometry + light interaction overdraw
			const_cast<viewDef_t *>(backEnd.viewDef)->drawSurfs = newDrawSurfs;
			const_cast<viewDef_t *>(backEnd.viewDef)->numDrawSurfs += interactions;
			break;
	}
}

/*
===================
RB_ShowIntensity

Debugging tool to see how much dynamic range a scene is using.
The greatest of the rgb values at each pixel will be used, with
the resulting color shading from red at 0 to green at 128 to blue at 255
===================
*/
void RB_ShowIntensity( void ) {
	byte	*colorReadback;
	int		i, j, c;

	if ( !r_showIntensity.GetBool() ) {
		return;
	}
	colorReadback = (byte *)R_StaticAlloc( glConfig.vidWidth * glConfig.vidHeight * 4 );
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, colorReadback );

	c = glConfig.vidWidth * glConfig.vidHeight * 4;
	for ( i = 0; i < c ; i+=4 ) {
		j = colorReadback[i];
		if ( colorReadback[i+1] > j ) {
			j = colorReadback[i+1];
		}
		if ( colorReadback[i+2] > j ) {
			j = colorReadback[i+2];
		}
		if ( j < 128 ) {
			colorReadback[i+0] = 2*(128-j);
			colorReadback[i+1] = 2*j;
			colorReadback[i+2] = 0;
		} else {
			colorReadback[i+0] = 0;
			colorReadback[i+1] = 2*(255-j);
			colorReadback[i+2] = 2*(j-128);
		}
	}

	// draw it back to the screen
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	qglPushMatrix();
	qglLoadIdentity(); 
    qglOrtho( 0, 1, 0, 1, -1, 1 );
	qglRasterPos2f( 0, 0 );
	qglPopMatrix();
	GL_FloatColor( 1, 1, 1 );
	qglMatrixMode( GL_MODELVIEW );

	qglDrawPixels( glConfig.vidWidth, glConfig.vidHeight, GL_RGBA , GL_UNSIGNED_BYTE, colorReadback );

	R_StaticFree( colorReadback );
}


/*
===================
RB_ShowDepthBuffer

Draw the depth buffer as colors
===================
*/
void RB_ShowDepthBuffer( void ) {
	void	*depthReadback;

	if ( !r_showDepth.GetBool() ) {
		return;
	}
	qglPushMatrix();
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity(); 
    qglOrtho( 0, 1, 0, 1, -1, 1 );
	qglRasterPos2f( 0, 0 );
	qglPopMatrix();
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix();

	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_FloatColor( 1, 1, 1 );

	depthReadback = R_StaticAlloc( glConfig.vidWidth * glConfig.vidHeight*4 );
	memset( depthReadback, 0, glConfig.vidWidth * glConfig.vidHeight*4 );

	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT , GL_FLOAT, depthReadback );
	qglDrawPixels( glConfig.vidWidth, glConfig.vidHeight, GL_RGBA , GL_UNSIGNED_BYTE, depthReadback );

	R_StaticFree( depthReadback );
}

/*
=================
RB_ShowLightCount

This is a debugging tool that will draw each surface with a color
based on how many lights are effecting it
=================
*/
void RB_ShowLightCount( void ) {
	int		i;
	const drawSurf_t	*surf;
	const viewLight_t	*vLight;

	if ( !r_showLightCount.GetBool() ) {
		return;
	}
	GL_State( GLS_DEPTHFUNC_EQUAL );

	RB_SimpleWorldSetup();
	qglClearStencil( 0 );
	qglClear( GL_STENCIL_BUFFER_BIT );

	qglEnable( GL_STENCIL_TEST );

	// optionally count everything through walls
	if ( r_showLightCount.GetInteger() >= 2 ) {
		qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
	} else {
		qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
	}
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		for ( i = 0 ; i < 2 ; i++ ) {
			for ( surf = i ? vLight->localInteractions: vLight->globalInteractions; surf; surf = (drawSurf_t *)surf->nextOnLight ) {
				RB_SimpleSurfaceSetup( surf );
				if (!surf->ambientCache.IsValid()) {
					continue;
				}
				vertexCache.VertexPosition( surf->ambientCache );
				RB_DrawElementsWithCounters( surf );
			}
		}
	}

	// display the results
	RB_ColorByStencilBuffer();

	if ( r_showLightCount.GetInteger() > 2 ) {
		RB_CountStencilBuffer();
	}
}


/*
=================
RB_ShowSilhouette

Blacks out all edges, then adds color for each edge that a shadow
plane extends from, allowing you to see doubled edges
=================
*/
void RB_ShowSilhouette( void ) {
	int		i;
	const drawSurf_t	*surf;
	const viewLight_t	*vLight;

	if ( !r_showSilhouette.GetBool() ) {
		return;
	}

	//
	// clear all triangle edges to black
	//
	qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_STENCIL_TEST );

	GL_FloatColor( 0, 0, 0 );

	GL_State( GLS_POLYMODE_LINE );

	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_DEPTH_TEST );

	RB_RenderDrawSurfListWithFunction( backEnd.viewDef->drawSurfs, backEnd.viewDef->numDrawSurfs, RB_T_RenderTriangleSurface );

	//
	// now blend in edges that cast silhouettes
	//
	RB_SimpleWorldSetup();
	GL_FloatColor( 0.5, 0, 0 );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		for ( i = 0 ; i < 2 ; i++ ) {
			for ( surf = i ? vLight->localShadows : vLight->globalShadows
				; surf ; surf = (drawSurf_t *)surf->nextOnLight ) {
				RB_SimpleSurfaceSetup( surf );

				const srfTriangles_t	*tri = surf->frontendGeo;

				vertexCache.VertexPosition( tri->shadowCache, ATTRIB_SHADOW );
				qglBegin( GL_LINES );

				for ( int j = 0 ; j < tri->numIndexes ; j+=3 ) {
					int		i1 = tri->indexes[j+0];
					int		i2 = tri->indexes[j+1];
					int		i3 = tri->indexes[j+2];

					if ( (i1 & 1) + (i2 & 1) + (i3 & 1) == 1 ) {
						if ( (i1 & 1) + (i2 & 1) == 0 ) {
							qglArrayElement( i1 );
							qglArrayElement( i2 );
						} else if ( (i1 & 1 ) + (i3 & 1) == 0 ) {
							qglArrayElement( i1 );
							qglArrayElement( i3 );
						}
					}
				}
				qglEnd();

			}
		}
	}
	qglEnable( GL_DEPTH_TEST );

	GL_State( GLS_DEFAULT );
	GL_FloatColor( 1,1,1 );
	GL_Cull( CT_FRONT_SIDED );
}



/*
=================
RB_ShowShadowCount

This is a debugging tool that will draw only the shadow volumes
and count up the total fill usage
=================
*/
static void RB_ShowShadowCount( void ) {
	int		i;
	const drawSurf_t	*surf;
	const viewLight_t	*vLight;

	if ( !r_showShadowCount.GetBool() ) {
		return;
	}
	GL_State( GLS_DEFAULT );

	qglClearStencil( 0 );
	qglClear( GL_STENCIL_BUFFER_BIT );
	qglEnable( GL_STENCIL_TEST );

	qglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
	qglStencilFunc( GL_ALWAYS, 1, 255 );

	// draw both sides
	GL_Cull( CT_TWO_SIDED );

	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		for ( i = 0 ; i < 2 ; i++ ) {
			for ( surf = i ? vLight->localShadows : vLight->globalShadows 
				; surf ; surf = (drawSurf_t *)surf->nextOnLight ) {
				RB_SimpleSurfaceSetup( surf );
				const srfTriangles_t	*tri = surf->frontendGeo;
				if ( !tri->shadowCache.IsValid() ) {
					continue;
				}

				if ( r_showShadowCount.GetInteger() == 3 ) {
					// only show turboshadows
					if ( tri->numShadowIndexesNoCaps != tri->numIndexes ) {
						continue;
					}
				}
				if ( r_showShadowCount.GetInteger() == 4 ) {
					// only show static shadows
					if ( tri->numShadowIndexesNoCaps == tri->numIndexes ) {
						continue;
					}
				}
				vertexCache.VertexPosition( surf->shadowCache, ATTRIB_SHADOW );
				RB_DrawElementsWithCounters( surf );
			}
		}
	}

	// display the results
	RB_ColorByStencilBuffer();

	if ( r_showShadowCount.GetInteger() == 2 ) {
		common->Printf( "all shadows " );
	} else if ( r_showShadowCount.GetInteger() == 3 ) {
		common->Printf( "turboShadows " );
	} else if ( r_showShadowCount.GetInteger() == 4 ) {
		common->Printf( "static shadows " );
	}

	if ( r_showShadowCount.GetInteger() >= 2 ) {
		RB_CountStencilBuffer();
	}
	GL_Cull( CT_FRONT_SIDED );
}


idCVar r_modelBvhShow(
	"r_modelBvhShow", "-1", CVAR_INTEGER | CVAR_RENDERER,
	"Highlight nodes of model BVH with different colors. "
	"Show leaf nodes and nodes of depth = value of cvar.\n"
	"Note: does not work with \"com_cmp 1\"."
);

static void FindBvhNodes(bvhNode_t *nodes, int idx, int remainingDepth, idList<int> &found) {
	if (!nodes[idx].HasSons() || remainingDepth == 0)
		found.AddGrow(idx);
	else {
		for (int s = 0; s < 2; s++)
			FindBvhNodes(nodes, nodes[idx].GetSon(idx, s), remainingDepth - 1, found);
	}
}

/*
=====================
RB_ShowModelBvh
=====================
*/
static void RB_ShowModelBvh() {
	if (r_modelBvhShow.GetInteger() < 0)
		return;

	if (tr.primaryView->IsLightGem())
		return;

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	qglDisable( GL_STENCIL_TEST );

	static float colors[12][3] = {
		{1,0,0},
		{0,1,0},
		{0,0,1},
		{0,1,1},
		{1,0,1},
		{1,1,0},
		{1,0.5,0},
		{0,1,0.5},
		{0.5,0,1},
		{0.5,1,0},
		{0,0.5,1},
		{1,0,0.5},
	};

	ImmediateRendering ir;

	idList<int> nodeIds;
	for (int ds = 0; ds < tr.primaryView->numDrawSurfs; ds++) {
		drawSurf_t *drawSurf = tr.primaryView->drawSurfs[ds];
		const srfTriangles_t *tri = tr.primaryView->drawSurfs[ds]->frontendGeo;
		if (!tri->bvhNodes)
			continue;
		if (!drawSurf->material)
			continue;
		RB_SimpleSurfaceSetup(drawSurf);

		nodeIds.Clear();
		FindBvhNodes(tri->bvhNodes, 0, r_modelBvhShow.GetInteger(), nodeIds);

		for (int j = 0; j < nodeIds.Num(); j++) {
			int idx = nodeIds[j];
			int numElems = tri->bvhNodes[idx].numElements;
			while (tri->bvhNodes[idx].HasSons())
				idx = tri->bvhNodes[idx].GetSon(idx, 0);
			int startElem = tri->bvhNodes[idx].firstElement;
			float *c = colors[j % 12];
			ir.glColor4f(c[0], c[1], c[2], 0.3f);
			ir.glBegin(GL_TRIANGLES);
			for (int i = 3 * startElem; i < 3 * (startElem + numElems); i++)
				ir.glVertex3fv(&tri->verts[tri->indexes[i]].xyz.x);
			ir.glEnd();
		}
		ir.Flush();
	}

	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowTris

Debugging tool
=====================
*/
static void RB_ShowTris( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	if ( !r_showTris.GetInteger() ) {
		return;
	}

	qglDisable( GL_STENCIL_TEST );

	GL_FloatColor( 1, 1, 1 );

	GL_State( GLS_POLYMODE_LINE );

	if ( r_showTris.GetInteger() & 1 ) {
		// only draw visible ones
		qglPolygonOffset( -1, -2 );
		qglEnable( GL_POLYGON_OFFSET_LINE );
	} else 
		qglDisable( GL_DEPTH_TEST );
	if ( r_showTris.GetInteger() & 2 ) 
		// draw all front facing
		GL_Cull( CT_FRONT_SIDED );
	else
		GL_Cull( CT_TWO_SIDED );
	RB_RenderDrawSurfListWithFunction( drawSurfs, numDrawSurfs, RB_T_RenderTriangleSurface );

	qglEnable( GL_DEPTH_TEST );
	qglDisable( GL_POLYGON_OFFSET_LINE );

#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0, 1 );
#else
	qglDepthRange( 0, 1 );
#endif
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
}


/*
=====================
RB_ShowSurfaceInfo

Debugging tool
=====================
*/
static void RB_ShowSurfaceInfo( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	
	// Skip if the current render is the lightgem render
	if ( !r_showSurfaceInfo.GetBool() || tr.primaryView->IsLightGem() ) {
		return;
	}
	modelTrace_t mt;

	// start far enough away that we don't hit the player model
	const idVec3 start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	const idVec3 end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;

	if ( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false, true ) ) {
		return;
	}

	//qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_STENCIL_TEST );

	GL_State( GLS_POLYMODE_LINE );

	qglEnable( GL_POLYGON_OFFSET_LINE );

	float	matrix[16];

	// transform the object verts into global space
	R_AxisToModelMatrix( mt.entity->axis, mt.entity->origin, matrix );

	idStr modelText;
	sprintf( modelText, "%s : %d", mt.entity->hModel->Name(), mt.surfIdx );

	tr.primaryWorld->DebugText( modelText, mt.point + tr.primaryView->renderView.viewaxis[2] * 16,
		0.35f, colorRed, tr.primaryView->renderView.viewaxis );
	tr.primaryWorld->DebugText( mt.material->GetName(), mt.point, 
		0.35f, colorBlue, tr.primaryView->renderView.viewaxis );
	if ( r_showSurfaceInfo.GetInteger() == 2 ) {
#if 1
		idStr index;
		for ( auto def : tr.primaryWorld->entityDefs ) {
			if ( &def->parms == mt.entity )
				index = idStr( def->index );
		}
#else
		auto ge = gameLocal.entities[mt.entity->entityNum];
		auto mh = ge->GetModelDefHandle();
		auto rel = backEnd.viewDef->renderWorld->entityDefs[mh];
		idStr index( rel->index );
#endif
		tr.primaryWorld->DebugText( index.c_str(), mt.point + tr.primaryView->renderView.viewaxis[2] * 32,
			0.35f, colorBlue, tr.primaryView->renderView.viewaxis );
	}

	qglEnable( GL_DEPTH_TEST );
	qglDisable( GL_POLYGON_OFFSET_LINE );

#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0, 1 );
#else
	qglDepthRange( 0, 1 );
#endif
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
}


/*
=====================
RB_ShowViewEntitys

Debugging tool
=====================
*/
static void RB_ShowViewEntitys( viewEntity_t *vModels ) {
	if ( !r_showViewEntitys.GetBool() ) {
		return;
	}
	if ( r_showViewEntitys.GetInteger() >= 2 ) {
		common->Printf( "view entities: " );
		for ( ; vModels ; vModels = vModels->next ) {
			if ( r_showViewEntitys.GetInteger() > 2 ) {
				renderEntity_t &re = vModels->entityDef->parms;
				if ( !re.hModel ) {
					common->Printf( "%3i NULL\n", vModels->entityDef->index );
				}
				common->Printf( "%3i %s\n", vModels->entityDef->index, re.hModel->Name() );
			} else {
				common->Printf( "%i ", vModels->entityDef->index );
			}
		}
		common->Printf( "\n" );
		if ( r_showViewEntitys.GetInteger() > 2 )
			r_showViewEntitys.SetInteger( 0 );
		return;
	}
	//qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_STENCIL_TEST );

	GL_FloatColor( 1, 1, 1 );

	GL_State( GLS_POLYMODE_LINE );

	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_SCISSOR_TEST );

	ImmediateRendering ir;
	for ( ; vModels ; vModels = vModels->next ) {
		idBounds	b;

		ir.DrawSetup( RB_SimpleSpaceSetup, vModels );

		if ( !vModels->entityDef || r_singleEntity.GetInteger() >= 0 && vModels->entityDef->index != r_singleEntity.GetInteger()  ) {
			continue;
		}

		// draw the reference bounds in yellow
		ir.glColor3f( 1, 1, 0 );
		RB_DrawBounds( ir, vModels->entityDef->referenceBounds );

		// draw the model bounds in white
		ir.glColor3f( 1, 1, 1 );

		idRenderModel *model = R_EntityDefDynamicModel( vModels->entityDef );

		if ( !model ) {
			continue;	// particles won't instantiate without a current view
		}
		b = model->Bounds( &vModels->entityDef->parms );
		RB_DrawBounds( ir, b );
	}
	ir.Flush();
	qglEnable( GL_DEPTH_TEST );
	qglDisable( GL_POLYGON_OFFSET_LINE );

#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0, 1 );
#else
	qglDepthRange( 0, 1 );
#endif
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
}

/*
=====================
RB_ShowEntityDraws

Performance tool
=====================
*/
static void RB_ShowEntityDraws() {
	if ( !r_showEntityDraws || backEnd.viewDef->isSubview ) {
		return;
	}

	const bool group = r_showEntityDraws & 2;
	const bool verts = r_showEntityDraws & 4;
	idStrList list;
	struct entityCalls {
		int index, calls;
		idStr model;
	};
	struct modelCalls {
		int entities, calls;
	};
	idList<entityCalls> stats;
	auto vModels = backEnd.viewDef->viewEntitys;
	for ( ; vModels; vModels = vModels->next ) {
		if ( !vModels->drawCalls )
			continue;
		renderEntity_t& re = vModels->entityDef->parms;
		idStr name = "NULL";
		if ( re.hModel )
			name = re.hModel->Name();
		//name.CapLength( 26 );
		if ( group ) {
			entityCalls calls{ vModels->entityDef->index, vModels->drawCalls, name, };
			stats.Append( calls );
		} else {
			if ( verts ) // vModels->drawCalls double serves as vertex count (huge number) in this mode
				list.Append( idStr::Fmt( "%6i %4i %s\n", vModels->drawCalls, vModels->entityDef->index, name.c_str() ) );
			else
				list.Append( idStr::Fmt( "%3i %4i %s\n", vModels->drawCalls, vModels->entityDef->index, name.c_str() ) );
		}
	}
	if ( group ) {
		std::map<idStr, modelCalls> grouped;
		for ( auto& stat : stats ) {
			auto& grp = grouped[stat.model];
			grp.calls += stat.calls;
			grp.entities++;
		}
		for ( auto& iterator : grouped )
			list.Append( idStr::Fmt( "%5i %2i %s\n", iterator.second.calls, iterator.second.entities, iterator.first.c_str() ) );
	}
	list.Sort();
	int runningTotal = 0;
	for ( int i = 0; i < list.Num(); i++ ) {
		std::stringstream stream( list[i].c_str() );
		int n;
		stream >> n;
		runningTotal += n;
		list[i] = idStr::Fmt( "%6i %s", runningTotal, list[i].c_str() );
	}
	for ( idStr s : list )
		common->Printf( "%s", s.c_str() );

	r_showEntityDraws = false;
}

/*
=====================
RB_ShowTexturePolarity

Shade triangle red if they have a positive texture area
green if they have a negative texture area, or blue if degenerate area
=====================
*/
static void RB_ShowTexturePolarity( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if ( !r_showTexturePolarity.GetBool() ) {
		return;
	}
	qglDisable( GL_STENCIL_TEST );

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	ImmediateRendering ir;
	ir.glColor3f( 1, 1, 1 );
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];
		tri = drawSurf->frontendGeo;

		if ( !tri->verts ) {
			continue;
		}
		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		ir.glBegin( GL_TRIANGLES );
		for ( j = 0 ; j < tri->numIndexes ; j+=3 ) {
			idDrawVert	*a, *b, *c;
			float		d0[5], d1[5];
			float		area;

			a = tri->verts + tri->indexes[j];
			b = tri->verts + tri->indexes[j+1];
			c = tri->verts + tri->indexes[j+2];

			d0[3] = b->st[0] - a->st[0];
			d0[4] = b->st[1] - a->st[1];

			d1[3] = c->st[0] - a->st[0];
			d1[4] = c->st[1] - a->st[1];

			area = d0[3] * d1[4] - d0[4] * d1[3];

			if ( idMath::Fabs( area ) < 0.0001 ) {
				ir.glColor4f( 0, 0, 1, 0.5 );
			} else  if ( area < 0 ) {
				ir.glColor4f( 1, 0, 0, 0.5 );
			} else {
				ir.glColor4f( 0, 1, 0, 0.5 );
			}
			ir.glVertex3fv( a->xyz.ToFloatPtr() );
			ir.glVertex3fv( b->xyz.ToFloatPtr() );
			ir.glVertex3fv( c->xyz.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
	GL_State( GLS_DEFAULT );
}


/*
=====================
RB_ShowUnsmoothedTangents

Shade materials that are using unsmoothed tangents
=====================
*/
static void RB_ShowUnsmoothedTangents( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if ( !r_showUnsmoothedTangents.GetBool() ) {
		return;
	}
	qglDisable( GL_STENCIL_TEST );

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	ImmediateRendering ir;
	ir.glColor4f( 0, 1, 0, 0.5 );
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		if ( !drawSurf->material->UseUnsmoothedTangents() ) {
			continue;
		}
		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		tri = drawSurf->frontendGeo;

		ir.glBegin( GL_TRIANGLES );

		for ( j = 0 ; j < tri->numIndexes ; j+=3 ) {
			idDrawVert	*a, *b, *c;

			a = tri->verts + tri->indexes[j];
			b = tri->verts + tri->indexes[j+1];
			c = tri->verts + tri->indexes[j+2];

			ir.glVertex3fv( a->xyz.ToFloatPtr() );
			ir.glVertex3fv( b->xyz.ToFloatPtr() );
			ir.glVertex3fv( c->xyz.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowTangentSpace

Shade a triangle by the RGB colors of its tangent space
1 = tangents[0]
2 = tangents[1]
3 = normal
=====================
*/
static void RB_ShowTangentSpace( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if ( !r_showTangentSpace.GetInteger() ) {
		return;
	}
	qglDisable( GL_STENCIL_TEST );

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	ImmediateRendering ir;
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		tri = drawSurf->frontendGeo;
		if ( !tri->verts ) {
			continue;
		}
		ir.glBegin( GL_TRIANGLES );

		for ( j = 0 ; j < tri->numIndexes ; j++ ) {
			const idDrawVert *v;

			v = &tri->verts[tri->indexes[j]];

			if ( r_showTangentSpace.GetInteger() == 1 ) {
				ir.glColor4f( 0.5 + 0.5 * v->tangents[0][0],  0.5 + 0.5 * v->tangents[0][1],  
					0.5 + 0.5 * v->tangents[0][2], 0.5 );
			} else if ( r_showTangentSpace.GetInteger() == 2 ) {
				ir.glColor4f( 0.5 + 0.5 * v->tangents[1][0],  0.5 + 0.5 * v->tangents[1][1],  
					0.5 + 0.5 * v->tangents[1][2], 0.5 );
			} else {
				ir.glColor4f( 0.5 + 0.5 * v->normal[0],  0.5 + 0.5 * v->normal[1],  
					0.5 + 0.5 * v->normal[2], 0.5 );
			}
			ir.glVertex3fv( v->xyz.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();

	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowVertexColor

Draw each triangle with the solid vertex colors
=====================
*/
static void RB_ShowVertexColor( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if ( !r_showVertexColor.GetBool() ) {
		return;
	}
	qglDisable( GL_STENCIL_TEST );

	GL_State( GLS_DEPTHFUNC_LESS );

	ImmediateRendering ir;
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		tri = drawSurf->frontendGeo;
		if ( !tri->verts ) {
			continue;
		}
		ir.glBegin( GL_TRIANGLES );

		for ( j = 0 ; j < tri->numIndexes ; j++ ) {
			const idDrawVert *v;
			v = &tri->verts[tri->indexes[j]];
			ir.glColor4ubv( v->color );
			ir.glVertex3fv( v->xyz.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
	GL_State( GLS_DEFAULT );
}


/*
=====================
RB_ShowNormals

Debugging tool
=====================
*/
static void RB_ShowNormals( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int			i, j;
	drawSurf_t	*drawSurf;
	idVec3		end;
	const srfTriangles_t	*tri;
	float		size;
	bool		showNumbers;
	idVec3		pos;

	if ( r_showNormals.GetFloat() == 0.0f ) {
		return;
	}
	GL_State( GLS_POLYMODE_LINE );

	qglDisable( GL_STENCIL_TEST );

	if ( !r_debugLineDepthTest.GetBool() ) {
		qglDisable( GL_DEPTH_TEST );
	} else {
		qglEnable( GL_DEPTH_TEST );
	}
	size = r_showNormals.GetFloat();

	if ( size < 0.0f ) {
		size = -size;
		showNumbers = true;
	} else {
		showNumbers = false;
	}

	ImmediateRendering ir;
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		tri = drawSurf->frontendGeo;
		if ( !tri->verts ) {
			continue;
		}
		ir.glBegin( GL_LINES );

		for ( j = 0 ; j < tri->numVerts ; j++ ) {
			ir.glColor3f( 0, 0, 1 );
			ir.glVertex3fv( tri->verts[j].xyz.ToFloatPtr() );
			VectorMA( tri->verts[j].xyz, size, tri->verts[j].normal, end );
			ir.glVertex3fv( end.ToFloatPtr() );

			ir.glColor3f( 1, 0, 0 );
			ir.glVertex3fv( tri->verts[j].xyz.ToFloatPtr() );
			VectorMA( tri->verts[j].xyz, size, tri->verts[j].tangents[0], end );
			ir.glVertex3fv( end.ToFloatPtr() );

			ir.glColor3f( 0, 1, 0 );
			ir.glVertex3fv( tri->verts[j].xyz.ToFloatPtr() );
			VectorMA( tri->verts[j].xyz, size, tri->verts[j].tangents[1], end );
			ir.glVertex3fv( end.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();

	if ( showNumbers ) {
		RB_SimpleWorldSetup();
		for ( i = 0 ; i < numDrawSurfs ; i++ ) {
			drawSurf = drawSurfs[i];
			tri = drawSurf->frontendGeo;
			if ( !tri->verts ) {
				continue;
			}
			
			for ( j = 0 ; j < tri->numVerts ; j++ ) {
				R_LocalPointToGlobal( drawSurf->space->modelMatrix, tri->verts[j].xyz + tri->verts[j].tangents[0] + tri->verts[j].normal * 0.2f, pos );
				RB_DrawText( ir, va( "%d", j ), pos, 0.01f, colorWhite, backEnd.viewDef->renderView.viewaxis, 1 );
			}

			for ( j = 0 ; j < tri->numIndexes; j += 3 ) {
				R_LocalPointToGlobal( drawSurf->space->modelMatrix, ( tri->verts[ tri->indexes[ j + 0 ] ].xyz + tri->verts[ tri->indexes[ j + 1 ] ].xyz + tri->verts[ tri->indexes[ j + 2 ] ].xyz ) * ( 1.0f / 3.0f ) + tri->verts[ tri->indexes[ j + 0 ] ].normal * 0.2f, pos );
				RB_DrawText( ir, va( "%d", j / 3 ), pos, 0.01f, colorCyan, backEnd.viewDef->renderView.viewaxis, 1 );
			}
		}
	}
	ir.Flush();
	qglEnable( GL_STENCIL_TEST );
}

/*
=====================
RB_ShowTextureVectors

Draw texture vectors in the center of each triangle
=====================
*/
static void RB_ShowTextureVectors( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int			i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if ( r_showTextureVectors.GetFloat() == 0.0f ) {
		return;
	}
	GL_State( GLS_DEPTHFUNC_LESS );

	ImmediateRendering ir;
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		tri = drawSurf->frontendGeo;

		if ( !tri->verts ) {
			continue;
		}

		if ( !tri->facePlanes ) {
			continue;
		}
		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		// draw non-shared edges in yellow
		ir.glBegin( GL_LINES );

		for ( j = 0 ; j < tri->numIndexes ; j+= 3 ) {
			const idDrawVert *a, *b, *c;
			float	area, inva;
			idVec3	temp;
			float		d0[5], d1[5];
			idVec3		mid;
			idVec3		tangents[2];

			a = &tri->verts[tri->indexes[j+0]];
			b = &tri->verts[tri->indexes[j+1]];
			c = &tri->verts[tri->indexes[j+2]];

			// make the midpoint slightly above the triangle
			mid = ( a->xyz + b->xyz + c->xyz ) * ( 1.0f / 3.0f );
			mid += 0.1f * tri->facePlanes[ j / 3 ].Normal();

			// calculate the texture vectors
			VectorSubtract( b->xyz, a->xyz, d0 );
			d0[3] = b->st[0] - a->st[0];
			d0[4] = b->st[1] - a->st[1];
			VectorSubtract( c->xyz, a->xyz, d1 );
			d1[3] = c->st[0] - a->st[0];
			d1[4] = c->st[1] - a->st[1];

			area = d0[3] * d1[4] - d0[4] * d1[3];
			if ( area == 0 ) {
				continue;
			}
			inva = 1.0 / area;

			temp[0] = (d0[0] * d1[4] - d0[4] * d1[0]) * inva;
			temp[1] = (d0[1] * d1[4] - d0[4] * d1[1]) * inva;
			temp[2] = (d0[2] * d1[4] - d0[4] * d1[2]) * inva;
			temp.Normalize();
			tangents[0] = temp;
        
			temp[0] = (d0[3] * d1[0] - d0[0] * d1[3]) * inva;
			temp[1] = (d0[3] * d1[1] - d0[1] * d1[3]) * inva;
			temp[2] = (d0[3] * d1[2] - d0[2] * d1[3]) * inva;
			temp.Normalize();
			tangents[1] = temp;

			// draw the tangents
			tangents[0] = mid + tangents[0] * r_showTextureVectors.GetFloat();
			tangents[1] = mid + tangents[1] * r_showTextureVectors.GetFloat();

			ir.glColor3f( 1, 0, 0 );
			ir.glVertex3fv( mid.ToFloatPtr() );
			ir.glVertex3fv( tangents[0].ToFloatPtr() );

			ir.glColor3f( 0, 1, 0 );
			ir.glVertex3fv( mid.ToFloatPtr() );
			ir.glVertex3fv( tangents[1].ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
}

/*
=====================
RB_ShowDominantTris

Draw lines from each vertex to the dominant triangle center
=====================
*/
static void RB_ShowDominantTris( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int			i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if ( !r_showDominantTri.GetBool() ) {
		return;
	}
	GL_State( GLS_DEPTHFUNC_LESS );

	qglPolygonOffset( -1, -2 );
	qglEnable( GL_POLYGON_OFFSET_LINE );

	ImmediateRendering ir;
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		tri = drawSurf->frontendGeo;

		if ( !tri->verts ) {
			continue;
		}
		if ( !tri->dominantTris ) {
			continue;
		}
		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		ir.glColor3f( 1, 1, 0 );
		ir.glBegin( GL_LINES );

		for ( j = 0 ; j < tri->numVerts ; j++ ) {
			const idDrawVert *a, *b, *c;
			idVec3		mid;

			// find the midpoint of the dominant tri
			a = &tri->verts[j];
			b = &tri->verts[tri->dominantTris[j].v2];
			c = &tri->verts[tri->dominantTris[j].v3];

			mid = ( a->xyz + b->xyz + c->xyz ) * ( 1.0f / 3.0f );

			ir.glVertex3fv( mid.ToFloatPtr() );
			ir.glVertex3fv( a->xyz.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
	qglDisable( GL_POLYGON_OFFSET_LINE );
}

/*
=====================
RB_ShowEdges

Debugging tool
=====================
*/
static void RB_ShowEdges( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int			i, j, k, m, n, o;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;
	const silEdge_t			*edge;
	int			danglePlane;

	if ( !r_showEdges.GetBool() ) {
		return;
	}
	GL_State( GLS_DEFAULT );
	qglDisable( GL_DEPTH_TEST );

	ImmediateRendering ir;
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf = drawSurfs[i];

		tri = drawSurf->frontendGeo;

		idDrawVert *ac = (idDrawVert *)tri->verts;

		if ( !ac ) {
			continue;
		}
		ir.DrawSetup( RB_SimpleSurfaceSetup, drawSurf );

		// draw non-shared edges in yellow
		ir.glColor3f( 1, 1, 0 );
		ir.glBegin( GL_LINES );
		for ( j = 0 ; j < tri->numIndexes ; j+= 3 ) {
			for ( k = 0 ; k < 3 ; k++ ) {
				int		l, i1, i2;
				l = ( k == 2 ) ? 0 : k + 1;
				i1 = tri->indexes[j+k];
				i2 = tri->indexes[j+l];

				// if these are used backwards, the edge is shared
				for ( m = 0 ; m < tri->numIndexes ; m += 3 ) {
					for ( n = 0 ; n < 3 ; n++ ) {
						o = ( n == 2 ) ? 0 : n + 1;
						if ( tri->indexes[m+n] == i2 && tri->indexes[m+o] == i1 ) {
							break;
						}
					}

					if ( n != 3 ) {
						break;
					}
				}

				// if we didn't find a backwards listing, draw it in yellow
				if ( m == tri->numIndexes ) {
					ir.glVertex3fv( ac[ i1 ].xyz.ToFloatPtr() );
					ir.glVertex3fv( ac[ i2 ].xyz.ToFloatPtr() );
				}
			}
		}
		ir.glEnd();

		// draw dangling sil edges in red
		if ( !tri->silEdges ) {
			continue;
		}

		// the plane number after all real planes
		// is the dangling edge
		danglePlane = tri->numIndexes / 3;

		ir.glColor3f( 1, 0, 0 );

		ir.glBegin( GL_LINES );
		for ( j = 0 ; j < tri->numSilEdges ; j++ ) {
			edge = tri->silEdges + j;

			if ( edge->p1 != danglePlane && edge->p2 != danglePlane ) {
				continue;
			}
			ir.glVertex3fv( ac[ edge->v1 ].xyz.ToFloatPtr() );
			ir.glVertex3fv( ac[ edge->v2 ].xyz.ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
	qglEnable( GL_DEPTH_TEST );
}

void RB_ShowLightScissors( void ) {
	if ( r_showLightScissors.GetInteger() < 2 ) {
		return;
	}
	common->Printf( "lights:" );	// FIXME: not in back end!
	for ( auto vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {
		auto light = vLight->lightDef;
		int index = backEnd.viewDef->renderWorld->lightDefs.FindIndex( vLight->lightDef );
		common->Printf( " %i>%ix%i", index, vLight->scissorRect.GetWidth(), vLight->scissorRect.GetHeight() );
	}
	common->Printf( "\n" );
}

/*
==============
RB_ShowLights

Visualize all light volumes used in the current scene
r_showLights bit 1	: print volumes numbers, highlighting ones covering the view
r_showLights bit 2	: draw planes of each volume
r_showLights bit 3	: draw edges of each volume
==============
*/
void RB_ShowLights( void ) {
	int					count;
	viewLight_t			*vLight;

	if ( !r_showLights.GetInteger() ) {
		return;
	}
	programManager->oldStageShader->Activate();
	OldStageUniforms* oldStageUniforms = programManager->oldStageShader->GetUniformGroup<OldStageUniforms>();
	const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	oldStageUniforms->colorMul.Set( one );
	oldStageUniforms->colorAdd.Set( zero );

	// all volumes are expressed in world coordinates
	GL_CheckErrors();
	RB_SimpleWorldSetup();
	GL_CheckErrors();
	qglDisable( GL_STENCIL_TEST );

	GL_Cull( CT_TWO_SIDED );
	GL_CheckErrors();

	idStr output = "volumes:";

	count = 0;

	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		GL_CheckErrors();
		count++;
		srfTriangles_t& tri = *vLight->frustumTris;
		int index = backEnd.viewDef->renderWorld->lightDefs.FindIndex( vLight->lightDef );
		
		// non-hidden lines
		if ( tri.ambientCache.IsValid() ) {
			vertexCache.VertexPosition( tri.ambientCache );
			// depth-tested planes
			if ( r_showLights.GetInteger() & 2 ) {
				auto color = vLight->lightShader->IsAmbientLight() ? idVec4( 0, .5, .5, 0.25 )
					: vLight->lightShader->LightCastsShadows() ? idVec4( 0, .5, .5, 0.25 )
					: idVec4( .5, 0, .5, 0.25 );
				GL_FloatColor( color );
				GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
				RB_DrawTriangles( tri );
				color.w /= 4;
				GL_FloatColor( color );
				GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_ALWAYS );
				RB_DrawTriangles( tri );
			}
			// no-depth wireframe
			if ( r_showLights.GetInteger() & 4 ) {
				GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_ALWAYS );
				int c = index % 7 + 1;
				GL_FloatColor( c & 1, c & 2, c & 4, 0.1f );
				RB_DrawTriangles( tri );
				GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
				GL_FloatColor( c & 1, c & 2, c & 4, 0.3f );
				RB_DrawTriangles( tri );
			}
			GL_CheckErrors();
		}

		if ( r_showLights.GetInteger() & 8 ) {
			// stgatilov: BFG-style frustums (may be a bit larger than normal frustums)
			ALIGNTYPE16 frustumCorners_t corners;
			idRenderMatrix bfgMatrix = vLight->lightDef->inverseBaseLightProject;
			idRenderMatrix::GetFrustumCorners( corners, bfgMatrix, bounds_zeroOneCube );
			ImmediateRendering ir;
			auto RenderLines = [&corners,&ir]() {
				ir.glBegin(GL_LINES);
				for ( int v = 0; v < 8; v++ )
					for ( int d = 0; d < 3; d++ ) {
						int u = v ^ (1 << d);
						if (u < v)
							continue;
						ir.glVertex3f(corners.x[v], corners.y[v], corners.z[v]);
						ir.glVertex3f(corners.x[u], corners.y[u], corners.z[u]);
					}
				ir.glEnd();
				ir.Flush();
			};
			GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_ALWAYS );
			int c = index % 7 + 1;
			ir.glColor4f( c & 1, c & 2, c & 4, 0.1f );
			RenderLines();
			GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
			ir.glColor4f( c & 1, c & 2, c & 4, 0.3f );
			RenderLines();
		}

		output += idStr::Fmt( " %i", index );
		if ( vLight->viewInsideLight ) // view is in this volume
			output +=  "i";
		if ( vLight->lightShader->IsAmbientLight() ) // ambient
			output += "a";
		if ( vLight->lightShader->IsFogLight() ) 
			output += "f";
		else if ( vLight->lightShader->IsBlendLight() ) 
			output += "b";
		else if ( vLight->lightShader->LightCastsShadows() ) // shadows
			output += "s";
		GL_CheckErrors();
	}

	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );

	if ( r_showLights.GetInteger() & 1 )
		common->Printf( "%s = %i total\n", output.c_str(), count );
	GL_CheckErrors();
}

/*
=====================
RB_ShowPortals

Duzenko: split into frontend/backend parts to allow SMP and subviews
=====================
*/
void RB_ShowPortals( void ) {
	auto portalStates = backEnd.viewDef->portalStates;
	if ( !r_showPortals || !portalStates )
		return;
	// stgatilov: only display stuff for the main view (skip subviews and lightgem)
	if ( backEnd.viewDef->renderView.viewID != VID_PLAYER_VIEW )
		return;

	// all portals are expressed in world coordinates
	RB_SimpleWorldSetup();

	qglDisable( GL_DEPTH_TEST );

	GL_State( GLS_DEFAULT );

	if ( r_showPortals > 1 )
		common->Printf( "^8EYE:^7%d^8 ", backEnd.viewDef->areaNum );

	ImmediateRendering ir;
	idRenderWorldLocal *world = backEnd.viewDef->renderWorld;
	for ( auto &area : world->portalAreas ) {
		if ( *portalStates++ == 'C' ) // area closed
			continue;
		idStr consoleMsg = idStr::Fmt( "  ^7%d^8->", area.areaNum );
		idVec4 color;
		for ( auto p : area.areaPortals ) {
			if ( consoleMsg.Right(2) != "->" )
				consoleMsg += ",";
			switch ( *portalStates++ ) {	// Changed to show 3 colours. -- SteveL #4162
			case 'G':
				color.Set( 0, 1, 0, 1 ); 	// green = we see through this portal
				consoleMsg += "^2";
				break;
			case 'Y':
				color.Set( 1, 1, 0, 1 );	// yellow = we see into this visleaf but not through this portal
				consoleMsg += "^3";
				break;
			default:
				color.Set( 1, 0, 0, 1 ); 	// red = can't see
				consoleMsg += "^1";			
				break;
			}
			ir.glColor4fv( color.ToFloatPtr() );
			consoleMsg += idStr::Fmt( "%d^8", p->intoArea );
			ir.glBegin( GL_LINE_LOOP );		// FIXME convert to modern OpenGL
			for ( int j = 0; j < p->w.GetNumPoints(); j++ )
				ir.glVertex3fv( p->w[j].ToFloatPtr() );
			ir.glEnd();
		}
		if ( r_showPortals > 1 )
			common->Printf( consoleMsg + "^0" );
	}
	if ( r_showPortals > 1 )
		common->Printf( "\n" );

	ir.Flush();

	qglEnable( GL_DEPTH_TEST );
}

/*
================
R_ClearDebugText
================
*/
void R_ClearDebugText( int time ) {
	r_debug.text.Clear(time);
}

/*
================
R_AddDebugText
================
*/
void R_AddDebugText( const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align, const int lifetime, const bool depthTest ) {
	if (debugText_t *debugText = r_debug.text.Append(lifetime, color, depthTest)) {
		debugText->text			= text;
		debugText->origin		= origin;
		debugText->scale		= scale;
		debugText->viewAxis		= viewAxis;
		debugText->align		= align;
	}
}

/*
================
R_DrawTextLength

  returns the length of the given text
================
*/
float R_DrawTextLength( const char *text, float scale, int len ) {
	int i, num, index, charIndex;
	float spacing, textLen = 0.0f;

	if ( text && *text ) {
		if ( !len ) {
            len = static_cast<int>(strlen(text));
		}
		for ( i = 0; i < len; i++ ) {
			charIndex = text[i] - 32;
			if ( charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS ) {
				continue;
			}
			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while( index - 2 < num ) {   
				if ( simplex[charIndex][index] < 0) {  
					index++;
					continue; 
				} 
				index += 2;
				if ( simplex[charIndex][index] < 0) {  
					index++;
					continue; 
				} 
			}   
			textLen += spacing * scale;  
		}
	}
	return textLen;
}

/*
================
RB_DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
static void RB_DrawText( ImmediateRendering &ir, const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align ) {
	int i, j, len, num, index, charIndex, line;
	float textLen = 0.0f, spacing;
	idVec3 org, p1, p2;

	if ( text && *text ) {
		ir.glBegin( GL_LINES );
		ir.glColor3fv( color.ToFloatPtr() );

		if ( text[0] == '\n' ) {
			line = 1;
		} else {
			line = 0;
		}
        len = static_cast<int>(strlen(text));

		for ( i = 0; i < len; i++ ) {

			if ( i == 0 || text[i] == '\n' ) {
				org = origin - viewAxis[2] * ( line * 36.0f * scale );
				if ( align != 0 ) {
					for ( j = 1; i+j <= len; j++ ) {
						if ( i+j == len || text[i+j] == '\n' ) {
							textLen = R_DrawTextLength( text+i, scale, j );
							break;
						}
					}
					if ( align == 2 ) {
						// right
						org += viewAxis[1] * textLen;
					} else {
						// center
						org += viewAxis[1] * ( textLen * 0.5f );
					}
				}
				line++;
			}
			charIndex = text[i] - 32;

			if ( charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS ) {
				continue;
			}
			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while( index - 2 < num ) {
				if ( simplex[charIndex][index] < 0) {  
					index++;
					continue; 
				}
				p1 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index+1] * viewAxis[2];
				index += 2;
				if ( simplex[charIndex][index] < 0) {
					index++;
					continue;
				}
				p2 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index+1] * viewAxis[2];

				ir.glVertex3fv( p1.ToFloatPtr() );
				ir.glVertex3fv( p2.ToFloatPtr() );
			}
			org -= viewAxis[1] * ( spacing * scale );
		}
		ir.glEnd();
	}
}

/*
================
RB_ShowDebugText
================
*/
void RB_ShowDebugText( void ) {
	if ( rb_debug.text.Num() == 0 ) {
		return;
	}

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	int width = r_debugLineWidth.GetInteger();
	if ( width < 1 ) {
		width = 1;
	} else if ( width > 10 ) {
		width = 10;
	}

	// draw lines
	GL_State( GLS_POLYMODE_LINE );
	qglLineWidth( width );

	if ( !r_debugLineDepthTest.GetBool() ) {
		qglDisable( GL_DEPTH_TEST );
	}

	ImmediateRendering ir;
	for ( int i = 0 ; i < rb_debug.text.Num(); i++ ) {
		if ( !rb_debug.text.GetDepthTest(i) ) {
			debugText_t *text = rb_debug.text.Draw(i);
			RB_DrawText( ir, text->text, text->origin, text->scale, text->rgb, text->viewAxis, text->align );
		}
	}
	ir.Flush();

	if ( !r_debugLineDepthTest.GetBool() ) {
		qglEnable( GL_DEPTH_TEST );
	}

	for ( int i = 0 ; i < rb_debug.text.Num(); i++ ) {
		if ( rb_debug.text.GetDepthTest(i) ) {
			debugText_t *text = rb_debug.text.Draw(i);
			RB_DrawText( ir, text->text, text->origin, text->scale, text->rgb, text->viewAxis, text->align );
		}
	}
	ir.Flush();
	qglLineWidth( 1 );

	GL_State( GLS_DEFAULT );
}

/*
================
R_ClearDebugLines
================
*/
void R_ClearDebugLines( int time ) {
	r_debug.lines.Clear(time);
}

/*
================
R_AddDebugLine
================
*/
void R_AddDebugLine( const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifeTime, const bool depthTest ) {
	if ( debugLine_t *line = r_debug.lines.Append(lifeTime, color, depthTest) ) {
		line->start		= start;
		line->end		= end;
	}
}

/*
================
RB_ShowDebugLines
================
*/
void RB_ShowDebugLines( void ) {
	if ( rb_debug.lines.Num() == 0 || backEnd.viewDef->renderView.viewID != VID_PLAYER_VIEW ) {
		return;
	}

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	int width = r_debugLineWidth.GetInteger();

	if ( width < 1 ) {
		width = 1;
	} else if ( width > 10 ) {
		width = 10;
	}

	// draw lines
	GL_State( GLS_POLYMODE_LINE );//| GLS_DEPTHMASK ); //| GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglLineWidth( width );

	if ( !r_debugLineDepthTest.GetBool() ) {
		qglDisable( GL_DEPTH_TEST );
	}

	ImmediateRendering ir;
	ir.glBegin( GL_LINES );
	for ( int i = 0 ; i < rb_debug.lines.Num(); i++ ) {
		if ( !rb_debug.lines.GetDepthTest(i) ) {
			debugLine_t *line = rb_debug.lines.Draw(i);
			ir.glColor4fv( line->rgb.ToFloatPtr() );
			ir.glVertex3fv( line->start.ToFloatPtr() );
			ir.glVertex3fv( line->end.ToFloatPtr() );
		}
	}
	ir.glEnd();
	ir.Flush();

	if ( !r_debugLineDepthTest.GetBool() ) {
		qglEnable( GL_DEPTH_TEST );
	}

	ir.glBegin( GL_LINES );
	for ( int i = 0 ; i < rb_debug.lines.Num(); i++ ) {
		if ( rb_debug.lines.GetDepthTest(i) ) {
			debugLine_t *line = rb_debug.lines.Draw(i);
			ir.glColor4fv( line->rgb.ToFloatPtr() );
			ir.glVertex3fv( line->start.ToFloatPtr() );
			ir.glVertex3fv( line->end.ToFloatPtr() );
		}
	}
	ir.glEnd();
	ir.Flush();

	qglLineWidth( 1 );
	GL_State( GLS_DEFAULT );
}

/*
================
R_ClearDebugPolygons
================
*/
void R_ClearDebugPolygons( int time ) {
	r_debug.polygons.Clear(time);
}

/*
================
R_AddDebugPolygon
================
*/
void R_AddDebugPolygon( const idVec4 &color, const idWinding &winding, const int lifeTime, const bool depthTest ) {
	if ( debugPolygon_t *poly = r_debug.polygons.Append(lifeTime, color, depthTest) ) {
		poly->winding	= winding;
	}
}

/*
================
RB_ShowDebugPolygons
================
*/
void RB_ShowDebugPolygons( void ) {
	if ( rb_debug.polygons.Num() == 0 ) {
		return;
	}

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	//qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_STENCIL_TEST );

	qglEnable( GL_DEPTH_TEST );

	if ( r_debugPolygonFilled.GetBool() ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );
		qglPolygonOffset( -1, -2 );
		qglEnable( GL_POLYGON_OFFSET_FILL );
	} else {
		GL_State( GLS_POLYMODE_LINE );
		qglPolygonOffset( -1, -2 );
		qglEnable( GL_POLYGON_OFFSET_LINE );
	}
	
	ImmediateRendering ir;
	for ( int i = 0 ; i < rb_debug.polygons.Num(); i++ ) {
		debugPolygon_t *poly = rb_debug.polygons.Draw(i);
		ir.glColor4fv( poly->rgb.ToFloatPtr() );

		ir.glBegin( GL_POLYGON );
		for ( int j = 0; j < poly->winding.GetNumPoints(); j++) {
			ir.glVertex3fv( poly->winding[j].ToFloatPtr() );
		}
		ir.glEnd();
	}
	ir.Flush();
	GL_State( GLS_DEFAULT );

	if ( r_debugPolygonFilled.GetBool() ) {
		qglDisable( GL_POLYGON_OFFSET_FILL );
	} else {
		qglDisable( GL_POLYGON_OFFSET_LINE );
	}

#ifdef __ANDROID__ //karin: GLES
	qglDepthRangef( 0, 1 );
#else
	qglDepthRange( 0, 1 );
#endif
	GL_State( GLS_DEFAULT );
}

/*
================
RB_TestGamma
================
*/
#define	G_WIDTH		512
#define	G_HEIGHT	512
#define	BAR_HEIGHT	64

void RB_TestGamma( void ) {
	byte	image[G_HEIGHT][G_WIDTH][4];
	int		i, j;
	int		c, comp;
	int		v, dither;
	int		mask, y;

	if ( r_testGamma.GetInteger() <= 0 ) {
		return;
	}

	v = r_testGamma.GetInteger();
	if ( v <= 1 || v >= 196 ) {
		v = 128;
	}

	memset( image, 0, sizeof( image ) );

	for ( mask = 0 ; mask < 8 ; mask++ ) {
		y = mask * BAR_HEIGHT;
		for ( c = 0 ; c < 4 ; c++ ) {
			v = c * 64 + 32;
			// solid color
			for ( i = 0 ; i < BAR_HEIGHT/2 ; i++ ) {
				for ( j = 0 ; j < G_WIDTH/4 ; j++ ) {
					for ( comp = 0 ; comp < 3 ; comp++ ) {
						if ( mask & ( 1 << comp ) ) {
							image[y+i][c*G_WIDTH/4+j][comp] = v;
						}
					}
				}
				// dithered color
				for ( j = 0 ; j < G_WIDTH/4 ; j++ ) {
					if ( ( i ^ j ) & 1 ) {
						dither = c * 64;
					} else {
						dither = c * 64 + 63;
					}
					for ( comp = 0 ; comp < 3 ; comp++ ) {
						if ( mask & ( 1 << comp ) ) {
							image[y+BAR_HEIGHT/2+i][c*G_WIDTH/4+j][comp] = dither;
						}
					}
				}
			}
		}
	}

	// draw geometrically increasing steps in the bottom row
	y = 0 * BAR_HEIGHT;
	float	scale = 1;
	for ( c = 0 ; c < 4 ; c++ ) {
		v = (int)(64 * scale);
		if ( v < 0 ) {
			v = 0;
		} else if ( v > 255 ) {
			v = 255;
		}
		scale = scale * 1.5;
		for ( i = 0 ; i < BAR_HEIGHT ; i++ ) {
			for ( j = 0 ; j < G_WIDTH/4 ; j++ ) {
				image[y+i][c*G_WIDTH/4+j][0] = v;
				image[y+i][c*G_WIDTH/4+j][1] = v;
				image[y+i][c*G_WIDTH/4+j][2] = v;
			}
		}
	}
	qglLoadIdentity();

	qglMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_FloatColor( 1, 1, 1 );
	qglPushMatrix();
	qglLoadIdentity(); 
	qglDisable( GL_TEXTURE_2D );
    qglOrtho( 0, 1, 0, 1, -1, 1 );
	qglRasterPos2f( 0.01f, 0.01f );
	qglDrawPixels( G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image );
	qglPopMatrix();
	qglEnable( GL_TEXTURE_2D );
	qglMatrixMode( GL_MODELVIEW );
}


/*
==================
RB_TestGammaBias
==================
*/
static void RB_TestGammaBias( void ) {
	byte	image[G_HEIGHT][G_WIDTH][4];

	if ( r_testGammaBias.GetInteger() <= 0 ) {
		return;
	}

	int y = 0;
	for ( int bias = -40 ; bias < 40 ; bias+=10, y += BAR_HEIGHT ) {
		float	scale = 1;
		for ( int c = 0 ; c < 4 ; c++ ) {
			int v = (int)(64 * scale + bias);
			scale = scale * 1.5;
			if ( v < 0 ) {
				v = 0;
			} else if ( v > 255 ) {
				v = 255;
			}
			for ( int i = 0 ; i < BAR_HEIGHT ; i++ ) {
				for ( int j = 0 ; j < G_WIDTH/4 ; j++ ) {
					image[y+i][c*G_WIDTH/4+j][0] = v;
					image[y+i][c*G_WIDTH/4+j][1] = v;
					image[y+i][c*G_WIDTH/4+j][2] = v;
				}
			}
		}
	}
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_FloatColor( 1, 1, 1 );
	qglPushMatrix();
	qglLoadIdentity(); 
	qglDisable( GL_TEXTURE_2D );
    qglOrtho( 0, 1, 0, 1, -1, 1 );
	qglRasterPos2f( 0.01f, 0.01f );
	qglDrawPixels( G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image );
	qglPopMatrix();
	qglEnable( GL_TEXTURE_2D );
	qglMatrixMode( GL_MODELVIEW );
}

/*
================
RB_TestImage

Display a single image over most of the screen
================
*/
void RB_TestImage( void ) {
	idImage *image = nullptr;
	float w = 0.25;
	float h = 0.25;

	if ( tr.testVideo && tr.testVideoFrame ) {
		cinData_t cin = tr.testVideo->ImageForTime( (int)(1000 * ( backEnd.viewDef->floatTime - tr.testVideoStartTime ) ) );
		if ( cin.image ) {
			tr.testVideoFrame->UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
			image = tr.testVideoFrame;
		} else {
			tr.testVideoFrame = NULL;
			return;
		}
	} else if ( tr.testImage && !tr.testImageIsCubemap ) {
		int max = idMath::Imax(tr.testImage->uploadWidth, tr.testImage->uploadHeight);

		w = 0.25 * tr.testImage->uploadWidth / max;
		h = 0.25 * tr.testImage->uploadHeight / max;
		w *= (float)glConfig.vidHeight / glConfig.vidWidth;

		image = tr.testImage;
	}

	if (!image) {
		return;
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	image->Bind();

	if ( tr.testImageIsCubemap ) {
		idMat3 axis = tr.primaryRenderView.viewaxis;
		idRenderMatrix viewMatrix;
		idRenderMatrix::CreateViewMatrix( idVec3( 0.0f ), axis * 100.0f, viewMatrix );
		const idRenderMatrix &projectionMatrix = backEnd.viewDef->projectionRenderMatrix;

		// render cubemap as if it was around us
		GL_ViewportRelative( 0.5f - w, 0, 2 * w, 2 * h );

		struct TestImageCubeUniforms : GLSLUniformGroup {
			UNIFORM_GROUP_DEF(TestImageCubeUniforms)
			DEFINE_UNIFORM(mat4, modelViewMatrix);
			DEFINE_UNIFORM(mat4, projectionMatrix);
			DEFINE_UNIFORM(sampler, texCube);
		};
		programManager->testImageCubeShader->Activate();
		auto uniforms = programManager->testImageCubeShader->GetUniformGroup<TestImageCubeUniforms>();
		uniforms->texCube.Set( 0 );
		uniforms->modelViewMatrix.Set( viewMatrix );
		uniforms->projectionMatrix.Set( projectionMatrix );

		ImmediateRendering ir;
		ir.glBegin( GL_QUADS );
		for ( int d = 0; d < 3; d++ )
			for ( int s = -1; s <= 1; s += 2) {
				idVec3 axisN( 0.0f ), axisX( 0.0f ), axisY( 0.0f );
				axisN[d] = s;
				axisX[(d+1)%3] = s;
				axisY[(d+2)%3] = 1;

				idVec3 p;
				p = axisN - axisX - axisY;
				ir.glVertex3fv( &p.x );
				p = axisN + axisX - axisY;
				ir.glVertex3fv( &p.x );
				p = axisN + axisX + axisY;
				ir.glVertex3fv( &p.x );
				p = axisN - axisX + axisY;
				ir.glVertex3fv( &p.x );
			}
		ir.glEnd();
	}
	else {
		// render screen-space 2D image
		RB_SimpleScreenSetup();

		ImmediateRendering ir;
		ir.glColor3f(1, 1, 1);

		ir.glBegin( GL_QUADS );
	
		ir.glTexCoord2f( 0, 1 );
		ir.glVertex2f( 0.5 - w, 0 );

		ir.glTexCoord2f( 0, 0 );
		ir.glVertex2f( 0.5 - w, h*2 );

		ir.glTexCoord2f( 1, 0 );
		ir.glVertex2f( 0.5 + w, h*2 );

		ir.glTexCoord2f( 1, 1 );
		ir.glVertex2f( 0.5 + w, 0 );

		ir.glEnd();
	}

	programManager->oldStageShader->Activate();
}

/*
=================
RB_DrawExpandedTriangles
=================
*/
void RB_DrawExpandedTriangles( ImmediateRendering &ir, const srfTriangles_t *tri, const float radius, const idVec3 &vieworg ) {
	int i, j, k;
	idVec3 dir[6], normal, point;

	for ( i = 0; i < tri->numIndexes; i += 3 ) {

		idVec3 p[3] = { tri->verts[ tri->indexes[ i + 0 ] ].xyz, tri->verts[ tri->indexes[ i + 1 ] ].xyz, tri->verts[ tri->indexes[ i + 2 ] ].xyz };

		dir[0] = p[0] - p[1];
		dir[1] = p[1] - p[2];
		dir[2] = p[2] - p[0];

		normal = dir[0].Cross( dir[1] );

		if ( normal * p[0] < normal * vieworg ) {
			continue;
		}
		dir[0] = normal.Cross( dir[0] );
		dir[1] = normal.Cross( dir[1] );
		dir[2] = normal.Cross( dir[2] );

		dir[0].Normalize();
		dir[1].Normalize();
		dir[2].Normalize();

		ir.glBegin( GL_LINE_LOOP );

		for ( j = 0; j < 3; j++ ) {
			k = ( j + 1 ) % 3;

			dir[4] = ( dir[j] + dir[k] ) * 0.5f;
			dir[4].Normalize();

			dir[3] = ( dir[j] + dir[4] ) * 0.5f;
			dir[3].Normalize();

			dir[5] = ( dir[4] + dir[k] ) * 0.5f;
			dir[5].Normalize();

			point = p[k] + dir[j] * radius;
			ir.glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[3] * radius;
			ir.glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[4] * radius;
			ir.glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[5] * radius;
			ir.glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[k] * radius;
			ir.glVertex3f( point[0], point[1], point[2] );
		}
		ir.glEnd();
	}
}

/*
================
RB_ShowTrace

Debug visualization
================
*/
void RB_ShowTrace( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	int						i;
	const srfTriangles_t	*tri;
	const drawSurf_t		*surf;
	idVec3					start, end;
	idVec3					localStart, localEnd;
	localTrace_t			hit;
	float					radius;

	if ( r_showTrace.GetInteger() == 0 ) {
		return;
	}

	if ( r_showTrace.GetInteger() == 2 ) {
		radius = 5.0f;
	} else {
		radius = 0.0f;
	}

	// determine the points of the trace
	start = backEnd.viewDef->renderView.vieworg;
	end = start + 4000 * backEnd.viewDef->renderView.viewaxis[0];

	// check and draw the surfaces
	globalImages->whiteImage->Bind();

	// find how many are ambient
	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		surf = drawSurfs[i];
		tri = surf->frontendGeo;

		if ( tri == NULL || tri->verts == NULL ) {
			continue;
		}

		// transform the points into local space
		R_GlobalPointToLocal( surf->space->modelMatrix, start, localStart );
		R_GlobalPointToLocal( surf->space->modelMatrix, end, localEnd );

		// check the bounding box
		if ( !tri->bounds.Expand( radius ).LineIntersection( localStart, localEnd ) ) {
			continue;
		}

		RB_SimpleSurfaceSetup( surf );

		// highlight the surface
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

		ImmediateRendering ir;
		ir.glBegin( GL_TRIANGLES );
		ir.glColor4f( 1, 0, 0, 0.25 );
		for ( int i = 0 ; i < tri->numIndexes ; i++ ) {
			ir.glVertex3fv( tri->verts[ tri->indexes[i] ].xyz.ToFloatPtr() );
		}
		ir.glEnd();
		ir.Flush();

		// draw the bounding box
		GL_State( GLS_DEPTHFUNC_ALWAYS );

		ir.glColor4f( 1, 1, 1, 1 );
		RB_DrawBounds( ir, tri->bounds );

		if ( radius != 0.0f ) {
			// draw the expanded triangles
			ir.glColor4f( 0.5f, 0.5f, 1.0f, 1.0f );
			RB_DrawExpandedTriangles( ir, tri, radius, localStart );
		}

		// check the exact surfaces
		hit = R_LocalTrace( localStart, localEnd, radius, tri );
		if ( hit.fraction < 1.0 ) {
			ir.glColor4f( 1, 1, 1, 1 );
			RB_DrawBounds( ir, idBounds( hit.point ).Expand( 1 ) );
		}
		ir.Flush();
	}
}

/*
=================
RB_RenderDebugTools
=================
*/
void RB_RenderDebugTools( drawSurf_t **drawSurfs, int numDrawSurfs ) {
	// don't do anything if this was a 2D rendering
	if ( !backEnd.viewDef->viewEntitys ) {
		return;
	}
	TRACE_GL_SCOPE( "RenderDebugTools" );

	GL_State( GLS_DEFAULT );
	backEnd.currentScissor = backEnd.viewDef->scissor;
	GL_ScissorVidSize( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	programManager->oldStageShader->Activate();
	OldStageUniforms* oldStageUniforms = programManager->oldStageShader->GetUniformGroup<OldStageUniforms>();
	oldStageUniforms->colorMul.Set( 1, 1, 1, 1 );
	oldStageUniforms->colorAdd.Set( 0, 0, 0, 0 );
	GL_SelectTexture(0);
	globalImages->whiteImage->Bind();

	RB_ShowLightCount();
	RB_ShowShadowCount();
	RB_ShowModelBvh();
	RB_ShowTexturePolarity( drawSurfs, numDrawSurfs );
	RB_ShowTangentSpace( drawSurfs, numDrawSurfs );
	RB_ShowVertexColor( drawSurfs, numDrawSurfs );
	RB_ShowTris( drawSurfs, numDrawSurfs );
	RB_ShowUnsmoothedTangents( drawSurfs, numDrawSurfs );
	RB_ShowSurfaceInfo( drawSurfs, numDrawSurfs );
	RB_ShowEdges( drawSurfs, numDrawSurfs );
	RB_ShowNormals( drawSurfs, numDrawSurfs );
	RB_ShowViewEntitys( backEnd.viewDef->viewEntitys );
	RB_ShowEntityDraws();
	RB_ShowLights();
	RB_ShowLightScissors();
	RB_ShowTextureVectors( drawSurfs, numDrawSurfs );
	RB_ShowDominantTris( drawSurfs, numDrawSurfs );
	RB_TestImage();
	RB_ShowPortals();
	RB_ShowDebugLines();
	RB_ShowDebugText();
	RB_ShowDebugPolygons();
	RB_ShowTrace( drawSurfs, numDrawSurfs );

	if (r_glCoreProfile.GetInteger() == 0) {
		//stgatilov: this one uses qglArrayElement to fetch vertices from VBO
		RB_ShowSilhouette();
		//stgatilov: these ones use qglDrawPixels to display FBO contents
		RB_ShowDepthBuffer();
		RB_ShowIntensity();
		RB_TestGamma();
		RB_TestGammaBias();
	}
}

/*
=================
RB_ShutdownDebugTools
=================
*/
void RB_ShutdownDebugTools( void ) {
	r_debug.text.Clear(0);
	r_debug.lines.Clear(0);
	r_debug.polygons.Clear(0);
	rb_debug.text.Clear(0);
	rb_debug.lines.Clear(0);
	rb_debug.polygons.Clear(0);
}

void R_Tools() {
	if ( r_showPortals ) // moved from backend to allow subviews and SMP
		tr.viewDef->renderWorld->ShowPortals();
	static idCVarInt r_maxTri( "r_maxTri", "0", CVAR_RENDERER, "Limit max tri per draw call" );
	if ( r_maxTri ) {
		auto limitTris = []( drawSurf_t* surf ) {
			surf->numIndexes = Min<int>( r_maxTri*3, surf->numIndexes );
		};
		for ( int i = 0; i < tr.viewDef->numDrawSurfs; i++ )
			limitTris( tr.viewDef->drawSurfs[i] );
		for ( auto vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
			for ( drawSurf_t* surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
				limitTris( surf );
			for ( drawSurf_t* surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
				limitTris( surf );
			for ( drawSurf_t* surf = vLight->globalShadows; surf; surf = surf->nextOnLight )
				limitTris( surf );
			for ( drawSurf_t* surf = vLight->localShadows; surf; surf = surf->nextOnLight )
				limitTris( surf );
			for ( drawSurf_t* surf = vLight->translucentInteractions; surf; surf = surf->nextOnLight )
				limitTris( surf );
		}
	}
	if ( r_showEntityDraws )
		for ( auto ent = tr.viewDef->viewEntitys; ent; ent = ent->next ) {
			ent->drawCalls = 0;
		}
}
