/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"
#include "simplex.h"	// line font definition

#define MAX_DEBUG_LINES			16384

typedef struct debugLine_s {
	idVec4		rgb;
	idVec3		start;
	idVec3		end;
	bool		depthTest;
	int			lifeTime;
} debugLine_t;

#ifdef _MULTITHREAD //karin: always using original variants in RB_ShowDebug[Text|Lines|Polygons] without multi-threading
debugLine_t		rb_debugLiness[NUM_FRAME_DATA][ MAX_DEBUG_LINES ];
int				rb_numDebugLiness[NUM_FRAME_DATA] = {0};
int				rb_debugLineTimes[NUM_FRAME_DATA] = {0};
#else
debugLine_t		rb_debugLines[ MAX_DEBUG_LINES ];
int				rb_numDebugLines = 0;
int				rb_debugLineTime = 0;
#endif

#define MAX_DEBUG_TEXT			512

typedef struct debugText_s {
	idStr		text;
	idVec3		origin;
	float		scale;
	idVec4		color;
	idMat3		viewAxis;
	int			align;
	int			lifeTime;
	bool		depthTest;
} debugText_t;

#ifdef _MULTITHREAD //karin: always using original variants in RB_ShowDebug[Text|Lines|Polygons] without multi-threading
debugText_t		rb_debugTexts[NUM_FRAME_DATA][ MAX_DEBUG_TEXT ];
int				rb_numDebugTexts[NUM_FRAME_DATA] = {0};
int				rb_debugTextTimes[NUM_FRAME_DATA] = {0};
#else
debugText_t		rb_debugText[ MAX_DEBUG_TEXT ];
int				rb_numDebugText = 0;
int				rb_debugTextTime = 0;
#endif

#define MAX_DEBUG_POLYGONS		8192

typedef struct debugPolygon_s {
	idVec4		rgb;
	idWinding	winding;
	bool		depthTest;
	int			lifeTime;
} debugPolygon_t;

#ifdef _MULTITHREAD //karin: always using original variants in RB_ShowDebug[Text|Lines|Polygons] without multi-threading
debugPolygon_t	rb_debugPolygonss[NUM_FRAME_DATA][ MAX_DEBUG_POLYGONS ];
int				rb_numDebugPolygonss[NUM_FRAME_DATA] = {0};
int				rb_debugPolygonTimes[NUM_FRAME_DATA] = {0};
#else
debugPolygon_t	rb_debugPolygons[ MAX_DEBUG_POLYGONS ];
int				rb_numDebugPolygons = 0;
int				rb_debugPolygonTime = 0;
#endif

#ifdef _MULTITHREAD //karin: always using original variants in RB_ShowDebug[Text|Lines|Polygons] without multi-threading
static volatile int backEndFrameIndex = 0;
#define frontEndFrameIndex ((backEndFrameIndex + 1) % NUM_FRAME_DATA)

#define RB_DEBUG_LINES \
	const int _frameIndex = multithreadActive ? backEndFrameIndex : 0; \
debugLine_t *rb_debugLines = rb_debugLiness[ _frameIndex ]; \
int &rb_numDebugLines = rb_numDebugLiness[_frameIndex]; \
int &rb_debugLineTime = rb_debugLineTimes[_frameIndex];

#define RB_DEBUG_TEXTS \
	const int _frameIndex = multithreadActive ? backEndFrameIndex : 0; \
debugText_t *rb_debugText = rb_debugTexts[ _frameIndex ]; \
int &rb_numDebugText = rb_numDebugTexts[_frameIndex]; \
int &rb_debugTextTime = rb_debugTextTimes[_frameIndex];

#define RB_DEBUG_POLYGONS \
	const int _frameIndex = multithreadActive ? backEndFrameIndex : 0; \
debugPolygon_t *rb_debugPolygons = rb_debugPolygonss[ _frameIndex ]; \
int &rb_numDebugPolygons = rb_numDebugPolygonss[_frameIndex]; \
int &rb_debugPolygonTime = rb_debugPolygonTimes[_frameIndex];


#define TR_DEBUG_LINES \
	const int _frameIndex = multithreadActive ? frontEndFrameIndex : 0; \
debugLine_t *rb_debugLines = rb_debugLiness[ _frameIndex ]; \
int &rb_numDebugLines = rb_numDebugLiness[_frameIndex]; \
int &rb_debugLineTime = rb_debugLineTimes[_frameIndex];

#define TR_DEBUG_TEXTS \
	const int _frameIndex = multithreadActive ? frontEndFrameIndex : 0; \
debugText_t *rb_debugText = rb_debugTexts[ _frameIndex ]; \
int &rb_numDebugText = rb_numDebugTexts[_frameIndex]; \
int &rb_debugTextTime = rb_debugTextTimes[_frameIndex];

#define TR_DEBUG_POLYGONS \
	const int _frameIndex = multithreadActive ? frontEndFrameIndex : 0; \
debugPolygon_t *rb_debugPolygons = rb_debugPolygonss[ _frameIndex ]; \
int &rb_numDebugPolygons = rb_numDebugPolygonss[_frameIndex]; \
int &rb_debugPolygonTime = rb_debugPolygonTimes[_frameIndex];


idCVar harm_r_renderToolsMultithread("harm_r_renderToolsMultithread", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Enable render tools debug with GLES in multi-threading.");

void RB_SetupRenderTools(void)
{
	if(multithreadActive/* && harm_r_renderToolsMultithread.GetBool()*/)
	{
		backEndFrameIndex = frontEndFrameIndex;
	}
}
#endif

static void RB_DrawText(const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align);

#ifdef GL_ES_VERSION_2_0
#include "glsl/gles2_compat.cpp"

static void RB_DrawElementsWithCounters_polygon(const srfTriangles_t *tri)
{
	HARM_CHECK_SHADER("RB_DrawElementsWithCounters_polygon");

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += tri->numIndexes;
	backEnd.pc.c_drawVertexes += tri->numVerts;

	if (tri->ambientSurface != NULL) {
		if (tri->indexes == tri->ambientSurface->indexes) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}

		if (tri->verts == tri->ambientSurface->verts) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	const int numIndexes = r_singleTriangle.GetBool() ? 3 : tri->numIndexes;
	if(backEnd.glState.glStateBits & GLS_POLYMODE_LINE)
	{
		for(int i = 0; i < numIndexes; i += 3)
		{
			if (tri->indexCache) {
				glrbDrawElements(GL_LINES,
						3,
						GL_INDEX_TYPE,
						(glIndex_t *)vertexCache.Position(tri->indexCache) + i);
				backEnd.pc.c_vboIndexes += 3;
			} else {
				vertexCache.UnbindIndex();

				glrbDrawElements(GL_LINES,
						3,
						GL_INDEX_TYPE,
						tri->indexes + i);
			}
		}
	}
	else
	{
		if (tri->indexCache) {
			glrbDrawElements(GL_TRIANGLES,
					numIndexes,
					GL_INDEX_TYPE,
					(int *)vertexCache.Position(tri->indexCache));
			backEnd.pc.c_vboIndexes += tri->numIndexes;
		} else {
			vertexCache.UnbindIndex();

			glrbDrawElements(GL_TRIANGLES,
					numIndexes,
					GL_INDEX_TYPE,
					tri->indexes);
		}
	}
}

static void RB_T_RenderTriangleSurface_polygon(const drawSurf_t *surf)
{
	const srfTriangles_t *tri = surf->geo;

	if (!tri->ambientCache) {
		return;
	}

	idDrawVert *ac = (idDrawVert *)vertexCache.Position(tri->ambientCache);
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof(idDrawVert), ac->xyz.ToFloatPtr());
	GL_VertexAttribPointer(offsetof(shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof(idDrawVert), ac->st.ToFloatPtr());

	RB_DrawElementsWithCounters_polygon(tri);
}
#endif

/*
================
RB_DrawBounds
================
*/
void RB_DrawBounds(const idBounds &bounds)
{
//#if !defined(GL_ES_VERSION_2_0)
	if (bounds.IsCleared()) {
		return;
	}

	glBegin(GL_LINE_LOOP);
	glVertex3f(bounds[0][0], bounds[0][1], bounds[0][2]);
	glVertex3f(bounds[0][0], bounds[1][1], bounds[0][2]);
	glVertex3f(bounds[1][0], bounds[1][1], bounds[0][2]);
	glVertex3f(bounds[1][0], bounds[0][1], bounds[0][2]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(bounds[0][0], bounds[0][1], bounds[1][2]);
	glVertex3f(bounds[0][0], bounds[1][1], bounds[1][2]);
	glVertex3f(bounds[1][0], bounds[1][1], bounds[1][2]);
	glVertex3f(bounds[1][0], bounds[0][1], bounds[1][2]);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(bounds[0][0], bounds[0][1], bounds[0][2]);
	glVertex3f(bounds[0][0], bounds[0][1], bounds[1][2]);

	glVertex3f(bounds[0][0], bounds[1][1], bounds[0][2]);
	glVertex3f(bounds[0][0], bounds[1][1], bounds[1][2]);

	glVertex3f(bounds[1][0], bounds[0][1], bounds[0][2]);
	glVertex3f(bounds[1][0], bounds[0][1], bounds[1][2]);

	glVertex3f(bounds[1][0], bounds[1][1], bounds[0][2]);
	glVertex3f(bounds[1][0], bounds[1][1], bounds[1][2]);
	glEnd();
//#endif
}


/*
================
RB_SimpleSurfaceSetup
================
*/
void RB_SimpleSurfaceSetup(const drawSurf_t *drawSurf)
{
//#if !defined(GL_ES_VERSION_2_0)
	// change the matrix if needed
	if (drawSurf->space != backEnd.currentSpace) {
		glLoadMatrixf(drawSurf->space->modelViewMatrix);
		backEnd.currentSpace = drawSurf->space;
	}
//#endif

	// change the scissor if needed
	if (r_useScissor.GetBool() && !backEnd.currentScissor.Equals(drawSurf->scissorRect)) {
		backEnd.currentScissor = drawSurf->scissorRect;
		qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
	}
}

/*
================
RB_SimpleWorldSetup
================
*/
void RB_SimpleWorldSetup(void)
{
	backEnd.currentSpace = &backEnd.viewDef->worldSpace;
//#if !defined(GL_ES_VERSION_2_0)
	glLoadMatrixf(backEnd.viewDef->worldSpace.modelViewMatrix);
//#endif

	backEnd.currentScissor = backEnd.viewDef->scissor;
	qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
	           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
	           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
	           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);
}

/*
=================
RB_PolygonClear

This will cover the entire screen with normal rasterization.
Texturing is disabled, but the existing glColor, qglDepthMask,
qglColorMask, and the enabled state of depth buffering and
stenciling will matter.
=================
*/
void RB_PolygonClear(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glLoadIdentity();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_SCISSOR_TEST);
	glBegin(GL_POLYGON);
	glVertex3f(-20, -20, -10);
	glVertex3f(20, -20, -10);
	glVertex3f(20, 20, -10);
	glVertex3f(-20, 20, -10);
	glEnd();
	glPopAttrib();
	glPopMatrix();
//#endif
}

