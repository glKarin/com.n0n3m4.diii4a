/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "quakedef.h"
#include "cl_collision.h"

#ifdef MAX_EXPLOSIONS
#define EXPLOSIONGRID 8
#define EXPLOSIONVERTS ((EXPLOSIONGRID+1)*(EXPLOSIONGRID+1))
#define EXPLOSIONTRIS (EXPLOSIONGRID*EXPLOSIONGRID*2)

static int numexplosions = 0;

static float explosiontexcoord2f[EXPLOSIONVERTS][2];
static unsigned short explosiontris[EXPLOSIONTRIS][3];
static int explosionnoiseindex[EXPLOSIONVERTS];
static vec3_t explosionpoint[EXPLOSIONVERTS];

typedef struct explosion_s
{
	float starttime;
	float endtime;
	float time;
	float alpha;
	float fade;
	vec3_t origin;
	vec3_t vert[EXPLOSIONVERTS];
	vec3_t vertvel[EXPLOSIONVERTS];
	qbool clipping;
}
explosion_t;

static explosion_t explosion[MAX_EXPLOSIONS];

static rtexture_t	*explosiontexture;
//static rtexture_t	*explosiontexturefog;

static rtexturepool_t	*explosiontexturepool;
#endif

cvar_t r_explosionclip = {CF_CLIENT | CF_ARCHIVE, "r_explosionclip", "1", "enables collision detection for explosion shell (so that it flattens against walls and floors)"};
#ifdef MAX_EXPLOSIONS
static cvar_t r_drawexplosions = {CF_CLIENT, "r_drawexplosions", "1", "enables rendering of explosion shells (see also cl_particles_explosions_shell)"};

//extern qbool r_loadfog;
static void r_explosion_start(void)
{
	int x, y;
	static unsigned char noise1[128][128], noise2[128][128], noise3[128][128], data[128][128][4];
	explosiontexturepool = R_AllocTexturePool();
	explosiontexture = NULL;
	//explosiontexturefog = NULL;
	fractalnoise(&noise1[0][0], 128, 32);
	fractalnoise(&noise2[0][0], 128, 4);
	fractalnoise(&noise3[0][0], 128, 4);
	for (y = 0;y < 128;y++)
	{
		for (x = 0;x < 128;x++)
		{
			int j, r, g, b, a;
			j = (noise1[y][x] * noise2[y][x]) * 3 / 256 - 128;
			r = (j * 512) / 256;
			g = (j * 256) / 256;
			b = (j * 128) / 256;
			a = noise3[y][x] * 3 - 128;
			data[y][x][2] = bound(0, r, 255);
			data[y][x][1] = bound(0, g, 255);
			data[y][x][0] = bound(0, b, 255);
			data[y][x][3] = bound(0, a, 255);
		}
	}
	explosiontexture = R_LoadTexture2D(explosiontexturepool, "explosiontexture", 128, 128, &data[0][0][0], TEXTYPE_BGRA, TEXF_MIPMAP | TEXF_ALPHA | TEXF_FORCELINEAR, -1, NULL);
//	if (r_loadfog)
//	{
//		for (y = 0;y < 128;y++)
//			for (x = 0;x < 128;x++)
//				data[y][x][0] = data[y][x][1] = data[y][x][2] = 255;
//		explosiontexturefog = R_LoadTexture2D(explosiontexturepool, "explosiontexture_fog", 128, 128, &data[0][0][0], TEXTYPE_BGRA, TEXF_MIPMAP | TEXF_ALPHA | TEXF_FORCELINEAR, NULL);
//	}
	// note that explosions survive the restart
}

static void r_explosion_shutdown(void)
{
	R_FreeTexturePool(&explosiontexturepool);
}

static void r_explosion_newmap(void)
{
	numexplosions = 0;
	memset(explosion, 0, sizeof(explosion));
}

static int R_ExplosionVert(int column, int row)
{
	int i;
	float yaw, pitch;
	// top and bottom rows are all one position...
	if (row == 0 || row == EXPLOSIONGRID)
		column = 0;
	i = row * (EXPLOSIONGRID + 1) + column;
	yaw = ((double) column / EXPLOSIONGRID) * M_PI * 2;
	pitch = (((double) row / EXPLOSIONGRID) - 0.5) * M_PI;
	explosionpoint[i][0] = cos(yaw) *  cos(pitch);
	explosionpoint[i][1] = sin(yaw) *  cos(pitch);
	explosionpoint[i][2] =        1 * -sin(pitch);
	explosiontexcoord2f[i][0] = (float) column / (float) EXPLOSIONGRID;
	explosiontexcoord2f[i][1] = (float) row / (float) EXPLOSIONGRID;
	explosionnoiseindex[i] = (row % EXPLOSIONGRID) * EXPLOSIONGRID + (column % EXPLOSIONGRID);
	return i;
}
#endif

