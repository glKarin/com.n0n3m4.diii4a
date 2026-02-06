#include "quakedef.h"

#ifdef HALFLIFEMODELS

#include "shader.h"
#include "com_mesh.h"
/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	Half-Life Model Renderer (Experimental) Copyright (C) 2001 James 'Ender' Brown [ender@quakesrc.org] This program is
	free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details. You should have received a copy of the GNU General Public License along with this program; if not, write
	to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. fromquake.h -

	render.c - apart from calculations (mostly range checking or value conversion code is a mix of standard Quake 1
	meshing, and vertex deforms. The rendering loop uses standard Quake 1 drawing, after SetupBones deforms the vertex.
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Note: this code has since been greatly modified to fix skin, submodels, hitboxes, attachments, etc.



  Also, please note that it won't do all hl models....
  Nor will it work 100%
 */

qboolean HLMDL_Trace		(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p1, const vec3_t p2, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int against, struct trace_s *trace);
unsigned int HLMDL_Contents	(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs);

void QuaternionGLMatrix(float x, float y, float z, float w, vec4_t *GLM)
{
	GLM[0][0] = 1 - 2 * y * y - 2 * z * z;
	GLM[1][0] = 2 * x * y + 2 * w * z;
	GLM[2][0] = 2 * x * z - 2 * w * y;
	GLM[0][1] = 2 * x * y - 2 * w * z;
	GLM[1][1] = 1 - 2 * x * x - 2 * z * z;
	GLM[2][1] = 2 * y * z + 2 * w * x;
	GLM[0][2] = 2 * x * z + 2 * w * y;
	GLM[1][2] = 2 * y * z - 2 * w * x;
	GLM[2][2] = 1 - 2 * x * x - 2 * y * y;
}

/*
 =======================================================================================================================
	QuaternionGLAngle - Convert a GL angle to a quaternion matrix
 =======================================================================================================================
 */
void QuaternionGLAngle(const vec3_t angles, vec4_t quaternion)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	yaw = angles[2] * 0.5;
	float	pitch = angles[1] * 0.5;
	float	roll = angles[0] * 0.5;
	float	siny = sin(yaw);
	float	cosy = cos(yaw);
	float	sinp = sin(pitch);
	float	cosp = cos(pitch);
	float	sinr = sin(roll);
	float	cosr = cos(roll);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	quaternion[0] = sinr * cosp * cosy - cosr * sinp * siny;
	quaternion[1] = cosr * sinp * cosy + sinr * cosp * siny;
	quaternion[2] = cosr * cosp * siny - sinr * sinp * cosy;
	quaternion[3] = cosr * cosp * cosy + sinr * sinp * siny;
}

matrix3x4 transform_matrix[MAX_BONES];	/* Vertex transformation matrix */

#ifndef SERVERONLY
void GL_Draw_HL_AliasFrame(short *order, vec3_t *transformed, float tex_w, float tex_h);

struct hlvremaps
{
	unsigned short vertidx;
	unsigned short normalidx;
	unsigned short scoord;
	unsigned short tcoord;
};
static index_t HLMDL_DeDupe(unsigned short *order, struct hlvremaps *rem, size_t *count, size_t first, size_t max)
{
	size_t i;
	for (i = *count; i-- > first;)
	{
		if (rem[i].vertidx == order[0] && rem[i].normalidx == order[1] && rem[i].scoord == order[2] && rem[i].tcoord == order[3])
			return i;
	}
	i = *count;
	if (i < max)
	{
		rem[i].vertidx = order[0];
		rem[i].normalidx = order[1];
		rem[i].scoord = order[2];
		rem[i].tcoord = order[3];
	}
	*count += 1;
	return i;
}

//parse the vertex info, pull out what we can
static void HLMDL_PrepareVerticies (model_t *mod, hlmodel_t *model)
{
	struct hlvremaps *uvert;
	size_t uvertcount=0, uvertstart;
	unsigned short count;
	int i;
	size_t idx = 0, m, maxidx=65536*3;
	size_t maxverts = 65536;

	index_t *index;
	mesh_t *mesh, *submesh;

	int body;

	uvert = malloc(sizeof(*uvert)*maxverts);
	index = malloc(sizeof(byte_vec4_t)*maxidx);

	model->numgeomsets = model->header->numbodyparts;
	model->geomset = ZG_Malloc(&mod->memgroup, sizeof(*model->geomset) * model->numgeomsets);
	for (body = 0; body < model->numgeomsets; body++)
	{
		hlmdl_bodypart_t	*bodypart = (hlmdl_bodypart_t *) ((qbyte *) model->header + model->header->bodypartindex) + body;
		int					bodyindex;
		model->geomset[body].numalternatives = bodypart->nummodels;
		model->geomset[body].alternatives = ZG_Malloc(&mod->memgroup, sizeof(*model->geomset[body].alternatives) * bodypart->nummodels);
		for (bodyindex = 0; bodyindex < bodypart->nummodels; bodyindex++)
		{
			hlmdl_submodel_t		*amodel = (hlmdl_submodel_t *) ((qbyte *) model->header + bodypart->modelindex) + bodyindex;
			struct hlalternative_s *submodel;

			model->geomset[body].alternatives[bodyindex].numsubmeshes = amodel->nummesh;
			model->geomset[body].alternatives[bodyindex].submesh = ZG_Malloc(&mod->memgroup, sizeof(*model->geomset[body].alternatives[bodyindex].submesh) * amodel->nummesh);

			submodel = &model->geomset[body].alternatives[bodyindex];

			for(m = 0; m < amodel->nummesh; m++)
			{
				hlmdl_mesh_t	*inmesh = (hlmdl_mesh_t *) ((qbyte *) model->header + amodel->meshindex) + m;
				unsigned short *order = (unsigned short *) ((qbyte *) model->header + inmesh->index);

				uvertstart = uvertcount;
				submodel->submesh[m].vbofirstvert = uvertstart;
				submodel->submesh[m].vbofirstelement = idx;
				submodel->submesh[m].numvertexes = 0;
				submodel->submesh[m].numindexes = 0;

				for(;;)
				{
					count = *order++;	/* get the vertex count and primitive type */
					if(!count) break;	/* done */

					if(count & 0x8000)
					{	//fan
						int first = HLMDL_DeDupe(order+0*4, uvert, &uvertcount, uvertstart, maxverts);
						int prev = HLMDL_DeDupe(order+1*4, uvert, &uvertcount, uvertstart, maxverts);
						count = (unsigned short)-(short)count;
						if (idx + (count-2)*3 > maxidx)
							break;	//would overflow. fixme: extend
						for (i = min(2,count); i < count; i++)
						{
							index[idx++] = first;
							index[idx++] = prev;
							index[idx++] = prev = HLMDL_DeDupe(order+i*4, uvert, &uvertcount, uvertstart, maxverts);
						}
					}
					else
					{
						int v0 = HLMDL_DeDupe(order+0*4, uvert, &uvertcount, uvertstart, maxverts);
						int v1 = HLMDL_DeDupe(order+1*4, uvert, &uvertcount, uvertstart, maxverts);
						//emit (count-2)*3 indicies as a strip
						//012 213, etc
						if (idx + (count-2)*3 > maxidx)
							break;	//would overflow. fixme: extend
						for (i = min(2,count); i < count; i++)
						{
							if (i & 1)
							{
								index[idx++] = v1;
								index[idx++] = v0;
							}
							else
							{
								index[idx++] = v0;
								index[idx++] = v1;
							}
							v0 = v1;
							index[idx++] = v1 = HLMDL_DeDupe(order+i*4, uvert, &uvertcount, uvertstart, maxverts);
						}
					}
					order += i*4;
				}

				if (uvertcount >= maxverts)
				{
					//if we're overflowing our verts, rewind, as we cannot generate this mesh. we'll just end up with a 0-index mesh, with no extra verts either
					uvertcount = uvertstart;
					idx = submodel->submesh[m].vbofirstelement;
				}

				submodel->submesh[m].numindexes = idx - submodel->submesh[m].vbofirstelement;
				submodel->submesh[m].numvertexes = uvertcount - uvertstart;
			}
		}
	}

	mesh = &model->mesh;
	mesh->indexes = ZG_Malloc(model->memgroup, sizeof(*mesh->indexes)*idx);
	memcpy(mesh->indexes, index, sizeof(*index)*idx);

	mesh->colors4b_array = ZG_Malloc(model->memgroup, sizeof(*mesh->colors4b_array)*uvertcount);
	mesh->st_array = ZG_Malloc(model->memgroup, sizeof(*mesh->st_array)*uvertcount);
	mesh->lmst_array[0] = ZG_Malloc(model->memgroup, sizeof(*mesh->lmst_array[0])*uvertcount);
	mesh->xyz_array = ZG_Malloc(model->memgroup, sizeof(*mesh->xyz_array)*uvertcount);
	mesh->normals_array = ZG_Malloc(model->memgroup, sizeof(*mesh->normals_array)*uvertcount);
	mesh->bonenums = ZG_Malloc(model->memgroup, sizeof(*mesh->bonenums)*uvertcount);
	mesh->boneweights = ZG_Malloc(model->memgroup, sizeof(*mesh->boneweights)*uvertcount);
#if defined(RTLIGHTS)
	mesh->snormals_array = ZG_Malloc(model->memgroup, sizeof(*mesh->snormals_array)*uvertcount);
	mesh->tnormals_array = ZG_Malloc(model->memgroup, sizeof(*mesh->tnormals_array)*uvertcount);
#endif

	mesh->numindexes = idx;
	mesh->numvertexes = uvertcount;

	for (body = 0; body < model->numgeomsets; body++)
	{
		hlmdl_bodypart_t	*bodypart = (hlmdl_bodypart_t *) ((qbyte *) model->header + model->header->bodypartindex) + body;
		int					bodyindex;
		for (bodyindex = 0; bodyindex < bodypart->nummodels; bodyindex++)
		{
			hlmdl_submodel_t		*amodel = (hlmdl_submodel_t *) ((qbyte *) model->header + bodypart->modelindex) + bodyindex;

			vec3_t *verts = (vec3_t *) ((qbyte *) model->header + amodel->vertindex);
			qbyte *bone = ((qbyte *) model->header + amodel->vertinfoindex);
			vec3_t *norms = (vec3_t *) ((qbyte *) model->header + amodel->normindex);
			size_t iv, ov;

			struct hlalternative_s *submodel = &model->geomset[body].alternatives[bodyindex];
			for(m = 0; m < amodel->nummesh; m++)
			{
				submesh = &submodel->submesh[m];

				submesh->indexes		= mesh->indexes			+ submesh->vbofirstelement;
				submesh->colors4b_array	= mesh->colors4b_array	+ submesh->vbofirstvert;
				submesh->st_array		= mesh->st_array		+ submesh->vbofirstvert;
				submesh->lmst_array[0]	= mesh->lmst_array[0]	+ submesh->vbofirstvert;
				submesh->xyz_array		= mesh->xyz_array		+ submesh->vbofirstvert;
				submesh->normals_array	= mesh->normals_array	+ submesh->vbofirstvert;
				submesh->bonenums		= mesh->bonenums		+ submesh->vbofirstvert;
				submesh->boneweights	= mesh->boneweights		+ submesh->vbofirstvert;

				//prepare the verticies now that we have the mappings
				for(ov = 0, iv = submesh->vbofirstvert; ov < submesh->numvertexes; ov++, iv++)
				{
					submesh->bonenums[ov][0] = submesh->bonenums[ov][1] = submesh->bonenums[ov][2] = submesh->bonenums[ov][3] = bone[uvert[iv].vertidx];
					Vector4Set(submesh->boneweights[ov], 1, 0, 0, 0);
					Vector4Set(submesh->colors4b_array[ov], 255, 255, 255, 255);	//why bytes? why not?

					submesh->lmst_array[0][ov][0] = uvert[iv].scoord;
					submesh->lmst_array[0][ov][1] = uvert[iv].tcoord;
					VectorCopy(verts[uvert[iv].vertidx], submesh->xyz_array[ov]);

					//Warning: these models use different tables for vertex and normals.
					//this means they might be transformed by different bones. we ignore that and just assume that the normals will want the same bone.
					VectorCopy(norms[uvert[iv].normalidx], submesh->normals_array[ov]);
				}

#if defined(RTLIGHTS)
				//treat this as the base pose, and calculate the sdir+tdir for bumpmaps.
				submesh->snormals_array = mesh->snormals_array + submesh->vbofirstvert;
				submesh->tnormals_array = mesh->tnormals_array + submesh->vbofirstvert;
//				R_Generate_Mesh_ST_Vectors(submesh);
#endif
			}
		}
	}

	//scratch space...
	mesh->indexes = ZG_Malloc(model->memgroup, sizeof(*mesh->indexes)*idx);

	//don't need that mapping any more
	free(uvert);
	free(index);
}
#endif