/*
====================
RB_ShowDestinationAlpha
====================
*/
void RB_ShowDestinationAlpha(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	GL_State(GLS_SRCBLEND_DST_ALPHA | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS);
	glColor3f(1, 1, 1);
	RB_PolygonClear();
//#endif
}

/*
===================
RB_ScanStencilBuffer

Debugging tool to see what values are in the stencil buffer
===================
*/
void RB_ScanStencilBuffer(void)
{
//#if !defined(GL_ES_VERSION_2_0) // qglReadPixels(GL_STENCIL_INDEX)
#if !defined(__ANDROID__)
    if(!USING_GL)
        return;
	int		counts[256];
	int		i;
	byte	*stencilReadback;

	memset(counts, 0, sizeof(counts));

	stencilReadback = (byte *)R_StaticAlloc(glConfig.vidWidth * glConfig.vidHeight);
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

	for (i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++) {
		counts[ stencilReadback[i] ]++;
	}

	R_StaticFree(stencilReadback);

	// print some stats (not supposed to do from back end in SMP...)
	common->Printf("stencil values:\n");

	for (i = 0 ; i < 255 ; i++) {
		if (counts[i]) {
			common->Printf("%i: %i\n", i, counts[i]);
		}
	}
#endif
//#endif
}


/*
===================
RB_CountStencilBuffer

Print an overdraw count based on stencil index values
===================
*/
void RB_CountStencilBuffer(void)
{
//#if !defined(GL_ES_VERSION_2_0) // qglReadPixels(GL_STENCIL_INDEX)
#if !defined(__ANDROID__)
    if(!USING_GL)
        return;
	int		count;
	int		i;
	byte	*stencilReadback;


	stencilReadback = (byte *)R_StaticAlloc(glConfig.vidWidth * glConfig.vidHeight);
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

	count = 0;

	for (i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++) {
		count += stencilReadback[i];
	}

	R_StaticFree(stencilReadback);

	// print some stats (not supposed to do from back end in SMP...)
	common->Printf("overdraw: %5.1f\n", (float)count/(glConfig.vidWidth * glConfig.vidHeight));
#endif
//#endif
}

/*
===================
R_ColorByStencilBuffer

Sets the screen colors based on the contents of the
stencil buffer.  Stencil of 0 = black, 1 = red, 2 = green,
3 = blue, ..., 7+ = white
===================
*/
static void R_ColorByStencilBuffer(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i;
	static float	colors[8][3] = {
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
	qglClearColor(1, 1, 1, 1);
	qglDisable(GL_SCISSOR_TEST);
	qglClear(GL_COLOR_BUFFER_BIT);

	// now draw color for each stencil value
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	for (i = 0 ; i < 6 ; i++) {
		glColor3fv(colors[i]);
		qglStencilFunc(GL_EQUAL, i, 255);
		RB_PolygonClear();
	}

	qglStencilFunc(GL_ALWAYS, 0, 255);
//#endif
}

//======================================================================

/*
==================
RB_ShowOverdraw
==================
*/
void RB_ShowOverdraw(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	const idMaterial 	*material;
	int					i;
	drawSurf_t * *		drawSurfs;
	const drawSurf_t 	*surf;
	int					numDrawSurfs;
	viewLight_t 		*vLight;

	if (r_showOverDraw.GetInteger() == 0) {
		return;
	}

	material = declManager->FindMaterial("textures/common/overdrawtest", false);

	if (material == NULL) {
		return;
	}

	drawSurfs = backEnd.viewDef->drawSurfs;
	numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	int interactions = 0;

	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {
		for (surf = vLight->localInteractions; surf; surf = surf->nextOnLight) {
			interactions++;
		}

		for (surf = vLight->globalInteractions; surf; surf = surf->nextOnLight) {
			interactions++;
		}
	}

	drawSurf_t **newDrawSurfs = (drawSurf_t **)R_FrameAlloc(numDrawSurfs + interactions * sizeof(newDrawSurfs[0]));

	for (i = 0; i < numDrawSurfs; i++) {
		surf = drawSurfs[i];

		if (surf->material) {
			const_cast<drawSurf_t *>(surf)->material = material;
		}

		newDrawSurfs[i] = const_cast<drawSurf_t *>(surf);
	}

	for (vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next) {
		for (surf = vLight->localInteractions; surf; surf = surf->nextOnLight) {
			const_cast<drawSurf_t *>(surf)->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t *>(surf);
		}

		for (surf = vLight->globalInteractions; surf; surf = surf->nextOnLight) {
			const_cast<drawSurf_t *>(surf)->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t *>(surf);
		}

		vLight->localInteractions = NULL;
		vLight->globalInteractions = NULL;
	}

	switch (r_showOverDraw.GetInteger()) {
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
//#endif
}

/*
===================
RB_ShowIntensity

Debugging tool to see how much dynamic range a scene is using.
The greatest of the rgb values at each pixel will be used, with
the resulting color shading from red at 0 to green at 128 to blue at 255
===================
*/
void RB_ShowIntensity(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	byte	*colorReadback;
	int		i, j, c;

	if (!r_showIntensity.GetBool()) {
		return;
	}

	colorReadback = (byte *)R_StaticAlloc(glConfig.vidWidth * glConfig.vidHeight * 4);
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, colorReadback);

	c = glConfig.vidWidth * glConfig.vidHeight * 4;

	for (i = 0; i < c ; i+=4) {
		j = colorReadback[i];

		if (colorReadback[i+1] > j) {
			j = colorReadback[i+1];
		}

		if (colorReadback[i+2] > j) {
			j = colorReadback[i+2];
		}

		if (j < 128) {
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
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	GL_State(GLS_DEPTHFUNC_ALWAYS);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glRasterPos2f(0, 0);
	glPopMatrix();
	glColor3f(1, 1, 1);
	globalImages->BindNull();
	glMatrixMode(GL_MODELVIEW);

	glDrawPixels(glConfig.vidWidth, glConfig.vidHeight, GL_RGBA , GL_UNSIGNED_BYTE, colorReadback);

	R_StaticFree(colorReadback);
//#endif
}


/*
===================
RB_ShowDepthBuffer

Draw the depth buffer as colors
===================
*/
void RB_ShowDepthBuffer(void)
{
//#if !defined(GL_ES_VERSION_2_0) // qglReadPixels(GL_DEPTH_COMPONENT)
#if !defined(__ANDROID__)
    if(!USING_GL)
        return;
	void	*depthReadback;

	if (!r_showDepth.GetBool()) {
		return;
	}

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glRasterPos2f(0, 0);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	GL_State(GLS_DEPTHFUNC_ALWAYS);
	glColor3f(1, 1, 1);
	globalImages->BindNull();

	depthReadback = R_StaticAlloc(glConfig.vidWidth * glConfig.vidHeight*4);
	memset(depthReadback, 0, glConfig.vidWidth * glConfig.vidHeight*4);

	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT , GL_FLOAT, depthReadback);

#if 0

	for (int i = 0 ; i < glConfig.vidWidth * glConfig.vidHeight ; i++) {
		((byte *)depthReadback)[i*4] =
		        ((byte *)depthReadback)[i*4+1] =
		                ((byte *)depthReadback)[i*4+2] = 255 * ((float *)depthReadback)[i];
		((byte *)depthReadback)[i*4+3] = 1;
	}

#endif

	glDrawPixels(glConfig.vidWidth, glConfig.vidHeight, GL_RGBA , GL_UNSIGNED_BYTE, depthReadback);
	R_StaticFree(depthReadback);
#endif
//#endif
}

/*
=================
RB_ShowLightCount

This is a debugging tool that will draw each surface with a color
based on how many lights are effecting it
=================
*/
void RB_ShowLightCount(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i;
	const drawSurf_t	*surf;
	const viewLight_t	*vLight;

	if (!r_showLightCount.GetBool()) {
		return;
	}

	GL_State(GLS_DEPTHFUNC_EQUAL);

	RB_SimpleWorldSetup();
	qglClearStencil(0);
	qglClear(GL_STENCIL_BUFFER_BIT);

	qglEnable(GL_STENCIL_TEST);

	// optionally count everything through walls
	if (r_showLightCount.GetInteger() >= 2) {
		qglStencilOp(GL_KEEP, GL_INCR, GL_INCR);
	} else {
		qglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	}

	qglStencilFunc(GL_ALWAYS, 1, 255);

	globalImages->defaultImage->Bind();

	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) {
		for (i = 0 ; i < 2 ; i++) {
			for (surf = i ? vLight->localInteractions: vLight->globalInteractions; surf; surf = (drawSurf_t *)surf->nextOnLight) {
				RB_SimpleSurfaceSetup(surf);

				if (!surf->geo->ambientCache) {
					continue;
				}

#ifdef GL_ES_VERSION_2_0
				glrbStartRender();
#endif
				const idDrawVert	*ac = (idDrawVert *)vertexCache.Position(surf->geo->ambientCache);

				glVertexPointer(3, GL_FLOAT, sizeof(idDrawVert), &ac->xyz);

				RB_DrawElementsWithCounters(surf->geo);
#ifdef GL_ES_VERSION_2_0
				glrbEndRender();
#endif
			}
		}
	}

	// display the results
	R_ColorByStencilBuffer();

	if (r_showLightCount.GetInteger() > 2) {
		RB_CountStencilBuffer();
	}
//#endif
}


/*
=================
RB_ShowSilhouette

Blacks out all edges, then adds color for each edge that a shadow
plane extends from, allowing you to see doubled edges
=================
*/
void RB_ShowSilhouette(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i;
	const drawSurf_t	*surf;
	const viewLight_t	*vLight;

	if (!r_showSilhouette.GetBool()) {
		return;
	}

	//
	// clear all triangle edges to black
	//
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_STENCIL_TEST);

	glColor3f(0, 0, 0);

	GL_State(GLS_POLYMODE_LINE);

	GL_Cull(CT_TWO_SIDED);
	qglDisable(GL_DEPTH_TEST);

#ifdef GL_ES_VERSION_2_0
	glrbStartRender();
#endif
	RB_RenderDrawSurfListWithFunction(backEnd.viewDef->drawSurfs, backEnd.viewDef->numDrawSurfs,
	                                  RB_T_RenderTriangleSurface_polygon);
#ifdef GL_ES_VERSION_2_0
	glrbEndRender();
#endif


	//
	// now blend in edges that cast silhouettes
	//
	RB_SimpleWorldSetup();
	glColor3f(0.5, 0, 0);
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) {
		for (i = 0 ; i < 2 ; i++) {
			for (surf = i ? vLight->localShadows : vLight->globalShadows
			            ; surf ; surf = (drawSurf_t *)surf->nextOnLight) {
				RB_SimpleSurfaceSetup(surf);

				const srfTriangles_t	*tri = surf->geo;

#ifdef GL_ES_VERSION_2_0
				glrbStartRender();
#endif
				glVertexPointer(3, GL_FLOAT, sizeof(shadowCache_t), vertexCache.Position(tri->shadowCache));
				glBegin(GL_LINES);

				for (int j = 0 ; j < tri->numIndexes ; j+=3) {
					int		i1 = tri->indexes[j+0];
					int		i2 = tri->indexes[j+1];
					int		i3 = tri->indexes[j+2];

					if ((i1 & 1) + (i2 & 1) + (i3 & 1) == 1) {
						if ((i1 & 1) + (i2 & 1) == 0) {
							glArrayElement(i1);
							glArrayElement(i2);
						} else if ((i1 & 1) + (i3 & 1) == 0) {
							glArrayElement(i1);
							glArrayElement(i3);
						}
					}
				}

				glEnd();

#ifdef GL_ES_VERSION_2_0
				glrbEndRender();
#endif
			}
		}
	}

	qglEnable(GL_DEPTH_TEST);

	GL_State(GLS_DEFAULT);
	glColor3f(1,1,1);
	GL_Cull(CT_FRONT_SIDED);
