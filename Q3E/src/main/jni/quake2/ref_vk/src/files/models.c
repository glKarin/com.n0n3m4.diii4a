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
 * The models file format
 *
 * =======================================================================
 */

#include "../common/header/ref_shared.h"

/*
=================
Mod_LoadSTvertList

load base s and t vertices (not used in gl version)
=================
*/
static void
Mod_LoadSTvertList (dmdl_t *pheader, dstvert_t *pinst)
{
	dstvert_t *poutst;
	int i;

	poutst = (dstvert_t *) ((byte *)pheader + pheader->ofs_st);

	for (i=0 ; i<pheader->num_st ; i++)
	{
		poutst[i].s = LittleShort (pinst[i].s);
		poutst[i].t = LittleShort (pinst[i].t);
	}
}

/*
=================
Mod_LoadCmdList

Load the glcmds
=================
*/
static void
Mod_LoadCmdList (const char *mod_name, dmdl_t *pheader, int *pincmd)
{
	int *poutcmd;
	int i;

	poutcmd = (int *)((char*)pheader + pheader->ofs_glcmds);
	for (i = 0; i < pheader->num_glcmds; i++)
	{
		poutcmd[i] = LittleLong (pincmd[i]);
	}

	if (poutcmd[pheader->num_glcmds-1] != 0)
	{
		R_Printf(PRINT_ALL, "%s: Entity %s has possible last element issues with %d verts.\n",
			__func__, mod_name, poutcmd[pheader->num_glcmds-1]);
	}
}

