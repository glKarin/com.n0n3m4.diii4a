#include "quakedef.h"
#ifdef MAP_PROC

#ifdef HAVE_CLIENT
#include "shader.h"
#endif
#include "com_mesh.h"

//FIXME: shadowmaps should build a cache of the nearby area surfaces and flag those models as RF_NOSHADOW or something
//fixme: merge areas and static ents too somehow.

void Mod_SetParent (mnode_t *node, mnode_t *parent);
static int	D3_ClusterForPoint (struct model_s *model, const vec3_t point, int *areaout);

#ifdef HAVE_CLIENT
static void R_BuildDefaultTexnums_Doom3(shader_t *shader)
{
	extern qboolean		r_loadbumpmapping;
	extern cvar_t gl_specular;
	extern cvar_t r_fb_bmodels;

	char *h;
	char imagename[MAX_QPATH];
	char mapname[MAX_QPATH];
	char *subpath = NULL;
	texnums_t *tex;
	unsigned int a, aframes;
	unsigned int imageflags = 0;
	strcpy(imagename, shader->name);
	h = strchr(imagename, '#');
	if (h)
		*h = 0;
	if (*imagename == '/' || strchr(imagename, ':'))
	{	//this is not security. this is anti-spam for the verbose security in the filesystem code.
		Con_Printf("Warning: shader has absolute path: %s\n", shader->name);
		*imagename = 0;
	}

	tex = shader->defaulttextures;
	aframes = max(1, shader->numdefaulttextures);
	//if any were specified explicitly, replicate that into all.
	//this means animmap can be used, with any explicit textures overriding all.

	for (a = 1; a < aframes; a++)
	{
		if (!TEXVALID(tex[a].base))
			tex[a].base			= tex[0].base;
		if (!TEXVALID(tex[a].bump))
			tex[a].bump			= tex[0].bump;
		if (!TEXVALID(tex[a].fullbright))
			tex[a].fullbright	= tex[0].fullbright;
		if (!TEXVALID(tex[a].specular))
			tex[a].specular		= tex[0].specular;
		if (!TEXVALID(tex[a].loweroverlay))
			tex[a].loweroverlay	= tex[0].loweroverlay;
		if (!TEXVALID(tex[a].upperoverlay))
			tex[a].upperoverlay	= tex[0].upperoverlay;
		if (!TEXVALID(tex[a].reflectmask))
			tex[a].reflectmask	= tex[0].reflectmask;
		if (!TEXVALID(tex[a].reflectcube))
			tex[a].reflectcube	= tex[0].reflectcube;
	}
	for (a = 0; a < aframes; a++, tex++)
	{
		COM_StripExtension(tex->mapname, mapname, sizeof(mapname));

		if (!TEXVALID(tex->base))
		{
			/*dlights/realtime lighting needs some stuff*/
			if (!TEXVALID(tex->base) && *tex->mapname)// && (shader->flags & SHADER_HASDIFFUSE))
				tex->base = R_LoadHiResTexture(tex->mapname, NULL, 0);

			if (!TEXVALID(tex->base))
				tex->base = R_LoadHiResTexture(va("%s_d", imagename), subpath, (*imagename=='{')?0:IF_NOALPHA);
		}

		imageflags |= IF_LOWPRIORITY;

		COM_StripExtension(imagename, imagename, sizeof(imagename));

		if (!TEXVALID(tex->bump))
		{
			if ((shader->flags & SHADER_HASNORMALMAP) && r_loadbumpmapping)
			{
				if (!TEXVALID(tex->bump) && *mapname && (shader->flags & SHADER_HASNORMALMAP))
					tex->bump = R_LoadHiResTexture(va("%s_local", mapname), NULL, imageflags|IF_TRYBUMP|IF_NOSRGB);
				if (!TEXVALID(tex->bump))
					tex->bump = R_LoadHiResTexture(va("%s_local", imagename), subpath, imageflags|IF_TRYBUMP|IF_NOSRGB);
			}
		}

		if (!TEXVALID(tex->loweroverlay))
		{
			if (shader->flags & SHADER_HASTOPBOTTOM)
			{
				if (!TEXVALID(tex->loweroverlay) && *mapname)
					tex->loweroverlay = R_LoadHiResTexture(va("%s_pants", mapname), NULL, imageflags);
				if (!TEXVALID(tex->loweroverlay))
					tex->loweroverlay = R_LoadHiResTexture(va("%s_pants", imagename), subpath, imageflags);	/*how rude*/
			}
		}

		if (!TEXVALID(tex->upperoverlay))
		{
			if (shader->flags & SHADER_HASTOPBOTTOM)
			{
				if (!TEXVALID(tex->upperoverlay) && *mapname)
					tex->upperoverlay = R_LoadHiResTexture(va("%s_shirt", mapname), NULL, imageflags);
				if (!TEXVALID(tex->upperoverlay))
					tex->upperoverlay = R_LoadHiResTexture(va("%s_shirt", imagename), subpath, imageflags);
			}
		}

		if (!TEXVALID(tex->specular))
		{
			if ((shader->flags & SHADER_HASGLOSS) && gl_specular.value)
			{
				if (!TEXVALID(tex->specular) && *mapname)
					tex->specular = R_LoadHiResTexture(va("%s_s", mapname), NULL, imageflags);
				if (!TEXVALID(tex->specular))
					tex->specular = R_LoadHiResTexture(va("%s_s", imagename), subpath, imageflags);
			}
		}

		if (!TEXVALID(tex->fullbright))
		{
			if ((shader->flags & SHADER_HASFULLBRIGHT) && r_fb_bmodels.value && gl_load24bit.value)
			{
				if (!TEXVALID(tex->fullbright) && *mapname)
					tex->fullbright = R_LoadHiResTexture(va("%s_luma:%s_glow", mapname, mapname), NULL, imageflags);
				if (!TEXVALID(tex->fullbright))
					tex->fullbright = R_LoadHiResTexture(va("%s_luma:%s_glow", imagename, imagename), subpath, imageflags);
			}
		}
	}
}
static void ModD3_GenAreaVBO(void *ctx, void *data, size_t a, size_t barg)
{
	model_t *sub = ctx;
	batch_t *b = sub->batches[0];
	int surf;
	sub->batches[0] = NULL;

	for (surf = 0; surf < sub->numbatches; surf++)
		sub->numsurfaces += b[surf].meshes;
	sub->texinfo = ZG_Malloc(&sub->memgroup, sizeof(*sub->texinfo)*sub->numsurfaces);
	sub->surfaces = ZG_Malloc(&sub->memgroup, sizeof(*sub->surfaces)*sub->numsurfaces);
	sub->firstmodelsurface = sub->nummodelsurfaces = 0;

	for (surf = 0; surf < sub->numbatches; surf++)
	{
		b[surf].shader = R_RegisterShader_Vertex(sub, b[surf].texture->name);
		R_BuildDefaultTexnums_Doom3(b[surf].shader);

		//now we know its sort key, we can link it properly. *sigh*
		b[surf].next = sub->batches[b[surf].shader->sort];
		sub->batches[b[surf].shader->sort] = &b[surf];

		//all this extra stuff so r_showshaders works. *sigh*
		sub->surfaces[sub->nummodelsurfaces].texinfo = &sub->texinfo[sub->nummodelsurfaces];
		sub->surfaces[sub->nummodelsurfaces].texinfo->texture = b[surf].texture;
		sub->surfaces[sub->nummodelsurfaces].mesh = b[surf].mesh[0];
		sub->nummodelsurfaces++;
	}

	BE_GenBrushModelVBO(sub);
}