//#endif
}



/*
=================
RB_ShowShadowCount

This is a debugging tool that will draw only the shadow volumes
and count up the total fill usage
=================
*/
static void RB_ShowShadowCount(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i;
	const drawSurf_t	*surf;
	const viewLight_t	*vLight;

	if (!r_showShadowCount.GetBool()) {
		return;
	}

	GL_State(GLS_DEFAULT);

	qglClearStencil(0);
	qglClear(GL_STENCIL_BUFFER_BIT);

	qglEnable(GL_STENCIL_TEST);

	qglStencilOp(GL_KEEP, GL_INCR, GL_INCR);

	qglStencilFunc(GL_ALWAYS, 1, 255);

	globalImages->defaultImage->Bind();

	// draw both sides
	GL_Cull(CT_TWO_SIDED);

	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) {
		for (i = 0 ; i < 2 ; i++) {
			for (surf = i ? vLight->localShadows : vLight->globalShadows
			            ; surf ; surf = (drawSurf_t *)surf->nextOnLight) {
				RB_SimpleSurfaceSetup(surf);
				const srfTriangles_t	*tri = surf->geo;

				if (!tri->shadowCache) {
					continue;
				}

				if (r_showShadowCount.GetInteger() == 3) {
					// only show turboshadows
					if (tri->numShadowIndexesNoCaps != tri->numIndexes) {
						continue;
					}
				}

				if (r_showShadowCount.GetInteger() == 4) {
					// only show static shadows
					if (tri->numShadowIndexesNoCaps == tri->numIndexes) {
						continue;
					}
				}

				shadowCache_t *cache = (shadowCache_t *)vertexCache.Position(tri->shadowCache);
#ifdef GL_ES_VERSION_2_0
				glrbStartRender();
#endif
				glVertexPointer(4, GL_FLOAT, sizeof(*cache), &cache->xyz);
				RB_DrawElementsWithCounters(tri);
#ifdef GL_ES_VERSION_2_0
				glrbEndRender();
#endif
			}
		}
	}

	// display the results
	R_ColorByStencilBuffer();

	if (r_showShadowCount.GetInteger() == 2) {
		common->Printf("all shadows ");
	} else if (r_showShadowCount.GetInteger() == 3) {
		common->Printf("turboShadows ");
	} else if (r_showShadowCount.GetInteger() == 4) {
		common->Printf("static shadows ");
	}

	if (r_showShadowCount.GetInteger() >= 2) {
		RB_CountStencilBuffer();
	}

	GL_Cull(CT_FRONT_SIDED);
//#endif
}


/*
===============
RB_T_RenderTriangleSurfaceAsLines

===============
*/
void RB_T_RenderTriangleSurfaceAsLines(const drawSurf_t *surf)
{
//#if !defined(GL_ES_VERSION_2_0)
	const srfTriangles_t *tri = surf->geo;

	if (!tri->verts) {
		return;
	}

	glBegin(GL_LINES);

	for (int i = 0 ; i < tri->numIndexes ; i+= 3) {
		for (int j = 0 ; j < 3 ; j++) {
			int k = (j + 1) % 3;
			glVertex3fv(tri->verts[ tri->silIndexes[i+j] ].xyz.ToFloatPtr());
			glVertex3fv(tri->verts[ tri->silIndexes[i+k] ].xyz.ToFloatPtr());
		}
	}

	glEnd();
//#endif
}


/*
=====================
RB_ShowTris

Debugging tool
=====================
*/
static void RB_ShowTris(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	modelTrace_t mt;
	idVec3 end;

	if (!r_showTris.GetInteger()) {
		return;
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_STENCIL_TEST);

	glColor3f(1, 1, 1);


	GL_State(GLS_POLYMODE_LINE);

	switch (r_showTris.GetInteger()) {
		case 1:	// only draw visible ones
			qglPolygonOffset(-1, -2);
#if !defined(GL_ES_VERSION_2_0)
			qglEnable(GL_POLYGON_OFFSET_LINE);
#endif
			break;
		default:
		case 2:	// draw all front facing
			GL_Cull(CT_FRONT_SIDED);
			qglDisable(GL_DEPTH_TEST);
			break;
		case 3: // draw all
			GL_Cull(CT_TWO_SIDED);
			qglDisable(GL_DEPTH_TEST);
			break;
	}

#ifdef GL_ES_VERSION_2_0
	glrbStartRender();
#endif
	RB_RenderDrawSurfListWithFunction(drawSurfs, numDrawSurfs, RB_T_RenderTriangleSurface_polygon);
#ifdef GL_ES_VERSION_2_0
	glrbEndRender();
#endif

	qglEnable(GL_DEPTH_TEST);
#if !defined(GL_ES_VERSION_2_0)
	qglDisable(GL_POLYGON_OFFSET_LINE);
#endif

	glDepthRange(0, 1);
	GL_State(GLS_DEFAULT);
	GL_Cull(CT_FRONT_SIDED);
//#endif
}


/*
=====================
RB_ShowSurfaceInfo

Debugging tool
=====================
*/
static void RB_ShowSurfaceInfo(drawSurf_t **drawSurfs, int numDrawSurfs)
{
#ifdef _MULTITHREAD //karin: using R_ShowSurfaceInfo on frontend on multi-threading
	if(multithreadActive)
		return;
#endif
	modelTrace_t mt;
	idVec3 start, end;

	if (!r_showSurfaceInfo.GetBool()) {
		return;
	}

	// start far enough away that we don't hit the player model
	start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;

	if (!tr.primaryWorld->Trace(mt, start, end, 0.0f, false)) {
		return;
	}

#if 1 //!defined(GL_ES_VERSION_2_0)
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_STENCIL_TEST);

	glColor3f(1, 1, 1);

	GL_State(GLS_POLYMODE_LINE);

	qglPolygonOffset(-1, -2);
#if !defined(GL_ES_VERSION_2_0)
	qglEnable(GL_POLYGON_OFFSET_LINE);