/*
=================
Mod_LoadFrames

Load the frames
=================
*/
static void
Mod_LoadFrames (dmdl_t *pheader, byte *src, vec3_t translate)
{
	int i;

	for (i=0 ; i<pheader->num_frames ; i++)
	{
		daliasframe_t		*pinframe, *poutframe;
		int j;

		pinframe = (daliasframe_t *) (src + i * pheader->framesize);
		poutframe = (daliasframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = LittleFloat (pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat (pinframe->translate[j]);
			poutframe->translate[j] += translate[j];
		}
		// verts are all 8 bit, so no swapping needed
		memcpy (poutframe->verts, pinframe->verts,
			pheader->num_xyz*sizeof(dtrivertx_t));
	}
}

/*
=================
Mod_LoadDTriangleList

Load triangle lists
=================
*/
static void
Mod_LoadDTriangleList (dmdl_t *pheader, dtriangle_t *pintri)
{
	dtriangle_t *pouttri;
	int i;

	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	for (i=0 ; i<pheader->num_tris ; i++)
	{
		int j;

		for (j=0 ; j<3 ; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort (pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort (pintri[i].index_st[j]);
		}
	}
}

/*
=================
Mod_LoadDTriangleList

Load DKM triangle lists
=================
*/
static void
Mod_LoadDkmTriangleList (dmdl_t *pheader, dkmtriangle_t *pintri)
{
	dtriangle_t *pouttri;
	int i;

	pouttri = (dtriangle_t *) ((char *)pheader + pheader->ofs_tris);

	for (i=0 ; i<pheader->num_tris ; i++)
	{
		int j;

		for (j=0 ; j<3 ; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort (pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort (pintri[i].index_st[j]);
		}
	}
}

/*
=================
Mod_LoadDKMCmdList

Load the DKM glcmds
=================
*/
static void
Mod_LoadDKMCmdList (const char *mod_name, dmdl_t *pheader, int *pincmd)
{
	int *poutcmd, *pendcmd;
	int i;

	poutcmd = (int *)((char*)pheader + pheader->ofs_glcmds);
	pendcmd = poutcmd + pheader->num_glcmds;
	/* read command count */
	i = LittleLong(*pincmd++);
	*poutcmd++ = i;

	while (i != 0)
	{
		if (i < 0)
		{
			i = -i;
		}

		/* skip unused surf_id and skin index */
		pincmd += 2;

		while (i)
		{
			poutcmd[0] = LittleLong (pincmd[1]);
			poutcmd[1] = LittleLong (pincmd[2]);
			poutcmd[2] = LittleLong (pincmd[0]);
			poutcmd += 3;
			pincmd += 3;
			i --;
		}

		/* read command count */
		i = LittleLong(*pincmd++);
		*poutcmd++ = i;

		if (pendcmd < poutcmd)
		{
			R_Printf(PRINT_ALL, "%s: Entity %s has possible broken glcmd.\n",
				__func__, mod_name);
			break;
		}
	}
	memset (poutcmd, 0, (pendcmd - poutcmd) * sizeof(int));
}

/*
=================
Mod_DkmLoadFrames

Load the Dkm v2 frames
=================
*/
static void
Mod_LoadDkmFrames (dmdl_t *pheader, const byte *src, size_t infamesize, vec3_t translate)
{
	int i;

	for (i=0 ; i<pheader->num_frames ; i++)
	{
		daliasframe_t	*pinframe, *poutframe;
		dtrivertx_t	*outverts;
		byte	*inverts;
		int j;

		pinframe = (daliasframe_t *) (src + i * infamesize);
		poutframe = (daliasframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = LittleFloat (pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat (pinframe->translate[j]);
			poutframe->translate[j] += translate[j];
		}

		poutframe->scale[0] *= 8;
		poutframe->scale[1] *= 4;
		poutframe->scale[2] *= 8;

		inverts = (byte *)&pinframe->verts;
		outverts = poutframe->verts;
		/* dkm vert version 2 has unalligned by int size struct */
		for(j=0; j < pheader->num_xyz; j++)
		{
			int xyz;

			xyz = LittleLong (*((int *)inverts));
			outverts[j].v[0] = ((xyz & 0xFFE00000) >> (21 + 3)) & 0xFF;
			outverts[j].v[1] = ((xyz & 0x1FF800) >> (11 + 2)) & 0xFF;
			outverts[j].v[2] = ((xyz & 0x7FF) >> 3) & 0xFF;
			inverts += sizeof(int);
			outverts[j].lightnormalindex = *inverts;
			inverts ++;
		}
	}
}

/*
=================
Mod_LoadMDL
=================
*/
static void *
Mod_LoadMDL (const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s **skins, findimage_t find_image,
	modtype_t *type)
{
	const mdl_header_t		*pinmodel;
	int		version;
	dmdl_t		*pheader;
	void	*extradata;

	/* local copy of all values */
	int skinwidth, skinheight, framesize;
	int num_skins, num_xyz, num_st, num_tris, num_glcmds, num_frames;
	int ofs_skins, ofs_st, ofs_tris, ofs_frames, ofs_glcmds, ofs_end;

	pinmodel = (mdl_header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MDL_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, version, MDL_VERSION);
		return NULL;
	}

	/* generate all offsets and sizes */
	num_skins = LittleLong (pinmodel->num_skins);
	skinwidth = LittleLong (pinmodel->skinwidth);
	skinheight = LittleLong (pinmodel->skinheight);
	num_xyz = LittleLong (pinmodel->num_xyz);
	num_st = num_xyz;
	num_tris = LittleLong (pinmodel->num_tris);
	num_glcmds = (
		(3 * num_tris) * sizeof(int) * 3 + /* 3 vert */
		(num_tris * sizeof(int)) + /* triangles count */
		sizeof(int) /* final zero */) / sizeof(int);
	num_frames = LittleLong (pinmodel->num_frames);
	framesize = sizeof(daliasframe_t) + sizeof (dtrivertx_t) * (num_xyz - 1);

	ofs_skins = sizeof(dmdl_t); // just skip header and go
	ofs_st = ofs_skins + num_skins * MAX_SKINNAME;
	ofs_tris = ofs_st + num_st * sizeof(dstvert_t);
	ofs_glcmds = ofs_tris + num_tris * sizeof(dtriangle_t);
	ofs_frames = ofs_glcmds + num_glcmds * sizeof(int);
	/* one less as single vertx in frame by default */
	ofs_end = ofs_frames + framesize * num_frames;

	/* validate */
	if (skinheight > MAX_LBM_HEIGHT)
	{
		R_Printf(PRINT_ALL, "%s: model %s has a skin taller than %d",
				__func__, mod_name, MAX_LBM_HEIGHT);
		return NULL;
	}

	if (skinwidth > MAX_LBM_HEIGHT)
	{
		R_Printf(PRINT_ALL, "%s: model %s has a skin wider than %d",
				__func__, mod_name, MAX_LBM_HEIGHT);
		return NULL;
	}

	if (num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices",
				__func__, mod_name);
		return NULL;
	}

	if (num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
				__func__, mod_name);
		return NULL;
	}

	if (num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles",
				__func__, mod_name);
		return NULL;
	}

	if (num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames",
				__func__, mod_name);
		return NULL;
	}

	if (modfilelen < ofs_end)
	{
		R_Printf(PRINT_ALL, "%s: model %s is too big.",
				__func__, mod_name);
		return NULL;
	}

	extradata = Hunk_Begin(ofs_end);
	pheader = Hunk_Alloc(ofs_end);

	/* copy back all values */
	pheader->ident = IDALIASHEADER;
	pheader->version = ALIAS_VERSION;
	pheader->skinwidth = skinwidth;
	pheader->skinheight = skinheight;
	pheader->framesize = framesize;

	pheader->num_skins = num_skins;
	pheader->num_xyz = num_xyz;
	pheader->num_st = num_st;
	pheader->num_tris = num_tris;
	pheader->num_glcmds = num_glcmds;
	pheader->num_frames = num_frames;

	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_end = ofs_end;

	{
		int i;
		const byte *curr_pos;

		mdl_triangle_t *triangles;
		mdl_texcoord_t *texcoords;

		curr_pos = (byte*)buffer + sizeof (mdl_header_t);

		// register all skins
		for (i = 0; i < num_skins; ++i)
		{
			char *out_pos;
			int skin_type;

			out_pos = (char*)pheader + sizeof(dmdl_t);
			snprintf(out_pos + MAX_SKINNAME * i, MAX_SKINNAME, "%s#%d.tga", mod_name, i);

			/* skip type / int */
			/* 0 = simple, !0 = group */
			/* this program can't read models composed of group frames! */
			skin_type = LittleLong (((int *)curr_pos)[0]);
			curr_pos += sizeof(int);
			if (skin_type)
			{
				R_Printf(PRINT_ALL, "%s: model %s has unsupported skin type %d",
						__func__, mod_name, skin_type);
				return NULL;
			}

			/* skip 8bit image */
			curr_pos += skinwidth * skinheight;
		}

		/* texcoordinates */
		{
			dstvert_t *poutst = (dstvert_t *) ((byte *)pheader + ofs_st);

			texcoords = (mdl_texcoord_t *)curr_pos;
			curr_pos += sizeof (mdl_texcoord_t) * num_st;

			for(i = 0; i < num_st; i++)
			{
				/* Compute texture coordinates */
				poutst[i].s = LittleLong (texcoords[i].s);
				poutst[i].t = LittleLong (texcoords[i].t);

				if (texcoords[i].onseam)
				{
					poutst[i].s += skinwidth * 0.5f; /* Backface */
				}

				/* Scale s and t to range from 0.0 to 1.0 */
				poutst[i].s = (poutst[i].s + 0.5) / skinwidth;
				poutst[i].t = (poutst[i].t + 0.5) / skinheight;
			}
		}

		/* triangles */
		{
			dtriangle_t *pouttri = (dtriangle_t *) ((byte *)pheader + ofs_tris);

			triangles = (mdl_triangle_t *) curr_pos;
			curr_pos += sizeof (mdl_triangle_t) * num_tris;

			for (i=0 ; i<num_tris ; i++)
			{
				int j;

				for (j=0 ; j<3 ; j++)
				{
					pouttri[i].index_xyz[j] = LittleLong (triangles[i].vertex[j]);
					pouttri[i].index_st[j] = pouttri[i].index_xyz[j];
				}
			}
		}

		{
			int *glcmds = (int *) ((byte *)pheader + ofs_glcmds);

			/* commands */
			int j, *curr_com = glcmds;

			/* Draw each triangle */
			for (i = 0; i < num_tris; ++i)
			{
				*curr_com = 3;
				curr_com++;

				/* Draw each vertex */
				for (j = 0; j < 3; ++j)
				{
					float s,t;
					int index;

					index = triangles[i].vertex[j];

					/* Compute texture coordinates */
					s = LittleLong (texcoords[index].s);
					t = LittleLong (texcoords[index].t);

					if (!triangles[i].facesfront &&
						texcoords[index].onseam)
					{
						s += skinwidth * 0.5f; /* Backface */
					}

					/* Scale s and t to range from 0.0 to 1.0 */
					s = (s + 0.5) / skinwidth;
					t = (t + 0.5) / skinheight;

					memcpy(curr_com, &s, sizeof(s));
					curr_com++;
					memcpy(curr_com, &t, sizeof(t));
					curr_com++;
					memcpy(curr_com, &index, sizeof(index));
					curr_com++;
				}
			}

			*curr_com = 0;
			curr_com++;
		}

		/* register all frames */
		for (i = 0; i < num_frames; ++i)
		{
			daliasframe_t *frame;
			int frame_type;

			frame = (daliasframe_t *) ((byte *)pheader + ofs_frames + i * framesize);
			frame->scale[0] = LittleFloat (pinmodel->scale[0]);
			frame->scale[1] = LittleFloat (pinmodel->scale[1]);
			frame->scale[2] = LittleFloat (pinmodel->scale[2]);

			frame->translate[0] = LittleFloat (pinmodel->translate[0]);
			frame->translate[1] = LittleFloat (pinmodel->translate[1]);
			frame->translate[2] = LittleFloat (pinmodel->translate[2]);

			/* Read frame data */
			/* skip type / int */
			/* 0 = simple, !0 = group */
			/* this program can't read models composed of group frames! */
			frame_type = LittleLong (((int *)curr_pos)[0]);
			curr_pos += sizeof (frame_type);

			if (frame_type)
			{
				R_Printf(PRINT_ALL, "%s: model %s has unsupported frame type %d",
						__func__, mod_name, frame_type);
				return NULL;
			}
			/* skip bboxmin, bouding box min */
			curr_pos += sizeof(dtrivertx_t);
			/* skip bboxmax, bouding box max */
			curr_pos += sizeof(dtrivertx_t);

			memcpy(&frame->name, curr_pos, sizeof (char) * 16);
			curr_pos += sizeof (char) * 16;

			memcpy(&frame->verts[0], curr_pos,
				sizeof (dtrivertx_t) * num_xyz);
			curr_pos += sizeof (dtrivertx_t) * num_xyz;
		}
	}

	*type = mod_alias;

	mins[0] = -32;
	mins[1] = -32;
	mins[2] = -32;
	maxs[0] = 32;
	maxs[1] = 32;
	maxs[2] = 32;

	return extradata;
}

