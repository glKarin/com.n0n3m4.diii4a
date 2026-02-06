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
// r_surf.c: surface-related refresh code

#include "quakedef.h"
#if defined(GLQUAKE)
#include "glquake.h"
#include "shader.h"
#include "renderque.h"
#include <math.h>

extern cvar_t r_lightmap_nearest;

void GLBE_ClearVBO(vbo_t *vbo, qboolean dataonly)
{
	int vboh[6 + MAXRLIGHTMAPS];
	int i, j;
	vboh[0] = vbo->indicies.gl.vbo;
	vboh[1] = vbo->coord.gl.vbo;
	vboh[2] = vbo->texcoord.gl.vbo;
	vboh[3] = vbo->normals.gl.vbo;
	vboh[4] = vbo->svector.gl.vbo;
	vboh[5] = vbo->tvector.gl.vbo;
	for (i = 0; i < MAXRLIGHTMAPS; i++)
		vboh[6+i] = vbo->lmcoord[i].gl.vbo;

	memset (&vbo->indicies, 0, sizeof(vbo->indicies));
	memset (&vbo->coord, 0, sizeof(vbo->coord));
	memset (&vbo->texcoord, 0, sizeof(vbo->texcoord));
	memset (&vbo->normals, 0, sizeof(vbo->normals));
	memset (&vbo->svector, 0, sizeof(vbo->svector));
	memset (&vbo->tvector, 0, sizeof(vbo->tvector));
	memset (&vbo->lmcoord, 0, sizeof(vbo->lmcoord));

	for (i = 0; i < 7; i++)
	{
		if (!vboh[i])
			continue;
		for (j = 0; j < i; j++)
		{
			if (vboh[j] == vboh[i])
				break;	//already freed by one of the other ones
		}
		if (j == i)
			qglDeleteBuffersARB(1, &vboh[i]);
	}
	if (vbo->vertdata)
		BZ_Free(vbo->vertdata);
	vbo->vertdata = NULL;
	BZ_Free(vbo->meshlist);
	vbo->meshlist = NULL;
	if (!dataonly)
		BZ_Free(vbo);
}

void GLBE_SetupVAO(vbo_t *vbo, unsigned int vaodynamic, unsigned int vaostatic);

