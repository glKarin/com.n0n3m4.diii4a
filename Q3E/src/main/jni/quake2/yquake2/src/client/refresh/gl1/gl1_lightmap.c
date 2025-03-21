/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Lightmap handling
 *
 * =======================================================================
 */

#include "header/local.h"

extern gllightmapstate_t gl_lms;

void R_SetCacheState(msurface_t *surf);
void R_BuildLightMap(msurface_t *surf, byte *dest, int stride);

void
LM_FreeLightmapBuffers(void)
{
	for (int i=0; i<MAX_LIGHTMAPS; i++)
	{
		if (gl_lms.lightmap_buffer[i])
		{
			free(gl_lms.lightmap_buffer[i]);
		}
		gl_lms.lightmap_buffer[i] = NULL;
	}

	if (gl_lms.allocated)
	{
		free(gl_lms.allocated);
		gl_lms.allocated = NULL;
	}
}

static void
LM_AllocLightmapBuffer(int buffer, qboolean clean)
{
	const unsigned int lightmap_size =
		gl_state.block_width * gl_state.block_height * LIGHTMAP_BYTES;

	if (!gl_lms.lightmap_buffer[buffer])
	{
		gl_lms.lightmap_buffer[buffer] = malloc (lightmap_size);
	}
	if (!gl_lms.lightmap_buffer[buffer])
	{
		ri.Sys_Error(ERR_FATAL, "Could not allocate lightmap buffer %d\n",
			buffer);
	}
	if (clean)
{
		memset (gl_lms.lightmap_buffer[buffer], 0, lightmap_size);
	}
}

void
LM_InitBlock(void)
{
	memset(gl_lms.allocated, 0, gl_state.block_width * sizeof(int));

	if (gl_config.multitexture)
	{
		LM_AllocLightmapBuffer(gl_lms.current_lightmap_texture, false);
	}
	}

void
LM_UploadBlock(qboolean dynamic)
	{
	const int texture = (dynamic)? 0 : gl_lms.current_lightmap_texture;
	const int buffer = (gl_config.multitexture)? gl_lms.current_lightmap_texture : 0;
	int height = 0;

	R_Bind(gl_state.lightmap_textures + texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (dynamic)
	{
		int i;

		for (i = 0; i < gl_state.block_width; i++)
		{
			if (gl_lms.allocated[i] > height)
			{
				height = gl_lms.allocated[i];
			}
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gl_state.block_width,
				height, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
				gl_lms.lightmap_buffer[buffer]);
	}
	else
	{
		gl_lms.internal_format = GL_LIGHTMAP_FORMAT;
		glTexImage2D(GL_TEXTURE_2D, 0, gl_lms.internal_format,
				gl_state.block_width, gl_state.block_height,
				0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
				gl_lms.lightmap_buffer[buffer]);

		if (++gl_lms.current_lightmap_texture == gl_state.max_lightmaps)
		{
			ri.Sys_Error(ERR_DROP,
					"LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n");
		}
	}
}

/*
 * returns a texture number and the position inside it
 */
qboolean
LM_AllocBlock(int w, int h, int *x, int *y)
{
	int i, j;
	int best, best2;

	best = gl_state.block_height;

	for (i = 0; i < gl_state.block_width - w; i++)
	{
		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (gl_lms.allocated[i + j] >= best)
			{
				break;
			}

			if (gl_lms.allocated[i + j] > best2)
			{
				best2 = gl_lms.allocated[i + j];
			}
		}

		if (j == w)
		{
			/* this is a valid spot */
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > gl_state.block_height)
	{
		return false;
	}

	for (i = 0; i < w; i++)
	{
		gl_lms.allocated[*x + i] = best + h;
	}

	return true;
}

void
LM_BuildPolygonFromSurface(model_t *currentmodel, msurface_t *fa)
{
	int i, lindex, lnumverts;
	medge_t *pedges, *r_pedge;
	float *vec;
	float s, t;
	glpoly_t *poly;
	vec3_t total;

	/* reconstruct the polygon */
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;

	VectorClear(total);

	/* draw texture */
	poly = Hunk_Alloc(sizeof(glpoly_t) +
		   (lnumverts - 4) * VERTEXSIZE * sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd(total, vec, total);
		VectorCopy(vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		/* lightmap texture coordinates */
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= gl_state.block_width * 16; /* fa->texinfo->texture->width; */

		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= gl_state.block_height * 16; /* fa->texinfo->texture->height; */

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}
}

void
LM_CreateSurfaceLightmap(msurface_t *surf)
{
	int smax, tmax, buffer;
	byte *base;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
	{
		return;
	}

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;

	if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
	{
		LM_UploadBlock(false);
		LM_InitBlock();

		if (!LM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			ri.Sys_Error(ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n",
					smax, tmax);
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;
	buffer = (gl_config.multitexture)? surf->lightmaptexturenum : 0;

	base = gl_lms.lightmap_buffer[buffer];
	base += (surf->light_t * gl_state.block_width + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState(surf);
	R_BuildLightMap(surf, base, gl_state.block_width * LIGHTMAP_BYTES);
}

void
LM_BeginBuildingLightmaps(model_t *m)
{
	static lightstyle_t lightstyles[MAX_LIGHTSTYLES];
	int i;

	LM_FreeLightmapBuffers();
	gl_lms.allocated = (int*)malloc(gl_state.block_width * sizeof(int));
	if (!gl_lms.allocated)
	{
		ri.Sys_Error(ERR_FATAL, "Could not create lightmap allocator\n");
	}
	memset(gl_lms.allocated, 0, gl_state.block_width * sizeof(int));

	r_framecount = 1; /* no dlightcache */

	/* setup the base lightstyles so the lightmaps
	   won't have to be regenerated the first time
	   they're seen */
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}

	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
	{
		gl_state.lightmap_textures = TEXNUM_LIGHTMAPS;
	}

	gl_lms.current_lightmap_texture = 1;
	gl_lms.internal_format = GL_LIGHTMAP_FORMAT;

	if (gl_config.multitexture)
	{
		LM_AllocLightmapBuffer(gl_lms.current_lightmap_texture, false);
		return;
	}

	// dynamic lightmap for classic rendering path (no multitexture)
	LM_AllocLightmapBuffer(0, true);

	/* initialize the dynamic lightmap texture */
	R_Bind(gl_state.lightmap_textures + 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, gl_lms.internal_format,
			gl_state.block_width, gl_state.block_height,
			0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
			gl_lms.lightmap_buffer[0]);
}

void
LM_EndBuildingLightmaps(void)
{
	LM_UploadBlock(false);
}