/*
=================
Mod_LoadMD2
=================
*/
static void *
Mod_LoadMD2 (const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s **skins, findimage_t find_image,
	modtype_t *type)
{
	vec3_t		translate = {0, 0, 0};
	dmdl_t		*pinmodel, *pheader;
	dtriangle_t	*pintri;
	dstvert_t	*pinst;
	int		*pincmd;
	void	*extradata;
	int		version;
	int		ofs_end;
	int		i;

	pinmodel = (dmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, version, ALIAS_VERSION);
		return NULL;
	}

	ofs_end = LittleLong(pinmodel->ofs_end);
	if (ofs_end < 0 || ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
				__func__, mod_name, modfilelen, ofs_end);
		return NULL;
	}

	extradata = Hunk_Begin(modfilelen);
	pheader = Hunk_Alloc(ofs_end);

	// byte swap the header fields and sanity check
	for (i=0 ; i<sizeof(dmdl_t)/sizeof(int) ; i++)
		((int *)pheader)[i] = LittleLong (((int *)buffer)[i]);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
	{
		R_Printf(PRINT_ALL, "%s: model %s has a skin taller than %d",
				__func__, mod_name, MAX_LBM_HEIGHT);
		return NULL;
	}

	if (pheader->num_xyz <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_xyz > MAX_VERTS)
	{
		R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_st <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no st vertices",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_tris <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no triangles",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_frames <= 0)
	{
		R_Printf(PRINT_ALL, "%s: model %s has no frames",
				__func__, mod_name);
		return NULL;
	}

	if (pheader->num_skins > MAX_MD2SKINS)
	{
		R_Printf(PRINT_ALL, "%s has too many skins (%i > %i), "
				"extra sprites will be ignored\n",
				mod_name, pheader->num_skins, MAX_MD2SKINS);
		pheader->num_skins = MAX_MD2SKINS;
	}

	//
	// load base s and t vertices (not used in gl version)
	//
	pinst = (dstvert_t *) ((byte *)pinmodel + pheader->ofs_st);
	Mod_LoadSTvertList (pheader, pinst);

	//
	// load triangle lists
	//
	pintri = (dtriangle_t *) ((byte *)pinmodel + pheader->ofs_tris);
	Mod_LoadDTriangleList (pheader, pintri);

	//
	// load the frames
	//
	Mod_LoadFrames (pheader, (byte *)pinmodel + pheader->ofs_frames, translate);

	//
	// load the glcmds
	//
	pincmd = (int *) ((byte *)pinmodel + pheader->ofs_glcmds);
	Mod_LoadCmdList (mod_name, pheader, pincmd);

	// register all skins
	memcpy ((char *)pheader + pheader->ofs_skins, (char *)pinmodel + pheader->ofs_skins,
		pheader->num_skins*MAX_SKINNAME);

	// Load in our skins.
	for (i=0; i < pheader->num_skins; i++)
	{
		skins[i] = find_image((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME,
			it_skin);
	}

	*type = mod_alias;

	mins[0] = -32;
	mins[1] = -32;
	mins[2] = -32;
	maxs[0] = 32;
	maxs[1] = 32;
	maxs[2] = 32;

	return extradata;
}