static qboolean GL_BuildVBO(vbo_t *vbo, void *vdata, int vsize, void *edata, int elementsize, unsigned int vaodynamic)
{
	unsigned int vbos[2];
	int s;
	unsigned int vaostatic = 0;

	if (!qglGenBuffersARB || !vsize || !elementsize)
		return false;

	qglGenBuffersARB(1+(elementsize>0), vbos);

	//opengl ate our data, fixup the vbo arrays to point to the vbo instead of the raw data

	if (vbo->indicies.gl.addr && elementsize)
	{
		vbo->indicies.gl.vbo = vbos[1];
		vbo->indicies.gl.addr = (index_t*)((char*)vbo->indicies.gl.addr - (char*)edata);
		vaostatic |= 1u<<VATTR_LEG_ELEMENTS;
	}
	if (vbo->coord.gl.addr)
	{
		vbo->coord.gl.vbo = vbos[0];
		vbo->coord.gl.addr = (vecV_t*)((char*)vbo->coord.gl.addr - (char*)vdata);
		vaostatic |= 1u<<VATTR_VERTEX1;
	}
	if (vbo->texcoord.gl.addr)
	{
		vbo->texcoord.gl.vbo = vbos[0];
		vbo->texcoord.gl.addr = (vec2_t*)((char*)vbo->texcoord.gl.addr - (char*)vdata);
		vaostatic |= 1u<<VATTR_TEXCOORD;
	}
	for (s = 0; s < MAXRLIGHTMAPS; s++)
	{
		if (vbo->colours[s].gl.addr)
		{
			vbo->colours[s].gl.vbo = vbos[0];
			vbo->colours[s].gl.addr = (vec4_t*)((char*)vbo->colours[s].gl.addr - (char*)vdata);
			switch(s)
			{
			default: vaostatic |= 1u<<VATTR_COLOUR; break;
#if MAXRLIGHTMAPS > 1
			case 1: vaostatic |= 1u<<VATTR_COLOUR2; break;
			case 2: vaostatic |= 1u<<VATTR_COLOUR3; break;
			case 3: vaostatic |= 1u<<VATTR_COLOUR4; break;
#endif
			}
		}
		if (vbo->lmcoord[s].gl.addr)
		{
			vbo->lmcoord[s].gl.vbo = vbos[0];
			vbo->lmcoord[s].gl.addr = (vec2_t*)((char*)vbo->lmcoord[s].gl.addr - (char*)vdata);
			switch(s)
			{
			default: vaostatic |= 1u<<VATTR_LMCOORD; break;
#if MAXRLIGHTMAPS > 1
			case 1: vaostatic |= 1u<<VATTR_LMCOORD2; break;
			case 2: vaostatic |= 1u<<VATTR_LMCOORD3; break;
			case 3: vaostatic |= 1u<<VATTR_LMCOORD4; break;
#endif
			}
		}
	}
	if (vbo->normals.gl.addr)
	{
		vbo->normals.gl.vbo = vbos[0];
		vbo->normals.gl.addr = (vec3_t*)((char*)vbo->normals.gl.addr - (char*)vdata);
		vaostatic |= 1u<<VATTR_NORMALS;
	}
	if (vbo->svector.gl.addr)
	{
		vbo->svector.gl.vbo = vbos[0];
		vbo->svector.gl.addr = (vec3_t*)((char*)vbo->svector.gl.addr - (char*)vdata);
		vaostatic |= 1u<<VATTR_SNORMALS;
	}
	if (vbo->tvector.gl.addr)
	{
		vbo->tvector.gl.vbo = vbos[0];
		vbo->tvector.gl.addr = (vec3_t*)((char*)vbo->tvector.gl.addr - (char*)vdata);
		vaostatic |= 1u<<VATTR_TNORMALS;
	}


	GLBE_SetupVAO(vbo, vaodynamic, vaostatic);
	
	if (qglBufferStorage)
	{	//gl4.4 allows us to explicitly create device-only vbos
		qglBufferStorage(GL_ARRAY_BUFFER_ARB, vsize, vdata, 0);
		if (elementsize>0)
			qglBufferStorage(GL_ELEMENT_ARRAY_BUFFER_ARB, elementsize, edata, 0);
	}
	else
	{
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vsize, vdata, GL_STATIC_DRAW_ARB);
		if (elementsize>0)
		{
			qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, elementsize, edata, GL_STATIC_DRAW_ARB);
		}
	}

	return true;
}

//allocates an aligned buffer. caller needs to make sure there's enough space.
void *allocbuf(char **p, int elements, int elementsize)
{
	void *ret;
	*p += elementsize - 1;
	*p -= (size_t)*p & (elementsize-1);
	ret = *p;
	*p += elements*elementsize;
	return ret;
}