#endif

	idVec3	trans[3];
	float	matrix[16];

	// transform the object verts into global space
	R_AxisToModelMatrix(mt.entity->axis, mt.entity->origin, matrix);

	tr.primaryWorld->DrawText(mt.entity->hModel->Name(), mt.point + tr.primaryView->renderView.viewaxis[2] * 12,
	                          0.35f, colorRed, tr.primaryView->renderView.viewaxis);
	tr.primaryWorld->DrawText(mt.material->GetName(), mt.point,
	                          0.35f, colorBlue, tr.primaryView->renderView.viewaxis);
#ifdef _RAVEN //karin: show materialType
	if(mt.materialType)
	{
		tr.primaryWorld->DrawText(mt.materialType->GetName(), mt.point - tr.primaryView->renderView.viewaxis[2] * 12,
				0.35f, colorGreen, tr.primaryView->renderView.viewaxis);
	}
#endif
#ifdef _K_DEV //karin: show stage texgen
	idStr str;
	const char *TG_NAMES[] = {
		"TG_EXPLICIT",
		"TG_DIFFUSE_CUBE",
		"TG_REFLECT_CUBE",
		"TG_SKYBOX_CUBE",
		"TG_WOBBLESKY_CUBE",
		"TG_SCREEN",
		"TG_SCREEN2",
		"TG_GLASSWARP",
	};
	for(int i = 0; i < mt.material->GetNumStages(); i++)
		str += va("%d. %s\n", i, TG_NAMES[mt.material->GetStage(i)->texture.texgen]);
	tr.primaryWorld->DrawText(str.c_str(), mt.point - tr.primaryView->renderView.viewaxis[2] * 24,
	                          0.35f, colorRed, tr.primaryView->renderView.viewaxis);
#endif

	qglEnable(GL_DEPTH_TEST);
#if !defined(GL_ES_VERSION_2_0)
	qglDisable(GL_POLYGON_OFFSET_LINE);
#endif

	glDepthRange(0, 1);
	GL_State(GLS_DEFAULT);
	GL_Cull(CT_FRONT_SIDED);
#else
	common->Printf("RB_ShowSurfaceInfo: surf = %s material = %s \n", mt.entity->hModel->Name(),
			mt.material->GetName());
#endif
}


/*
=====================
RB_ShowViewEntitys

Debugging tool
=====================
*/
static void RB_ShowViewEntitys(viewEntity_t *vModels)
{
	if (!r_showViewEntitys.GetBool()) {
		return;
	}

	if (r_showViewEntitys.GetInteger() == 2) {
		common->Printf("view entities: ");

		for (; vModels ; vModels = vModels->next) {
			common->Printf("%i ", vModels->entityDef->index);
		}

		common->Printf("\n");
		return;
	}

//#if !defined(GL_ES_VERSION_2_0)
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_STENCIL_TEST);

	glColor3f(1, 1, 1);


	GL_State(GLS_POLYMODE_LINE);

	GL_Cull(CT_TWO_SIDED);
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_SCISSOR_TEST);

	for (; vModels ; vModels = vModels->next) {
		idBounds	b;

		glLoadMatrixf(vModels->modelViewMatrix);

		if (!vModels->entityDef) {
			continue;
		}

		// draw the reference bounds in yellow
		glColor3f(1, 1, 0);
		RB_DrawBounds(vModels->entityDef->referenceBounds);


		// draw the model bounds in white
		glColor3f(1, 1, 1);

		idRenderModel *model = R_EntityDefDynamicModel(vModels->entityDef);

		if (!model) {
			continue;	// particles won't instantiate without a current view
		}

		b = model->Bounds(&vModels->entityDef->parms);
		RB_DrawBounds(b);
	}

	qglEnable(GL_DEPTH_TEST);
#if !defined(GL_ES_VERSION_2_0)
	qglDisable(GL_POLYGON_OFFSET_LINE);
#endif

	glDepthRange(0, 1);
	GL_State(GLS_DEFAULT);
	GL_Cull(CT_FRONT_SIDED);
//#endif
}

/*
=====================
RB_ShowTexturePolarity

Shade triangle red if they have a positive texture area
green if they have a negative texture area, or blue if degenerate area
=====================
*/
static void RB_ShowTexturePolarity(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if (!r_showTexturePolarity.GetBool()) {
		return;
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);

	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	glColor3f(1, 1, 1);

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];
		tri = drawSurf->geo;

		if (!tri->verts) {
			continue;
		}

		RB_SimpleSurfaceSetup(drawSurf);

		glBegin(GL_TRIANGLES);

		for (j = 0 ; j < tri->numIndexes ; j+=3) {
			idDrawVert	*a, *b, *c;
			float		d0[5], d1[5];
			float		area;

			a = tri->verts + tri->indexes[j];
			b = tri->verts + tri->indexes[j+1];
			c = tri->verts + tri->indexes[j+2];

			// VectorSubtract( b->xyz, a->xyz, d0 );
			d0[3] = b->st[0] - a->st[0];
			d0[4] = b->st[1] - a->st[1];
			// VectorSubtract( c->xyz, a->xyz, d1 );
			d1[3] = c->st[0] - a->st[0];
			d1[4] = c->st[1] - a->st[1];

			area = d0[3] * d1[4] - d0[4] * d1[3];

			if (idMath::Fabs(area) < 0.0001) {
				glColor4f(0, 0, 1, 0.5);
			} else  if (area < 0) {
				glColor4f(1, 0, 0, 0.5);
			} else {
				glColor4f(0, 1, 0, 0.5);
			}

			glVertex3fv(a->xyz.ToFloatPtr());
			glVertex3fv(b->xyz.ToFloatPtr());
			glVertex3fv(c->xyz.ToFloatPtr());
		}

		glEnd();
	}

	GL_State(GLS_DEFAULT);
//#endif
}


/*
=====================
RB_ShowUnsmoothedTangents

Shade materials that are using unsmoothed tangents
=====================
*/
static void RB_ShowUnsmoothedTangents(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if (!r_showUnsmoothedTangents.GetBool()) {
		return;
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);

	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	glColor4f(0, 1, 0, 0.5);

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		if (!drawSurf->material->UseUnsmoothedTangents()) {
			continue;
		}

		RB_SimpleSurfaceSetup(drawSurf);

		tri = drawSurf->geo;
		glBegin(GL_TRIANGLES);

		for (j = 0 ; j < tri->numIndexes ; j+=3) {
			idDrawVert	*a, *b, *c;

			a = tri->verts + tri->indexes[j];
			b = tri->verts + tri->indexes[j+1];
			c = tri->verts + tri->indexes[j+2];

			glVertex3fv(a->xyz.ToFloatPtr());
			glVertex3fv(b->xyz.ToFloatPtr());
			glVertex3fv(c->xyz.ToFloatPtr());
		}

		glEnd();
	}

	GL_State(GLS_DEFAULT);
//#endif
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
static void RB_ShowTangentSpace(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if (!r_showTangentSpace.GetInteger()) {
		return;
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);

	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		RB_SimpleSurfaceSetup(drawSurf);

		tri = drawSurf->geo;

		if (!tri->verts) {
			continue;
		}

		glBegin(GL_TRIANGLES);

		for (j = 0 ; j < tri->numIndexes ; j++) {
			const idDrawVert *v;

			v = &tri->verts[tri->indexes[j]];

			if (r_showTangentSpace.GetInteger() == 1) {
				glColor4f(0.5 + 0.5 * v->tangents[0][0],  0.5 + 0.5 * v->tangents[0][1],
				           0.5 + 0.5 * v->tangents[0][2], 0.5);
			} else if (r_showTangentSpace.GetInteger() == 2) {
				glColor4f(0.5 + 0.5 * v->tangents[1][0],  0.5 + 0.5 * v->tangents[1][1],
				           0.5 + 0.5 * v->tangents[1][2], 0.5);
			} else {
				glColor4f(0.5 + 0.5 * v->normal[0],  0.5 + 0.5 * v->normal[1],
				           0.5 + 0.5 * v->normal[2], 0.5);
			}

			glVertex3fv(v->xyz.ToFloatPtr());
		}

		glEnd();
	}

	GL_State(GLS_DEFAULT);
//#endif
}

/*
=====================
RB_ShowVertexColor

Draw each triangle with the solid vertex colors
=====================
*/
static void RB_ShowVertexColor(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int		i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if (!r_showVertexColor.GetBool()) {
		return;
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);

	GL_State(GLS_DEPTHFUNC_LESS);

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		RB_SimpleSurfaceSetup(drawSurf);

		tri = drawSurf->geo;

		if (!tri->verts) {
			continue;
		}

		glBegin(GL_TRIANGLES);

		for (j = 0 ; j < tri->numIndexes ; j++) {
			const idDrawVert *v;

			v = &tri->verts[tri->indexes[j]];
			glColor4ubv(v->color);
			glVertex3fv(v->xyz.ToFloatPtr());
		}

		glEnd();
	}

	GL_State(GLS_DEFAULT);
//#endif
}