/*
=============
Mod_LoadFlexModel
=============
*/
static void *
Mod_LoadFlexModel(const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s **skins, findimage_t find_image,
	modtype_t *type)
{
	char *src = (char *)buffer;
	int version, size, i;
	void *extradata = NULL;
	dmdl_t *pheader = NULL;

	while (modfilelen > 0)
	{
		char blockname[32];

		memcpy(blockname, src, sizeof(blockname));

		src += sizeof(blockname);
		version = *(int*)src;
		src += sizeof(version);
		size = *(int*)src;
		src += sizeof(size);
		modfilelen = modfilelen - sizeof(blockname) - sizeof(version) - sizeof(size);

		if (Q_strncasecmp(blockname, "header", sizeof(blockname)) == 0)
		{
			dmdl_t dmdlheader;
			fmheader_t *header = (fmheader_t *)src;

			if (sizeof(fmheader_t) > size)
			{
				R_Printf(PRINT_ALL, "%s: Too short header", __func__);
				return NULL;
			}

			if (version != 2)
			{
				R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
					__func__, blockname, version);
				return NULL;
			}

			/* copy back all values */
			dmdlheader.ident = IDALIASHEADER;
			dmdlheader.version = ALIAS_VERSION;
			dmdlheader.skinwidth = LittleLong (header->skinwidth);
			dmdlheader.skinheight = LittleLong (header->skinheight);
			dmdlheader.framesize = LittleLong (header->framesize);

			dmdlheader.num_skins = LittleLong (header->num_skins);
			dmdlheader.num_xyz = LittleLong (header->num_xyz);
			dmdlheader.num_st = LittleLong (header->num_st);
			dmdlheader.num_tris = LittleLong (header->num_tris);
			dmdlheader.num_glcmds = LittleLong (header->num_glcmds);
			dmdlheader.num_frames = LittleLong (header->num_frames);

			// just skip header and meshes
			dmdlheader.ofs_skins = sizeof(dmdl_t) + sizeof(short) * 2 * LittleLong (header->num_mesh_nodes);
			dmdlheader.ofs_st = dmdlheader.ofs_skins + dmdlheader.num_skins * MAX_SKINNAME;
			dmdlheader.ofs_tris = dmdlheader.ofs_st + dmdlheader.num_st * sizeof(dstvert_t);
			dmdlheader.ofs_frames = dmdlheader.ofs_tris + dmdlheader.num_tris * sizeof(dtriangle_t);
			dmdlheader.ofs_glcmds = dmdlheader.ofs_frames + dmdlheader.num_frames * dmdlheader.framesize;
			dmdlheader.ofs_end = dmdlheader.ofs_glcmds + dmdlheader.num_glcmds * sizeof(int);

			if (dmdlheader.skinheight > MAX_LBM_HEIGHT)
			{
				R_Printf(PRINT_ALL, "%s: model %s has a skin taller than %d",
						__func__, mod_name, MAX_LBM_HEIGHT);
				return NULL;
			}

			if (dmdlheader.num_xyz <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no vertices",
						__func__, mod_name);
				return NULL;
			}

			if (dmdlheader.num_xyz > MAX_VERTS)
			{
				R_Printf(PRINT_ALL, "%s: model %s has too many vertices",
						__func__, mod_name);
				return NULL;
			}

			if (dmdlheader.num_st <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no st vertices",
						__func__, mod_name);
				return NULL;
			}

			if (dmdlheader.num_tris <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no triangles",
						__func__, mod_name);
				return NULL;
			}

			if (dmdlheader.num_frames <= 0)
			{
				R_Printf(PRINT_ALL, "%s: model %s has no frames",
						__func__, mod_name);
				return NULL;
			}

			extradata = Hunk_Begin(dmdlheader.ofs_end);
			pheader = Hunk_Alloc(dmdlheader.ofs_end);

			memcpy(pheader, &dmdlheader, sizeof(dmdl_t));
		}
		else {
			if (!pheader)
			{
				R_Printf(PRINT_ALL, "%s: %s has broken header.",
					__func__, mod_name);
				return NULL;
			}
			else if (Q_strncasecmp(blockname, "skin", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_skins * MAX_SKINNAME))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}
				memcpy((char*) pheader + pheader->ofs_skins, src, size);
			}
			else if (Q_strncasecmp(blockname, "st coord", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_st * sizeof(dstvert_t)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadSTvertList (pheader, (dstvert_t *)src);
			}
			else if (Q_strncasecmp(blockname, "tris", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_tris * sizeof(dtriangle_t)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadDTriangleList (pheader, (dtriangle_t *) src);
			}
			else if (Q_strncasecmp(blockname, "frames", sizeof(blockname)) == 0)
			{
				vec3_t translate = {0, 0, 0};

				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_frames * pheader->framesize))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadFrames (pheader, (byte *)src, translate);
			}
			else if (Q_strncasecmp(blockname, "glcmds", sizeof(blockname)) == 0)
			{
				if (version != 1)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				if (size != (pheader->num_glcmds * sizeof(int)))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				Mod_LoadCmdList (mod_name, pheader, (int *)src);
			}
			else if (Q_strncasecmp(blockname, "mesh nodes", sizeof(blockname)) == 0)
			{
				int num_mesh_nodes;

				num_mesh_nodes = (pheader->ofs_skins - sizeof(dmdl_t)) / sizeof(short) / 2;

				if (version != 3)
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s version %d",
						__func__, blockname, version);
					return NULL;
				}
				/* 516 mesh node size */
				if (size != (num_mesh_nodes * 516))
				{
					R_Printf(PRINT_ALL, "%s: Invalid %s size",
						__func__, blockname);
					return NULL;
				}

				if (num_mesh_nodes > 0)
				{
					short *mesh_nodes;
					char *in_mesh = src;
					int i;

					mesh_nodes = (short *)((char*)pheader + sizeof(dmdl_t));
					for (i = 0; i < num_mesh_nodes; i++)
					{
						/* 256 bytes of tri data */
						/* 256 bytes of vert data */
						/* 2 bytes of start */
						/* 2 bytes of number commands */
						in_mesh += 512;
						mesh_nodes[i * 2] = LittleShort(*(short *)in_mesh);
						in_mesh += 2;
						mesh_nodes[i * 2 + 1] = LittleShort(*(short *)in_mesh);
						in_mesh += 2;
					}
				}
			}
			else if (Q_strncasecmp(blockname, "normals", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "short frames", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "comp data", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "skeleton", sizeof(blockname)) == 0 ||
					 Q_strncasecmp(blockname, "references", sizeof(blockname)) == 0)
			{
				/* skipped block */
			}
			else
			{
				R_Printf(PRINT_ALL, "%s: %s Unknown block %s\n",
					__func__, mod_name, blockname);
				return NULL;
			}
		}
		modfilelen -= size;
		src += size;
	}

	if (pheader->num_skins > MAX_MD2SKINS)
	{
		R_Printf(PRINT_ALL, "%s has too many skins (%i > %i), "
				"extra skins will be ignored\n",
				mod_name, pheader->num_skins, MAX_MD2SKINS);
		pheader->num_skins = MAX_MD2SKINS;
	}

	// Load in our skins.
	for (i=0; i < pheader->num_skins; i++)
	{
		skins[i] = find_image((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME,
			it_skin);
	}

	*type = mod_alias;

	mins[0] = -32;
	mins[1] = -32;
	mins[2] = -32;
	maxs[0] = 32;
	maxs[1] = 32;
	maxs[2] = 32;

	return extradata;
}