void GLBE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch, int lightmaps)
{
	unsigned int maxvboverts;
	unsigned int maxvboelements;

	unsigned int i, s;
	unsigned int v;
	unsigned int vcount, ecount;
	unsigned int pervertsize;	//erm, that name wasn't intentional
	unsigned int meshes;

	vbo_t *vbo;
	mesh_t *m;
	char *p;

	vecV_t *coord;
	vec2_t *texcoord;
	vec2_t *lmcoord[MAXRLIGHTMAPS];
	vec3_t *normals;
	vec3_t *svector;
	vec3_t *tvector;
	vec4_t *colours[MAXRLIGHTMAPS];
	index_t *indicies;
	batch_t *batch;
	int vbosize;


	vbo = Z_Malloc(sizeof(*vbo));

	maxvboverts = 0;
	maxvboelements = 0;
	meshes = 0;
	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		for (i=0 ; i<batch->maxmeshes ; i++)
		{
			m = batch->mesh[i];
			meshes++;
			maxvboelements += m->numindexes;
			maxvboverts += m->numvertexes;
		}
	}
	if (maxvboverts > MAX_INDICIES)
		Sys_Error("Building a vbo with too many verticies\n");


	vcount = 0;
	ecount = 0;

	pervertsize =	sizeof(vecV_t)+	//coord
				sizeof(vec2_t)+	//tex
				sizeof(vec2_t)*lightmaps+	//lm
				sizeof(vec3_t)+	//normal
				sizeof(vec3_t)+	//sdir
				sizeof(vec3_t)+	//tdir
				sizeof(vec4_t)*lightmaps;	//colours

	vbo->vertdata = BZ_Malloc((maxvboverts+1)*pervertsize + (maxvboelements+1)*sizeof(index_t));

	p = vbo->vertdata;

	vbo->coord.gl.addr = allocbuf(&p, maxvboverts, sizeof(vecV_t));
	vbo->texcoord.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec2_t));
	for (s = 0; s < lightmaps; s++)
		vbo->lmcoord[s].gl.addr = allocbuf(&p, maxvboverts, sizeof(vec2_t));
	for (; s < MAXRLIGHTMAPS; s++)
		vbo->lmcoord[s].gl.addr = NULL;
	vbo->normals.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec3_t));
	vbo->svector.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec3_t));
	vbo->tvector.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec3_t));
	for (s = 0; s < lightmaps; s++)
		vbo->colours[s].gl.addr = allocbuf(&p, maxvboverts, sizeof(vec4_t));
	for (; s < MAXRLIGHTMAPS; s++)
		vbo->lmcoord[s].gl.addr = NULL;
	vbosize = (char*)p - (char*)vbo->coord.gl.addr;
	if ((char*)p - (char*)vbo->vertdata > (maxvboverts+1)*pervertsize)
		Sys_Error("GLBE_GenBatchVBOs: aligned overflow");
	vbo->indicies.gl.addr = allocbuf(&p, maxvboelements, sizeof(index_t));

	coord = vbo->coord.gl.addr;
	texcoord = vbo->texcoord.gl.addr;
	for (s = 0; s < MAXRLIGHTMAPS; s++)
	{
		lmcoord[s] = vbo->lmcoord[s].gl.addr;
		colours[s] = vbo->colours[s].gl.addr;
	}
	normals = vbo->normals.gl.addr;
	svector = vbo->svector.gl.addr;
	tvector = vbo->tvector.gl.addr;
	indicies = vbo->indicies.gl.addr;

	//vbo->meshcount = meshes;
	//vbo->meshlist = BZ_Malloc(meshes*sizeof(*vbo->meshlist));

	meshes = 0;


	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		batch->vbo = vbo;
		for (i=0 ; i<batch->maxmeshes ; i++)
		{
			m = batch->mesh[i];

//			surf->mark = &vbo->meshlist[meshes++];
//			*surf->mark = NULL;

			m->vbofirstvert = vcount;
			m->vbofirstelement = ecount;
			for (v = 0; v < m->numindexes; v++)
				indicies[ecount++] = vcount + m->indexes[v];
			for (v = 0; v < m->numvertexes; v++)
			{
				coord[vcount+v][0] = m->xyz_array[v][0];
				coord[vcount+v][1] = m->xyz_array[v][1];
				coord[vcount+v][2] = m->xyz_array[v][2];
				if (m->st_array)
				{
					texcoord[vcount+v][0] = m->st_array[v][0];
					texcoord[vcount+v][1] = m->st_array[v][1];
				}
				for (s = 0; s < lightmaps; s++)
				{
					if (m->lmst_array[s])
					{
						lmcoord[s][vcount+v][0] = m->lmst_array[s][v][0];
						lmcoord[s][vcount+v][1] = m->lmst_array[s][v][1];
					}
					if (m->colors4f_array[s])
					{
						colours[s][vcount+v][0] = m->colors4f_array[s][v][0];
						colours[s][vcount+v][1] = m->colors4f_array[s][v][1];
						colours[s][vcount+v][2] = m->colors4f_array[s][v][2];
						colours[s][vcount+v][3] = m->colors4f_array[s][v][3];
					}
				}
				if (m->normals_array)
				{
					normals[vcount+v][0] = m->normals_array[v][0];
					normals[vcount+v][1] = m->normals_array[v][1];
					normals[vcount+v][2] = m->normals_array[v][2];
				}
				if (m->snormals_array)
				{
					svector[vcount+v][0] = m->snormals_array[v][0];
					svector[vcount+v][1] = m->snormals_array[v][1];
					svector[vcount+v][2] = m->snormals_array[v][2];
				}
				if (m->tnormals_array)
				{
					tvector[vcount+v][0] = m->tnormals_array[v][0];
					tvector[vcount+v][1] = m->tnormals_array[v][1];
					tvector[vcount+v][2] = m->tnormals_array[v][2];
				}
			}
			vcount += v;
		}
	}

	if (GL_BuildVBO(vbo, vbo->coord.gl.addr, vbosize/*vcount*pervertsize*/, indicies, ecount*sizeof(index_t), 0))
	{
		BZ_Free(vbo->vertdata);
		vbo->vertdata = NULL;
	}
	vbo->vertcount = vcount;

	vbo->next = *vbochain;
	*vbochain = vbo;
}

