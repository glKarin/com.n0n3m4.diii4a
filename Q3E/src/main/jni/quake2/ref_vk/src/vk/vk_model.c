/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
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
 * Model loading and caching. Includes the .bsp file format
 *
 * =======================================================================
 */

#include "header/local.h"

static YQ2_ALIGNAS_TYPE(int) byte mod_novis[MAX_MAP_LEAFS / 8];

#define	MAX_MOD_KNOWN	512
static model_t *models_known;
static int mod_numknown = 0;
static int mod_max = 0;
static int mod_loaded = 0;
static int models_known_max = 0;
int registration_sequence;

const byte *
Mod_ClusterPVS(int cluster, const model_t *model)
{
	if ((cluster == -1) || !model->vis)
	{
		return mod_novis;
	}

	return Mod_DecompressVis((byte *)model->vis +
			model->vis->bitofs[cluster][DVIS_PVS],
			(model->vis->numclusters + 7) >> 3);
}


//===============================================================================

static void
Mod_Reallocate (void)
{
	if ((models_known_max >= (mod_max * 4)) && (models_known != NULL))
	{
		return;
	}

	if (models_known)
	{
		// Always up if already allocated
		models_known_max *= 2;
		// free up
		Mod_FreeAll();
		Mod_FreeModelsKnown();
	}

	if (models_known_max < (mod_max * 4))
	{
		// double is not enough?
		models_known_max = ROUNDUP(mod_max * 4, 16);
	}

	R_Printf(PRINT_ALL, "Reallocate space for %d models.\n", models_known_max);

	models_known = calloc(models_known_max, sizeof(model_t));
}

void
Mod_Init(void)
{
	mod_max = 0;
	memset(mod_novis, 0xff, sizeof(mod_novis));
	mod_numknown = 0;
	mod_loaded = 0;

	models_known_max = MAX_MOD_KNOWN;
	models_known = NULL;

	Mod_Reallocate ();
}

static void
Mod_Free(model_t *mod)
{
	if (!mod->extradata)
	{
		/* looks as empty model */
		memset (mod, 0, sizeof(*mod));
		return;
	}

	if (r_validation->value > 0)
	{
		R_Printf(PRINT_ALL, "%s: Unload %s[%d]\n", __func__, mod->name, mod_loaded);
	}

	Hunk_Free(mod->extradata);
	memset(mod, 0, sizeof(*mod));
	mod_loaded --;
	if (mod_loaded < 0)
	{
		ri.Sys_Error (ERR_DROP, "%s: Broken unload", __func__);
	}
}

void
Mod_FreeAll (void)
{
	int		i;

	for (i=0 ; i<mod_numknown ; i++)
	{
		if (models_known[i].extradatasize)
			Mod_Free (&models_known[i]);
	}
}

void
Mod_FreeModelsKnown(void)
{
	free(models_known);
	models_known = NULL;
}

static void
Mod_LoadSubmodels(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	dmodel_t *in;
	model_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		if (i == 0)
		{
			// copy parent as template for first model
			memcpy(out, loadmodel, sizeof(*out));
		}
		else
		{
			// copy first as template for model
			memcpy(out, loadmodel->submodels, sizeof(*out));
		}

		Com_sprintf (out->name, sizeof(out->name), "*%d", i);

		for (j = 0; j < 3; j++)
		{
			/* spread the mins / maxs by a pixel */
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}

		out->radius = Mod_RadiusFromBounds(out->mins, out->maxs);
		out->firstnode = LittleLong(in->headnode);
		out->firstmodelsurface = LittleLong(in->firstface);
		out->nummodelsurfaces = LittleLong(in->numfaces);
		// visleafs
		out->numleafs = 0;
		//  check limits
		if (out->firstnode >= loadmodel->numnodes)
		{
			Com_Error(ERR_DROP, "%s: Inline model %i has bad firstnode",
					__func__, i);
		}
	}
}

/*
 * Fills in s->texturemins[] and s->extents[]
 */