static void *
Mod_LoadDKMModel(const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s **skins, findimage_t find_image,
	modtype_t *type)
{
	dmdl_t dmdlheader, *pheader = NULL;
	dkm_header_t header = {0};
	void *extradata = NULL;
	int i;

	if (sizeof(dkm_header_t) > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small",
				__func__, mod_name, modfilelen);
	}

	// byte swap the header fields and sanity check
	for (i=0 ; i<sizeof(dkm_header_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong (((int *)buffer)[i]);

	if (header.ident != DKMHEADER)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong ident (%i should be %i)",
				__func__, mod_name, header.ident, DKMHEADER);
		return NULL;
	}

	if (header.version != DKM1_VERSION && header.version != DKM2_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)",
				__func__, mod_name, header.version, DKM2_VERSION);
		return NULL;
	}

	if (header.ofs_end < 0 || header.ofs_end > modfilelen)
	{
		R_Printf(PRINT_ALL, "%s: model %s file size(%d) too small, should be %d",
				__func__, mod_name, modfilelen, header.ofs_end);
		return NULL;
	}

	/* copy back all values */
	dmdlheader.ident = IDALIASHEADER;
	dmdlheader.version = ALIAS_VERSION;
	dmdlheader.skinwidth = 256;
	dmdlheader.skinheight = 256;
	if (header.version != DKM2_VERSION)
	{
		/* has same frame structure */
		dmdlheader.framesize = header.framesize;
	}
	else
	{
		dmdlheader.framesize = sizeof(daliasframe_t) - sizeof(dtrivertx_t);
		dmdlheader.framesize += header.num_xyz * sizeof(dtrivertx_t);
	}

	dmdlheader.num_skins = header.num_skins;
	dmdlheader.num_xyz = header.num_xyz;
	dmdlheader.num_st = header.num_st;
	dmdlheader.num_tris = header.num_tris;
	dmdlheader.num_glcmds = header.num_glcmds;
	dmdlheader.num_frames = header.num_frames;

	/* just skip header */
	dmdlheader.ofs_skins = sizeof(dmdl_t);
	dmdlheader.ofs_st = dmdlheader.ofs_skins + dmdlheader.num_skins * MAX_SKINNAME;
	dmdlheader.ofs_tris = dmdlheader.ofs_st + dmdlheader.num_st * sizeof(dstvert_t);
	dmdlheader.ofs_frames = dmdlheader.ofs_tris + dmdlheader.num_tris * sizeof(dtriangle_t);
	dmdlheader.ofs_glcmds = dmdlheader.ofs_frames + dmdlheader.num_frames * dmdlheader.framesize;
	dmdlheader.ofs_end = dmdlheader.ofs_glcmds + dmdlheader.num_glcmds * sizeof(int);

	extradata = Hunk_Begin(dmdlheader.ofs_end);
	pheader = Hunk_Alloc(dmdlheader.ofs_end);
	memcpy(pheader, &dmdlheader, sizeof(dmdl_t));

	memcpy ((byte*)pheader + pheader->ofs_skins, (byte *)buffer + header.ofs_skins,
		pheader->num_skins * MAX_SKINNAME);
	Mod_LoadSTvertList (pheader,
		(dstvert_t *)((byte *)buffer + header.ofs_st));
	Mod_LoadDKMCmdList (mod_name, pheader,
		(int *)((byte *)buffer + header.ofs_glcmds));
	if (header.version == DKM1_VERSION)
	{
		Mod_LoadFrames (pheader, (byte *)buffer + header.ofs_frames,
			header.translate);
	}
	else
	{
		Mod_LoadDkmFrames (pheader, (byte *)buffer + header.ofs_frames,
			header.framesize, header.translate);
	}

	Mod_LoadDkmTriangleList (pheader,
		(dkmtriangle_t *)((byte *)buffer + header.ofs_tris));

	if (pheader->num_skins > MAX_MD2SKINS)
	{
		R_Printf(PRINT_ALL, "%s has too many skins (%i > %i), "
				"extra skins will be ignored\n",
				mod_name, pheader->num_skins, MAX_MD2SKINS);
		pheader->num_skins = MAX_MD2SKINS;
	}

	// Load in our skins.
	for (i=0; i < pheader->num_skins; i++)
	{
		skins[i] = find_image((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME,
			it_skin);
	}

	*type = mod_alias;

	mins[0] = -32;
	mins[1] = -32;
	mins[2] = -32;
	maxs[0] = 32;
	maxs[1] = 32;
	maxs[2] = 32;

	return extradata;
}