void GLBE_GenBrushModelVBO(model_t *mod)
{
	unsigned int vcount;
	unsigned int cvcount;


	batch_t *batch, *fbatch;
	int sortid;
	int i;

	fbatch = NULL;
	vcount = 0;
	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	{
		if (!mod->batches[sortid])
			continue;

		for (fbatch = batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
		{
			for (i = 0, cvcount = 0; i < batch->maxmeshes; i++)
				cvcount += batch->mesh[i]->numvertexes;

			//firstmesh got reused as the number of verticies in each batch
			if (vcount + cvcount > MAX_INDICIES)
			{
				GLBE_GenBatchVBOs(&mod->vbos, fbatch, batch, mod->lightmaps.surfstyles);
				fbatch = batch;
				vcount = 0;
			}

			vcount += cvcount;
		}

		GLBE_GenBatchVBOs(&mod->vbos, fbatch, batch, mod->lightmaps.surfstyles);
	}

#if 0
	if (!mod->numsurfaces)
		return;

	for (t = 0; t < mod->numtextures; t++)
	{
		if (!mod->textures[t])
			continue;
		vbo = &mod->textures[t]->vbo;
		BE_ClearVBO(vbo);

		maxvboverts = 0;
		maxvboelements = 0;
		meshes = 0;
		for (i=0 ; i<mod->numsurfaces ; i++)
		{
			if (mod->surfaces[i].texinfo->texture != mod->textures[t])
				continue;
			m = mod->surfaces[i].mesh;
			if (!m)
				continue;

			meshes++;
			maxvboelements += m->numindexes;
			maxvboverts += m->numvertexes;
		}
#if sizeof_index_t == 2
		if (maxvboverts > (1<<(sizeof(index_t)*8))-1)
			continue;
#endif
		if (!maxvboverts)
			continue;

		//fixme: stop this from leaking!
		vcount = 0;
		ecount = 0;

		pervertsize =	sizeof(vecV_t)+	//coord
					sizeof(vec2_t)+	//tex
					sizeof(vec2_t)+	//lm
					sizeof(vec3_t)+	//normal
					sizeof(vec3_t)+	//sdir
					sizeof(vec3_t)+	//tdir
					sizeof(vec4_t);	//colours

		vbo->vertdata = BZ_Malloc((maxvboverts+1)*pervertsize + (maxvboelements+1)*sizeof(index_t));

		p = vbo->vertdata;

		vbo->coord.gl.addr = allocbuf(&p, maxvboverts, sizeof(vecV_t));
		vbo->texcoord.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec2_t));
		vbo->lmcoord.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec2_t));
		vbo->normals.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec3_t));
		vbo->svector.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec3_t));
		vbo->tvector.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec3_t));
		vbo->colours.gl.addr = allocbuf(&p, maxvboverts, sizeof(vec4_t));
		vbo->indicies.gl.addr = allocbuf(&p, maxvboelements, sizeof(index_t));

		coord = vbo->coord.gl.addr;
		texcoord = vbo->texcoord.gl.addr;
		lmcoord = vbo->lmcoord.gl.addr;
		normals = vbo->normals.gl.addr;
		svector = vbo->svector.gl.addr;
		tvector = vbo->tvector.gl.addr;
		colours = vbo->colours.gl.addr;
		indicies = vbo->indicies.gl.addr;

		vbo->meshcount = meshes;
		vbo->meshlist = BZ_Malloc(meshes*sizeof(*vbo->meshlist));

		meshes = 0;

		for (i=0 ; i<mod->numsurfaces ; i++)
		{
			if (mod->surfaces[i].texinfo->texture != mod->textures[t])
				continue;
			m = mod->surfaces[i].mesh;
			if (!m)
				continue;

			mod->surfaces[i].mark = &vbo->meshlist[meshes++];
			*mod->surfaces[i].mark = NULL;

			m->vbofirstvert = vcount;
			m->vbofirstelement = ecount;
			for (v = 0; v < m->numindexes; v++)
				indicies[ecount++] = vcount + m->indexes[v];
			for (v = 0; v < m->numvertexes; v++)
			{
				coord[vcount+v][0] = m->xyz_array[v][0];
				coord[vcount+v][1] = m->xyz_array[v][1];
				coord[vcount+v][2] = m->xyz_array[v][2];
				if (m->st_array)
				{
					texcoord[vcount+v][0] = m->st_array[v][0];
					texcoord[vcount+v][1] = m->st_array[v][1];
				}
				if (m->lmst_array)
				{
					lmcoord[vcount+v][0] = m->lmst_array[v][0];
					lmcoord[vcount+v][1] = m->lmst_array[v][1];
				}
				if (m->normals_array)
				{
					normals[vcount+v][0] = m->normals_array[v][0];
					normals[vcount+v][1] = m->normals_array[v][1];
					normals[vcount+v][2] = m->normals_array[v][2];
				}
				if (m->snormals_array)
				{
					svector[vcount+v][0] = m->snormals_array[v][0];
					svector[vcount+v][1] = m->snormals_array[v][1];
					svector[vcount+v][2] = m->snormals_array[v][2];
				}
				if (m->tnormals_array)
				{
					tvector[vcount+v][0] = m->tnormals_array[v][0];
					tvector[vcount+v][1] = m->tnormals_array[v][1];
					tvector[vcount+v][2] = m->tnormals_array[v][2];
				}
				if (m->colors4f_array)
				{
					colours[vcount+v][0] = m->colors4f_array[v][0];
					colours[vcount+v][1] = m->colors4f_array[v][1];
					colours[vcount+v][2] = m->colors4f_array[v][2];
					colours[vcount+v][3] = m->colors4f_array[v][3];
				}
			}
			vcount += v;
		}

		if (GL_BuildVBO(vbo, vbo->vertdata, vcount*pervertsize, indicies, ecount*sizeof(index_t), 0))
		{
			BZ_Free(vbo->vertdata);
			vbo->vertdata = NULL;
		}
		vbo->vertcount = vcount;
	}
#endif
}

#endif