/*
=====================
RB_ShowNormals

Debugging tool
=====================
*/
static void RB_ShowNormals(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i, j;
	drawSurf_t	*drawSurf;
	idVec3		end;
	const srfTriangles_t	*tri;
	float		size;
	bool		showNumbers;
	idVec3		pos;

	if (r_showNormals.GetFloat() == 0.0f) {
		return;
	}

	GL_State(GLS_POLYMODE_LINE);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);

	if (!r_debugLineDepthTest.GetBool()) {
		qglDisable(GL_DEPTH_TEST);
	} else {
		qglEnable(GL_DEPTH_TEST);
	}

	size = r_showNormals.GetFloat();

	if (size < 0.0f) {
		size = -size;
		showNumbers = true;
	} else {
		showNumbers = false;
	}

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		RB_SimpleSurfaceSetup(drawSurf);

		tri = drawSurf->geo;

		if (!tri->verts) {
			continue;
		}

		glBegin(GL_LINES);

		for (j = 0 ; j < tri->numVerts ; j++) {
			glColor3f(0, 0, 1);
			glVertex3fv(tri->verts[j].xyz.ToFloatPtr());
			VectorMA(tri->verts[j].xyz, size, tri->verts[j].normal, end);
			glVertex3fv(end.ToFloatPtr());

			glColor3f(1, 0, 0);
			glVertex3fv(tri->verts[j].xyz.ToFloatPtr());
			VectorMA(tri->verts[j].xyz, size, tri->verts[j].tangents[0], end);
			glVertex3fv(end.ToFloatPtr());

			glColor3f(0, 1, 0);
			glVertex3fv(tri->verts[j].xyz.ToFloatPtr());
			VectorMA(tri->verts[j].xyz, size, tri->verts[j].tangents[1], end);
			glVertex3fv(end.ToFloatPtr());
		}

		glEnd();
	}

	if (showNumbers) {
		RB_SimpleWorldSetup();

		for (i = 0 ; i < numDrawSurfs ; i++) {
			drawSurf = drawSurfs[i];
			tri = drawSurf->geo;

			if (!tri->verts) {
				continue;
			}

			for (j = 0 ; j < tri->numVerts ; j++) {
				R_LocalPointToGlobal(drawSurf->space->modelMatrix, tri->verts[j].xyz + tri->verts[j].tangents[0] + tri->verts[j].normal * 0.2f, pos);
				RB_DrawText(va("%d", j), pos, 0.01f, colorWhite, backEnd.viewDef->renderView.viewaxis, 1);
			}

			for (j = 0 ; j < tri->numIndexes; j += 3) {
				R_LocalPointToGlobal(drawSurf->space->modelMatrix, (tri->verts[ tri->indexes[ j + 0 ] ].xyz + tri->verts[ tri->indexes[ j + 1 ] ].xyz + tri->verts[ tri->indexes[ j + 2 ] ].xyz) *(1.0f / 3.0f) + tri->verts[ tri->indexes[ j + 0 ] ].normal * 0.2f, pos);
				RB_DrawText(va("%d", j / 3), pos, 0.01f, colorCyan, backEnd.viewDef->renderView.viewaxis, 1);
			}
		}
	}

	qglEnable(GL_STENCIL_TEST);
//#endif
}


/*
=====================
RB_ShowNormals

Debugging tool
=====================
*/
static void RB_AltShowNormals(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i, j, k;
	drawSurf_t	*drawSurf;
	idVec3		end;
	const srfTriangles_t	*tri;

	if (r_showNormals.GetFloat() == 0.0f) {
		return;
	}

	GL_State(GLS_DEFAULT);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);
	qglDisable(GL_DEPTH_TEST);

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		RB_SimpleSurfaceSetup(drawSurf);

		tri = drawSurf->geo;
		glBegin(GL_LINES);

		for (j = 0 ; j < tri->numIndexes ; j += 3) {
			const idDrawVert *v[3];
			idVec3		mid;

			v[0] = &tri->verts[tri->indexes[j+0]];
			v[1] = &tri->verts[tri->indexes[j+1]];
			v[2] = &tri->verts[tri->indexes[j+2]];

			// make the midpoint slightly above the triangle
			mid = (v[0]->xyz + v[1]->xyz + v[2]->xyz) * (1.0f / 3.0f);
			mid += 0.1f * tri->facePlanes[ j / 3 ].Normal();

			for (k = 0 ; k < 3 ; k++) {
				idVec3	pos;

				pos = (mid + v[k]->xyz * 3.0f) * 0.25f;

				glColor3f(0, 0, 1);
				glVertex3fv(pos.ToFloatPtr());
				VectorMA(pos, r_showNormals.GetFloat(), v[k]->normal, end);
				glVertex3fv(end.ToFloatPtr());

				glColor3f(1, 0, 0);
				glVertex3fv(pos.ToFloatPtr());
				VectorMA(pos, r_showNormals.GetFloat(), v[k]->tangents[0], end);
				glVertex3fv(end.ToFloatPtr());

				glColor3f(0, 1, 0);
				glVertex3fv(pos.ToFloatPtr());
				VectorMA(pos, r_showNormals.GetFloat(), v[k]->tangents[1], end);
				glVertex3fv(end.ToFloatPtr());

				glColor3f(1, 1, 1);
				glVertex3fv(pos.ToFloatPtr());
				glVertex3fv(v[k]->xyz.ToFloatPtr());
			}
		}

		glEnd();
	}

	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_STENCIL_TEST);
//#endif
}



/*
=====================
RB_ShowTextureVectors

Draw texture vectors in the center of each triangle
=====================
*/
static void RB_ShowTextureVectors(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if (r_showTextureVectors.GetFloat() == 0.0f) {
		return;
	}

	GL_State(GLS_DEPTHFUNC_LESS);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	globalImages->BindNull();

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		tri = drawSurf->geo;

		if (!tri->verts) {
			continue;
		}

		if (!tri->facePlanes) {
			continue;
		}

		RB_SimpleSurfaceSetup(drawSurf);

		// draw non-shared edges in yellow
		glBegin(GL_LINES);

		for (j = 0 ; j < tri->numIndexes ; j+= 3) {
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
			mid = (a->xyz + b->xyz + c->xyz) * (1.0f / 3.0f);
			mid += 0.1f * tri->facePlanes[ j / 3 ].Normal();

			// calculate the texture vectors
			VectorSubtract(b->xyz, a->xyz, d0);
			d0[3] = b->st[0] - a->st[0];
			d0[4] = b->st[1] - a->st[1];
			VectorSubtract(c->xyz, a->xyz, d1);
			d1[3] = c->st[0] - a->st[0];
			d1[4] = c->st[1] - a->st[1];

			area = d0[3] * d1[4] - d0[4] * d1[3];

			if (area == 0) {
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

			glColor3f(1, 0, 0);
			glVertex3fv(mid.ToFloatPtr());
			glVertex3fv(tangents[0].ToFloatPtr());

			glColor3f(0, 1, 0);
			glVertex3fv(mid.ToFloatPtr());
			glVertex3fv(tangents[1].ToFloatPtr());
		}

		glEnd();
	}
//#endif
}

/*
=====================
RB_ShowDominantTris

Draw lines from each vertex to the dominant triangle center
=====================
*/
static void RB_ShowDominantTris(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i, j;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;

	if (!r_showDominantTri.GetBool()) {
		return;
	}

	GL_State(GLS_DEPTHFUNC_LESS);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglPolygonOffset(-1, -2);
#if !defined(GL_ES_VERSION_2_0)
	qglEnable(GL_POLYGON_OFFSET_LINE);
#endif

	globalImages->BindNull();

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		tri = drawSurf->geo;

		if (!tri->verts) {
			continue;
		}

		if (!tri->dominantTris) {
			continue;
		}

		RB_SimpleSurfaceSetup(drawSurf);

		glColor3f(1, 1, 0);
		glBegin(GL_LINES);

		for (j = 0 ; j < tri->numVerts ; j++) {
			const idDrawVert *a, *b, *c;
			idVec3		mid;

			// find the midpoint of the dominant tri

			a = &tri->verts[j];
			b = &tri->verts[tri->dominantTris[j].v2];
			c = &tri->verts[tri->dominantTris[j].v3];

			mid = (a->xyz + b->xyz + c->xyz) * (1.0f / 3.0f);

			glVertex3fv(mid.ToFloatPtr());
			glVertex3fv(a->xyz.ToFloatPtr());
		}

		glEnd();
	}

#if !defined(GL_ES_VERSION_2_0)
	qglDisable(GL_POLYGON_OFFSET_LINE);
#endif
//#endif
}

/*
=====================
RB_ShowEdges

Debugging tool
=====================
*/
static void RB_ShowEdges(drawSurf_t **drawSurfs, int numDrawSurfs)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i, j, k, m, n, o;
	drawSurf_t	*drawSurf;
	const srfTriangles_t	*tri;
	const silEdge_t			*edge;
	int			danglePlane;

	if (!r_showEdges.GetBool()) {
		return;
	}

	GL_State(GLS_DEFAULT);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	globalImages->BindNull();
	qglDisable(GL_DEPTH_TEST);

	for (i = 0 ; i < numDrawSurfs ; i++) {
		drawSurf = drawSurfs[i];

		tri = drawSurf->geo;

		idDrawVert *ac = (idDrawVert *)tri->verts;

		if (!ac) {
			continue;
		}

		RB_SimpleSurfaceSetup(drawSurf);

		// draw non-shared edges in yellow
		glColor3f(1, 1, 0);
		glBegin(GL_LINES);

		for (j = 0 ; j < tri->numIndexes ; j+= 3) {
			for (k = 0 ; k < 3 ; k++) {
				int		l, i1, i2;
				l = (k == 2) ? 0 : k + 1;
				i1 = tri->indexes[j+k];
				i2 = tri->indexes[j+l];

				// if these are used backwards, the edge is shared
				for (m = 0 ; m < tri->numIndexes ; m += 3) {
					for (n = 0 ; n < 3 ; n++) {
						o = (n == 2) ? 0 : n + 1;

						if (tri->indexes[m+n] == i2 && tri->indexes[m+o] == i1) {
							break;
						}
					}

					if (n != 3) {
						break;
					}
				}

				// if we didn't find a backwards listing, draw it in yellow
				if (m == tri->numIndexes) {
					glVertex3fv(ac[ i1 ].xyz.ToFloatPtr());
					glVertex3fv(ac[ i2 ].xyz.ToFloatPtr());
				}

			}
		}

		glEnd();

		// draw dangling sil edges in red
		if (!tri->silEdges) {
			continue;
		}

		// the plane number after all real planes
		// is the dangling edge
		danglePlane = tri->numIndexes / 3;

		glColor3f(1, 0, 0);

		glBegin(GL_LINES);

		for (j = 0 ; j < tri->numSilEdges ; j++) {
			edge = tri->silEdges + j;

			if (edge->p1 != danglePlane && edge->p2 != danglePlane) {
				continue;
			}

			glVertex3fv(ac[ edge->v1 ].xyz.ToFloatPtr());
			glVertex3fv(ac[ edge->v2 ].xyz.ToFloatPtr());
		}

		glEnd();
	}

	qglEnable(GL_DEPTH_TEST);