static void
CalcSurfaceExtents(model_t *loadmodel, msurface_t *s)
{
	float mins[2], maxs[2], val;
	int i;
	mtexinfo_t *tex;
	int bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		int e, j;
		mvertex_t *v;

		e = loadmodel->surfedges[s->firstedge + i];

		if (e >= 0)
		{
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		}
		else
		{
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		}

		for (j = 0; j < 2; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				  v->position[1] * tex->vecs[j][1] +
				  v->position[2] * tex->vecs[j][2] +
				  tex->vecs[j][3];

			if (val < mins[j])
			{
				mins[j] = val;
			}

			if (val > maxs[j])
			{
				maxs[j] = val;
			}
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}

static int
calcTexinfoAndFacesSize(const lump_t *fl, const byte *mod_base, const lump_t *tl)
{
	dface_t* face_in = (void *)(mod_base + fl->fileofs);
	texinfo_t* texinfo_in = (void *)(mod_base + tl->fileofs);

	if (fl->filelen % sizeof(*face_in) || tl->filelen % sizeof(*texinfo_in))
	{
		// will error out when actually loading it
		return 0;
	}

	int ret = 0;

	int face_count = fl->filelen / sizeof(*face_in);
	int texinfo_count = tl->filelen / sizeof(*texinfo_in);

	{
		// out = Hunk_Alloc(count * sizeof(*out));
		int baseSize = face_count * sizeof(msurface_t);
		baseSize = (baseSize + 31) & ~31;
		ret += baseSize;

		int ti_size = texinfo_count * sizeof(mtexinfo_t);
		ti_size = (ti_size + 31) & ~31;
		ret += ti_size;
	}

	int numWarpFaces = 0;

	for (int surfnum = 0; surfnum < face_count; surfnum++, face_in++)
	{
		int numverts = LittleShort(face_in->numedges);
		int ti = LittleShort(face_in->texinfo);
		if ((ti < 0) || (ti >= texinfo_count))
		{
			return 0; // will error out
		}
		int texFlags = LittleLong(texinfo_in[ti].flags);

		/* set the drawing flags */
		if (texFlags & SURF_WARP)
		{
			if (numverts > 60)
				return 0; // will error out in R_SubdividePolygon()

			// Vk_SubdivideSurface(out, loadmodel); /* cut up polygon for warps */
			// for each (pot. recursive) call to R_SubdividePolygon():
			//   sizeof(vkpoly_t) + ((numverts - 4) + 2) * VERTEXSIZE*sizeof(float)

			// this is tricky, how much is allocated depends on the size of the surface
			// which we don't know (we'd need the vertices etc to know, but we can't load
			// those without allocating...)
			// so we just count warped faces and use a generous estimate below

			++numWarpFaces;
		}
		else
		{
			// Vk_BuildPolygonFromSurface(out);
			// => poly = Hunk_Alloc(sizeof(vkpoly_t) + (numverts - 4) * VERTEXSIZE*sizeof(float));
			int polySize = sizeof(vkpoly_t) + (numverts - 4) * VERTEXSIZE*sizeof(float);
			polySize = (polySize + 31) & ~31;
			ret += polySize;
		}
	}

	// yeah, this is a bit hacky, but it looks like for each warped face
	// 256-55000 bytes are allocated (usually on the lower end),
	// so just assume 48k per face to be safe
	ret += numWarpFaces * 49152;
	ret += 5000000; // and 5MB extra just in case

	return ret;
}

// Extension to support lightmaps that aren't tied to texture scale.
static int
Mod_LoadDecoupledLM(const dlminfo_t* lminfos, int surfnum, msurface_t *out)
{
	const dlminfo_t *lminfo;
	unsigned short lmwidth, lmheight;

	if (lminfos == NULL) {
		return -1;
	}

	lminfo = lminfos + surfnum;

	lmwidth = LittleShort(lminfo->lmwidth);
	lmheight = LittleShort(lminfo->lmheight);

	if (lmwidth <= 0 || lmheight <= 0) {
		return -1;
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			out->lmvecs[i][j] = LittleFloat(lminfo->vecs[i][j]);
		}
	}

	out->extents[0] = (short)(lmwidth - 1);
	out->extents[1] = (short)(lmheight - 1);
	out->lmshift = 0;
	out->texturemins[0] = 0;
	out->texturemins[1] = 0;

	float v0 = VectorLength(out->lmvecs[0]);
	out->lmvlen[0] = v0 > 0.0f ? 1.0f / v0 : 0.0f;

	float v1 = VectorLength(out->lmvecs[1]);
	out->lmvlen[1] = v1 > 0.0f ? 1.0f / v1 : 0.0f;

	return LittleLong(lminfo->lightofs);
}

