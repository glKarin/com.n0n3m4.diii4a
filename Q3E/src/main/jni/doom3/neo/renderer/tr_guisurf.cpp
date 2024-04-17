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

/*
==========================================================================================

GUI SHADERS

==========================================================================================
*/

#ifdef _RAVEN
/*
================
R_CalcGuiRange

Calculates gui surface range in window coordinates
================
*/
void R_CalcGuiRangeInWindow(idUserInterface *gui, const srfTriangles_t *tri, const float defModelMatrix[16])
{
	if (tri->silEdges && tri->verts && tr.primaryView) {
		idScreenRect	r;
		idVec3			v;
		idVec3			ndc;
		float			windowX, windowY;
		const int viewportWidth = SCREEN_WIDTH;
		const int viewportHeight = SCREEN_HEIGHT;

		r.Clear();
		idDrawVert *ac = (idDrawVert *)tri->verts;
		int danglePlane = tri->numIndexes / 3;
		for (int j = 0 ; j < tri->numSilEdges ; j++) {
			const silEdge_t			*edge = tri->silEdges + j;
			if (edge->p1 != danglePlane && edge->p2 != danglePlane) {
				continue;
			}

			float *ptr = ac[ edge->v1 ].xyz.ToFloatPtr();
			R_LocalPointToGlobal(defModelMatrix, idVec3(ptr[0], ptr[1], ptr[2]), v);
			R_GlobalToNormalizedDeviceCoordinates(v, ndc);

			windowX = 0.5f * (1.0f + ndc[0]) * viewportWidth;
			windowY = 0.5f * (1.0f + ndc[1]) * viewportHeight;

			r.AddPoint(windowX, windowY);

			ptr = ac[ edge->v2 ].xyz.ToFloatPtr();
			R_LocalPointToGlobal(defModelMatrix, idVec3(ptr[0], ptr[1], ptr[2]), v);
			R_GlobalToNormalizedDeviceCoordinates(v, ndc);

			windowX = 0.5f * (1.0f + ndc[0]) * viewportWidth;
			windowY = 0.5f * (1.0f + ndc[1]) * viewportHeight;

			r.AddPoint(windowX, windowY);
		}

		r.Expand();

		gui->SetStateInt("2d_min_x", r.x1);
		gui->SetStateInt("2d_min_y", viewportHeight - r.y2);
		gui->SetStateInt("2d_max_x", r.x2);
		gui->SetStateInt("2d_max_y", viewportHeight - r.y1);
	}
}
#endif
/*
================
R_SurfaceToTextureAxis

Calculates two axis for the surface sutch that a point dotted against
the axis will give a 0.0 to 1.0 range in S and T when inside the gui surface
================
*/
void R_SurfaceToTextureAxis(const srfTriangles_t *tri, idVec3 &origin, idVec3 axis[3])
{
	float		area, inva;
	float		d0[5], d1[5];
	idDrawVert	*a, *b, *c;
	float		bounds[2][2];
	float		boundsOrg[2];
	int			i, j;
	float		v;

	// find the bounds of the texture
	bounds[0][0] = bounds[0][1] = 999999;
	bounds[1][0] = bounds[1][1] = -999999;

	for (i = 0 ; i < tri->numVerts ; i++) {
		for (j = 0 ; j < 2 ; j++) {
			v = tri->verts[i].st[j];

			if (v < bounds[0][j]) {
				bounds[0][j] = v;
			}

			if (v > bounds[1][j]) {
				bounds[1][j] = v;
			}
		}
	}

	// use the floor of the midpoint as the origin of the
	// surface, which will prevent a slight misalignment
	// from throwing it an entire cycle off
	boundsOrg[0] = floor((bounds[0][0] + bounds[1][0]) * 0.5);
	boundsOrg[1] = floor((bounds[0][1] + bounds[1][1]) * 0.5);


	// determine the world S and T vectors from the first drawSurf triangle
	a = tri->verts + tri->indexes[0];
	b = tri->verts + tri->indexes[1];
	c = tri->verts + tri->indexes[2];

	VectorSubtract(b->xyz, a->xyz, d0);
	d0[3] = b->st[0] - a->st[0];
	d0[4] = b->st[1] - a->st[1];
	VectorSubtract(c->xyz, a->xyz, d1);
	d1[3] = c->st[0] - a->st[0];
	d1[4] = c->st[1] - a->st[1];

	area = d0[3] * d1[4] - d0[4] * d1[3];

	if (area == 0.0) {
		axis[0].Zero();
		axis[1].Zero();
		axis[2].Zero();
		return;	// degenerate
	}

	inva = 1.0 / area;

	axis[0][0] = (d0[0] * d1[4] - d0[4] * d1[0]) * inva;
	axis[0][1] = (d0[1] * d1[4] - d0[4] * d1[1]) * inva;
	axis[0][2] = (d0[2] * d1[4] - d0[4] * d1[2]) * inva;

	axis[1][0] = (d0[3] * d1[0] - d0[0] * d1[3]) * inva;
	axis[1][1] = (d0[3] * d1[1] - d0[1] * d1[3]) * inva;
	axis[1][2] = (d0[3] * d1[2] - d0[2] * d1[3]) * inva;

	idPlane plane;
	plane.FromPoints(a->xyz, b->xyz, c->xyz);
	axis[2][0] = plane[0];
	axis[2][1] = plane[1];
	axis[2][2] = plane[2];

	// take point 0 and project the vectors to the texture origin
	VectorMA(a->xyz, boundsOrg[0] - a->st[0], axis[0], origin);
	VectorMA(origin, boundsOrg[1] - a->st[1], axis[1], origin);
}