//#endif
}

/*
==============
RB_ShowLights

Visualize all light volumes used in the current scene
r_showLights 1	: just print volumes numbers, highlighting ones covering the view
r_showLights 2	: also draw planes of each volume
r_showLights 3	: also draw edges of each volume
==============
*/
void RB_ShowLights(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	const idRenderLightLocal	*light;
	int					count;
	srfTriangles_t		*tri;
	viewLight_t			*vLight;

	if (!r_showLights.GetInteger()) {
		return;
	}

	// all volumes are expressed in world coordinates
	RB_SimpleWorldSetup();

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	globalImages->BindNull();
	qglDisable(GL_STENCIL_TEST);


	GL_Cull(CT_TWO_SIDED);
	qglDisable(GL_DEPTH_TEST);


	common->Printf("volumes: ");	// FIXME: not in back end!

	count = 0;

	for (vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next) {
		light = vLight->lightDef;
		count++;

		tri = light->frustumTris;

		// depth buffered planes
		if (r_showLights.GetInteger() >= 2) {
			GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK);
			glColor4f(0, 0, 1, 0.25);
			qglEnable(GL_DEPTH_TEST);
#ifdef GL_ES_VERSION_2_0
			glrbStartRender();
#endif
			RB_RenderTriangleSurface(tri);
#ifdef GL_ES_VERSION_2_0
			glrbEndRender();
#endif
		}

		// non-hidden lines
		if (r_showLights.GetInteger() >= 3) {
			GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK);
			qglDisable(GL_DEPTH_TEST);
			glColor3f(1, 1, 1);
#ifdef GL_ES_VERSION_2_0
			glrbStartRender();
#endif
			RB_RenderTriangleSurface(tri);
#ifdef GL_ES_VERSION_2_0
			glrbEndRender();
#endif
		}

		int index;

		index = backEnd.viewDef->renderWorld->lightDefs.FindIndex(vLight->lightDef);

		if (vLight->viewInsideLight) {
			// view is in this volume
			common->Printf("[%i] ", index);
		} else {
			common->Printf("%i ", index);
		}
	}

	qglEnable(GL_DEPTH_TEST);
#if !defined(GL_ES_VERSION_2_0)
	qglDisable(GL_POLYGON_OFFSET_LINE);
#endif

	glDepthRange(0, 1);
	GL_State(GLS_DEFAULT);
	GL_Cull(CT_FRONT_SIDED);

	common->Printf(" = %i total\n", count);
//#endif
}

/*
=====================
RB_ShowPortals

Debugging tool, won't work correctly with SMP or when mirrors are present
=====================
*/
#if defined(GL_ES_VERSION_2_0) // from RenderWorld_portal.cpp
static void idRenderWorldLocal__ShowPortals()
{
	int			i, j;
	portalArea_t	*area;
	portal_t	*p;
	idWinding	*w;
	idRenderWorldLocal *renderWorld = (idRenderWorldLocal *)backEnd.viewDef->renderWorld;
	portalArea_t 			*portalAreas = renderWorld->portalAreas;
	int						numPortalAreas = renderWorld->numPortalAreas;

	// flood out through portals, setting area viewCount
	for (i = 0 ; i < numPortalAreas ; i++) {
		area = &portalAreas[i];

		if (area->viewCount != tr.viewCount) {
			continue;
		}

		for (p = area->portals ; p ; p = p->next) {
			w = p->w;

			if (!w) {
				continue;
			}

			if (portalAreas[ p->intoArea ].viewCount != tr.viewCount) {
				// red = can't see
				glColor3f(1, 0, 0);
			} else {
				// green = see through
				glColor3f(0, 1, 0);
			}

			glBegin(GL_LINE_LOOP);

			for (j = 0 ; j < w->GetNumPoints() ; j++) {
				glVertex3fv((*w)[j].ToFloatPtr());
			}

			glEnd();
		}
	}
}
#endif

void RB_ShowPortals(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	if (!r_showPortals.GetBool()) {
		return;
	}

	// all portals are expressed in world coordinates
	RB_SimpleWorldSetup();

	globalImages->BindNull();
	qglDisable(GL_DEPTH_TEST);

	GL_State(GLS_DEFAULT);

#if !defined(GL_ES_VERSION_2_0)
	((idRenderWorldLocal *)backEnd.viewDef->renderWorld)->ShowPortals();
#else
	idRenderWorldLocal__ShowPortals();
#endif

	qglEnable(GL_DEPTH_TEST);
//#endif
}

/*
================
RB_ClearDebugText
================
*/
void RB_ClearDebugText(int time)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i;
	int			num;
	debugText_t	*text;
#ifdef _MULTITHREAD
	TR_DEBUG_TEXTS
#endif

	rb_debugTextTime = time;

	if (!time) {
		// free up our strings
		text = rb_debugText;

		for (i = 0 ; i < MAX_DEBUG_TEXT; i++, text++) {
			text->text.Clear();
		}

		rb_numDebugText = 0;
		return;
	}

	// copy any text that still needs to be drawn
	num	= 0;
	text = rb_debugText;

	for (i = 0 ; i < rb_numDebugText; i++, text++) {
		if (text->lifeTime > time) {
			if (num != i) {
				rb_debugText[ num ] = *text;
			}

			num++;
		}
	}

	rb_numDebugText = num;
//#endif
}

/*
================
RB_AddDebugText
================
*/
void RB_AddDebugText(const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align, const int lifetime, const bool depthTest)
{
//#if !defined(GL_ES_VERSION_2_0)
	debugText_t *debugText;
#ifdef _MULTITHREAD
	TR_DEBUG_TEXTS
#endif

	if (rb_numDebugText < MAX_DEBUG_TEXT) {
		debugText = &rb_debugText[ rb_numDebugText++ ];
		debugText->text			= text;
		debugText->origin		= origin;
		debugText->scale		= scale;
		debugText->color		= color;
		debugText->viewAxis		= viewAxis;
		debugText->align		= align;
		debugText->lifeTime		= rb_debugTextTime + lifetime;
		debugText->depthTest	= depthTest;
	}
//#endif
}

/*
================
RB_DrawTextLength

  returns the length of the given text
================
*/
float RB_DrawTextLength(const char *text, float scale, int len)
{
//#if !defined(GL_ES_VERSION_2_0)
	int i, num, index, charIndex;
	float spacing, textLen = 0.0f;

	if (text && *text) {
		if (!len) {
			len = strlen(text);
		}

		for (i = 0; i < len; i++) {
			charIndex = text[i] - 32;

			if (charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS) {
				continue;
			}

			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while (index - 2 < num) {
				if (simplex[charIndex][index] < 0) {
					index++;
					continue;
				}

				index += 2;

				if (simplex[charIndex][index] < 0) {
					index++;
					continue;
				}
			}

			textLen += spacing * scale;
		}
	}

	return textLen;
//#endif
}

/*
================
RB_DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
static void RB_DrawText(const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align)
{
//#if !defined(GL_ES_VERSION_2_0)
	int i, j, len, num, index, charIndex, line;
	float textLen, spacing;
	idVec3 org, p1, p2;

	if (text && *text) {
		glBegin(GL_LINES);
		glColor3fv(color.ToFloatPtr());

		if (text[0] == '\n') {
			line = 1;
		} else {
			line = 0;
		}

		len = strlen(text);

		for (i = 0; i < len; i++) {

			if (i == 0 || text[i] == '\n') {
				org = origin - viewAxis[2] * (line * 36.0f * scale);

				if (align != 0) {
					for (j = 1; i+j <= len; j++) {
						if (i+j == len || text[i+j] == '\n') {
							textLen = RB_DrawTextLength(text+i, scale, j);
							break;
						}
					}

					if (align == 2) {
						// right
						org += viewAxis[1] * textLen;
					} else {
						// center
						org += viewAxis[1] * (textLen * 0.5f);
					}
				}

				line++;
			}

			charIndex = text[i] - 32;

			if (charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS) {
				continue;
			}

			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while (index - 2 < num) {
				if (simplex[charIndex][index] < 0) {
					index++;
					continue;
				}

				p1 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index+1] * viewAxis[2];
				index += 2;

				if (simplex[charIndex][index] < 0) {
					index++;
					continue;
				}

				p2 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index+1] * viewAxis[2];

				glVertex3fv(p1.ToFloatPtr());
				glVertex3fv(p2.ToFloatPtr());
			}

			org -= viewAxis[1] * (spacing * scale);
		}

		glEnd();
	}
//#endif
}

/*
================
RB_ShowDebugText
================
*/
void RB_ShowDebugText(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i;
	int			width;
	debugText_t	*text;
#ifdef _MULTITHREAD
	RB_DEBUG_TEXTS
#endif

	if (!rb_numDebugText) {
		return;
	}

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	globalImages->BindNull();

	width = r_debugLineWidth.GetInteger();

	if (width < 1) {
		width = 1;
	} else if (width > 10) {
		width = 10;
	}

	// draw lines
	GL_State(GLS_POLYMODE_LINE);
	qglLineWidth(width);

	if (!r_debugLineDepthTest.GetBool()) {
		qglDisable(GL_DEPTH_TEST);
	}

	text = rb_debugText;

	for (i = 0 ; i < rb_numDebugText; i++, text++) {
		if (!text->depthTest) {
			RB_DrawText(text->text, text->origin, text->scale, text->color, text->viewAxis, text->align);
		}
	}

	if (!r_debugLineDepthTest.GetBool()) {
		qglEnable(GL_DEPTH_TEST);
	}

	text = rb_debugText;

	for (i = 0 ; i < rb_numDebugText; i++, text++) {
		if (text->depthTest) {
			RB_DrawText(text->text, text->origin, text->scale, text->color, text->viewAxis, text->align);
		}
	}

	qglLineWidth(1);
	GL_State(GLS_DEFAULT);
//#endif
}