const void *
Mod_LoadBSPXFindLump(const bspx_header_t* bspx_header, char* lumpname, int* plumpsize,
	const byte* mod_base)
{
	int i;
	bspx_lump_t* lump;

	if (!bspx_header) {
		return NULL;
	}

	lump = (bspx_lump_t*)(bspx_header + 1);
	for (i = 0; i < bspx_header->numlumps; i++, lump++) {
		if (!strcmp(lump->lumpname, lumpname)) {
			if (plumpsize) {
				*plumpsize = lump->filelen;
			}
			return mod_base + lump->fileofs;
		}
	}

	return NULL;
}

bspx_header_t *
Mod_LoadBSPX(int filesize, byte* mod_base)
{
	dheader_t* header;
	bspx_header_t* xheader;
	bspx_lump_t* lump;
	int i;
	int xofs;

	// find end of last lump
	header = (dheader_t*)mod_base;
	xofs = 0;
	for (i = 0; i < HEADER_LUMPS; i++) {
		xofs = Q_max(xofs,
			ROUNDUP(header->lumps[i].fileofs + header->lumps[i].filelen, sizeof(int)));
	}

	if (xofs + sizeof(bspx_header_t) > filesize) {
		return NULL;
	}

	xheader = (bspx_header_t*)(mod_base + xofs);
	if (LittleLong(xheader->ident) != BSPXHEADER)
	{
		R_Printf(PRINT_ALL, "%s: Incorrect header ident.\n", __func__);
		return NULL;
	}

	xheader->numlumps = LittleLong(xheader->numlumps);

	if (xheader->numlumps < 0 || xofs + sizeof(bspx_header_t) + xheader->numlumps * sizeof(bspx_lump_t) > filesize) {
		return NULL;
	}

	// byte-swap and check sanity
	lump = (bspx_lump_t*)(xheader + 1); // lumps immediately follow the header
	for (i = 0; i < xheader->numlumps; i++, lump++) {
		lump->lumpname[sizeof(lump->lumpname) - 1] = '\0'; // make sure it ends with zero
		lump->fileofs = LittleLong(lump->fileofs);
		lump->filelen = LittleLong(lump->filelen);
		if (lump->fileofs < 0 || lump->filelen < 0 || (unsigned)(lump->fileofs + lump->filelen) >(unsigned)filesize) {
			return NULL;
		}
	}

	// success
	return xheader;
}

static void
SetSurfaceLighting(model_t *loadmodel, msurface_t *out, byte *styles, int lightofs)
{
	int i;

	/* lighting info */
	for (i = 0; i < MAXLIGHTMAPS; i++)
	{
		out->styles[i] = styles[i];
	}

	i = LittleLong(lightofs);
	if (i == -1 || loadmodel->lightdata == NULL)
	{
		out->samples = NULL;
	}
	else
	{
		out->samples = loadmodel->lightdata + i;
	}
}