/*
 =======================================================================================================================
	Mod_LoadHLModel - read in the model's constituent parts
 =======================================================================================================================
 */
qboolean QDECL Mod_LoadHLModel (model_t *mod, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	int i, j;
	struct hlmodelshaders_s *shaders;
	hlmdl_tex_t	*tex;

	lmalloc_t atlas;
	char texname[MAX_QPATH];
#endif

	hlmodel_t *model;
	hlmdl_header_t *header;
	hlmdl_header_t *texheader;
	hlmdl_bone_t	*bones;
	hlmdl_bonecontroller_t	*bonectls;
	void *texmem = NULL;


	//load the model into hunk
	model = ZG_Malloc(&mod->memgroup, sizeof(hlmodel_t));
	model->memgroup = &mod->memgroup;

	header = ZG_Malloc(&mod->memgroup, fsize);
	memcpy(header, buffer, fsize);

#if defined(HLSERVER) && (defined(__powerpc__) || defined(__ppc__))
//this is to let anyone who tries porting it know that there is a serious issue. And I'm lazy.
#ifdef warningmsg
#pragma warningmsg("-----------------------------------------")
#pragma warningmsg("FIXME: No byteswapping on halflife models")	//hah, yeah, good luck with that, you'll need it.
#pragma warningmsg("-----------------------------------------")
#endif
#endif

	if (header->version != 10)
	{
		Con_Printf(CON_ERROR "Cannot load halflife model %s - unknown version %i\n", mod->name, header->version);
		return false;
	}

	if (header->numcontrollers > MAX_BONE_CONTROLLERS)
	{
		Con_Printf(CON_ERROR "Cannot load model %s - too many controllers %i\n", mod->name, header->numcontrollers);
		return false;
	}
	if (header->numbones > MAX_BONES)
	{
		Con_Printf(CON_ERROR "Cannot load model %s - too many bones %i\n", mod->name, header->numbones);
		return false;
	}

	texheader = NULL;
	if (!header->numtextures)
	{
		size_t fz;
		char texmodelname[MAX_QPATH];
		COM_StripExtension(mod->name, texmodelname, sizeof(texmodelname));
		Q_strncatz(texmodelname, "t.mdl", sizeof(texmodelname));
		//no textures? eesh. They must be stored externally.
		texheader = texmem = (hlmdl_header_t*)FS_LoadMallocFile(texmodelname, &fz);
		if (texheader)
		{
			if (texheader->version != 10)
				texheader = NULL;
		}
	}

	if (!texheader)
		texheader = header;
	else
		header->numtextures = texheader->numtextures;

	bones = (hlmdl_bone_t *) ((qbyte *) header + header->boneindex);
	bonectls = (hlmdl_bonecontroller_t *) ((qbyte *) header + header->controllerindex);

	model->header = header;
	model->bones = bones;
	model->bonectls = bonectls;

#ifndef SERVERONLY
	model->compatbones = ZG_Malloc(&mod->memgroup, header->numbones * sizeof(*model->compatbones));
	for (i = 0; i < header->numbones; i++)
	{
		float matrix[12];
		Q_strncpyz(model->compatbones[i].name, model->bones[i].name, sizeof(model->bones[i].name));
		model->compatbones[i].parent = model->bones[i].parent;
		model->compatbones[i].ref.org[0] = model->bones[i].value[0];
		model->compatbones[i].ref.org[1] = model->bones[i].value[1];
		model->compatbones[i].ref.org[2] = model->bones[i].value[2];
		QuaternionGLAngle(model->bones[i].value+3, model->compatbones[i].ref.quat);
		model->compatbones[i].ref.scale[0] = 1.0f;
		model->compatbones[i].ref.scale[1] = 1.0f;
		model->compatbones[i].ref.scale[2] = 1.0f;

		//compute rel matrix
		GenMatrixPosQuat4Scale(model->compatbones[i].ref.org, model->compatbones[i].ref.quat, model->compatbones[i].ref.scale, matrix);
		//compute abs matrix.
		if(model->bones[i].parent>=0)
			R_ConcatTransforms((void*)transform_matrix[model->bones[i].parent], (void*)matrix, transform_matrix[i]);
		else
			memcpy(transform_matrix[i], matrix, 12 * sizeof(float));
		//keep the ragdoll code happy with its insistance on using inverses.
		Matrix3x4_Invert_Simple((const float*)transform_matrix[i], model->compatbones[i].inverse);
	}

	tex = (hlmdl_tex_t *) ((qbyte *) texheader + texheader->textures);

	shaders = ZG_Malloc(&mod->memgroup, texheader->numtextures*sizeof(shader_t));
	model->shaders = shaders;

	for(i = 0; i < texheader->numtextures; i++)
	{
		Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "%s/%s", mod->name, COM_SkipPath(tex[i].name));

		/* handle the special textures - eukara */
		if (tex[i].flags)
		{
			char *shader;
			if (tex[i].flags & HLMDLFL_FULLBRIGHT)
			{
				if (tex[i].flags & HLMDLFL_CHROME)
				{
					shader = HLSHADER_FULLBRIGHTCHROME;
					Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "common/hlmodel_fullbrightchrome");
				}
				else
				{
					shader = HLSHADER_FULLBRIGHT;
					Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "common/hlmodel_fullbright");
				}
			}
			else if ( (tex[i].flags & HLMDLFL_MASKED) || (tex[i].flags & (HLMDLFL_MASKED | HLMDLFL_ALPHASOLID)))
			{
				shader = HLSHADER_MASKED;
				Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "common/hlmodel_masked");
			}
			else if (tex[i].flags & HLMDLFL_CHROME)
			{
				shader = HLSHADER_CHROME;
				Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "common/hlmodel_chrome");
			}
			else
			{
				shader = "";
				Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "common/hlmodel_other");
			}
			shaders[i].defaultshadertext = shader;
		}
		else
		{
			shaders[i].defaultshadertext = NULL;
			Q_snprintfz(shaders[i].name, sizeof(shaders[i].name), "common/hlmodel");
		}
		memset(&shaders[i].defaulttex, 0, sizeof(shaders[i].defaulttex));
	}


	//figure out the preferred atlas size. hopefully it'll fit well enough...
	if (texheader->numtextures == 1)
		Mod_LightmapAllocInit(&atlas, false, tex[0].w, tex[0].h, 0);
	else
	{
		int sz = 1;
		for(i = 0; i < texheader->numtextures; i++)
			while (sz < tex[i].w || sz < tex[i].h)
				sz <<= 1;
		for (; sz < sh_config.texture2d_maxsize && sz <= LMBLOCK_SIZE_MAX; sz<<=1)
		{
			unsigned short x,y;
			int atlasid;
			Mod_LightmapAllocInit(&atlas, false, sz, sz, 0);
			for(i = 0; i < texheader->numtextures; i++)
			{
				if ((tex[i].flags & HLMDLFL_CHROME) || !Q_strncasecmp(tex[i].name, "DM_Base", 7))
					continue;
				Mod_LightmapAllocBlock(&atlas, tex[i].w, tex[i].h, &x, &y, &atlasid);
			}
			if (i == texheader->numtextures && atlas.lmnum <= 0)
				break;	//okay, just go with it.
		}
		Mod_LightmapAllocInit(&atlas, false, sz, sz, 0);
	}
	for(i = 0; i < texheader->numtextures; i++)
	{
		if ((tex[i].flags & HLMDLFL_CHROME) || !Q_strncasecmp(tex[i].name, "DM_Base", 7))
		{
			shaders[i].x =
			shaders[i].y = 0;
			shaders[i].w = tex[i].w;
			shaders[i].h = tex[i].h;
			shaders[i].atlasid = -1;
			continue;
		}
		shaders[i].w = tex[i].w;
		shaders[i].h = tex[i].h;
		Mod_LightmapAllocBlock(&atlas, shaders[i].w, shaders[i].h, &shaders[i].x, &shaders[i].y, &shaders[i].atlasid);
	}
	if (atlas.allocated[0])
		atlas.lmnum++;
	//now we know where the various textures will be, generate the atlas images.
	for (j = 0; j < atlas.lmnum; j++)
	{
		texid_t basetex;
		int y, x;
		unsigned int *basepix = Z_Malloc(atlas.width * atlas.height * sizeof(*basepix));
		for(i = 0; i < texheader->numtextures; i++)
		{
			if (shaders[i].atlasid == j)
			{
				unsigned *out = basepix + atlas.width*shaders[i].y + shaders[i].x;
				qbyte *in = (qbyte *) texheader + tex[i].offset;
				qbyte *pal = (qbyte *) texheader + tex[i].w * tex[i].h + tex[i].offset;
				qbyte *rgb;
				for(y = 0; y < tex[i].h; y++, out += atlas.width-shaders[i].w)
					for(x = 0; x < tex[i].w; x++, in++)
					{
						rgb = pal + *in*3;
						*out++ = 0xff000000 | (rgb[0]<<0) | (rgb[1]<<8) | (rgb[2]<<16);
					}
			}
		}
		Q_snprintfz(texname, sizeof(texname), "%s*%i", mod->name, j);
		basetex = Image_GetTexture(texname, "", IF_NOALPHA|IF_NOREPLACE, basepix, NULL, atlas.width, atlas.height, PTI_RGBX8);
		Z_Free(basepix);
		for(i = 0; i < texheader->numtextures; i++)
		{
			if (shaders[i].atlasid == j)
				shaders[i].defaulttex.base = basetex;
		}
	}
	//and chrome textures need to preserve their texture coords to avoid weirdness.
	for(i = 0; i < texheader->numtextures; i++)
	{
		if (!Q_strncasecmp(tex[i].name, "DM_Base", 7))
		{
			int y, x;
			unsigned int *basepix = Z_Malloc(tex[i].w*tex[i].h*sizeof(*basepix) + tex[i].w*tex[i].h*2);
			unsigned int *out = basepix;
			qbyte *upper = (qbyte*)(basepix + tex[i].w * tex[i].h);	//we use an L8 texture, because we can.
			qbyte *lower = upper + tex[i].w * tex[i].h;
			qbyte *in = (qbyte *) texheader + tex[i].offset;
			qbyte *pal = (qbyte *) texheader + tex[i].w * tex[i].h + tex[i].offset;
			qbyte *rgb;
			for(y = 0; y < tex[i].h; y++)
				for(x = 0; x < tex[i].w; x++, in++)
				{
					if (*in >= 256-96 && *in < 256-64)
					{	//rows 11 and 12 are the player's upper colour (in the lower range)
						*out++ = 0xff000000;
						*upper++ = 255-(7+(*in-(256-96))*(256/32));
						*lower++ = 0;
					}
					else if (*in >= 256-64 && *in < 256-32)
					{	//rows 13 and 14 are the player's lower colour
						*out++ = 0xff000000;
						*upper++ = 0;
						*lower++ = 255-(7+(*in-(256-64))*(256/32));
					}
					else
					{	//regular and fullbright ranges... not that there is fullbrights on hlmdl
						rgb = pal + *in*3;
						*out++ = 0xff000000 | (rgb[0]<<0) | (rgb[1]<<8) | (rgb[2]<<16);
						*upper++ = 0;
						*lower++ = 0;
					}
				}

			out = basepix;
			upper = (qbyte*)(basepix + tex[i].w * tex[i].h);	//we use an L8 texture, because we can.
			lower = upper + tex[i].w * tex[i].h;

			Q_snprintfz(texname, sizeof(texname), "%s*%s", mod->name, tex[i].name);
			shaders[i].defaulttex.base         = Image_GetTexture(texname, "", IF_NOALPHA|IF_NOREPLACE, out, NULL, tex[i].w, tex[i].h, PTI_RGBX8);
			Q_snprintfz(texname, sizeof(texname), "%s*%s*upper", mod->name, tex[i].name);
			shaders[i].defaulttex.upperoverlay = Image_GetTexture(texname, "", IF_NOALPHA|IF_NOREPLACE, upper, NULL, tex[i].w, tex[i].h, PTI_L8);
			Q_snprintfz(texname, sizeof(texname), "%s*%s*lower", mod->name, tex[i].name);
			shaders[i].defaulttex.loweroverlay = Image_GetTexture(texname, "", IF_NOALPHA|IF_NOREPLACE, lower, NULL, tex[i].w, tex[i].h, PTI_L8);
			Z_Free(basepix);
		}
		else if (tex[i].flags & HLMDLFL_CHROME)
		{
			qbyte *in = (qbyte *) texheader + tex[i].offset;
			qbyte *pal = (qbyte *) texheader + tex[i].w * tex[i].h + tex[i].offset;

			shaders[i].atlasid = j++;
			Q_snprintfz(texname, sizeof(texname), "%s*%i", mod->name, shaders[i].atlasid);
			shaders[i].defaulttex.base = Image_GetTexture(texname, "", IF_NOALPHA|IF_NOREPLACE, in, pal, tex[i].w, tex[i].h, TF_8PAL24);
		}
		else if (tex[i].flags & HLMDLFL_MASKED)
		{
			int k = 0;
			qbyte *in = (qbyte *) texheader + tex[i].offset;
			qbyte *pal = (qbyte *) texheader + tex[i].w * tex[i].h + tex[i].offset;
			qbyte alphaPal[1024]; /* 256 color 32-bit palette */

			for (k = 0; k < 255; k+= 1) {
				int p = k * 4;
				int x = k * 3;
				alphaPal[p + 0] = pal[x + 0];
				alphaPal[p + 1] = pal[x + 1];
				alphaPal[p + 2] = pal[x + 2];
				alphaPal[p + 3] = 255;
			}

			/* pal index 255 = always transparent ~eukara */
			alphaPal[255*4+0] = 0;
			alphaPal[255*4+1] = 0;
			alphaPal[255*4+2] = 0;
			alphaPal[255*4+3] = 0;

			shaders[i].atlasid = j++;
			Q_snprintfz(texname, sizeof(texname), "%s*%i", mod->name, shaders[i].atlasid);
			shaders[i].defaulttex.base = Image_GetTexture(texname, "", IF_NOREPLACE, in, alphaPal, tex[i].w, tex[i].h, TF_8PAL32);
		}
	}




	model->numskinrefs = texheader->skinrefs;
	model->numskingroups = texheader->skingroups;
	model->skinref = ZG_Malloc(&mod->memgroup, model->numskinrefs*model->numskingroups*sizeof(*model->skinref));
	memcpy(model->skinref, (short *) ((qbyte *) texheader + texheader->skins), model->numskinrefs*model->numskingroups*sizeof(*model->skinref));