/*
================
RB_ClearDebugLines
================
*/
void RB_ClearDebugLines(int time)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i;
	int			num;
	debugLine_t	*line;
#ifdef _MULTITHREAD
	TR_DEBUG_LINES
#endif

	rb_debugLineTime = time;

	if (!time) {
		rb_numDebugLines = 0;
		return;
	}

	// copy any lines that still need to be drawn
	num	= 0;
	line = rb_debugLines;

	for (i = 0 ; i < rb_numDebugLines; i++, line++) {
		if (line->lifeTime > time) {
			if (num != i) {
				rb_debugLines[ num ] = *line;
			}

			num++;
		}
	}

	rb_numDebugLines = num;
//#endif
}

/*
================
RB_AddDebugLine
================
*/
void RB_AddDebugLine(const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifeTime, const bool depthTest)
{
//#if !defined(GL_ES_VERSION_2_0)
	debugLine_t *line;
#ifdef _MULTITHREAD
	TR_DEBUG_LINES
#endif

	if (rb_numDebugLines < MAX_DEBUG_LINES) {
		line = &rb_debugLines[ rb_numDebugLines++ ];
		line->rgb		= color;
		line->start		= start;
		line->end		= end;
		line->depthTest = depthTest;
		line->lifeTime	= rb_debugLineTime + lifeTime;
	}
//#endif
}

/*
================
RB_ShowDebugLines
================
*/
void RB_ShowDebugLines(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int			i;
	int			width;
	debugLine_t	*line;
#ifdef _MULTITHREAD
	RB_DEBUG_LINES
#endif

	if (!rb_numDebugLines) {
		return;
	}

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	globalImages->BindNull();

	width = r_debugLineWidth.GetInteger();

	if (width < 1) {
		width = 1;
	} else if (width > 10) {
		width = 10;
	}

	// draw lines
	GL_State(GLS_POLYMODE_LINE);  //| GLS_DEPTHMASK ); //| GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglLineWidth(width);

	if (!r_debugLineDepthTest.GetBool()) {
		qglDisable(GL_DEPTH_TEST);
	}

	glBegin(GL_LINES);

	line = rb_debugLines;

	for (i = 0 ; i < rb_numDebugLines; i++, line++) {
		if (!line->depthTest) {
			glColor3fv(line->rgb.ToFloatPtr());
			glVertex3fv(line->start.ToFloatPtr());
			glVertex3fv(line->end.ToFloatPtr());
		}
	}

	glEnd();

	if (!r_debugLineDepthTest.GetBool()) {
		qglEnable(GL_DEPTH_TEST);
	}

	glBegin(GL_LINES);

	line = rb_debugLines;

	for (i = 0 ; i < rb_numDebugLines; i++, line++) {
		if (line->depthTest) {
			glColor4fv(line->rgb.ToFloatPtr());
			glVertex3fv(line->start.ToFloatPtr());
			glVertex3fv(line->end.ToFloatPtr());
		}
	}

	glEnd();

	qglLineWidth(1);
	GL_State(GLS_DEFAULT);
//#endif
}

/*
================
RB_ClearDebugPolygons
================
*/
void RB_ClearDebugPolygons(int time)
{
//#if !defined(GL_ES_VERSION_2_0)
	int				i;
	int				num;
	debugPolygon_t	*poly;
#ifdef _MULTITHREAD
	TR_DEBUG_POLYGONS
#endif

	rb_debugPolygonTime = time;

	if (!time) {
		rb_numDebugPolygons = 0;
		return;
	}

	// copy any polygons that still need to be drawn
	num	= 0;

	poly = rb_debugPolygons;

	for (i = 0 ; i < rb_numDebugPolygons; i++, poly++) {
		if (poly->lifeTime > time) {
			if (num != i) {
				rb_debugPolygons[ num ] = *poly;
			}

			num++;
		}
	}

	rb_numDebugPolygons = num;
//#endif
}

/*
================
RB_AddDebugPolygon
================
*/
void RB_AddDebugPolygon(const idVec4 &color, const idWinding &winding, const int lifeTime, const bool depthTest)
{
//#if !defined(GL_ES_VERSION_2_0)
	debugPolygon_t *poly;
#ifdef _MULTITHREAD
	TR_DEBUG_POLYGONS
#endif

	if (rb_numDebugPolygons < MAX_DEBUG_POLYGONS) {
		poly = &rb_debugPolygons[ rb_numDebugPolygons++ ];
		poly->rgb		= color;
		poly->winding	= winding;
		poly->depthTest = depthTest;
		poly->lifeTime	= rb_debugPolygonTime + lifeTime;
	}
//#endif
}

/*
================
RB_ShowDebugPolygons
================
*/
void RB_ShowDebugPolygons(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	int				i, j;
	debugPolygon_t	*poly;
#ifdef _MULTITHREAD
	RB_DEBUG_POLYGONS
#endif

	if (!rb_numDebugPolygons) {
		return;
	}

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	globalImages->BindNull();

	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_STENCIL_TEST);

	qglEnable(GL_DEPTH_TEST);

	if (r_debugPolygonFilled.GetBool()) {
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK);
		qglPolygonOffset(-1, -2);
		qglEnable(GL_POLYGON_OFFSET_FILL);
	} else {
		GL_State(GLS_POLYMODE_LINE);
		qglPolygonOffset(-1, -2);
#if !defined(GL_ES_VERSION_2_0)
		qglEnable(GL_POLYGON_OFFSET_LINE);
#endif
	}

	poly = rb_debugPolygons;

	for (i = 0 ; i < rb_numDebugPolygons; i++, poly++) {
//		if ( !poly->depthTest ) {

		glColor4fv(poly->rgb.ToFloatPtr());

		glBegin(GL_POLYGON);

		for (j = 0; j < poly->winding.GetNumPoints(); j++) {
			glVertex3fv(poly->winding[j].ToFloatPtr());
		}

		glEnd();
//		}
	}

	GL_State(GLS_DEFAULT);

	if (r_debugPolygonFilled.GetBool()) {
		qglDisable(GL_POLYGON_OFFSET_FILL);
	} else {
#if !defined(GL_ES_VERSION_2_0)
		qglDisable(GL_POLYGON_OFFSET_LINE);
#endif
	}

	glDepthRange(0, 1);
	GL_State(GLS_DEFAULT);
//#endif
}

/*
================
RB_TestGamma
================
*/
#define	G_WIDTH		512
#define	G_HEIGHT	512
#define	BAR_HEIGHT	64