/*
=================
Mod_LoadFaces
=================
*/
static void
Mod_LoadFaces(model_t *loadmodel, const byte *mod_base, const lump_t *l, const bspx_header_t *bspx_header)
{
	int i, count, surfnum, lminfosize, lightofs;
	const dlminfo_t *lminfos;
	msurface_t *out;
	dface_t *in;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	lminfos = Mod_LoadBSPXFindLump(bspx_header, "DECOUPLED_LM", &lminfosize, mod_base);
	if (lminfos != NULL && lminfosize / sizeof(dlminfo_t) != loadmodel->numsurfaces) {
		R_Printf(PRINT_ALL, "%s: [%s] decoupled_lm size %ld does not match surface count %d\n",
			__func__, loadmodel->name, lminfosize / sizeof(dlminfo_t), loadmodel->numsurfaces);
		lminfos = NULL;
	}

	Vk_BeginBuildingLightmaps(loadmodel);

	for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		int side, ti, planenum;

		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);

		if (out->numedges < 3)
		{
			Com_Error(ERR_DROP, "%s: Surface with %d edges",
					__func__, out->numedges);
		}
		out->flags = 0;
		out->polys = NULL;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
		{
			out->flags |= SURF_PLANEBACK;
		}

		if (planenum < 0 || planenum >= loadmodel->numplanes)
		{
			Com_Error(ERR_DROP, "%s: Incorrect %d planenum.",
					__func__, planenum);
		}
		out->plane = loadmodel->planes + planenum;

		ti = LittleShort(in->texinfo);

		if ((ti < 0) || (ti >= loadmodel->numtexinfo))
		{
			Com_Error(ERR_DROP, "%s: bad texinfo number",
					__func__);
		}

		out->texinfo = loadmodel->texinfo + ti;

		lightofs = Mod_LoadDecoupledLM(lminfos, surfnum, out);
		if (lightofs < 0) {
			memcpy(out->lmvecs, out->texinfo->vecs, sizeof(out->lmvecs));
			out->lmshift = DEFAULT_LMSHIFT;
			out->lmvlen[0] = 1.0f;
			out->lmvlen[1] = 1.0f;

			CalcSurfaceExtents(loadmodel, out);

			lightofs = in->lightofs;
		}

		SetSurfaceLighting(loadmodel, out, in->styles, lightofs);

		/* set the drawing flags */
		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;

			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			Vk_SubdivideSurface(out, loadmodel);	// cut up polygon for warps
		}

		if (r_fixsurfsky->value)
		{
			if (out->texinfo->flags & SURF_SKY)
			{
				out->flags |= SURF_DRAWSKY;
			}
		}

		/* create lightmaps and polygons */
		if (!(out->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
		{
			Vk_CreateSurfaceLightmap(out);
		}

		if (!(out->texinfo->flags & SURF_WARP))
		{
			Vk_BuildPolygonFromSurface(out, loadmodel);
		}
	}

	Vk_EndBuildingLightmaps();
}

/*
=================
Mod_LoadLeafs
=================
*/
static void
Mod_LoadLeafs(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	dleaf_t *in;
	mleaf_t *out;
	int i, j, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		unsigned firstleafface;

		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		// make unsigned long from signed short
		firstleafface = LittleShort(in->firstleafface) & 0xFFFF;
		out->nummarksurfaces = LittleShort(in->numleaffaces) & 0xFFFF;

		out->firstmarksurface = loadmodel->marksurfaces + firstleafface;
		if ((firstleafface + out->nummarksurfaces) > loadmodel->nummarksurfaces)
		{
			ri.Sys_Error(ERR_DROP, "%s: wrong marksurfaces position in %s",
				__func__, loadmodel->name);
		}
	}
}

static void
Mod_LoadMarksurfaces(model_t *loadmodel, const byte *mod_base, const lump_t *l)
{
	int i, count;
	short *in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, loadmodel->name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		int j;
		j = LittleShort(in[i]);

		if ((j < 0) || (j >= loadmodel->numsurfaces))
		{
			ri.Sys_Error(ERR_DROP, "%s: bad surface number",
					__func__);
		}

		out[i] = loadmodel->surfaces + j;
	}
}

// calculate the size that Hunk_Alloc(), called by Mod_Load*() from Mod_LoadBrushModel(),
// will use (=> includes its padding), so we'll know how big the hunk needs to be
static int calcLumpHunkSize(const lump_t *l, int inSize, int outSize)
{
	if (l->filelen % inSize)
	{
		// Mod_Load*() will error out on this because of "funny size"
		// don't error out here because in Mod_Load*() it can print the functionname
		// (=> tells us what kind of lump) before shutting down the game
		return 0;
	}

	int count = l->filelen / inSize;
	int size = count * outSize;

	// round to cacheline, like Hunk_Alloc() does
	size = (size + 31) & ~31;
	return size;
}