#endif

	if (texmem)
		Z_Free(texmem);

	mod->funcs.NativeContents = HLMDL_Contents;
	mod->funcs.NativeTrace = HLMDL_Trace;
	mod->type = mod_halflife;
	mod->numframes = model->header->numseq;
	mod->meshinfo = model;

#ifndef SERVERONLY
	model->numgeomsets = model->header->numbodyparts;
	model->geomset = ZG_Malloc(&mod->memgroup, sizeof(*model->geomset) * model->numgeomsets);
	HLMDL_PrepareVerticies(mod, model);
	//FIXME: No VBOs used.
#endif
	return true;
}

#ifdef HLSERVER
void *Mod_GetHalfLifeModelData(model_t *mod)
{
	hlmodelcache_t *mc;
	if (!mod || mod->type != mod_halflife)
		return NULL;	//halflife models only, please

	mc = Mod_Extradata(mod);
	return (void*)mc->header;
}
#endif

int HLMDL_FrameForName(model_t *mod, const char *name)
{
	int i;
	hlmdl_header_t *h;
	hlmdl_sequencelist_t *seqs;
	hlmodel_t *mc;
	if (!mod || mod->type != mod_halflife)
		return -1;	//halflife models only, please

	mc = Mod_Extradata(mod);

	h = mc->header;
	seqs = (hlmdl_sequencelist_t*)((char*)h+h->seqindex);

	for (i = 0; i < h->numseq; i++)
	{
		if (!strcmp(seqs[i].name, name))
			return i;
	}
	return -1;
}