void RB_TestGamma(void)
{
#if !defined(GL_ES_VERSION_2_0)
	byte	image[G_HEIGHT][G_WIDTH][4];
	int		i, j;
	int		c, comp;
	int		v, dither;
	int		mask, y;

	if (r_testGamma.GetInteger() <= 0) {
		return;
	}

	v = r_testGamma.GetInteger();

	if (v <= 1 || v >= 196) {
		v = 128;
	}

	memset(image, 0, sizeof(image));

	for (mask = 0 ; mask < 8 ; mask++) {
		y = mask * BAR_HEIGHT;

		for (c = 0 ; c < 4 ; c++) {
			v = c * 64 + 32;

			// solid color
			for (i = 0 ; i < BAR_HEIGHT/2 ; i++) {
				for (j = 0 ; j < G_WIDTH/4 ; j++) {
					for (comp = 0 ; comp < 3 ; comp++) {
						if (mask & (1 << comp)) {
							image[y+i][c *G_WIDTH/4+j][comp] = v;
						}
					}
				}

				// dithered color
				for (j = 0 ; j < G_WIDTH/4 ; j++) {
					if ((i ^ j) & 1) {
						dither = c * 64;
					} else {
						dither = c * 64 + 63;
					}

					for (comp = 0 ; comp < 3 ; comp++) {
						if (mask & (1 << comp)) {
							image[y+BAR_HEIGHT/2+i][c *G_WIDTH/4+j][comp] = dither;
						}
					}
				}
			}
		}
	}

	// draw geometrically increasing steps in the bottom row
	y = 0 * BAR_HEIGHT;
	float	scale = 1;

	for (c = 0 ; c < 4 ; c++) {
		v = (int)(64 * scale);

		if (v < 0) {
			v = 0;
		} else if (v > 255) {
			v = 255;
		}

		scale = scale * 1.5;

		for (i = 0 ; i < BAR_HEIGHT ; i++) {
			for (j = 0 ; j < G_WIDTH/4 ; j++) {
				image[y+i][c *G_WIDTH/4+j][0] = v;
				image[y+i][c *G_WIDTH/4+j][1] = v;
				image[y+i][c *G_WIDTH/4+j][2] = v;
			}
		}
	}


	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	GL_State(GLS_DEPTHFUNC_ALWAYS);
	glColor3f(1, 1, 1);
	glPushMatrix();
	glLoadIdentity();
	qglDisable(GL_TEXTURE_2D);
	glOrtho(0, 1, 0, 1, -1, 1);
	glRasterPos2f(0.01f, 0.01f);
	glDrawPixels(G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glPopMatrix();
	qglEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
#endif
}


/*
==================
RB_TestGammaBias
==================
*/
static void RB_TestGammaBias(void)
{
#if !defined(GL_ES_VERSION_2_0)
	byte	image[G_HEIGHT][G_WIDTH][4];

	if (r_testGammaBias.GetInteger() <= 0) {
		return;
	}

	int y = 0;

	for (int bias = -40 ; bias < 40 ; bias+=10, y += BAR_HEIGHT) {
		float	scale = 1;

		for (int c = 0 ; c < 4 ; c++) {
			int v = (int)(64 * scale + bias);
			scale = scale * 1.5;

			if (v < 0) {
				v = 0;
			} else if (v > 255) {
				v = 255;
			}

			for (int i = 0 ; i < BAR_HEIGHT ; i++) {
				for (int j = 0 ; j < G_WIDTH/4 ; j++) {
					image[y+i][c *G_WIDTH/4+j][0] = v;
					image[y+i][c *G_WIDTH/4+j][1] = v;
					image[y+i][c *G_WIDTH/4+j][2] = v;
				}
			}
		}
	}


	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	GL_State(GLS_DEPTHFUNC_ALWAYS);
	glColor3f(1, 1, 1);
	glPushMatrix();
	glLoadIdentity();
	qglDisable(GL_TEXTURE_2D);
	glOrtho(0, 1, 0, 1, -1, 1);
	glRasterPos2f(0.01f, 0.01f);
	glDrawPixels(G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glPopMatrix();
	qglEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
#endif
}

/*
================
RB_TestImage

Display a single image over most of the screen
================
*/
void RB_TestImage(void)
{
//#if !defined(GL_ES_VERSION_2_0)
	idImage	*image;
	int		max;
	float	w, h;

	image = tr.testImage;

	if (!image) {
		return;
	}

	if (tr.testVideo) {
		cinData_t	cin;

		cin = tr.testVideo->ImageForTime((int)(1000 * (backEnd.viewDef->floatTime - tr.testVideoStartTime)));

		if (cin.image) {
			image->UploadScratch(cin.image, cin.imageWidth, cin.imageHeight);
		} else {
			tr.testImage = NULL;
			return;
		}

		w = 0.25;
		h = 0.25;
	} else {
		// common->Printf("testImage::image size %d x %d\n", image->uploadWidth,image->uploadHeight);
		max = image->uploadWidth > image->uploadHeight ? image->uploadWidth : image->uploadHeight;

		w = 0.25 * image->uploadWidth / max;
		h = 0.25 * image->uploadHeight / max;

		w *= (float)glConfig.vidHeight / glConfig.vidWidth;
	}

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	GL_State(GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	glColor3f(1, 1, 1);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

#ifdef GL_ES_VERSION_2_0
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
/*	GLboolean t2d = qglIsEnabled(GL_TEXTURE_2D);
	if(!t2d)
		qglEnable(GL_TEXTURE_2D);*/
	GLboolean cf = qglIsEnabled(GL_CULL_FACE);
	if(cf)
		qglDisable(GL_CULL_FACE);
#endif
	tr.testImage->Bind();
	glBegin(GL_QUADS);

	glTexCoord2f(0, 1);
	glVertex2f(0.5 - w, 0);

	glTexCoord2f(0, 0);
	glVertex2f(0.5 - w, h*2);

	glTexCoord2f(1, 0);
	glVertex2f(0.5 + w, h*2);

	glTexCoord2f(1, 1);
	glVertex2f(0.5 + w, 0);

	glEnd();
#ifdef GL_ES_VERSION_2_0
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
/*	if(!t2d)
		qglDisable(GL_TEXTURE_2D);*/
	if(cf)
		qglEnable(GL_CULL_FACE);
#endif

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
//#endif
}

/*
=================
RB_RenderDebugTools
=================
*/
void RB_RenderDebugTools(drawSurf_t **drawSurfs, int numDrawSurfs)
{
#ifdef _MULTITHREAD
	if(multithreadActive && !harm_r_renderToolsMultithread.GetBool())
		return;
#endif
	// don't do anything if this was a 2D rendering
	if (!backEnd.viewDef->viewEntitys) {
		return;
	}

	RB_LogComment("---------- RB_RenderDebugTools ----------\n");
	GL_State(GLS_DEFAULT);
	backEnd.currentScissor = backEnd.viewDef->scissor;
	qglScissor(backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
	           backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
	           backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
	           backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1);


	RB_ShowLightCount();
	RB_ShowShadowCount();
	RB_ShowTexturePolarity(drawSurfs, numDrawSurfs);
	RB_ShowTangentSpace(drawSurfs, numDrawSurfs);
	RB_ShowVertexColor(drawSurfs, numDrawSurfs);
	RB_ShowTris(drawSurfs, numDrawSurfs);
	RB_ShowUnsmoothedTangents(drawSurfs, numDrawSurfs);
	RB_ShowSurfaceInfo(drawSurfs, numDrawSurfs);
	RB_ShowEdges(drawSurfs, numDrawSurfs);
	RB_ShowNormals(drawSurfs, numDrawSurfs);
	RB_ShowViewEntitys(backEnd.viewDef->viewEntitys);
	RB_ShowLights();
	RB_ShowTextureVectors(drawSurfs, numDrawSurfs);
	RB_ShowDominantTris(drawSurfs, numDrawSurfs);

	if (r_testGamma.GetInteger() > 0) {	// test here so stack check isn't so damn slow on debug builds
		RB_TestGamma();
	}

	if (r_testGammaBias.GetInteger() > 0) {
		RB_TestGammaBias();
	}

	RB_TestImage();
	RB_ShowPortals();
	RB_ShowSilhouette();
	RB_ShowDepthBuffer();
	RB_ShowIntensity();
	RB_ShowDebugLines();
	RB_ShowDebugText();
	RB_ShowDebugPolygons();
	RB_ShowTrace(drawSurfs, numDrawSurfs);
#ifdef GL_ES_VERSION_2_0
	DEBUG_RENDER_COMPAT
#endif
}

/*
=================
RB_ShutdownDebugTools
=================
*/
void RB_ShutdownDebugTools(void)
{
//#if !defined(GL_ES_VERSION_2_0)
#ifdef _MULTITHREAD
	const int num = multithreadActive ? NUM_FRAME_DATA : 1;
	for(int i = 0; i < num; i++)
	{
		debugPolygon_t *rb_debugPolygons = rb_debugPolygonss[i];
#endif
	for (int i = 0; i < MAX_DEBUG_POLYGONS; i++) {
		rb_debugPolygons[i].winding.Clear();
	}
#ifdef _MULTITHREAD
	}
#endif
//#endif

    glrbShutdown();
}

#ifdef _MULTITHREAD
void R_ShowSurfaceInfo(void)
{
	if(!r_showSurfaceInfo.GetBool() || !harm_r_renderToolsMultithread.GetBool() || !multithreadActive)
		return;

	modelTrace_t mt;
	idVec3 start, end;

	// start far enough away that we don't hit the player model
	start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;

	if (!tr.primaryWorld->Trace(mt, start, end, 0.0f, false)) {
		return;
	}

	idVec3	trans[3];
	float	matrix[16];

	// transform the object verts into global space
	R_AxisToModelMatrix(mt.entity->axis, mt.entity->origin, matrix);

	tr.primaryWorld->DrawText(mt.entity->hModel->Name(), mt.point + tr.primaryView->renderView.viewaxis[2] * 12,
	                          0.35f, colorRed, tr.primaryView->renderView.viewaxis);
	tr.primaryWorld->DrawText(mt.material->GetName(), mt.point,
	                          0.35f, colorBlue, tr.primaryView->renderView.viewaxis);
#ifdef _RAVEN //karin: show materialType
	if(mt.materialType)
	{
		tr.primaryWorld->DrawText(mt.materialType->GetName(), mt.point - tr.primaryView->renderView.viewaxis[2] * 12,
				0.35f, colorGreen, tr.primaryView->renderView.viewaxis);
	}
#endif
#ifdef _K_DEV //karin: show stage texgen
	idStr str;
	const char *TG_NAMES[] = {
		"TG_EXPLICIT",
		"TG_DIFFUSE_CUBE",
		"TG_REFLECT_CUBE",
		"TG_SKYBOX_CUBE",
		"TG_WOBBLESKY_CUBE",
		"TG_SCREEN",
		"TG_SCREEN2",
		"TG_GLASSWARP",
	};
	for(int i = 0; i < mt.material->GetNumStages(); i++)
		str += va("%d. %s\n", i, TG_NAMES[mt.material->GetStage(i)->texture.texgen]);
	tr.primaryWorld->DrawText(str.c_str(), mt.point - tr.primaryView->renderView.viewaxis[2] * 24,
	                          0.35f, colorRed, tr.primaryView->renderView.viewaxis);
#endif
}

#endif

/*
=================
RB_DrawElementsImmediate

Draws with immediate mode commands, which is going to be very slow.
This should never happen if the vertex cache is operating properly.
=================
*/
void RB_DrawElementsImmediate( const srfTriangles_t *tri ) {

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += tri->numIndexes;
	backEnd.pc.c_drawVertexes += tri->numVerts;

	if ( tri->ambientSurface != NULL  ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.c_drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.c_drawRefVertexes += tri->numVerts;
		}
	}

	glBegin( GL_TRIANGLES );
	for ( int i = 0 ; i < tri->numIndexes ; i++ ) {
		glTexCoord2fv( tri->verts[ tri->indexes[i] ].st.ToFloatPtr() );
		glVertex3fv( tri->verts[ tri->indexes[i] ].xyz.ToFloatPtr() );
	}
	glEnd();
}