/* Need to clean */
struct rctx_s {
	const byte *data;
	int ofs, size;
};

static byte ReadByte(struct rctx_s *ctx)
{
	if (ctx->ofs >= ctx->size)
	{
		ctx->ofs++;
		return 0;
	}
	return ctx->data[ctx->ofs++];
}

static int ReadInt(struct rctx_s *ctx)
{
	int r = (int)ReadByte(ctx)<<0;
		r|= (int)ReadByte(ctx)<<8;
		r|= (int)ReadByte(ctx)<<16;
		r|= (int)ReadByte(ctx)<<24;
	return r;
}

static float ReadFloat(struct rctx_s *ctx)
{
	union {float f; int i;} u;
	u.i = ReadInt(ctx);
	return u.f;
}

static bspxlightgrid_t*
Mod_LoadBSPXLightGrid(const bspx_header_t *bspx_header, const byte *mod_base)
{
	vec3_t step, mins;
	int size[3];
	bspxlightgrid_t *grid;
	unsigned int numstyles, numnodes, numleafs, rootnode;
	unsigned int nodestart, leafsamps = 0, i, j, k, s;
	struct bspxlgsamp_s *samp;
	struct rctx_s ctx = {0};

	ctx.data = Mod_LoadBSPXFindLump(bspx_header, "LIGHTGRID_OCTREE", &ctx.size, mod_base);
	if (!ctx.data)
	{
		return NULL;
	}

	for (j = 0; j < 3; j++)
		step[j] = ReadFloat(&ctx);
	for (j = 0; j < 3; j++)
		size[j] = ReadInt(&ctx);
	for (j = 0; j < 3; j++)
		mins[j] = ReadFloat(&ctx);

	numstyles = ReadByte(&ctx);	//urgh, misaligned the entire thing
	rootnode = ReadInt(&ctx);
	numnodes = ReadInt(&ctx);
	nodestart = ctx.ofs;
	ctx.ofs += (3+8)*4*numnodes;
	numleafs = ReadInt(&ctx);
	for (i = 0; i < numleafs; i++)
	{
		unsigned int lsz[3];
		ctx.ofs += 3*4;
		for (j = 0; j < 3; j++)
			lsz[j] = ReadInt(&ctx);
		j = lsz[0]*lsz[1]*lsz[2];
		leafsamps += j;
		while (j --> 0)
		{	//this loop is annonying, memcpy dreams...
			s = ReadByte(&ctx);
			if (s == 255)
				continue;
			ctx.ofs += s*4;
		}
	}

	grid = Hunk_Alloc(sizeof(*grid) + sizeof(*grid->leafs)*numleafs + sizeof(*grid->nodes)*numnodes + sizeof(struct bspxlgsamp_s)*leafsamps);
	memset(grid, 0xcc, sizeof(*grid) + sizeof(*grid->leafs)*numleafs + sizeof(*grid->nodes)*numnodes + sizeof(struct bspxlgsamp_s)*leafsamps);
	grid->leafs = (void*)(grid+1);
	grid->nodes = (void*)(grid->leafs + numleafs);
	samp = (void*)(grid->nodes+numnodes);

	for (j = 0; j < 3; j++)
		grid->gridscale[j] = 1/step[j];	//prefer it as a multiply
	VectorCopy(mins, grid->mins);
	VectorCopy(size, grid->count);
	grid->numnodes = numnodes;
	grid->numleafs = numleafs;
	grid->rootnode = rootnode;
	(void)numstyles;

	//rewind to the nodes. *sigh*
	ctx.ofs = nodestart;
	for (i = 0; i < numnodes; i++)
	{
		for (j = 0; j < 3; j++)
			grid->nodes[i].mid[j] = ReadInt(&ctx);
		for (j = 0; j < 8; j++)
			grid->nodes[i].child[j] = ReadInt(&ctx);
	}
	ctx.ofs += 4;
	for (i = 0; i < numleafs; i++)
	{
		for (j = 0; j < 3; j++)
			grid->leafs[i].mins[j] = ReadInt(&ctx);
		for (j = 0; j < 3; j++)
			grid->leafs[i].size[j] = ReadInt(&ctx);

		grid->leafs[i].rgbvalues = samp;

		j = grid->leafs[i].size[0]*grid->leafs[i].size[1]*grid->leafs[i].size[2];
		while (j --> 0)
		{
			s = ReadByte(&ctx);
			if (s == 0xff)
				memset(samp, 0xff, sizeof(*samp));
			else
			{
				for (k = 0; k < s; k++)
				{
					if (k >= 4)
						ReadInt(&ctx);
					else
					{
						samp->map[k].style = ReadByte(&ctx);
						samp->map[k].rgb[0] = ReadByte(&ctx);
						samp->map[k].rgb[1] = ReadByte(&ctx);
						samp->map[k].rgb[2] = ReadByte(&ctx);
					}
				}
				for (; k < 4; k++)
				{
					samp->map[k].style = (byte)~0u;
					samp->map[k].rgb[0] =
					samp->map[k].rgb[1] =
					samp->map[k].rgb[2] = 0;
				}
			}
			samp++;
		}
	}

	if (ctx.ofs != ctx.size)
		grid = NULL;

	return grid;
}