int HLMDL_FrameForAction(model_t *mod, int actionid)
{
	int i;
	hlmdl_header_t *h;
	hlmdl_sequencelist_t *seqs;
	hlmodel_t *mc;
	int weight = 0;
	if (!mod || mod->type != mod_halflife)
		return -1;	//halflife models only, please

	mc = Mod_Extradata(mod);

	h = mc->header;
	seqs = (hlmdl_sequencelist_t*)((char*)h+h->seqindex);

	//figure out the total weight.
	for (i = 0; i < h->numseq; i++)
		if (seqs[i].action == actionid)
			weight += seqs[i].actionweight;
	//pick a random number between 0 and the total weight...
	weight *= frandom();
	//now figure out which sequence that gives us.
	for (i = 0; i < h->numseq; i++)
		if (seqs[i].action == actionid)
		{
			if (weight <= seqs[i].actionweight)
				return i;
			weight -= seqs[i].actionweight;
		}
	return -1;	//failed...
}

qboolean HLMDL_GetModelEvent(model_t *model, int animation, int eventidx, float *timestamp, int *eventcode, char **eventdata)
{
	hlmodel_t *mc = Mod_Extradata(model);
	hlmdl_header_t *h = mc->header;
	hlmdl_event_t *ev;
	hlmdl_sequencelist_t *seq = animation + (hlmdl_sequencelist_t*)((char*)h+h->seqindex);
	if (animation < 0 || animation >= h->numseq || eventidx < 0 || eventidx >= seq->num_events)
		return false;
	ev = eventidx + (hlmdl_event_t*)((char*)h+seq->ofs_events);
	*timestamp = ev->pose / seq->timing;
	*eventcode = ev->code;
	*eventdata = ev->data;
	return true;
}

int HLMDL_BoneForName(model_t *mod, const char *name)
{
	int i;
	hlmdl_header_t *h;
	hlmdl_bone_t *bones;
	hlmodel_t *mc;
	if (!mod || mod->type != mod_halflife)
		return -1;	//halflife models only, please

	mc = Mod_Extradata(mod);

	h = mc->header;
	bones = (hlmdl_bone_t*)((char*)h+h->boneindex);

	for (i = 0; i < h->numbones; i++)
	{
		if (!strcmp(bones[i].name, name))
			return i+1;
	}

	//FIXME: hlmdl has tags as well as bones.
	return 0;
}

/*
 =======================================================================================================================
	HL_CalculateBones - calculate bone positions - quaternion+vector in one function
 =======================================================================================================================
 */
void HL_CalculateBones
(
	int				frame,
	vec4_t			adjust,
	hlmdl_bone_t	*bone,
	hlmdl_anim_t	*animation,
	float			*organg
)
{
	int		i;

	/* For each vector */
	for(i = 0; i < 6; i++)
	{
		organg[i] = bone->value[i];	/* Take the bone value */

		if(animation->offset[i] != 0)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int					tempframe;
			hlmdl_animvalue_t	*animvalue = (hlmdl_animvalue_t *) ((qbyte *) animation + animation->offset[i]);
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			/* find values including the required frame */
			tempframe = frame;
			while(animvalue->num.total <= tempframe)
			{
				tempframe -= animvalue->num.total;
				animvalue += animvalue->num.valid + 1;
			}
			if (tempframe >= animvalue->num.valid)
				tempframe = animvalue->num.valid;
			else
				tempframe += 1;

			organg[i] += animvalue[tempframe].value * bone->scale[i];
		}

		if(bone->bonecontroller[i] != -1)
		{	/* Add the programmable offset. */
			organg[i] += adjust[bone->bonecontroller[i]];
		}
	}
}

/*
 =======================================================================================================================
	HL_CalcBoneAdj - Calculate the adjustment values for the programmable controllers
 =======================================================================================================================
 */
void HL_CalcBoneAdj(hlmodel_t *model)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int						i;
	float					value;
	hlmdl_bonecontroller_t	*control = (hlmdl_bonecontroller_t *)
									  ((qbyte *) model->header + model->header->controllerindex);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	for(i = 0; i < model->header->numcontrollers; i++)
	{
		/*~~~~~~~~~~~~~~~~~~~~~*/
		int j = control[i].index;
		/*~~~~~~~~~~~~~~~~~~~~~*/

		if(control[i].type & 0x8000)
		{	//wraps normally
			value = model->controller[j];// + control[i].start;
		}
		else
		{
//			value = (model->controller[j]+1)*0.5;	//shifted to give a valid range between -1 and 1, with 0 being mid-range.
//			if(value < 0)
//				value = 0;
//			else if(value > 1.0)
//				value = 1.0;
//			value = (1.0 - value) * control[i].start + value * control[i].end;

			value = model->controller[j];
			if (value < control[i].start)
				value = control[i].start;
			if (value > control[i].end)
				value = control[i].end;
		}

		/* Rotational controllers need their values converted */
		if(control[i].type >= 0x0008 && control[i].type <= 0x0020)
			model->adjust[i] = M_PI * value / 180;
		else
			model->adjust[i] = value;
	}
}

/*
 =======================================================================================================================
	HL_SetupBones - determine where vertex should be using bone movements
 =======================================================================================================================
 */