/*
=================
Mod_LoadAliasModel
=================
*/
void *
Mod_LoadAliasModel (const char *mod_name, const void *buffer, int modfilelen,
	vec3_t mins, vec3_t maxs, struct image_s **skins, findimage_t find_image,
	modtype_t *type)
{
	switch (LittleLong(*(unsigned *)buffer))
	{
		case DKMHEADER:
			return Mod_LoadDKMModel(mod_name, buffer, modfilelen, mins, maxs, skins,
				find_image, type);

		case RAVENFMHEADER:
			return Mod_LoadFlexModel(mod_name, buffer, modfilelen, mins, maxs, skins,
				find_image, type);

		case IDALIASHEADER:
			return Mod_LoadMD2(mod_name, buffer, modfilelen, mins, maxs, skins,
				find_image, type);

		case IDMDLHEADER:
			return Mod_LoadMDL(mod_name, buffer, modfilelen, mins, maxs, skins,
				find_image, type);
	}

	return NULL;
}


/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

/*
=================
Mod_LoadSP2

support for .sp2 sprites
=================
*/
void *
Mod_LoadSP2 (const char *mod_name, const void *buffer, int modfilelen,
	struct image_s **skins, findimage_t find_image, modtype_t *type)
{
	dsprite_t *sprin, *sprout;
	void	*extradata;
	int i;

	sprin = (dsprite_t *)buffer;
	extradata = Hunk_Begin(modfilelen);
	sprout = Hunk_Alloc(modfilelen);

	sprout->ident = LittleLong(sprin->ident);
	sprout->version = LittleLong(sprin->version);
	sprout->numframes = LittleLong(sprin->numframes);

	if (sprout->version != SPRITE_VERSION)
	{
		R_Printf(PRINT_ALL, "%s has wrong version number (%i should be %i)",
				mod_name, sprout->version, SPRITE_VERSION);
		return NULL;
	}

	if (sprout->numframes > MAX_MD2SKINS)
	{
		R_Printf(PRINT_ALL, "%s has too many frames (%i > %i), "
				"extra frames will be ignored\n",
				mod_name, sprout->numframes, MAX_MD2SKINS);
		sprout->numframes = MAX_MD2SKINS;
	}