static qboolean Mod_LoadMap_Proc(model_t *model, char *data)
{
	char token[256];
	int ver = 0;
	data = COM_ParseOut(data, token, sizeof(token));
	if (!strcmp(token, "mapProcFile003"))
		ver = 3;
	if (!strcmp(token, "PROC"))
	{
		data = COM_ParseOut(data, token, sizeof(token));
		ver = atoi(token);
	}

	if (ver != 3 && ver != 4)
	{
		Con_Printf("proc format not compatible %s\n", token);
		return false;
	}
	/*FIXME: add sanity checks*/

	if (ver == 4)
	{
		data = COM_ParseOut(data, token, sizeof(token));
	}

	while(1)
	{
		data = COM_ParseOut(data, token, sizeof(token));
		if (!data)
			break;
		else if (!strcmp(token, "model"))
		{
			batch_t *b;
			mesh_t *m, **ml;
			model_t *sub;
			float f;
			int numsurfs, surf;
			int numverts, v, j;
			int numindicies;
			char *vdata;

			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "{"))
				return false;

			*token = '*';
			data = COM_ParseOut(data, token+1, sizeof(token)-1);
			Q_strncatz(token, ":", sizeof(token));
			Q_strncatz(token, model->publicname, sizeof(token));
			sub = Mod_FindName(token);

			if (sub->loadstate != MLS_NOTLOADED)
				return false;

			data = COM_ParseOut(data, token, sizeof(token));
			numsurfs = atoi(token);
			if (numsurfs < 0 || numsurfs > 10000)
				return false;
			if (numsurfs)
			{
				b = ZG_Malloc(&model->memgroup, sizeof(*b) * numsurfs);
				m = ZG_Malloc(&model->memgroup, sizeof(*m) * numsurfs);
				ml = ZG_Malloc(&model->memgroup, sizeof(*ml) * numsurfs);
			}
			else
			{
				b = NULL;
				m = NULL;
				ml = NULL;
			}
			sub->numsurfaces = numsurfs;

			//ver4 may have a 'sky' field here
			vdata = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "{") && strcmp(token, "}"))
			{
				//sky = atoi(token);
				data = vdata;
			}

			sub->mins[0] = 99999999;
			sub->mins[1] = 99999999;
			sub->mins[2] = 99999999;
			sub->maxs[0] = -99999999;
			sub->maxs[1] = -99999999;
			sub->maxs[2] = -99999999;
			for (surf = 0; surf < numsurfs; surf++)
			{
				data = COM_ParseOut(data, token, sizeof(token));
				if (strcmp(token, "{"))
					break;
				if (!data)
					return false;
				b[surf].maxmeshes = 1;
				b[surf].mesh = &ml[surf];
				ml[surf] = &m[surf];
				b[surf].lightmap[0] = -1;
				b[surf].lightmap[1] = -1;
				b[surf].lightmap[2] = -1;
				b[surf].lightmap[3] = -1;
				b[surf].lmlightstyle[0] = 0;
				b[surf].lmlightstyle[1] = INVALID_LIGHTSTYLE;
				b[surf].lmlightstyle[2] = INVALID_LIGHTSTYLE;
				b[surf].lmlightstyle[3] = INVALID_LIGHTSTYLE;

				data = COM_ParseOut(data, token, sizeof(token));
				b[surf].texture = ZG_Malloc(&sub->memgroup, sizeof(*b[surf].texture));
				Q_strncpyz(b[surf].texture->name, token, sizeof(b[surf].texture->name));

				data = COM_ParseOut(data, token, sizeof(token));
				numverts = atoi(token);
				data = COM_ParseOut(data, token, sizeof(token));
				numindicies = atoi(token);

				m[surf].numvertexes = numverts;
				m[surf].numindexes = numindicies;
				vdata = ZG_Malloc(&sub->memgroup, numverts * (sizeof(vecV_t) + sizeof(vec2_t) + sizeof(vec3_t)*3 + sizeof(vec4_t)) + numindicies * sizeof(index_t));

				m[surf].colors4f_array[0] = (vec4_t*)vdata;vdata += sizeof(vec4_t)*numverts;
				m[surf].xyz_array = (vecV_t*)vdata;vdata += sizeof(vecV_t)*numverts;
				m[surf].st_array = (vec2_t*)vdata;vdata += sizeof(vec2_t)*numverts;
				m[surf].normals_array = (vec3_t*)vdata;vdata += sizeof(vec3_t)*numverts;
				m[surf].snormals_array = (vec3_t*)vdata;vdata += sizeof(vec3_t)*numverts;
				m[surf].tnormals_array = (vec3_t*)vdata;vdata += sizeof(vec3_t)*numverts;
				m[surf].indexes = (index_t*)vdata;

				for (v = 0; v < numverts; v++)
				{
					data = COM_ParseOut(data, token, sizeof(token));
					if (strcmp(token, "("))
						return false;

					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].xyz_array[v][0] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].xyz_array[v][1] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].xyz_array[v][2] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].st_array[v][0] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].st_array[v][1] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].normals_array[v][0] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].normals_array[v][1] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].normals_array[v][2] = atof(token);

					for (j = 0; j < 3; j++)
					{
						f = m[surf].xyz_array[v][j];
						if (f > sub->maxs[j])
							sub->maxs[j] = f;
						if (f < sub->mins[j])
							sub->mins[j] = f;
					}

					m[surf].colors4f_array[0][v][0] = 1;
					m[surf].colors4f_array[0][v][1] = 1;
					m[surf].colors4f_array[0][v][2] = 1;
					m[surf].colors4f_array[0][v][3] = 1;

					data = COM_ParseOut(data, token, sizeof(token));
					/*if its not closed yet, there's an optional colour value*/
					if (strcmp(token, ")"))
					{
						m[surf].colors4f_array[0][v][0] = atof(token)/255;
						data = COM_ParseOut(data, token, sizeof(token));
						m[surf].colors4f_array[0][v][1] = atof(token)/255;
						data = COM_ParseOut(data, token, sizeof(token));
						m[surf].colors4f_array[0][v][2] = atof(token)/255;
						data = COM_ParseOut(data, token, sizeof(token));
						m[surf].colors4f_array[0][v][3] = atof(token)/255;

						data = COM_ParseOut(data, token, sizeof(token));
						if (strcmp(token, ")"))
							return false;
					}
				}
				for (v = 0; v < numindicies; v++)
				{
					data = COM_ParseOut(data, token, sizeof(token));
					m[surf].indexes[v] = atoi(token);
				}

				//generate the s+t vectors according to the normals that we just parsed.
				R_Generate_Mesh_ST_Vectors(&m[surf]);

				data = COM_ParseOut(data, token, sizeof(token));
				if (strcmp(token, "}"))
					return false;
			}
			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "}"))
				return false;