void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt );
void HL_SetupBones(hlmodel_t *model, int seqnum, int firstbone, int lastbone, float subblendfrac1, float subblendfrac2, float frametime, float *matrix)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int						i, j;
	vec3_t					organg1[2];
	vec3_t					organg2[2];
	vec3_t					organgb[2];
	vec4_t					quat1, quat2;

	int frame1, frame2;

	hlmdl_sequencelist_t	*sequence = (hlmdl_sequencelist_t *) ((qbyte *) model->header + model->header->seqindex) +
										 ((unsigned int)seqnum>=model->header->numseq?0:seqnum);
	hlmdl_sequencedata_t	*sequencedata = (hlmdl_sequencedata_t *)
										 ((qbyte *) model->header + model->header->seqgroups) +
										 sequence->seqindex;
	hlmdl_anim_t			*animation;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	matrix += firstbone*12;

	if (sequencedata->name[32])
	{
		size_t fz;
		if (sequence->seqindex >= MAX_ANIM_GROUPS)
		{
			Sys_Error("Too many animation sequence cache groups\n");
			return;
		}
		if (!model->animcache[sequence->seqindex])
			model->animcache[sequence->seqindex] = FS_LoadMallocGroupFile(model->memgroup, sequencedata->name+32, &fz, true);
		if (!model->animcache[sequence->seqindex] || model->animcache[sequence->seqindex]->magic != (('I'<<0)|('D'<<8)|('S'<<16)|('Q'<<24)) || model->animcache[sequence->seqindex]->version != 10)
		{
			Sys_Error("Unable to load %s\n", sequencedata->name+32);
			return;
		}
		animation = (hlmdl_anim_t *)((qbyte*)model->animcache[sequence->seqindex] + sequence->index);
	}
	else
		animation = (hlmdl_anim_t *) ((qbyte *) model->header + sequencedata->data + sequence->index);

	frametime *= sequence->timing;
	if (frametime < 0)
		frametime = 0;

	frame1 = (int)frametime;
	frametime -= frame1;
	frame2 = frame1+1;

	if (!sequence->numframes)
		return;
	//halflife seems to dupe the last frame in looping animations, so don't use it.
	if(frame1 >= sequence->numframes)
	{
		if (sequence->loop)
			frame1 %= sequence->numframes;
		else
			frame1 = sequence->numframes-1;
	}
	if(frame2 >= sequence->numframes)
	{
		if (sequence->loop)
			frame2 %= sequence->numframes;
		else
			frame2 = sequence->numframes-1;
	}

	if (frame2 < frame1)
	{
		i = frame2;
		frame2 = frame1;
		frame1 = i;
		frametime = 1-frametime;
	}

	if (lastbone > model->header->numbones)
		lastbone = model->header->numbones;



	HL_CalcBoneAdj(model);	/* Deal with programmable controllers */

	/*FIXME:this is useless*/
	/*
	if(sequence->motiontype & 0x0001)
		positions[sequence->motionbone][0] = 0.0;
	if(sequence->motiontype & 0x0002)
		positions[sequence->motionbone][1] = 0.0;
	if(sequence->motiontype & 0x0004)
		positions[sequence->motionbone][2] = 0.0;
		*/

	/*
	this is hellish.
	a hl model blends:
		4 controllers (on a player, it seems each one of them twists a separate bone in the chest)
		a mouth (not used on players)
		its a sequence (to be smooth we need to blend between two frames in the sequence)
		up to four source animations (ironically used to pitch up/down)
		alternate sequence (walking+firing)
		frame2 (quake expectations.)

		this is madness, quite frankly.

		luckily...
		controllers and mouth control the entire thing. they should be interpolated outside, and have no affect on blending here
		alternate sequences replace. we can just call this function twice (so long as bone ranges are incremental).
		autoanimating sequence is handled inside HL_CalculateBones (sequences are weird and it has to be handled there anyway)

		this means we only have sources and alternate frames left to cope with.

		FIXME: we don't handle frame2.
	*/

	if (sequence->hasblendseq>1)
	{
		int bf0, bf1;
		unsigned int bweights;
		struct
		{
			int frame;
			float weight;
			hlmdl_anim_t *anim;
		} blend[8];
		//right, so, this stuff is annoying.
		//we have two different blend factors.
		//we have up to 9 blend weights. figure out which frames are what
		switch(sequence->hasblendseq)
		{
		case 0:	//erk?
			return;
		case 1: //no blending.
			bf0 = bf1 = 1;
			break;
		default:
		case 2: //mix(0, 1, weight0)
		case 3: //mix(0, 1, 2, weight0);
		case 8: //weight0 only...
			bf0 = sequence->hasblendseq;
			bf1 = 1;
			break;
		case 4: //mix(mix(0, 1, weight0), mix(2, 3, weight0), weight1)
			bf0 = bf1 = 2;
			break;
		//case 6: //???
		//	bf[0] = 3; bf[1] = 2;
		//	break;
		case 9: //mix(mix(0, 1, 2, weight0), mix(2, 3, 4, weight0), mix(5, 6, 7, weight0), weight1)
			bf0 = bf1 = 3;
			break;
		}

		subblendfrac1 = (subblendfrac1+1)/2;
		subblendfrac2 = (subblendfrac2+1)/2;
		bweights = 0;
		if (bf0 > 1)
		{
			float frac = (bf0-1) * bound(0, subblendfrac1, 1);
			int f1 = bound(0, frac, bf0-1);
			int f2 = bound(0, f1+1, bf0-1);
			frac = (frac-f1);
			frac = bound(0, frac, 1);
			if (bf1 > 1)
			{
				float frac2 = (bf1-1) * bound(0, subblendfrac2, 1);
				int a1 = bound(0, frac2, bf1-1);
				int a2 = bound(0, a1+1, bf1-1);
				frac2 = (frac2-a1);
				frac2 = bound(0, frac2, 1);

				if (frac2)
				{
					if (frac)
					{
						blend[bweights].frame = frame1;
						blend[bweights].anim = animation + model->header->numbones * (f2 + a2*bf0);
						blend[bweights++].weight = frac*frac2;
					}
					if (1-frac)
					{
						blend[bweights].frame = frame1;
						blend[bweights].anim = animation + model->header->numbones * (f1 + a2*bf0);
						blend[bweights++].weight = (1-frac)*frac2;
					}
				}

				if (1-frac2)
				{
					if (frac)
					{
						blend[bweights].frame = frame1;
						blend[bweights].anim = animation + model->header->numbones * (f2 + a1*bf0);
						blend[bweights++].weight = frac*(1-frac2);
					}
					if (1-frac)
					{
						blend[bweights].frame = frame1;
						blend[bweights].anim = animation + model->header->numbones * (f1 + a1*bf0);
						blend[bweights++].weight = (1-frac)*(1-frac2);
					}
				}
			}
			else
			{
				if (frac)
				{
					blend[bweights].frame = frame1;
					blend[bweights].anim = animation + model->header->numbones * f1;
					blend[bweights++].weight = frac;
				}
				if (1-frac)
				{
					blend[bweights].frame = frame1;
					blend[bweights].anim = animation + model->header->numbones * f2;
					blend[bweights++].weight = 1-frac;
				}
			}
		}
		else
		{
			blend[bweights].frame = frame1;
			blend[bweights].anim = animation;
			blend[bweights++].weight = 1;
		}
		if (frame1 != frame2)
		{
			//bweights can be 0-4 here..
			for (i = 0; i < bweights; i++)
			{
				blend[bweights+i].frame = frame2;
				blend[bweights+i].anim = blend[i].anim;
				blend[bweights+i].weight = blend[i].weight;

				blend[i].weight *= 1-frametime;
				blend[bweights+i].weight *= frametime;
			}
			bweights *= 2;
		}

		for(i = firstbone; i < lastbone; i++, matrix+=12)
		{
			vec3_t t;
			float len;

			HL_CalculateBones(blend[0].frame, model->adjust, model->bones + i, blend[0].anim + i, organgb[0]);
			QuaternionGLAngle(organgb[1], quat2);
			Vector4Scale(quat2, blend[0].weight, quat1);
			VectorScale(organgb[0], blend[0].weight, t);

			for (j = 1; j < bweights; j++)
			{
				HL_CalculateBones(blend[j].frame, model->adjust, model->bones + i, blend[j].anim + i, organgb[0]);
				QuaternionGLAngle(organgb[1], quat2);
				if (DotProduct4(quat2, quat1) < 0)	//negative quats are annoying
					Vector4MA(quat1, -blend[j].weight, quat2, quat1);
				else
					Vector4MA(quat1, blend[j].weight, quat2, quat1);
				VectorMA(t, blend[j].weight, organgb[0], t);
			}

			//we were lame and didn't use slerp. boo hiss. now we need to normalise the things and hope we didn't hit any singularities
			len = sqrt(DotProduct4(quat1,quat1));
			if (len && len != 1)
			{
				len = 1/len;
				quat1[0] *= len;
				quat1[1] *= len;
				quat1[2] *= len;
				quat1[3] *= len;
			}

			QuaternionGLMatrix(quat1[0], quat1[1], quat1[2], quat1[3], (vec4_t*)matrix);
			matrix[0*4+3] = t[0];
			matrix[1*4+3] = t[1];
			matrix[2*4+3] = t[2];
		}
	}
	else
	{
		for(i = firstbone; i < lastbone; i++, matrix+=12)
		{
			HL_CalculateBones(frame1, model->adjust, model->bones + i, animation + i, organg1[0]);
			QuaternionGLAngle(organg1[1], quat1);	/* A quaternion */
			if (frame1 != frame2)
			{
				HL_CalculateBones(frame2, model->adjust, model->bones + i, animation + i, organg2[0]);
				QuaternionGLAngle(organg2[1], quat2);	/* A quaternion */

				//lerp the quats properly rather than poorly lerping eular angles.
				QuaternionSlerp(quat1, quat2, frametime, quat1);
				VectorInterpolate(organg1[0], frametime, organg2[0], organg1[0]);
			}

			//figure out the relative bone matrix.
			//we probably ought to keep them as quats or something.
			QuaternionGLMatrix(quat1[0], quat1[1], quat1[2], quat1[3], (vec4_t*)matrix);
			matrix[0*4+3] = organg1[0][0];
			matrix[1*4+3] = organg1[0][1];
			matrix[2*4+3] = organg1[0][2];
		}
	}
}