	/* byte swap everything */
	for (i = 0; i < sprout->numframes; i++)
	{
		sprout->frames[i].width = LittleLong(sprin->frames[i].width);
		sprout->frames[i].height = LittleLong(sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong(sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong(sprin->frames[i].origin_y);
		memcpy(sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);

		skins[i] = find_image((char *)sprout->frames[i].name, it_sprite);
		if (!skins[i])
		{
			/* heretic2 sprites have no "sprites/" prefix */
			snprintf(sprout->frames[i].name, MAX_SKINNAME,
				"sprites/%s", sprin->frames[i].name);
			skins[i] = find_image(sprout->frames[i].name, it_sprite);
		}
	}

	*type = mod_sprite;

	return extradata;
}

/*
=================
Mod_LoadFile
=================
*/
int
Mod_LoadFile(char *name, void **buffer)
{
	const char* ext;

	*buffer = NULL;

	if (!name)
	{
		return -1;
	}

	ext = COM_FileExtension(name);
	if(!ext[0])
	{
		/* file has no extension */
		return -1;
	}

	if (!strcmp(ext, "fm") ||
		!strcmp(ext, "dkm") ||
		!strcmp(ext, "md2") ||
		!strcmp(ext, "mdl"))
	{
		char namewe[256], newname[256];
		int filesize, len;

		len = strlen(name);
		if (len < 5)
		{
			return -1;
		}

		/* Remove the extension */
		size_t tlen = len - (strlen(ext) + 1);
		memset(namewe, 0, 256);
		memcpy(namewe, name, tlen);

		/* Check Heretic2 model */
		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcat(newname, ".fm", sizeof(newname));
		filesize = ri.FS_LoadFile (newname, buffer);
		if (filesize > 0)
		{
			return filesize;
		}

		/* Check Quake 2 model */
		Q_strlcpy(newname + tlen, ".md2", sizeof(newname));
		filesize = ri.FS_LoadFile (newname, buffer);
		if (filesize > 0)
		{
			return filesize;
		}

		/* Check Daikatana model */
		Q_strlcpy(newname + tlen, ".dkm", sizeof(newname));
		filesize = ri.FS_LoadFile (newname, buffer);
		if (filesize > 0)
		{
			return filesize;
		}

		/* Check Quake model */
		Q_strlcpy(newname + tlen, ".mdl", sizeof(newname));
		filesize = ri.FS_LoadFile (newname, buffer);
		if (filesize > 0)
		{
			return filesize;
		}
	}
	return ri.FS_LoadFile (name, buffer);
}

/*
=================
Mod_ReLoad

Reload images in SP2/MD2 (mark registration_sequence)
=================
*/
int
Mod_ReLoadSkins(struct image_s **skins, findimage_t find_image, void *extradata,
	modtype_t type)
{
	if (type == mod_sprite)
	{
		dsprite_t	*sprout;
		int	i;

		sprout = (dsprite_t *)extradata;
		for (i=0; i < sprout->numframes; i++)
		{
			skins[i] = find_image(sprout->frames[i].name, it_sprite);
		}
		return sprout->numframes;
	}
	else if (type == mod_alias)
	{
		dmdl_t *pheader;
		int	i;

		pheader = (dmdl_t *)extradata;
		for (i=0; i < pheader->num_skins; i++)
		{
			skins[i] = find_image ((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME, it_skin);
		}
		return  pheader->num_frames;
	}
	/* Unknow format, no images associated with it */
	return 0;
}

/*
=================
Mod_SetParent
=================
*/
static void
Mod_SetParent(mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != CONTENTS_NODE)
	{
		return;
	}

	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_NumberLeafs
=================
*/
static void
Mod_NumberLeafs(mleaf_t *leafs, mnode_t *node, int *r_leaftovis, int *r_vistoleaf,
	int *numvisleafs)
{
	if (node->contents != CONTENTS_NODE)
	{
		mleaf_t *leaf;
		int leafnum;

		leaf = (mleaf_t *)node;
		leafnum = leaf - leafs;
		if (leaf->contents & CONTENTS_SOLID)
		{
			return;
		}

		r_leaftovis[leafnum] = *numvisleafs;
		r_vistoleaf[*numvisleafs] = leafnum;
		(*numvisleafs) ++;
		return;
	}

	Mod_NumberLeafs(leafs, node->children[0], r_leaftovis, r_vistoleaf,
		numvisleafs);
	Mod_NumberLeafs(leafs, node->children[1], r_leaftovis, r_vistoleaf,
		numvisleafs);
}

/*
=================
Mod_LoadNodes
=================
*/
void
Mod_LoadNodes(const char *name, cplane_t *planes, int numplanes, mleaf_t *leafs,
	int numleafs, mnode_t **nodes, int *numnodes, const byte *mod_base,
	const lump_t *l)
{
	int	r_leaftovis[MAX_MAP_LEAFS], r_vistoleaf[MAX_MAP_LEAFS];
	int	i, count, numvisleafs;
	dnode_t	*in;
	mnode_t	*out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	*nodes = out;
	*numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		int j, planenum;

		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		planenum = LittleLong(in->planenum);
		if (planenum  < 0 || planenum >= numplanes)
		{
			ri.Sys_Error(ERR_DROP, "%s: Incorrect %d < %d planenum.",
					__func__, planenum, numplanes);
		}
		out->plane = planes + planenum;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);
		out->contents = CONTENTS_NODE; /* differentiate from leafs */

		for (j = 0; j < 2; j++)
		{
			int leafnum;

			leafnum = LittleLong(in->children[j]);

			if (leafnum >= 0)
			{
				if (leafnum  < 0 || leafnum >= *numnodes)
				{
					ri.Sys_Error(ERR_DROP, "%s: Incorrect %d nodenum as leaf.",
							__func__, leafnum);
				}

				out->children[j] = *nodes + leafnum;
			}
			else
			{
				leafnum = -1 - leafnum;
				if (leafnum  < 0 || leafnum >= numleafs)
				{
					ri.Sys_Error(ERR_DROP, "%s: Incorrect %d leafnum.",
							__func__, leafnum);
				}

				out->children[j] = (mnode_t *)(leafs + leafnum);
			}
		}
	}

	Mod_SetParent(*nodes, NULL); /* sets nodes and leafs */

	numvisleafs = 0;
	Mod_NumberLeafs (leafs, *nodes, r_leaftovis, r_vistoleaf, &numvisleafs);
}

/*
=================
Mod_LoadVisibility
=================
*/
void
Mod_LoadVisibility (dvis_t **vis, const byte *mod_base, const lump_t *l)
{
	dvis_t	*out;
	int	i;

	if (!l->filelen)
	{
		*vis = NULL;
		return;
	}

	out = Hunk_Alloc(l->filelen);
	*vis = out;
	memcpy(out, mod_base + l->fileofs, l->filelen);

	out->numclusters = LittleLong(out->numclusters);

	for (i = 0; i < out->numclusters; i++)
	{
		out->bitofs[i][0] = LittleLong(out->bitofs[i][0]);
		out->bitofs[i][1] = LittleLong(out->bitofs[i][1]);
	}
}

/*
=================
Mod_LoadVertexes

extra for skybox
=================
*/
void
Mod_LoadVertexes(const char *name, mvertex_t **vertexes, int *numvertexes,
	const byte *mod_base, const lump_t *l, int extra)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int	i, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + extra)*sizeof(*out));

	/*
	 * FIXME: Recheck with soft render
	 * Fix for the problem where the games dumped core
	 * when changing levels.
	 */
	memset(out, 0, (count + extra) * sizeof(*out));

	*vertexes = out;
	*numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void