/*
=================
R_RenderGuiSurf

Create a texture space on the given surface and
call the GUI generator to create quads for it.
=================
*/
#ifdef _HUMANHEAD //karin: auto translate alien text
const char	*harm_ui_translateAlienFontArgs[]	= {
	"fonts",
	"fonts/menu",
	NULL };
static idCVar harm_ui_translateAlienFont( "harm_ui_translateAlienFont", harm_ui_translateAlienFontArgs[0], CVAR_GUI | CVAR_ARCHIVE, "[Harmattan]: Setup font name for automitic translate `alien` font text of GUI(empty to disable).", idCmdSystem::ArgCompletion_String<harm_ui_translateAlienFontArgs> );
static idCVar harm_ui_translateAlienFontDistance( "harm_ui_translateAlienFontDistance", "200", CVAR_GUI | CVAR_FLOAT | CVAR_ARCHIVE, "[Harmattan]: Setup max distance of GUI to view origin for enable translate `alien` font text(0 to disable, -1 to always)." ); //karin: initial distance see in prey/Prey/game_player.cpp::UpdateFocus is 70(for interactive GUI)
#endif
void R_RenderGuiSurf(idUserInterface *gui, drawSurf_t *drawSurf)
{
	idVec3	origin, axis[3];

	// for testing the performance hit
	if (r_skipGuiShaders.GetInteger() == 1) {
		return;
	}

	// don't allow an infinite recursion loop
	if (tr.guiRecursionLevel == 4) {
		return;
	}

	tr.pc.c_guiSurfs++;

	// create the new matrix to draw on this surface
	R_SurfaceToTextureAxis(drawSurf->geo, origin, axis);

	float	guiModelMatrix[16];
	float	modelMatrix[16];

	guiModelMatrix[0] = axis[0][0] / 640.0;
	guiModelMatrix[4] = axis[1][0] / 480.0;
	guiModelMatrix[8] = axis[2][0];
	guiModelMatrix[12] = origin[0];

	guiModelMatrix[1] = axis[0][1] / 640.0;
	guiModelMatrix[5] = axis[1][1] / 480.0;
	guiModelMatrix[9] = axis[2][1];
	guiModelMatrix[13] = origin[1];

	guiModelMatrix[2] = axis[0][2] / 640.0;
	guiModelMatrix[6] = axis[1][2] / 480.0;
	guiModelMatrix[10] = axis[2][2];
	guiModelMatrix[14] = origin[2];

	guiModelMatrix[3] = 0;
	guiModelMatrix[7] = 0;
	guiModelMatrix[11] = 0;
	guiModelMatrix[15] = 1;

	myGlMultMatrix(guiModelMatrix, drawSurf->space->modelMatrix,
	               modelMatrix);

	tr.guiRecursionLevel++;

#ifdef _RAVEN //karin: player focus GUI with brackets
	if(gui->State().GetBool( "2d_calc" )) // mark in idPlayer::UpdateFocus
	{
		if(gui->State().GetBool( "harm_2d_calc" )) // mark in idRenderWorldLocal::GuiTrace
		{
			const srfTriangles_t	*tri;
			const float *defModelMatrix = drawSurf->space->modelMatrix;

			tri = drawSurf->geo; //tri = drawSurf->geoOrig;

			R_CalcGuiRangeInWindow(gui, tri, defModelMatrix);
			// reset mark
			gui->SetStateBool ( "harm_2d_calc", false );
			// In game code, setup false for idPlayer::DrawHUD will render focus brackets
			gui->SetStateBool ( "2d_calc", false );
		}
	}
#endif

#ifdef _HUMANHEAD //karin: auto translate alien text
	//if(tr.primaryRenderView.viewID) // it's 0 when pm_thirdPersion = 1
	{
		const char *translateAlienFont = harm_ui_translateAlienFont.GetString();
		if ( translateAlienFont && translateAlienFont[0] && harm_ui_translateAlienFontDistance.GetFloat() != 0.0f ) {
			if(harm_ui_translateAlienFontDistance.GetFloat() < 0.0f)
				gui->Translate(translateAlienFont);
			else // > 0
			{
				if((tr.primaryRenderView.vieworg - idVec3(/*drawSurf->space->*/modelMatrix[12], /*drawSurf->space->*/modelMatrix[13], /*drawSurf->space->*/modelMatrix[14])).LengthFast() <= harm_ui_translateAlienFontDistance.GetFloat())
					gui->Translate(translateAlienFont);
			}
		}
	}
#endif
	// call the gui, which will call the 2D drawing functions
	tr.guiModel->Clear();
	gui->Redraw(tr.viewDef->renderView.time);
	tr.guiModel->EmitToCurrentView(modelMatrix, drawSurf->space->weaponDepthHack);
	tr.guiModel->Clear();

	tr.guiRecursionLevel--;
}




/*
================,
R_ReloadGuis_f

Reloads any guis that have had their file timestamps changed.
An optional "all" parameter will cause all models to reload, even
if they are not out of date.

Should we also reload the map models?
================
*/
void R_ReloadGuis_f(const idCmdArgs &args)
{
	bool all;

	if (!idStr::Icmp(args.Argv(1), "all")) {
		all = true;
		common->Printf("Reloading all gui files...\n");
	} else {
		all = false;
		common->Printf("Checking for changed gui files...\n");
	}

	uiManager->Reload(all);
}

/*
================,
R_ListGuis_f

================
*/
void R_ListGuis_f(const idCmdArgs &args)
{
	uiManager->ListGuis();
}