int HLMDL_GetNumBones(model_t *mod, qboolean tags)
{
	hlmodel_t *mc;
	if (!mod || mod->type != mod_halflife)
		return -1;	//halflife models only, please

	mc = Mod_Extradata(mod);
	if (tags)
		return mc->header->numbones + mc->header->num_attachments;
	return mc->header->numbones;
}

int HLMDL_GetBoneParent(model_t *mod, int bonenum)
{
	hlmodel_t *model = Mod_Extradata(mod);

	if (bonenum >= 0 && bonenum < model->header->numbones)
		return model->bones[bonenum].parent;
	bonenum -= model->header->numbones;
	if (bonenum >= 0 && bonenum < model->header->num_attachments)
	{
		hlmdl_attachment_t *attachments = bonenum+(hlmdl_attachment_t*)((char*)model->header + model->header->ofs_attachments);
		return attachments->bone;
	}
	return -1;
}

const char *HLMDL_GetBoneName(model_t *mod, int bonenum)
{
	hlmodel_t *model = Mod_Extradata(mod);

	if (bonenum >= 0 && bonenum < model->header->numbones)
		return model->bones[bonenum].name;
	bonenum -= model->header->numbones;
	if (bonenum >= 0 && bonenum < model->header->num_attachments)
	{
		hlmdl_attachment_t *attachments = bonenum+(hlmdl_attachment_t*)((char*)model->header + model->header->ofs_attachments);
		if (*attachments->name)
			return attachments->name;
		return "Unnamed Attachment";
	}
	return NULL;
}

int HLMDL_GetAttachment(model_t *mod, int tagnum, float *resultmatrix)
{
	hlmodel_t *model = Mod_Extradata(mod);
	if (tagnum >= 0 && tagnum < model->header->num_attachments)
	{
		hlmdl_attachment_t *attachments = tagnum+(hlmdl_attachment_t*)((char*)model->header + model->header->ofs_attachments);
		resultmatrix[3] = attachments->org[0];
		resultmatrix[7] = attachments->org[1];
		resultmatrix[11] = attachments->org[2];
		return attachments->bone;
	}
	return -1;
}

static int HLMDL_GetBoneData_Internal(hlmodel_t *model, int firstbone, int lastbone, const framestate_t *fstate, float *result)
{
	int b, cbone, bgroup;

	for (b = 0; b < MAX_BONE_CONTROLLERS; b++)
		model->controller[b] = fstate->bonecontrols[b];
	for (cbone = 0, bgroup = 0; bgroup < FS_COUNT; bgroup++)
	{
		lastbone = fstate->g[bgroup].endbone;
		if (bgroup == FS_COUNT-1)
			lastbone = model->header->numbones;
		if (cbone >= lastbone)
			continue;
		HL_SetupBones(model, fstate->g[bgroup].frame[0] & ~0x8000, cbone, lastbone, fstate->g[bgroup].subblendfrac, fstate->g[bgroup].subblend2frac, fstate->g[bgroup].frametime[0], result);	/* Setup the bones */
		cbone = lastbone;
	}
	return cbone;
}
int HLMDL_GetBoneData(model_t *mod, int firstbone, int lastbone, const framestate_t *fstate, float *result)
{
	return HLMDL_GetBoneData_Internal(Mod_Extradata(mod), firstbone, lastbone, fstate, result);
}

const char *HLMDL_FrameNameForNum(model_t *mod, int surfaceidx, int seqnum)
{
	hlmodel_t *model = Mod_Extradata(mod);
	hlmdl_sequencelist_t	*sequence = (hlmdl_sequencelist_t *) ((qbyte *) model->header + model->header->seqindex) +
										 ((unsigned int)seqnum>=model->header->numseq?0:seqnum);
	return sequence->name;
}
qboolean HLMDL_FrameInfoForNum(model_t *mod, int surfaceidx, int seqnum, char **name, int *numframes, float *duration, qboolean *loop, int *act)
{
	hlmodel_t *model = Mod_Extradata(mod);
	hlmdl_sequencelist_t	*sequence = (hlmdl_sequencelist_t *) ((qbyte *) model->header + model->header->seqindex) +
										 ((unsigned int)seqnum>=model->header->numseq?0:seqnum);

	*name = sequence->name;
	*numframes = sequence->numframes;
	*duration = (sequence->numframes-1)/sequence->timing;
	*loop = sequence->loop;
	*act = sequence->action;
	return true;
}



qboolean HLMDL_Trace		(model_t *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p1, const vec3_t p2, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int against, struct trace_s *trace)
{
	hlmodel_t *hm = Mod_Extradata(model);
	float *relbones;
	float calcrelbones[MAX_BONES*12];
	int bonecount;
	int b, i;
	vec3_t norm, p1l, p2l;
	float inverse[12];
	hlmdl_hitbox_t *hitbox = (hlmdl_hitbox_t*)((char*)hm->header+hm->header->ofs_hitboxes);
	float dist, d1, d2, f, enterfrac, enterdist, exitfrac;
	qboolean startout, endout;
	int enterplane;

	memset (trace, 0, sizeof(trace_t));
	trace->fraction = trace->truefraction = 1;
	if (!(against & FTECONTENTS_BODY) || !framestate)
		return false;

	if (framestate->bonestate && framestate->skeltype == SKEL_ABSOLUTE)
	{
		relbones = framestate->bonestate;
		bonecount = framestate->bonecount;
		if (axis)
		{
			for (b = 0; b < bonecount; b++)
				R_ConcatTransformsAxis(axis, (void*)(relbones+b*12), transform_matrix[b]);
		}
		else
			memcpy(transform_matrix, relbones, bonecount * 12 * sizeof(float));
	}
	else
	{
		//get relative bones from th emodel.
		if (framestate->bonestate)
		{
			relbones = framestate->bonestate;
			bonecount = framestate->bonecount;
		}
		else
		{
			relbones = calcrelbones;
			bonecount = HLMDL_GetBoneData(model, 0, MAX_BONES, framestate, calcrelbones);
		}

		//convert relative to absolutes
		for (b = 0; b < bonecount; b++)
		{
			/* If we have a parent, take the addition. Otherwise just copy the values */
			if(hm->bones[b].parent>=0)
				R_ConcatTransforms((void*)transform_matrix[hm->bones[b].parent], (void*)(relbones+b*12), transform_matrix[b]);
			else if (axis)
				R_ConcatTransformsAxis(axis, (void*)(relbones+b*12), transform_matrix[b]);
			else
				memcpy(transform_matrix[b], relbones+b*12, 12 * sizeof(float));
		}
	}

	for (b = 0; b < hm->header->num_hitboxes; b++, hitbox++)
	{
		startout = false;
		endout = false;
		enterplane = 0;
		enterfrac = -1;
		exitfrac = 10;
		enterdist = 0;

		//fixme: would be nice to check if there's a possible collision a bit faster, without needing to do lots of excess maths.

		//transform start+end into the bbox, so everything is axial and simple.
		Matrix3x4_Invert_Simple((void*)transform_matrix[hitbox->bone], inverse);
		Matrix3x4_RM_Transform3(inverse, p1, p1l);
		Matrix3x4_RM_Transform3(inverse, p2, p2l);
		//fixme: would it be faster to just generate the plane and transform that, colliding non-axially? would probably be better for sized impactors.

		//clip against the 6 axial faces
		for (i = 0; i < 6; i++)
		{
			if (i < 3)
			{	//normal>0
				dist = hitbox->maxs[i] - mins[i];
				d1 = p1l[i] - dist;
				d2 = p2l[i] - dist;
			}
			else
			{//normal<0
				dist = maxs[i-3] - hitbox->mins[i-3];
				d1 = -p1l[i-3] - dist;
				d2 = -p2l[i-3] - dist;
			}
			//FIXME: if the trace has size, we should insert 6 extra planes for the shape of the impactor
			//FIXME: capsules

			if (d1 >= 0)
				startout = true;
			if (d2 > 0)
				endout = true;

			//if we're fully outside any plane, then we cannot possibly enter the brush, skip to the next one
			if (d1 > 0 && d2 >= 0)
				goto nextbrush;

			//if we're fully inside the plane, then whatever is happening is not relevent for this plane
			if (d1 < 0 && d2 <= 0)
				continue;

			f = d1 / (d1-d2);
			if (d1 > d2)
			{
				//entered the brush. favour the furthest fraction to avoid extended edges (yay for convex shapes)
				if (enterfrac < f)
				{
					enterfrac = f;
					enterplane = i;
					enterdist = dist;
				}
			}
			else
			{
				//left the brush, favour the nearest plane (smallest frac)
				if (exitfrac > f)
				{
					exitfrac = f;
				}
			}
		}

		if (!startout)
		{
			trace->startsolid = true;
			if (!endout)
				trace->allsolid = true;
			trace->contents = FTECONTENTS_BODY;

			trace->brush_face = 0;
			trace->bone_id = hitbox->bone+1;
			trace->brush_id = b+1;
			trace->surface_id = hitbox->body;
			break;
		}
		if (enterfrac != -1 && enterfrac < exitfrac)
		{
			//impact!
			if (enterfrac < trace->fraction)
			{
				trace->fraction = trace->truefraction = enterfrac;
				trace->plane.dist = enterdist;
				trace->contents = FTECONTENTS_BODY;

				trace->brush_face = enterplane+1;
				trace->bone_id = hitbox->bone+1;
				trace->brush_id = b+1;
				trace->surface_id = hitbox->body;
			}
		}
nextbrush:
		;
	}

	if (trace->brush_face)
	{
		VectorClear(norm);
		if (trace->brush_face < 4)
			norm[trace->brush_face-1] = 1;
		else
			norm[trace->brush_face-4] = -1;
		Matrix3x4_RM_Transform3x3((void*)transform_matrix[trace->bone_id-1], norm, trace->plane.normal);
	}
	else
		VectorClear(trace->plane.normal);
	VectorInterpolate(p1, trace->fraction, p2, trace->endpos);

	return trace->truefraction != 1;
}
unsigned int HLMDL_Contents	(model_t *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs)
{
	trace_t tr;
	HLMDL_Trace(model, hulloverride, framestate, axis, p, p, mins, maxs, false, ~0, &tr);
	return tr.contents;
}