//			sub->loadstate = MLS_LOADED;
			sub->fromgame = fg_new;
			sub->type = mod_brush;
			sub->lightmaps.surfstyles = 1;

			memset(sub->batches, 0, sizeof(sub->batches));
			sub->batches[0] = b;
			sub->numbatches = numsurfs;

			COM_AddWork(WG_MAIN, ModD3_GenAreaVBO, sub, NULL, MLS_LOADED, 0);
			COM_AddWork(WG_MAIN, Mod_ModelLoaded, sub, NULL, MLS_LOADED, 0);
		}
		else if (!strcmp(token, "shadowModel"))
		{
			int numverts, v;
			int numindexes, i;
			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "{"))
				return false;

			data = COM_ParseOut(data, token, sizeof(token));
			//name
			data = COM_ParseOut(data, token, sizeof(token));
			numverts = atoi(token);
			data = COM_ParseOut(data, token, sizeof(token));
			//nocaps
			data = COM_ParseOut(data, token, sizeof(token));
			//nofrontcaps
			data = COM_ParseOut(data, token, sizeof(token));
			numindexes = atoi(token);
			data = COM_ParseOut(data, token, sizeof(token));
			//planebits

			for (v = 0; v < numverts; v++)
			{
				data = COM_ParseOut(data, token, sizeof(token));
				if (strcmp(token, "("))
					return false;

				data = COM_ParseOut(data, token, sizeof(token));
				//x
				data = COM_ParseOut(data, token, sizeof(token));
				//y
				data = COM_ParseOut(data, token, sizeof(token));
				//z

				data = COM_ParseOut(data, token, sizeof(token));
				if (strcmp(token, ")"))
					return false;
			}

			for (i = 0; i < numindexes; i++)
			{
				data = COM_ParseOut(data, token, sizeof(token));
			}

			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "}"))
				return false;
		}
		else if (!strcmp(token, "nodes"))
		{
			int numnodes, n;
			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "{"))
				return false;

			data = COM_ParseOut(data, token, sizeof(token));
			numnodes = atoi(token);
			model->nodes = ZG_Malloc(&model->memgroup, sizeof(*model->nodes)*numnodes);
			model->planes = ZG_Malloc(&model->memgroup, sizeof(*model->planes)*numnodes);

			for (n = 0; n < numnodes; n++)
			{
				data = COM_ParseOut(data, token, sizeof(token));
				if (strcmp(token, "("))
					return false;

				model->nodes[n].plane = &model->planes[n];

				data = COM_ParseOut(data, token, sizeof(token));
				model->planes[n].normal[0] = atof(token);
				data = COM_ParseOut(data, token, sizeof(token));
				model->planes[n].normal[1] = atof(token);
				data = COM_ParseOut(data, token, sizeof(token));
				model->planes[n].normal[2] = atof(token);
				data = COM_ParseOut(data, token, sizeof(token));
				model->planes[n].dist = atof(token);

				data = COM_ParseOut(data, token, sizeof(token));
				if (strcmp(token, ")"))
					return false;

				data = COM_ParseOut(data, token, sizeof(token));
				model->nodes[n].childnum[0] = atoi(token);
				data = COM_ParseOut(data, token, sizeof(token));
				model->nodes[n].childnum[1] = atoi(token);
			}

			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "}"))
				return false;

			Mod_SetParent(model->nodes, NULL);
		}
		else if (!strcmp(token, "interAreaPortals"))
		{
			//int numareas;
			int pno, v;
			portal_t *p;

			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "{"))
				return false;

			data = COM_ParseOut(data, token, sizeof(token));
			model->numclusters = atoi(token);
			data = COM_ParseOut(data, token, sizeof(token));
			model->numportals = atoi(token);

			model->portal = p = ZG_Malloc(&model->memgroup, sizeof(*p) * model->numportals);

			for (pno = 0; pno < model->numportals; pno++, p++)
			{
				data = COM_ParseOut(data, token, sizeof(token));
				p->numpoints = atoi(token);
				data = COM_ParseOut(data, token, sizeof(token));
				p->area[0] = atoi(token);
				data = COM_ParseOut(data, token, sizeof(token));
				p->area[1] = atoi(token);

				p->points = ZG_Malloc(&model->memgroup, sizeof(*p->points) * p->numpoints);

				ClearBounds(p->min, p->max);
				for (v = 0; v < p->numpoints; v++)
				{
					data = COM_ParseOut(data, token, sizeof(token));
					if (strcmp(token, "("))
						return false;

					data = COM_ParseOut(data, token, sizeof(token));
					p->points[v][0] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					p->points[v][1] = atof(token);
					data = COM_ParseOut(data, token, sizeof(token));
					p->points[v][2] = atof(token);
					p->points[v][3] = 1;

					AddPointToBounds(p->points[v], p->min, p->max);

					data = COM_ParseOut(data, token, sizeof(token));
					if (strcmp(token, ")"))
						return false;
				}

			}

			data = COM_ParseOut(data, token, sizeof(token));
			if (strcmp(token, "}"))
				return false;
		}
		else
		{
			Con_Printf("unexpected token %s\n", token);
			return false;
		}
	}

	return true;
}