Mod_LoadLighting(byte **lightdata, const byte *mod_base, const lump_t *l)
{
	int	size;

	if (!l->filelen)
	{
		*lightdata = NULL;
		return;
	}

	size = l->filelen;
	*lightdata = Hunk_Alloc(size);
	memcpy(*lightdata, mod_base + l->fileofs, size);
}

/*
=================
Mod_LoadTexinfo

extra for skybox in soft render
=================
*/
void
Mod_LoadTexinfo(const char *name, mtexinfo_t **texinfo, int *numtexinfo,
	const byte *mod_base, const lump_t *l, findimage_t find_image,
	struct image_s *notexture, int extra)
{
	texinfo_t *in;
	mtexinfo_t *out, *step;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + extra)*sizeof(*out));

	*texinfo = out;
	*numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		struct image_s *image;
		int j, next;

		for (j = 0; j < 4; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		out->flags = LittleLong (in->flags);

		next = LittleLong (in->nexttexinfo);
		if (next > 0)
		{
			out->next = *texinfo + next;
		}
		else
		{
			/*
			 * Fix for the problem where the game
			 * domed core when loading a new level.
			 */
			out->next = NULL;
		}

		image = GetTexImage(in->texture, find_image);
		if (!image)
		{
			R_Printf(PRINT_ALL, "%s: Couldn't load %s\n",
				__func__, in->texture);
			image = notexture;
		}

		out->image = image;
	}

	// count animation frames
	for (i=0 ; i<count ; i++)
	{
		out = (*texinfo) + i;
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
		{
			out->numframes++;
		}
	}
}

/*
=================
Mod_LoadEdges

extra is used for skybox, which adds 6 surfaces
=================
*/
void
Mod_LoadEdges(const char *name, medge_t **edges, int *numedges,
	const byte *mod_base, const lump_t *l, int extra)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + extra) * sizeof(*out));

	*edges = out;
	*numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadPlanes

extra is used for skybox, which adds 6 surfaces
=================
*/
void
Mod_LoadPlanes(const char *name, cplane_t **planes, int *numplanes,
	const byte *mod_base, const lump_t *l, int extra)
{
	int i;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	// FIXME: why double of count
	out = Hunk_Alloc((count * 2 + extra) * sizeof(*out));

	*planes = out;
	*numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		int bits, j;

		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void
Mod_LoadSurfedges(const char *name, int **surfedges, int *numsurfedges,
	const byte *mod_base, const lump_t *l, int extra)
{
	int		i, count;
	int		*in, *out;

	in = (void *)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
	{
		ri.Sys_Error(ERR_DROP, "%s: funny lump size in %s",
				__func__, name);
	}

	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc((count + extra)*sizeof(*out));	// extra for skybox

	*surfedges = out;
	*numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}

/*
=================
Mod_LoadSurfedges

calculate the size that Hunk_Alloc(), called by Mod_Load*() from Mod_LoadBrushModel(),
will use (=> includes its padding), so we'll know how big the hunk needs to be
extra is used for skybox, which adds 6 surfaces
=================
*/
int
Mod_CalcLumpHunkSize(const lump_t *l, int inSize, int outSize, int extra)
{
	if (l->filelen % inSize)
	{
		// Mod_Load*() will error out on this because of "funny size"
		// don't error out here because in Mod_Load*() it can print the functionname
		// (=> tells us what kind of lump) before shutting down the game
		return 0;
	}

	int count = l->filelen / inSize + extra;
	int size = count * outSize;

	// round to cacheline, like Hunk_Alloc() does
	size = (size + 31) & ~31;
	return size;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *
Mod_PointInLeaf(const vec3_t p, mnode_t *node)
{
	if (!node)
	{
		ri.Sys_Error(ERR_DROP, "%s: bad node.", __func__);
		return NULL;
	}

	while (1)
	{
		float d;
		cplane_t *plane;

		if (node->contents != CONTENTS_NODE)
		{
			return (mleaf_t *)node;
		}

		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;

		if (d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return NULL; /* never reached */
}