#ifndef SERVERONLY
static void R_HL_BuildFrame(hlmodel_t *model, int bodypart, int bodyidx, int meshidx, struct hlmodelshaders_s *texinfo, mesh_t *outmesh, float fatness)
{
	int v;
	int w = texinfo->defaulttex.base->width;
	int h = texinfo->defaulttex.base->height;
	vec2_t texbase = {texinfo->x/(float)w, texinfo->y/(float)h};
	vec2_t texscale = {1.0/w, 1.0/h};

	mesh_t *srcmesh = &model->geomset[bodypart].alternatives[bodyidx].submesh[meshidx];

	//copy out the indexes into the final mesh.
	memcpy(outmesh->indexes+outmesh->numindexes, srcmesh->indexes, sizeof(index_t)*srcmesh->numindexes);
	outmesh->numindexes += srcmesh->numindexes;

	if (outmesh == &model->mesh)
	{	//get the backend to do the skeletal stuff (read: glsl)
		for(v = 0; v < srcmesh->numvertexes; v++)
		{	//should really come up with a better way to deal with this, like rect textures.
			srcmesh->st_array[v][0] = texbase[0] + srcmesh->lmst_array[0][v][0] * texscale[0];
			srcmesh->st_array[v][1] = texbase[1] + srcmesh->lmst_array[0][v][1] * texscale[1];
		}
	}
	else
	{	//backend can't handle it, apparently. do it in software.
		int fvert = srcmesh->vbofirstvert;
		vecV_t *nxyz = outmesh->xyz_array+fvert;
		vec3_t *nnorm = outmesh->normals_array+fvert;

		for(v = 0; v < srcmesh->numvertexes; v++)
		{	//should really come up with a better way to deal with this, like rect textures.
			srcmesh->st_array[v][0] = texbase[0] + srcmesh->lmst_array[0][v][0] * texscale[0];
			srcmesh->st_array[v][1] = texbase[1] + srcmesh->lmst_array[0][v][1] * texscale[1];

			//transform to nxyz (a separate buffer from the srcmesh data)
			VectorTransform(srcmesh->xyz_array[v], (void *)transform_matrix[srcmesh->bonenums[v][0]], nxyz[v]);

			//transform to nnorm (a separate buffer from the srcmesh data)
			nnorm[v][0] = DotProduct(srcmesh->normals_array[v], transform_matrix[srcmesh->bonenums[v][0]][0]);
			nnorm[v][1] = DotProduct(srcmesh->normals_array[v], transform_matrix[srcmesh->bonenums[v][0]][1]);
			nnorm[v][2] = DotProduct(srcmesh->normals_array[v], transform_matrix[srcmesh->bonenums[v][0]][2]);

			//FIXME: svector, tvector!
		}

		if (fatness)
			for(v = 0; v < srcmesh->numvertexes; v++)
				VectorMA(nxyz[v], fatness, nnorm[v], nxyz[v]);
	}
}

static void R_HL_BuildMeshes(batch_t *b)
{
	entity_t *rent = b->ent;
	hlmodel_t *model = Mod_Extradata(rent->model);
	int						body, m;
	static mesh_t *mptr[1], softbonemesh;
	skinfile_t *sk = rent->customskin?Mod_LookupSkin(rent->customskin):NULL;

	const unsigned int entity_body = 0/*rent->body*/;
	int surf;

	float *bones;
	int numbones;

	if (b->shader->prog && (b->shader->prog->supportedpermutations & PERMUTATION_SKELETAL) && model->header->numbones < sh_config.max_gpu_bones && !b->ent->fatness)
	{	//okay, we can use gpu gones. yay.
		b->mesh = mptr;
		*b->mesh = &model->mesh;
	}
	else
	{
		static vecV_t nxyz_buffer[65536];
		static vec3_t nnorm_buffer[65536];
		//no gpu bone support. :(
		softbonemesh = model->mesh;
		b->mesh = mptr;
		*b->mesh = &softbonemesh;

		//this stuff will get recalculated
		softbonemesh.xyz_array = nxyz_buffer;
		softbonemesh.normals_array = nnorm_buffer;

		//don't get confused.
		softbonemesh.bonenums = NULL;
		softbonemesh.boneweights = NULL;
		softbonemesh.bones = NULL;
		softbonemesh.numbones = 0;
	}
	(*b->mesh)->numindexes = 0;

	//FIXME: cache this!
	if (rent->framestate.bonecount >= model->header->numbones)
	{	//skeletal object...
		int b;
		if (rent->framestate.skeltype == SKEL_RELATIVE)
		{
			numbones = model->header->numbones;
			for (b = 0; b < numbones; b++)
			{
				/* If we have a parent, take the addition. Otherwise just copy the values */
				if(model->bones[b].parent>=0)
				{
					R_ConcatTransforms((void*)transform_matrix[model->bones[b].parent], (void*)(rent->framestate.bonestate+b*12), transform_matrix[b]);
				}
				else
				{
					memcpy(transform_matrix[b], rent->framestate.bonestate+b*12, 12 * sizeof(float));
				}
			}
			bones = transform_matrix[0][0];
		}
		else
		{
			bones = rent->framestate.bonestate;
			numbones = rent->framestate.bonecount;
		}
	}
	else
	{	//lerp the bone data ourselves.
		float relatives[12*MAX_BONES];
		int cbone, b;
		bones = transform_matrix[0][0];
		numbones = model->header->numbones;

		cbone = HLMDL_GetBoneData_Internal(model, 0, model->header->numbones, &rent->framestate, relatives);

		//convert relative to absolutes
		for (b = 0; b < cbone; b++)
		{
			/* If we have a parent, take the addition. Otherwise just copy the values */
			if(model->bones[b].parent>=0)
			{
				R_ConcatTransforms((void*)transform_matrix[model->bones[b].parent], (void*)(relatives+b*12), transform_matrix[b]);
			}
			else
			{
				memcpy(transform_matrix[b], relatives+b*12, 12 * sizeof(float));
			}
		}
	}

	model->mesh.bones = bones;
	model->mesh.numbones = numbones;

	for (surf = 0; surf < b->meshes; surf++)
	{
		body = b->user.alias.surfrefs[surf] >> 8;
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			hlmdl_bodypart_t	*bodypart = (hlmdl_bodypart_t *) ((qbyte *) model->header + model->header->bodypartindex) + body;
			int					bodyindex = ((sk && body < MAX_GEOMSETS && sk->geomset[body] >= 1)?sk->geomset[body]-1:(entity_body / bodypart->base)) % bodypart->nummodels;
			hlmdl_submodel_t	*amodel = (hlmdl_submodel_t *) ((qbyte *) model->header + bodypart->modelindex) + bodyindex;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			/* Draw each mesh */
			m = b->user.alias.surfrefs[surf] & 0xff;
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				hlmdl_mesh_t	*mesh = (hlmdl_mesh_t *) ((qbyte *) model->header + amodel->meshindex) + m;
				struct hlmodelshaders_s *texinfo;
				int skinidx = mesh->skinindex;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				if (rent->skinnum < model->numskingroups)
					skinidx += rent->skinnum * model->numskinrefs;
				texinfo = &model->shaders[model->skinref[skinidx]];

				R_HL_BuildFrame(model, body, bodyindex, m, texinfo, *b->mesh, b->ent->fatness);
			}
		}
	}
	b->meshes = 1;
}
qboolean R_CalcModelLighting(entity_t *e, model_t *clmodel);