void R_Explosion_Init(void)
{
#ifdef MAX_EXPLOSIONS
	int i, x, y;
	i = 0;
	for (y = 0;y < EXPLOSIONGRID;y++)
	{
		for (x = 0;x < EXPLOSIONGRID;x++)
		{
			explosiontris[i][0] = R_ExplosionVert(x    , y    );
			explosiontris[i][1] = R_ExplosionVert(x + 1, y    );
			explosiontris[i][2] = R_ExplosionVert(x    , y + 1);
			i++;
			explosiontris[i][0] = R_ExplosionVert(x + 1, y    );
			explosiontris[i][1] = R_ExplosionVert(x + 1, y + 1);
			explosiontris[i][2] = R_ExplosionVert(x    , y + 1);
			i++;
		}
	}

#endif
	Cvar_RegisterVariable(&r_explosionclip);
#ifdef MAX_EXPLOSIONS
	Cvar_RegisterVariable(&r_drawexplosions);

	R_RegisterModule("R_Explosions", r_explosion_start, r_explosion_shutdown, r_explosion_newmap, NULL, NULL);
#endif
}

void R_NewExplosion(const vec3_t org)
{
#ifdef MAX_EXPLOSIONS
	int i, j;
	float dist, n;
	explosion_t *e;
	trace_t trace;
	unsigned char noise[EXPLOSIONGRID*EXPLOSIONGRID];
	fractalnoisequick(noise, EXPLOSIONGRID, 4); // adjust noise grid size according to explosion
	for (i = 0, e = explosion;i < MAX_EXPLOSIONS;i++, e++)
	{
		if (!e->alpha)
		{
			numexplosions = max(numexplosions, i + 1);
			e->starttime = cl.time;
			e->endtime = cl.time + cl_explosions_lifetime.value;
			e->time = e->starttime;
			e->alpha = cl_explosions_alpha_start.value;
			e->fade = (cl_explosions_alpha_start.value - cl_explosions_alpha_end.value) / cl_explosions_lifetime.value;
			e->clipping = r_explosionclip.integer != 0;
			VectorCopy(org, e->origin);
			for (j = 0;j < EXPLOSIONVERTS;j++)
			{
				// calculate start origin and velocity
				n = noise[explosionnoiseindex[j]] * (1.0f / 255.0f) + 0.5;
				dist = n * cl_explosions_size_start.value;
				VectorMA(e->origin, dist, explosionpoint[j], e->vert[j]);
				dist = n * (cl_explosions_size_end.value - cl_explosions_size_start.value) / cl_explosions_lifetime.value;
				VectorScale(explosionpoint[j], dist, e->vertvel[j]);
				// clip start origin
				if (e->clipping)
				{
					trace = CL_TraceLine(e->origin, e->vert[j], MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value, true, false, NULL, false, false);
					VectorCopy(trace.endpos, e->vert[i]);
				}
			}
			break;
		}
	}
#endif
}

#ifdef MAX_EXPLOSIONS
static void R_DrawExplosion_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int surfacelistindex = 0;
	const int numtriangles = EXPLOSIONTRIS, numverts = EXPLOSIONVERTS;
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	GL_DepthMask(false);
	GL_DepthRange(0, 1);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);
	GL_DepthTest(true);
	GL_CullFace(r_refdef.view.cullface_back);
	R_EntityMatrix(&identitymatrix);

//	R_Mesh_ResetTextureState();
	R_SetupShader_Generic(explosiontexture, false, false, false);
	for (surfacelistindex = 0;surfacelistindex < numsurfaces;surfacelistindex++)
	{
		const explosion_t *e = explosion + surfacelist[surfacelistindex];
		// FIXME: this can't properly handle r_refdef.view.colorscale > 1
		GL_Color(e->alpha * r_refdef.view.colorscale, e->alpha * r_refdef.view.colorscale, e->alpha * r_refdef.view.colorscale, 1);
		R_Mesh_PrepareVertices_Generic_Arrays(numverts, e->vert[0], NULL, explosiontexcoord2f[0]);
		R_Mesh_Draw(0, numverts, 0, numtriangles, NULL, NULL, 0, explosiontris[0], NULL, 0);
	}
}

static void R_MoveExplosion(explosion_t *e)
{
	int i;
	float dot, end[3], frametime;
	trace_t trace;

	frametime = cl.time - e->time;
	e->time = cl.time;
	e->alpha = e->alpha - (e->fade * frametime);
	if (e->alpha < 0 || cl.time > e->endtime)
	{
		e->alpha = 0;
		return;
	}
	for (i = 0;i < EXPLOSIONVERTS;i++)
	{
		if (e->vertvel[i][0] || e->vertvel[i][1] || e->vertvel[i][2])
		{
			VectorMA(e->vert[i], frametime, e->vertvel[i], end);
			if (e->clipping)
			{
				trace = CL_TraceLine(e->vert[i], end, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value, true, false, NULL, false, false);
				if (trace.fraction < 1)
				{
					// clip velocity against the wall
					dot = -DotProduct(e->vertvel[i], trace.plane.normal);
					VectorMA(e->vertvel[i], dot, trace.plane.normal, e->vertvel[i]);
				}
				VectorCopy(trace.endpos, e->vert[i]);
			}
			else
				VectorCopy(end, e->vert[i]);
		}
	}
}
#endif

void R_DrawExplosions(void)
{
#ifdef MAX_EXPLOSIONS
	int i;

	if (!r_drawexplosions.integer)
		return;

	for (i = 0;i < numexplosions;i++)
	{
		if (explosion[i].alpha)
		{
			R_MoveExplosion(&explosion[i]);
			if (explosion[i].alpha)
				R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, explosion[i].origin, R_DrawExplosion_TransparentCallback, NULL, i, NULL);
		}
	}
	while (numexplosions > 0 && explosion[i-1].alpha <= 0)
		numexplosions--;
#endif
}