/*
=================
Mod_LoadBrushModel
=================
*/
static void
Mod_LoadBrushModel (model_t *mod, const void *buffer, int modfilelen)
{
	const bspx_header_t* bspx_header;
	int i, lightgridsize = 0;
	dheader_t *header;
	byte *mod_base;

	header = (dheader_t *)buffer;

	i = LittleLong(header->version);

	if (i != BSPVERSION)
	{
		ri.Sys_Error(ERR_DROP, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod->name, i, BSPVERSION);
	}

	/* swap all the lumps */
	mod_base = (byte *)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
	{
		((int *)header)[i] = LittleLong(((int *)header)[i]);
	}

	/* check for BSPX extensions */
	bspx_header = Mod_LoadBSPX(modfilelen, (byte*)header);

	// calculate the needed hunksize from the lumps
	int hunkSize = 0;
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_VERTEXES], sizeof(dvertex_t), sizeof(mvertex_t));
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_EDGES], sizeof(dedge_t), sizeof(medge_t));
	hunkSize += sizeof(medge_t) + 31; // for count+1 in Mod_LoadEdges()
	int surfEdgeCount = (header->lumps[LUMP_SURFEDGES].filelen+sizeof(int)-1)/sizeof(int);
	if(surfEdgeCount < MAX_MAP_SURFEDGES) // else it errors out later anyway
		hunkSize += calcLumpHunkSize(&header->lumps[LUMP_SURFEDGES], sizeof(int), sizeof(int));
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_LIGHTING], 1, 1);
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_PLANES], sizeof(dplane_t), sizeof(cplane_t)*2);
	hunkSize += calcTexinfoAndFacesSize(&header->lumps[LUMP_FACES], mod_base, &header->lumps[LUMP_TEXINFO]);
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_LEAFFACES], sizeof(short), sizeof(msurface_t *)); // yes, out is indeed a pointer!
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_VISIBILITY], 1, 1);
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_LEAFS], sizeof(dleaf_t), sizeof(mleaf_t));
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_NODES], sizeof(dnode_t), sizeof(mnode_t));
	hunkSize += calcLumpHunkSize(&header->lumps[LUMP_MODELS], sizeof(dmodel_t), sizeof(model_t));

	/* Get size of octree on disk, need to recheck real size */
	if (Mod_LoadBSPXFindLump(bspx_header, "LIGHTGRID_OCTREE", &lightgridsize, mod_base))
	{
		hunkSize += lightgridsize * 4;
	}

	mod->extradata = Hunk_Begin(hunkSize);
	mod->type = mod_brush;

	if (bspx_header)
	{
		mod->grid = Mod_LoadBSPXLightGrid(bspx_header, mod_base);
	}
	else
	{
		mod->grid = NULL;
	}

	/* load into heap */
	Mod_LoadVertexes(mod->name, &mod->vertexes, &mod->numvertexes, mod_base,
		&header->lumps[LUMP_VERTEXES], 0);
	Mod_LoadEdges(mod->name, &mod->edges, &mod->numedges,
		mod_base, &header->lumps[LUMP_EDGES], 1);
	Mod_LoadSurfedges(mod->name, &mod->surfedges, &mod->numsurfedges,
		mod_base, &header->lumps[LUMP_SURFEDGES], 0);
	Mod_LoadLighting(&mod->lightdata, mod_base, &header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(mod->name, &mod->planes, &mod->numplanes,
		mod_base, &header->lumps[LUMP_PLANES], 0);
	Mod_LoadTexinfo(mod->name, &mod->texinfo, &mod->numtexinfo,
		mod_base, &header->lumps[LUMP_TEXINFO], (findimage_t)Vk_FindImage,
		r_notexture, 0);
	Mod_LoadFaces(mod, mod_base, &header->lumps[LUMP_FACES], bspx_header);
	Mod_LoadMarksurfaces(mod, mod_base, &header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility(&mod->vis, mod_base, &header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(mod, mod_base, &header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes(mod->name, mod->planes, mod->numplanes, mod->leafs,
		mod->numleafs, &mod->nodes, &mod->numnodes, mod_base,
		&header->lumps[LUMP_NODES]);
	Mod_LoadSubmodels (mod, mod_base, &header->lumps[LUMP_MODELS]);
	mod->numframes = 2; /* regular and alternate animation */
}

//=============================================================================

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
static model_t *
Mod_ForName (const char *name, model_t *parent_model, qboolean crash)
{
	model_t *mod;
	void *buf;
	int i, modfilelen;

	if (!name[0])
	{
		Com_Error(ERR_DROP, "%s: NULL name", __func__);
	}

	/* inline models are grabbed only from worldmodel */
	if (name[0] == '*' && parent_model)
	{
		i = (int)strtol(name + 1, (char **)NULL, 10);

		if (i < 1 || i >= parent_model->numsubmodels)
		{
			Com_Error(ERR_DROP, "%s: bad inline model number",
					__func__);
		}

		return &parent_model->submodels[i];
	}

	/* search the currently loaded models */
	for (i = 0, mod = models_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			continue;
		}

		if (!strcmp(mod->name, name))
		{
			return mod;
		}
	}

	/* find a free model slot spot */
	for (i = 0, mod = models_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			break; /* free spot */
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == models_known_max)
		{
			Com_Error(ERR_DROP, "%s: mod_numknown == models_known_max", __func__);
		}

		mod_numknown++;
	}

	strcpy (mod->name, name);

	/* load the file */
	modfilelen = Mod_LoadFile(mod->name, &buf);
	if (!buf)
	{
		if (crash)
		{
			Com_Error(ERR_DROP, "%s: %s not found",
					__func__, mod->name);
		}

		if (r_validation->value > 0)
		{
			R_Printf(PRINT_ALL, "%s: Can't load %s\n", __func__, mod->name);
		}

		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	/* update count of loaded models */
	mod_loaded ++;
	if (r_validation->value > 0)
	{
		R_Printf(PRINT_ALL, "%s: Load %s[%d]\n", __func__, mod->name, mod_loaded);
	}

	/* fill it in */

	/* call the apropriate loader */
	switch (LittleLong(*(unsigned *)buf))
	{
	case DKMHEADER:
		/* fall through */
	case RAVENFMHEADER:
		/* fall through */
	case IDALIASHEADER:
		/* fall through */
	case IDMDLHEADER:
		{
			mod->extradata = Mod_LoadAliasModel(mod->name, buf, modfilelen,
				mod->mins, mod->maxs,
				(struct image_s **)mod->skins, (findimage_t)Vk_FindImage,
				&(mod->type));
			if (!mod->extradata)
			{
				ri.Sys_Error(ERR_DROP, "%s: Failed to load %s",
					__func__, mod->name);
			}
		};
		break;

	case IDSPRITEHEADER:
		{
			mod->extradata = Mod_LoadSP2(mod->name, buf, modfilelen,
				(struct image_s **)mod->skins, (findimage_t)Vk_FindImage,
				&(mod->type));
			if (!mod->extradata)
			{
				ri.Sys_Error(ERR_DROP, "%s: Failed to load %s",
					__func__, mod->name);
			}
		}
		break;

	case IDBSPHEADER:
		Mod_LoadBrushModel(mod, buf, modfilelen);
		break;

	default:
		ri.Sys_Error(ERR_DROP, "%s: unknown fileid for %s",
				__func__, mod->name);
		break;
	}

	if (mod->extradata)
	{
		mod->extradatasize = Hunk_End();
	}
	else
	{
		mod->extradatasize = 0;
	}

	ri.FS_FreeFile(buf);

	return mod;
}

/*
=====================
RE_BeginRegistration

Specifies the model that will be used as the world
=====================
*/
void
RE_BeginRegistration(char *model)
{
	char fullname[MAX_QPATH];
	const cvar_t *flushmap;

	Mod_Reallocate();

	registration_sequence++;
	r_oldviewcluster = -1; /* force markleafs */

	Com_sprintf(fullname, sizeof(fullname), "maps/%s.bsp", model);

	/* explicitly free the old map if different
	   this guarantees that mod_known[0] is the
	   world map */
	flushmap = ri.Cvar_Get("flushmap", "0", 0);

	if (strcmp(models_known[0].name, fullname) || flushmap->value)
	{
		Mod_Free(&models_known[0]);
	}

	r_worldmodel = Mod_ForName(fullname, NULL, true);
	if (r_worldmodel != models_known)
		ri.Sys_Error(ERR_DROP, "%s: Loaded a brush model after the world", __func__);

	r_viewcluster = -1;
}

struct model_s *
RE_RegisterModel(char *name)
{
	model_t *mod;

	mod = Mod_ForName(name, r_worldmodel, false);

	if (mod)
	{
		mod->registration_sequence = registration_sequence;

		/* register any images used by the models */
		if (mod->type == mod_brush)
		{
			int i;

			for (i = 0; i < mod->numtexinfo; i++)
			{
				mod->texinfo[i].image->registration_sequence =
					registration_sequence;
			}
		}
		else
		{
			/* numframes is unused for SP2 but lets set it also  */
			mod->numframes = Mod_ReLoadSkins((struct image_s **)mod->skins,
				(findimage_t)Vk_FindImage, mod->extradata, mod->type);
		}
	}

	return mod;
}

static qboolean
Mod_HasFreeSpace(void)
{
	int i, used;
	model_t *mod;

	used = 0;

	for (i=0, mod=models_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence == registration_sequence)
		{
			used ++;
		}
	}

	if (mod_max < used)
	{
		mod_max = used;
	}

	// should same size of free slots as currently used
	return (mod_loaded + mod_max) < models_known_max;
}

void
Mod_Modellist_f(void)
{
	int		i, total, used;
	model_t	*mod;
	qboolean	freeup;

	total = 0;
	used = 0;

	R_Printf(PRINT_ALL,"Loaded models:\n");
	for (i=0, mod=models_known ; i < mod_numknown ; i++, mod++)
	{
		char *in_use = "";

		if (mod->registration_sequence == registration_sequence)
		{
			in_use = "*";
			used ++;
		}

		if (!mod->name[0])
			continue;
		R_Printf(PRINT_ALL, "%8i : %s %s r: %.2f #%d\n",
			 mod->extradatasize, mod->name, in_use, mod->radius, mod->numsubmodels);
		total += mod->extradatasize;
	}
	R_Printf(PRINT_ALL, "Total resident: %i in %d models\n", total, mod_loaded);
	// update statistics
	freeup = Mod_HasFreeSpace();
	R_Printf(PRINT_ALL, "Used %d of %d models%s.\n", used, mod_max, freeup ? ", has free space" : "");
}

void
RE_EndRegistration(void)
{
	int i;
	model_t *mod;

	if (Mod_HasFreeSpace() && Vk_ImageHasFreeSpace())
	{
		// should be enough space for load next maps
		return;
	}

	for (i = 0, mod = models_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			continue;
		}

		if (mod->registration_sequence != registration_sequence)
		{
			/* don't need this model */
			Mod_Free(mod);
		}
	}

	Vk_FreeUnusedImages();
}