void R_HalfLife_TouchTextures(model_t *mod)
{
	hlmodel_t *model = Mod_Extradata(mod);
	unsigned int t;
	for (t = 0; t < model->header->numtextures; t++)
		Shader_TouchTexnums(&model->shaders[t].defaulttex);
}
void R_HalfLife_GenerateBatches(entity_t *rent, batch_t **batches)
{
	hlmodel_t *model = Mod_Extradata(rent->model);
	int						body, m;
	skinfile_t *sk = rent->customskin?Mod_LookupSkin(rent->customskin):NULL;

	const unsigned int entity_body = 0/*rent->body*/;
	batch_t *b = NULL;

	unsigned int surfidx = 0;

	R_CalcModelLighting(rent, rent->model);	//make sure the ent's lighting is right.

	/*if (!model->vbobuilt)
	{
		mesh_t *mesh = &model->mesh;
		vbo_t *vbo = &model->vbo;
		vbobctx_t ctx;

		model->vbobuilt = true;

		BE_VBO_Begin(&ctx, (sizeof(*mesh->xyz_array)+
							sizeof(*mesh->colors4b_array)+
							sizeof(*mesh->st_array)+
							sizeof(*mesh->lmst_array[0])+
							sizeof(*mesh->normals_array)+
							sizeof(*mesh->bonenums)+
							sizeof(*mesh->boneweights)+
							sizeof(*mesh->snormals_array)+
							sizeof(*mesh->tnormals_array))*mesh->numvertexes);
		BE_VBO_Data(&ctx, mesh->xyz_array, sizeof(*mesh->xyz_array)*mesh->numvertexes, &vbo->coord);
		BE_VBO_Data(&ctx, mesh->colors4b_array, sizeof(*mesh->colors4b_array)*mesh->numvertexes, &vbo->colours[0]);vbo->colours_bytes = true;
		BE_VBO_Data(&ctx, mesh->st_array, sizeof(*mesh->st_array)*mesh->numvertexes, &vbo->texcoord);
		BE_VBO_Data(&ctx, mesh->lmst_array[0], sizeof(*mesh->lmst_array[0])*mesh->numvertexes, &vbo->lmcoord[0]);
		BE_VBO_Data(&ctx, mesh->normals_array, sizeof(*mesh->normals_array)*mesh->numvertexes, &vbo->normals);
		BE_VBO_Data(&ctx, mesh->bonenums, sizeof(*mesh->bonenums)*mesh->numvertexes, &vbo->bonenums);
		BE_VBO_Data(&ctx, mesh->boneweights, sizeof(*mesh->boneweights)*mesh->numvertexes, &vbo->boneweights);
#if defined(RTLIGHTS)
		BE_VBO_Data(&ctx, mesh->snormals_array, sizeof(*mesh->snormals_array)*mesh->numvertexes, &vbo->tvector);
		BE_VBO_Data(&ctx, mesh->tnormals_array, sizeof(*mesh->tnormals_array)*mesh->numvertexes, &vbo->svector);
#endif
		BE_VBO_Finish(&ctx, mesh->indexes, mesh->numindexes, &vbo->indicies, &vbo->vbomem, &vbo->ebomem);
	}*/

	for (body = 0; body < model->numgeomsets; body++)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		hlmdl_bodypart_t	*bodypart = (hlmdl_bodypart_t *) ((qbyte *) model->header + model->header->bodypartindex) + body;
		int					bodyindex = ((sk && body < MAX_GEOMSETS && sk->geomset[body] >= 1)?sk->geomset[body]-1:(entity_body / bodypart->base)) % bodypart->nummodels;
		hlmdl_submodel_t	*amodel = (hlmdl_submodel_t *) ((qbyte *) model->header + bodypart->modelindex) + bodyindex;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


		/* Draw each mesh */
		for(m = 0; m < amodel->nummesh; m++, surfidx++)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			hlmdl_mesh_t	*mesh = (hlmdl_mesh_t *) ((qbyte *) model->header + amodel->meshindex) + m;
			struct hlmodelshaders_s *s;
			int skinidx = mesh->skinindex;
			texnums_t *skin;
			shader_t *shader;
			int sort, j;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			if (skinidx >= model->numskinrefs)
				continue;	//can happen from bad mesh/skin mixing
			if (rent->skinnum < model->numskingroups)
				skinidx += rent->skinnum * model->numskinrefs;
			s = &model->shaders[model->skinref[skinidx]];


			if (!s->shader)
			{
				if (s->defaultshadertext)
					s->shader = R_RegisterShader(s->name, SUF_NONE, s->defaultshadertext);
				else
					s->shader = R_RegisterSkin(rent->model, s->name);
//				R_BuildDefaultTexnums(&s->defaulttex, s->shader, 0);
			}

			skin = &s->defaulttex;
			shader = s->shader;
			if (sk)
			{
				int i;
				for (i = 0; i < sk->nummappings; i++)
				{
					if (!strcmp(sk->mappings[i].surface, s->name))
					{
						skin = &sk->mappings[i].texnums;
						shader = sk->mappings[i].shader;
						break;
					}
				}
			}

			if ( rent->forcedshader ) {
				shader = rent->forcedshader;
			}

			if (b && b->skin->base == skin->base && b->shader == shader && b->meshes < countof(b->user.alias.surfrefs))
				;	//merging it.
			else
			{
				b = BE_GetTempBatch();
				if (!b)
					return;
				b->skin = skin;
				b->shader = shader;

				b->buildmeshes = R_HL_BuildMeshes;
				b->ent = rent;
				b->mesh = NULL;
				b->firstmesh = 0;
				b->meshes = 0;
				b->texture = NULL;
				for (j = 0; j < MAXRLIGHTMAPS; j++)
				{
					b->lightmap[j] = -1;
					b->lmlightstyle[j] = INVALID_LIGHTSTYLE;
				}
				b->flags = 0;
				sort = shader->sort;
				//fixme: we probably need to force some blend modes based on the surface flags.
				if (rent->flags & RF_FORCECOLOURMOD)
					b->flags |= BEF_FORCECOLOURMOD;
				if (rent->flags & RF_ADDITIVE)
				{
					b->flags |= BEF_FORCEADDITIVE;
					if (sort < SHADER_SORT_ADDITIVE)
						sort = SHADER_SORT_ADDITIVE;
				}
				if (rent->flags & RF_TRANSLUCENT)
				{
					b->flags |= BEF_FORCETRANSPARENT;
					if (SHADER_SORT_PORTAL < sort && sort < SHADER_SORT_BLEND)
						sort = SHADER_SORT_BLEND;
				}
				if (rent->flags & RF_NODEPTHTEST)
				{
					b->flags |= BEF_FORCENODEPTH;
					if (sort < SHADER_SORT_NEAREST)
						sort = SHADER_SORT_NEAREST;
				}
				if (rent->flags & RF_NOSHADOW)
					b->flags |= BEF_NOSHADOWS;
				b->vbo = NULL;//&model->vbo;
				b->next = batches[sort];
				batches[sort] = b;
			}

			b->user.alias.surfrefs[b->meshes++] = (body<<8)|(m&0xff);
		}
	}
}

void HLMDL_DrawHitBoxes(entity_t *rent)
{
	hlmodel_t *model = Mod_Extradata(rent->model);
	hlmdl_hitbox_t *hitbox = (hlmdl_hitbox_t*)((char*)model->header+model->header->ofs_hitboxes);
	matrix3x4 entitymatrix;

	shader_t *shader = R_RegisterShader("hitbox_nodepth", SUF_NONE,
			"{\n"
				"polygonoffset\n"
				"{\n"
					"map $whiteimage\n"
					"blendfunc gl_src_alpha gl_one\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
					"nodepthtest\n"
				"}\n"
			"}\n");

	float relbones[MAX_BONES*12];
	int bonecount = HLMDL_GetBoneData(rent->model, 0, MAX_BONES, &rent->framestate, relbones);
	int b;

	entitymatrix[0][0] = rent->axis[0][0];
	entitymatrix[0][1] = rent->axis[1][0];
	entitymatrix[0][2] = rent->axis[2][0];
	entitymatrix[1][0] = rent->axis[0][1];
	entitymatrix[1][1] = rent->axis[1][1];
	entitymatrix[1][2] = rent->axis[2][1];
	entitymatrix[2][0] = rent->axis[0][2];
	entitymatrix[2][1] = rent->axis[1][2];
	entitymatrix[2][2] = rent->axis[2][2];
	entitymatrix[0][3] = rent->origin[0];
	entitymatrix[1][3] = rent->origin[1];
	entitymatrix[2][3] = rent->origin[2];

	//convert relative to absolutes
	for (b = 0; b < bonecount; b++)
	{
		//If we have a parent, take the addition. Otherwise just copy the values
		if(model->bones[b].parent>=0)
			R_ConcatTransforms((void*)transform_matrix[model->bones[b].parent], (void*)(relbones+b*12), transform_matrix[b]);
		else
			R_ConcatTransforms((void*)entitymatrix, (void*)(relbones+b*12), transform_matrix[b]);
	}

	for (b = 0; b < model->header->num_hitboxes; b++, hitbox++)
		CLQ1_AddOrientedCube(shader, hitbox->mins, hitbox->maxs, transform_matrix[hitbox->bone][0], 1, 1, 1, 0.2);
}
#endif

#endif