qboolean R_CullBox (vec3_t mins, vec3_t maxs);

static int walkno;
/*fixme: convert each portal to a 2d box, because its much much simpler than true poly clipping*/
/*fixme: use occlusion tests, with temporal coherance (draw the portal as black or something if we think its invisible)*/
static void D3_WalkPortal(model_t *mod, int start, vec_t bounds[4], unsigned char *vis)
{
	int i;
	portal_t *p;
	int side;
	vec_t newbounds[4];
	
	vis[start>>3] |= 1<<(start&7);

	for (i = 0; i < mod->numportals; i++)
	{
		p = mod->portal+i;
		if (p->walkno == walkno)
			continue;
		if (p->area[0] == start)
			side = 0;
		else if (p->area[1] == start)
			side = 1;
		else
			continue;

		if (R_CullBox(p->min, p->max))
		{
			continue;
		}

		p->walkno = walkno;
		D3_WalkPortal(mod, p->area[!side], newbounds, vis);
	}
}

static void D3_PrepareFrame(model_t *mod, refdef_t *refdef, int inarea, int inclusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out)
{
	int start;
	static qbyte visbuf[256];
	qbyte *usevis;
	vec_t newbounds[4];

	int area;
	entity_t ent;

	start = D3_ClusterForPoint(mod, refdef->vieworg, NULL);
	/*figure out which area we're in*/
	if (start < 0)
	{
		/*outside the world, just make it all visible, and take the fps hit*/
		memset(visbuf, 255, 4);
		usevis = visbuf;
	}
	else if (r_novis.value)
		usevis = visbuf;
	else
	{
		memset(visbuf, 0, 4);
		/*make a bounds the size of the view*/
		newbounds[0] = -1;
		newbounds[1] = 1;
		newbounds[2] = -1;
		newbounds[3] = 1;
		walkno++;
		D3_WalkPortal(mod, start, newbounds, visbuf);
//		Con_Printf("%x %x %x %x\n", vis[0], vis[1], vis[2], vis[3]);
		usevis = visbuf;
	}

	//now generate the various entities for that region.
	memset(&ent, 0, sizeof(ent));
	for (area = 0; area < 256*8; area++)
	{
		if (usevis[area>>3] & (1u<<(area&7)))
		{
			ent.model = Mod_FindName(va("*_area%i", area));
			ent.scale = 1;
			AngleVectors(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
			VectorInverse(ent.axis[1]);
			ent.shaderRGBAf[0] = 1;
			ent.shaderRGBAf[1] = 1;
			ent.shaderRGBAf[2] = 1;
			ent.shaderRGBAf[3] = 1;

			V_AddEntity(&ent);
		}
	}
	*entvis_out = *surfvis_out = usevis;
}

static void D3_StainNode			(struct model_s *model, float *parms)
{
}

static void D3_LightPointValues (struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	/*basically require rtlighting for any light*/
	VectorClear(res_diffuse);
	VectorClear(res_ambient);
	VectorClear(res_dir);
	res_dir[2] = 1;
}
#endif

//edict system as opposed to q2 game dll system.
static void D3_FindTouchedLeafs (struct model_s *model, struct pvscache_s *ent, const vec3_t cullmins, const vec3_t cullmaxs)
{
}
static qbyte *D3_ClusterPVS (struct model_s *model, int num, pvsbuffer_t *buffer, pvsmerge_t merge)
{
	memset(buffer->buffer, 0xff, buffer->buffersize);
	return buffer->buffer;
}
static int	D3_ClusterForPoint (struct model_s *model, const vec3_t point, int *areaout)
{
	float p;
	int c;
	mnode_t *node;
	node = model->nodes;
	if (areaout)
		*areaout = 0;
	while(1)
	{
		p = DotProduct(point, node->plane->normal) + node->plane->dist;
		c = node->childnum[p<0];
		if (c <= 0)
			return -1-c;
		node = model->nodes + c;
	}
	return 0;
}
static unsigned int D3_FatPVS (struct model_s *model, const vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge)
{
	return 0;
}

static qboolean D3_EdictInFatPVS (struct model_s *model, const struct pvscache_s *edict, const qbyte *pvsbuffer, const int *areas)
{
	int i;
	for (i = 0; i < edict->num_leafs; i++)
		if (pvsbuffer[edict->leafnums[i]>>3] & (1u<<(edict->leafnums[i]&7)))
			return true;
	return false;
}






typedef struct cm_surface_s
{
	vec3_t mins, maxs;
	vec4_t plane;
	int numedges;
	vec4_t *edge;

//	shader_t *shader;
	struct cm_surface_s *next;
} cm_surface_t;

typedef struct cm_brush_s
{
	vec3_t mins, maxs;
	int numplanes;
	vec4_t *plane;
	unsigned int contents;
	struct cm_brush_s *next;
} cm_brush_t;

typedef struct cm_node_s
{
	int axis; /*0=x,1=y,2=z*/
	float dist;
	vec3_t mins, maxs;
	struct cm_node_s *parent;
	struct cm_node_s *child[2];

	cm_brush_t *brushlist;
	cm_surface_t *surfacelist;
} cm_node_t;

static struct
{
	float truefraction;

	qboolean ispoint;
	vec3_t start;
	vec3_t end;
	vec3_t absmins, absmaxs;
	vec3_t szmins, szmaxs;
	vec3_t extents;

	cm_surface_t *surf;
} traceinfo;

#define	DIST_EPSILON	(0.03125)

static void D3_TraceToLeaf (cm_node_t *leaf)
{
	float diststart;
	float distend;
	float frac;
	vec3_t impactpoint;
	qboolean back;
	int i, j;
	float pdist, expand;
	vec3_t ofs;

	cm_surface_t *surf;
	for (surf = leaf->surfacelist; surf; surf = surf->next)
	{
		/*lots of maths in this function, we should check the surf's bbox*/
		if (surf->mins[0] > traceinfo.absmaxs[0] || traceinfo.absmins[0] > surf->maxs[0] ||
			surf->mins[1] > traceinfo.absmaxs[1] || traceinfo.absmins[1] > surf->maxs[1] ||
			surf->mins[2] > traceinfo.absmaxs[2] || traceinfo.absmins[2] > surf->maxs[2])
			continue;

		if (!traceinfo.ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (i=0 ; i<3 ; i++)
			{
				if (surf->plane[i] < 0)
					ofs[i] = traceinfo.szmaxs[i];
				else
					ofs[i] = traceinfo.szmins[i];
			}
			expand = DotProduct (ofs, surf->plane);
//			pdist = surf->plane[3] - expand;
		}
		else
		{	// special point case
//			pdist = surf->plane[3];
			expand = 0;
		}

		diststart = DotProduct(traceinfo.start, surf->plane);
		/*started behind?*/
		back = diststart < surf->plane[3];
		if (diststart <= surf->plane[3]-expand)
		{
			/*the trace started behind our expanded front plane*/

			/*don't stop just because the point is closer than the extended plane*/
			/*epsilon here please*/
			if (diststart <= surf->plane[3])
				continue;

			distend = DotProduct(traceinfo.end, surf->plane);
			if (distend < diststart)
				frac = 0; /*don't let us go further into the wall*/
			else
				continue;
		}
		else
		{
			distend = DotProduct(traceinfo.end, surf->plane);
			/*ended on the other side*/
			if (back)
			{
				if (distend+expand > -surf->plane[3])
					continue;
			}
			else
			{
				if (distend+expand > surf->plane[3])
					continue;
			}

			if (diststart == distend)
				frac = 0;
			else
				frac = (diststart - (surf->plane[3]-expand)) / (diststart-distend);
		}

		/*give up if we already found a closer plane*/
		if (frac >= traceinfo.truefraction)
			continue;

		/*okay, this is where it hits this plane*/
		impactpoint[0] = traceinfo.start[0] + frac*(traceinfo.end[0] - traceinfo.start[0]);
		impactpoint[1] = traceinfo.start[1] + frac*(traceinfo.end[1] - traceinfo.start[1]);
		impactpoint[2] = traceinfo.start[2] + frac*(traceinfo.end[2] - traceinfo.start[2]);

		/*if the impact was not on the surface*/
		for (i = 0; i < surf->numedges; i++)
		{
			if (!traceinfo.ispoint)
			{	// general box case

				// push the plane out apropriately for mins/maxs

				// FIXME: use signbits into 8 way lookup for each mins/maxs
				for (j=0 ; j<3 ; j++)
				{
					if (surf->edge[i][j] < 0)
						ofs[j] = traceinfo.szmaxs[j];
					else
						ofs[j] = traceinfo.szmins[j];
				}
				pdist = DotProduct (ofs, surf->edge[i]);
				pdist = surf->edge[i][3] - pdist;
			}
			else
			{	// special point case
				pdist = surf->edge[i][3];
			}

			if (DotProduct(impactpoint, surf->edge[i]) > pdist)
			{
				break;
			}
		}
		/*if we were inside all edges, we hit the surface*/
		if (i == surf->numedges)
		{

			traceinfo.truefraction = frac;
			traceinfo.surf = surf;
			
			/*we can't early out. there are multiple surfs in each leaf, and they could overlap. earlying out will result in errors if we hit a further one before the nearer*/
		}
	}
}

/*returns the most distant node which contains the entire box*/
static cm_node_t *D3_ChildNodeForBox(cm_node_t *node, vec3_t mins, vec3_t maxs)
{
	float t1, t2;
	for(;;)
	{
		t1 = mins[node->axis] - node->dist;
		t2 = maxs[node->axis] - node->dist;

		//if its completely to one side, walk down that side
		if (t1 > maxs[node->axis] && t2 > maxs[node->axis])
		{
			//if this is a leaf, we can't insert in a child anyway.
			if (!node->child[0])
				break;
			node = node->child[0];
			continue;
		}
		if (t1 < mins[node->axis] && t2 < mins[node->axis])
		{
			//if this is a leaf, we can't insert in a child anyway.
			if (!node->child[1])
				break;
			node = node->child[1];
			continue;
		}

		//the box crosses this node
		break;
	}

	return node;
}

static void D3_InsertClipSurface(cm_node_t *node, cm_surface_t *surf)
{
	node = D3_ChildNodeForBox(node, surf->mins, surf->maxs);

	surf->next = node->surfacelist;
	node->surfacelist = surf;
}
static void D3_InsertClipBrush(cm_node_t *node, cm_brush_t *brush)
{
	node = D3_ChildNodeForBox(node, brush->mins, brush->maxs);

	brush->next = node->brushlist;
	node->brushlist = brush;
}

static void D3_RecursiveSurfCheck (cm_node_t *node, float p1f, float p2f, const vec3_t p1, const vec3_t p2)
{
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (traceinfo.truefraction <= p1f)
		return;		// already hit something nearer

	/*err, no child here*/
	if (!node)
		return;

	D3_TraceToLeaf (node);

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//

	t1 = p1[node->axis] - node->dist;
	t2 = p2[node->axis] - node->dist;
	offset = traceinfo.extents[node->axis];

#if 0
D3_RecursiveHullCheck (node->childnum[0], p1f, p2f, p1, p2);
D3_RecursiveHullCheck (node->childnum[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		D3_RecursiveSurfCheck (node->child[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		D3_RecursiveSurfCheck ( node->child[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	D3_RecursiveSurfCheck (node->child[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;

	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	D3_RecursiveSurfCheck (node->child[side^1], midf, p2f, mid, p2);
}

static qboolean D3_Trace (struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p1, const vec3_t p2, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int hitcontentsmask, struct trace_s *trace)
{
	int i;
	float e1,e2;
	traceinfo.truefraction = 1;
	VectorCopy(p1, traceinfo.start);
	VectorCopy(p2, traceinfo.end);
	for (i = 0; i < 3; i++)
	{
		e1 = fabs(mins[i]);
		e2 = fabs(maxs[i]);
		traceinfo.extents[i] = ((e1>e2)?e1:e2);
		traceinfo.szmins[i] = mins[i];
		traceinfo.szmaxs[i] = maxs[i];

		traceinfo.absmins[i] = ((p1[i]<p2[i])?p1[i]:p2[i]) + mins[i];
		traceinfo.absmaxs[i] = ((p1[i]>p2[i])?p1[i]:p2[i]) + maxs[i];
	}
	traceinfo.ispoint = !traceinfo.extents[0] && !traceinfo.extents[1] && !traceinfo.extents[2];

	traceinfo.surf = NULL;

	D3_RecursiveSurfCheck(model->cnodes, 0, 1, p1, p2);

	memset(trace, 0, sizeof(*trace));
	if (!traceinfo.surf)
	{
		trace->fraction = 1;
		VectorCopy(p2, trace->endpos);
	}
	else
	{
		float frac;
		/*we now know which surface it hit. recalc the impact point, but with an epsilon this time, so we can never get too close to the surface*/

		VectorCopy(traceinfo.surf->plane, trace->plane.normal);
		if (!traceinfo.ispoint)
		{	// general box case
			vec3_t ofs;
			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (i=0 ; i<3 ; i++)
			{
				if (traceinfo.surf->plane[i] < 0)
					ofs[i] = traceinfo.szmaxs[i];
				else
					ofs[i] = traceinfo.szmins[i];
			}
			e1 = DotProduct (ofs, traceinfo.surf->plane);
			trace->plane.dist = traceinfo.surf->plane[3] - e1;
		}
		else
		{	// special point case
			trace->plane.dist = traceinfo.surf->plane[3];
		}

		frac = traceinfo.truefraction;
		/*
		diststart = DotProduct(traceinfo.start, trace->plane.normal);
		distend = DotProduct(traceinfo.end, trace->plane.normal);
		if (diststart == distend)
			frac = 0;
		else
		{
			frac = (diststart - trace->plane.dist) / (diststart-distend);
			if (frac < 0)
				frac = 0;
			else if (frac > 1)
				frac = 1;
		}*/

		/*okay, this is where it hits this plane*/
		trace->endpos[0] = traceinfo.start[0] + frac*(traceinfo.end[0] - traceinfo.start[0]);
		trace->endpos[1] = traceinfo.start[1] + frac*(traceinfo.end[1] - traceinfo.start[1]);
		trace->endpos[2] = traceinfo.start[2] + frac*(traceinfo.end[2] - traceinfo.start[2]);
		trace->fraction = frac;
	}
	trace->ent = NULL;
	return false;
}

static unsigned int D3_PointContents (struct model_s *model, const vec3_t axis[3], const vec3_t p)
{
	cm_node_t *node = model->cnodes;
	cm_brush_t *brush;
	float t1;
	unsigned int contents = 0;
	int i;
	vec3_t np;

	if (axis)
	{
		np[0] = DotProduct(p, axis[0]);
		np[1] = DotProduct(p, axis[1]);
		np[2] = DotProduct(p, axis[2]);
		p = np;
	}

	while(node)
	{
		for (brush = node->brushlist; brush; brush = brush->next)
		{
			if (brush->mins[0] > p[0] || p[0] > brush->maxs[0] ||
				brush->mins[1] > p[1] || p[1] > brush->maxs[1] ||
				brush->mins[2] > p[2] || p[2] > brush->maxs[2])
				continue;

			for (i = 0; i < brush->numplanes; i++)
			{
				if (DotProduct(p, brush->plane[i]) > brush->plane[i][3])
					break;
			}
			if (i == brush->numplanes)
				contents |= brush->contents;
		}

		t1 = p[node->axis] - node->dist;

		// see which side we need to go down
		if (t1 >= 0)
		{
			node = node->child[0];
		}
		else
		{
			node = node->child[1];
		}
	}

	return contents;
}

#define ensurenewtoken(t) buf = COM_ParseOut(buf, token, sizeof(token)); if (strcmp(token, t)) break

static int D3_ParseContents(char *str)
{
	char *e, *n;
	unsigned int contents = 0;
	while(str)
	{
		e = strchr(str, ',');
		if (e)
		{
			*e = 0;
			n = e+1;
		}
		else 
			n = NULL;

		if (!strcmp(str, "solid") || !strcmp(str, "opaque"))
			contents |= FTECONTENTS_SOLID;
		else if (!strcmp(str, "playerclip"))
			contents |= FTECONTENTS_PLAYERCLIP;
		else if (!strcmp(str, "monsterclip"))
			contents |= FTECONTENTS_PLAYERCLIP;
		else
			Con_Printf("Unknown contents type \"%s\"\n", str);
		str = n;
	}
	return contents;
}
typedef struct
{
	int v[2];
	int fl[2];
} d3edge_t;
qboolean QDECL D3_LoadMap_CollisionMap(model_t *mod, void *buf, size_t bufsize)
{
	int pedges[64];
	cm_surface_t *surf;
	cm_brush_t *brush;
	vec3_t *verts;
	d3edge_t *edges;
	int i, j;
	int filever = 0;
	int numverts, numedges, numpedges;
	model_t *cmod;
	char token[256];
	buf = COM_ParseOut(buf, token, sizeof(token));
	if (strcmp(token, "CM"))
		return false;
	
	buf = COM_ParseOut(buf, token, sizeof(token));
	filever = atof(token);
	if (filever != 1 && filever != 3)
		return false;

	buf = COM_ParseOut(buf, token, sizeof(token));
	/*some number, discard*/

	while(buf)
	{
		buf = COM_ParseOut(buf, token, sizeof(token));
		if (!strcmp(token, "collisionModel"))
		{
			buf = COM_ParseOut(buf, token, sizeof(token));
			if (!strcmp(token, "worldMap"))
				cmod = mod;
			else
			{
				Q_strncatz(token, ":", sizeof(token));
				Q_strncatz(token, mod->publicname, sizeof(token));
				cmod = Mod_FindName(token);
				if (cmod->loadstate != MLS_NOTLOADED)
					return false;
			}

			if (filever == 3)
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*don't know*/
			}

			ClearBounds(cmod->mins, cmod->maxs);
			ensurenewtoken("{");
			ensurenewtoken("vertices");
			ensurenewtoken("{");
				buf = COM_ParseOut(buf, token, sizeof(token));
				numverts = atoi(token);
				verts = malloc(numverts * sizeof(*verts));
				for (i = 0; i < numverts; i++)
				{
					ensurenewtoken("(");
					buf = COM_ParseOut(buf, token, sizeof(token));
					verts[i][0] = atof(token);
					buf = COM_ParseOut(buf, token, sizeof(token));
					verts[i][1] = atof(token);
					buf = COM_ParseOut(buf, token, sizeof(token));
					verts[i][2] = atof(token);
					ensurenewtoken(")");
				}
			ensurenewtoken("}");
			ensurenewtoken("edges");
			ensurenewtoken("{");
				buf = COM_ParseOut(buf, token, sizeof(token));
				numedges = atoi(token);
				edges = malloc(numedges * sizeof(*edges));
				for (i = 0; i < numedges; i++)
				{
					ensurenewtoken("(");
					buf = COM_ParseOut(buf, token, sizeof(token));
					edges[i].v[0] = atoi(token);
					buf = COM_ParseOut(buf, token, sizeof(token));
					edges[i].v[1] = atoi(token);
					ensurenewtoken(")");
					buf = COM_ParseOut(buf, token, sizeof(token));
					edges[i].fl[0] = atoi(token);
					buf = COM_ParseOut(buf, token, sizeof(token));
					edges[i].fl[1] = atoi(token);
				}
			ensurenewtoken("}");
			ensurenewtoken("nodes");
			ensurenewtoken("{");
			cmod->cnodes = ZG_Malloc(&mod->memgroup, sizeof(cm_node_t));
			for (;;)
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				if (strcmp(token, "("))
					break;

				buf = COM_ParseOut(buf, token, sizeof(token));
				buf = COM_ParseOut(buf, token, sizeof(token));
				//axis, dist
				ensurenewtoken(")");
			}
			if (strcmp(token, "}"))
				break;

			ensurenewtoken("polygons");
			if (filever == 1)
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*'polygonMemory', which is unusable for us*/
			}
			else
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*numPolygons*/
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*numPolygonEdges*/
			}
			ensurenewtoken("{");
			for (;;)
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				if (!strcmp(token, "}"))
					break;

				numpedges = atoi(token);
				surf = ZG_Malloc(&mod->memgroup, sizeof(*surf) + sizeof(vec4_t)*numpedges);
				surf->numedges = numpedges;
				surf->edge = (vec4_t*)(surf+1);

				ensurenewtoken("(");
				for (j = 0; j < numpedges; j++)
				{
					buf = COM_ParseOut(buf, token, sizeof(token));
					pedges[j] = atoi(token);
				}
				ensurenewtoken(")");
				ensurenewtoken("(");
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->plane[0] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->plane[1] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->plane[2] = atof(token);
				ensurenewtoken(")");
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->plane[3] = atof(token);

				ensurenewtoken("(");
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->mins[0] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->mins[1] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->mins[2] = atof(token);
				ensurenewtoken(")");
			
				ensurenewtoken("(");
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->maxs[0] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->maxs[1] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				surf->maxs[2] = atof(token);
				ensurenewtoken(")");

				buf = COM_ParseOut(buf, token, sizeof(token));
#ifdef HAVE_CLIENT
//				surf->shader = R_RegisterShader_Vertex(token);
//				R_BuildDefaultTexnums_Doom3(NULL, surf->shader);
#endif

				if (filever == 3)
				{
					ensurenewtoken("(");
					buf = COM_ParseOut(buf, token, sizeof(token));
					buf = COM_ParseOut(buf, token, sizeof(token));
					ensurenewtoken(")");

					ensurenewtoken("(");
					buf = COM_ParseOut(buf, token, sizeof(token));
					buf = COM_ParseOut(buf, token, sizeof(token));
					ensurenewtoken(")");

					ensurenewtoken("(");
					buf = COM_ParseOut(buf, token, sizeof(token));
					buf = COM_ParseOut(buf, token, sizeof(token));
					ensurenewtoken(")");

					buf = COM_ParseOut(buf, token, sizeof(token));
				}

				for (j = 0; j < numpedges; j++)
				{
					float *v1, *v2;
					vec3_t dir;
					if (pedges[j] < 0)
					{
						v2 = verts[edges[-pedges[j]].v[0]];
						v1 = verts[edges[-pedges[j]].v[1]];
					}
					else
					{
						v1 = verts[edges[pedges[j]].v[0]];
						v2 = verts[edges[pedges[j]].v[1]];
					}
					VectorSubtract(v1, v2, dir);
					VectorNormalize(dir);
					CrossProduct(surf->plane, dir, surf->edge[j]);
					surf->edge[j][3] = DotProduct(v1, surf->edge[j]);

					surf->edge[j][3] += DIST_EPSILON;
				}

				D3_InsertClipSurface(cmod->cnodes, surf);

				AddPointToBounds(surf->mins, cmod->mins, cmod->maxs);
				AddPointToBounds(surf->maxs, cmod->mins, cmod->maxs);
			}
			free(verts);
			free(edges);

			ensurenewtoken("brushes");
			if (filever == 1)
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*'brushMemory', which is unusable for us*/
			}
			else
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*numBrushes */
				buf = COM_ParseOut(buf, token, sizeof(token));
				/*numBrushPlanes*/
			}
			ensurenewtoken("{");
			for (;;)
			{
				buf = COM_ParseOut(buf, token, sizeof(token));
				if (!strcmp(token, "}"))
					break;
				j = atoi(token);
				brush = ZG_Malloc(&mod->memgroup, j*sizeof(vec4_t) + sizeof(*brush));
				brush->numplanes = j;
				brush->plane = (vec4_t*)(brush+1);
				ensurenewtoken("{");
				for (i = 0; i < brush->numplanes; i++)
				{
					ensurenewtoken("(");
					buf = COM_ParseOut(buf, token, sizeof(token));
					brush->plane[i][0] = atof(token);
					buf = COM_ParseOut(buf, token, sizeof(token));
					brush->plane[i][1] = atof(token);
					buf = COM_ParseOut(buf, token, sizeof(token));
					brush->plane[i][2] = atof(token);
					ensurenewtoken(")");
					buf = COM_ParseOut(buf, token, sizeof(token));
					brush->plane[i][3] = atof(token);
				}
				ensurenewtoken("}");

				ensurenewtoken("(");
				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->mins[0] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->mins[1] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->mins[2] = atof(token);
				ensurenewtoken(")");

				ensurenewtoken("(");
				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->maxs[0] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->maxs[1] = atof(token);
				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->maxs[2] = atof(token);
				ensurenewtoken(")");

				buf = COM_ParseOut(buf, token, sizeof(token));
				brush->contents = D3_ParseContents(token);

				if (filever == 3)
					buf = COM_ParseOut(buf, token, sizeof(token));

				D3_InsertClipBrush(cmod->cnodes, brush);

				AddPointToBounds(brush->mins, cmod->mins, cmod->maxs);
				AddPointToBounds(brush->maxs, cmod->mins, cmod->maxs);
			}
		}
		else
			break;
	}


	/*load up the .map so we can get some entities (anyone going to bother making a qc mod compatible with this?)*/
	COM_StripExtension(mod->name, token, sizeof(token));
	Q_strncatz(token, ".map", sizeof(token));
	Mod_SetEntitiesString(mod, FS_LoadMallocFile(token, NULL), true);

	mod->funcs.FindTouchedLeafs = D3_FindTouchedLeafs;
	mod->funcs.NativeTrace = D3_Trace;
	mod->funcs.PointContents = D3_PointContents;
	mod->funcs.FatPVS = D3_FatPVS;
	mod->funcs.ClusterForPoint = D3_ClusterForPoint;
	mod->funcs.EdictInFatPVS = D3_EdictInFatPVS;
	mod->funcs.ClusterPVS = D3_ClusterPVS;
#ifdef HAVE_CLIENT
	mod->funcs.StainNode = D3_StainNode;
	mod->funcs.LightPointValues = D3_LightPointValues;
	mod->funcs.PrepareFrame = D3_PrepareFrame;
#endif

	mod->type = mod_brush;	//err, kinda, sorta, maybe.
	mod->fromgame = fg_new;

	/*that's the physics sorted*/
#ifdef HAVE_CLIENT
	if (!isDedicated)
	{
		COM_StripExtension(mod->name, token, sizeof(token));
		Q_strncatz(token, ".proc", sizeof(token));
		buf = FS_LoadMallocFile(token, NULL);
		Mod_LoadMap_Proc(mod, buf);
		BZ_Free(buf);
	}
#endif

	return true;
}

#endif
