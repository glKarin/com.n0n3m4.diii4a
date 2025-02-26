/*
See gl_terrain.h for terminology, networking notes, etc.
*/

//FIXME: render in lightmap batches. generate vbos accordingly.
//FIXME: assign to lightmaps by matching textures. should be able to get up to 65536/(17*17)=226 per section before index limits hit, 16*16=256 allows for 1024*1024 lightmaps.
//FIXME: sort texture blend names to reduce combinations

#include "quakedef.h"

#ifdef TERRAIN
#include "glquake.h"
#include "shader.h"
#include "com_mesh.h"

#include "pr_common.h"

#include "gl_terrain.h"

static plugterrainfuncs_t terrainfuncs;


cvar_t mod_terrain_networked = CVARD("mod_terrain_networked", "0", "Terrain edits are networked. Clients will download sections on demand, and servers will notify clients of changes.");
cvar_t mod_terrain_defaulttexture = CVARD("mod_terrain_defaulttexture", "", "Newly created terrain tiles will use this texture. This should generally be updated by the terrain editor.");
cvar_t mod_terrain_savever = CVARD("mod_terrain_savever", "", "Which terrain section version to write if terrain was edited.");
cvar_t mod_terrain_sundir = CVARD("mod_terrain_sundir", "0.4 0.7 2", "The direction of the sun (vector will be normalised).");
cvar_t mod_terrain_ambient = CVARD("mod_terrain_ambient", "0.5", "Proportion of ambient light.");
cvar_t mod_terrain_shadows = CVARD("mod_terrain_shadows", "0", "Cast rays to determine whether parts of the terrain should be in shadow.");
cvar_t mod_terrain_shadow_dist = CVARD("mod_terrain_shadow_dist", "2048", "How far rays should be cast in order to look for occlusing geometry.");
cvar_t mod_terrain_brushlights = CVARD("mod_map_lights", "0", "Calculates lighting on brushes/patches.");
cvar_t mod_terrain_brushtexscale = CVARD("mod_map_texscale", "1", "Defines the scale of texture texels. Use 1 for quake+quake2 maps, and 0.5 for quake3 maps.");

enum
{
	hmcmd_brush_delete,		//brush OR patch destruction
	hmcmd_brush_insert,		//brush creation
	hmcmd_prespawning,		//sent before initial inserts
	hmcmd_prespawned,		//sent just after initial inserts
	hmcmd_patch_insert,		//patch creation

	hmcmd_ent_edit = 0x40,
	hmcmd_ent_remove
};


void validatelinks(link_t *firstnode)
{
/*	link_t *node;

	COM_AssertMainThread("foo");

	for (node = firstnode->next; node; node = node->next)
		if (firstnode == node)
			break;

	for (node = firstnode->prev; node; node = node->prev)
		if (firstnode == node)
			break;

	return;*/
}

void validatelinks2(link_t *firstnode, link_t *panic)
{
/*	link_t *node;

	COM_AssertMainThread("foo");

	for (node = firstnode->next; node; node = node->next)
	{
		if (node == panic)
			Sys_Error("Panic\n");
		if (firstnode == node)
			break;
	}

	for (node = firstnode->prev; node; node = node->prev)
	{
		if (node == panic)
			Sys_Error("Panic\n");
		if (firstnode == node)
			break;
	}

	return;*/
}


static hmsection_t *QDECL Terr_GetSection(heightmap_t *hm, int x, int y, unsigned int flags);
static void Terr_LoadSectionWorker(void *ctx, void *data, size_t a, size_t b);
static void Terr_WorkerLoadedSection(void *ctx, void *data, size_t a, size_t b);
static void Terr_WorkerFailedSection(void *ctx, void *data, size_t a, size_t b);

static void Terr_Brush_DeleteIdx(heightmap_t *hm, size_t idx);
#ifdef HAVE_CLIENT
static void ted_dorelight(model_t *m, heightmap_t *hm);
static void Terr_WorkerLoadedSectionLightmap(void *ctx, void *data, size_t a, size_t b);
static qboolean Terr_Collect(heightmap_t *hm);
static void Terr_Brush_Draw(heightmap_t *hm, batch_t **batches, entity_t *e);

static texid_t Terr_LoadTexture(char *name)
{
	extern texid_t missing_texture;
	texid_t id;
	if (*name)
	{
		id = R_LoadHiResTexture(name, NULL, 0);
		if (!TEXVALID(id))
		{
			id = missing_texture;
			Con_Printf("Unable to load texture %s\n", name);
		}
	}
	else
		id = missing_texture;
	return id;
}
#endif

static void Terr_LoadSectionTextures(hmsection_t *s)
{
#ifdef HAVE_CLIENT
	extern texid_t missing_texture;
	struct hmwater_s *w;
	if (isDedicated)
		return;
	//CL_CheckOrEnqueDownloadFile(s->texname[0], NULL, 0);
	//CL_CheckOrEnqueDownloadFile(s->texname[1], NULL, 0);
	//CL_CheckOrEnqueDownloadFile(s->texname[2], NULL, 0);
	//CL_CheckOrEnqueDownloadFile(s->texname[3], NULL, 0);
	switch(s->hmmod->mode)
	{
	case HMM_BLOCKS:
		s->textures.base			= Terr_LoadTexture(va("maps/%s/atlas.tga", s->hmmod->path));
		s->textures.fullbright		= Terr_LoadTexture(va("maps/%s/atlas_luma.tga", s->hmmod->path));
		s->textures.bump			= Terr_LoadTexture(va("maps/%s/atlas_norm.tga", s->hmmod->path));
		s->textures.specular		= Terr_LoadTexture(va("maps/%s/atlas_spec.tga", s->hmmod->path));
		s->textures.upperoverlay	= missing_texture;
		s->textures.loweroverlay	= missing_texture;
		break;
	case HMM_TERRAIN:
		s->textures.base			= Terr_LoadTexture(s->texname[0]);
		s->textures.upperoverlay	= Terr_LoadTexture(s->texname[1]);
		s->textures.loweroverlay	= Terr_LoadTexture(s->texname[2]);
		s->textures.fullbright		= Terr_LoadTexture(s->texname[3]);
		s->textures.bump			= *s->texname[0]?R_LoadHiResTexture(va("%s_norm", s->texname[0]), NULL, 0):r_nulltex;
		s->textures.specular		= *s->texname[0]?R_LoadHiResTexture(va("%s_spec", s->texname[0]), NULL, 0):r_nulltex;
		break;
	}
	for (w = s->water; w; w = w->next)
	{
		w->shader = R_RegisterCustom (NULL, w->shadername, SUF_NONE, Shader_DefaultWaterShader, NULL);
		R_BuildDefaultTexnums(NULL, w->shader, IF_WORLDTEX);	//this might get expensive. hideously so.
	}
#endif
}

static qboolean QDECL Terr_InitLightmap(hmsection_t *s, qboolean initialise)
{
#ifndef HAVE_CLIENT
	return false;
#else
	heightmap_t *hm = s->hmmod;

	if (s->lightmap < 0)
	{
		struct lmsect_s *lms;
		Sys_LockMutex(com_resourcemutex);
		while (!hm->unusedlmsects)
		{
			int lm;
			int i;
			Sys_UnlockMutex(com_resourcemutex);
			lm = Surf_NewLightmaps(1, SECTTEXSIZE*LMCHUNKS, SECTTEXSIZE*LMCHUNKS, PTI_BGRA8, false);
			Sys_LockMutex(com_resourcemutex);
			for (i = 0; i < LMCHUNKS*LMCHUNKS; i++)
			{
				lms = BZ_Malloc(sizeof(*lms));
				lms->lm = lm;
				lms->x = (i & (LMCHUNKS-1))*SECTTEXSIZE;
				lms->y = (i / LMCHUNKS)*SECTTEXSIZE;
				lms->next = hm->unusedlmsects;
				hm->unusedlmsects = lms;
				hm->numunusedlmsects++;
			}
		}

		lms = hm->unusedlmsects;
		hm->unusedlmsects = lms->next;
		
		s->lightmap = lms->lm;
		s->lmx = lms->x;
		s->lmy = lms->y;

		hm->numunusedlmsects--;
		hm->numusedlmsects++;
		Sys_UnlockMutex(com_resourcemutex);

		Z_Free(lms);
		initialise = true;
	}

	if (initialise && s->lightmap >= 0)
	{
		int x, y;
		unsigned char *lm = lightmap[s->lightmap]->lightmaps;
		unsigned int pixbytes = lightmap[s->lightmap]->pixbytes;
		lm += (s->lmy * HMLMSTRIDE + s->lmx) * pixbytes;
		for (y = 0; y < SECTTEXSIZE; y++)
		{
			for (x = 0; x < SECTTEXSIZE; x++)
			{
				lm[x*4+0] = 0;
				lm[x*4+1] = 0;
				lm[x*4+2] = 0;
				lm[x*4+3] = 255;
			}
			lm += (HMLMSTRIDE)*pixbytes;
		}
	}

	if (s->lightmap >= 0)
	{
		lightmap[s->lightmap]->modified = true;
		lightmap[s->lightmap]->rectchange.l = 0;
		lightmap[s->lightmap]->rectchange.t = 0;
		lightmap[s->lightmap]->rectchange.r = HMLMSTRIDE;
		lightmap[s->lightmap]->rectchange.b = HMLMSTRIDE;
	}

	return s->lightmap>=0;
#endif
}

static char *genextendedhex(int n, char *buf)
{
	char *ret;
	static char nibble[16] = "0123456789abcdef";
	unsigned int m;
	int i;
	for (i = 7; i >= 1; i--)	//>=1 ensures at least two nibbles appear.
	{
		m = 0xfffffff8<<(i*4);
		if ((n & m) != m && (n & m) != 0)
			break;
	}
	ret = buf;
	for(i++; i >= 0; i--)
		*buf++ = nibble[(n>>i*4) & 0xf];
	*buf++ = 0;
	return ret;
}
static char *Terr_DiskBlockName(heightmap_t *hm, int sx, int sy, char *out, size_t outsize)
{
	char xpart[9];
	char ypart[9];
	//using a naming scheme centered around 0 means we can gracefully expand the map away from 0,0
	sx -= CHUNKBIAS;
	sy -= CHUNKBIAS;
	//wrap cleanly
	sx &= CHUNKLIMIT-1;
	sy &= CHUNKLIMIT-1;
	sx /= SECTIONSPERBLOCK;
	sy /= SECTIONSPERBLOCK;
	if (sx >= CHUNKBIAS/SECTIONSPERBLOCK)
		sx |= 0xffffff00;
	if (sy >= CHUNKBIAS/SECTIONSPERBLOCK)
		sy |= 0xffffff00;
	Q_snprintfz(out, outsize, "maps/%s/block_%s_%s.hms", hm->path, genextendedhex(sx, xpart), genextendedhex(sy, ypart));
	return out;
}
static char *Terr_DiskSectionName(heightmap_t *hm, int sx, int sy, char *out, size_t outsize)
{
	sx -= CHUNKBIAS;
	sy -= CHUNKBIAS;
	//wrap cleanly
	sx &= CHUNKLIMIT-1;
	sy &= CHUNKLIMIT-1;
	Q_snprintfz(out, outsize, "maps/%s/sect_%03x_%03x.hms", hm->path, sx, sy);
	return out;
}
#ifdef HAVE_CLIENT
static char *Terr_TempDiskSectionName(heightmap_t *hm, int sx, int sy)
{
	sx -= CHUNKBIAS;
	sy -= CHUNKBIAS;
	//wrap cleanly
	sx &= CHUNKLIMIT-1;
	sy &= CHUNKLIMIT-1;
	return va("temp/%s/sect_%03x_%03x.hms", hm->path, sx, sy);
}
#endif

#ifdef HAVE_SERVER
static int dehex_e(int i, qboolean *error)
{
	if      (i >= '0' && i <= '9')
		return (i-'0');
	else if (i >= 'A' && i <= 'F')
		return (i-'A'+10);
	else if (i >= 'a' && i <= 'f')
		return (i-'a'+10);
	else
		*error = true;
	return 0;
}
static qboolean Terr_IsSectionFName(heightmap_t *hm, const char *fname, int *sx, int *sy)
{
	int l;
	qboolean error = false;
	*sx = 0xdeafbeef;	//something clearly invalid
	*sy = 0xdeafbeef;

	//not this model...
	if (!hm)
		return false;

	//expect the first 5 chars to be maps/ or temp/
	fname += 5;

	l = strlen(hm->path);
	if (strncmp(fname, hm->path, l) || fname[l] != '/')
		return false;
	fname += l+1;

	//fname now has a fixed length.
	if (strlen(fname) != 16)
		return false;
	if (strncmp(fname, "sect_", 5) || fname[8] != '_' || (strcmp(fname+12, ".hms") && strcmp(fname+12, ".tmp")))
		return false;

	*sx = 0;
	*sx += dehex_e(fname[5], &error)<<8;
	*sx += dehex_e(fname[6], &error)<<4;
	*sx += dehex_e(fname[7], &error)<<0;

	*sy = 0;
	*sy += dehex_e(fname[9], &error)<<8;
	*sy += dehex_e(fname[10], &error)<<4;
	*sy += dehex_e(fname[11], &error)<<0;

	*sx += CHUNKBIAS;
	*sy += CHUNKBIAS;

	if ((unsigned)*sx >= CHUNKLIMIT)
		*sx -= CHUNKLIMIT;
	if ((unsigned)*sy >= CHUNKLIMIT)
		*sy -= CHUNKLIMIT;

	//make sure its a valid section index.
	if ((unsigned)*sx >= CHUNKLIMIT)
		return false;
	if ((unsigned)*sy >= CHUNKLIMIT)
		return false;
	return true;
}
#endif

static int QDECL Terr_GenerateSections(heightmap_t *hm, int sx, int sy, int count, hmsection_t **sects)
{
	//a worker is trying to load multiple sections at once.
	//lock ALL of them atomically, so that we don't end up with too many workers all doing stuff at once.
	int x, y;
	
	hmsection_t *s;
	hmcluster_t *cluster;
	int numgen = 0;

	Sys_LockMutex(com_resourcemutex);
	for (y = 0; y < count; y++)
		for (x = 0; x < count; x++)
		{
			int clusternum = ((sx+x)/MAXSECTIONS) + ((sy+y)/MAXSECTIONS)*MAXCLUSTERS;
			cluster = hm->cluster[clusternum];
			if (!cluster)
				cluster = hm->cluster[clusternum] = Z_Malloc(sizeof(*cluster));
			s = cluster->section[((sx+x)%MAXSECTIONS) + ((sy+y)%MAXSECTIONS)*MAXSECTIONS];
			if (!s)
			{
				s = Z_Malloc(sizeof(*s));
				s->loadstate = TSLS_LOADING0;
#ifdef HAVE_CLIENT
				s->lightmap = -1;
#endif
				s->numents = 0;
				s->sx = sx + x;
				s->sy = sy + y;
				cluster->section[(s->sx%MAXSECTIONS) + (s->sy%MAXSECTIONS)*MAXSECTIONS] = s;
				hm->activesections++;
				s->hmmod = hm;

				s->flags = TSF_DIRTY;

				hm->loadingsections+=1;
			}
#ifdef HAVE_CLIENT
			else if (s->loadstate == TSLS_LOADED && s->lightmap < 0)
				;	//it lost its lightmap. the main thread won't be drawing with it, nor do any loaders.
					//FIXME: might be used by tracelines on a worker (eg lightmap generation)
#endif
			else if (s->loadstate != TSLS_LOADING0)
			{
				//this one is already active.
				sects[x + y*count] = NULL;
				continue;
			}

			s->loadstate = TSLS_LOADING1;

			sects[x + y*count] = s;
			numgen++;
		}
	Sys_UnlockMutex(com_resourcemutex);
	return numgen;
}
static hmsection_t *QDECL Terr_GenerateSection(heightmap_t *hm, int sx, int sy, qboolean scheduleload)
{
	hmsection_t *s;
	hmcluster_t *cluster;
	int clusternum = (sx/MAXSECTIONS) + (sy/MAXSECTIONS)*MAXCLUSTERS;

#ifdef LOADERTHREAD
	Sys_LockMutex(com_resourcemutex);
#endif
	cluster = hm->cluster[clusternum];
	if (!cluster)
		cluster = hm->cluster[clusternum] = Z_Malloc(sizeof(*cluster));
	s = cluster->section[(sx%MAXSECTIONS) + (sy%MAXSECTIONS)*MAXSECTIONS];
	if (!s)
	{
		s = Z_Malloc(sizeof(*s));
		if (!s)
		{
#ifdef LOADERTHREAD
			Sys_UnlockMutex(com_resourcemutex);
#endif
			return NULL;
		}
#ifdef HAVE_CLIENT
		s->lightmap = -1;
		s->numents = 0;
#endif

		s->sx = sx;
		s->sy = sy;
		cluster->section[(sx%MAXSECTIONS) + (sy%MAXSECTIONS)*MAXSECTIONS] = s;
		hm->activesections++;
		s->hmmod = hm;

		s->flags = TSF_DIRTY;

		hm->loadingsections+=1;

		if (!scheduleload)
		{	//no scheduling means that we're loading it NOW, on this thread.
			s->loadstate = TSLS_LOADING1;
#ifdef LOADERTHREAD
			Sys_UnlockMutex(com_resourcemutex);
#endif
			return s;
		}
		s->loadstate = TSLS_LOADING0;

#ifdef LOADERTHREAD
		Sys_UnlockMutex(com_resourcemutex);
#endif
		COM_AddWork(WG_LOADER, Terr_LoadSectionWorker, s, hm, sx, sy);
		return s;
	}
	if (!scheduleload)
	{
		if (s->loadstate == TSLS_LOADING0)
			s->loadstate = TSLS_LOADING1;
		else
			s = NULL;
	}
#ifdef LOADERTHREAD
	Sys_UnlockMutex(com_resourcemutex);
#endif
	return s;
}

//generates some water
static void *QDECL Terr_GenerateWater(hmsection_t *s, float maxheight)
{
	int i;
	struct hmwater_s *w;
	w = Z_Malloc(sizeof(*s->water));
	w->next = s->water;
	s->water = w;
	Q_strncpyz(w->shadername, s->hmmod->defaultwatershader, sizeof(w->shadername));
	w->simple = true;
	w->contentmask = FTECONTENTS_WATER;
	memset(w->holes, 0, sizeof(w->holes));
	for (i = 0; i < 9*9; i++)
		w->heights[i] = maxheight;
	w->maxheight = w->minheight = maxheight;
	if (s->maxh_cull < w->maxheight)
		s->maxh_cull = w->maxheight;
	return w;
}

//embeds a mesh
static void QDECL Terr_AddMesh(heightmap_t *hm, int loadflags, model_t *mod, const char *modelname, vec3_t epos, vec3_t axis[3], float scale)
{
#ifdef HAVE_CLIENT
	struct hmentity_s *e, *f = NULL;
	hmsection_t *s;
	int min[2], max[2], coord[2];
	int i;

	if (!mod)
	{
		if (modelname)
			mod = Mod_ForName(modelname, MLV_WARN);
		if (!mod)
			return;
	}

	if (!scale)
		scale = 1;

	if (mod->loadstate != MLS_LOADED)
		Con_DPrintf("Terr_AddMesh: model is not loaded yet\n");

	//I do NOT like that this depends on the size of the model.
	if (axis[0][0] != 1 || axis[0][1] != 0 || axis[0][2] != 0 ||
		axis[1][0] != 0 || axis[1][1] != 1 || axis[1][2] != 0 ||
		axis[2][0] != 0 || axis[2][1] != 0 || axis[2][2] != 1)
	{
		min[0] = floor((epos[0]-mod->radius*scale) / hm->sectionsize) + CHUNKBIAS;
		min[1] = floor((epos[1]-mod->radius*scale) / hm->sectionsize) + CHUNKBIAS;
		min[0] = bound(hm->firstsegx, min[0], hm->maxsegx-1);
		min[1] = bound(hm->firstsegy, min[1], hm->maxsegy-1);
		
		max[0] = floor((epos[0]+mod->radius*scale) / hm->sectionsize) + CHUNKBIAS;
		max[1] = floor((epos[1]+mod->radius*scale) / hm->sectionsize) + CHUNKBIAS;
		max[0] = bound(hm->firstsegx, max[0], hm->maxsegx-1);
		max[1] = bound(hm->firstsegy, max[1], hm->maxsegy-1);
	}
	else
	{
		min[0] = floor((epos[0]+mod->mins[0]*scale) / hm->sectionsize) + CHUNKBIAS;
		min[1] = floor((epos[1]+mod->mins[1]*scale) / hm->sectionsize) + CHUNKBIAS;
		min[0] = bound(hm->firstsegx, min[0], hm->maxsegx-1);
		min[1] = bound(hm->firstsegy, min[1], hm->maxsegy-1);
		
		max[0] = floor((epos[0]+mod->maxs[0]*scale) / hm->sectionsize) + CHUNKBIAS;
		max[1] = floor((epos[1]+mod->maxs[1]*scale) / hm->sectionsize) + CHUNKBIAS;
		max[0] = bound(hm->firstsegx, max[0], hm->maxsegx-1);
		max[1] = bound(hm->firstsegy, max[1], hm->maxsegy-1);
	}

	Sys_LockMutex(hm->entitylock);
	//try to find the ent if it already exists (don't do dupes)
	for (e = hm->entities; e; e = e->next)
	{
		if (!e->refs)
			f = e;
		else
		{
			if (e->ent.origin[0] != epos[0] || e->ent.origin[1] != epos[1] || e->ent.origin[2] != epos[2])
				continue;
			if (e->ent.model != mod || e->ent.scale != scale)
				continue;
			if (memcmp(axis, e->ent.axis, sizeof(e->ent.axis)))
				continue;
			break;	//looks like a match.
		}
	}
	//allocate it if needed
	if (!e)
	{
		if (f)
			e = f;	//can reuse a released one
		else
		{	//allocate one
			e = Z_Malloc(sizeof(*e));
			e->next = hm->entities;
			hm->entities = e;
		}

#ifdef HEXEN2
		e->ent.drawflags = SCALE_ORIGIN_ORIGIN;
#endif
		e->ent.scale = scale;
		e->ent.playerindex = -1;
		e->ent.framestate.g[FS_REG].lerpweight[0] = 1;
		e->ent.topcolour = TOP_DEFAULT;
		e->ent.bottomcolour = BOTTOM_DEFAULT;
		e->ent.shaderRGBAf[0] = 1;
		e->ent.shaderRGBAf[1] = 1;
		e->ent.shaderRGBAf[2] = 1;
		e->ent.shaderRGBAf[3] = 1;
		VectorCopy(epos, e->ent.origin);
		memcpy(e->ent.axis, axis, sizeof(e->ent.axis));
		e->ent.model = mod;
	}

	for (coord[0] = min[0]; coord[0] <= max[0]; coord[0]++)
	{
		for (coord[1] = min[1]; coord[1] <= max[1]; coord[1]++)
		{
			s = Terr_GetSection(hm, coord[0], coord[1], loadflags|TGS_ANYSTATE);
			if (!s)
				continue;

			//don't add pointless dupes
			for (i = 0; i < s->numents; i++)
			{
				if (s->ents[i] == e)
					break;
			}
			if (i < s->numents)
				continue;

			//FIXME: while technically correct, this causes issues with the v1 format.
			s->flags |= TSF_EDITED;

			//FIXME: race condition - main thread might be walking the entity list.
			//FIXME: even worse: the editor might be running through this routine adding/removing entities at the same time as the loader.
			if (s->maxents == s->numents)
			{
				s->maxents++;
				s->ents = realloc(s->ents, sizeof(*s->ents)*(s->maxents));
			}
			s->ents[s->numents++] = e;
			e->refs++;
		}
	}
	Sys_UnlockMutex(hm->entitylock);
#endif
}

static void *Terr_ReadV1(heightmap_t *hm, hmsection_t *s, void *ptr, int len)
{
#ifdef HAVE_CLIENT
	dsmesh_v1_t *dm;
	float *colours;
	qbyte *lmstart;
#endif
	dsection_v1_t *ds = ptr;
	int i;

	unsigned int flags = LittleLong(ds->flags);
	s->flags |= flags & ~(TSF_INTERNAL|TSF_HASWATER_V0);
	for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
	{
		s->heights[i] = LittleFloat(ds->heights[i]);
	}
	s->minh = ds->minh;
	s->maxh = ds->maxh;
	if (flags & TSF_HASWATER_V0)
		Terr_GenerateWater(s, ds->waterheight);

	memset(s->holes, 0, sizeof(s->holes));
	for (i = 0; i < 8*8; i++)
	{
		int x = (i & 7);
		int y = (i>>3);
		int b = (1u<<(x>>1)) << ((y>>1)<<2);
		if (ds->holes & b)
			s->holes[y] |= 1u<<x;
	}

	ptr = ds+1;

#ifdef HAVE_CLIENT
	/*deal with textures*/
	Q_strncpyz(s->texname[0], ds->texname[0], sizeof(s->texname[0]));
	Q_strncpyz(s->texname[1], ds->texname[1], sizeof(s->texname[1]));
	Q_strncpyz(s->texname[2], ds->texname[2], sizeof(s->texname[2]));
	Q_strncpyz(s->texname[3], ds->texname[3], sizeof(s->texname[3]));

	/*load in the mixture/lighting*/
	lmstart = BZ_Malloc(SECTTEXSIZE*SECTTEXSIZE*4);
	memcpy(lmstart, ds->texmap, SECTTEXSIZE*SECTTEXSIZE*4);
	COM_AddWork(WG_MAIN, Terr_WorkerLoadedSectionLightmap, hm, lmstart, s->sx, s->sy);

	s->mesh.colors4f_array[0] = s->colours;
	if (flags & TSF_HASCOLOURS)
	{
		for (i = 0, colours = (float*)ptr; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++, colours+=4)
		{
			s->colours[i][0] = LittleFloat(colours[0]);
			s->colours[i][1] = LittleFloat(colours[1]);
			s->colours[i][2] = LittleFloat(colours[2]);
			s->colours[i][3] = LittleFloat(colours[3]);
		}
		ptr = colours;
	}
	else
	{
		for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
		{
			s->colours[i][0] = 1;
			s->colours[i][1] = 1;
			s->colours[i][2] = 1;
			s->colours[i][3] = 1;
		}
	}

	/*load any static ents*/
	for (i = 0, dm = (dsmesh_v1_t*)ptr; i < ds->ents_num; i++, dm = (dsmesh_v1_t*)((qbyte*)dm + dm->size))
	{
		vec3_t org;
		org[0] = dm->axisorg[3][0] + (s->sx-CHUNKBIAS)*hm->sectionsize;
		org[1] = dm->axisorg[3][1] + (s->sy-CHUNKBIAS)*hm->sectionsize;
		org[2] = dm->axisorg[3][2];
		Terr_AddMesh(hm, TGS_NOLOAD, NULL, (char*)(dm + 1), org, dm->axisorg, dm->scale); 
	}
#endif
	return ptr;
}




struct terrstream_s
{
	qbyte *buffer;
	int maxsize;
	int pos;
};
//I really hope these get inlined properly.
static int Terr_Read_SInt(struct terrstream_s *strm)
{
	int val;
	strm->pos = (strm->pos + sizeof(val)-1) & ~(sizeof(val)-1);
	val = *(int*)(strm->buffer+strm->pos);
	strm->pos += sizeof(val);
	return LittleLong(val);
}
static qbyte Terr_Read_Byte(struct terrstream_s *strm)
{
	qbyte val;
	val = *(qbyte*)(strm->buffer+strm->pos);
	strm->pos += sizeof(val);
	return val;
}
static float Terr_Read_Float(struct terrstream_s *strm)
{
	float val;
	strm->pos = (strm->pos + sizeof(val)-1) & ~(sizeof(val)-1);
	val = *(float*)(strm->buffer+strm->pos);
	strm->pos += sizeof(val);
	return LittleFloat(val);
}
static char *Terr_Read_String(struct terrstream_s *strm, char *val, int maxlen)
{
	int len = strlen(strm->buffer + strm->pos);
	maxlen = min(len, maxlen-1);	//truncate
	memcpy(val, strm->buffer + strm->pos, maxlen);
	val[maxlen] = 0;
	strm->pos += len+1;
	return val;
}
#ifdef HAVE_CLIENT
static void Terr_Write_SInt(struct terrstream_s *strm, int val)
{
	val = LittleLong(val);
	strm->pos = (strm->pos + sizeof(val)-1) & ~(sizeof(val)-1);
	*(int*)(strm->buffer+strm->pos) = val;
	strm->pos += sizeof(val);
}
static void Terr_Write_Byte(struct terrstream_s *strm, qbyte val)
{
	*(qbyte*)(strm->buffer+strm->pos) = val;
	strm->pos += sizeof(val);
}
static void Terr_Write_Float(struct terrstream_s *strm, float val)
{
	val = LittleFloat(val);
	strm->pos = (strm->pos + sizeof(val)-1) & ~(sizeof(val)-1);
	*(float*)(strm->buffer+strm->pos) = val;
	strm->pos += sizeof(val);
}
static void Terr_Write_String(struct terrstream_s *strm, char *val)
{
	int len = strlen(val)+1;
	memcpy(strm->buffer + strm->pos, val, len);
	strm->pos += len;
}
static void Terr_TrimWater(hmsection_t *s)
{
	int i;
	struct hmwater_s *w, **link;

	for (link = &s->water; (w = *link); )
	{
		//one has a height above the terrain?
		for (i = 0; i < 9*9; i++)
			if (w->heights[i] > s->minh)
				break;
		if (i == 9*9)
		{
			*link = w->next;
			Z_Free(w);
			continue;
		}
		else
			link = &(*link)->next;
	}
}
static void Terr_SaveV2(heightmap_t *hm, hmsection_t *s, vfsfile_t *f, int sx, int sy)
{
	qbyte buffer[65536], last, delta, *lm;
	struct terrstream_s strm = {buffer, sizeof(buffer), 0};
	unsigned int flags = s->flags;
	int i, j, x, y;
	struct hmwater_s *w;
	unsigned int pixbytes;

	flags &= ~(TSF_INTERNAL);
	flags &= ~(TSF_HASCOLOURS|TSF_HASHEIGHTS|TSF_HASSHADOW);

	for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
	{
		if (s->colours[i][0] != 1 || s->colours[i][1] != 1 || s->colours[i][2] != 1 || s->colours[i][3] != 1)
		{
			flags |= TSF_HASCOLOURS;
			break;
		}
	}
	for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
	{
		if (s->heights[i] != s->heights[0])
		{
			flags |= TSF_HASHEIGHTS;
			break;
		}
	}

	pixbytes = lightmap[s->lightmap]->pixbytes;
	lm = lightmap[s->lightmap]->lightmaps;
	lm += (s->lmy * HMLMSTRIDE + s->lmx) * pixbytes;
	for (y = 0; y < SECTTEXSIZE; y++)
	{
		for (x = 0; x < SECTTEXSIZE; x++)
		{
			if (lm[x*4+3] != 255)
			{
				flags |= TSF_HASSHADOW;
				y = SECTTEXSIZE;
				break;
			}
		}
		lm += (HMLMSTRIDE)*pixbytes;
	}

	//write the flags so the loader knows what to load
	Terr_Write_SInt(&strm, flags);

	//if heights are compressed, only the first is present.
	if (!(flags & TSF_HASHEIGHTS))
		Terr_Write_Float(&strm, s->heights[0]);
	else
	{
		for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
			Terr_Write_Float(&strm, s->heights[i]);
	}

	for (i = 0; i < sizeof(s->holes); i++)
		Terr_Write_Byte(&strm, s->holes[i]);

	Terr_TrimWater(s);
	for (j = 0, w = s->water; w; j++)
		w = w->next;
	Terr_Write_SInt(&strm, j);
	for (i = 0, w = s->water; i < j; i++, w = w->next)
	{
		char *shadername = w->shader->name;
		int fl = 0;

		if (strcmp(shadername, hm->defaultwatershader))
			fl |= 1;
		for (x = 0; x < 8; x++)
			if (w->holes[x])
				break;
		fl |= ((x==8)?0:2);
		for (x = 0; x < 9*9; x++)
			if (w->heights[x] != w->heights[0])
				break;
		fl |= ((x==9*9)?0:4);

		
		Terr_Write_SInt(&strm, fl);
		Terr_Write_SInt(&strm, w->contentmask);
		if (fl & 1)
			Terr_Write_String(&strm, shadername);
		if (fl & 2)
		{
			for (x = 0; x < 8; x++)
				Terr_Write_Byte(&strm, w->holes[x]);
		}
		if (fl & 4)
		{
			for (x = 0; x < 9*9; x++)
				Terr_Write_Float(&strm, w->heights[x]);
		}
		else
			Terr_Write_Float(&strm, w->heights[0]);
	}

	if (flags & TSF_HASCOLOURS)
	{
		//FIXME: bytes? channels?
		for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
		{
			Terr_Write_Float(&strm, s->colours[i][0]);
			Terr_Write_Float(&strm, s->colours[i][1]);
			Terr_Write_Float(&strm, s->colours[i][2]);
			Terr_Write_Float(&strm, s->colours[i][3]);
		}
	}

	for (j = 0; j < 4; j++)
		Terr_Write_String(&strm, s->texname[j]);
	for (j = 0; j < 4; j++)
	{
		if (j == 3)
		{
			//only write the channel if it has actual data
			if (!(flags & TSF_HASSHADOW))
				continue;
		}
		else
		{
			//only write the data if there's actually a texture.
			//its not meant to be possible to delete a texture without deleting its data too.
			//
			if (!*s->texname[2-j])
				continue;
		}

		//write the channel
		last = 0;
		pixbytes = lightmap[s->lightmap]->pixbytes;
		lm = lightmap[s->lightmap]->lightmaps;
		lm += (s->lmy * HMLMSTRIDE + s->lmx) * pixbytes;
		for (y = 0; y < SECTTEXSIZE; y++)
		{
			for (x = 0; x < SECTTEXSIZE; x++)
			{
				delta = lm[x*4+j] - last;
				last = lm[x*4+j];
				Terr_Write_Byte(&strm, delta);
			}
			lm += (HMLMSTRIDE)*pixbytes;
		}
	}

	Sys_LockMutex(hm->entitylock);
	Terr_Write_SInt(&strm, s->numents);
	for (i = 0; i < s->numents; i++)
	{
		unsigned int mf;

		//make sure we don't overflow. we should always be aligned at this point.
		if (strm.pos > strm.maxsize/2)
		{
			VFS_WRITE(f, strm.buffer, strm.pos);
			strm.pos = 0;
		}

		mf = 0;
		if (s->ents[i]->ent.scale != 1)
			mf |= TMF_SCALE;
		Terr_Write_SInt(&strm, mf);
		if (s->ents[i]->ent.model)
			Terr_Write_String(&strm, s->ents[i]->ent.model->name);
		else
			Terr_Write_String(&strm, "*invalid");
		Terr_Write_Float(&strm, s->ents[i]->ent.origin[0]+(CHUNKBIAS-sx)*hm->sectionsize);
		Terr_Write_Float(&strm, s->ents[i]->ent.origin[1]+(CHUNKBIAS-sy)*hm->sectionsize);
		Terr_Write_Float(&strm, s->ents[i]->ent.origin[2]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[0][0]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[0][1]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[0][2]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[1][0]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[1][1]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[1][2]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[2][0]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[2][1]);
		Terr_Write_Float(&strm, s->ents[i]->ent.axis[2][2]);
		if (mf & TMF_SCALE)
			Terr_Write_Float(&strm, s->ents[i]->ent.scale);
	}
	Sys_UnlockMutex(hm->entitylock);

	//reset it in case the buffer is getting a little full
	strm.pos = (strm.pos + sizeof(int)-1) & ~(sizeof(int)-1);
	VFS_WRITE(f, strm.buffer, strm.pos);
	strm.pos = 0;
}
#ifdef HAVE_CLIENT
static void Terr_WorkerLoadedSectionLightmap(void *ctx, void *data, size_t a, size_t b)
{
	heightmap_t *hm = ctx;
	hmsection_t *s = Terr_GetSection(hm, a, b, TGS_NOLOAD|TGS_ANYSTATE);
	qbyte *inlm = data;
	qbyte *outlm;
	int y;

	if (s)
	if (Terr_InitLightmap(s, false))
	{
		int pixbytes = lightmap[s->lightmap]->pixbytes;
		outlm = lightmap[s->lightmap]->lightmaps;
		outlm += (s->lmy * HMLMSTRIDE + s->lmx) * pixbytes;
		for (y = 0; y < SECTTEXSIZE; y++)
		{
			memcpy(outlm, inlm, SECTTEXSIZE*4);
			inlm += SECTTEXSIZE*4;
			outlm += (HMLMSTRIDE)*pixbytes;
		}
	}

	BZ_Free(data);
}
#endif
#endif
static void *Terr_ReadV2(heightmap_t *hm, hmsection_t *s, void *ptr, int len)
{
#ifdef HAVE_CLIENT
	char modelname[MAX_QPATH];
	qbyte last;
	int y;
	qboolean present;
	qbyte *lmstart = NULL, *lm, delta;
#endif
	struct terrstream_s strm = {ptr, len, 0};
	float f;
	int i, j, x;
	unsigned int flags = Terr_Read_SInt(&strm);

	s->flags |= flags & ~TSF_INTERNAL;
	if (flags & TSF_HASHEIGHTS)
	{
		s->minh = s->maxh = s->heights[0] = Terr_Read_Float(&strm);
		for (i = 1; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
		{
			f = Terr_Read_Float(&strm);
			if (s->minh > f)
				s->minh = f;
			if (s->maxh < f)
				s->maxh = f;
			s->heights[i] = f;
		}
	}
	else
	{
		s->minh = s->maxh = f = Terr_Read_Float(&strm);
		for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
			s->heights[i] = f;
	}

	for (i = 0; i < sizeof(s->holes); i++)
		s->holes[i] = Terr_Read_Byte(&strm);

	j = Terr_Read_SInt(&strm);
	for (i = 0; i < j; i++)
	{
		struct hmwater_s *w = Z_Malloc(sizeof(*w));
		int fl = Terr_Read_SInt(&strm);
		w->next = s->water;
		s->water = w;
		w->simple = true;
		w->contentmask = Terr_Read_SInt(&strm);
		if (fl & 1)
			Terr_Read_String(&strm, w->shadername, sizeof(w->shadername));
		else
			Q_strncpyz(w->shadername, hm->defaultwatershader, sizeof(w->shadername));
		if (fl & 2)
		{
			for (x = 0; x < 8; x++)
				w->holes[i] = Terr_Read_Byte(&strm);
			w->simple = false;
		}
		if (fl & 4)
		{
			for (x = 0; x < 9*9; x++)
			{
				w->heights[x] = Terr_Read_Float(&strm);
			}
			w->simple = false;
		}
		else
		{	//all heights the same can be used as a way to compress the data
			w->minheight = w->maxheight = Terr_Read_Float(&strm);
			for (x = 0; x < 9*9; x++)
				w->heights[x] = w->minheight = w->maxheight;
		}
	}

	//dedicated server can stop reading here.

#ifdef HAVE_CLIENT
	if (flags & TSF_HASCOLOURS)
	{
		for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
		{
			s->colours[i][0] = Terr_Read_Float(&strm);
			s->colours[i][1] = Terr_Read_Float(&strm);
			s->colours[i][2] = Terr_Read_Float(&strm);
			s->colours[i][3] = Terr_Read_Float(&strm);
		}
	}
	else
	{
		for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
		{
			s->colours[i][0] = 1;
			s->colours[i][1] = 1;
			s->colours[i][2] = 1;
			s->colours[i][3] = 1;
		}
	}

	for (j = 0; j < 4; j++)
		Terr_Read_String(&strm, s->texname[j], sizeof(s->texname[j]));
	for (j = 0; j < 4; j++)
	{
		if (j == 3)
			present = !!(flags & TSF_HASSHADOW);
		else
			present = !!(*s->texname[2-j]);

		//should be able to skip this if no shadows or textures
		if (!lmstart)
			lmstart = BZ_Malloc(SECTTEXSIZE*SECTTEXSIZE*4);

		if (present)
		{
			//read the channel
			last = 0;
			lm = lmstart;
			for (y = 0; y < SECTTEXSIZE; y++)
			{
				for (x = 0; x < SECTTEXSIZE; x++)
				{
					delta = Terr_Read_Byte(&strm);
					last = (last+delta)&0xff;
					lm[x*4+j] = last;
				}
				lm += x*4;
			}
		}
		else
		{
			last = ((j==3)?255:0);
			lm = lmstart;
			for (y = 0; y < SECTTEXSIZE; y++)
			{
				for (x = 0; x < SECTTEXSIZE; x++)
					lm[x*4+j] = last;
				lm += x*4;
			}
		}
	}

	if (lmstart)
		COM_AddWork(WG_MAIN, Terr_WorkerLoadedSectionLightmap, hm, lmstart, s->sx, s->sy);

	/*load any static ents*/
	j = Terr_Read_SInt(&strm);
	for (i = 0; i < j; i++)
	{
		vec3_t axis[3];
		vec3_t org;
		unsigned int mf;
		model_t *mod;
		float scale;
		mf = Terr_Read_SInt(&strm);

		mod = Mod_FindName(Terr_Read_String(&strm, modelname, sizeof(modelname)));
		org[0] = Terr_Read_Float(&strm);
		org[1] = Terr_Read_Float(&strm);
		org[2] = Terr_Read_Float(&strm);
		axis[0][0] = Terr_Read_Float(&strm);
		axis[0][1] = Terr_Read_Float(&strm);
		axis[0][2] = Terr_Read_Float(&strm);
		axis[1][0] = Terr_Read_Float(&strm);
		axis[1][1] = Terr_Read_Float(&strm);
		axis[1][2] = Terr_Read_Float(&strm);
		axis[2][0] = Terr_Read_Float(&strm);
		axis[2][1] = Terr_Read_Float(&strm);
		axis[2][2] = Terr_Read_Float(&strm);
		scale = (mf&TMF_SCALE)?Terr_Read_Float(&strm):1;

		org[0] += (s->sx-CHUNKBIAS)*hm->sectionsize;
		org[1] += (s->sy-CHUNKBIAS)*hm->sectionsize;

		Terr_AddMesh(hm, TGS_NOLOAD, mod, NULL, org, axis, scale);
	}
#endif
	return ptr;
}

static void Terr_ClearSection(hmsection_t *s)
{
	struct hmwater_s *w;
	int i;
	Sys_LockMutex(s->hmmod->entitylock);
	for (i = 0; i < s->numents; i++)
		s->ents[i]->refs-=1;
	s->numents = 0;
	Sys_UnlockMutex(s->hmmod->entitylock);

	while(s->water)
	{
		w = s->water;
		s->water = w->next;
		Z_Free(w);
	}
}

static void Terr_GenerateDefault(heightmap_t *hm, hmsection_t *s)
{
	int i;

	memset(s->holes, 0, sizeof(s->holes));

#ifdef HAVE_CLIENT
	Q_strncpyz(s->texname[0], "", sizeof(s->texname[0]));
	Q_strncpyz(s->texname[1], "", sizeof(s->texname[1]));
	Q_strncpyz(s->texname[2], "", sizeof(s->texname[2]));
	Q_strncpyz(s->texname[3], hm->defaultgroundtexture, sizeof(s->texname[3]));

	if (s->lightmap >= 0)
	{
		int j;
		qbyte *lm = lightmap[s->lightmap]->lightmaps;
		int pixbytes = lightmap[s->lightmap]->pixbytes;
		lm += (s->lmy * HMLMSTRIDE + s->lmx) * pixbytes;
		for (i = 0; i < SECTTEXSIZE; i++)
		{
			for (j = 0; j < SECTTEXSIZE; j++)
			{
				lm[j*4+0] = 0;
				lm[j*4+0] = 0;
				lm[j*4+0] = 0;
				lm[j*4+3] = 255;
			}
			lm += (HMLMSTRIDE)*pixbytes;
		}
		lightmap[s->lightmap]->modified = true;
		lightmap[s->lightmap]->rectchange.l = 0;
		lightmap[s->lightmap]->rectchange.t = 0;
		lightmap[s->lightmap]->rectchange.r = HMLMSTRIDE;
		lightmap[s->lightmap]->rectchange.b = HMLMSTRIDE;
	}
	for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
	{
		s->colours[i][0] = 1;
		s->colours[i][1] = 1;
		s->colours[i][2] = 1;
		s->colours[i][3] = 1;
	}
	s->mesh.colors4f_array[0] = s->colours;
#endif

	for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
		s->heights[i] = hm->defaultgroundheight;

	if (hm->defaultwaterheight > hm->defaultgroundheight)
		Terr_GenerateWater(s, hm->defaultwaterheight);

#if 0//def DEBUG
	void *f;
	if (lightmap_bytes == 4 && lightmap_bgra && FS_LoadFile(va("maps/%s/splatt.png", hm->path), &f) != (qofs_t)-1)
	{
		//temp
		int vx, vy;
		int x, y;
		extern qbyte *Read32BitImageFile(qbyte *buf, int len, int *width, int *height, qboolean *hasalpha, const char *fname);
		int sw, sh;
		qboolean hasalpha;
		unsigned char *splatter = Read32BitImageFile(f, com_filesize, &sw, &sh, &hasalpha, "splattermap");
		if (splatter)
		{
			lm = lightmap[s->lightmap]->lightmaps;
			lm += (s->lmy * HMLMSTRIDE + s->lmx) * lightmap_bytes;

			for (vx = 0; vx < SECTTEXSIZE; vx++)
			{
				x = sw * (((float)sy) + ((float)vx / (SECTTEXSIZE-1))) / hm->numsegsx;
				if (x > sw-1)
					x = sw-1;
				for (vy = 0; vy < SECTTEXSIZE; vy++)
				{
					y = sh * (((float)sx) + ((float)vy / (SECTTEXSIZE-1))) / hm->numsegsy;
					if (y > sh-1)
						y = sh-1;

					lm[2] = splatter[(y + x*sh)*4+0];
					lm[1] = splatter[(y + x*sh)*4+1];
					lm[0] = splatter[(y + x*sh)*4+2];
					lm[3] = splatter[(y + x*sh)*4+3];
					lm += 4;
				}
				lm += (HMLMSTRIDE - SECTTEXSIZE)*lightmap_bytes;
			}
			BZ_Free(splatter);

			lightmap[s->lightmap]->modified = true;
			lightmap[s->lightmap]->rectchange.l = 0;
			lightmap[s->lightmap]->rectchange.t = 0;
			lightmap[s->lightmap]->rectchange.w = HMLMSTRIDE;
			lightmap[s->lightmap]->rectchange.h = HMLMSTRIDE;
		}
		FS_FreeFile(f);
	}

	if (lightmap_bytes == 4 && lightmap_bgra && !qofs_Error(FS_LoadFile(va("maps/%s/heightmap.png", hm->path), &f)))
	{
		//temp
		int vx, vy;
		int x, y;
		extern qbyte *Read32BitImageFile(qbyte *buf, int len, int *width, int *height, qboolean *hasalpha, const char *fname);
		int sw, sh;
		float *h;
		qboolean hasalpha;
		unsigned char *hmimage = Read32BitImageFile(f, com_filesize, &sw, &sh, &hasalpha, "heightmap");
		if (hmimage)
		{
			h = s->heights;

			for (vx = 0; vx < SECTHEIGHTSIZE; vx++)
			{
				x = sw * (((float)sy) + ((float)vx / (SECTHEIGHTSIZE-1))) / hm->numsegsx;
				if (x > sw-1)
					x = sw-1;
				for (vy = 0; vy < SECTHEIGHTSIZE; vy++)
				{
					y = sh * (((float)sx) + ((float)vy / (SECTHEIGHTSIZE-1))) / hm->numsegsy;
					if (y > sh-1)
						y = sh-1;

					*h = 0;
					*h += hmimage[(y + x*sh)*4+0];
					*h += hmimage[(y + x*sh)*4+1]<<8;
					*h += hmimage[(y + x*sh)*4+2]<<16;
					*h *= 4.0f/(1<<16);
					h++;
				}
			}
			BZ_Free(hmimage);
		}
		FS_FreeFile(f);
	}
#endif
}

static void Terr_WorkerLoadedSection(void *ctx, void *data, size_t a, size_t b)
{
	hmsection_t *s = ctx;
	validatelinks(&s->hmmod->recycle);

	Terr_LoadSectionTextures(s);
	validatelinks2(&s->hmmod->recycle, &s->recycle);
	InsertLinkBefore(&s->recycle, &s->hmmod->recycle);
	validatelinks(&s->hmmod->recycle);
	s->hmmod->loadingsections-=1;
	s->flags &= ~TSF_EDITED;
	s->loadstate = TSLS_LOADED;
	s->timestamp = realtime;

	validatelinks(&s->hmmod->recycle);
}
static void Terr_WorkerFailedSection(void *ctx, void *data, size_t a, size_t b)
{
	hmsection_t *s = ctx;
	Terr_WorkerLoadedSection(ctx, data, a, b);
	s->flags &= ~TSF_EDITED;
	s->loadstate = TSLS_FAILED;

	validatelinks(&s->hmmod->recycle);
}

void QDECL Terr_FinishedSection(hmsection_t *s, qboolean success)
{
	s->flags &= ~TSF_EDITED;	//its just been loaded (and was probably edited by the loader), make sure it doesn't get saved or whatever

	s->loadstate = TSLS_LOADING2;
	if (!success)
		COM_AddWork(WG_MAIN, Terr_WorkerFailedSection, s, NULL, s->sx, s->sy);
	else
		COM_AddWork(WG_MAIN, Terr_WorkerLoadedSection, s, NULL, s->sx, s->sy);
}

static hmsection_t *Terr_ReadSection(heightmap_t *hm, hmsection_t *s, int ver, void *filebase, unsigned int filelen)
{
	qboolean failed = false;
	void *ptr = filebase;

	if (ptr && ver == 1)
		Terr_ReadV1(hm, s, ptr, filelen);
	else if (ptr && ver == 2)
		Terr_ReadV2(hm, s, ptr, filelen);
	else
	{
//		s->flags |= TSF_RELIGHT;
		Terr_GenerateDefault(hm, s);

		failed = true;
	}

	Terr_FinishedSection(s, !failed);

	return s;
}

#ifdef HAVE_CLIENT
qboolean Terr_DownloadedSection(char *fname)
{
/*
	qofs_t len;
	dsection_t *fileptr;
	int x, y;
	heightmap_t *hm;
	int ver = 0;

	if (!cl.worldmodel)
		return false;

	hm = cl.worldmodel->terrain;

	if (Terr_IsSectionFName(hm, fname, &x, &y))
	{
		fileptr = NULL;
		len = FS_LoadFile(fname, (void**)&fileptr);

		if (!qofs_Error(len) && len >= sizeof(*fileptr) && fileptr->magic == SECTION_MAGIC)
			Terr_ReadSection(hm, ver, x, y, fileptr+1, len - sizeof(*fileptr));
		else
			Terr_ReadSection(hm, ver, x, y, NULL, 0);

		if (fileptr)
			FS_FreeFile(fileptr);
		return true;
	}
*/
	return false;
}
#endif

#ifdef HAVE_CLIENT
static void Terr_LoadSection(heightmap_t *hm, hmsection_t *s, int sx, int sy, unsigned int flags)
{
	//when using networked terrain, the client will never load a section from disk, but will only load it from the server
	//one section at a time.
	if (mod_terrain_networked.ival && !sv_state)
	{
		char fname[MAX_QPATH];
		if (flags & TGS_NODOWNLOAD)
			return;
		//try to download it now...
		if (!cl.downloadlist)
			CL_CheckOrEnqueDownloadFile(Terr_DiskSectionName(hm, sx, sy, fname, sizeof(fname)), Terr_TempDiskSectionName(hm, sx, sy), DLLF_OVERWRITE|DLLF_TEMPORARY);
		return;
	}

	if (!s)
	{
		Terr_GenerateSection(hm, sx, sy, true);
	}
}
#endif
static void Terr_LoadSectionWorker(void *ctx, void *data, size_t a, size_t b)
{
	heightmap_t *hm = data;
	hmsection_t *s = ctx;
	int sx = a;
	int sy = b;
	void *diskimage;
	qofs_t len;
	char fname[MAX_QPATH];

	//already processed, or not otherwise valid
	if (s->loadstate != TSLS_LOADING0)
		return;

#if SECTIONSPERBLOCK > 1
	len = FS_LoadFile(Terr_DiskBlockName(hm, sx, sy, fname, sizeof(fname)), (void**)&diskimage);
	if (!qofs_Error(len))
	{
		int offset;
		int x, y;
		int ver;
		dblock_t *block = diskimage;
		if (block->magic != SECTION_MAGIC || !(block->ver & 0x80000000))
		{
			s = Terr_GenerateSection(hm, sx, sy, false);

			//give it a dummy so we don't constantly hit the disk
			Terr_ReadSection(hm, s, 0, NULL, 0);
		}
		else
		{
			hmsection_t *sects[SECTIONSPERBLOCK*SECTIONSPERBLOCK];

			sx&=~(SECTIONSPERBLOCK-1);
			sy&=~(SECTIONSPERBLOCK-1);

			ver = block->ver & ~0x80000000;
			if (Terr_GenerateSections(hm, sx, sy, SECTIONSPERBLOCK, sects))
			{
				for (y = 0; y < SECTIONSPERBLOCK; y++)
					for (x = 0; x < SECTIONSPERBLOCK; x++)
					{
						//noload avoids recursion.
						s = sects[x+y*SECTIONSPERBLOCK];
						if (s)
						{
							offset = block->offset[x + y*SECTIONSPERBLOCK];
							if (!offset)
								Terr_ReadSection(hm, s, ver, NULL, 0);	//no data in the file for this section
							else
								Terr_ReadSection(hm, s, ver, (char*)diskimage + offset, len - offset);
						}
					}
			}
		}
		FS_FreeFile(diskimage);
		return;
	}
#endif

	//legacy one-section-per-file format.
	len = FS_LoadFile(Terr_DiskSectionName(hm, sx, sy, fname, sizeof(fname)), (void**)&diskimage);
	if (!qofs_Error(len))
	{
		dsection_t *h = diskimage;
		if (len >= sizeof(*h) && h->magic == SECTION_MAGIC)
		{
			s = Terr_GenerateSection(hm, sx, sy, false);
			if (!s)
				return;
			Terr_ReadSection(hm, s, h->ver, h+1, len-sizeof(*h));
			FS_FreeFile(diskimage);
			return;
		}
		if (diskimage)
			FS_FreeFile(diskimage);
	}

	if (terrainfuncs.AutogenerateSection && terrainfuncs.AutogenerateSection(hm, sx, sy, 0))
		return;

	s = Terr_GenerateSection(hm, sx, sy, false);
	if (!s)
		return;

	//generate a dummy one
	Terr_ReadSection(hm, s, 0, NULL, 0);
}

#ifdef HAVE_CLIENT
static void Terr_SaveV1(heightmap_t *hm, hmsection_t *s, vfsfile_t *f, int sx, int sy)
{
	int i;
	dsmesh_v1_t dm;
	qbyte *lm;
	dsection_v1_t ds;
	vec4_t dcolours[SECTHEIGHTSIZE*SECTHEIGHTSIZE];
	int nothing = 0;
	struct hmwater_s *w = s->water;
	int pixbytes;

	memset(&ds, 0, sizeof(ds));
	memset(&dm, 0, sizeof(dm));

	//mask off the flags which are only valid in memory
	ds.flags = s->flags & ~(TSF_INTERNAL|TSF_HASWATER_V0);

	//kill the haswater flag if its entirely above any possible water anyway.
	if (w)
		ds.flags |= TSF_HASWATER_V0;
	ds.flags &= ~TSF_HASCOLOURS;	//recalculated

	Q_strncpyz(ds.texname[0], s->texname[0], sizeof(ds.texname[0]));
	Q_strncpyz(ds.texname[1], s->texname[1], sizeof(ds.texname[1]));
	Q_strncpyz(ds.texname[2], s->texname[2], sizeof(ds.texname[2]));
	Q_strncpyz(ds.texname[3], s->texname[3], sizeof(ds.texname[3]));

	for (i = 0; i < 8*8; i++)
	{
		int x = (i & 7);
		int y = (i>>3);
		int b = (1u<<(x>>1)) << ((y>>1)<<2);
		if (s->holes[y] & (1u<<x))
			ds.holes |= b;
	}

	//make sure the user can see the holes they just saved.
	memset(s->holes, 0, sizeof(s->holes));
	for (i = 0; i < 8*8; i++)
	{
		int x = (i & 7);
		int y = (i>>3);
		int b = (1u<<(x>>1)) << ((y>>1)<<2);
		if (ds.holes & b)
			s->holes[y] |= 1u<<x;
	}
	s->flags |= TSF_DIRTY;

	pixbytes = lightmap[s->lightmap]->pixbytes;
	lm = lightmap[s->lightmap]->lightmaps;
	lm += (s->lmy * HMLMSTRIDE + s->lmx) * pixbytes;
	for (i = 0; i < SECTTEXSIZE; i++)
	{
		memcpy(ds.texmap + i, lm, sizeof(ds.texmap[0]));
		lm += (HMLMSTRIDE)*pixbytes;
	}

	for (i = 0; i < SECTHEIGHTSIZE*SECTHEIGHTSIZE; i++)
	{
		ds.heights[i] = LittleFloat(s->heights[i]);

		if (s->colours[i][0] != 1 || s->colours[i][1] != 1 || s->colours[i][2] != 1 || s->colours[i][3] != 1)
		{
			ds.flags |= TSF_HASCOLOURS;
			dcolours[i][0] = LittleFloat(s->colours[i][0]);
			dcolours[i][1] = LittleFloat(s->colours[i][1]);
			dcolours[i][2] = LittleFloat(s->colours[i][2]);
			dcolours[i][3] = LittleFloat(s->colours[i][3]);
		}
		else
		{
			dcolours[i][0] = dcolours[i][1] = dcolours[i][2] = dcolours[i][3] = LittleFloat(1);
		}
	}
	ds.waterheight = w?w->heights[4*8+4]:s->minh;
	ds.minh = s->minh;
	ds.maxh = s->maxh;
	Sys_LockMutex(hm->entitylock);
	ds.ents_num = s->numents;

	VFS_WRITE(f, &ds, sizeof(ds));
	if (ds.flags & TSF_HASCOLOURS)
		VFS_WRITE(f, dcolours, sizeof(dcolours));
	for (i = 0; i < s->numents; i++)
	{
		int pad;
		dm.scale = s->ents[i]->ent.scale;
		VectorCopy(s->ents[i]->ent.axis[0], dm.axisorg[0]);
		VectorCopy(s->ents[i]->ent.axis[1], dm.axisorg[1]);
		VectorCopy(s->ents[i]->ent.axis[2], dm.axisorg[2]);
		VectorCopy(s->ents[i]->ent.origin, dm.axisorg[3]);
		dm.axisorg[3][0] += (CHUNKBIAS-sx)*hm->sectionsize;
		dm.axisorg[3][1] += (CHUNKBIAS-sy)*hm->sectionsize;
		dm.size = sizeof(dm) + strlen(s->ents[i]->ent.model->name) + 1;
		if (dm.size & 3)
			pad = 4 - (dm.size&3);
		else
			pad = 0;
		dm.size += pad;
		VFS_WRITE(f, &dm, sizeof(dm));
		VFS_WRITE(f, s->ents[i]->ent.model->name, strlen(s->ents[i]->ent.model->name)+1);
		if (pad)
			VFS_WRITE(f, &nothing, pad);
	}
	Sys_UnlockMutex(hm->entitylock);
}

static void Terr_Save(heightmap_t *hm, hmsection_t *s, vfsfile_t *f, int sx, int sy, int ver)
{
	if (ver == 1)
		Terr_SaveV1(hm, s, f, sx, sy);
	else if (ver == 2)
		Terr_SaveV2(hm, s, f, sx, sy);
}
#endif

//doesn't clear edited/dirty flags or anything
static qboolean Terr_SaveSection(heightmap_t *hm, hmsection_t *s, int sx, int sy, qboolean blocksave)
{
#ifndef HAVE_CLIENT
	return true;
#else
	vfsfile_t *f;
	char fname[MAX_QPATH];
	int x, y;
	int writever = mod_terrain_savever.ival;
	if (!writever)
		writever = SECTION_VER_DEFAULT;
	//if its invalid or doesn't contain all the data...
	if (!s || s->lightmap < 0)
		return true;

#if SECTIONSPERBLOCK > 1
	if (blocksave)
	{
		dblock_t dbh;
		sx = sx & ~(SECTIONSPERBLOCK-1);
		sy = sy & ~(SECTIONSPERBLOCK-1);

		//make sure its loaded before we replace the file
		for (y = 0; y < SECTIONSPERBLOCK; y++)
		{
			for (x = 0; x < SECTIONSPERBLOCK; x++)
			{
				s = Terr_GetSection(hm, sx+x, sy+y, TGS_WAITLOAD|TGS_NODOWNLOAD);
				if (s)
					s->flags |= TSF_EDITED;	//stop them from getting reused for something else.
			}
		}

		//make sure all lightmap info was loaded.
		COM_WorkerFullSync();

		Terr_DiskBlockName(hm, sx, sy, fname, sizeof(fname));
		FS_CreatePath(fname, FS_GAMEONLY);
		f = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
		if (!f)
		{
			Con_Printf("Failed to open %s\n", fname);
			return false;
		}

		memset(&dbh, 0, sizeof(dbh));
		dbh.magic = LittleLong(SECTION_MAGIC);
		dbh.ver = LittleLong(writever | 0x80000000);
		VFS_WRITE(f, &dbh, sizeof(dbh));
		for (y = 0; y < SECTIONSPERBLOCK; y++)
		{
			for (x = 0; x < SECTIONSPERBLOCK; x++)
			{
				s = Terr_GetSection(hm, sx+x, sy+y, TGS_WAITLOAD|TGS_NODOWNLOAD);
				if (s && s->loadstate == TSLS_LOADED && Terr_InitLightmap(s, false))
				{
					dbh.offset[y*SECTIONSPERBLOCK + x] = VFS_TELL(f);
					Terr_Save(hm, s, f, sx+x, sy+y, writever);
					s->flags &= ~TSF_EDITED;
				}
				else
					dbh.offset[y*SECTIONSPERBLOCK + x] = 0;
			}
		}

		VFS_SEEK(f, 0);
		VFS_WRITE(f, &dbh, sizeof(dbh));
		VFS_CLOSE(f);
		FS_FlushFSHashWritten(fname);
	}
	else
#endif
	{
		dsection_t dsh;
		Terr_DiskSectionName(hm, sx, sy, fname, sizeof(fname));

//		if (s && (s->flags & (TSF_EDITED|TSF_FAILEDLOAD)) != TSF_FAILEDLOAD)
//			return FS_Remove(fname, FS_GAMEONLY);	//delete the file if the section got reverted to default, and wasn't later modified.

		//make sure all lightmap info was loaded.
		COM_WorkerFullSync();

		FS_CreatePath(fname, FS_GAMEONLY);
		f = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
		if (!f)
		{
			Con_Printf("Failed to open %s\n", fname);
			return false;
		}

		memset(&dsh, 0, sizeof(dsh));
		dsh.magic = SECTION_MAGIC;
		dsh.ver = writever;
		VFS_WRITE(f, &dsh, sizeof(dsh));
		Terr_Save(hm, s, f, sx, sy, writever);
		VFS_CLOSE(f);
		FS_FlushFSHashWritten(fname);
	}
	return true;
#endif
}

/*convienience function*/
static hmsection_t *QDECL Terr_GetSection(heightmap_t *hm, int x, int y, unsigned int flags)
{
	hmcluster_t *cluster;
	hmsection_t *section;
	int cx = x / MAXSECTIONS;
	int cy = y / MAXSECTIONS;
	int sx = x & (MAXSECTIONS-1);
	int sy = y & (MAXSECTIONS-1);
	cluster = hm->cluster[cx + cy*MAXCLUSTERS];
	if (!cluster)
		section = NULL;
	else
		section = cluster->section[sx + sy*MAXSECTIONS];
	if (!section)
	{
		if (flags & (TGS_LAZYLOAD|TGS_TRYLOAD|TGS_WAITLOAD))
		{
			if ((flags & TGS_LAZYLOAD) && hm->loadingsections)
				return NULL;
			section = Terr_GenerateSection(hm, x, y, true);
		}
	}
#ifdef HAVE_CLIENT
	//when using networked terrain, the client will never load a section from disk, but only loading it from the server
	//this means we need to send a new request to download the section if it was flagged as modified.
	if (!(flags & TGS_NODOWNLOAD))
	if (section && (section->flags & TSF_NOTIFY) && mod_terrain_networked.ival && !sv_state)
	{
		//try to download it now...
		if (!cl.downloadlist)
		{
			char fname[MAX_QPATH];
			CL_CheckOrEnqueDownloadFile(Terr_DiskSectionName(hm, x, y, fname, sizeof(fname)), Terr_TempDiskSectionName(hm, x, y), DLLF_OVERWRITE|DLLF_TEMPORARY);

			section->flags &= ~TSF_NOTIFY;
		}
	}
#endif

	if (section && section->loadstate != TSLS_LOADED)
	{
		//wait for it to load if we're meant to be doing that.
		if (flags & TGS_WAITLOAD)
		{
			//wait for it to load if we're meant to be doing that.
			if (section->loadstate == TSLS_LOADING0)
				COM_WorkerPartialSync(section, &section->loadstate, TSLS_LOADING0);
			if (section->loadstate == TSLS_LOADING1)
				COM_WorkerPartialSync(section, &section->loadstate, TSLS_LOADING1);
			if (section->loadstate == TSLS_LOADING2)
				COM_MainThreadFlush();	//make sure any associated lightmaps also got read+handled
		}

		//if it failed, generate a default (for editing)
		if (section->loadstate == TSLS_FAILED && ((flags & TGS_DEFAULTONFAIL) || hm->forcedefault))
		{
			section->flags = (section->flags & ~TSF_EDITED);
			section->loadstate = TSLS_LOADED;
			Terr_ClearSection(section);
			Terr_GenerateDefault(hm, section);
		}


		if ((section->loadstate != TSLS_LOADED) && !(flags & TGS_ANYSTATE))
			section = NULL;
	}
	if (section)
		section->timestamp = realtime;

	return section;
}

/*save all currently loaded sections*/
int Heightmap_Save(heightmap_t *hm)
{
	hmsection_t *s;
	int x, y;
	int sectionssaved = 0;
	for (x = hm->firstsegx; x < hm->maxsegx; x++)
	{
		for (y = hm->firstsegy; y < hm->maxsegy; y++)
		{
			s = Terr_GetSection(hm, x, y, TGS_NOLOAD);
			if (!s)
				continue;
			if (s->flags & TSF_EDITED)
			{
/*				//make sure all the parts are loaded before trying to write them, so we don't try reading partial files, which would be bad, mmkay?
				for (sy = y&~(SECTIONSPERBLOCK-1); sy < y+SECTIONSPERBLOCK && sy < hm->maxsegy; sy++)
				{
					for (sx = x&~(SECTIONSPERBLOCK-1); sx < x+SECTIONSPERBLOCK && sx < hm->maxsegx; sx++)
					{
						os = Terr_GetSection(hm, sx, sy, TGS_WAITLOAD|TGS_NODOWNLOAD|TGS_NORENDER);
						if (os)
							os->flags |= TSF_EDITED;
					}
				}
*/

				if (Terr_SaveSection(hm, s, x, y, true))
				{
					s->flags &= ~TSF_EDITED;
					sectionssaved++;
				}
			}
		}
	}

	return sectionssaved;
}

#ifdef HAVE_SERVER
//on servers, we can get requests to download current map sections. if so, give them it.
qboolean Terrain_LocateSection(const char *name, flocation_t *loc)
{
	heightmap_t *hm;
	hmsection_t *s;
	int x, y;
	char fname[MAX_QPATH];

	//reject if its not in maps
	if (Q_strncasecmp(name, "maps/", 5))
		return false;

	if (!sv.world.worldmodel)
		return false;
	hm = sv.world.worldmodel->terrain;
	if (!Terr_IsSectionFName(hm, name, &x, &y))
		return false;

	//verify that its valid
	if (strcmp(name, Terr_DiskSectionName(hm, x, y, fname, sizeof(fname))))
		return false;

	s = Terr_GetSection(hm, x, y, TGS_NOLOAD);
	if (!s || !(s->flags & TSF_EDITED))
		return false;	//its not been edited, might as well just use the regular file

	if (!Terr_SaveSection(hm, s, x, y, false))
		return false;

	return FS_FLocateFile(name, FSLF_IFFOUND, loc);
}
#endif

void Terr_DestroySection(heightmap_t *hm, hmsection_t *s, qboolean lightmapreusable)
{
	if (s && s->loadstate == TSLS_LOADING0)
		COM_WorkerPartialSync(s, &s->loadstate, TSLS_LOADING0);
	if (s && s->loadstate == TSLS_LOADING1)
		COM_WorkerPartialSync(s, &s->loadstate, TSLS_LOADING1);
	if (s && s->loadstate == TSLS_LOADING2)
		COM_MainThreadFlush();	//make sure any associated lightmaps also got read+handled

	if (!s || s->loadstate < TSLS_LOADING2)
		return;

	{
		int cx = s->sx/MAXSECTIONS;
		int cy = s->sy/MAXSECTIONS;
		hmcluster_t *c = hm->cluster[cx + cy*MAXCLUSTERS];
		int sx = s->sx & (MAXSECTIONS-1);
		int sy = s->sy & (MAXSECTIONS-1);

		if (c->section[sx+sy*MAXSECTIONS] != s)
			Sys_Error("Section %i,%i already destroyed...\n", s->sx, s->sy);
		c->section[sx+sy*MAXSECTIONS] = NULL;
	}

	validatelinks(&hm->recycle);

	RemoveLink(&s->recycle);
	validatelinks(&s->hmmod->recycle);

	Terr_ClearSection(s);

#ifdef HAVE_CLIENT
	if (s->lightmap >= 0)
	{
		struct lmsect_s *lms;

		if (lightmapreusable)
		{
			lms = BZ_Malloc(sizeof(*lms));
			lms->lm = s->lightmap;
			lms->x = s->lmx;
			lms->y = s->lmy;
			lms->next = hm->unusedlmsects;
			hm->unusedlmsects = lms;
			hm->numunusedlmsects++;
		}
		hm->numusedlmsects--;
	}

	if (hm->relight == s)
		hm->relight = NULL;

#ifdef GLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		if (qglDeleteBuffersARB)
		{
			if (s->vbo.coord.gl.vbo)
			{
				qglDeleteBuffersARB(1, &s->vbo.coord.gl.vbo);
				s->vbo.coord.gl.vbo = 0;
			}
			if (s->vbo.indicies.gl.vbo)
			{
				qglDeleteBuffersARB(1, &s->vbo.indicies.gl.vbo);
				s->vbo.indicies.gl.vbo = 0;
			}
		}
	}
	else
#endif
	{
		BE_ClearVBO(&s->vbo, true);
	}

	Z_Free(s->ents);
	Z_Free(s->mesh.xyz_array);
	Z_Free(s->mesh.indexes);
#endif

	Z_Free(s);

	hm->activesections--;

	validatelinks(&hm->recycle);
}

#ifdef HAVE_CLIENT
//dedicated servers do not support editing. no lightmap info causes problems.

//when a terrain section has the notify flag set on the server, the server needs to go through and set out notifications to replicate it to the various clients
//so the clients know to re-download the section.
static void Terr_DoEditNotify(heightmap_t *hm)
{
#ifdef HAVE_SERVER
	int i;
	char *cmd;
	hmsection_t *s;
	link_t *ln = &hm->recycle;

	if (!sv_state)
		return;

	for (i = 0; i < sv.allocated_client_slots; i++)
	{
		if (svs.clients[i].state >= cs_connected && svs.clients[i].netchan.remote_address.type != NA_LOOPBACK)
		{
			if (svs.clients[i].backbuf.cursize)
				return;
		}
	}

	for (ln = &hm->recycle; ln->next != &hm->recycle; ln = &s->recycle)
	{
		s = (hmsection_t*)ln->next;
		if (s->flags & TSF_NOTIFY)
		{
			s->flags &= ~TSF_NOTIFY;
			cmd = va("mod_terrain_reload %s %i %i\n", hm->path, s->sx - CHUNKBIAS, s->sy - CHUNKBIAS);
			for (i = 0; i < sv.allocated_client_slots; i++)
			{
				if (svs.clients[i].state >= cs_connected && svs.clients[i].netchan.remote_address.type != NA_LOOPBACK)
				{
					SV_StuffcmdToClient(&svs.clients[i], cmd);
				}
			}
			return;
		}
	}
#endif
}

//garbage collect the oldest section, to make space for another
static qboolean Terr_Collect(heightmap_t *hm)
{
	hmcluster_t *c;
	hmsection_t *s;
	int cx, cy;
	int sx, sy;
	float timeout = realtime-2;	//must used no later than 2 seconds in the past

	link_t *ln = &hm->recycle;
	validatelinks(&hm->recycle);
	for (ln = &hm->recycle; ln->next != &hm->recycle; )
	{
		s = (hmsection_t*)ln->next;
		if ((s->flags & TSF_EDITED) || s->loadstate <= TSLS_LOADING2 || s->timestamp > timeout)
			ln = &s->recycle;
		else
		{
			cx = s->sx/MAXSECTIONS;
			cy = s->sy/MAXSECTIONS;
			c = hm->cluster[cx + cy*MAXCLUSTERS];
			sx = s->sx & (MAXSECTIONS-1);
			sy = s->sy & (MAXSECTIONS-1);
			if (c->section[sx+sy*MAXSECTIONS] != s)
				Sys_Error("invalid section collection");
			c->section[sx+sy*MAXSECTIONS] = NULL;

#if 0
			if (hm->relight == s)
				hm->relight = NULL;
			RemoveLink(&s->recycle);
			InsertLinkAfter(&s->recycle, &hm->collected);
			hm->activesections--;
#else
			Terr_DestroySection(hm, s, true);
#endif
			validatelinks(&hm->recycle);
			return true;
		}
	}
	return false;
}
#endif

/*purge all sections, but not root
lightmaps only are purged whenever the client rudely kills lightmaps (purges all lightmaps on map changes, to cope with models/maps potentially being unloaded)
we'll reload those when its next seen.
(lightmaps will already have been destroyed, so no poking them)
*/
void Terr_PurgeTerrainModel(model_t *mod, qboolean lightmapsonly, qboolean lightmapreusable)
{
	heightmap_t *hm = mod->terrain;
	hmcluster_t *c;
	hmsection_t *s;
	int cx, cy;
	int sx, sy;

	COM_WorkerFullSync();	//should probably be inside the caller or something. make sure there's no loaders still loading lightmaps when lightmaps are going to be nuked.


validatelinks(&hm->recycle);

//	Con_Printf("PrePurge: %i lm chunks used, %i unused\n", hm->numusedlmsects, hm->numunusedlmsects);

	for (cy = 0; cy < MAXCLUSTERS; cy++)
	for (cx = 0; cx < MAXCLUSTERS; cx++)
	{
		int numremaining = 0;
		c = hm->cluster[cx + cy*MAXCLUSTERS];
		if (!c)
			continue;

		for (sy = 0; sy < MAXSECTIONS; sy++)
		for (sx = 0; sx < MAXSECTIONS; sx++)
		{
			s = c->section[sx + sy*MAXSECTIONS];
			if (!s)
			{
			}
			else if (lightmapsonly)
			{
				numremaining++;
#ifdef HAVE_CLIENT
				s->lightmap = -1;
#endif
			}
			else
			{
				validatelinks(&hm->recycle);
				Terr_DestroySection(hm, s, lightmapreusable);
				validatelinks(&hm->recycle);
			}
		}
		if (!numremaining)
		{
			hm->cluster[cx + cy*MAXSECTIONS] = NULL;
			BZ_Free(c);
			validatelinks(&hm->recycle);
		}
	}
	validatelinks(&hm->recycle);
#ifdef HAVE_CLIENT
	if (!lightmapreusable)
	{
		while (hm->unusedlmsects)
		{
			struct lmsect_s *lms;
			lms = hm->unusedlmsects;
			hm->unusedlmsects = lms->next;
			BZ_Free(lms);

			hm->numunusedlmsects--;
		}


		hm->recalculatebrushlighting = true;
		BZ_Free(hm->brushlmremaps);
		hm->brushlmremaps = NULL;
		hm->brushmaxlms = 0;
	}
#endif
	validatelinks(&hm->recycle);

//	Con_Printf("PostPurge: %i lm chunks used, %i unused\n", hm->numusedlmsects, hm->numunusedlmsects);
}

void Terr_FreeModel(model_t *mod)
{
	heightmap_t *hm = mod->terrain;
	if (hm)
	{
		validatelinks(&hm->recycle);
		while(hm->numbrushes)
			Terr_Brush_DeleteIdx(hm, hm->numbrushes-1);
		while(hm->brushtextures)
		{
			brushtex_t *bt = hm->brushtextures;
#ifdef HAVE_CLIENT
			brushbatch_t *bb;
			while((bb = bt->batches))
			{
				bt->batches = bb->next;
				BE_VBO_Destroy(&bb->vbo.coord, bb->vbo.vbomem);
				BE_VBO_Destroy(&bb->vbo.indicies, bb->vbo.ebomem);
				BZ_Free(bb);
			}
#endif
			hm->brushtextures = bt->next;
			BZ_Free(bt);
		}
#ifdef RUNTIMELIGHTING
		if (hm->relightcontext)
			LightShutdown(hm->relightcontext, mod);
		if (hm->lightthreadmem && !hm->inheritedlightthreadmem)
			BZ_Free(hm->lightthreadmem);
#endif
		BZ_Free(hm->wbrushes);
		Terr_PurgeTerrainModel(mod, false, false);
		while(hm->entities)
		{
			struct hmentity_s *n = hm->entities->next;
			Z_Free(hm->entities);
			hm->entities = n;
		}
		Sys_DestroyMutex(hm->entitylock);
		Z_Free(hm->seed);
		Z_Free(hm);
		mod->terrain = NULL;
	}
}

#ifdef HAVE_CLIENT
void Terr_DrawTerrainWater(heightmap_t *hm, float *mins, float *maxs, struct hmwater_s *w)
{
	scenetris_t *t;
	int flags = BEF_NOSHADOWS;
	int firstv;
	int y, x;
	
	//need to filter by height too, or reflections won't work properly.
	if (cl_numstris && cl_stris[cl_numstris-1].shader == w->shader && cl_stris[cl_numstris-1].flags == flags && cl_strisvertv[cl_stris[cl_numstris-1].firstvert][2] == w->maxheight)
	{
		t = &cl_stris[cl_numstris-1];
	}
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris+=8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = w->shader;
		t->flags = flags;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->numvert = 0;
		t->numidx = 0;
	}

	if (!w->simple)
	{
		float step = (maxs[0] - mins[0]) / 8;
		if (cl_numstrisidx+9*9*6 > cl_maxstrisidx)
		{
			cl_maxstrisidx=cl_numstrisidx+12 + 9*9*6*4;
			cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
		}
		if (cl_numstrisvert+9*9 > cl_maxstrisvert)
			cl_stris_ExpandVerts(cl_numstrisvert+9*9+64);

		firstv = t->numvert;
		for (y = 0; y < 9; y++)
		{
			for (x = 0; x < 9; x++)
			{
				cl_strisvertv[cl_numstrisvert][0] = mins[0] + step*x;
				cl_strisvertv[cl_numstrisvert][1] = mins[1] + step*y;
				cl_strisvertv[cl_numstrisvert][2] = w->heights[x + y*9];
				cl_strisvertt[cl_numstrisvert][0] = cl_strisvertv[cl_numstrisvert][0]/64;
				cl_strisvertt[cl_numstrisvert][1] = cl_strisvertv[cl_numstrisvert][1]/64;
				Vector4Set(cl_strisvertc[cl_numstrisvert], 1,1,1,1);
				cl_numstrisvert++;
			}
		}
		for (y = 0; y < 8; y++)
		{
			for (x = 0; x < 8; x++)
			{
				if (w->holes[y] & (1u<<x))
					continue;
				cl_strisidx[cl_numstrisidx++] = firstv+(x+0)+(y+0)*9;
				cl_strisidx[cl_numstrisidx++] = firstv+(x+0)+(y+1)*9;
				cl_strisidx[cl_numstrisidx++] = firstv+(x+1)+(y+0)*9;

				cl_strisidx[cl_numstrisidx++] = firstv+(x+1)+(y+0)*9;
				cl_strisidx[cl_numstrisidx++] = firstv+(x+0)+(y+1)*9;
				cl_strisidx[cl_numstrisidx++] = firstv+(x+1)+(y+1)*9;
			}
		}
		t->numidx = cl_numstrisidx - t->firstidx;
		t->numvert = cl_numstrisvert - t->firstvert;
	}
	else
	{
		if (cl_numstrisidx+12 > cl_maxstrisidx)
		{
			cl_maxstrisidx=cl_numstrisidx+12 + 64;
			cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
		}
		if (cl_numstrisvert+4 > cl_maxstrisvert)
			cl_stris_ExpandVerts(cl_numstrisvert+64);

		{
			VectorSet(cl_strisvertv[cl_numstrisvert], mins[0], mins[1], w->maxheight);
			Vector4Set(cl_strisvertc[cl_numstrisvert], 1,1,1,1);
			Vector2Set(cl_strisvertt[cl_numstrisvert], mins[0]/64, mins[1]/64);
			cl_numstrisvert++;

			VectorSet(cl_strisvertv[cl_numstrisvert], mins[0], maxs[1], w->maxheight);
			Vector4Set(cl_strisvertc[cl_numstrisvert], 1,1,1,1);
			Vector2Set(cl_strisvertt[cl_numstrisvert], mins[0]/64, maxs[1]/64);
			cl_numstrisvert++;

			VectorSet(cl_strisvertv[cl_numstrisvert], maxs[0], maxs[1], w->maxheight);
			Vector4Set(cl_strisvertc[cl_numstrisvert], 1,1,1,1);
			Vector2Set(cl_strisvertt[cl_numstrisvert], maxs[0]/64, maxs[1]/64);
			cl_numstrisvert++;

			VectorSet(cl_strisvertv[cl_numstrisvert], maxs[0], mins[1], w->maxheight);
			Vector4Set(cl_strisvertc[cl_numstrisvert], 1,1,1,1);
			Vector2Set(cl_strisvertt[cl_numstrisvert], maxs[0]/64, mins[1]/64);
			cl_numstrisvert++;
		}


		firstv = t->numvert;

		/*build the triangles*/
		cl_strisidx[cl_numstrisidx++] = firstv + 0;
		cl_strisidx[cl_numstrisidx++] = firstv + 1;
		cl_strisidx[cl_numstrisidx++] = firstv + 2;

		cl_strisidx[cl_numstrisidx++] = firstv + 0;
		cl_strisidx[cl_numstrisidx++] = firstv + 2;
		cl_strisidx[cl_numstrisidx++] = firstv + 3;

		cl_strisidx[cl_numstrisidx++] = firstv + 3;
		cl_strisidx[cl_numstrisidx++] = firstv + 2;
		cl_strisidx[cl_numstrisidx++] = firstv + 1;

		cl_strisidx[cl_numstrisidx++] = firstv + 3;
		cl_strisidx[cl_numstrisidx++] = firstv + 1;
		cl_strisidx[cl_numstrisidx++] = firstv + 0;


		t->numidx = cl_numstrisidx - t->firstidx;
		t->numvert = cl_numstrisvert - t->firstvert;
	}
}

static void Terr_RebuildMesh(model_t *model, hmsection_t *s, int x, int y)
{
	int vx, vy;
	int v;
	mesh_t *mesh = &s->mesh;
	heightmap_t *hm = s->hmmod;
	
	Terr_InitLightmap(s, false);

	s->minh = 9999999999999999.f;
	s->maxh = -9999999999999999.f;

	switch(hm->mode)
	{
	case HMM_BLOCKS:
		//tiles, like dungeon keeper
		if (mesh->xyz_array)
			BZ_Free(mesh->xyz_array);
		{
			mesh->xyz_array = BZ_Malloc((sizeof(vecV_t)+sizeof(vec2_t)+sizeof(vec2_t)) * (SECTHEIGHTSIZE-1)*(SECTHEIGHTSIZE-1)*4*3);
			mesh->st_array = (void*) (mesh->xyz_array + (SECTHEIGHTSIZE-1)*(SECTHEIGHTSIZE-1)*4*3);
			mesh->lmst_array[0] = (void*) (mesh->st_array + (SECTHEIGHTSIZE-1)*(SECTHEIGHTSIZE-1)*4*3);
		}
		mesh->numvertexes = 0;

		if (mesh->indexes)
			BZ_Free(mesh->indexes);
		mesh->indexes = BZ_Malloc(sizeof(index_t) * SECTHEIGHTSIZE*SECTHEIGHTSIZE*6*3);
		mesh->numindexes = 0;
		mesh->colors4f_array[0] = NULL;

		for (vy = 0; vy < SECTHEIGHTSIZE-1; vy++)
		{
			for (vx = 0; vx < SECTHEIGHTSIZE-1; vx++)
			{
				float st[2], inst[2];
#if SECTHEIGHTSIZE == 17
				int holebit;
				int holerow;

				//skip generation of the mesh above holes
				holerow = ((vy<<3)/(SECTHEIGHTSIZE-1));
				holebit = 1u<<((vx<<3)/(SECTHEIGHTSIZE-1));
				if (s->holes[holerow] & holebit)
					continue;
#endif

				//top face
				v = mesh->numvertexes;
				mesh->numvertexes += 4;
				mesh->xyz_array[v+0][0] = (x-CHUNKBIAS + (vx+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+0][1] = (y-CHUNKBIAS + (vy+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+0][2] = s->heights[vx + vy*SECTHEIGHTSIZE];

				mesh->xyz_array[v+1][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+1][1] = (y-CHUNKBIAS + (vy+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+1][2] = s->heights[vx + vy*SECTHEIGHTSIZE];

				mesh->xyz_array[v+2][0] = (x-CHUNKBIAS + (vx+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+2][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+2][2] = s->heights[vx + vy*SECTHEIGHTSIZE];

				mesh->xyz_array[v+3][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+3][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+3][2] = s->heights[vx + vy*SECTHEIGHTSIZE];

				if (s->maxh < mesh->xyz_array[v][2])
					s->maxh = mesh->xyz_array[v][2];
				if (s->minh > mesh->xyz_array[v][2])
					s->minh = mesh->xyz_array[v][2];

				st[0] = 1.0f/hm->tilecount[0] * vx;
				st[1] = 1.0f/hm->tilecount[1] * vy;
				inst[0] = 0.5f/(hm->tilecount[0]*hm->tilepixcount[0]);
				inst[1] = 0.5f/(hm->tilecount[1]*hm->tilepixcount[1]);
				mesh->st_array[v+0][0] = st[0]+inst[0];
				mesh->st_array[v+0][1] = st[1]+inst[1];
				mesh->st_array[v+1][0] = st[0]-inst[0]+1.0f/hm->tilecount[0];
				mesh->st_array[v+1][1] = st[1]+inst[1];
				mesh->st_array[v+2][0] = st[0]+inst[0];
				mesh->st_array[v+2][1] = st[1]-inst[1]+1.0f/hm->tilecount[1];
				mesh->st_array[v+3][0] = st[0]-inst[0]+1.0f/hm->tilecount[0];
				mesh->st_array[v+3][1] = st[1]-inst[1]+1.0f/hm->tilecount[1];

				//calc the position in the range -0.5 to 0.5
				mesh->lmst_array[0][v][0] = (((float)vx / (SECTHEIGHTSIZE-1))-0.5);
				mesh->lmst_array[0][v][1] = (((float)vy / (SECTHEIGHTSIZE-1))-0.5);
				//scale down to a half-texel
				mesh->lmst_array[0][v][0] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				mesh->lmst_array[0][v][1] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				//bias it
				mesh->lmst_array[0][v][0] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmx) / HMLMSTRIDE);
				mesh->lmst_array[0][v][1] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmy) / HMLMSTRIDE);

				mesh->indexes[mesh->numindexes++] = v+0;
				mesh->indexes[mesh->numindexes++] = v+2;
				mesh->indexes[mesh->numindexes++] = v+1;
				mesh->indexes[mesh->numindexes++] = v+1;
				mesh->indexes[mesh->numindexes++] = v+2;
				mesh->indexes[mesh->numindexes++] = v+1+2;


				//x boundary
				v = mesh->numvertexes;
				mesh->numvertexes += 4;
				mesh->xyz_array[v+0][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+0][1] = (y-CHUNKBIAS + (vy+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+0][2] = s->heights[vx+0 + vy*SECTHEIGHTSIZE];

				mesh->xyz_array[v+1][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+1][1] = (y-CHUNKBIAS + (vy+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+1][2] = s->heights[(vx+1) + vy*SECTHEIGHTSIZE];

				mesh->xyz_array[v+2][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+2][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+2][2] = s->heights[(vx+0) + vy*SECTHEIGHTSIZE];

				mesh->xyz_array[v+3][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+3][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+3][2] = s->heights[(vx+1) + vy*SECTHEIGHTSIZE];

				if (s->maxh < mesh->xyz_array[v][2])
					s->maxh = mesh->xyz_array[v][2];
				if (s->minh > mesh->xyz_array[v][2])
					s->minh = mesh->xyz_array[v][2];

				st[0] = 1.0f/hm->tilecount[0] * vx;
				st[1] = 1.0f/hm->tilecount[1] * vy;
				inst[0] = 0.5f/(hm->tilecount[0]*hm->tilepixcount[0]);
				inst[1] = 0.5f/(hm->tilecount[1]*hm->tilepixcount[1]);
				mesh->st_array[v+0][0] = st[0]+inst[0];
				mesh->st_array[v+0][1] = st[1]+inst[1];
				mesh->st_array[v+1][0] = st[0]+inst[0];
				mesh->st_array[v+1][1] = st[1]-inst[1]+1.0f/hm->tilecount[1];
				mesh->st_array[v+2][0] = st[0]-inst[0]+1.0f/hm->tilecount[0];
				mesh->st_array[v+2][1] = st[1]+inst[1];
				mesh->st_array[v+3][0] = st[0]-inst[0]+1.0f/hm->tilecount[0];
				mesh->st_array[v+3][1] = st[1]-inst[1]+1.0f/hm->tilecount[1];

				//calc the position in the range -0.5 to 0.5
				mesh->lmst_array[0][v][0] = (((float)vx / (SECTHEIGHTSIZE-1))-0.5);
				mesh->lmst_array[0][v][1] = (((float)vy / (SECTHEIGHTSIZE-1))-0.5);
				//scale down to a half-texel
				mesh->lmst_array[0][v][0] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				mesh->lmst_array[0][v][1] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				//bias it
				mesh->lmst_array[0][v][0] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmx) / HMLMSTRIDE);
				mesh->lmst_array[0][v][1] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmy) / HMLMSTRIDE);


				mesh->indexes[mesh->numindexes++] = v+0;
				mesh->indexes[mesh->numindexes++] = v+2;
				mesh->indexes[mesh->numindexes++] = v+1;
				mesh->indexes[mesh->numindexes++] = v+1;
				mesh->indexes[mesh->numindexes++] = v+2;
				mesh->indexes[mesh->numindexes++] = v+1+2;

				//y boundary
				v = mesh->numvertexes;
				mesh->numvertexes += 4;
				mesh->xyz_array[v+0][0] = (x-CHUNKBIAS + (vx+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+0][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+0][2] = s->heights[vx + (vy+0)*SECTHEIGHTSIZE];

				mesh->xyz_array[v+1][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+1][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+1][2] = s->heights[vx + (vy+0)*SECTHEIGHTSIZE];

				mesh->xyz_array[v+2][0] = (x-CHUNKBIAS + (vx+0)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+2][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+2][2] = s->heights[vx + (vy+1)*SECTHEIGHTSIZE];

				mesh->xyz_array[v+3][0] = (x-CHUNKBIAS + (vx+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+3][1] = (y-CHUNKBIAS + (vy+1)/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v+3][2] = s->heights[vx + (vy+1)*SECTHEIGHTSIZE];

				if (s->maxh < mesh->xyz_array[v][2])
					s->maxh = mesh->xyz_array[v][2];
				if (s->minh > mesh->xyz_array[v][2])
					s->minh = mesh->xyz_array[v][2];

				st[0] = 1.0f/hm->tilecount[0] * vx;
				st[1] = 1.0f/hm->tilecount[1] * vy;
				inst[0] = 0.5f/(hm->tilecount[0]*hm->tilepixcount[0]);
				inst[1] = 0.5f/(hm->tilecount[1]*hm->tilepixcount[1]);
				mesh->st_array[v+0][0] = st[0]+inst[0];
				mesh->st_array[v+0][1] = st[1]+inst[1];
				mesh->st_array[v+1][0] = st[0]-inst[0]+1.0f/hm->tilecount[0];
				mesh->st_array[v+1][1] = st[1]+inst[1];
				mesh->st_array[v+2][0] = st[0]+inst[0];
				mesh->st_array[v+2][1] = st[1]-inst[1]+1.0f/hm->tilecount[1];
				mesh->st_array[v+3][0] = st[0]-inst[0]+1.0f/hm->tilecount[0];
				mesh->st_array[v+3][1] = st[1]-inst[1]+1.0f/hm->tilecount[1];

				//calc the position in the range -0.5 to 0.5
				mesh->lmst_array[0][v][0] = (((float)vx / (SECTHEIGHTSIZE-1))-0.5);
				mesh->lmst_array[0][v][1] = (((float)vy / (SECTHEIGHTSIZE-1))-0.5);
				//scale down to a half-texel
				mesh->lmst_array[0][v][0] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				mesh->lmst_array[0][v][1] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				//bias it
				mesh->lmst_array[0][v][0] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmx) / HMLMSTRIDE);
				mesh->lmst_array[0][v][1] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmy) / HMLMSTRIDE);

				mesh->indexes[mesh->numindexes++] = v+0;
				mesh->indexes[mesh->numindexes++] = v+2;
				mesh->indexes[mesh->numindexes++] = v+1;
				mesh->indexes[mesh->numindexes++] = v+1;
				mesh->indexes[mesh->numindexes++] = v+2;
				mesh->indexes[mesh->numindexes++] = v+1+2;
			}
		}
		break;
	case HMM_TERRAIN:
		//smooth terrain
		if (!mesh->xyz_array)
		{
			mesh->xyz_array = BZ_Malloc((sizeof(vecV_t)+sizeof(vec2_t)+sizeof(vec2_t)) * (SECTHEIGHTSIZE)*(SECTHEIGHTSIZE));
			mesh->st_array = (void*) (mesh->xyz_array + (SECTHEIGHTSIZE)*(SECTHEIGHTSIZE));
			mesh->lmst_array[0] = (void*) (mesh->st_array + (SECTHEIGHTSIZE)*(SECTHEIGHTSIZE));
		}
		mesh->colors4f_array[0] = s->colours;
		mesh->numvertexes = 0;
		/*64 quads across requires 65 verticies*/
		for (vy = 0; vy < SECTHEIGHTSIZE; vy++)
		{
			for (vx = 0; vx < SECTHEIGHTSIZE; vx++)
			{
				v = mesh->numvertexes++;
				mesh->xyz_array[v][0] = (x-CHUNKBIAS + vx/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v][1] = (y-CHUNKBIAS + vy/(SECTHEIGHTSIZE-1.0f)) * hm->sectionsize;
				mesh->xyz_array[v][2] = s->heights[vx + vy*SECTHEIGHTSIZE];

				if (s->maxh < mesh->xyz_array[v][2])
					s->maxh = mesh->xyz_array[v][2];
				if (s->minh > mesh->xyz_array[v][2])
					s->minh = mesh->xyz_array[v][2];

				mesh->st_array[v][0] = mesh->xyz_array[v][0] / 128;
				mesh->st_array[v][1] = mesh->xyz_array[v][1] / 128;

				//calc the position in the range -0.5 to 0.5
				mesh->lmst_array[0][v][0] = (((float)vx / (SECTHEIGHTSIZE-1))-0.5);
				mesh->lmst_array[0][v][1] = (((float)vy / (SECTHEIGHTSIZE-1))-0.5);
				//scale down to a half-texel
				mesh->lmst_array[0][v][0] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				mesh->lmst_array[0][v][1] *= (SECTTEXSIZE-1.0f)/HMLMSTRIDE;
				//bias it
				mesh->lmst_array[0][v][0] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmx) / HMLMSTRIDE);
				mesh->lmst_array[0][v][1] += ((float)SECTTEXSIZE/(HMLMSTRIDE*2)) + ((float)(s->lmy) / HMLMSTRIDE);
			}
		}

		if (!mesh->indexes)
			mesh->indexes = BZ_Malloc(sizeof(index_t) * SECTHEIGHTSIZE*SECTHEIGHTSIZE*6);

		mesh->numindexes = 0;
		for (vy = 0; vy < SECTHEIGHTSIZE-1; vy++)
		{
			for (vx = 0; vx < SECTHEIGHTSIZE-1; vx++)
			{
	#ifndef STRICTEDGES
				float d1,d2;
	#endif

	#if SECTHEIGHTSIZE == 17
				int holerow;
				int holebit;

				//skip generation of the mesh above holes
				holerow = ((vy<<3)/(SECTHEIGHTSIZE-1));
				holebit = 1u<<((vx<<3)/(SECTHEIGHTSIZE-1));
				if (s->holes[holerow] & holebit)
					continue;
	#endif
				v = vx + vy*(SECTHEIGHTSIZE);

	#ifndef STRICTEDGES
				d1 = fabs(mesh->xyz_array[v][2] - mesh->xyz_array[v+1+SECTHEIGHTSIZE][2]);
				d2 = fabs(mesh->xyz_array[v+1][2] - mesh->xyz_array[v+SECTHEIGHTSIZE][2]);
				if (d1 < d2)
				{
					mesh->indexes[mesh->numindexes++] = v+0;
					mesh->indexes[mesh->numindexes++] = v+1+SECTHEIGHTSIZE;
					mesh->indexes[mesh->numindexes++] = v+1;
					mesh->indexes[mesh->numindexes++] = v+0;
					mesh->indexes[mesh->numindexes++] = v+SECTHEIGHTSIZE;
					mesh->indexes[mesh->numindexes++] = v+1+SECTHEIGHTSIZE;
				}
				else
	#endif
				{
					mesh->indexes[mesh->numindexes++] = v+0;
					mesh->indexes[mesh->numindexes++] = v+SECTHEIGHTSIZE;
					mesh->indexes[mesh->numindexes++] = v+1;
					mesh->indexes[mesh->numindexes++] = v+1;
					mesh->indexes[mesh->numindexes++] = v+SECTHEIGHTSIZE;
					mesh->indexes[mesh->numindexes++] = v+1+SECTHEIGHTSIZE;
				}
			}
		}
		break;
	}

	//pure holes
	if (!mesh->numindexes)
	{
		memset(&s->pvscache, 0, sizeof(s->pvscache));
		return;
	}

	if (s->maxh_cull < s->maxh)
		s->maxh_cull = s->maxh;
	{
		vec3_t mins, maxs;
		mins[0] = (x-CHUNKBIAS) * hm->sectionsize;
		mins[1] = (y-CHUNKBIAS) * hm->sectionsize;
		mins[2] = s->minh;
		maxs[0] = (x+1-CHUNKBIAS) * hm->sectionsize;
		maxs[1] = (y+1-CHUNKBIAS) * hm->sectionsize;
		maxs[2] = s->maxh_cull;
		model->funcs.FindTouchedLeafs(model, &s->pvscache, mins, maxs);
	}

#ifdef GLQUAKE
	#if 0
	if (qrenderer == QR_OPENGL && qglGenBuffersARB)
	{
		vbobctx_t ctx;
		size_t vertsize = sizeof(*mesh->xyz_array)+sizeof(*mesh->st_array)+sizeof(*mesh->lmst_array)+(mesh->colors4f_array?sizeof(*mesh->colors4f_array):0);
		BE_VBO_Begin(&ctx, vertsize * mesh->numvertexes);
		BE_VBO_Data(&ctx, mesh->xyz_array, sizeof(*mesh->xyz_array) * mesh->numvertexes, &s->vbo.coord);
		BE_VBO_Data(&ctx, mesh->st_array, sizeof(*mesh->st_array) * mesh->numvertexes, &s->vbo.texcoord);
		BE_VBO_Data(&ctx, mesh->lmst_array, sizeof(*mesh->lmst_array) * mesh->numvertexes, &s->vbo.lmcoord[0]);
		if (mesh->colors4f_array)
			BE_VBO_Data(&ctx, mesh->colors4f_array, sizeof(*mesh->colors4f_array) * mesh->numvertexes, &s->vbo.colours[0]);
		BE_VBO_Finish(&ctx, mesh->indexes, sizeof(*mesh->indexes)*mesh->numindexes, &s->vbo.indicies, NULL, NULL);
	}
	#else
	if (qrenderer == QR_OPENGL && qglGenBuffersARB)
	{
		if (!s->vbo.coord.gl.vbo)
		{
			qglGenBuffersARB(1, &s->vbo.coord.gl.vbo);
			GL_SelectVBO(s->vbo.coord.gl.vbo);
		}
		else
			GL_SelectVBO(s->vbo.coord.gl.vbo);

		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, (sizeof(vecV_t)+sizeof(vec2_t)+sizeof(vec2_t)+sizeof(vec4_t)) * (mesh->numvertexes), NULL, GL_STATIC_DRAW_ARB);

		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, (sizeof(vecV_t)+sizeof(vec2_t)+sizeof(vec2_t)) * mesh->numvertexes, mesh->xyz_array);
		if (mesh->colors4f_array[0])
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (sizeof(vecV_t)+sizeof(vec2_t)+sizeof(vec2_t)) * mesh->numvertexes, sizeof(vec4_t)*mesh->numvertexes,  mesh->colors4f_array[0]);
		GL_SelectVBO(0);
		s->vbo.coord.gl.addr = 0;
		s->vbo.texcoord.gl.addr = (void*)((char*)mesh->st_array - (char*)mesh->xyz_array);
		s->vbo.texcoord.gl.vbo = s->vbo.coord.gl.vbo;
		s->vbo.lmcoord[0].gl.addr = (void*)((char*)mesh->lmst_array[0] - (char*)mesh->xyz_array);
		s->vbo.lmcoord[0].gl.vbo = s->vbo.coord.gl.vbo;
		s->vbo.colours[0].gl.addr = (void*)((sizeof(vecV_t)+sizeof(vec2_t)+sizeof(vec2_t)) * mesh->numvertexes);
		s->vbo.colours[0].gl.vbo = s->vbo.coord.gl.vbo;

		if (!s->vbo.indicies.gl.vbo)
			qglGenBuffersARB(1, &s->vbo.indicies.gl.vbo);
		s->vbo.indicies.gl.addr = 0;
		GL_SelectEBO(s->vbo.indicies.gl.vbo);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(index_t) * mesh->numindexes, mesh->indexes, GL_STATIC_DRAW_ARB);
		GL_SelectEBO(0);

#if 1
		Z_Free(mesh->xyz_array);
		mesh->xyz_array = NULL;
		mesh->st_array = NULL;
		mesh->lmst_array[0] = NULL;

		Z_Free(mesh->indexes);
		mesh->indexes = NULL;
#endif
	}
	#endif
#endif
#ifdef VKQUAKE
	if (qrenderer == QR_VULKAN)
	{
		void VKBE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch);
		batch_t batch = {0};
		mesh_t *meshes = &s->mesh;
		vbo_t *vbo = NULL;
		batch.maxmeshes = 1;
		batch.mesh = &meshes;

		VKBE_GenBatchVBOs(&vbo, &batch, NULL);
		s->vbo = *vbo;
	}
#endif
#ifdef D3D9QUAKE
	if (qrenderer == QR_DIRECT3D9)
	{
		void D3D9BE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch);
		batch_t batch = {0};
		mesh_t *meshes = &s->mesh;
		vbo_t *vbo = NULL;
		batch.maxmeshes = 1;
		batch.mesh = &meshes;

		//BE_ClearVBO(&s->vbo);
		D3D9BE_GenBatchVBOs(&vbo, &batch, NULL);
		s->vbo = *vbo;
	}
#endif
#ifdef D3D11QUAKE
	if (qrenderer == QR_DIRECT3D11)
	{
		void D3D11BE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch);
		batch_t batch = {0};
		mesh_t *meshes = &s->mesh;
		vbo_t *vbo = NULL;
		batch.maxmeshes = 1;
		batch.mesh = &meshes;

		//BE_ClearVBO(&s->vbo);
		D3D11BE_GenBatchVBOs(&vbo, &batch, NULL);
		s->vbo = *vbo;
	}
#endif
}

struct tdibctx
{
	heightmap_t *hm;
	int vx;
	int vy;
	entity_t *ent;
	batch_t **batches;
	qbyte *pvs;
	model_t *wmodel;
};
void Terr_DrawInBounds(struct tdibctx *ctx, int x, int y, int w, int h)
{
	vec3_t mins, maxs;
	hmsection_t *s;
	struct hmwater_s *wa;
	int i, j;
	batch_t *b;
	heightmap_t *hm = ctx->hm;

	mins[0] = (x+0 - CHUNKBIAS)*hm->sectionsize;
	maxs[0] = (x+w - CHUNKBIAS)*hm->sectionsize;

	mins[1] = (y+0 - CHUNKBIAS)*hm->sectionsize;
	maxs[1] = (y+h - CHUNKBIAS)*hm->sectionsize;

	mins[2] = r_origin[2]-999999;
	maxs[2] = r_origin[2]+999999;

	if (w == 1 && h == 1)
	{
//		if (R_CullBox(mins, maxs))
//			return;

		s = Terr_GetSection(hm, x, y, TGS_LAZYLOAD);
		if (!s)
			return;

		/*move to head*/
		validatelinks(&hm->recycle);
		RemoveLink(&s->recycle);
		validatelinks(&hm->recycle);
		InsertLinkBefore(&s->recycle, &hm->recycle);
		validatelinks(&hm->recycle);

		if (s->lightmap < 0)
			Terr_LoadSection(hm, s, x, y, TGS_NODOWNLOAD);

		if (s->flags & TSF_RELIGHT)
		{
			if (!hm->relight)
			{
				hm->relight = s;
				hm->relightidx = 0;
				hm->relightmin[0] = mins[0];
				hm->relightmin[1] = mins[1];
			}
		}

		if (s->flags & TSF_DIRTY)
		{
			s->flags &= ~TSF_DIRTY;

			Terr_RebuildMesh(ctx->wmodel, s, x, y);
		}

		if (ctx->pvs && !ctx->wmodel->funcs.EdictInFatPVS(ctx->wmodel, &s->pvscache, ctx->pvs, NULL))
			return;	//this section isn't in any visible bsp leafs

		if (s->numents)
		{
			Sys_LockMutex(hm->entitylock);
			//chuck out any batches for models in this section
			for (i = 0; i < s->numents; i++)
			{
				struct hmentity_s *e = s->ents[i];
				vec3_t dist;
				float a, dmin, dmax;
				model_t *model;
				//skip the entity if its already been added to some batch this frame.
				if (e->drawnframe == hm->drawnframe)
					continue;
				e->drawnframe = hm->drawnframe;

				model = e->ent.model;
				if (!model)
					continue;

				if (model->loadstate == MLS_NOTLOADED)
				{
	//				if (hm->beinglazy)
	//					continue;
	//				hm->beinglazy = true;
					Mod_LoadModel(model, MLV_WARN);
				}
				if (model->loadstate != MLS_LOADED)
					continue;

				VectorSubtract(e->ent.origin, r_origin, dist);
				a = VectorLength(dist);
				dmin = 1024 + model->radius*160;
				dmax = dmin + 1024;
				a = (a - dmin) / (dmax - dmin);
				a = 1-a;
				if (a < 0)
					continue;

				if (R_CullSphere(e->ent.origin, model->radius))
					continue;

				if (a >= 1)
				{
					a = 1;
					e->ent.flags &= ~RF_TRANSLUCENT;
				}
				else
					e->ent.flags |= RF_TRANSLUCENT;
				e->ent.shaderRGBAf[3] = a;
				switch(model->type)
				{
				case mod_alias:
					R_GAlias_GenerateBatches(&e->ent, ctx->batches);
					break;
				case mod_brush:
					Surf_GenBrushBatches(ctx->batches, &e->ent);
					break;
				default:	//FIXME: no sprites! oh noes!
					break;
				}
			}
			Sys_UnlockMutex(hm->entitylock);
		}

		for (wa = s->water; wa; wa = wa->next)
		{
			mins[2] = wa->minheight;
			maxs[2] = wa->maxheight;
			if (!R_CullBox(mins, maxs))
			{
				Terr_DrawTerrainWater(hm, mins, maxs, wa);
			}
		}

		mins[2] = s->minh;
		maxs[2] = s->maxh;

//		if (!BoundsIntersect(mins, maxs, r_refdef.vieworg, r_refdef.vieworg))
			if (R_CullBox(mins, maxs))
				return;


		if (hm->texmask)
		{
			for (i = 0; i < 4; i++)
			{
				if (!*s->texname[i])
					break;
				if (!strcmp(s->texname[i], hm->texmask))
					break;
			}
			if (i == 4)
			{	//flicker if the surface cannot accept the named texture
				int xor = (x&1)^(y&1);
				if (((int)(realtime*10) & 1) ^ xor)
					return;
			}
		}

		b = BE_GetTempBatch();
		if (!b)
			return;
		b->ent = ctx->ent;
		b->shader = hm->shader;
		b->flags = 0;
		b->mesh = &s->amesh;
		b->mesh[0] = &s->mesh;
		b->meshes = 1;
		b->buildmeshes = NULL;
		b->skin = &s->textures;
		b->texture = NULL;
		b->vbo = &s->vbo;
		b->lightmap[0] = s->lightmap;
		for (j = 1; j < MAXRLIGHTMAPS; j++)
			b->lightmap[j] = -1;

		b->next = ctx->batches[b->shader->sort];
		ctx->batches[b->shader->sort] = b;
	}
	else if (w && h)
	{
		//divide and conquer, radiating outwards from the view.
		if (w > h)
		{
			i = x + w;
			w = x + w/2;
			if (ctx->vx >= w)
			{
				Terr_DrawInBounds(ctx, w, y, i-w, h);
				Terr_DrawInBounds(ctx, x, y, w-x, h);
			}
			else
			{
				Terr_DrawInBounds(ctx, x, y, w-x, h);
				Terr_DrawInBounds(ctx, w, y, i-w, h);
			}
		}
		else
		{
			i = y + h;
			h = y + h/2;
			if (ctx->vy >= h)
			{
				Terr_DrawInBounds(ctx, x, h, w, i-h);
				Terr_DrawInBounds(ctx, x, y, w, h-y);
			}
			else
			{
				Terr_DrawInBounds(ctx, x, y, w, h-y);
				Terr_DrawInBounds(ctx, x, h, w, i-h);
			}
		}
	}
}

void Terr_DrawTerrainModel (batch_t **batches, entity_t *e)
{
	model_t *m = e->model;
	heightmap_t *hm = m->terrain;
	batch_t *b;
	int bounds[4], j;
	struct tdibctx tdibctx;

	if (!r_refdef.recurse)
	{
		Terr_DoEditNotify(hm);
//		while (hm->activesections > 0)
//			if (!Terr_Collect(hm))
//				break;
		while (hm->activesections > TERRAINACTIVESECTIONS)
		{
			if (!Terr_Collect(hm))
				break;
			break;
		}
	}
	
//	hm->beinglazy = false;
	if (hm->relight)
		ted_dorelight(m, hm);

	if (e->model == cl.worldmodel && hm->skyshader)
	{
		b = BE_GetTempBatch();
		if (b)
		{
			for (j = 0; j < MAXRLIGHTMAPS; j++)
				b->lightmap[j] = -1;
			b->ent = e;
			b->shader = hm->skyshader;
			b->flags = 0;
			b->mesh = &hm->askymesh;
			b->mesh[0] = &hm->skymesh;
			b->meshes = 1;
			b->buildmeshes = NULL;
			b->skin = NULL;
			b->texture = NULL;
	//		vbo = b->vbo = hm->vbo[x+y*MAXSECTIONS];
			b->vbo = NULL;

			b->next = batches[b->shader->sort];
			batches[b->shader->sort] = b;
		}
	}

	Terr_Brush_Draw(hm, batches, e);

	if ((r_refdef.globalfog.density&&r_refdef.globalfog.alpha>=1) || r_refdef.maxdist>0)
	{
		float culldist;
		extern cvar_t r_fog_exp2;

		if (r_refdef.globalfog.density&&r_refdef.globalfog.alpha>=1)
		{	//fogalpha<1 means you can always see through it, so don't cull when its invisible.
			//figure out the eyespace distance required to reach that fog value
			culldist = log(0.5/255.0f);
			if (r_fog_exp2.ival)
				culldist = sqrt(culldist / (-r_refdef.globalfog.density * r_refdef.globalfog.density));
			else
				culldist = culldist / (-r_refdef.globalfog.density);
			//anything drawn beyond this point is fully obscured by fog
			culldist += 4096;
		}
		else
			culldist = 999999999999999.f;

		if (culldist < hm->maxdrawdist)
			culldist = hm->maxdrawdist;
		if (culldist > r_refdef.maxdist && r_refdef.maxdist>0)
			culldist = r_refdef.maxdist;

		bounds[0] = bound(hm->firstsegx, (r_refdef.vieworg[0] + (CHUNKBIAS + 0)*hm->sectionsize - culldist) / hm->sectionsize,  hm->maxsegx);
		bounds[1] = bound(hm->firstsegx, (r_refdef.vieworg[0] + (CHUNKBIAS + 1)*hm->sectionsize + culldist) / hm->sectionsize,  hm->maxsegx);
		bounds[2] = bound(hm->firstsegy, (r_refdef.vieworg[1] + (CHUNKBIAS + 0)*hm->sectionsize - culldist) / hm->sectionsize,  hm->maxsegy);
		bounds[3] = bound(hm->firstsegy, (r_refdef.vieworg[1] + (CHUNKBIAS + 1)*hm->sectionsize + culldist) / hm->sectionsize,  hm->maxsegy);
	}
	else
	{
		bounds[0] = hm->firstsegx;
		bounds[1] = hm->maxsegx;
		bounds[2] = hm->firstsegy;
		bounds[3] = hm->maxsegy;
	}
	//FIXME: project the near+far clip planes onto the screen, generate bounds from those, instead of the above overkill code.

	hm->drawnframe+=1;
	tdibctx.hm = hm;
	tdibctx.batches = batches;
	tdibctx.ent = e;
	tdibctx.vx = (r_refdef.vieworg[0] + CHUNKBIAS*hm->sectionsize) / hm->sectionsize;
	tdibctx.vy = (r_refdef.vieworg[1] + CHUNKBIAS*hm->sectionsize) / hm->sectionsize;
	tdibctx.wmodel = e->model;
	tdibctx.pvs = (e->model == cl.worldmodel)?r_refdef.scenevis:NULL;
validatelinks(&hm->recycle);
	Terr_DrawInBounds(&tdibctx, bounds[0], bounds[2], bounds[1]-bounds[0], bounds[3]-bounds[2]);

validatelinks(&hm->recycle);
	/*{
	trace_t trace;
	vec3_t player_mins = {-16, -16, -24};
	vec3_t player_maxs = {16, 16, 32};
	vec3_t start, end;
	VectorCopy(cl.playerview[0].simorg, start);
	VectorCopy(start, end);
	start[0] += 5;
	end[2] -= 100;
	Heightmap_Trace(cl.worldmodel, 0, 0, NULL, start, end, player_mins, player_maxs, false, ~0, &trace);
	}*/
}

void Terrain_ClipDecal(fragmentdecal_t *dec, float *center, float radius, model_t *model)
{
	int min[2], max[2], mint[2], maxt[2];
	int x, y, tx, ty;
	vecV_t vert[6];
	hmsection_t *s;
	heightmap_t *hm = model->terrain; 
	min[0] = floor((center[0] - radius)/(hm->sectionsize)) + CHUNKBIAS;
	min[1] = floor((center[1] - radius)/(hm->sectionsize)) + CHUNKBIAS;
	max[0] = ceil((center[0] + radius)/(hm->sectionsize)) + CHUNKBIAS;
	max[1] = ceil((center[1] + radius)/(hm->sectionsize)) + CHUNKBIAS;

	min[0] = bound(hm->firstsegx, min[0], hm->maxsegx);
	min[1] = bound(hm->firstsegy, min[1], hm->maxsegy);
	max[0] = bound(hm->firstsegx, max[0], hm->maxsegx);
	max[1] = bound(hm->firstsegy, max[1], hm->maxsegy);

	for (y = min[1]; y < max[1]; y++)
	{
		for (x = min[0]; x < max[0]; x++)
		{
			s = Terr_GetSection(hm, x, y, TGS_WAITLOAD);
			if (!s)
				continue;

			mint[0] = floor((center[0] - radius)*(SECTHEIGHTSIZE-1)/(hm->sectionsize) + (CHUNKBIAS - x)*(SECTHEIGHTSIZE-1));
			mint[1] = floor((center[1] - radius)*(SECTHEIGHTSIZE-1)/(hm->sectionsize) + (CHUNKBIAS - y)*(SECTHEIGHTSIZE-1));
			maxt[0] =  ceil((center[0] + radius)*(SECTHEIGHTSIZE-1)/(hm->sectionsize) + (CHUNKBIAS - x)*(SECTHEIGHTSIZE-1));
			maxt[1] =  ceil((center[1] + radius)*(SECTHEIGHTSIZE-1)/(hm->sectionsize) + (CHUNKBIAS - y)*(SECTHEIGHTSIZE-1));

			mint[0] = bound(0, mint[0], (SECTHEIGHTSIZE-1));
			mint[1] = bound(0, mint[1], (SECTHEIGHTSIZE-1));
			maxt[0] = bound(0, maxt[0], (SECTHEIGHTSIZE-1));
			maxt[1] = bound(0, maxt[1], (SECTHEIGHTSIZE-1));

			for (ty = mint[1]; ty < maxt[1]; ty++)
			{
				for (tx = mint[0]; tx < maxt[0]; tx++)
				{
#ifndef STRICTEDGES
					float d1, d2;
					d1 = fabs(s->heights[(tx+0) + (ty+0)*SECTHEIGHTSIZE] - s->heights[(tx+1) + (ty+1)*SECTHEIGHTSIZE]);
					d2 = fabs(s->heights[(tx+1) + (ty+0)*SECTHEIGHTSIZE] - s->heights[(tx+0) + (ty+1)*SECTHEIGHTSIZE]);
					if (d1 < d2)
					{
						vert[0][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[0][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[1][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[1][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[2][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[2][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;

						vert[3][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[3][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[4][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[4][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[5][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[5][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;

						vert[0][2] = s->heights[(tx+0) + (ty+0)*SECTHEIGHTSIZE];
						vert[1][2] = s->heights[(tx+1) + (ty+1)*SECTHEIGHTSIZE];
						vert[2][2] = s->heights[(tx+1) + (ty+0)*SECTHEIGHTSIZE];
						vert[3][2] = s->heights[(tx+0) + (ty+0)*SECTHEIGHTSIZE];
						vert[4][2] = s->heights[(tx+0) + (ty+1)*SECTHEIGHTSIZE];
						vert[5][2] = s->heights[(tx+1) + (ty+1)*SECTHEIGHTSIZE];
					}
					else
#endif
					{
						vert[0][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[0][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[1][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[1][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[2][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[2][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;

						vert[3][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[3][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[4][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+0)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[4][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;
						vert[5][0] = (x-CHUNKBIAS)*hm->sectionsize + (tx+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;vert[5][1] = (y-CHUNKBIAS)*hm->sectionsize + (ty+1)/(float)(SECTHEIGHTSIZE-1)*hm->sectionsize;

						vert[0][2] = s->heights[(tx+0) + (ty+0)*SECTHEIGHTSIZE];
						vert[1][2] = s->heights[(tx+0) + (ty+1)*SECTHEIGHTSIZE];
						vert[2][2] = s->heights[(tx+1) + (ty+0)*SECTHEIGHTSIZE];
						vert[3][2] = s->heights[(tx+1) + (ty+0)*SECTHEIGHTSIZE];
						vert[4][2] = s->heights[(tx+0) + (ty+1)*SECTHEIGHTSIZE];
						vert[5][2] = s->heights[(tx+1) + (ty+1)*SECTHEIGHTSIZE];
					}

					//fixme: per-section shaders for clutter info. this kinda sucks.
					Fragment_ClipPoly(dec, 3, &vert[0][0], hm->shader);
					Fragment_ClipPoly(dec, 3, &vert[3][0], hm->shader);
				}
			}
		}
	}
}

#endif

unsigned int Heightmap_PointContentsHM(heightmap_t *hm, float clipmipsz, const vec3_t org)
{
	float x, y;
	float z, tz;
	int sx, sy;
	unsigned int holerow;
	unsigned int holebit;
	hmsection_t *s;
	struct hmwater_s *w;
	unsigned int contents;
	const float wbias = CHUNKBIAS * hm->sectionsize;

	sx = (org[0]+wbias)/hm->sectionsize;
	sy = (org[1]+wbias)/hm->sectionsize;
	if (sx < hm->firstsegx || sy < hm->firstsegy)
		return hm->exteriorcontents;
	if (sx >= hm->maxsegx || sy >= hm->maxsegy)
		return hm->exteriorcontents;
	s = Terr_GetSection(hm, sx, sy, TGS_TRYLOAD | TGS_ANYSTATE);
	if (!s || s->loadstate != TSLS_LOADED)
	{
		if (s && s->loadstate == TSLS_FAILED)
			return hm->exteriorcontents;
		return FTECONTENTS_SOLID;
	}

	x = (org[0]+wbias - (sx*hm->sectionsize))*(SECTHEIGHTSIZE-1)/hm->sectionsize;
	y = (org[1]+wbias - (sy*hm->sectionsize))*(SECTHEIGHTSIZE-1)/hm->sectionsize;
	z = (org[2]+clipmipsz);

	if (z < s->minh-16)
		return hm->exteriorcontents;

	sx = x; x-=sx;
	sy = y; y-=sy;

	holerow = ((sy<<3)/(SECTHEIGHTSIZE-1));
	holebit = 1u<<((sx<<3)/(SECTHEIGHTSIZE-1));
	if (s->holes[holerow] & (1u<<holebit))
		return FTECONTENTS_EMPTY;

	//made of two triangles:
	if (x+y>1)	//the 1, 1 triangle
	{
		float v1, v2, v3;
		v3 = 1-y;
		v2 = x+y-1;
		v1 = 1-x;
		//0, 1
		//1, 1
		//1, 0
		tz = (s->heights[(sx+0)+(sy+1)*SECTHEIGHTSIZE]*v1 +
			  s->heights[(sx+1)+(sy+1)*SECTHEIGHTSIZE]*v2 +
			  s->heights[(sx+1)+(sy+0)*SECTHEIGHTSIZE]*v3);
	}
	else
	{
		float v1, v2, v3;
		v1 = y;
		v2 = x;
		v3 = 1-y-x;

		//0, 1
		//1, 0
		//0, 0
		tz = (s->heights[(sx+0)+(sy+1)*SECTHEIGHTSIZE]*v1 +
			  s->heights[(sx+1)+(sy+0)*SECTHEIGHTSIZE]*v2 +
			  s->heights[(sx+0)+(sy+0)*SECTHEIGHTSIZE]*v3);
	}
	if (z <= tz)
		return FTECONTENTS_SOLID;	//contained within

	contents = FTECONTENTS_EMPTY;
	for (w = s->water; w; w = w->next)
	{
		if (w->holes[holerow] & (1u<<holebit))
			continue;
		if (z < w->maxheight)	//FIXME
			contents |= w->contentmask;
	}
	return contents;
}

unsigned int Heightmap_PointContents(model_t *model, const vec3_t axis[3], const vec3_t org)
{
	heightmap_t *hm = model->terrain;
	unsigned int cont;
	brushes_t *br;
	unsigned int i, j;
	float dist;

	cont = Heightmap_PointContentsHM(hm, 0, org);
	if (cont & FTECONTENTS_SOLID)
		return cont;


	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->patch)
			continue;	//infinitely thin...

		for (j = 0; j < br->numplanes; j++)
		{
			/*
			for (k=0 ; k<3 ; k++)
			{
				if (in_normals[j][k] < 0)
					best[k] = br->maxs[k];
				else
					best[k] = br->mins[k];
			}
			*/
			dist = DotProduct (org/*best*/, br->planes[j]);
			dist = br->planes[j][3] - dist;
			if (dist < 0)
				break;
		}
		if (j == br->numplanes)
		{
			cont |= br->contents;
		}
	}

	return cont;
}
unsigned int Heightmap_NativeBoxContents(model_t *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t org, const vec3_t mins, const vec3_t maxs)
{
	heightmap_t *hm = model->terrain;
	return Heightmap_PointContentsHM(hm, mins[2], org);
}

float Heightmap_Normal(heightmap_t *hm, vec2_t org, vec3_t norm)	//returns the z
{
#if 0
	float z = 0;
	norm[0] = 0;
	norm[1] = 0;
	norm[2] = 1;
#else
	float x, y;
	int sx, sy;
	vec3_t d1, d2;
	const float wbias = CHUNKBIAS * hm->sectionsize;
	hmsection_t *s;
	float z;

	norm[0] = 0;
	norm[1] = 0;
	norm[2] = 1;

	sx = (org[0]+wbias)/hm->sectionsize;
	sy = (org[1]+wbias)/hm->sectionsize;
	if (sx < hm->firstsegx || sy < hm->firstsegy)
		return hm->defaultgroundheight;
	if (sx >= hm->maxsegx || sy >= hm->maxsegy)
		return hm->defaultgroundheight;
	s = Terr_GetSection(hm, sx, sy, TGS_TRYLOAD);
	if (!s)
		return hm->defaultgroundheight;

	x = (org[0]+wbias - (sx*hm->sectionsize))*(SECTHEIGHTSIZE-1)/hm->sectionsize;
	y = (org[1]+wbias - (sy*hm->sectionsize))*(SECTHEIGHTSIZE-1)/hm->sectionsize;

	sx = x; x-=sx;
	sy = y; y-=sy;

	if (x+y>1)	//the 1, 1 triangle
	{
		//0, 1
		//1, 1
		//1, 0
		d1[0] = (hm->sectionsize / SECTHEIGHTSIZE);
		d1[1] = 0;
		d1[2] = (s->heights[(sx+1)+(sy+1)*SECTHEIGHTSIZE] - s->heights[(sx+0)+(sy+1)*SECTHEIGHTSIZE]);
		d2[0] = 0;
		d2[1] = (hm->sectionsize / SECTHEIGHTSIZE);
		d2[2] = (s->heights[(sx+1)+(sy+1)*SECTHEIGHTSIZE] - s->heights[(sx+1)+(sy+0)*SECTHEIGHTSIZE]);

		z = (s->heights[(sx+0)+(sy+1)*SECTHEIGHTSIZE]*(1-y) +
			 s->heights[(sx+1)+(sy+1)*SECTHEIGHTSIZE]*(x+y-1) +
			 s->heights[(sx+1)+(sy+0)*SECTHEIGHTSIZE]*(1-x));
	}
	else
	{	//the 0,0 triangle
		//0, 1
		//1, 0
		//0, 0
		d1[0] = (hm->sectionsize / SECTHEIGHTSIZE);
		d1[1] = 0;
		d1[2] = (s->heights[(sx+1)+(sy+0)*SECTHEIGHTSIZE] - s->heights[(sx+0)+(sy+0)*SECTHEIGHTSIZE]);
		d2[0] = 0;
		d2[1] = (hm->sectionsize / SECTHEIGHTSIZE);
		d2[2] = (s->heights[(sx+0)+(sy+1)*SECTHEIGHTSIZE] - s->heights[(sx+0)+(sy+0)*SECTHEIGHTSIZE]);

		z = (s->heights[(sx+0)+(sy+1)*SECTHEIGHTSIZE]*(y) +
			 s->heights[(sx+1)+(sy+0)*SECTHEIGHTSIZE]*(x) +
			 s->heights[(sx+0)+(sy+0)*SECTHEIGHTSIZE]*(1-y-x));
	}


	VectorNormalize(d1);
	VectorNormalize(d2);
	CrossProduct(d1, d2, norm);
	VectorNormalize(norm);
#endif
	return z;
}

typedef struct {
	vec3_t start;
	vec3_t end;
	vec3_t impact;
	vec4_t plane;
	vec3_t mins;
	vec3_t maxs;
	vec3_t absmins;
	vec3_t absmaxs;
	vec3_t up;
	vec3_t capsulesize;
	enum {ispoint, iscapsule, isbox} shape;
	qboolean startsolid;
	double nearfrac;
	float truefrac;
	float htilesize;
	heightmap_t *hm;
	int contents;
	int hitcontentsmask;
	trace_t *result;

#ifdef _DEBUG
	qboolean debug;
#endif
} hmtrace_t;

#ifdef HAVE_CLIENT
shader_t *Terr_GetShader(model_t *mod, trace_t *trace)
{

	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	brushid			= trace->brush_id;
	unsigned int	fa, i;
	brushes_t		*br;

	if (!brushid)
		return NULL;

	if (!hm)
		return NULL;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == trace->brush_id)
		{
			if (br->patch)
				return br->patch->tex->shader;
			fa = trace->brush_face-1;
			if (fa >= br->numplanes)
				return NULL;
			return br->faces[fa].tex->shader;
		}
	}
	return NULL;
}
#endif

static int Heightmap_Trace_Brush(hmtrace_t *tr, vec4_t *planes, int numplanes, brushes_t *brushinfo)
{
	qboolean startout;
	float *enterplane;
	double enterfrac, exitfrac, nearfrac=0;
	double enterdist=0;
	double dist, d1, d2, f;
	unsigned int i, j;
	vec3_t ofs;

	startout = false;
	enterplane= NULL;
	enterfrac = -1;
	exitfrac = 10;
	for (i = 0; i < numplanes; i++)
	{
		/*calculate the distance based upon the shape of the object we're tracing for*/
		switch(tr->shape)
		{
		default:
		case isbox: // general box case
			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (planes[i][j] < 0)
					ofs[j] = tr->maxs[j];
				else
					ofs[j] = tr->mins[j];
			}
			dist = DotProduct (ofs, planes[i]);
			dist = planes[i][3] - dist;
			break;
		case iscapsule:
			dist = DotProduct(tr->up, planes[i]);
			dist = dist*(tr->capsulesize[(dist<0)?1:2]) - tr->capsulesize[0];
			dist = planes[i][3] - dist;
			break;
		case ispoint: // special point case
			dist = planes[i][3];
			break;
		}


		d1 = DotProduct (tr->start, planes[i]) - dist;
		d2 = DotProduct (tr->end, planes[i]) - dist;

		//if we're fully outside any plane, then we cannot possibly enter the brush, skip to the next one
		if (d1 > 0 && d2 >= d1)
			return false;

		if (d1 > 0)
			startout = true;

		//if we're fully inside the plane, then whatever is happening is not relevent for this plane
		if (d1 < 0 && d2 <= 0)
			continue;

		f = (d1) / (d1-d2);
		if (d1 > d2)
		{
			//entered the brush. favour the furthest fraction to avoid extended edges (yay for convex shapes)
			if (enterfrac < f)
			{
				enterfrac = f;
				nearfrac = (d1 - (0.03125)) / (d1-d2);
				enterplane = planes[i];
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

	//non-point traces need to clip against the brush's edges
	if (brushinfo && tr->shape != ispoint && brushinfo->axialplanes != 0x3f)
	{
		static vec3_t axis[] = {{1,0,0},{0,1,0},{0,0,1},{-1,0,0},{0,-1,0},{0,0,-1}};
		for (i = 0; i < 6; i++)
		{
//			if (brushinfo->axialplanes & (1u<<i))
//				continue;	//should have already checked this plane.
			if (i >= 3)
			{
				/*calculate the distance based upon the shape of the object we're tracing for*/
				switch(tr->shape)
				{
				default:
				case isbox:
					dist = -tr->maxs[i-3];
					dist = -brushinfo->mins[i-3] - dist;
					break;
				case iscapsule:
					dist = -tr->up[i-3];
					dist = dist*(tr->capsulesize[(dist<0)?1:2]) - tr->capsulesize[0];
					dist = -brushinfo->mins[i-3] - dist;
					break;
				case ispoint:
					dist = -brushinfo->mins[i-3];
					break;
				}
				d1 = -tr->start[i-3] - dist;
				d2 = -tr->end[i-3] - dist;
			}
			else
			{
				switch(tr->shape)
				{
				default:
				case isbox:
					dist = brushinfo->maxs[i] - tr->mins[i];
					break;
				case iscapsule:
					dist = tr->up[i];
					dist = dist*(tr->capsulesize[(dist<0)?1:2]) - tr->capsulesize[0];
					dist = brushinfo->maxs[i] - dist;
					break;
				case ispoint:
					dist = brushinfo->maxs[i];
					break;
				}
				d1 = (tr->start[i]) - dist;
				d2 = (tr->end[i]) - dist;
			}

			//if we're fully outside any plane, then we cannot possibly enter the brush, skip to the next one
			if (d1 > 0 && d2 >= d1)
				return false;

			if (d1 > 0)
				startout = true;

			//if we're fully inside the plane, then whatever is happening is not relevent for this plane
			if (d1 <= 0 && d2 <= 0)
				continue;

			f = (d1) / (d1-d2);
			if (d1 > d2)
			{
				//entered the brush. favour the furthest fraction to avoid extended edges (yay for convex shapes)
				if (enterfrac < f)
				{
					enterfrac = f;
					nearfrac = (d1 - (0.03125)) / (d1-d2);
					enterplane = axis[i];
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
	}

	if (!startout)
	{

#if 0//def _DEBUG
	if (tr->debug)
	{
		vecV_t			facepoints[256];
		unsigned int	numpoints;

		for (i = 0; i < numplanes; i++)
		{
			scenetris_t *t;
			extern shader_t *shader_draw_fill;
			//generate points now (so we know the correct mins+maxs for the brush, and whether the plane is relevent)
			numpoints = Terr_GenerateBrushFace(facepoints, countof(facepoints), planes, numplanes, planes[i]);


			if (cl_numstrisvert+numpoints > cl_maxstrisvert)
				break;
			if (cl_numstrisidx+(numpoints-2)*3 > cl_maxstrisidx)
				break;

			if (cl_numstris == cl_maxstris)
			{
				cl_maxstris+=8;
				cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
			}
			t = &cl_stris[cl_numstris++];
			t->shader = shader_draw_fill;
			t->flags = 0;
			t->firstidx = cl_numstrisidx;
			t->firstvert = cl_numstrisvert;
			for (j = 2; j < numpoints; j++)
			{
				cl_strisidx[cl_numstrisidx++] = 0;
				cl_strisidx[cl_numstrisidx++] = j-1;
				cl_strisidx[cl_numstrisidx++] = j;
			}
			for (j = 0; j < numpoints; j++)
			{
				VectorCopy(facepoints[j], cl_strisvertv[cl_numstrisvert]);
				cl_strisvertv[cl_numstrisvert][2] += 1;
				Vector4Set(cl_strisvertc[cl_numstrisvert], 1, 0, 0, 0.2);
				Vector2Set(cl_strisvertt[cl_numstrisvert], 0, 0);
				cl_numstrisvert++;
			}
			t->numidx = cl_numstrisidx - t->firstidx;
			t->numvert = cl_numstrisvert-t->firstvert;
		}
	}
#endif


		tr->startsolid = true;
		return false;
	}
	if (enterfrac != -1 && enterfrac < exitfrac)
	{
		//impact!
		if (enterfrac < tr->truefrac)
		{
			if (nearfrac < 0)
				nearfrac = 0;
			tr->nearfrac = nearfrac;
			tr->truefrac = enterfrac;
			tr->plane[3] = enterdist;
			VectorCopy(enterplane, tr->plane);


#if 0//def _DEBUG
	if (tr->debug)
	{
		vecV_t			facepoints[256];
		unsigned int	numpoints;

		for (i = 0; i < numplanes; i++)
		{
			scenetris_t *t;
			extern shader_t *shader_draw_fill;
			//generate points now (so we know the correct mins+maxs for the brush, and whether the plane is relevent)
			numpoints = Terr_GenerateBrushFace(facepoints, countof(facepoints), planes, numplanes, planes[i]);


			if (cl_numstrisvert+numpoints > cl_maxstrisvert)
				break;
			if (cl_numstrisidx+(numpoints-2)*3 > cl_maxstrisidx)
				break;

			if (cl_numstris == cl_maxstris)
			{
				cl_maxstris+=8;
				cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
			}
			t = &cl_stris[cl_numstris++];
			t->shader = shader_draw_fill;
			t->flags = 0;
			t->firstidx = cl_numstrisidx;
			t->firstvert = cl_numstrisvert;
			for (j = 2; j < numpoints; j++)
			{
				cl_strisidx[cl_numstrisidx++] = 0;
				cl_strisidx[cl_numstrisidx++] = j-1;
				cl_strisidx[cl_numstrisidx++] = j;
			}
			for (j = 0; j < numpoints; j++)
			{
				VectorCopy(facepoints[j], cl_strisvertv[cl_numstrisvert]);
				cl_strisvertv[cl_numstrisvert][2] += 1;
				Vector4Set(cl_strisvertc[cl_numstrisvert], 0, 1, 0, 0.2);
				Vector2Set(cl_strisvertt[cl_numstrisvert], 0, 0);
				cl_numstrisvert++;
			}
			t->numidx = cl_numstrisidx - t->firstidx;
			t->numvert = cl_numstrisvert-t->firstvert;
		}
	}
#endif


			return ((vec4_t*)enterplane - planes)+1;
		}
	}
	return false;
}

static qboolean Heightmap_Trace_Quad(hmtrace_t *tr, const float *v0, const float *v1, const float *v2, const float *v3)
{
	//super lame shite. be lazy and just use a bbox
	static vec4_t n[6] = {
		{-1, 0, 0, 0},
		{ 0,-1, 0, 0},
		{ 0, 0,-1, 0},
		{ 1, 0, 0, 0},
		{ 0, 1, 0, 0},
		{ 0, 0, 1, 0},
	};
	vec3_t d[2];
	const float epsilon = 1.0/64;
	VectorCopy(v0, d[0]);
	VectorCopy(v0, d[1]);
	AddPointToBounds(v1, d[0], d[1]);
	AddPointToBounds(v2, d[0], d[1]);
	AddPointToBounds(v3, d[0], d[1]);

//I'm implementing this primarily for selecting patches.
//decals are often infinitely thin things.
//so expand them by a tiny amount in the hopes that traces will hit patches before the wall they're coplanar with.
	n[0][3] =-d[0][0]+epsilon;
	n[1][3] =-d[0][1]+epsilon;
	n[2][3] =-d[0][2]+epsilon;
	n[3][3] = d[1][0]+epsilon;
	n[4][3] = d[1][1]+epsilon;
	n[5][3] = d[1][2]+epsilon;

	return Heightmap_Trace_Brush(tr, n, 6, NULL) != 0;
}
static qboolean Heightmap_Trace_Patch(hmtrace_t *tr, brushes_t *brushinfo)
{
	const struct patchdata_s *patch = brushinfo->patch;
	unsigned int w, h, x, y;
	qboolean ret = false;

	if (!patch->tessvert)
	{
		const struct qcpatchvert_s *r1 = patch->cp, *r2;
		w = patch->numcp[0];
		h = patch->numcp[1];

		for (y = 0, r2 = r1 + w; y < h-1; y++)
		{
			for (x = 0; x < w-1; x++, r1++, r2++)
				ret |= Heightmap_Trace_Quad(tr, r1[0].v, r1[1].v, r2[0].v, r1[1].v);
			r1++; r2++;
		}
	}
	else
	{
		const struct patchtessvert_s *r1 = patch->tessvert, *r2;
		w = patch->tesssize[0];
		h = patch->tesssize[1];

		for (y = 0, r2 = r1 + w; y < h-1; y++)
		{
			for (x = 0; x < w-1; x++, r1++, r2++)
				ret |= Heightmap_Trace_Quad(tr, r1[0].v, r1[1].v, r2[0].v, r1[1].v);
			r1++; r2++;
		}
	}
	return ret;
}

//sx,sy are the tile coord
//note that tile SECTHEIGHTSIZE-1 does not exist, as the last sample overlaps the first sample of the next section
static void Heightmap_Trace_Square(hmtrace_t *tr, int tx, int ty)
{
	vec3_t d[2];
	vec3_t p[4];
	vec4_t n[6];
	int i;

#ifndef STRICTEDGES
	float d1, d2;
#endif
	int sx, sy;
	hmsection_t *s;
	unsigned int holerow;
	unsigned int holebit;

	sx = tx/(SECTHEIGHTSIZE-1);
	sy = ty/(SECTHEIGHTSIZE-1);
	if (sx < tr->hm->firstsegx || sx >= tr->hm->maxsegx ||
		sy < tr->hm->firstsegy || sy >= tr->hm->maxsegy)
		s = NULL;
	else
		s = Terr_GetSection(tr->hm, sx, sy, TGS_TRYLOAD|TGS_WAITLOAD|TGS_ANYSTATE);

	if (!s || s->loadstate != TSLS_LOADED)
	{
		if ((tr->hitcontentsmask & tr->hm->exteriorcontents) || (s && s->loadstate != TSLS_FAILED))
		{
			//you're not allowed to walk into sections that have not loaded.
			//might as well check the entire section instead of just one tile
			Vector4Set(n[0],  1, 0, 0, (tx/(SECTHEIGHTSIZE-1) + 1 - CHUNKBIAS)*tr->hm->sectionsize);
			Vector4Set(n[1], -1, 0, 0, -(tx/(SECTHEIGHTSIZE-1) + 0 - CHUNKBIAS)*tr->hm->sectionsize);
			Vector4Set(n[2], 0,  1, 0, (ty/(SECTHEIGHTSIZE-1) + 1 - CHUNKBIAS)*tr->hm->sectionsize);
			Vector4Set(n[3], 0, -1, 0, -(ty/(SECTHEIGHTSIZE-1) + 0 - CHUNKBIAS)*tr->hm->sectionsize);
			Heightmap_Trace_Brush(tr, n, 4, NULL);
		}
		return;
	}

	if (s->traceseq != tr->hm->traceseq && s->numents)
	{
		s->traceseq = tr->hm->traceseq;
		Sys_LockMutex(tr->hm->entitylock);
		for (i = 0; i < s->numents; i++)
		{
			vec3_t start_l, end_l;
			trace_t etr;
			model_t *model;
			if (s->ents[i]->traceseq == tr->hm->traceseq)
				continue;
			s->ents[i]->traceseq = tr->hm->traceseq;
			model = s->ents[i]->ent.model;
			//FIXME: IGNORE the entity if it isn't loaded yet? surely that's bad?
			if (!model || model->loadstate != MLS_LOADED || !model->funcs.NativeTrace)
				continue;
			//figure out where on the submodel the trace is.
			VectorSubtract (tr->start, s->ents[i]->ent.origin, start_l);
			VectorSubtract (tr->end, s->ents[i]->ent.origin, end_l);
//			start_l[2] -= tr->mins[2];
//			end_l[2] -= tr->mins[2];
			VectorScale(start_l, s->ents[i]->ent.scale, start_l);
			VectorScale(end_l, s->ents[i]->ent.scale, end_l);

			//skip if the local trace points are outside the model's bounds
/*			for (j = 0; j < 3; j++)
			{
				if (start_l[j]+tr->mins[j] > model->maxs[j] && end_l[j]+tr->mins[j] > model->maxs[j])
					continue;
				if (start_l[j]+tr->maxs[j] < model->mins[j] && end_l[j]+tr->maxs[j] < model->mins[j])
					continue;
			}
*/
			//do the trace
			memset(&etr, 0, sizeof(etr));
			etr.fraction = 1;
			model->funcs.NativeTrace (model, 0, &s->ents[i]->ent.framestate, s->ents[i]->ent.axis, start_l, end_l, tr->mins, tr->maxs, tr->shape == iscapsule, tr->hitcontentsmask, &etr);

			if (etr.startsolid)
			{	//many many bsp objects are not enclosed 'properly' (qbsp strips any surfaces outside the world).
				//this means that such bsps extend to infinity, resulting in sudden glitchy stuck issues when you enter a section containing such a bsp
				//so if we started solid, constrain that solidity to the volume of the submodel
				VectorCopy  (s->ents[i]->ent.axis[0], n[0]);
				VectorNegate(s->ents[i]->ent.axis[0], n[1]);
				VectorCopy  (s->ents[i]->ent.axis[1], n[2]);
				VectorNegate(s->ents[i]->ent.axis[1], n[3]);
				VectorCopy  (s->ents[i]->ent.axis[2], n[4]);
				VectorNegate(s->ents[i]->ent.axis[2], n[5]);
				n[0][3] = DotProduct(n[0], s->ents[i]->ent.origin) + model->maxs[0];
				n[1][3] = DotProduct(n[1], s->ents[i]->ent.origin) + -model->mins[0];
				n[2][3] = DotProduct(n[2], s->ents[i]->ent.origin) + model->maxs[1];
				n[3][3] = DotProduct(n[3], s->ents[i]->ent.origin) + -model->mins[1];
				n[4][3] = DotProduct(n[4], s->ents[i]->ent.origin) + model->maxs[2];
				n[5][3] = DotProduct(n[5], s->ents[i]->ent.origin) + -model->mins[2];
				Heightmap_Trace_Brush(tr, n, 6, NULL);
			}
			else
			{
				tr->result->startsolid |= etr.startsolid;
				tr->result->allsolid |= etr.allsolid;
				if (etr.fraction < tr->nearfrac)
				{
					tr->contents = etr.contents;
					tr->truefrac = etr.truefraction;
					tr->nearfrac = etr.fraction;
					tr->plane[3] = etr.plane.dist;
					tr->plane[0] = etr.plane.normal[0];
					tr->plane[1] = etr.plane.normal[1];
					tr->plane[2] = etr.plane.normal[2];
				}
			}
		}
		Sys_UnlockMutex(tr->hm->entitylock);
	}

	sx = tx - CHUNKBIAS*(SECTHEIGHTSIZE-1);
	sy = ty - CHUNKBIAS*(SECTHEIGHTSIZE-1);

	tx = tx % (SECTHEIGHTSIZE-1);
	ty = ty % (SECTHEIGHTSIZE-1);

	holerow = ((ty<<3)/(SECTHEIGHTSIZE-1));
	holebit = 1u<<((tx<<3)/(SECTHEIGHTSIZE-1));
	if (s->holes[holerow] & holebit)
		return;	//no collision with holes

	switch(tr->hm->mode)
	{
	case HMM_BLOCKS:
		//left-most
		Vector4Set(n[0], -1, 0, 0, -tr->htilesize*(sx+0));
		//bottom-most
		Vector4Set(n[1], 0, 1, 0, tr->htilesize*(sy+1));
		//right-most
		Vector4Set(n[2], 1, 0, 0, tr->htilesize*(sx+1));
		//top-most
		Vector4Set(n[3], 0, -1, 0, -tr->htilesize*(sy+0));
		//top
		Vector4Set(n[4], 0, 0, 1, s->heights[(tx+0)+(ty+0)*SECTHEIGHTSIZE]);

		Heightmap_Trace_Brush(tr, n, 5, NULL);
		return;
	case HMM_TERRAIN:
		VectorSet(p[0], tr->htilesize*(sx+0), tr->htilesize*(sy+0), s->heights[(tx+0)+(ty+0)*SECTHEIGHTSIZE]);
		VectorSet(p[1], tr->htilesize*(sx+1), tr->htilesize*(sy+0), s->heights[(tx+1)+(ty+0)*SECTHEIGHTSIZE]);
		VectorSet(p[2], tr->htilesize*(sx+0), tr->htilesize*(sy+1), s->heights[(tx+0)+(ty+1)*SECTHEIGHTSIZE]);
		VectorSet(p[3], tr->htilesize*(sx+1), tr->htilesize*(sy+1), s->heights[(tx+1)+(ty+1)*SECTHEIGHTSIZE]);

		VectorSet(n[5], 0, 0, 1);
#ifndef STRICTEDGES
		d1 = fabs(p[0][2] - p[3][2]);
		d2 = fabs(p[1][2] - p[2][2]);
		if (d1 < d2)
		{
			/*generate the brush (in world space*/
			{
				VectorSubtract(p[3], p[0], d[0]);
				VectorSubtract(p[2], p[0], d[1]);
				//left-most
				Vector4Set(n[0], -1, 0, 0, -tr->htilesize*(sx+0));
				//bottom-most
				Vector4Set(n[1], 0, 1, 0, tr->htilesize*(sy+1));
				//top-right
				VectorSet(n[2], 0.70710678118654752440084436210485, -0.70710678118654752440084436210485, 0);
				n[2][3] = DotProduct(n[2], p[0]);
				//top
				VectorNormalize(d[0]);
				VectorNormalize(d[1]);
				CrossProduct(d[0], d[1], n[3]);
				VectorNormalize(n[3]);
				n[3][3] = DotProduct(n[3], p[0]);
				//down
				VectorNegate(n[3], n[4]);
				n[4][3] = DotProduct(n[4], p[0]) - n[4][2]*TERRAINTHICKNESS;

				n[5][3] = max(p[0][2], p[2][2]);
				n[5][3] = max(n[5][3], p[3][2]);
				Heightmap_Trace_Brush(tr, n, 6, NULL);
			}

			{
				VectorSubtract(p[3], p[0], d[0]);
				VectorSubtract(p[3], p[1], d[1]);

				//right-most
				Vector4Set(n[0], 1, 0, 0, tr->htilesize*(sx+1));
				//top-most
				Vector4Set(n[1], 0, -1, 0, -tr->htilesize*(sy+0));
				//bottom-left
				VectorSet(n[2], -0.70710678118654752440084436210485, 0.70710678118654752440084436210485, 0);
				n[2][3] = DotProduct(n[2], p[0]);
				//top
				VectorNormalize(d[0]);
				VectorNormalize(d[1]);
				CrossProduct(d[0], d[1], n[3]);
				VectorNormalize(n[3]);
				n[3][3] = DotProduct(n[3], p[0]);
				//down
				VectorNegate(n[3], n[4]);
				n[4][3] = DotProduct(n[4], p[0]) - n[4][2]*TERRAINTHICKNESS;

				n[5][3] = max(p[0][2], p[1][2]);
				n[5][3] = max(n[5][3], p[3][2]);
				Heightmap_Trace_Brush(tr, n, 6, NULL);
			}
		}
		else
#endif
		{
			/*generate the brush (in world space*/
			{
				VectorSubtract(p[1], p[0], d[0]);
				VectorSubtract(p[2], p[0], d[1]);
				//left-most
				Vector4Set(n[0], -1, 0, 0, -tr->htilesize*(sx+0));
				//top-most
				Vector4Set(n[1], 0, -1, 0, -tr->htilesize*(sy+0));
				//bottom-right
				VectorSet(n[2], 0.70710678118654752440084436210485, 0.70710678118654752440084436210485, 0);
				n[2][3] = DotProduct(n[2], p[1]);
				//top
				VectorNormalize(d[0]);
				VectorNormalize(d[1]);
				CrossProduct(d[0], d[1], n[3]);
				VectorNormalize(n[3]);
				n[3][3] = DotProduct(n[3], p[1]);
				//down
				VectorNegate(n[3], n[4]);
				n[4][3] = DotProduct(n[4], p[1]) - n[4][2]*TERRAINTHICKNESS;

				n[5][3] = max(p[0][2], p[1][2]);
				n[5][3] = max(n[5][3], p[2][2]);
				Heightmap_Trace_Brush(tr, n, 6, NULL);
			}
			{
				VectorSubtract(p[3], p[2], d[0]);
				VectorSubtract(p[3], p[1], d[1]);

				//right-most
				Vector4Set(n[0], 1, 0, 0, tr->htilesize*(sx+1));
				//bottom-most
				Vector4Set(n[1], 0, 1, 0, tr->htilesize*(sy+1));
				//top-left
				VectorSet(n[2], -0.70710678118654752440084436210485, -0.70710678118654752440084436210485, 0);
				n[2][3] = DotProduct(n[2], p[1]);
				//top
				VectorNormalize(d[0]);
				VectorNormalize(d[1]);
				CrossProduct(d[0], d[1], n[3]);
				VectorNormalize(n[3]);
				n[3][3] = DotProduct(n[3], p[1]);
				//down
				VectorNegate(n[3], n[4]);
				n[4][3] = DotProduct(n[4], p[1]) - n[4][2]*TERRAINTHICKNESS;

				n[5][3] = max(p[1][2], p[2][2]);
				n[5][3] = max(n[5][3], p[3][2]);
				Heightmap_Trace_Brush(tr, n, 6, NULL);
			}
		}
		break;
	}
}

#define DIST_EPSILON 0
/*
Heightmap_TraceRecurse
Traces an arbitary box through a heightmap. (interface with outside)

Why is recursion good?
1: it is consistant with bsp models. :)
2: it allows us to use any size model we want
3: we don't have to work out the height of the terrain every X units, but can be more precise.

Obviously, we don't care all that much about 1
*/
qboolean Heightmap_Trace(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t mataxis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int against, struct trace_s *trace)
{
	vec2_t pos;
	vec2_t frac;
	vec2_t emins;
	vec2_t emaxs;
	vec3_t tmp;
	int ipos[2], npos[2];
	int x, y, e;
	int axis;
	int breaklimit = 1000;
	float zbias;
	hmtrace_t hmtrace;
	hmtrace.hm = model->terrain;
	hmtrace.hm->traceseq++;
	hmtrace.htilesize = hmtrace.hm->sectionsize / (SECTHEIGHTSIZE-1);
	hmtrace.nearfrac = hmtrace.truefrac = 1;
	hmtrace.contents = 0;
	hmtrace.hitcontentsmask = against;

	hmtrace.plane[0] = 0;
	hmtrace.plane[1] = 0;
	hmtrace.plane[2] = 0;
	hmtrace.plane[3] = 0;
	if (capsule)
	{
		hmtrace.shape = iscapsule;
		zbias = 0;

		if (mataxis)
			VectorSet(hmtrace.up, mataxis[0][2], -mataxis[1][2], mataxis[2][2]);
		else
			VectorSet(hmtrace.up, 0, 0, 1);

		//determine the capsule sizes
		hmtrace.capsulesize[0] = ((maxs[0]-mins[0]) + (maxs[1]-mins[1]))/4.0;
		hmtrace.capsulesize[1] = maxs[2];
		hmtrace.capsulesize[2] = mins[2];
//		zbias = (trace_capsulesize[1] > -hmtrace.capsulesize[2])?hmtrace.capsulesize[1]:-hmtrace.capsulesize[2];
		hmtrace.capsulesize[1] -= hmtrace.capsulesize[0];
		hmtrace.capsulesize[2] += hmtrace.capsulesize[0];

		zbias = 0;
	}
	else if (mins[0] || mins[1] || mins[2] || maxs[0] || maxs[1] || maxs[2])
	{
		hmtrace.shape = isbox;
		zbias = 0;
	}
	else
	{
		hmtrace.shape = ispoint;
		zbias = mins[2];
	}

	memset(trace, 0, sizeof(*trace));
	hmtrace.result = trace;
	hmtrace.startsolid = false;

	//to tile space
	hmtrace.start[0] = (start[0]);
	hmtrace.start[1] = (start[1]);
	hmtrace.start[2] = (start[2] + zbias);
	hmtrace.end[0] = (end[0]);
	hmtrace.end[1] = (end[1]);
	hmtrace.end[2] = (end[2] + zbias);

//	mins = vec3_origin;
//	maxs = vec3_origin;

	VectorCopy(mins, hmtrace.mins);
	VectorCopy(maxs, hmtrace.maxs);

	//determine extents
	VectorAdd(hmtrace.start, hmtrace.mins, hmtrace.absmins);
	VectorCopy(hmtrace.absmins, hmtrace.absmaxs);
	VectorAdd(hmtrace.start, hmtrace.maxs, tmp);
	AddPointToBounds (tmp, hmtrace.absmins, hmtrace.absmaxs);
	VectorAdd(hmtrace.end, hmtrace.mins, tmp);
	AddPointToBounds (tmp, hmtrace.absmins, hmtrace.absmaxs);
	VectorAdd(hmtrace.end, hmtrace.maxs, tmp);
	AddPointToBounds (tmp, hmtrace.absmins, hmtrace.absmaxs);
	hmtrace.absmaxs[0] += 1;
	hmtrace.absmaxs[1] += 1;
	hmtrace.absmaxs[2] += 1;
	hmtrace.absmins[0] -= 1;
	hmtrace.absmins[1] -= 1;
	hmtrace.absmins[2] -= 1;

	//figure out where we are in terms of tiles
	pos[0] = (hmtrace.start[0]+CHUNKBIAS*hmtrace.hm->sectionsize)/hmtrace.htilesize;
	pos[1] = (hmtrace.start[1]+CHUNKBIAS*hmtrace.hm->sectionsize)/hmtrace.htilesize;

	emins[0] = (mins[0]-1.5)/hmtrace.htilesize;
	emins[1] = (mins[1]-1.5)/hmtrace.htilesize;
	emaxs[0] = (maxs[0]+1.5)/hmtrace.htilesize;
	emaxs[1] = (maxs[1]+1.5)/hmtrace.htilesize;

	//Test code
	if (0)
	{
		vec2_t minb, maxb;
		Vector2Copy(pos, minb);
		Vector2Copy(pos, maxb);
		npos[0] = (hmtrace.end[0]+CHUNKBIAS*hmtrace.hm->sectionsize)/hmtrace.htilesize;
		npos[1] = (hmtrace.end[1]+CHUNKBIAS*hmtrace.hm->sectionsize)/hmtrace.htilesize;
		if (npos[0] > pos[0])
			maxb[0] = pos[0];
		else
			minb[0] = pos[0];
		if (npos[1] > pos[1])
			maxb[1] = pos[1];
		else
			minb[1] = pos[1];

		minb[0] += emins[0];
		minb[1] += emins[1];
		maxb[0] += emaxs[0];
		maxb[1] += emaxs[1];

		for (y = floor(minb[1]); y <= ceil(maxb[1]); y++)
			for (x = floor(minb[0]); x <= ceil(maxb[0]); x++)
				Heightmap_Trace_Square(&hmtrace, x, y);
	}

	//trace against the heightmap, if it exists.
	if (hmtrace.hm->maxsegx != hmtrace.hm->firstsegx)
	{
		//make sure the start tile is valid
		for (y = floor(pos[1] + emins[1]); y <= ceil(pos[1] + emaxs[1]); y++)
			for (x = floor(pos[0] + emins[0]); x <= ceil(pos[0] + emaxs[0]); x++)
				Heightmap_Trace_Square(&hmtrace, x, y);

		//now walk over the terrain
		if (hmtrace.end[0] != hmtrace.start[0] || hmtrace.end[1] != hmtrace.start[1])
		{
			vec2_t dir, trstart, trdist;

			//figure out the leading point
			for (axis = 0; axis < 2; axis++)
			{
				trdist[axis] = hmtrace.end[axis]-hmtrace.start[axis];
				dir[axis] = (hmtrace.end[axis] - hmtrace.start[axis])/hmtrace.htilesize;

				if (dir[axis] > 0)
				{
					ipos[axis] = pos[axis] + emins[axis];
					trstart[axis] = CHUNKBIAS*hmtrace.hm->sectionsize + (maxs[axis]) + hmtrace.start[axis];
				}
				else
				{
					ipos[axis] = pos[axis] + emaxs[axis];
					trstart[axis] = CHUNKBIAS*hmtrace.hm->sectionsize + (mins[axis]) + hmtrace.start[axis];
				}
				trstart[axis] /= hmtrace.htilesize;
				trdist[axis] /= hmtrace.htilesize;
			}
			for(;;)
			{
				if (breaklimit--< 0)
					break;
				for (axis = 0; axis < 2; axis++)
				{
					if (dir[axis] > 0)
					{
						npos[axis] = ipos[axis]+1;
						frac[axis] = (npos[axis]-trstart[axis])/trdist[axis];
					}
					else if (dir[axis] < 0)
					{
						npos[axis] = ipos[axis];
						frac[axis] = (ipos[axis]-trstart[axis])/trdist[axis];
					}
					else
						frac[axis] = 1000000000000000.0;
				}

				//which side are we going down?
				if (frac[0] < frac[1])
					axis = 0;
				else
					axis = 1;

				if (frac[axis] >= 1)
					break;

				//progress to the crossed boundary
				if (dir[axis] < 0)
					ipos[axis] = ipos[axis]-1;
				else
					ipos[axis] = ipos[axis]+1;

				axis = !axis;
				if (dir[axis] > 0)
				{	//leading edge is on the right, so start on the left and keep going until we hit the leading edge
					npos[0] = ipos[0];
					npos[1] = ipos[1];

					npos[axis] -= ceil(emins[axis]-emaxs[axis]);
					e = ipos[axis];

					npos[axis] -= 1;
					e++;

					for (; npos[axis] <= e; npos[axis]++)
						Heightmap_Trace_Square(&hmtrace, npos[0], npos[1]);
				}
				else
				{
					//leading edge is on the left
					npos[0] = ipos[0];
					npos[1] = ipos[1];
					e = ipos[axis] + ceil(emaxs[axis]-emins[axis]);

					npos[axis] -= 1;
					e++;

					for (; npos[axis] <= e; npos[axis]++)
						Heightmap_Trace_Square(&hmtrace, npos[0], npos[1]);
				}

//				axis = !axis;
				//and make sure our position on the other axis is correct, for the next time around the loop
//				if (frac[axis] > hmtrace.truefrac)
//					break;
			}
		}
	}

	//now trace against the brushes.
	//FIXME: optimise into the section grid
	{
		brushes_t *brushes = hmtrace.hm->wbrushes;
		int count = hmtrace.hm->numbrushes;
		for (count = hmtrace.hm->numbrushes; count-->0; brushes++)
		{
			if (brushes->contents & against)
			{
				int face;
				if (hmtrace.absmaxs[0] < brushes->mins[0] ||
					hmtrace.absmaxs[1] < brushes->mins[1] ||
					hmtrace.absmaxs[2] < brushes->mins[2])
					continue;
				if (hmtrace.absmins[0] > brushes->maxs[0] ||
					hmtrace.absmins[1] > brushes->maxs[1] ||
					hmtrace.absmins[2] > brushes->maxs[2])
					continue;
				if (brushes->patch)
				{
					if (Heightmap_Trace_Patch(&hmtrace, brushes))
						face = -1;
					else
						face = 0;
				}
				else
					face = Heightmap_Trace_Brush(&hmtrace, brushes->planes, brushes->numplanes, brushes);
				if (face)
				{
					trace->brush_id = brushes->id;
					trace->brush_face = face;
				}
			}
		}
	}

	trace->plane.dist = hmtrace.plane[3];
	trace->plane.normal[0] = hmtrace.plane[0];
	trace->plane.normal[1] = hmtrace.plane[1];
	trace->plane.normal[2] = hmtrace.plane[2];

	trace->startsolid = trace->allsolid = hmtrace.startsolid;
	if (hmtrace.nearfrac < 0)
		hmtrace.nearfrac = 0;
	trace->fraction = hmtrace.nearfrac;
	trace->truefraction = hmtrace.truefrac;
	VectorInterpolate(start, hmtrace.nearfrac, end, trace->endpos);
	return trace->fraction < 1;
}

qboolean Heightmap_Trace_Test(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t mataxis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int against, struct trace_s *trace)
{
	qboolean ret = Heightmap_Trace(model, hulloverride, framestate, mataxis, start, end, mins, maxs, capsule, against, trace);
	
	if (!trace->startsolid)
	{	//FIXME: this code should not be needed.
		trace_t testtrace;
		Heightmap_Trace(model, hulloverride, framestate, mataxis, trace->endpos, trace->endpos, mins, maxs, capsule, against, &testtrace);
		if (testtrace.startsolid)
		{	//yup, we're bugged.
			Con_DPrintf("Trace became solid\n");
			trace->fraction = 0;
			VectorCopy(start, trace->endpos);
			trace->startsolid = trace->allsolid = true;
		}
	}

	return ret;
}

typedef struct
{
	int id;
	int pos[3];
} hmpvs_t;
typedef struct
{
	int id;
	int min[3], max[3];
} hmpvsent_t;
unsigned int Heightmap_FatPVS		(model_t *mod, const vec3_t org, pvsbuffer_t *fte_restrict pvsbuffer, qboolean add)
{
	//embed the org onto the pvs
	hmpvs_t *hmpvs;
	if (pvsbuffer->buffersize < sizeof(*hmpvs))
		pvsbuffer->buffer = BZ_Realloc(pvsbuffer->buffer, pvsbuffer->buffersize=sizeof(*hmpvs));
	hmpvs = (hmpvs_t*)pvsbuffer->buffer;
	hmpvs->id = 0xdeadbeef;
	VectorCopy(org, hmpvs->pos);
	return sizeof(*hmpvs);
}

#ifdef HAVE_SERVER
qboolean Heightmap_EdictInFatPVS	(model_t *mod, const struct pvscache_s *edict, const qbyte *pvsdata, const int *areas)
{
	heightmap_t *hm = mod->terrain;
	int o[3], i;
	const hmpvs_t *hmpvs = (const hmpvs_t*)pvsdata;
	const hmpvsent_t *hmed = (const hmpvsent_t*)edict;

	if (!hm->culldistance || !hmpvs)
		return true;

	//check distance
	for (i = 0; i < 3; i++)
	{
		if (hmpvs->pos[i] < hmed->min[i])
			o[i] = hmed->min[i] - hmpvs->pos[i];
		else if (hmpvs->pos[i] > hmed->max[i])
			o[i] = hmed->max[i] - hmpvs->pos[i];
		else
			o[i] = 0;
	}

	return DotProduct(o,o) < hm->culldistance;
}

void Heightmap_FindTouchedLeafs	(model_t *mod, pvscache_t *ent, const float *mins, const float *maxs)
{
	hmpvsent_t *hmed = (hmpvsent_t*)ent;

	VectorCopy(mins, hmed->min);
	VectorCopy(maxs, hmed->max);
}
#endif

void Heightmap_LightPointValues	(model_t *mod, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	res_diffuse[0] = 128;
	res_diffuse[1] = 128;
	res_diffuse[2] = 128;
	res_ambient[0] = 64;
	res_ambient[1] = 64;
	res_ambient[2] = 64;
	res_dir[0] = 1;//sin(time);
	res_dir[1] = 0;//cos(time);
	res_dir[2] = 0;//sin(time);
	VectorNormalize(res_dir);
}
void Heightmap_StainNode			(model_t *mod, float *parms)
{
}
void Heightmap_MarkLights			(dlight_t *light, dlightbitmask_t bit, mnode_t *node)
{
}

qbyte *Heightmap_ClusterPVS	(model_t *model, int num, pvsbuffer_t *buffer, pvsmerge_t merge)
{
	return NULL;
//	static qbyte heightmappvs = 255;
//	return &heightmappvs;
}
int	Heightmap_ClusterForPoint	(model_t *model, const vec3_t point, int *area)
{
	if (area)
		*area = 0;
	return -1;
}

#ifdef HAVE_CLIENT
static unsigned char *QDECL Terr_GetLightmap(hmsection_t *s, int idx, qboolean edit)
{
	int x = idx % SECTTEXSIZE, y = idx / SECTTEXSIZE;
	if (s->lightmap < 0)
	{
		Terr_LoadSection(s->hmmod, s, s->sx, s->sy, true);
		Terr_InitLightmap(s, true);
	}
	if (s->lightmap < 0)
		return NULL;

	if (edit)
	{
		s->flags |= TSF_EDITED;

		lightmap[s->lightmap]->modified = true;
		lightmap[s->lightmap]->rectchange.l = 0;
		lightmap[s->lightmap]->rectchange.t = 0;
		lightmap[s->lightmap]->rectchange.r = HMLMSTRIDE;
		lightmap[s->lightmap]->rectchange.b = HMLMSTRIDE;
	}
	return lightmap[s->lightmap]->lightmaps + ((s->lmy+y) * HMLMSTRIDE + (s->lmx+x)) * lightmap[s->lightmap]->pixbytes;
}
static void ted_dorelight(model_t *m, heightmap_t *hm)
{
	unsigned char *lm = Terr_GetLightmap(hm->relight, 0, true);
	int x, y, k;
#define EXPAND 2
	vec3_t surfnorms[(SECTTEXSIZE+EXPAND*2)*(SECTTEXSIZE+EXPAND*2)];
	vec3_t surfpoint[(SECTTEXSIZE+EXPAND*2)*(SECTTEXSIZE+EXPAND*2)];
//	float scaletab[EXPAND*2*EXPAND*2];
	vec3_t ldir;
	hmsection_t *s = hm->relight;
	float ambient, diffuse;
	trace_t trace;
	s->flags &= ~TSF_RELIGHT;
	hm->relight = NULL;

	if (s->lightmap < 0)
		return;

	ambient = 255*mod_terrain_ambient.value;
	diffuse = 255-ambient;

	for (y = -EXPAND; y < SECTTEXSIZE+EXPAND; y++)
	for (x = -EXPAND; x < SECTTEXSIZE+EXPAND; x++)
	{
		k = x+EXPAND + (y+EXPAND)*(SECTTEXSIZE+EXPAND*2);
		surfpoint[k][0] = hm->relightmin[0] + (x*hm->sectionsize/(SECTTEXSIZE-1));
		surfpoint[k][1] = hm->relightmin[1] + (y*hm->sectionsize/(SECTTEXSIZE-1));
		surfpoint[k][2] = Heightmap_Normal(s->hmmod, surfpoint[k], surfnorms[k])+0.1;
	}

	VectorNormalize2(mod_terrain_sundir.vec4, ldir);

	for (y = 0; y < SECTTEXSIZE; y++, lm += (HMLMSTRIDE-SECTTEXSIZE)*4)
	for (x = 0; x < SECTTEXSIZE; x++, lm += 4)
	{
		vec3_t norm;
		float d;
		int sx,sy;
		VectorClear(norm);
		for (sy = -EXPAND; sy <= EXPAND; sy++)
		for (sx = -EXPAND; sx <= EXPAND; sx++)
		{
			d = sqrt((EXPAND*2+1)*(EXPAND*2+1) - sx*sx+sy*sy);
			VectorMA(norm, d, surfnorms[x+sx+EXPAND + (y+sy+EXPAND)*(SECTTEXSIZE+EXPAND*2)], norm);
		}

		VectorNormalize(norm);
		d = DotProduct(ldir, norm);
		if (d < 0)
			d = 0;
		else if (mod_terrain_shadows.ival)
		{
			float *point = surfpoint[x+EXPAND + (y+EXPAND)*(SECTTEXSIZE+EXPAND*2)];
			vec3_t sun;
			VectorMA(point, mod_terrain_shadow_dist.value, ldir, sun);
			if (m->funcs.NativeTrace(m, 0, NULL, NULL, point, sun, vec3_origin, vec3_origin, false, FTECONTENTS_SOLID|FTECONTENTS_BODY, &trace))
				d = 0;
		}
//		lm[0] = norm[0]*127 + 128;
//		lm[1] = norm[1]*127 + 128;
//		lm[2] = norm[2]*127 + 128;
		lm[3] = ambient + d*diffuse;
	}

	lightmap[s->lightmap]->modified = true;
	lightmap[s->lightmap]->rectchange.l = 0;
	lightmap[s->lightmap]->rectchange.t = 0;
	lightmap[s->lightmap]->rectchange.r = HMLMSTRIDE;
	lightmap[s->lightmap]->rectchange.b = HMLMSTRIDE;
}
static void ted_sethole(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned int row = idx/9;
	unsigned int col = idx%9;
	unsigned int bit;
	unsigned int mask;
	if (row == 8 || col == 8)
		return;	//meh, our painting function is written with an overlap of 1
	if (w <= 0)
		return;
	mask = 1u<<(col);
	if (*(float*)ctx > 0)
		bit = mask;
	else
		bit = 0;
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED;
	s->holes[row] = (s->holes[row] & ~mask) | bit;
}
static void ted_heighttally(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	/*raise the terrain*/
	((float*)ctx)[0] += s->heights[idx]*w;
	((float*)ctx)[1] += w;
}
static void ted_heightsmooth(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED|TSF_RELIGHT;
	/*interpolate the terrain towards a certain value*/

	if (IS_NAN(s->heights[idx]))
		s->heights[idx] = *(float*)ctx;
	else
		s->heights[idx] = s->heights[idx]*(1-w) + w**(float*)ctx;
}
static void ted_heightdebug(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	int tx = idx/SECTHEIGHTSIZE, ty = idx % SECTHEIGHTSIZE;
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED|TSF_RELIGHT;
	/*interpolate the terrain towards a certain value*/

	if (tx == 16)
		tx = 0;
	if (ty == 16)
		ty = 0;

//	if (ty < tx)
//		tx = ty;
	s->heights[idx] = (tx>>1) * 32 + (ty>>1) * 32;
}
static void ted_heightraise(void *ctx, hmsection_t *s, int idx, float wx, float wy, float strength)
{
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED|TSF_RELIGHT;
	/*raise the terrain*/
	s->heights[idx] += strength;
}
static void ted_heightset(void *ctx, hmsection_t *s, int idx, float wx, float wy, float strength)
{
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED|TSF_RELIGHT;
	/*set the terrain to a specific value*/
	s->heights[idx] = *(float*)ctx;
}

static void ted_waterset(void *ctx, hmsection_t *s, int idx, float wx, float wy, float strength)
{
	struct hmwater_s *w = s->water;
	if (!w)
		w = Terr_GenerateWater(s, *(float*)ctx);
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED;

	//FIXME: water doesn't render properly. don't let people make dodgy water regions because they can't see it.
	//this is temp code.
	//for (idx = 0; idx < 9*9; idx++)
		//w->heights[idx] = *(float*)ctx;
	//end fixme

	w->heights[idx] = *(float*)ctx;
	if (w->minheight > w->heights[idx])
		w->minheight = w->heights[idx];
	if (w->maxheight < w->heights[idx])
		w->maxheight = w->heights[idx];

	//FIXME: what about holes?
}

static void ted_texconcentrate(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned char *lm = Terr_GetLightmap(s, idx, true);
	s->flags |= TSF_NOTIFY|TSF_EDITED;

	/*concentrate the lightmap values to a single channel*/
	if (lm[0] > lm[1] && lm[0] > lm[2] && lm[0] > (255-(lm[0]+lm[1]+lm[2])))
	{
		lm[0] = lm[0]*(1-w) + 255*(w);
		lm[1] = lm[1]*(1-w) + 0*(w);
		lm[2] = lm[2]*(1-w) + 0*(w);
	}
	else if (lm[1] > lm[2] && lm[1] > (255-(lm[0]+lm[1]+lm[2])))
	{
		lm[0] = lm[0]*(1-w) + 0*(w);
		lm[1] = lm[1]*(1-w) + 255*(w);
		lm[2] = lm[2]*(1-w) + 0*(w);
	}
	else if (lm[2] > (255-(lm[0]+lm[1]+lm[2])))
	{
		lm[0] = lm[0]*(1-w) + 0*(w);
		lm[1] = lm[1]*(1-w) + 0*(w);
		lm[2] = lm[2]*(1-w) + 255*(w);
	}
	else
	{
		lm[0] = lm[0]*(1-w) + 0*(w);
		lm[1] = lm[1]*(1-w) + 0*(w);
		lm[2] = lm[2]*(1-w) + 0*(w);
	}
}

static void ted_texnoise(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned char *lm = Terr_GetLightmap(s, idx, true);
	vec4_t v;
	float sc;

	s->flags |= TSF_NOTIFY|TSF_EDITED;

	/*randomize the lightmap somewhat (you'll probably want to concentrate it a bit after)*/
	v[0] = (rand()&255);
	v[1] = (rand()&255);
	v[2] = (rand()&255);
	v[3] = (rand()&255);
	sc = v[0] + v[1] + v[2] + v[3];
	Vector4Scale(v, 255/sc, v);

	lm[0] = lm[0]*(1-w) + (v[0]*(w));
	lm[1] = lm[1]*(1-w) + (v[1]*(w));
	lm[2] = lm[2]*(1-w) + (v[2]*(w));
}

static void ted_texpaint(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned char *lm = Terr_GetLightmap(s, idx, true);
	const char *texname = ctx;
	int t;
	vec4_t newval;
	if (w > 1)
		w = 1;

	s->flags |= TSF_NOTIFY|TSF_EDITED;

	for (t = 0; t < 4; t++)
	{
		if (!strncmp(s->texname[t], texname, sizeof(s->texname[t])-1))
		{
			int extra;
			newval[0] = (t == 0);
			newval[1] = (t == 1);
			newval[2] = (t == 2);
			newval[3] = (t == 3);
			extra = 255 - (lm[0]+lm[1]+lm[2]);
			lm[2] = lm[2]*(1-w) + (255*newval[0]*(w));
			lm[1] = lm[1]*(1-w) + (255*newval[1]*(w));
			lm[0] = lm[0]*(1-w) + (255*newval[2]*(w));
			extra = extra*(1-w) + (255*newval[3]*(w));

			//the extra stuff is to cope with numerical precision. add any lost values to the new texture instead of the implicit one
			extra = 255 - (extra+lm[0]+lm[1]+lm[2]);
			if (t != 3)
				lm[2-t] += extra;
			return;
		}
	}

	/*special handling to make a section accept the first texture painted on it as a base texture. no more chessboard*/
	if (!*s->texname[0] && !*s->texname[1] && !*s->texname[2] && !*s->texname[3])
	{
		Q_strncpyz(s->texname[3], texname, sizeof(s->texname[3]));
		Terr_LoadSectionTextures(s);

		for (idx = 0; idx < SECTTEXSIZE*SECTTEXSIZE; idx++)
		{
			lm = Terr_GetLightmap(s, idx, true);
			lm[2] = 0;
			lm[1] = 0;
			lm[0] = 0;
		}
		return;
	}
	for (t = 0; t < 4; t++)
	{
		if (!*s->texname[t])
		{
			Q_strncpyz(s->texname[t], texname, sizeof(s->texname[t]));

			newval[0] = (t == 0);
			newval[1] = (t == 1);
			newval[2] = (t == 2);
			lm[2] = lm[2]*(1-w) + (255*newval[0]*(w));
			lm[1] = lm[1]*(1-w) + (255*newval[1]*(w));
			lm[0] = lm[0]*(1-w) + (255*newval[2]*(w));

			Terr_LoadSectionTextures(s);
			return;
		}
	}
}

static void ted_texreplace(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	if (w > 0)
		ted_texpaint(ctx, s, idx, wx, wy, 1);
}

/*
static void ted_texlight(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned char *lm = ted_getlightmap(s, idx);
	vec3_t pos, pos2;
	vec3_t norm, tnorm;
	vec3_t ldir = {0.4, 0.7, 2};
	float d;
	int x,y;
	trace_t tr;
	VectorClear(norm);
	for (y = -4; y < 4; y++)
	for (x = -4; x < 4; x++)
	{
		pos[0] = wx - (CHUNKBIAS + x/64.0) * s->hmmod->sectionsize;
		pos[1] = wy - (CHUNKBIAS + y/64.0) * s->hmmod->sectionsize;
#if 0
		pos[2] = 10000;
		pos2[0] = wx - (CHUNKBIAS + x/64.0) * s->hmmod->sectionsize;
		pos2[1] = wy - (CHUNKBIAS + y/64.0) * s->hmmod->sectionsize;
		pos2[2] = -10000;
		Heightmap_Trace(cl.worldmodel, 0, 0, NULL, pos, pos2, vec3_origin, vec3_origin, FTECONTENTS_SOLID, &tr);
		VectorCopy(tr.plane.normal, tnorm);
#else
		Heightmap_Normal(s->hmmod, pos, tnorm);
#endif
		d = sqrt(32 - x*x+y*y);
		VectorMA(norm, d, tnorm, norm);
	}

	VectorNormalize(ldir);
	VectorNormalize(norm);
	d = DotProduct(ldir, norm);
	if (d < 0)
		d = 0;
	lm[3] = d*255;
}
*/
static void ted_texset(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned char *lm = Terr_GetLightmap(s, idx, true);
	if (w > 1)
		w = 1;
	s->flags |= TSF_NOTIFY|TSF_EDITED;

	lm[2] = lm[2]*(1-w) + (255*((float*)ctx)[0]*(w));
	lm[1] = lm[1]*(1-w) + (255*((float*)ctx)[1]*(w));
	lm[0] = lm[0]*(1-w) + (255*((float*)ctx)[2]*(w));
}

static void ted_textally(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	unsigned char *lm = Terr_GetLightmap(s, idx, false);
	((float*)ctx)[0] += lm[0]*w;
	((float*)ctx)[1] += lm[1]*w;
	((float*)ctx)[2] += lm[2]*w;
	((float*)ctx)[3] += w;
}

static void ted_tint(void *ctx, hmsection_t *s, int idx, float wx, float wy, float w)
{
	float *col = s->colours[idx];
	float *newval = ctx;
	if (w > 1)
		w = 1;
	s->flags |= TSF_NOTIFY|TSF_DIRTY|TSF_EDITED|TSF_HASCOLOURS;	/*dirty because of the vbo*/
	col[0] = col[0]*(1-w) + (newval[0]*(w));
	col[1] = col[1]*(1-w) + (newval[1]*(w));
	col[2] = col[2]*(1-w) + (newval[2]*(w));
	col[3] = col[3]*(1-w) + (newval[3]*(w));
}

enum
{
	tid_linear,
	tid_exponential,
	tid_square_linear,
	tid_square_exponential,
	tid_flat
};
//calls 'func' for each tile upon the terrain. the 'tile' can be either height or texel
static void ted_itterate(heightmap_t *hm, int distribution, float *pos, float radius, float strength, int steps, void(*func)(void *ctx, hmsection_t *s, int idx, float wx, float wy, float strength), void *ctx)
{
	int tx, ty;
	float wx, wy;
	float sc[2];
	int min[2], max[2];
	int sx,sy;
	hmsection_t *s;
	float w, xd, yd;

	if (radius < 0)
	{
		radius *= -1;
		distribution |= 2;
	}

	min[0] = floor((pos[0] - radius)/(hm->sectionsize) - 1.5);
	min[1] = floor((pos[1] - radius)/(hm->sectionsize) - 1.5);
	max[0] = ceil((pos[0] + radius)/(hm->sectionsize) + 1.5);
	max[1] = ceil((pos[1] + radius)/(hm->sectionsize) + 1.5);

	min[0] = bound(hm->firstsegx, min[0], hm->maxsegx);
	min[1] = bound(hm->firstsegy, min[1], hm->maxsegy);
	max[0] = bound(hm->firstsegx, max[0], hm->maxsegx);
	max[1] = bound(hm->firstsegy, max[1], hm->maxsegy);

	sc[0] = hm->sectionsize/(steps-1);
	sc[1] = hm->sectionsize/(steps-1);

	for (sy = min[1]; sy < max[1]; sy++)
	{
		for (sx = min[0]; sx < max[0]; sx++)
		{
			s = Terr_GetSection(hm, sx, sy, TGS_WAITLOAD|TGS_DEFAULTONFAIL);
			if (!s)
				continue;

			for (ty = 0; ty < steps; ty++)
			{
				wy = (sy*(steps-1.0) + ty)*sc[1];
				yd = wy - pos[1];// - sc[1]/4;
				for (tx = 0; tx < steps; tx++)
				{
					/*both heights and textures have an overlapping/matching sample at the edge, there's no need for any half-pixels or anything here*/
					wx = (sx*(steps-1.0) + tx)*sc[0];
					xd = wx - pos[0];// - sc[0]/4;

					switch(distribution)
					{
					case tid_exponential:
						w = radius*radius - (xd*xd+yd*yd);
						if (w > 0)
							func(ctx, s, tx+ty*steps, wx, wy, sqrt(w)*strength/(radius));
						break;
					case tid_linear:
						w = radius - sqrt(xd*xd+yd*yd);
						if (w > 0)
							func(ctx, s, tx+ty*steps, wx, wy, w*strength/(radius));
						break;
					case tid_square_exponential:
						w = max(fabs(xd), fabs(yd));
						w = radius*radius - w*w;
						if (w > 0)
							func(ctx, s, tx+ty*steps, wx, wy, sqrt(w)*strength/(radius));
						break;
					case tid_square_linear:
						w = max(fabs(xd), fabs(yd));
						w = radius - w;
						if (w > 0)
							func(ctx, s, tx+ty*steps, wx, wy, w*strength/(radius));
						break;
					case tid_flat:
						w = max(fabs(xd), fabs(yd));
						w = radius - w;
						if (w > 0)
							func(ctx, s, tx+ty*steps, wx, wy, strength);
						break;
					}
				}
			}
		}
	}
}

void ted_texkill(hmsection_t *s, const char *killtex)
{
	int x, y, t, to;
	if (!s)
		return;
	for (t = 0; t < 4; t++)
	{
		if (!strcmp(s->texname[t], killtex))
		{
			unsigned char *lm = Terr_GetLightmap(s, 0, true);
			s->flags |= TSF_EDITED;
			s->texname[t][0] = 0;
			for (to = 0; to < 4; to++)
				if (*s->texname[to])
					break;
			if (to == 4)
				to = 0;

			if (to == 0 || to == 2)
				to = 2 - to;
			if (t == 0 || t == 2)
				t = 2 - t;

			for (y = 0; y < SECTTEXSIZE; y++)
			{
				for (x = 0; x < SECTTEXSIZE; x++, lm+=4)
				{
					if (t == 3)
					{
						//to won't be 3
						lm[to] = lm[to] + (255 - (lm[0] + lm[1] + lm[2]));
					}
					else
					{
						if (to != 3)
							lm[to] = (lm[to]+lm[t])&0xff;
						lm[t] = 0;
					}
				}
				lm += SECTTEXSIZE*4*(LMCHUNKS-1);
			}
			if (t == 0 || t == 2)
				t = 2 - t;
			Terr_LoadSectionTextures(s);
		}
	}
}

void QCBUILTIN PF_terrain_edit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	int action = G_FLOAT(OFS_PARM0);
	vec3_t pos;// G_VECTOR(OFS_PARM1);
	float radius = G_FLOAT(OFS_PARM2);
	float quant = G_FLOAT(OFS_PARM3);
	int modelindex = ((wedict_t*)PROG_TO_EDICT(prinst, *vmw->g.self))->v->modelindex;
//	G_FLOAT(OFS_RETURN) = Heightmap_Edit(w->worldmodel, action, pos, radius, quant);
	model_t *mod = vmw->Get_CModel(vmw, modelindex);
	heightmap_t *hm;
	vec4_t tally;

	G_FLOAT(OFS_RETURN) = 0;

	if (!mod)
		return;
	if (mod->loadstate == MLS_LOADING)
		COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
	if (mod->loadstate != MLS_LOADED)
		return;

	switch(action)
	{
	case ter_ent_get:
		{
			unsigned int idx = G_INT(OFS_PARM1);
			if (!mod->numentityinfo)
				Mod_ParseEntities(mod);
			if (idx >= mod->numentityinfo || !mod->entityinfo[idx].keyvals)
				G_INT(OFS_RETURN) = 0;
			else
				G_INT(OFS_RETURN) = PR_TempString(prinst, mod->entityinfo[idx].keyvals);
		}
		return;
	case ter_ent_set:
		{
			unsigned int idx = G_INT(OFS_PARM1);
			int id;
			const char *newvals;
			if (idx >= MAX_EDICTS)	//we need some sanity limit... many ents will get removed like lights so this one isn't quite correct, but it'll be in the right sort of ballpark.
			{
				G_INT(OFS_RETURN) = 0;
				return;
			}
			//if there's no ents, then that's a problem. make sure that there's at least a worldspawn.
			if (!mod->numentityinfo)
				Mod_ParseEntities(mod);
			//make sure we don't have any cached entities string, by wiping it all.
			Z_Free((char*)mod->entities_raw);
			mod->entities_raw = NULL;

			G_INT(OFS_RETURN) = 0;
			if (idx < mod->numentityinfo)
			{
				if (!G_INT(OFS_PARM2) && !mod->entityinfo[idx].keyvals)
					return; //no-op
				Z_Free(mod->entityinfo[idx].keyvals);
				mod->entityinfo[idx].keyvals = NULL;
				id = mod->entityinfo[idx].id;
			}
			else
				id = 0;
			if (G_INT(OFS_PARM2))
			{
				newvals = PR_GetStringOfs(prinst, OFS_PARM2);
				if (idx >= mod->numentityinfo)
					Z_ReallocElements((void**)&mod->entityinfo, &mod->numentityinfo, idx+64, sizeof(*mod->entityinfo));
				mod->entityinfo[idx].keyvals = Z_StrDup(newvals);
				if (!id)
					id = (idx+1) | ((cl.playerview[0].playernum+1)<<24);
				mod->entityinfo[idx].id = id;
			}
			else
			{
				newvals = NULL;
				if (idx < mod->numentityinfo)
					mod->entityinfo[idx].id = 0;
			}

#ifdef HAVE_SERVER
			if (sv_state && modelindex > 0)
			{
				MSG_WriteByte(&sv.multicast, svcfte_brushedit);
				MSG_WriteShort(&sv.multicast, modelindex);
				MSG_WriteByte(&sv.multicast, newvals?hmcmd_ent_edit:hmcmd_ent_remove);
				MSG_WriteLong(&sv.multicast, id);
				if (newvals)
					MSG_WriteString(&sv.multicast, newvals);
				SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
				//tell ssqc, csqc will be told by the server

				SSQC_MapEntityEdited(modelindex, idx, newvals);
			}
			else
#endif
#ifdef HAVE_CLIENT
			if (cls.state && modelindex > 0)
			{
				MSG_WriteByte(&cls.netchan.message, clcfte_brushedit);
				MSG_WriteShort(&cls.netchan.message, modelindex);
				MSG_WriteByte(&cls.netchan.message, newvals?hmcmd_ent_edit:hmcmd_ent_remove);
				MSG_WriteLong(&cls.netchan.message, id);
				if (newvals)
					MSG_WriteString(&cls.netchan.message, newvals);

				#ifdef CSQC_DAT
					CSQC_MapEntityEdited(modelindex, idx, newvals);
				#endif
			}
			else
#endif
			{
				#ifdef CSQC_DAT
					CSQC_MapEntityEdited(modelindex, idx, newvals);
				#endif
			}
		}
		return;
	case ter_ent_add:
		{
//			int idx = G_INT(OFS_PARM1);
//			const char *news = PR_GetStringOfs(prinst, OFS_PARM2);
			G_INT(OFS_RETURN) = mod->numentityinfo;
		}
		return;
	case ter_ent_count:
		if (!mod->numentityinfo)
			Mod_ParseEntities(mod);
		G_INT(OFS_RETURN) = mod->numentityinfo;
		return;
	case ter_ents_wipe_deprecated:
		G_INT(OFS_RETURN) = PR_TempString(prinst, Mod_GetEntitiesString(mod));
		Mod_SetEntitiesString(mod, "", true);
		return;
	case ter_ents_concat_deprecated:
		{
			char *newv;
			const char *olds = Mod_GetEntitiesString(mod);
			const char *news = PR_GetStringOfs(prinst, OFS_PARM1);
			size_t oldlen = strlen(olds);
			size_t newlen = strlen(news);
			newv = Z_Malloc(oldlen + newlen + 1);
			memcpy(newv, olds, oldlen);
			memcpy(newv+oldlen, news, newlen);
			newv[oldlen + newlen] = 0;
			Z_Free((char*)olds);
			G_FLOAT(OFS_RETURN) = oldlen + newlen;
			
			Mod_SetEntitiesString(mod, newv, false);
			if (mod->terrain)
			{
				hm = mod->terrain;
				hm->entsdirty = true;
			}
		}
		return;
	case ter_ents_get:
		G_INT(OFS_RETURN) = PR_TempString(prinst, Mod_GetEntitiesString(mod));
		return;
	case ter_save:
		if (mod->terrain)
		{
			quant = Heightmap_Save(mod->terrain);
			Con_DPrintf("ter_save: %g sections saved\n", quant);
		}
		G_FLOAT(OFS_RETURN) = quant;
		/*
		if (mod->type == mod_brush)
		{
			Con_TPrintf("that model isn't a suitable worldmodel\n");
			return;
		}
		else
		{
			FS_CreatePath(fname, FS_GAMEONLY);
			file = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
			if (!file)
				Con_TPrintf("unable to open %s\n", fname);
			else
			{
				Terr_WriteMapFile(file, mod);
				VFS_CLOSE(file);
			}
		}*/
		return;
	}

	if (!mod->terrain)
	{
		char basename[MAX_QPATH];
		COM_FileBase(mod->name, basename, sizeof(basename));
		mod->terrain = Mod_LoadTerrainInfo(mod, basename, true);
		hm = mod->terrain;
		if (!hm)
			return;
		Terr_FinishTerrain(mod);
	}
	hm = mod->terrain;

	pos[0] = G_FLOAT(OFS_PARM1+0) + hm->sectionsize * CHUNKBIAS;
	pos[1] = G_FLOAT(OFS_PARM1+1) + hm->sectionsize * CHUNKBIAS;
	pos[2] = G_FLOAT(OFS_PARM1+2);

	switch(action)
	{
	case ter_reload:
		G_FLOAT(OFS_RETURN) = 1;
		Terr_PurgeTerrainModel(mod, false, true);
		break;
	case ter_sethole:
	/*	{
			int x, y;
			hmsection_t *s;
			x = pos[0]*4 / hm->sectionsize;
			y = pos[1]*4 / hm->sectionsize;
			x = bound(hm->firstsegx*4, x, hm->maxsegx*4-1);
			y = bound(hm->firstsegy*4, y, hm->maxsegy*4-1);
		
			s = Terr_GetSection(hm, x/4, y/4, TGS_FORCELOAD);
			if (!s)
				return;
			ted_sethole(&quant, s, (x&3) + (y&3)*4, x/4, y/4, 0);
		}
	*/	
		pos[0] -= 0.5 * hm->sectionsize / 8;
		pos[1] -= 0.5 * hm->sectionsize / 8;
		ted_itterate(hm, tid_linear, pos, radius, 1, 9, ted_sethole, &quant);
		break;
	case ter_height_set:
		ted_itterate(hm, tid_linear, pos, radius, 1, SECTHEIGHTSIZE, ted_heightset, &quant);
		break;
	case ter_height_flatten:
		tally[0] = 0;
		tally[1] = 0;
		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTHEIGHTSIZE, ted_heighttally, &tally);
		tally[0] /= tally[1];
		if (IS_NAN(tally[0]))
			tally[0] = 0;
		ted_itterate(hm, tid_exponential, pos, radius, quant, SECTHEIGHTSIZE, ted_heightsmooth, &tally);

		ted_itterate(hm, tid_exponential, pos, radius, quant, SECTHEIGHTSIZE, ted_heightdebug, &tally);
		break;
	case ter_height_smooth:
		tally[0] = 0;
		tally[1] = 0;
		ted_itterate(hm, tid_linear, pos, radius, 1, SECTHEIGHTSIZE, ted_heighttally, &tally);
		tally[0] /= tally[1];
		if (IS_NAN(tally[0]))
			tally[0] = 0;
		ted_itterate(hm, tid_linear, pos, radius, quant, SECTHEIGHTSIZE, ted_heightsmooth, &tally);
		break;
	case ter_height_spread:
		tally[0] = 0;
		tally[1] = 0;
		ted_itterate(hm, tid_exponential, pos, radius/2, 1, SECTHEIGHTSIZE, ted_heighttally, &tally);
		tally[0] /= tally[1];
		if (IS_NAN(tally[0]))
			tally[0] = 0;
		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTHEIGHTSIZE, ted_heightsmooth, &tally);
		break;
	case ter_water_set:
		ted_itterate(hm, tid_linear, pos, radius, 1, 9, ted_waterset, &quant);
		break;
	case ter_lower:
		quant *= -1;
	case ter_raise:
		ted_itterate(hm, tid_exponential, pos, radius, quant, SECTHEIGHTSIZE, ted_heightraise, &quant);
		break;
	case ter_tint:
		ted_itterate(hm, tid_exponential, pos, radius, quant, SECTHEIGHTSIZE, ted_tint, G_VECTOR(OFS_PARM4));	//and parm5 too
		break;
//	case ter_mixset:
//		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTTEXSIZE, ted_mixset, G_VECTOR(OFS_PARM4));
//		break;
	case ter_tex_blend:
		ted_itterate(hm, tid_exponential, pos, radius, quant/10, SECTTEXSIZE, ted_texpaint, (void*)PR_GetStringOfs(prinst, OFS_PARM4));
		break;
	case ter_tex_replace:
		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTTEXSIZE, ted_texreplace, (void*)PR_GetStringOfs(prinst, OFS_PARM3));
		break;
	case ter_tex_concentrate:
		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTTEXSIZE, ted_texconcentrate, NULL);
		break;
	case ter_tex_noise:
		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTTEXSIZE, ted_texnoise, NULL);
		break;
	case ter_tex_blur:
		Vector4Set(tally, 0, 0, 0, 0);
		ted_itterate(hm, tid_exponential, pos, radius, 1, SECTTEXSIZE, ted_textally, &tally);
		VectorScale(tally, 1/(tally[3]*255), tally);
		ted_itterate(hm, tid_exponential, pos, radius, quant, SECTTEXSIZE, ted_texset, &tally);
		break;
	case ter_tex_get:
		{
			int x, y;
			hmsection_t *s;
			x = pos[0] / hm->sectionsize;
			y = pos[1] / hm->sectionsize;
			x = bound(hm->firstsegx, x, hm->maxsegx-1);
			y = bound(hm->firstsegy, y, hm->maxsegy-1);
		
			s = Terr_GetSection(hm, x, y, TGS_WAITLOAD|TGS_DEFAULTONFAIL);
			if (!s)
				return;
			x = bound(0, quant, 3);
			G_INT(OFS_RETURN) = PR_TempString(prinst, s->texname[x]);
		}
		break;
	case ter_tex_mask:
		Z_Free(hm->texmask);
		hm->texmask = NULL;

		if (G_INT(OFS_PARM1) == 0)
			hm->texmask = NULL;
		else
			hm->texmask = Z_StrDup(PR_GetStringOfs(prinst, OFS_PARM1));
		break;
	case ter_tex_kill:
		{
			int x, y;
			x = pos[0] / hm->sectionsize;
			y = pos[1] / hm->sectionsize;
			x = bound(hm->firstsegx, x, hm->maxsegx-1);
			y = bound(hm->firstsegy, y, hm->maxsegy-1);

			ted_texkill(Terr_GetSection(hm, x, y, TGS_WAITLOAD|TGS_DEFAULTONFAIL), PR_GetStringOfs(prinst, OFS_PARM4));
		}
		break;
	case ter_reset:
		{
			int x, y;
			hmsection_t *s;
			x = pos[0] / hm->sectionsize;
			y = pos[1] / hm->sectionsize;
			x = bound(hm->firstsegx, x, hm->maxsegx-1);
			y = bound(hm->firstsegy, y, hm->maxsegy-1);
			s = Terr_GetSection(hm, x, y, TGS_WAITLOAD|TGS_DEFAULTONFAIL);
			if (s)
			{
				s->flags = (s->flags & ~TSF_EDITED);
				Terr_ClearSection(s);
				Terr_GenerateDefault(hm, s);
			}
		}
		break;
	case ter_mesh_add:
		{
			vec3_t axis[3];
			wedict_t *ed = G_WEDICT(prinst, OFS_PARM1);
			//FIXME: modeltype pitch inversion
			AngleVectorsFLU(ed->v->angles, axis[0], axis[1], axis[2]);
			Terr_AddMesh(hm, TGS_WAITLOAD|TGS_DEFAULTONFAIL, vmw->Get_CModel(vmw, ed->v->modelindex), NULL, ed->v->origin, axis, ed->xv->scale);
		}
		break;
	case ter_mesh_kill:
		{
			int i;
//			entity_t *e;
			int x, y;
//			float r;
			hmsection_t *s;
			x = pos[0] / hm->sectionsize;
			y = pos[1] / hm->sectionsize;
			x = bound(hm->firstsegx, x, hm->maxsegx-1);
			y = bound(hm->firstsegy, y, hm->maxsegy-1);
		
			s = Terr_GetSection(hm, x, y, TGS_WAITLOAD|TGS_DEFAULTONFAIL);
			if (!s)
				return;

			Sys_LockMutex(hm->entitylock);
			//FIXME: this doesn't work properly.
			if (s->numents)
			{
				for (i = 0; i < s->numents; i++)
					s->ents[i]->refs -= 1;
				s->flags |= TSF_EDITED;
				s->numents = 0;
			}
			Sys_UnlockMutex(hm->entitylock);
		}
		break;
	}
}
#else
static unsigned char *QDECL Terr_GetLightmap(hmsection_t *s, int idx, qboolean edit)
{
	return NULL;
}
void QCBUILTIN PF_terrain_edit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = 0;
}
#endif

void Terr_ParseEntityLump(model_t *mod, heightmap_t *heightmap)
{
	char key[128];
	char value[2048];
	const char *data = Mod_GetEntitiesString(mod);

	heightmap->sectionsize = 1024;
	heightmap->mode = HMM_TERRAIN;
	heightmap->culldistance = 4096*4096;
	heightmap->forcedefault = false;

	heightmap->defaultgroundheight = 0;
	heightmap->defaultwaterheight = 0;
	Q_snprintfz(heightmap->defaultwatershader, sizeof(heightmap->defaultwatershader), "water/%s", heightmap->path);
	Q_strncpyz(heightmap->defaultgroundtexture, "", sizeof(heightmap->defaultgroundtexture));

	if (data)
	if ((data=COM_ParseOut(data, key, sizeof(key))))	//read the map info.
	if (key[0] == '{')
	while (1)
	{
		if (!(data=COM_ParseOut(data, key, sizeof(key))))
			break; // error
		if (key[0] == '}')
			break; // end of worldspawn
		if (key[0] == '_')
			memmove(key, key+1, strlen(key)); //_ vars are for comments/utility stuff that arn't visible to progs and for compat. We want to support these stealth things.
		if (!((data=COM_ParseOut(data, value, sizeof(value)))))
			break; // error		
		if (!strcmp("segmentsize", key))
			heightmap->sectionsize = atof(value);
		else if (!strcmp("minxsegment", key))
			heightmap->firstsegx = atoi(value);
		else if (!strcmp("minysegment", key))
			heightmap->firstsegy = atoi(value);
		else if (!strcmp("maxxsegment", key))
			heightmap->maxsegx = atoi(value);
		else if (!strcmp("maxysegment", key))
			heightmap->maxsegy = atoi(value);
		else if (!strcmp("forcedefault", key))
			heightmap->forcedefault = !!atoi(value);
		else if (!strcmp("defaultwaterheight", key))
			heightmap->defaultwaterheight = atof(value);
		else if (!strcmp("defaultgroundheight", key))
			heightmap->defaultgroundheight = atof(value);
		else if (!strcmp("defaultgroundtexture", key))
			Q_strncpyz(heightmap->defaultgroundtexture, value, sizeof(heightmap->defaultgroundtexture));
		else if (!strcmp("defaultwatertexture", key))
			Q_strncpyz(heightmap->defaultwatershader, value, sizeof(heightmap->defaultwatershader));
		else if (!strcmp("culldistance", key))
		{
			heightmap->culldistance = atof(value);
			heightmap->culldistance *= heightmap->culldistance;
		}
		else if (!strcmp("drawdist", key))
			heightmap->maxdrawdist = atof(value);
		else if (!strcmp("seed", key))
		{
			Z_Free(heightmap->seed);
			heightmap->seed = Z_StrDup(value);
		}
		else if (!strcmp("exterior", key))
		{
			heightmap->legacyterrain = false;
			if (!strcmp(value, "empty") || !strcmp(value, ""))
				heightmap->exteriorcontents = FTECONTENTS_EMPTY;
			else if (!strcmp(value, "sky"))
				heightmap->exteriorcontents = FTECONTENTS_SKY;
			else if (!strcmp(value, "lava"))
				heightmap->exteriorcontents = FTECONTENTS_LAVA;
			else //if (!strcmp(value, "solid"))
				heightmap->exteriorcontents = FTECONTENTS_SOLID;
		}
		else if (!strcmp("skybox", key))
			Q_strncpyz(heightmap->skyname, value, sizeof(heightmap->skyname));
		else if (!strcmp("tiles", key))
		{
			char *d;
			heightmap->mode = HMM_BLOCKS;
			d = value;
			d = COM_ParseOut(d, key, sizeof(key));
			heightmap->tilepixcount[0] = atoi(key);
			d = COM_ParseOut(d, key, sizeof(key));
			heightmap->tilepixcount[1] = atoi(key);
			d = COM_ParseOut(d, key, sizeof(key));
			heightmap->tilecount[0] = atoi(key);
			d = COM_ParseOut(d, key, sizeof(key));
			heightmap->tilecount[1] = atoi(key);
		}
	}

	/*bias and bound it*/
	heightmap->firstsegx += CHUNKBIAS;
	heightmap->firstsegy += CHUNKBIAS;
	heightmap->maxsegx += CHUNKBIAS;
	heightmap->maxsegy += CHUNKBIAS;
	if (heightmap->firstsegx < 0)
		heightmap->firstsegx = 0;
	if (heightmap->firstsegy < 0)
		heightmap->firstsegy = 0;
	if (heightmap->maxsegx > CHUNKLIMIT)
		heightmap->maxsegx = CHUNKLIMIT;
	if (heightmap->maxsegy > CHUNKLIMIT)
		heightmap->maxsegy = CHUNKLIMIT;
}

void Terr_FinishTerrain(model_t *mod)
{
#ifdef HAVE_CLIENT
	heightmap_t *hm = mod->terrain;
	if (qrenderer != QR_NONE)
	{
		if (*hm->skyname)
		{
			hm->skyshader = R_RegisterCustom(mod, va("skybox_%s", hm->skyname), SUF_NONE, Shader_DefaultSkybox, NULL);
			if (!hm->skyshader->skydome)
				hm->skyshader = NULL;
		}
		else
			hm->skyshader = NULL;

		switch (hm->mode)
		{
		case HMM_BLOCKS:
			hm->shader = R_RegisterShader("terraintileshader", SUF_NONE,
					"{\n"
						"{\n"
							"map $diffuse\n"	
						"}\n"
					"}\n"
				);
			break;
		case HMM_TERRAIN:
			hm->shader = R_RegisterShader(hm->groundshadername, SUF_LIGHTMAP,
					"{\n"
						"bemode rtlight\n"
							"{\n"
								"{\n"
									"map $diffuse\n"
									"blendfunc add\n"
								"}\n"
								//FIXME: these maps are a legacy thing, and could be removed if third-party glsl properly contains s_diffuse
								"{\n"
									"map $upperoverlay\n"
								"}\n"
								"{\n"
									"map $loweroverlay\n"
								"}\n"
								"{\n"
									"map $fullbright\n"
								"}\n"
								"{\n"
									"map $lightmap\n"
								"}\n"
								"{\n"
									"map $shadowmap\n"
								"}\n"
								"{\n"
									"map $lightcubemap\n"
								"}\n"
								//woo, one glsl to rule them all
								"program terrain#RTLIGHT\n"
							"}\n"
						"bemode depthdark\n"
							"{\n"
								"program depthonly\n"
								"{\n"
									"depthwrite\n"
								"}\n"
							"}\n"
						"bemode depthonly\n"
							"{\n"
								"program depthonly\n"
								"{\n"
									"depthwrite\n"
									"maskcolor\n"
								"}\n"
							"}\n"

						//FIXME: these maps are a legacy thing, and could be removed if third-party glsl properly contains s_diffuse
						"{\n"
							"map $diffuse\n"
							"rgbgen vertex\n"
							"alphagen vertex\n"
						"}\n"
						"{\n"
							"map $upperoverlay\n"
						"}\n"
						"{\n"
							"map $loweroverlay\n"
						"}\n"
						"{\n"
							"map $fullbright\n"
						"}\n"
						"{\n"
							"map $lightmap\n"
						"}\n"
						"program terrain\n"
						"if r_terraindebug\n"
						"program terraindebug\n"
						"endif\n"
					"}\n"
				);
			break;
		}
	}
#endif
}

#ifdef HAVE_CLIENT
void Terr_Brush_Draw(heightmap_t *hm, batch_t **batches, entity_t *e)
{
	batch_t *b;
	size_t i, j;
	vbobctx_t ctx;

	brushbatch_t *bb;
	brushtex_t *bt;
	brushes_t *br;

	struct {
		vecV_t coord[65536];
		vec2_t texcoord[65536];
		vec2_t lmcoord[65536];
		vec3_t normal[65536];
		vec3_t svector[65536];
		vec3_t tvector[65536];
		vec4_t rgba[65536];
		index_t index[65535];
	} *arrays = NULL;
	size_t numverts = 0;
	size_t numindicies = 0;
	int w, h, lmnum;
	float scale[2];
#ifdef RUNTIMELIGHTING
	lightmapinfo_t *lm;
	qboolean dorelight = true;

	//FIXME: lightmaps
	//if we're enabling lightmaps, make sure all surfaces have known sizes first.
	//allocate lightmap space for all surfaces, and then rebuild all textures.
	//if a surface is modified, clear its lightmap to -1 and when its batches are rebuilt, it'll unlight naturally.

	if (hm->entsdirty)
	{
		model_t *mod = e->model;
		if (mod->submodelof)
			mod = mod->submodelof;
		hm->entsdirty = false;
		if (hm->relightcontext)
			LightReloadEntities(hm->relightcontext, Mod_GetEntitiesString(mod), true);

		//FIXME: figure out some way to hint this without having to relight the entire frigging world.
		for (bt = hm->brushtextures; bt; bt = bt->next)
			for (i = 0, br = hm->wbrushes; i < hm->numbrushes; i++, br++)
				for (j = 0; j < br->numplanes; j++)
					br->faces[j].relight = true;
	}

	if (hm->recalculatebrushlighting && !r_fullbright.ival)
	{
		unsigned int lmcount;
		unsigned int lmblocksize = 512;//LMBLOCK_SIZE_MAX
		hm->recalculatebrushlighting = false;

		if (!hm->relightcontext)
		{
			for (numverts = 0, numindicies = 0, i = 0, br = hm->wbrushes; i < hm->numbrushes; i++, br++)
			{
				for (j = 0; j < br->numplanes; j++)
				{
					br->faces[j].lightmap = -1;
					br->faces[j].lmbase[0] = 0;
					br->faces[j].lmbase[1] = 0;
				}
			}
			for (bt = hm->brushtextures; bt; bt = bt->next)
			{
				bt->rebuild = true;
				bt->firstlm = 0;
				bt->lmcount = 0;
			}

			BZ_Free(hm->brushlmremaps);
			hm->brushlmremaps = NULL;
			hm->brushmaxlms = 0;
		}
		else
		{
			Mod_LightmapAllocInit(&hm->brushlmalloc, false, lmblocksize, lmblocksize, 0);
			hm->brushlmscale = 1.0/lmblocksize;

			//textures is to try to ensure that they are allocated consecutively.
			for (bt = hm->brushtextures; bt; bt = bt->next)
			{
				bt->firstlm = hm->brushlmalloc.lmnum;
				for (numverts = 0, numindicies = 0, i = 0, br = hm->wbrushes; i < hm->numbrushes; i++, br++)
				{
					for (j = 0; j < br->numplanes; j++)
					{
						if (br->faces[j].tex == bt)
						{
							if (br->faces[j].lightdata)
							{
								Mod_LightmapAllocBlock(&hm->brushlmalloc, br->faces[j].lmextents[0], br->faces[j].lmextents[1], &br->faces[j].lmbase[0], &br->faces[j].lmbase[1], &br->faces[j].lightmap);
								br->faces[j].relit = true;
							}
							else
							{	//this surface has no lightmap info or something.
								br->faces[j].lightmap = -1;
								br->faces[j].lmbase[0] = 0;
								br->faces[j].lmbase[1] = 0;
							}
						}
					}
				}
				bt->rebuild = true;
				bt->lmcount = hm->brushlmalloc.lmnum - bt->firstlm;
				if (hm->brushlmalloc.allocated[0])
					bt->lmcount++;
				if (hm->brushlmalloc.deluxe)
				{
					bt->firstlm *= 2;
					bt->lmcount *= 2;
				}
			}

			lmcount = hm->brushlmalloc.lmnum;
			if (hm->brushlmalloc.allocated[0])
				lmcount++;
			if (hm->brushlmalloc.deluxe)
				lmcount *= 2;

			if (lmcount > hm->brushmaxlms)
			{
				int first;
				hm->brushlmremaps = BZ_Realloc(hm->brushlmremaps, sizeof(*hm->brushlmremaps) * lmcount);
				first = Surf_NewLightmaps(lmcount - hm->brushmaxlms, hm->brushlmalloc.width, hm->brushlmalloc.height, PTI_BGRA8, hm->brushlmalloc.deluxe);

				while(hm->brushmaxlms < lmcount)
					hm->brushlmremaps[hm->brushmaxlms++] = first++;
			}
		}
	}

	if (hm->relightcontext && !r_fullbright.ival)
	for (i = 0, br = hm->wbrushes; i < hm->numbrushes; i++, br++)
	{
		for (j = 0; j < br->numplanes; j++)
		{
			if (br->faces[j].relight && dorelight)
			{
				lightstyleindex_t styles[max(2,MAXCPULIGHTMAPS)] = {0,INVALID_LIGHTSTYLE};
				int texsize[2] = {br->faces[j].lmextents[0]-1, br->faces[j].lmextents[1]-1};
				vec2_t exactmins, exactmaxs;
				int m, k;
				vec2_t lm;
				for (m = 0; m < br->faces[j].numpoints; m++)
				{
					for (k = 0; k < 2; k++)
					{
						lm[k] = DotProduct(br->faces[j].points[m], br->faces[j].stdir[k]) + br->faces[j].stdir[k][3];
						if (m == 0)
						exactmins[k] = exactmaxs[k] = lm[k];
						else if (lm[k] > exactmaxs[k])
						exactmaxs[k] = lm[k];
						else if (lm[k] < exactmins[k])
						exactmins[k] = lm[k];
					}
				}

				dorelight = false;
				br->faces[j].relight = false;
				LightPlane (hm->relightcontext, hm->lightthreadmem, styles, NULL, br->faces[j].lightdata, NULL, br->planes[j], br->faces[j].stdir, exactmins, exactmaxs, br->faces[j].lmbias, texsize, br->faces[j].lmscale);	//special version that doesn't know what a face is or anything.
				br->faces[j].relit = true;
			}
			if (br->faces[j].relit && br->faces[j].lightmap >= 0)
			{
				int s,t;
				qbyte *out, *in;
				lm = lightmap[hm->brushlmremaps[br->faces[j].lightmap]];

				br->faces[j].relit = false;

				lm->modified = true;
				lm->rectchange.l = 0;
				lm->rectchange.t = 0;
				lm->rectchange.r = lm->width;
				lm->rectchange.b = lm->height;

				in = br->faces[j].lightdata;
				out = lm->lightmaps + (br->faces[j].lmbase[1] * lm->width + br->faces[j].lmbase[0]) * lm->pixbytes;
				switch(lm->fmt)
				{
				default:
					Sys_Error("Bad terrain lightmap format %i\n", lm->fmt);
					break;
				case PTI_BGRA8:
				case PTI_BGRX8:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*out++ = in[2];
							*out++ = in[1];
							*out++ = in[0];
							*out++ = 0xff;
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 4;
					}
					break;
				case PTI_RGBA8:
				case PTI_RGBX8:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*out++ = in[0];
							*out++ = in[1];
							*out++ = in[2];
							*out++ = 0xff;
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 4;
					}
					break;
				case PTI_BGR8:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*out++ = in[2];
							*out++ = in[1];
							*out++ = in[0];
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 3;
					}
					break;
				case PTI_RGB8:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*out++ = in[0];
							*out++ = in[1];
							*out++ = in[2];
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 3;
					}
					break;


				case PTI_A2BGR10:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*(unsigned int*)out = (0x3u<<30) | (in[2]<<22) | (in[1]<<12) | (in[0]<<2);
							out+=4;
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 4;
					}
					break;
				/*case PTI_E5BGR9:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*(unsigned int*)out = Surf_PackE5BRG9(in[0], in[1], in[2], 8);
							out+=4;
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 4;
					}
					break;*/
				case PTI_L8:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							*out++ = max(max(in[0], in[1]), in[2]);
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]);
					}
					break;
				case PTI_RGBA32F:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							((float*)out)[0] = in[0]/255.0;
							((float*)out)[1] = in[1]/255.0;
							((float*)out)[2] = in[2]/255.0;
							((float*)out)[3] = 1.0;
							out+=16;
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 16;
					}
					break;
				/*case PTI_RGBA16F:
					for (t = 0; t < br->faces[j].lmextents[1]; t++)
					{
						for (s = 0; s < br->faces[j].lmextents[0]; s++)
						{
							Surf_PackRGB16F(in[0], in[1], in[2], 255);
							out+=8;
							in+=3;
						}
						out += (lm->width - br->faces[j].lmextents[0]) * 8;
					}
					break;*/
				case PTI_RGB565:
				case PTI_RGBA4444:
				case PTI_RGBA5551:
				case PTI_ARGB4444:
				case PTI_ARGB1555:
					break;
				}
			}
		}
	}
#endif

	for (bt = hm->brushtextures; bt; bt = bt->next)
	{
		if (!bt->shader)
		{
#ifdef PACKAGE_TEXWAD
			miptex_t *tx = W_GetMipTex(bt->shadername);
#else
			const miptex_t *tx = NULL;
#endif

			bt->shader = R_RegisterCustom (NULL, va("textures/%s", bt->shadername), SUF_LIGHTMAP, NULL, NULL);
			if (!bt->shader)
			{
				if (!Q_strcasecmp(bt->shadername, "clip") || !Q_strcasecmp(bt->shadername, "hint") || !Q_strcasecmp(bt->shadername, "skip"))
					bt->shader = R_RegisterShader(bt->shadername, SUF_LIGHTMAP, "{\nsurfaceparm nodraw\n}");
				else
					bt->shader = R_RegisterCustom (NULL, bt->shadername, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);
//					bt->shader = R_RegisterShader_Lightmap(bt->shadername);
			}

			if (!Q_strncasecmp(bt->shadername, "sky", 3) && tx)
				R_InitSky (bt->shader, bt->shadername, TF_SOLID8, (qbyte*)tx + tx->offsets[0], tx->width, tx->height);
			else if (tx)
			{
				unsigned int mapflags = SHADER_HASPALETTED | SHADER_HASDIFFUSE | SHADER_HASFULLBRIGHT | SHADER_HASNORMALMAP | SHADER_HASGLOSS;
				R_BuildLegacyTexnums(bt->shader, tx->name, NULL, mapflags, 0, TF_SOLID8, tx->width, tx->height, (qbyte*)tx + tx->offsets[0], NULL);
			}
			else
				R_BuildDefaultTexnums(NULL, bt->shader, IF_WORLDTEX);

			if (tx)
			{
				if (!bt->shader->width)
					bt->shader->width = tx->width;
				if (!bt->shader->height)
					bt->shader->height = tx->height;
				BZ_Free(tx);
			}
		}

		if (bt->rebuild)
		{
			//FIXME: don't block.
			if (R_GetShaderSizes(bt->shader, &w, &h, false) < 0)
				continue;
			bt->rebuild = false;

			if (w<1) w = 64;
			if (h<1) h = 64;
			scale[0] = mod_terrain_brushtexscale.value/w;	//I hate needing this.
			scale[1] = mod_terrain_brushtexscale.value/h;

			while(bt->batches)
			{
				bb = bt->batches;
				bt->batches = bb->next;

				BE_VBO_Destroy(&bb->vbo.coord, bb->vbo.vbomem);
				BE_VBO_Destroy(&bb->vbo.indicies, bb->vbo.ebomem);
				BZ_Free(bb);
			}

			if (!arrays)
				arrays = BZ_Malloc(sizeof(*arrays));

			for (lmnum = -1; lmnum < bt->firstlm+bt->lmcount; ((lmnum==-1)?(lmnum=bt->firstlm):(lmnum=lmnum+1)))
			{
				i = 0;
				br = hm->wbrushes;
				for (;i < hm->numbrushes;)
				{
					for (numverts = 0, numindicies = 0; i < hm->numbrushes; i++, br++)
					{
						if (br->selected)
							continue;
						if (br->patch)
						{	//this one's a patch
							if (br->patch->tex == bt && lmnum == -1)
							{
								int x, y;
								index_t r1, r2;

								if (br->patch->tessvert && !br->selected)
								{	//tessellated version of the patch.

									//make sure we don't overflow anything.
									size_t newverts = br->patch->tesssize[0]*br->patch->tesssize[1], newindexes = (br->patch->tesssize[0]-1)*(br->patch->tesssize[1]-1)*6;
									if (numverts+newverts >= 0xffff || numindicies+newindexes >= 0xffff)
										break;

									for (y = 0, r1 = numverts, r2 = 0; y < br->patch->tesssize[1]; y++)
									{
										for (x = 0; x < br->patch->tesssize[0]; x++, r1++, r2++)
										{
											VectorCopy(br->patch->tessvert[r2].v, arrays->coord[r1]);
											Vector2Copy(br->patch->tessvert[r2].tc, arrays->texcoord[r1]);
											Vector4Copy(br->patch->tessvert[r2].rgba, arrays->rgba[r1]);

											//lame
											Vector2Copy(br->patch->tessvert[r2].tc, arrays->lmcoord[r1]);
										}
									}
									for (y = 0, r1 = numverts, r2 = r1 + br->patch->tesssize[0]; y < br->patch->tesssize[1]-1; y++)
									{
										for (x = 0; x < br->patch->tesssize[0]-1; x++, r1++, r2++)
										{
											arrays->index[numindicies++] = r1;
											arrays->index[numindicies++] = r1+1;
											arrays->index[numindicies++] = r2;

											arrays->index[numindicies++] = r1+1;
											arrays->index[numindicies++] = r2+1;
											arrays->index[numindicies++] = r2;
										}
										r1++; r2++;
									}
									Mod_AccumulateTextureVectors(arrays->coord, arrays->texcoord, arrays->normal, arrays->svector, arrays->tvector, arrays->index+numindicies-newindexes, newindexes, true);
									Mod_NormaliseTextureVectors(arrays->normal+numverts, arrays->svector+numverts, arrays->tvector+numverts, newverts, true);
									numverts += newverts;
								}
								else
								{	//control-point representation.

									//make sure we don't overflow anything.
									size_t newverts = br->patch->numcp[0]*br->patch->numcp[1], newindexes = (br->patch->numcp[0]-1)*(br->patch->numcp[1]-1)*6;
									if (numverts+newverts >= 0xffff || numindicies+newindexes >= 0xffff)
										break;

									for (y = 0, r1 = numverts, r2 = 0; y < br->patch->numcp[1]; y++)
									{
										for (x = 0; x < br->patch->numcp[0]; x++, r1++, r2++)
										{
											VectorCopy(br->patch->cp[r2].v, arrays->coord[r1]);
											Vector2Copy(br->patch->cp[r2].tc, arrays->texcoord[r1]);
											Vector4Copy(br->patch->cp[r2].rgba, arrays->rgba[r1]);

											//lame
											Vector2Copy(br->patch->cp[r2].tc, arrays->lmcoord[r1]);
										}
									}
									for (y = 0, r1 = numverts, r2 = r1 + br->patch->numcp[0]; y < br->patch->numcp[1]-1; y++)
									{
										for (x = 0; x < br->patch->numcp[0]-1; x++, r1++, r2++)
										{
											arrays->index[numindicies++] = r1;
											arrays->index[numindicies++] = r1+1;
											arrays->index[numindicies++] = r2;

											arrays->index[numindicies++] = r1+1;
											arrays->index[numindicies++] = r2+1;
											arrays->index[numindicies++] = r2;
										}
										r1++; r2++;
									}
									Mod_AccumulateTextureVectors(arrays->coord, arrays->texcoord, arrays->normal, arrays->svector, arrays->tvector, arrays->index+numindicies-newindexes, newindexes, true);
									Mod_NormaliseTextureVectors(arrays->normal+numverts, arrays->svector+numverts, arrays->tvector+numverts, newverts, true);
									numverts += newverts;
								}
							}
						}
						else
						{	//regular brush

							//make sure we don't overflow anything.
							size_t newverts = 0, newindexes = 0;
							for (j = 0; j < br->numplanes; j++)
								if (br->faces[j].tex == bt && !br->selected && br->faces[j].lightmap == lmnum)
									newverts += br->faces[j].numpoints, newindexes += (br->faces[j].numpoints-2)*3;
							if (numverts+newverts >= 0xffff || numindicies+newindexes >= 0xffff)
								break;

							for (j = 0; j < br->numplanes; j++)
							{
								if (br->faces[j].tex == bt && !br->selected && br->faces[j].lightmap == lmnum)
								{
									size_t k, o;
									float s,t;

									for (k = 0, o = numverts; k < br->faces[j].numpoints; k++, o++)
									{
										VectorCopy(br->faces[j].points[k], arrays->coord[o]);
										VectorCopy(br->planes[j], arrays->normal[o]);
										VectorCopy(br->faces[j].stdir[0], arrays->svector[o]);
										VectorCopy(br->faces[j].stdir[1], arrays->tvector[o]);
										Vector4Set(arrays->rgba[o], 1.0, 1.0, 1.0, 1.0);

										//compute the texcoord planes
										s = (DotProduct(arrays->svector[o], arrays->coord[o]) + br->faces[j].stdir[0][3]);
										t = (DotProduct(arrays->tvector[o], arrays->coord[o]) + br->faces[j].stdir[1][3]);
										arrays->texcoord[o][0] = s * scale[0];
										arrays->texcoord[o][1] = t * scale[1];

										//maths, maths, and more maths.
										arrays->lmcoord[o][0] = (br->faces[j].lmbase[0]+0.5 + s/br->faces[j].lmscale-br->faces[j].lmbias[0]) * hm->brushlmscale;
										arrays->lmcoord[o][1] = (br->faces[j].lmbase[1]+0.5 + t/br->faces[j].lmscale-br->faces[j].lmbias[1]) * hm->brushlmscale;
									}
									for (k = 2; k < br->faces[j].numpoints; k++)
									{	//triangle fans
										arrays->index[numindicies++] = numverts + 0;
										arrays->index[numindicies++] = numverts + k-1;
										arrays->index[numindicies++] = numverts + k-0;
									}
									numverts += br->faces[j].numpoints;
								}
							}
						}
					}

					if (numverts || numindicies)
					{
						bb = Z_Malloc(sizeof(*bb) + (sizeof(bb->mesh.xyz_array[0])+sizeof(arrays->texcoord[0])+sizeof(arrays->lmcoord[0])+sizeof(arrays->normal[0])+sizeof(arrays->svector[0])+sizeof(arrays->tvector[0])+sizeof(arrays->rgba[0])) * numverts);
						bb->next = bt->batches;
						bt->batches = bb;
						bb->lightmap = lmnum;
						BE_VBO_Begin(&ctx, (sizeof(arrays->coord[0])+sizeof(arrays->texcoord[0])+sizeof(arrays->lmcoord[0])+sizeof(arrays->normal[0])+sizeof(arrays->svector[0])+sizeof(arrays->tvector[0])+sizeof(arrays->rgba[0])) * numverts);
						BE_VBO_Data(&ctx, arrays->coord,	sizeof(arrays->coord	[0])*numverts,		&bb->vbo.coord);
						BE_VBO_Data(&ctx, arrays->texcoord, sizeof(arrays->texcoord	[0])*numverts,		&bb->vbo.texcoord);
						BE_VBO_Data(&ctx, arrays->lmcoord,	sizeof(arrays->lmcoord	[0])*numverts,		&bb->vbo.lmcoord[0]);
						BE_VBO_Data(&ctx, arrays->normal,	sizeof(arrays->normal	[0])*numverts,		&bb->vbo.normals);
						BE_VBO_Data(&ctx, arrays->svector,	sizeof(arrays->svector	[0])*numverts,		&bb->vbo.svector);
						BE_VBO_Data(&ctx, arrays->tvector,	sizeof(arrays->tvector	[0])*numverts,		&bb->vbo.tvector);
						BE_VBO_Data(&ctx, arrays->rgba,		sizeof(arrays->rgba		[0])*numverts,		&bb->vbo.colours[0]);
						BE_VBO_Finish(&ctx, arrays->index,	sizeof(arrays->index	[0])*numindicies,	&bb->vbo.indicies, &bb->vbo.vbomem, &bb->vbo.ebomem);

						bb->mesh.xyz_array = (vecV_t*)(bb+1);
						memcpy(bb->mesh.xyz_array, arrays->coord, sizeof(*bb->mesh.xyz_array) * numverts);
						bb->mesh.st_array = (vec2_t*)(bb->mesh.xyz_array+numverts);
						memcpy(bb->mesh.st_array, arrays->texcoord, sizeof(*bb->mesh.st_array) * numverts);
						bb->mesh.lmst_array[0] = (vec2_t*)(bb->mesh.st_array+numverts);
						memcpy(bb->mesh.lmst_array[0], arrays->lmcoord, sizeof(*bb->mesh.lmst_array) * numverts);
						bb->mesh.normals_array = (vec3_t*)(bb->mesh.lmst_array[0]+numverts);
						memcpy(bb->mesh.normals_array, arrays->normal, sizeof(*bb->mesh.normals_array) * numverts);
						bb->mesh.snormals_array = (vec3_t*)(bb->mesh.normals_array+numverts);
						memcpy(bb->mesh.snormals_array, arrays->svector, sizeof(*bb->mesh.snormals_array) * numverts);
						bb->mesh.tnormals_array = (vec3_t*)(bb->mesh.snormals_array+numverts);
						memcpy(bb->mesh.tnormals_array, arrays->tvector, sizeof(*bb->mesh.tnormals_array) * numverts);
						bb->mesh.colors4f_array[0] = (vec4_t*)(bb->mesh.tnormals_array+numverts);
						memcpy(bb->mesh.colors4f_array[0], arrays->rgba, sizeof(*bb->mesh.colors4f_array[0]) * numverts);

						bb->pmesh = &bb->mesh;
						bb->mesh.numindexes = numindicies;
						bb->mesh.numvertexes = numverts;

						numverts = 0;
						numindicies = 0;
					}
				}
			}
		}

		for(bb = bt->batches; bb; bb = bb->next)
		{
			b = BE_GetTempBatch();
			if (b)
			{
				j = 0;
				if (bb->lightmap >= 0)
					b->lightmap[j++] = r_fullbright.ival?-1:hm->brushlmremaps[bb->lightmap];
				for (; j < MAXRLIGHTMAPS; j++)
					b->lightmap[j] = -1;
				b->ent = e;
				b->shader = bt->shader;
				b->flags = 0;
				b->mesh = &bb->pmesh;
				b->meshes = 1;
				b->buildmeshes = NULL;
				b->skin = NULL;
				b->texture = NULL;
				b->vbo = &bb->vbo;

				b->next = batches[b->shader->sort];
				batches[b->shader->sort] = b;
			}
		}
	}
	if (arrays)
		BZ_Free(arrays);
}
#endif

static brushtex_t *Terr_Brush_FindTexture(heightmap_t *hm, const char *texname)
{
	brushtex_t *bt;
	if (!hm)
		return NULL;

	for (bt = hm->brushtextures; bt; bt = bt->next)
	{
		if (!strcmp(bt->shadername, texname))
			return bt;
	}
	bt = Z_Malloc(sizeof(*bt));
	bt->next = hm->brushtextures;
	hm->brushtextures = bt;
	Q_strncpyz(bt->shadername, texname, sizeof(bt->shadername));

	return bt;
}

static brushes_t *Terr_Brush_Insert(model_t *model, heightmap_t *hm, brushes_t *brush)
{
	vecV_t facepoints[64];
	unsigned int iface, oface, j, k;
	unsigned int numpoints;
	brushes_t *out;
	vec2_t mins, maxs;
	vec2_t lm;

	if (!hm)
	{
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (model && model->loadstate == MLS_LOADED)
		{
			char basename[MAX_QPATH];
			COM_FileBase(model->name, basename, sizeof(basename));
			model->terrain = Mod_LoadTerrainInfo(model, basename, true);
			hm = model->terrain;
			if (!hm)
				return NULL;
			Terr_FinishTerrain(model);
		}
		else
			return NULL;
	}

	hm->wbrushes = BZ_Realloc(hm->wbrushes, sizeof(*hm->wbrushes) * (hm->numbrushes+1));
	out = &hm->wbrushes[hm->numbrushes];
	out->selected = false;
	out->contents = brush->contents;
	out->axialplanes = 0;
	out->patch = NULL;

	out->planes = NULL;
	out->faces = NULL;
	out->numplanes = 0;
	out->ispatch = !!brush->patch;
	out->selected = false;
	ClearBounds(out->mins, out->maxs);
	if (brush->numplanes)
	{
		out->planes = BZ_Malloc((sizeof(*out->planes)+sizeof(*out->faces)) * brush->numplanes);
		out->faces = (void*)(out->planes+brush->numplanes);
		for (iface = 0, oface = 0; iface < brush->numplanes; iface++)
		{
			for (j = 0; j < oface; j++)
			{
				if (out->planes[j][0] == brush->planes[iface][0] &&
					out->planes[j][1] == brush->planes[iface][1] &&
					out->planes[j][2] == brush->planes[iface][2] &&
					out->planes[j][3] == brush->planes[iface][3])
					break;
			}
			if (j < oface)
			{
				Con_DPrintf("duplicate plane\n");
				continue;
			}

			//generate points now (so we know the correct mins+maxs for the brush, and whether the plane is relevent)
			numpoints = Fragment_ClipPlaneToBrush(facepoints, countof(facepoints), brush->planes, sizeof(*brush->planes), brush->numplanes, brush->planes[iface]);
			if (!numpoints)
			{
				Con_DPrintf("redundant face\n");
				continue;	//this surface was chopped away entirely, and isn't relevant.
			}

			//copy the basic face info out so we can save/restore/query/edit it later.
			Vector4Copy(brush->planes[iface], out->planes[oface]);
			out->faces[oface].tex = brush->faces[iface].tex;
			Vector4Copy(brush->faces[iface].stdir[0], out->faces[oface].stdir[0]);
			Vector4Copy(brush->faces[iface].stdir[1], out->faces[oface].stdir[1]);

			if (out->planes[oface][0] == 1)
				out->axialplanes |= 1u<<0;
			else if (out->planes[oface][1] == 1)
				out->axialplanes |= 1u<<1;
			else if (out->planes[oface][2] == 1)
				out->axialplanes |= 1u<<2;
			else if (out->planes[oface][0] == -1)
				out->axialplanes |= 1u<<3;
			else if (out->planes[oface][1] == -1)
				out->axialplanes |= 1u<<4;
			else if (out->planes[oface][2] == -1)
				out->axialplanes |= 1u<<5;

			//make sure this stuff is rebuilt properly.
			out->faces[oface].tex->rebuild = true;

			//keep this stuff cached+reused, so everything is consistant. also work out min/max lightmap texture coords
			out->faces[oface].points = BZ_Malloc(numpoints * sizeof(*out->faces[oface].points));
			Vector2Set(mins, 0, 0);
			Vector2Set(maxs, 0, 0);
			for (j = 0; j < numpoints; j++)
			{
				AddPointToBounds(facepoints[j], out->mins, out->maxs);
				VectorCopy(facepoints[j], out->faces[oface].points[j]);
				for (k = 0; k < 2; k++)
				{
					lm[k] = DotProduct(out->faces[oface].points[j], out->faces[oface].stdir[k]) + out->faces[oface].stdir[k][3];
					if (j == 0)
						mins[k] = maxs[k] = lm[k];
					else if (lm[k] > maxs[k])
						maxs[k] = lm[k];
					else if (lm[k] < mins[k])
						mins[k] = lm[k];
				}
			}
			out->faces[oface].numpoints = numpoints;

			//determine lightmap scale, and extents. rescale the lightmap if it ought to have been subdivided.
			out->faces[oface].relight = true;
			out->faces[oface].lmscale = 16;
			for (k = 0; k < 2; )
			{
				out->faces[oface].lmbias[k] = floor(mins[k]/out->faces[oface].lmscale);
				out->faces[oface].lmextents[k] = ceil((maxs[k])/out->faces[oface].lmscale)-out->faces[oface].lmbias[k]+1;
				if (out->faces[oface].lmextents[k] > 128)
				{	//surface is too large for lightmap data. just drop its resolution, because splitting the face in plane-defined geometry is a bad idea.
					if (out->faces[oface].lmscale > 256)
					{
						out->faces[oface].relight = false;
						k++;
					}
					else
					{
						out->faces[oface].lmscale *= 2;
						k = 0;
					}
				}
				else
					k++;
			}
			out->faces[oface].lightmap = -1;
			out->faces[oface].lmbase[0] = 0;
			out->faces[oface].lmbase[1] = 0;
			if (out->faces[oface].relight)
			{
				out->faces[oface].lightdata = BZ_Malloc(out->faces[oface].lmextents[0] * out->faces[oface].lmextents[1] * 3);
				memset(out->faces[oface].lightdata, 0x3f, out->faces[oface].lmextents[0]*out->faces[oface].lmextents[1]*3);
			}
			else
				out->faces[oface].lightdata = NULL;

	//		Con_Printf("lm extents: %u %u (%i points)\n", out->faces[oface].lmextents[0], out->faces[oface].lmextents[1], numpoints);
			oface++;
		}
		out->numplanes = oface;
	}

	if (brush->patch)
	{
		out->patch = BZ_Malloc(sizeof(*out->patch)-sizeof(out->patch->cp) + sizeof(*out->patch->cp)*brush->patch->numcp[0]*brush->patch->numcp[1]);
		memcpy(out->patch, brush->patch, sizeof(*out->patch)-sizeof(out->patch->cp) + sizeof(*out->patch->cp)*brush->patch->numcp[0]*brush->patch->numcp[1]);

		numpoints = out->patch->numcp[0]*out->patch->numcp[1];
		//FIXME: lightmap...
		for (j = 0; j < numpoints; j++)
			AddPointToBounds(out->patch->cp[j].v, out->mins, out->maxs);


		out->patch->tex->rebuild = true;
	}

	if ((out->numplanes < 4 && out->numplanes) || (out->numplanes && out->patch) || (!out->numplanes && !out->patch))
	{	//a brush with less than 4 planes cannot be a valid convex area (but can happen when certain redundant planes are chopped out). don't accept creation
		//(we often get 2-plane brushes if the sides are sucked in)
		for (j = 0; j < out->numplanes; j++)
		{
			BZ_Free(out->faces[j].lightdata);
			BZ_Free(out->faces[j].points);
		}
		BZ_Free(out->planes);
		BZ_Free(out->patch);
		return NULL;
	}

	if (brush->id)
		out->id = brush->id;
	else
	{
		unsigned int i;
		//loop to avoid creating two brushes with the same id
		do
		{
			out->id = (++hm->brushidseq)&0x00ffffff;
#ifdef HAVE_CLIENT
			if (cls.state)	//avoid networking conflicts by having each node generating its own private ids
				out->id |= (cl.playerview[0].playernum+1)<<24;
#endif

			for (i = 0; i < hm->numbrushes; i++)
			{
				if (hm->wbrushes[i].id == out->id)
					break;
			}
		} while (i != hm->numbrushes);
	}
//	Con_Printf("brush %u (%i faces)\n", out->id, oface);

	hm->numbrushes+=1;
	hm->brushesedited = true;

	hm->recalculatebrushlighting = true;	//lightmaps need to be reallocated

	//make sure the brush's bounds are added to the containing model.
	AddPointToBounds(out->mins, model->mins, model->maxs);
	AddPointToBounds(out->maxs, model->mins, model->maxs);

	if (out->patch && (out->patch->subdiv[0] || out->patch->subdiv[1]))
		out->patch->tessvert = PatchInfo_Evaluate(out->patch->cp, out->patch->numcp, out->patch->subdiv, out->patch->tesssize);

	return out;
}


static brushes_t *Terr_Patch_Insert(model_t *model, heightmap_t *hm, brushtex_t *patch_tex, unsigned patch_w, unsigned patch_h, unsigned subdiv_w, unsigned subdiv_h, qcpatchvert_t *patch_v, int stride)
{
	int x, y;
	brushes_t brush;
	//finish the brush
	brush.contents = FTECONTENTS_SOLID;
	brush.numplanes = 0;
	brush.planes = NULL;
	brush.faces = NULL;
	brush.id = 0;
	brush.patch = alloca(sizeof(*brush.patch)-sizeof(brush.patch->cp) + sizeof(*brush.patch->cp)*patch_w*patch_h);

	brush.patch->tex = patch_tex;
	brush.patch->numcp[0] = patch_w;
	brush.patch->numcp[1] = patch_h;
	brush.patch->subdiv[0] = subdiv_w;
	brush.patch->subdiv[1] = subdiv_h;
	brush.patch->tessvert = NULL;

	for (y = 0; y < patch_h; y++)
	{
		for (x = 0; x < patch_w; x++)
		{
			VectorCopy(patch_v[x].v, brush.patch->cp[x + y*patch_w].v);
			Vector2Copy(patch_v[x].tc, brush.patch->cp[x + y*patch_w].tc);
			Vector4Copy(patch_v[x].rgba, brush.patch->cp[x + y*patch_w].rgba);
			//brush.patch->verts[x + y*patch_w].norm
			//brush.patch->verts[x + y*patch_w].sdir
			//brush.patch->verts[x + y*patch_w].tdir
		}
		patch_v += stride;
	}

	return Terr_Brush_Insert(model, hm, &brush);
}

static void Terr_Brush_DeleteIdx(heightmap_t *hm, size_t idx)
{
	int i;
	brushes_t *br = &hm->wbrushes[idx];
	if (!hm)
		return;

	for (i = 0; i < br->numplanes; i++)
	{
		BZ_Free(br->faces[i].lightdata);
		BZ_Free(br->faces[i].points);
		br->faces[i].tex->rebuild = true;
	}

	BZ_Free(br->planes);
	if (br->patch)
	{
		BZ_Free(br->patch->tessvert);
		BZ_Free(br->patch);
	}
	hm->numbrushes--;
	hm->brushesedited = true;
	//plug the hole with some other brush.
	if (idx < hm->numbrushes)
		hm->wbrushes[idx] = hm->wbrushes[hm->numbrushes];
}
static qboolean Terr_Brush_DeleteId(heightmap_t *hm, unsigned int brushid)
{
	size_t i;
	brushes_t *br;
	if (!hm)
		return false;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == brushid)
		{
			Terr_Brush_DeleteIdx(hm, i);
			return true;
		}
	}
	return false;
}


static void Patch_Serialise(sizebuf_t *sb, brushes_t *br)
{
	qbyte flags = 0;
	unsigned int i, m = br->patch->numcp[0]*br->patch->numcp[1];

	for (i = 0; i < m; i++)
	{
		if (br->patch->cp[i].rgba[0] != 1)
			flags |= 1;
		if (br->patch->cp[i].rgba[1] != 1)
			flags |= 2;
		if (br->patch->cp[i].rgba[2] != 1)
			flags |= 4;
		if (br->patch->cp[i].rgba[3] != 1)
			flags |= 8;
	}

	MSG_WriteLong(sb, br->id);
	MSG_WriteLong(sb, br->contents);
	MSG_WriteShort(sb, br->patch->numcp[0]);
	MSG_WriteShort(sb, br->patch->numcp[1]);

	MSG_WriteByte(sb, flags);

	MSG_WriteString(sb, br->patch->tex->shadername);
	MSG_WriteShort(sb, br->patch->subdiv[0]);
	MSG_WriteShort(sb, br->patch->subdiv[1]);

	for (i = 0; i < m; i++)
	{
		MSG_WriteFloat(sb, br->patch->cp[i].v[0]);
		MSG_WriteFloat(sb, br->patch->cp[i].v[1]);
		MSG_WriteFloat(sb, br->patch->cp[i].v[2]);
		MSG_WriteFloat(sb, br->patch->cp[i].tc[0]);
		MSG_WriteFloat(sb, br->patch->cp[i].tc[1]);

		if (flags&1)
			MSG_WriteFloat(sb, br->patch->cp[i].rgba[0]);
		if (flags&2)
			MSG_WriteFloat(sb, br->patch->cp[i].rgba[1]);
		if (flags&4)
			MSG_WriteFloat(sb, br->patch->cp[i].rgba[2]);
		if (flags&8)
			MSG_WriteFloat(sb, br->patch->cp[i].rgba[3]);
	}
}
static size_t Patch_DeserialiseHeader(brushes_t *br)
{
	unsigned int numcp[2];
	br->id = MSG_ReadLong();
	br->contents = MSG_ReadLong();

	br->numplanes   = numcp[0] = (unsigned short)MSG_ReadShort();
	br->axialplanes = numcp[1] = (unsigned short)MSG_ReadShort();

	if (numcp[0]*numcp[1] > 8192)
		return 0; //too many. reject it as bad.
	return sizeof(*br->patch) + sizeof(*br->patch->cp)*(numcp[0]*numcp[1]-countof(br->patch->cp));
}
static qboolean Patch_Deserialise(heightmap_t *hm, brushes_t *br, void *mem)
{
	struct qcpatchvert_s vert;
	qboolean flags;
	unsigned int i, m;
	flags = MSG_ReadByte();

	br->patch = mem;
	br->patch->numcp[0] = br->numplanes;
	br->patch->numcp[1] = br->axialplanes;
	br->numplanes = br->axialplanes = 0;

	m = br->patch->numcp[0]*br->patch->numcp[1];

	//FIXME: as a server, we probably want to reject the brush if we exceed some texnum/memory limitation, so clients can't just spam new textures endlessly.
	br->patch->tex = Terr_Brush_FindTexture(hm, MSG_ReadString());

	br->patch->subdiv[0] = MSG_ReadShort();
	br->patch->subdiv[1] = MSG_ReadShort();

	for (i = 0; i < m; i++)
	{
		vert.v[0] = MSG_ReadFloat();
		vert.v[1] = MSG_ReadFloat();
		vert.v[2] = MSG_ReadFloat();
		vert.tc[0] = MSG_ReadFloat();
		vert.tc[1] = MSG_ReadFloat();

		vert.rgba[0] = (flags&1)?MSG_ReadFloat():1;
		vert.rgba[1] = (flags&2)?MSG_ReadFloat():1;
		vert.rgba[2] = (flags&4)?MSG_ReadFloat():1;
		vert.rgba[3] = (flags&8)?MSG_ReadFloat():1;

		br->patch->cp[i] = vert;
	}
	return true;
}

static void Brush_Serialise(sizebuf_t *sb, brushes_t *br)
{
	unsigned int i;
	MSG_WriteLong(sb, br->id);
	MSG_WriteLong(sb, br->contents);
	MSG_WriteLong(sb, br->numplanes);

	for (i = 0; i < br->numplanes; i++)
	{
		MSG_WriteString(sb, br->faces[i].tex->shadername);

		MSG_WriteFloat(sb, br->planes[i][0]);
		MSG_WriteFloat(sb, br->planes[i][1]);
		MSG_WriteFloat(sb, br->planes[i][2]);
		MSG_WriteFloat(sb, br->planes[i][3]);

		MSG_WriteFloat(sb, br->faces[i].stdir[0][0]);
		MSG_WriteFloat(sb, br->faces[i].stdir[0][1]);
		MSG_WriteFloat(sb, br->faces[i].stdir[0][2]);
		MSG_WriteFloat(sb, br->faces[i].stdir[0][3]);

		MSG_WriteFloat(sb, br->faces[i].stdir[1][0]);
		MSG_WriteFloat(sb, br->faces[i].stdir[1][1]);
		MSG_WriteFloat(sb, br->faces[i].stdir[1][2]);
		MSG_WriteFloat(sb, br->faces[i].stdir[1][3]);
	}
}
static size_t Brush_DeserialiseHeader(brushes_t *br, qboolean ispatch)
{
	br->ispatch = ispatch;
	if (br->ispatch)
		return Patch_DeserialiseHeader(br);

	br->id = MSG_ReadLong();
	br->contents = MSG_ReadLong();
	br->numplanes = MSG_ReadLong();

	if (br->numplanes > 8192)
		return 0;	//abusive

	return sizeof(*br->faces)  * br->numplanes
		 + sizeof(*br->planes) * br->numplanes;
}
static qboolean Brush_Deserialise(heightmap_t *hm, brushes_t *br, void *mem)
{
	unsigned int i;
	if (br->ispatch)
		return Patch_Deserialise(hm, br, mem);

	br->faces = mem;
	br->planes = (vec4_t*)(br->faces + br->numplanes);

	for (i = 0; i < br->numplanes; i++)
	{
		//FIXME: as a server, we probably want to reject the brush if we exceed some texnum/memory limitation, so clients can't just spam new textures endlessly.
		br->faces[i].tex = Terr_Brush_FindTexture(hm, MSG_ReadString());

		br->planes[i][0] = MSG_ReadFloat();
		br->planes[i][1] = MSG_ReadFloat();
		br->planes[i][2] = MSG_ReadFloat();
		br->planes[i][3] = MSG_ReadFloat();

		//FIXME: can we optimise this part? a flag to say whether its needed?
		br->faces[i].stdir[0][0] = MSG_ReadFloat();
		br->faces[i].stdir[0][1] = MSG_ReadFloat();
		br->faces[i].stdir[0][2] = MSG_ReadFloat();
		br->faces[i].stdir[0][3] = MSG_ReadFloat();

		br->faces[i].stdir[1][0] = MSG_ReadFloat();
		br->faces[i].stdir[1][1] = MSG_ReadFloat();
		br->faces[i].stdir[1][2] = MSG_ReadFloat();
		br->faces[i].stdir[1][3] = MSG_ReadFloat();

		br->faces[i].surfaceflags = 0;	//used by q2
		br->faces[i].surfacevalue = 0;	//used by q2 (generally light levels)
	}
	return true;
}


#ifdef HAVE_CLIENT
heightmap_t	*CL_BrushEdit_ForceContext(model_t *mod)
{
	heightmap_t *hm = mod?mod->terrain:NULL;
	if (!hm)
	{
		if (mod && mod->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
		if (mod && mod->loadstate == MLS_LOADED)
		{
			char basename[MAX_QPATH];
			COM_FileBase(mod->name, basename, sizeof(basename));
			mod->terrain = Mod_LoadTerrainInfo(mod, basename, true);
			hm = mod->terrain;
			if (!hm)
				return NULL;
			Terr_FinishTerrain(mod);
		}
		else
			return NULL;
	}
	return hm;
}

void CL_Parse_BrushEdit(void)
{
	unsigned int	modelindex		= MSG_ReadShort();
	int				cmd				= MSG_ReadByte();

	model_t			*mod			= (modelindex<countof(cl.model_precache))?cl.model_precache[modelindex]:NULL;
	heightmap_t		*hm				= mod?mod->terrain:NULL;

#ifdef HAVE_SERVER
	const qboolean		ignore = (sv_state>=ss_loading);	//if we're the server then we already have this info. don't break anything (this info is present for demos).
#else
	const qboolean		ignore = false;
#endif

	if (cmd == hmcmd_brush_delete)
	{
		int id = MSG_ReadLong();
		if (ignore)
			return;	//ignore if we're the server, we should already have it anyway.
		Terr_Brush_DeleteId(hm, id);
	}
	else if (cmd == hmcmd_brush_insert || cmd == hmcmd_patch_insert)	//1=create/replace
	{
		brushes_t brush;
		size_t tempmemsize;

		hm = CL_BrushEdit_ForceContext(mod);	//do this early, to ensure that the textures are correct

		memset(&brush, 0, sizeof(brush));
		tempmemsize = Brush_DeserialiseHeader(&brush, (cmd == hmcmd_patch_insert));
		if (!tempmemsize)
			Host_EndGame("CL_Parse_BrushEdit: unparsable %s\n", brush.ispatch?"patch":"brush");
		if (!Brush_Deserialise(hm, &brush, alloca(tempmemsize)))
			Host_EndGame("CL_Parse_BrushEdit: unparsable %s\n", brush.ispatch?"patch":"brush");

		if (!ignore)	//ignore if we're the server, we should already have it anyway (but might need it for demos, hence why its still sent).
		{
			if (brush.id)
			{
				int i;
				if (cls.demoplayback)
					Terr_Brush_DeleteId(hm, brush.id);
				else
				{
					for (i = 0; i < hm->numbrushes; i++)
					{
						brushes_t *br = &hm->wbrushes[i];
						if (br->id == brush.id)
							return;	//we already have it. assume we just edited it.
					}
				}
			}
			Terr_Brush_Insert(mod, hm, &brush);
		}
	}
	else if (cmd == hmcmd_prespawning)
	{	//delete all
		if (ignore)
			return;	//ignore if we're the server, we should already have it anyway.

		hm = CL_BrushEdit_ForceContext(mod);	//make sure we don't end up with any loaded brushes.
		if (hm)
		{
			while(hm->numbrushes)
				Terr_Brush_DeleteIdx(hm, hm->numbrushes-1);
		}
	}
	else if (cmd == hmcmd_prespawned)
	{
	}
	else if (cmd == hmcmd_ent_edit || cmd == hmcmd_ent_remove)
	{	//ent edit
		int id = MSG_ReadLong();
		const char *data;
		int idx = mod->numentityinfo, i;
		if (cmd == hmcmd_ent_edit)
			data = MSG_ReadString();
		else
			data = NULL;

		//convert id to idx
		for (i = 0; i < mod->numentityinfo; i++)
		{
			if (mod->entityinfo[i].id == id)
			{
				idx = i;
				break;
			}
			if (!mod->entityinfo[i].keyvals)
				idx = i;
		}

		//FIXME: cap the maximum data sizes (both count and storage, to prevent DOS attacks).

		if (idx == mod->numentityinfo && data)
			Z_ReallocElements((void**)&mod->entityinfo, &mod->numentityinfo, mod->numentityinfo+64, sizeof(*mod->entityinfo));
		if (idx < mod->numentityinfo)
		{
			if (!ignore)
			{
				mod->entityinfo[idx].id = id;
				Z_Free(mod->entityinfo[idx].keyvals);
				if (data)
					mod->entityinfo[idx].keyvals = Z_StrDup(data);
				else
					mod->entityinfo[idx].keyvals = NULL;

#ifdef CSQC_DAT
				CSQC_MapEntityEdited(modelindex, idx, data);
#endif
			}
		}
	}
	else
		Host_EndGame("CL_Parse_BrushEdit: unknown command %i\n", cmd);
}
#endif
#ifdef HAVE_SERVER
qboolean SV_Prespawn_Brushes(sizebuf_t *msg, unsigned int *modelindex, unsigned int *lastid)
{
	//lastid starts at 0
	unsigned int bestid, i;
	brushes_t *best;
	model_t *mod;
	heightmap_t *hm;
	while(1)
	{
		if (*modelindex < MAX_PRECACHE_MODELS)
			mod = sv.models[*modelindex];
		else
			mod = NULL;
		if (!mod)
		{
			if (!(*modelindex)++)
				continue;
			return false;
		}
		hm = mod->terrain;
		if (!hm || !hm->brushesedited)
		{
			*modelindex+=1;
			*lastid = 0;
			continue;
		}

		if (!*lastid)
		{	//make sure the client starts with a clean slate.
			MSG_WriteByte(msg, svcfte_brushedit);
			MSG_WriteShort(msg, *modelindex);
			MSG_WriteByte(msg, hmcmd_prespawning);
		}

		//weird loop to try to ensure we never miss any brushes.
		//get the lowest index that is 1 higher than our previous.
		for (best = NULL, bestid = ~0u, i = 0; i < hm->numbrushes; i++)
		{
			unsigned int bid = hm->wbrushes[i].id;
			if (bid > *lastid && bid <= bestid)
			{
				best = &hm->wbrushes[i];
				bestid = best->id;
				if (bestid == *lastid+1)
					break;
			}
		}

		if (best)
		{
			MSG_WriteByte(msg, svcfte_brushedit);
			MSG_WriteShort(msg, *modelindex);
			if (best->patch)
			{
				MSG_WriteByte(msg, hmcmd_patch_insert);
				Patch_Serialise(msg, best);
			}
			else
			{
				MSG_WriteByte(msg, hmcmd_brush_insert);
				Brush_Serialise(msg, best);
			}
			*lastid = bestid;
			return true;
		}
		
		*modelindex+=1;
		*lastid = 0;
	}
}
qboolean SV_Parse_BrushEdit(void)
{
	qboolean authorise = (host_client->penalties & BAN_MAPPER) || (host_client->netchan.remote_address.type == NA_LOOPBACK);
	unsigned int	modelindex		= MSG_ReadShort();
	int				cmd				= MSG_ReadByte();
	model_t			*mod			= (modelindex<countof(sv.models))?sv.models[modelindex]:NULL;
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	if (cmd == hmcmd_brush_delete)
	{	//delete
		unsigned int brushid = MSG_ReadLong();
		if (!authorise)
		{
			SV_PrintToClient(host_client, PRINT_MEDIUM, "Brush editing ignored: you are not a mapper\n");
			return true;
		}
		Terr_Brush_DeleteId(hm, brushid);

		MSG_WriteByte(&sv.multicast, svcfte_brushedit);
		MSG_WriteShort(&sv.multicast, modelindex);
		MSG_WriteByte(&sv.multicast, hmcmd_brush_delete);
		MSG_WriteLong(&sv.multicast, brushid);
		SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
		return true;
	}
	else if (cmd == hmcmd_brush_insert || cmd == hmcmd_patch_insert)
	{
		brushes_t brush;
		size_t tempmemsize;
		memset(&brush, 0, sizeof(brush));
		brush.ispatch = (cmd == hmcmd_patch_insert);

		tempmemsize = Brush_DeserialiseHeader(&brush, cmd == hmcmd_patch_insert);
		if (!tempmemsize)
		{
			Con_Printf("SV_Parse_BrushEdit: %s sent an abusive %s\n", host_client->name, brush.ispatch?"patch":"brush");
			return false;
		}
		if (!Brush_Deserialise(hm, &brush, alloca(tempmemsize)))
		{
			Con_Printf("SV_Parse_BrushEdit: %s sent an unparsable brush\n", host_client->name);
			return false;
		}

		if (!authorise)
		{
			SV_PrintToClient(host_client, PRINT_MEDIUM, "Brush editing ignored: you are not a mapper\n");
			return true;
		}

		Terr_Brush_DeleteId(hm, brush.id);
		if (!Terr_Brush_Insert(mod, hm, &brush))
			return true;	//looks mostly valid, but something was degenerate. fpu precision...

		//FIXME: expand the world entity's sizes if needed?

		MSG_WriteByte(&sv.multicast, svcfte_brushedit);
		MSG_WriteShort(&sv.multicast, modelindex);
		MSG_WriteByte(&sv.multicast, cmd);
		if (cmd == hmcmd_patch_insert)
			Patch_Serialise(&sv.multicast, &brush);
		else
			Brush_Serialise(&sv.multicast, &brush);
		SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
		return true;
	}
	else if (cmd == hmcmd_ent_edit || cmd == hmcmd_ent_remove)
	{
		unsigned int entid = MSG_ReadLong();
		char *keyvals = (cmd == hmcmd_ent_edit)?MSG_ReadString():NULL;
		if (mod->submodelof != mod)
			return true;
		if (!authorise)
		{
			SV_PrintToClient(host_client, PRINT_MEDIUM, "Entity editing ignored: you are not a mapper\n");
			//FIXME: undo the client's edit? or is that rude?
			return true;
		}

		//FIXME: need to update the server's entity list
		//SSQC_MapEntityEdited(idx, newvals);

		MSG_WriteByte(&sv.multicast, svcfte_brushedit);
		MSG_WriteShort(&sv.multicast, modelindex);
		MSG_WriteByte(&sv.multicast, keyvals?hmcmd_ent_edit:hmcmd_ent_remove);
		MSG_WriteLong(&sv.multicast, entid);
		if (keyvals)
			MSG_WriteString(&sv.multicast, keyvals);
		SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
	}
	else
	{
		Con_Printf("SV_Parse_BrushEdit: %s sent an unknown command: %i\n", host_client->name, cmd);
		return false;
	}

	return true;
}
#endif

static void *validateqcpointer(pubprogfuncs_t *prinst, size_t qcptr, size_t elementsize, size_t elementcount, qboolean allownull)
{
	//make sure that the sizes can't overflow
	if (elementcount > 0x10000)
	{
		PR_BIError(prinst, "brush: elementcount %u is too large\n", (unsigned int)elementcount);
		return NULL;
	}
	if (qcptr+(elementsize*elementcount) > (size_t)prinst->stringtablesize)
	{
		PR_BIError(prinst, "brush: invalid qc pointer\n");
		return NULL;
	}
	if (!qcptr)
	{
		if (!allownull)
			PR_BIError(prinst, "brush: null qc pointer\n");
		return NULL;
	}
	return prinst->stringtable + qcptr;
}

//	{"patch_getcp",		PF_patch_getcp,		0,		0,		0,		0,		D(qcpatchvert "int(float modelidx, int patchid, patchvert_t *out_controlverts, int maxcp, __out patchinfo_t out_info)", "Queries a patch's information. You must pre-allocate the face array for the builtin to write to. Return value is the total number of control verts that were retrieved, 0 on error.")},
void QCBUILTIN PF_patch_getcp(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t			*vmw			= prinst->parms->user;
	model_t			*mod			= vmw->Get_CModel(vmw, G_FLOAT(OFS_PARM0));
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	patchid			= G_INT(OFS_PARM1);
	unsigned int	maxverts		= G_INT(OFS_PARM3);
	qcpatchvert_t	*out_verts		= validateqcpointer(prinst, G_INT(OFS_PARM2), sizeof(*out_verts), maxverts, true);
	qcpatchinfo_t	*out_info		= validateqcpointer(prinst, G_INT(OFS_PARM4), sizeof(*out_info), 1, true);
	unsigned int	i, j;
	brushes_t		*br;

	//assume the worst.
	G_INT(OFS_RETURN) = 0;
	if (out_info)
		memset(out_info, 0, sizeof(*out_info));

	if (!hm)
		return;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == patchid)
		{
			if (!br->patch)
				return;
			if (out_info)
			{
				out_info->contents = br->contents;
				out_info->cp_width = br->patch->numcp[0];
				out_info->cp_height = br->patch->numcp[1];
				out_info->subdiv_x = br->patch->subdiv[0];
				out_info->subdiv_y = br->patch->subdiv[1];
				out_info->shadername = PR_TempString(prinst, br->patch->tex->shadername);
			}

			if (!out_verts)
				G_INT(OFS_RETURN) = br->patch->numcp[0]*br->patch->numcp[1];
			else
			{
				maxverts = min(br->patch->numcp[0]*br->patch->numcp[1], maxverts);

				for (j = 0; j < maxverts; j++)
				{
					VectorCopy(br->patch->cp[j].v, out_verts->v);
					Vector2Copy(br->patch->cp[j].tc, out_verts->tc);
					Vector4Copy(br->patch->cp[j].rgba, out_verts->rgba);

					out_verts++;
				}
				G_INT(OFS_RETURN) = j;
			}
			return;
		}
	}
}
//  {"patch_evaluate",	PF_patch_evaluate,	0,		0,		0,		0,		D("int(patchvert_t *in_controlverts, patchvert_t *out_renderverts, int maxout, patchinfo_t *inout_info)", "Calculates the geometry of a hyperthetical patch.")},
void QCBUILTIN PF_patch_evaluate(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qcpatchinfo_t	*inout_info		= validateqcpointer(prinst, G_INT(OFS_PARM3), sizeof(*inout_info), 1, false);
	unsigned int	maxverts		= G_INT(OFS_PARM2);
	qcpatchvert_t	*out_verts		= validateqcpointer(prinst, G_INT(OFS_PARM1), sizeof(*out_verts), maxverts, true);
	qcpatchvert_t	*in_cp			= validateqcpointer(prinst, G_INT(OFS_PARM0), sizeof(*in_cp), inout_info->cp_width*inout_info->cp_height, false);

	unsigned short numcp[] = {inout_info->cp_width, inout_info->cp_height}, size[2];
	short subdiv[] = {inout_info->subdiv_x, inout_info->subdiv_y};
	patchtessvert_t	*working_verts	= PatchInfo_Evaluate(in_cp, numcp, subdiv, size);

	unsigned int i;
	if (working_verts)
	{
		if (out_verts)
		{
			maxverts = min(maxverts, size[0]*size[1]);
			for (i = 0; i < maxverts; i++)
			{
				VectorCopy(working_verts[i].v, out_verts[i].v);
				Vector4Copy(working_verts[i].rgba, out_verts[i].rgba);
				Vector2Copy(working_verts[i].tc, out_verts[i].tc);
			}
		}
		BZ_Free(working_verts);

		inout_info->cp_width = size[0];	//not really controlpoints, but the data works the same.
		inout_info->cp_height = size[1];
	}
	else
		inout_info->cp_width = inout_info->cp_height = 0; //erk...
	inout_info->subdiv_x = inout_info->subdiv_y = 0;	//make it as explicit tessellation, so we can maybe skip this.

	G_INT(OFS_RETURN) = maxverts;
}
//	{"patch_getmesh",	PF_patch_getmesh,	0,		0,		0,		0,		D("int(float modelidx, int patchid, patchvert_t *out_verts, int maxverts, patchinfo_t *out_info)", "Queries a patch's information. You must pre-allocate the face array for the builtin to write to. Return value is the total number of control verts that were retrieved, 0 on error.")},
void QCBUILTIN PF_patch_getmesh(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t			*vmw			= prinst->parms->user;
	model_t			*mod			= vmw->Get_CModel(vmw, G_FLOAT(OFS_PARM0));
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	patchid			= G_INT(OFS_PARM1);
	unsigned int	maxverts		= G_INT(OFS_PARM3);
	qcpatchvert_t	*out_verts		= validateqcpointer(prinst, G_INT(OFS_PARM2), sizeof(*out_verts), maxverts, true);
	qcpatchinfo_t	*out_info		= validateqcpointer(prinst, G_INT(OFS_PARM4), sizeof(*out_info), 1, true);
	unsigned int	i, j;
	brushes_t		*br;

	//assume the worst.
	G_INT(OFS_RETURN) = 0;
	if (out_info)
		memset(out_info, 0, sizeof(*out_info));

	if (!hm)
		return;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == patchid)
		{
			if (!br->patch)
				return;
			if (out_info)
			{
				out_info->contents = br->contents;
				out_info->cp_width = br->patch->tesssize[0];
				out_info->cp_height = br->patch->tesssize[1];
				out_info->subdiv_x = 0;
				out_info->subdiv_y = 0;
				out_info->shadername = PR_TempString(prinst, br->patch->tex->shadername);
			}

			if (!out_verts)
				G_INT(OFS_RETURN) = br->patch->tesssize[0]*br->patch->tesssize[1];
			else
			{
				maxverts = min(br->patch->tesssize[0]*br->patch->tesssize[1], maxverts);

				for (j = 0; j < maxverts; j++)
				{
					VectorCopy(br->patch->tessvert[j].v, out_verts->v);
					Vector2Copy(br->patch->tessvert[j].tc, out_verts->tc);
					Vector4Copy(br->patch->tessvert[j].rgba, out_verts->rgba);

					out_verts++;
				}
				G_INT(OFS_RETURN) = j;
			}
			return;
		}
	}
}

//	{"brush_get",		PF_brush_get,		0,		0,		0,		0,		D(qcbrushface "int(float modelidx, int brushid, brushface_t *out_faces, int maxfaces, int *out_contents)", "Queries a brush's information. You must pre-allocate the face array for the builtin to write to. Return value is the number of faces retrieved, 0 on error.")},
void QCBUILTIN PF_brush_get(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t			*vmw			= prinst->parms->user;
	model_t			*mod			= vmw->Get_CModel(vmw, G_FLOAT(OFS_PARM0));
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	brushid			= G_INT(OFS_PARM1);
	unsigned int	maxfaces		= G_INT(OFS_PARM3);
	qcbrushface_t	*out_faces		= validateqcpointer(prinst, G_INT(OFS_PARM2), sizeof(*out_faces), maxfaces, true);
	unsigned int	*out_contents	= validateqcpointer(prinst, G_INT(OFS_PARM4), sizeof(*out_contents), 1, true);
	unsigned int	fa, i;
	brushes_t		*br;
	
	//assume the worst.
	G_INT(OFS_RETURN) = 0;
	if (out_contents)
		*out_contents = 0;

	if (!hm)
		return;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == brushid)
		{
			if (br->patch)
				return;
			if (out_contents)
				*out_contents = br->contents;
			if (!out_faces)
				G_INT(OFS_RETURN) = br->numplanes;
			else
			{
				maxfaces = min(br->numplanes, maxfaces);

				for (fa = 0; fa < maxfaces; fa++)
				{
					out_faces->shadername = PR_TempString(prinst, br->faces[fa].tex->shadername);
					VectorCopy(br->planes[fa], out_faces->planenormal);
					out_faces->planedist = br->planes[fa][3];

					VectorCopy(br->faces[fa].stdir[0], out_faces->sdir);
					out_faces->sbias = br->faces[fa].stdir[0][3];
					VectorCopy(br->faces[fa].stdir[1], out_faces->tdir);
					out_faces->tbias = br->faces[fa].stdir[1][3];

					out_faces++;
				}
				G_INT(OFS_RETURN) = fa;
			}
			return;
		}
	}
}
//	{"brush_create",	PF_brush_create,	0,		0,		0,		0,		D("int(float modelidx, brushface_t *in_faces, int numfaces, int contents, optional int prevbrushid=0)", "Inserts a new brush into the model. Return value is the new brush's id.")},
void QCBUILTIN PF_brush_create(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	int				modelindex		= G_FLOAT(OFS_PARM0);
	model_t			*mod			= vmw->Get_CModel(vmw, modelindex);
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	numfaces		= G_INT(OFS_PARM2);
	qcbrushface_t	*in_faces		= validateqcpointer(prinst, G_INT(OFS_PARM1), sizeof(*in_faces), numfaces, numfaces==0);
	unsigned int	contents		= G_INT(OFS_PARM3);
	unsigned int	brushid			= (prinst->callargc > 4)?G_INT(OFS_PARM4):0;	//to simplify edits

	unsigned int			i;
	brushes_t				brush, *nb;
	vec4_t					*planes;
	struct brushface_s		*faces;

	G_INT(OFS_RETURN) = 0;

	if (!hm)
	{
		if (mod && mod->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
		if (mod && mod->loadstate == MLS_LOADED)
		{
			char basename[MAX_QPATH];
			COM_FileBase(mod->name, basename, sizeof(basename));
			mod->terrain = Mod_LoadTerrainInfo(mod, basename, true);
			hm = mod->terrain;
			if (!hm)
				return;
			Terr_FinishTerrain(mod);
		}
		else
			return;
	}

	//if we're creating one that already exists, then assume that its a move.
	if (brushid && Terr_Brush_DeleteId(hm, brushid))
	{
#ifdef HAVE_SERVER
		if (sv.state && modelindex > 0)
		{
			MSG_WriteByte(&sv.multicast, svcfte_brushedit);
			MSG_WriteShort(&sv.multicast, modelindex);
			MSG_WriteByte(&sv.multicast, hmcmd_brush_delete);
			MSG_WriteLong(&sv.multicast, brushid);
			SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
		}
		else
#endif
#ifdef HAVE_CLIENT
		if (cls.state && modelindex > 0)
		{
			MSG_WriteByte(&cls.netchan.message, clcfte_brushedit);
			MSG_WriteShort(&cls.netchan.message, modelindex);
			MSG_WriteByte(&cls.netchan.message, hmcmd_brush_delete);
			MSG_WriteLong(&cls.netchan.message, brushid);
		}
#else
		{
		}
#endif
	}

	planes = alloca(sizeof(*planes) * numfaces);
	faces = alloca(sizeof(*faces) * numfaces);
	for (i = 0; i < numfaces; i++)
	{
		VectorCopy(in_faces[i].planenormal, planes[i]);
		planes[i][3] = in_faces[i].planedist;

		faces[i].tex = Terr_Brush_FindTexture(hm, PR_GetString(prinst, in_faces[i].shadername));

		VectorCopy(in_faces[i].sdir, faces[i].stdir[0]);
		faces[i].stdir[0][3] = in_faces[i].sbias;
		VectorCopy(in_faces[i].tdir, faces[i].stdir[1]);
		faces[i].stdir[1][3] = in_faces[i].tbias;

		//these are for compat with (q2/)q3 so as to not be lossy even if they're not really used.
		faces[i].surfaceflags = 0;
		faces[i].surfacevalue = 0;
	}

	//now emit it
	brush.id = 0;
	brush.contents = contents;
	brush.numplanes = numfaces;
	brush.planes = planes;
	brush.faces = faces;
	brush.patch = NULL;
	if (numfaces)
	{
		nb = Terr_Brush_Insert(mod, hm, &brush);
		if (nb)
		{
			G_INT(OFS_RETURN) = nb->id;
#ifdef HAVE_SERVER
			if (sv.state && modelindex > 0)
			{
				MSG_WriteByte(&sv.multicast, svcfte_brushedit);
				MSG_WriteShort(&sv.multicast, modelindex);
				MSG_WriteByte(&sv.multicast, hmcmd_brush_insert);
				Brush_Serialise(&sv.multicast, nb);
				SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
				return;
			}
#endif
#ifdef HAVE_CLIENT
			if (cls.state && modelindex > 0)
			{
				MSG_WriteByte(&cls.netchan.message, clcfte_brushedit);
				MSG_WriteShort(&cls.netchan.message, modelindex);
				MSG_WriteByte(&cls.netchan.message, hmcmd_brush_insert);
				Brush_Serialise(&cls.netchan.message, nb);
				return;
			}
#endif
		}
	}
}
//{"patch_create",	PF_patch_create,	0,		0,		0,		0,		D("int(float modelidx, int oldpatchid, patchvert_t *in_controlverts, patchinfo_t in_info)", "Inserts a new patch into the model. Return value is the new patch's id.")},
void QCBUILTIN PF_patch_create(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	int				modelindex		= G_FLOAT(OFS_PARM0);
	model_t			*mod			= vmw->Get_CModel(vmw, modelindex);
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	brushid			= G_INT(OFS_PARM1);	//to simplify edits
	qcpatchinfo_t	*info			= (qcpatchinfo_t*)&G_INT(OFS_PARM3);
	unsigned int	totalcp			= info->cp_width*info->cp_width;
	qcpatchvert_t	*in_cverts		= validateqcpointer(prinst, G_INT(OFS_PARM2), sizeof(*in_cverts), totalcp, false);

	unsigned int			i;
	brushes_t				brush, *nb;

	G_INT(OFS_RETURN) = 0;

	if (!hm)
	{
		if (mod && mod->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
		if (mod && mod->loadstate == MLS_LOADED)
		{
			char basename[MAX_QPATH];
			COM_FileBase(mod->name, basename, sizeof(basename));
			mod->terrain = Mod_LoadTerrainInfo(mod, basename, true);
			hm = mod->terrain;
			if (!hm)
				return;
			Terr_FinishTerrain(mod);
		}
		else
			return;
	}

	//if we're creating one that already exists, then assume that its a move.
	if (brushid && Terr_Brush_DeleteId(hm, brushid))
	{
#ifdef HAVE_SERVER
		if (sv.state && modelindex > 0)
		{
			MSG_WriteByte(&sv.multicast, svcfte_brushedit);
			MSG_WriteShort(&sv.multicast, modelindex);
			MSG_WriteByte(&sv.multicast, hmcmd_brush_delete);
			MSG_WriteLong(&sv.multicast, brushid);
			SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
		}
		else
#endif
#ifdef HAVE_CLIENT
		if (cls.state && modelindex > 0)
		{
			MSG_WriteByte(&cls.netchan.message, clcfte_brushedit);
			MSG_WriteShort(&cls.netchan.message, modelindex);
			MSG_WriteByte(&cls.netchan.message, hmcmd_brush_delete);
			MSG_WriteLong(&cls.netchan.message, brushid);
		}
#else
		{
		}
#endif
	}

	brush.patch = alloca(sizeof(*brush.patch) + sizeof(brush.patch->cp[0])*(totalcp-countof(brush.patch->cp)));
	memset (brush.patch, 0, sizeof(*brush.patch) - sizeof(brush.patch->cp));
	brush.patch->numcp[0] = info->cp_width;
	brush.patch->numcp[1] = info->cp_height;
	brush.patch->subdiv[0] = info->subdiv_x;
	brush.patch->subdiv[1] = info->subdiv_y;

	brush.patch->tex = Terr_Brush_FindTexture(hm, PR_GetString(prinst, info->shadername));

	for (i = 0; i < totalcp; i++)
	{
		VectorCopy(in_cverts[i].v, brush.patch->cp[i].v);
		Vector2Copy(in_cverts[i].tc, brush.patch->cp[i].tc);
		Vector4Copy(in_cverts[i].rgba, brush.patch->cp[i].rgba);
	}

	//now emit it
	brush.id = 0;
	brush.contents = info->contents;
	brush.numplanes = 0;
	brush.planes = NULL;
	if (info->cp_width > 1 && info->cp_width > 1)
	{
		nb = Terr_Brush_Insert(mod, hm, &brush);
		if (nb)
		{
			G_INT(OFS_RETURN) = nb->id;
#ifdef HAVE_SERVER
			if (sv.state && modelindex > 0)
			{
				MSG_WriteByte(&sv.multicast, svcfte_brushedit);
				MSG_WriteShort(&sv.multicast, modelindex);
				MSG_WriteByte(&sv.multicast, hmcmd_patch_insert);
				Patch_Serialise(&sv.multicast, nb);
				SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
				return;
			}
#endif
#ifdef HAVE_CLIENT
			if (cls.state && modelindex > 0)
			{
				MSG_WriteByte(&cls.netchan.message, clcfte_brushedit);
				MSG_WriteShort(&cls.netchan.message, modelindex);
				MSG_WriteByte(&cls.netchan.message, hmcmd_patch_insert);
				Patch_Serialise(&cls.netchan.message, nb);
				return;
			}
#endif
		}
	}
}
//	{"brush_delete",	PF_brush_delete,	0,		0,		0,		0,		D("void(float modelidx, int brushid)", "Destroys the specified brush.")},
void QCBUILTIN PF_brush_delete(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	int				modelindex		= G_FLOAT(OFS_PARM0);
	model_t			*mod			= vmw->Get_CModel(vmw, modelindex);
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	brushid			= G_INT(OFS_PARM1);

	if (!hm)
		return;

	Terr_Brush_DeleteId(hm, brushid);

#ifdef HAVE_SERVER
	if (sv.state && modelindex > 0)
	{
		MSG_WriteByte(&sv.multicast, svcfte_brushedit);
		MSG_WriteShort(&sv.multicast, modelindex);
		MSG_WriteByte(&sv.multicast, hmcmd_brush_delete);
		MSG_WriteLong(&sv.multicast, brushid);
		SV_MulticastProtExt(vec3_origin, MULTICAST_ALL_R, ~0, 0, 0);
		return;
	}
#endif
#ifdef HAVE_CLIENT
	if (cls.state && modelindex > 0)
	{
		MSG_WriteByte(&cls.netchan.message, clcfte_brushedit);
		MSG_WriteShort(&cls.netchan.message, modelindex);
		MSG_WriteByte(&cls.netchan.message, hmcmd_brush_delete);
		MSG_WriteLong(&cls.netchan.message, brushid);
		return;
	}
#endif
}
//	{"brush_selected",	PF_brush_selected,	0,		0,		0,		0,		D("float(float modelid, int brushid, int faceid, float selectedstate)", "Allows you to easily set transient visual properties of a brush. If brush/face is -1, applies to all. returns old value. selectedstate=-1 changes nothing (called for its return value).")},
void QCBUILTIN PF_brush_selected(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	model_t			*mod			= vmw->Get_CModel(vmw, G_FLOAT(OFS_PARM0));
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	brushid			= G_INT(OFS_PARM1);
//	unsigned int	faceid			= G_INT(OFS_PARM2);
	int				state			= G_FLOAT(OFS_PARM3);
	unsigned int	i;
	brushes_t		*br;

	G_FLOAT(OFS_RETURN) = 0;
	if (!hm)
		return;

//	hm->recalculatebrushlighting = true;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == brushid)
		{
			G_FLOAT(OFS_RETURN) = br->selected;

			if (state >= 0)
			{
				if (br->selected != state)
				{
					if (br->patch)
					{
						br->patch->tex->rebuild = true;
//						br->patch->relight = true;
					}
					else
					{
						for (i = 0; i < br->numplanes; i++)
						{
							br->faces[i].tex->rebuild = true;
							br->faces[i].relight = true;
						}
					}
					br->selected = state;
				}
			}
//			return;
		}
	}
}



//	{"brush_calcfacepoints",PF_brush_calcfacepoints,0,0,		0,		0,		D("int(int faceid, brushface_t *in_faces, int numfaces, vector *points, int maxpoints)", "Determines the points of the specified face, if the specified brush were to actually be created.")},
void QCBUILTIN PF_brush_calcfacepoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t	faceid			= G_INT(OFS_PARM0);
	size_t	numfaces		= G_INT(OFS_PARM2);
	qcbrushface_t	*in_faces		= validateqcpointer(prinst, G_INT(OFS_PARM1), sizeof(*in_faces), numfaces, false);
	size_t	maxpoints		= G_INT(OFS_PARM4);
	vec3_t			*out_verts		= validateqcpointer(prinst, G_INT(OFS_PARM3), sizeof(*out_verts), maxpoints, false);
	vecV_t			facepoints[256];
	vec4_t			planes[256];
	unsigned int	j, numpoints;

	faceid--;
	if ((size_t)faceid >= numfaces)
	{
		G_INT(OFS_RETURN) = 0;
		return;
	}

	//make sure this isn't a dupe face
	for (j = 0; j < faceid; j++)
	{
		if (in_faces[j].planenormal[0] == in_faces[faceid].planenormal[0] &&
			in_faces[j].planenormal[1] == in_faces[faceid].planenormal[1] &&
			in_faces[j].planenormal[2] == in_faces[faceid].planenormal[2] && 
			in_faces[j].planedist == in_faces[faceid].planedist)
		{
			G_INT(OFS_RETURN) = 0;
			return;
		}
	}

	//generate a list that Terr_GenerateBrushFace can actually use, silly, but lets hope this isn't needed to be nippy
	for (j = 0; j < numfaces; j++)
	{
		VectorCopy(in_faces[j].planenormal, planes[j]);
		planes[j][3] = in_faces[j].planedist;
	}

	//generate points now (so we know the correct mins+maxs for the brush, and whether the plane is relevent)
	numpoints = Fragment_ClipPlaneToBrush(facepoints, countof(facepoints), planes, sizeof(*planes), numfaces, planes[faceid]);
	G_INT(OFS_RETURN) = numpoints;
	if (numpoints > maxpoints)
		numpoints = maxpoints;

	//... and copy them out without padding. yeah, silly.
	for (j = 0; j < numpoints; j++)
	{
		VectorCopy(facepoints[j], out_verts[j]);
	}
}

//	{"brush_getfacepoints",PF_brush_getfacepoints,0,0,		0,		0,		D("int(float modelid, int brushid, int faceid, vector *points, int maxpoints)", "Allows you to easily set transient visual properties of a brush. If brush/face is -1, applies to all. returns old value. selectedstate=-1 changes nothing (called for its return value).")},
void QCBUILTIN PF_brush_getfacepoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	model_t			*mod			= vmw->Get_CModel(vmw, G_FLOAT(OFS_PARM0));
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	unsigned int	brushid			= G_INT(OFS_PARM1);
	unsigned int	faceid			= G_INT(OFS_PARM2);
	unsigned int	maxpoints		= G_INT(OFS_PARM4), p;
	vec3_t			*out_verts		= validateqcpointer(prinst, G_INT(OFS_PARM3), sizeof(*out_verts), maxpoints, false);
	size_t i;
	brushes_t *br;

	G_INT(OFS_RETURN) = 0;

	if (!hm)
		return;

	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];
		if (br->id == brushid)
		{
			if (!faceid)
			{
				if (maxpoints >= 2)
				{
					VectorCopy(br->mins, out_verts[0]);
					VectorCopy(br->maxs, out_verts[1]);
					G_INT(OFS_RETURN) = 2;
				}
				else if (maxpoints == 1)
				{
					VectorInterpolate(br->mins, 0.5, br->maxs, out_verts[0]);
					G_INT(OFS_RETURN) = 1;
				}
			}
			else
			{
				faceid--;
				if (br->patch)
				{
					int w = br->patch->numcp[0];
					int h = br->patch->numcp[1];
					int x = faceid % (w-1);
					int y = faceid / (w-1);
					if (x >= w-1 || y >= h-1)
						break;
					if (maxpoints >= 1)
						VectorCopy(br->patch->cp[(x+0)+(y+0)*w].v, out_verts[0]);
					if (maxpoints >= 2)
						VectorCopy(br->patch->cp[(x+1)+(y+0)*w].v, out_verts[1]);
					if (maxpoints >= 3)
						VectorCopy(br->patch->cp[(x+1)+(y+1)*w].v, out_verts[2]);
					if (maxpoints >= 3)
						VectorCopy(br->patch->cp[(x+0)+(y+1)*w].v, out_verts[3]);
					p = min(4, maxpoints);
				}
				else
				{
					if (faceid >= br->numplanes)
						break;
					maxpoints = min(maxpoints, br->faces[faceid].numpoints);
					for (p = 0; p < maxpoints; p++)
						VectorCopy(br->faces[faceid].points[p], out_verts[p]);
				}
				G_INT(OFS_RETURN) = p;
			}
			break;
		}
	}
}
//	{"brush_findinvolume",PF_brush_findinvolume,0,0,		0,		0,		D("int(float modelid, vector *planes, float *dists, int numplanes, int *out_brushes, int *out_faces, int maxresults)", "Allows you to easily obtain a list of brushes+faces within the given bounding region. If out_faces is not null, the same brush might be listed twice.")},
void QCBUILTIN PF_brush_findinvolume(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *vmw = prinst->parms->user;
	model_t			*mod			= vmw->Get_CModel(vmw, G_FLOAT(OFS_PARM0));
	heightmap_t		*hm				= mod?mod->terrain:NULL;
	int				in_numplanes	= G_INT(OFS_PARM3);
	vec3_t			*in_normals		= validateqcpointer(prinst, G_INT(OFS_PARM1), sizeof(*in_normals), in_numplanes, false);
	float			*in_distances	= validateqcpointer(prinst, G_INT(OFS_PARM2), sizeof(*in_distances), in_numplanes, false);
	unsigned int	maxresults		= G_INT(OFS_PARM6);
	unsigned int	*out_brushids	= validateqcpointer(prinst, G_INT(OFS_PARM4), sizeof(*out_brushids), maxresults, false);
	unsigned int	*out_faceids	= G_INT(OFS_PARM5)?validateqcpointer(prinst, G_INT(OFS_PARM5), sizeof(*out_faceids), maxresults, false):NULL;
	unsigned int	i, j, k, r = 0;
	brushes_t *br;
	vec3_t best;
	float dist;

	//find all brushes/faces with a vetex within the region
	//the brush is inside if any every plane has at least one vertex on the inner side

	if (hm)
	for (i = 0; i < hm->numbrushes; i++)
	{
		br = &hm->wbrushes[i];

		for (j = 0; j < in_numplanes; j++)
		{
			for (k=0 ; k<3 ; k++)
			{
				if (in_normals[j][k] < 0)
					best[k] = br->maxs[k];
				else
					best[k] = br->mins[k];
			}
			dist = DotProduct (best, in_normals[j]);
			dist = in_distances[j] - dist;
			if (dist <= 0)	//don't find coplanar brushes. add an epsilon if you need this.
				break;
		}
		if (j == in_numplanes)
		{
			//the box had some point on the near side of every single plane, and thus must contain at least part of the box
			if (r == maxresults)
				break;	//ran out
			out_brushids[r] = br->id;
			if (out_faceids)	//FIXME: handle this properly.
				out_faceids[r] = 0;
			r++;
		}
	}
	G_INT(OFS_RETURN) = r;
}

void Terr_WriteBrushInfo(vfsfile_t *file, brushes_t *br)
{
	float *point[3];
	int i, x, y;
	const qboolean valve220 = true;

	VFS_PRINTF(file, "\n{");
	if (br->patch)
	{
		qboolean hasrgba = false;
		for (y = 0; y < br->patch->numcp[1]*br->patch->numcp[0]; y++)
		{
			if (br->patch->cp[y].rgba[0] != 1.0 || br->patch->cp[y].rgba[1] != 1.0 || br->patch->cp[y].rgba[2] != 1.0 || br->patch->cp[y].rgba[3] != 1.0)
				break;
		}
		hasrgba = (y < br->patch->numcp[1]*br->patch->numcp[0]);

		if (br->patch->subdiv[0]>=0 && br->patch->subdiv[1]>=0)
		{
			VFS_PRINTF(file, "\n\tpatchDef3%s\n\t{\n\t\t\"%s\"\n\t\t( %u %u %u %u %.9g %.9g %.9g )\n\t\t(\n",
					hasrgba?"WS":"",
					br->patch->tex?br->patch->tex->shadername:"",
					br->patch->numcp[0]/*width*/,
					br->patch->numcp[1]/*height*/,
					br->patch->subdiv[0]/*width*/,
					br->patch->subdiv[1]/*height*/,
					0.0/*rotation*/,
					1.0/*xscale*/,
					1.0/*yscale*/);
		}
		else
		{
			VFS_PRINTF(file, "\n\tpatchDef2%s\n\t{\n\t\t\"%s\"\n\t\t( %u %u %.9g %.9g %.9g )\n\t\t(\n",
					hasrgba?"WS":"",
					br->patch->tex?br->patch->tex->shadername:"",
					br->patch->numcp[0]/*width*/,
					br->patch->numcp[1]/*height*/,
					0.0/*rotation*/,
					1.0/*xscale*/,
					1.0/*yscale*/);
		}
		for (y = 0; y < br->patch->numcp[1]; y++)
		{
			VFS_PRINTF(file, "\t\t\t(\n");
			for (x = 0; x < br->patch->numcp[0]; x++)
			{
				const char *fmt;
				if (hasrgba)
					fmt = "\t\t\t\t( %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g )\n";
				else
					fmt = "\t\t\t\t( %.9g %.9g %.9g %.9g %.9g )\n";	//q3 compat.
				VFS_PRINTF(file, fmt,	br->patch->cp[x + y*br->patch->numcp[0]].v[0],
										br->patch->cp[x + y*br->patch->numcp[0]].v[1],
										br->patch->cp[x + y*br->patch->numcp[0]].v[2],
										br->patch->cp[x + y*br->patch->numcp[0]].tc[0],
										br->patch->cp[x + y*br->patch->numcp[0]].tc[1],
										br->patch->cp[x + y*br->patch->numcp[0]].rgba[0],
										br->patch->cp[x + y*br->patch->numcp[0]].rgba[1],
										br->patch->cp[x + y*br->patch->numcp[0]].rgba[2],
										br->patch->cp[x + y*br->patch->numcp[0]].rgba[3]);
			}
			VFS_PRINTF(file, "\t\t\t)\n");
		}
		VFS_PRINTF(file, "\t\t)\n\t}\n");
	}
	else
	{
		for (i = 0; i < br->numplanes; i++)
		{
			const char *texname, *s;
			point[0] = br->faces[i].points[0];
			point[1] = br->faces[i].points[1];
			point[2] = br->faces[i].points[2];

			//valve 220 format:
			//(-0 -0 16) (-0 -0 32) (64 -0 16) texname [x y z d] [x y z d] rotation sscale tscale
			//don't treat whitespace as optional, even if it works with qbsp it'll screw up third party editors.

			//%.9g is 'meant' to be lossless for a standard ieee single-precision float. (%.17g for a double)

			//write the 3 points-on-plane. I really hope its not degenerate
			VFS_PRINTF(file, "\n( %.9g %.9g %.9g ) ( %.9g %.9g %.9g ) ( %.9g %.9g %.9g )",
					point[0][0], point[0][1], point[0][2],
					point[1][0], point[1][1], point[1][2],
					point[2][0], point[2][1], point[2][2]
				);

			//write the name - if it contains markup or control chars, or other weird glyphs then be sure to quote it.
			//we could unconditionally quote it, but that can and will screw up some editor somewhere (like trenchbroom...)
			for (s = texname = br->faces[i].tex?br->faces[i].tex->shadername:""; *s; s++)
			{
				if (*s <= 32 || *s >= 127 || *s == '\\' || *s == '(' || *s == '[' || *s == '{' || *s == ')' || *s == ']' || *s == '}')
					break;	//
			}
			VFS_PRINTF(file, (!*texname || *s)?" \"%s\"":" %s", texname);

			if (valve220)
			{
				VFS_PRINTF(file, " [ %.9g %.9g %.9g %.9g ] [ %.9g %.9g %.9g %.9g ] 0 1 1",
						br->faces[i].stdir[0][0], br->faces[i].stdir[0][1], br->faces[i].stdir[0][2], br->faces[i].stdir[0][3],
						br->faces[i].stdir[1][0], br->faces[i].stdir[1][1], br->faces[i].stdir[1][2], br->faces[i].stdir[1][3]
					);
			}
			else
			{
				float soffset, toffset, rotation, sscale, tscale;
				//FIXME: project onto the axial plane, then figure out new values.
				soffset = toffset = 0;
				rotation = 0;
				sscale = tscale = 1;
				VFS_PRINTF(file, " %.9g %.9g %.9g %.9g %.9g", soffset, toffset, rotation, sscale, tscale);
			}

			//historical note: Q2 used contents|surfaceflags|value.
			//                 however, Q3 uses the contents value exclusively for a detail flag. everything else comes from shaders.
			if (br->contents != FTECONTENTS_SOLID || br->faces[i].surfaceflags || br->faces[i].surfacevalue)
				VFS_PRINTF(file, " %i %i %i", br->contents, br->faces[i].surfaceflags, br->faces[i].surfacevalue);
//			else if (hexen2)
//				VFS_PRINTF(file, " -1");	//Light
		}
	}

	VFS_PRINTF(file, "\n}");
}
void Terr_WriteMapFile(vfsfile_t *file, model_t *mod)
{
	char token[8192];
	int nest = 0;
	const char *start, *entities = Mod_GetEntitiesString(mod);
	int i;
	unsigned int entnum = 0;
	heightmap_t *hm;
	
	hm = mod->terrain;
	if (hm && hm->legacyterrain)
		VFS_WRITE(file, "terrain\n", 8);

	start = entities;
	while(entities)
	{
		entities = COM_ParseOut(entities, token, sizeof(token));
		if (token[0] == '}' && token[1] == 0)
		{
			nest--;
			if (!nest)
			{
				if (!entnum)
				{
//					VFS_PRINTF(file, "\n//Worldspawn brushes go here");

					hm = mod->terrain;
					if (hm)
						for (i = 0; i < hm->numbrushes; i++)
							Terr_WriteBrushInfo(file, &hm->wbrushes[i]);
				}
				entnum++;
			}
		}
		else if (token[0] == '{' && token[1] == 0)
		{
			nest++;
		}
		else
		{
			if (!strcmp(token, "model"))
			{
				int submodelnum;
				entities = COM_ParseOut(entities, token, sizeof(token));

				if (*token == '*')
					submodelnum = atoi(token+1);
				else
					submodelnum = 0;

				if (submodelnum)
				{
					model_t *submod;

					Q_snprintfz(token, sizeof(token), "*%i:%s", submodelnum, mod->name);
					submod = Mod_FindName (token);

//					VFS_PRINTF(file, "\nBrushes for %s go here", token);
					hm = submod->terrain;
					if (hm)
					{
						for (i = 0; i < hm->numbrushes; i++)
							Terr_WriteBrushInfo(file, &hm->wbrushes[i]);

						start = entities;
					}
				}
			}
			else
				entities = COM_ParseOut(entities, token, sizeof(token));
		}
		if (entities)
			VFS_WRITE(file, start, entities - start);
		start = entities;
	}
}
void Mod_Terrain_Save_f(void)
{
	vfsfile_t *file;
	model_t *mod;
	const char *mapname = Cmd_Argv(1);
	char fname[MAX_QPATH];
	if (Cmd_IsInsecure())
	{
		Con_Printf("Please use this command via the console\n");
		return;
	}
	if (*mapname)
		mod = Mod_FindName(va("maps/%s", mapname));
#ifdef HAVE_CLIENT
	else if (cls.state)
		mod = cl.worldmodel;
#endif
	else
		mod = NULL;

	if (!mod)
	{
		Con_Printf("no model loaded by that name\n");
		return;
	}
	if (mod->loadstate != MLS_LOADED)
	{
		Con_Printf("that model isn't fully loaded\n");
		return;
	}
	if (*Cmd_Argv(2))
		Q_snprintfz(fname, sizeof(fname), "maps/%s.map", Cmd_Argv(2));
	else
		Q_snprintfz(fname, sizeof(fname), "%s", mod->name);

	if (mod->type != mod_heightmap)
	{
		//warning: brushes are not saved unless its a .map
		COM_StripExtension(mod->name, fname, sizeof(fname));
		Q_strncatz(fname, mod_modifier, sizeof(fname));
		Q_strncatz(fname, ".ent", sizeof(fname));

		FS_CreatePath(fname, FS_GAMEONLY);
		file = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
		if (!file)
			Con_TPrintf("unable to open %s\n", fname);
		else
		{
			const char *s = Mod_GetEntitiesString(mod);
			VFS_WRITE(file, s, strlen(s));
			VFS_CLOSE(file);
			FS_FlushFSHashWritten(fname);
		}
	}
	else
	{
		FS_CreatePath(fname, FS_GAMEONLY);
		file = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
		if (!file)
			Con_TPrintf("unable to open %s\n", fname);
		else
		{
			Terr_WriteMapFile(file, mod);
			VFS_CLOSE(file);
			FS_FlushFSHashWritten(fname);
		}
	}
}
qboolean Terr_ReformEntitiesLump(model_t *mod, heightmap_t *hm, char *entities)
{
	char token[8192];
	int nest = 0;
	int buflen = strlen(entities);
	char *out, *outstart, *start;
	int i;
	int submodelnum = 0;
	qboolean foundsubmodel = false;
	qboolean inbrush = false;
	int brushcontents = FTECONTENTS_SOLID;
	heightmap_t *subhm = NULL;
	model_t *submod = NULL;
	const char *brushpunct = "(){}[]";	//use an empty string for better compat with vanilla qbsp...

	//brush planes
	int numplanes = 0;
	vec4_t planes[256];
	struct brushface_s faces[countof(planes)];

	//patch info
	brushtex_t *patch_tex=NULL;
	int	patchsz[2]={0,0}, patchsubdiv[2]={-1,-1};
	qcpatchvert_t patch_v[64][64];

#ifdef RUNTIMELIGHTING
	hm->entsdirty = true;
	hm->relightcontext = mod_terrain_brushlights.ival?LightStartup(NULL, mod, mod_terrain_brushlights.ival>1, false):NULL;
	hm->lightthreadmem = BZ_Malloc(lightthreadctxsize);
	hm->inheritedlightthreadmem = false;
#endif

	/*FIXME: we need to re-form the entities lump to insert model fields as appropriate*/
	outstart = out = Z_Malloc(buflen+1);

	while(entities)
	{
		start = entities;
		entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
		if (!entities)
		{
			if (inbrush || nest)
			{
				Con_Printf(CON_ERROR "%s: File truncated?\n", mod->name);
				return false;
			}
			break;
		}
		else if (token[0] == '}' && token[1] == 0)
		{
			nest--;
			if (inbrush)
			{
				if (subhm)
				{
					qboolean oe = subhm->brushesedited;
					if (numplanes)
					{
						brushes_t brush;
						//finish the brush
						brush.contents = brushcontents;
						brush.numplanes = numplanes;
						brush.planes = planes;
						brush.faces = faces;
						brush.id = 0;
						brush.patch = NULL;
						Terr_Brush_Insert(submod, subhm, &brush);
					}
					else if (patch_tex)
						Terr_Patch_Insert(submod, subhm, patch_tex, patchsz[0], patchsz[1], patchsubdiv[0], patchsubdiv[1], patch_v[0], countof(patch_v[0]));
					subhm->brushesedited = oe;
				}
				numplanes = 0;
				inbrush = false;
				patch_tex = NULL;
				brushcontents = FTECONTENTS_SOLID;
				continue;
			}
		}
		else if (token[0] == '{' && token[1] == 0)
		{
			nest++;
			if (nest == 1)
			{	//entering a new entity
				foundsubmodel = false;
			}
			if (nest == 2)
			{
				if (!foundsubmodel)
				{
					foundsubmodel = true;
					if (submodelnum)
					{
						Q_snprintfz(token, sizeof(token), "*%i", submodelnum);
						*out++ = '\n';
						*out++ = 'm';
						*out++ = 'o';
						*out++ = 'd';
						*out++ = 'e';
						*out++ = 'l';
						*out++ = ' ';
						*out++ = '\"';
						for (i = 0; token[i]; i++)
							*out++ = token[i];
						*out++ = '\"';
						*out++ = ' ';
						
						Q_snprintfz(token, sizeof(token), "*%i:%s", submodelnum, mod->name);
						submod = Mod_FindName (token);
						if (submod->loadstate == MLS_NOTLOADED)
						{
							submod->type = mod_heightmap;
							Mod_SetEntitiesString(submod, "", true);
							subhm = submod->terrain = Mod_LoadTerrainInfo(submod, submod->name, true);

							subhm->exteriorcontents = FTECONTENTS_EMPTY;

							ClearBounds(submod->mins, submod->maxs);

							submod->funcs.NativeTrace			= Heightmap_Trace_Test;
							submod->funcs.PointContents			= Heightmap_PointContents;
							submod->funcs.NativeContents		= Heightmap_NativeBoxContents;
							submod->funcs.LightPointValues		= Heightmap_LightPointValues;
							submod->funcs.StainNode				= Heightmap_StainNode;
							submod->funcs.MarkLights			= Heightmap_MarkLights;
							submod->funcs.ClusterForPoint		= Heightmap_ClusterForPoint;
							submod->funcs.ClusterPVS			= Heightmap_ClusterPVS;
#ifdef HAVE_SERVER
							submod->funcs.FindTouchedLeafs		= Heightmap_FindTouchedLeafs;
							submod->funcs.EdictInFatPVS			= Heightmap_EdictInFatPVS;
							submod->funcs.FatPVS				= Heightmap_FatPVS;
#endif
							submod->loadstate = MLS_LOADED;
							submod->pvsbytes = sizeof(hmpvs_t);

#ifdef RUNTIMELIGHTING
							if (hm->relightcontext)
								subhm->relightcontext = LightStartup(hm->relightcontext, submod, false, false);
							subhm->lightthreadmem = hm->lightthreadmem;
							subhm->inheritedlightthreadmem = true;
#endif
						}
						else
							subhm = NULL;
					}
					else
					{
						submod = mod;
						subhm = hm;
					}
					submodelnum++;
				}
				inbrush = true;
				continue;
			}
		}
		else if (!nest)
		{
			Con_Printf(CON_ERROR "%s: junk found\n", mod->name);
			return false;
		}
		else if (inbrush && (!strcmp(token, "patchDef2")   || !strcmp(token, "patchDef3") ||
							 !strcmp(token, "patchDef2WS") || !strcmp(token, "patchDef3WS")))
		{
			int x, y;
			qboolean patchdef3 = !!strchr(token, '3');	//explict tessellation info (doom3-like)
			qboolean parsergba = !!strstr(token, "WS");	//fancy alternative with rgba colours per control point
			patchsz[0] = patchsz[1] = 0;
			patchsubdiv[0] = patchsubdiv[1] = -1;
			if (numplanes || patch_tex)
			{
				Con_Printf(CON_ERROR "%s: mixed patch+planes\n", mod->name);
				return false;
			}
			memset(patch_v, 0, sizeof(patch_v));
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			if (strcmp(token, "{")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			/*parse texture name*/
			patch_tex = Terr_Brush_FindTexture(subhm, token);
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			if (strcmp(token, "(")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			/*patch_w = atof(token);*/
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			/*patch_h = atof(token);*/
			if (patchdef3)
			{
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				patchsubdiv[0] = atof(token);
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				patchsubdiv[1] = atof(token);
			}
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			/*rotation = atof(token);*/
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			/*xscale = atof(token);*/
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			/*yscale = atof(token);*/
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			if (strcmp(token, ")")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			if (strcmp(token, "(")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			y = 0;
			while (!strcmp(token, "("))
			{
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				x = 0;
				while (!strcmp(token, "("))
				{
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					patch_v[y][x].v[0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					patch_v[y][x].v[1] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					patch_v[y][x].v[2] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					patch_v[y][x].tc[0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					patch_v[y][x].tc[1] = atof(token);

					if (parsergba)
					{	//the following four lines are stupid.
						entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
						if (strcmp(token, ")")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
						entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
						if (strcmp(token, "(")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}

						entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
						patch_v[y][x].rgba[0] = atof(token);
						entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
						patch_v[y][x].rgba[1] = atof(token);
						entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
						patch_v[y][x].rgba[2] = atof(token);
						entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
						patch_v[y][x].rgba[3] = atof(token);
					}
					else
					{	//no data provided, use default values.
						patch_v[y][x].rgba[0] =
						patch_v[y][x].rgba[1] =
						patch_v[y][x].rgba[2] =
						patch_v[y][x].rgba[3] = 1.0;
					}

					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					if (strcmp(token, ")")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}

					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					if (x < countof(patch_v[y])-1)
						x++;
				}
				if (patchsz[0] < x)
					patchsz[0] = x;
				if (strcmp(token, ")")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				if (y < countof(patch_v)-1)
					y++;
			}
			patchsz[1] = y;
			if (strcmp(token, ")")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
			entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			if (strcmp(token, "}")) {Con_Printf(CON_ERROR "%s: invalid patch\n", mod->name);return false;}
			continue;
		}
		else if (inbrush)
		{
			//parse a plane
			//Quake:             ( -0 -0 16 ) ( -0 -0 32 ) ( 64 -0 16 )                         texname soffset toffset     rotation sscale tscale
			//Hexen2:            ( -0 -0 16 ) ( -0 -0 32 ) ( 64 -0 16 )                         texname soffset toffset     rotation sscale tscale surfvalue
			//Valve:             ( -0 -0 16 ) ( -0 -0 32 ) ( 64 -0 16 )                         texname [x y z d] [x y z d] rotation sscale tscale
			//FTE  :             ( px py pz pd )                                                texname [x y z d] [x y z d] rotation sscale tscale contents surfflags surfvalue
			//Quake2:            ( -0 -0 16 ) ( -0 -0 32 ) ( 64 -0 16 )                         texname soffset toffset     rotation sscale tscale contents surfflags surfvalue
			//Quake3:            ( -0 -0 16 ) ( -0 -0 32 ) ( 64 -0 16 )                         texname soffset toffset     rotation sscale tscale detailfl surfflags surfvalue
			//Q3 BP: brushDef {  ( -0 -0 16 ) ( -0 -0 32 ) ( 64 -0 16 ) ( ( x y o ) ( x y o ) ) texname                                            detailfl surfflags surfvalue } //generate tangent+bitangent from the normal to generate base texcoords, then transform by the given 2*3 matrix. I prefer valve's way - it rotates more cleanly.
			//Doom3: brushDef3 { ( px py pz pd )                        ( ( x y o ) ( x y o ) ) texname                                            detailfl surfflags surfvalue }
			//hexen2's extra surfvalue is completely unused, and should normally be -1
			//q3 ignores all contents except detail, as well surfaceflags and surfacevalue
			//220 ignores rotation, provided only for UI info, scale is still used

			//we don't care whether the input is planes or points.
			//if we get a [ instead of an soffset then its

			brushtex_t *bt;
			vec3_t d1,d2;
			vec3_t points[3];
			vec4_t texplane[2];
			float scale[2], rot;
			int p;
			enum
			{
				TEXTYPE_AXIAL,	//urgh
				TEXTYPE_PLANES,
				TEXTYPE_BP,		//weird 2d planes
			} textype = TEXTYPE_AXIAL;
			memset(points, 0, sizeof(points));
			if (patch_tex)
			{
				Con_Printf(CON_ERROR "%s: mixed patch+planes\n", mod->name);
				return false;
			}
			for (p = 0; p < 3; p++)
			{
				if (token[0] != '(' || token[1] != 0)
					break;
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				points[p][0] = atof(token);
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				points[p][1] = atof(token);
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				points[p][2] = atof(token);
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				if (token[0] != ')' || token[1] != 0)
				{
//					VectorClear(points[1]);
//					VectorClear(points[2]);
					points[1][0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					if (p == 0 && !strcmp(token, ")"))
						p = 4;	//we just managed to read an entire plane instead of 3 points.
					break;
				}
				entities = COM_ParseTokenOut(entities, "()", token, sizeof(token), NULL);
			}
			if (p < 3)
			{
				Con_Printf(CON_ERROR "%s: malformed brush\n", mod->name);
				return false;
			}
			if (numplanes == sizeof(planes)/sizeof(planes[0]))
			{
				Con_Printf(CON_ERROR "%s: too many planes in brush\n", mod->name);
				return false;
			}

			if (token[0] == '(')
			{
				textype = TEXTYPE_BP;
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				if (token[0] == '(')
				{
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][1] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][3] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					if (token[0] != ')')
						return false;
				}
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				if (token[0] == '(')
				{
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][1] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][3] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					if (token[0] != ')')
						return false;
				}
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				if (token[0] != ')')
					return false;
			}

			bt = Terr_Brush_FindTexture(subhm, token);

			if (textype != TEXTYPE_BP)
			{
				//halflife/valve220 format has the entire [x y z dist] plane specified.
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				if (*token == '[')
				{
					textype = TEXTYPE_PLANES;

					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][1] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][2] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[0][3] = atof(token);

					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					//]
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					//[

					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][0] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][1] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][2] = atof(token);
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][3] = atof(token);

					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					//]
				}
				else
				{	//vanilla quake
					VectorClear(texplane[0]);
					VectorClear(texplane[1]);
					texplane[0][3] = atof(token);	//aka soffset
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					texplane[1][3] = atof(token);	//aka toffset
				}

				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				rot = atof(token);
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				scale[0] = atof(token);
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				scale[1] = atof(token);
			}
			else rot = 0, scale[0] = 1, scale[1] = 1;

			//hexen2 has some extra junk that is useless - some 'light' value, but its never used and should normally be -1.
			//quake2/3 on the other hand has 3 different args. Contents SurfaceFlags SurfaceValue.
			//the SurfaceFlags and SurfaceVales are no longer used in q3 (shaders do it all), but contents is still partially used.
			//The contents conveys only CONTENTS_DETAIL. which is awkward as it varies somewhat by game, but we assume q2/q3.
			faces[numplanes].surfaceflags = 0;
			faces[numplanes].surfacevalue = 0;

			while (*entities == ' ' || *entities == '\t')
				entities++;
			if (*entities == '-' || (*entities >= '0' && *entities <= '9'))
			{
				int ex1, ex2 = 0, ex3 = 0;
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
				ex1 = atoi(token);

				while (*entities == ' ' || *entities == '\t')
					entities++;
				if (*entities == '-' || (*entities >= '0' && *entities <= '9'))
				{
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					ex2 = atoi(token);
				}

				while (*entities == ' ' || *entities == '\t')
					entities++;
				if (*entities == '-' || (*entities >= '0' && *entities <= '9'))
				{
					entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
					ex3 = atoi(token);
					//if we got this far, then its q3 format.
					//q3 is weird. the first extra arg is contents. but only the detail contents is used.
					//ex1 &= Q3CONTENTS_DETAIL;
					brushcontents |= ex1;

					//propagate these, in case someone tries editing a q2bsp.
					faces[numplanes].surfaceflags = ex2;
					faces[numplanes].surfacevalue = ex3;
				}
			}

			//okay, that's all the actual parsing, now try to make sense of this plane.
			if (p == 4)
			{	//parsed an actual plane
				VectorCopy(points[0], planes[numplanes]);
				planes[numplanes][3] = points[1][0];
			}
			else
			{	//parsed 3 points.
				VectorSubtract(points[0], points[1], d1);
				VectorSubtract(points[2], points[1], d2);
				CrossProduct(d1, d2, planes[numplanes]);
				VectorNormalize(planes[numplanes]);
				planes[numplanes][3] = DotProduct(points[1], planes[numplanes]);
			}
			faces[numplanes].tex = bt;

			/*
			shader_t *shader = R_RegisterCustom(NULL, bt->shadername, SUF_LIGHTMAP, NULL, NULL);
			if (shader)
			{
				brushcontents &= Q3CONTENTS_DETAIL;
				brushcontents |= shader->contentbits&~Q3CONTENTS_DETAIL;
				faces[numplanes].surfaceflags = shader->surfacebits;
			}
			else
			*/
			if (bt && !numplanes)
			{
				if (*bt->shadername == '*')
				{
					if (!Q_strncasecmp(bt->shadername, "*lava", 5))
						brushcontents |= FTECONTENTS_LAVA;
					else if (!Q_strncasecmp(bt->shadername, "*slime", 5))
						brushcontents |= FTECONTENTS_SLIME;
					else
						brushcontents |= FTECONTENTS_WATER;
				}
				else if (!Q_strncasecmp(bt->shadername, "*sky", 4))
					brushcontents |= FTECONTENTS_SKY;
				else if (!Q_strcasecmp(bt->shadername, "clip"))
					brushcontents |= FTECONTENTS_PLAYERCLIP|FTECONTENTS_MONSTERCLIP;
				else if (!Q_strcasecmp(bt->shadername, "hint"))
					brushcontents |= 0;
				else if (!Q_strcasecmp(bt->shadername, "skip"))	//skip should not force content values if paired with lava etc.
					;//brushcontents = 0;
				else
					brushcontents |= FTECONTENTS_SOLID;
			}

			if (textype == TEXTYPE_BP)
			{
				float *norm = planes[numplanes];
				float RotY = -atan2(norm[2], sqrt(norm[1]*norm[1] + norm[0]*norm[0]));
				float RotZ = atan2(norm[1], norm[0]);
				vec3_t tx = {-sin(RotZ), cos(RotZ), 0};		//tangent
				vec3_t ty = {-sin(RotY)*cos(RotZ), -sin(RotY)*sin(RotZ), -cos(RotY)};	//bitangent
				vec2_t tms = {texplane[0][0],texplane[0][1]}, tmt = {texplane[1][0],texplane[1][1]};	//bah, locals reuse suck
				texplane[0][0] = (tx[0] * tms[0]) + (ty[0] * tms[1]);	//multiply out some matricies
				texplane[0][1] = (tx[1] * tms[0]) + (ty[1] * tms[1]);
				texplane[0][2] = (tx[2] * tms[0]) + (ty[2] * tms[1]);
				texplane[1][0] = (tx[0] * tmt[0]) + (ty[0] * tmt[1]);
				texplane[1][1] = (tx[1] * tmt[0]) + (ty[1] * tmt[1]);
				texplane[1][2] = (tx[2] * tmt[0]) + (ty[2] * tmt[1]);

				//scale is part of the matrix.
				scale[0] = 1;
				scale[1] = 1;

				//FIXME: these faces should NOT be scaled by the texture's size!
			}
			else if (textype == TEXTYPE_PLANES)
				;//texture planes were properly loaded above (the scaling below is still needed though).
			else if (textype == TEXTYPE_AXIAL)
			{	//quake's .maps use the normal to decide which texture directions to use in some lame axially-aligned way.
				float a=fabs(planes[numplanes][0]),b=fabs(planes[numplanes][1]),c=fabs(planes[numplanes][2]);
				if (a>=b&&a>=c)
					texplane[0][1] = 1;
				else
					texplane[0][0] = 1;
				if (c>a&&c>b)
					texplane[1][1] = -1;
				else
					texplane[1][2] = -1;

				if (rot)
				{
					int mas, mat;
					float s,t;
					float a = rot*(M_PI/180);
					float cosa = cos(a), sina=sin(a);
					for (mas=0; mas<2&&!texplane[0][mas]; mas++);
					for (mat=0; mat<2&&!texplane[1][mat]; mat++);
					for (i = 0; i < 2; i++)
					{
						s = cosa*texplane[i][mas] - sina*texplane[i][mat];
						t = sina*texplane[i][mas] + cosa*texplane[i][mat];
						texplane[i][mas] = s;
						texplane[i][mat] = t;
					}
				}
			}

			if (!scale[0]) scale[0] = 1;
			if (!scale[1]) scale[1] = 1;
			VectorScale(texplane[0], 1.0/scale[0], faces[numplanes].stdir[0]);
			VectorScale(texplane[1], 1.0/scale[1], faces[numplanes].stdir[1]);
			faces[numplanes].stdir[0][3] = texplane[0][3];
			faces[numplanes].stdir[1][3] = texplane[1][3];

			numplanes++;
			continue;
		}
		else
		{
			/*if (!strcmp(token, "classname"))
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
			else*/
				entities = COM_ParseTokenOut(entities, brushpunct, token, sizeof(token), NULL);
		}
		while(start < entities)
			*out++ = *start++;
	}
	*out = 0;

	Mod_SetEntitiesString(mod, outstart, false);

	mod->numsubmodels = submodelnum;

	return true;
}

qboolean QDECL Terr_LoadTerrainModel (model_t *mod, void *buffer, size_t bufsize)
{
	int legacyterrain;
	heightmap_t *hm;

	char token[MAX_QPATH];
	int sectsize = 0;
	char *src;

	src = COM_ParseOut(buffer, token, sizeof(token));
	if (!strcmp(token, "terrain"))
	{
		legacyterrain = true;
		buffer = src;
	}
	else if (!strcmp(token, "{"))
		legacyterrain = false;
	else
	{
		Con_Printf(CON_ERROR "%s wasn't terrain map\n", mod->name);	//shouldn't happen
		return false;
	}

	mod->type = mod_heightmap;

	ClearBounds(mod->mins, mod->maxs);

	hm = Z_Malloc(sizeof(*hm));
	ClearLink(&hm->recycle);
//	ClearLink(&hm->collected);
	COM_FileBase(mod->name, hm->path, sizeof(hm->path));

	if (!Terr_ReformEntitiesLump(mod, hm, buffer))
		return false;

	strcpy(hm->groundshadername, "terrainshader");
	strcpy(hm->skyname, "sky1");

	hm->entitylock = Sys_CreateMutex();
	hm->sectionsize = sectsize;
	if (legacyterrain)
	{
		hm->firstsegx = -1;
		hm->firstsegy = -1;
		hm->maxsegx = +1;
		hm->maxsegy = +1;
	}
	else
	{
		hm->firstsegx = 0;
		hm->firstsegy = 0;
		hm->maxsegx = 0;
		hm->maxsegy = 0;
	}
	hm->legacyterrain = legacyterrain;
	if (legacyterrain)
		hm->exteriorcontents = FTECONTENTS_SOLID;	//sky outside the map

	Terr_ParseEntityLump(mod, hm);

	if (hm->firstsegx != hm->maxsegx)
	{
		vec3_t point;
		point[0] = (hm->firstsegx - CHUNKBIAS) * hm->sectionsize;
		point[1] = (hm->firstsegy - CHUNKBIAS) * hm->sectionsize;
		point[2] = -999999999999999999999999.f;
		AddPointToBounds(point, mod->mins, mod->maxs);
		point[0] = (hm->maxsegx - CHUNKBIAS) * hm->sectionsize;
		point[1] = (hm->maxsegy - CHUNKBIAS) * hm->sectionsize;
		point[2] = 999999999999999999999999.f;
		AddPointToBounds(point, mod->mins, mod->maxs);
	}

	mod->funcs.NativeTrace			= Heightmap_Trace_Test;
	mod->funcs.PointContents		= Heightmap_PointContents;

	mod->funcs.NativeContents		= Heightmap_NativeBoxContents;

	mod->funcs.LightPointValues		= Heightmap_LightPointValues;
	mod->funcs.StainNode			= Heightmap_StainNode;
	mod->funcs.MarkLights			= Heightmap_MarkLights;

	mod->funcs.ClusterForPoint		= Heightmap_ClusterForPoint;
	mod->funcs.ClusterPVS			= Heightmap_ClusterPVS;

#ifdef HAVE_SERVER
	mod->funcs.FindTouchedLeafs		= Heightmap_FindTouchedLeafs;
	mod->funcs.EdictInFatPVS		= Heightmap_EdictInFatPVS;
	mod->funcs.FatPVS				= Heightmap_FatPVS;
#endif
/*	mod->hulls[0].funcs.HullPointContents = Heightmap_PointContents;
	mod->hulls[1].funcs.HullPointContents = Heightmap_PointContents;
	mod->hulls[2].funcs.HullPointContents = Heightmap_PointContents;
	mod->hulls[3].funcs.HullPointContents = Heightmap_PointContents;
*/
	mod->pvsbytes = sizeof(hmpvs_t);

	mod->terrain = hm;

#ifdef RUNTIMELIGHTING
	if (hm->relightcontext)
	{
		LightReloadEntities(hm->relightcontext, Mod_GetEntitiesString(mod), true);
		hm->entsdirty = false;
	}
#endif

	validatelinks(&hm->recycle);
	return true;
}

void *Mod_LoadTerrainInfo(model_t *mod, char *loadname, qboolean force)
{
	heightmap_t *hm;
	heightmap_t potential;
	if (!Mod_GetEntitiesString(mod))
		return NULL;

	memset(&potential, 0, sizeof(potential));
	Terr_ParseEntityLump(mod, &potential);

	if (potential.firstsegx >= potential.maxsegx || potential.firstsegy >= potential.maxsegy)
	{
		//figure out the size such that it encompases the entire bsp.
		potential.firstsegx = floor(mod->mins[0] / potential.sectionsize) + CHUNKBIAS;
		potential.firstsegy = floor(mod->mins[1] / potential.sectionsize) + CHUNKBIAS;
		potential.maxsegx = ceil(mod->maxs[0] / potential.sectionsize) + CHUNKBIAS;
		potential.maxsegy = ceil(mod->maxs[1] / potential.sectionsize) + CHUNKBIAS;
		if (*loadname=='*')
		{
			potential.firstsegx = bound(0, potential.firstsegx, CHUNKLIMIT);
			potential.firstsegy = bound(0, potential.firstsegy, CHUNKLIMIT);
			potential.maxsegx = bound(potential.firstsegx, potential.maxsegx, CHUNKLIMIT);
			potential.maxsegy = bound(potential.firstsegx, potential.maxsegy, CHUNKLIMIT);
		}
		else
		{//bound it, such that 0 0 will always be loaded.
			potential.firstsegx = bound(0, potential.firstsegx, CHUNKBIAS);
			potential.firstsegy = bound(0, potential.firstsegy, CHUNKBIAS);
			potential.maxsegx = bound(CHUNKBIAS+1, potential.maxsegx, CHUNKLIMIT);
			potential.maxsegy = bound(CHUNKBIAS+1, potential.maxsegy, CHUNKLIMIT);
		}

		if (!force)
		{
			char sect[MAX_QPATH];
			Q_snprintfz(sect, sizeof(sect), "maps/%s/sect_%03x_%03x.hms", loadname, potential.firstsegx + (potential.maxsegx-potential.firstsegx)/2, potential.firstsegy + (potential.maxsegy-potential.firstsegy)/2);
			if (!COM_FCheckExists(sect))
			{
				Q_snprintfz(sect, sizeof(sect), "maps/%s/block_00_00.hms", loadname);
				if (!COM_FCheckExists(sect))
					return NULL;
			}
		}
	}

	hm = Z_Malloc(sizeof(*hm));
	*hm = potential;
	hm->entitylock = Sys_CreateMutex();
	ClearLink(&hm->recycle);
	Q_strncpyz(hm->path, loadname, sizeof(hm->path));
	Q_strncpyz(hm->groundshadername, "terrainshader", sizeof(hm->groundshadername));

	hm->exteriorcontents = FTECONTENTS_EMPTY;	//bsp geometry outside the heightmap

	return hm;
}

#ifdef HAVE_CLIENT
#if 0 //not yet ready
struct ted_import_s
{
	size_t x, y;
	size_t width;
	size_t height;
	unsigned short *data;
};
//static void ted_itterate(heightmap_t *hm, int distribution, float *pos, float radius, float strength, int steps,
static void ted_import_heights_r16(void *vctx, hmsection_t *s, int idx, float wx, float wy, float strength)
{
	struct ted_import_s *ctx = vctx;
	unsigned int y = idx/SECTHEIGHTSIZE;
	unsigned int x = idx%SECTHEIGHTSIZE;
	x += s->sx*(SECTHEIGHTSIZE-1) - ctx->x;
	y += s->sy*(SECTHEIGHTSIZE-1) - ctx->y;
	if (x >= ctx->width || y >= ctx->height)
		return;
	s->flags |= TSF_NOTIFY|TSF_EDITED|TSF_DIRTY|TSF_RELIGHT;
	s->heights[idx] = ctx->data[x + y*ctx->width] * (8192.0/(1<<16));
}
static void Mod_Terrain_Import_f(void)
{
	model_t *mod;
	struct ted_import_s ctx;
	const char *mapname = Cmd_Argv(1);
	const char *filename;
	size_t fsize;
	heightmap_t *hm;
	vec3_t pos = {0};
	if (Cmd_IsInsecure())
	{
		Con_Printf("Please use this command via the console\n");
		return;
	}
	if (*mapname)
		mod = NULL;//Mod_FindName(va("maps/%s", mapname));
	else
		mod = cl.worldmodel;
	if (!mod || mod->type == mod_dummy)
		return;
	hm = mod->terrain;
	if (!hm)
		return;

	fsize = 0;
	filename = va("maps/%s.r16", mapname);
	ctx.data = (void*)FS_LoadMallocFile(filename, &fsize);
	if (!ctx.data)
	{
		Con_Printf("Unable to read %s\n", filename);
		return;
	}
	ctx.width = ctx.height = sqrt(fsize/2);
	ctx.x = 0;
	ctx.y = 0;
	pos[0] += hm->sectionsize * CHUNKBIAS;
	pos[1] += hm->sectionsize * CHUNKBIAS;
	if (fsize == ctx.width*ctx.height*2)
		ted_itterate(hm, tid_flat, pos, max(ctx.width, ctx.height), 1, SECTHEIGHTSIZE, ted_import_heights_r16, &ctx);
	FS_FreeFile(ctx.data);
}
static void Mod_Terrain_Export_f(void)
{
	model_t *mod;
	struct ted_import_s ctx;
	char mapname[MAX_QPATH];
	const char *filename;
	heightmap_t *hm;
	size_t w, h;
	size_t tx, ty;
	size_t sx, sy;
	unsigned int outtilex=0,outtiley=0;
	qboolean populated;
	if (Cmd_IsInsecure())
	{
		Con_Printf("Please use this command via the console\n");
		return;
	}
	if (*Cmd_Argv(1))
		mod = NULL;//Mod_FindName(va("maps/%s", mapname));
	else
		mod = cl.worldmodel;
	if (!mod || mod->type == mod_dummy)
		return;
	hm = mod->terrain;
	if (!hm)
		return;

	COM_StripExtension(mod->name, mapname, sizeof(mapname));

	ctx.x = hm->firstsegx * (SECTHEIGHTSIZE-1);
	w = (hm->maxsegx-hm->firstsegx) * (SECTHEIGHTSIZE-1) + 1;
	while(w)
	{
		ctx.width = w;
		if (ctx.width > 2048+1)
			ctx.width = 2048;

		outtiley = 0;
		ctx.y = hm->firstsegy * (SECTHEIGHTSIZE-1);
		h = (hm->maxsegy-hm->firstsegy) * (SECTHEIGHTSIZE-1) + 1;
		while(h)
		{
			ctx.height = h;
			if (ctx.height > 2048+1)
				ctx.height = 2048;

			populated = false;
			ctx.data = Z_Malloc(ctx.width*ctx.height*2);
			for (sy = ctx.y/(SECTHEIGHTSIZE-1); sy < (ctx.y+ctx.height + SECTHEIGHTSIZE-3)/(SECTHEIGHTSIZE-1); sy++)
			for (sx = ctx.x/(SECTHEIGHTSIZE-1); sx < (ctx.x+ctx.width  + SECTHEIGHTSIZE-3)/(SECTHEIGHTSIZE-1); sx++)
			{
				hmsection_t *s = Terr_GetSection(hm, sx, sy, TGS_WAITLOAD|TGS_ANYSTATE);
				if (s->loadstate == TSLS_FAILED)
				{	//we're doing this weirdly so we can destroy sections as we go.
					Terr_DestroySection(hm, s, true);
					s = NULL;
				}
				if (s)
				{
					populated = true;
					for (ty = 0; ty < SECTHEIGHTSIZE; ty++)
					{
						size_t y = sy*(SECTHEIGHTSIZE-1)+ty - ctx.y;
						if (y >= ctx.height)
							continue;
						for (tx = 0; tx < SECTHEIGHTSIZE; tx++)
						{
							size_t x = sx*(SECTHEIGHTSIZE-1)+tx - ctx.x;
							if (x >= ctx.width)
								continue;
							ctx.data[x + y*ctx.width] = s->heights[tx+y*SECTHEIGHTSIZE] / (8192.0/(1<<16));
						}
					}
					if (!(s->flags & TSF_EDITED))
						Terr_DestroySection(hm, s, true);
				}
				else
				{
					for (ty = 0; ty < SECTHEIGHTSIZE; ty++)
					{
						size_t y = sy*(SECTHEIGHTSIZE-1)+ty - ctx.y;
						if (y >= ctx.height)
							continue;
						for (tx = 0; tx < SECTHEIGHTSIZE; tx++)
						{
							size_t x = sx*(SECTHEIGHTSIZE-1)+tx - ctx.x;
							if (x >= ctx.width)
								continue;
							ctx.data[x + y*ctx.width] = hm->defaultgroundheight / (8192.0/(1<<16));
						}
					}
				}
			}

			filename = va("%s/x%u_y%u.r16", mapname, outtilex, outtiley);
			if (populated)
			{
				if (FS_WriteFile(filename, ctx.data, ctx.width*ctx.height*2, FS_GAMEONLY))
				{
					char sysname[1024];
					FS_NativePath(filename, FS_GAMEONLY, sysname, sizeof(sysname));
					Con_Printf("Wrote %s\n", sysname);
				}
				else
					Con_Printf("Unable to write %s\n", filename);
			}
			else
				Con_Printf("Skipping unpopulated %s\n", filename);
			Z_Free(ctx.data);

			outtiley++;
			ctx.y += ctx.height;
			h -= ctx.height;
		}
		outtilex++;
		ctx.x += ctx.width;
		w -= ctx.width;
	}
}
#endif

void Mod_Terrain_Create_f(void)
{
	int x,y;
	hmsection_t *s;
	heightmap_t *hm;
	char *mname;
	char *mapdesc;
	char *skyname;
	char *groundname;
	char *watername;
	char *groundheight;
	char *waterheight;
	char *seed;
	vfsfile_t *file;
	model_t mod;
	memset(&mod, 0, sizeof(mod));
	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s: NAME \"DESCRIPTION\" SKYNAME DEFAULTGROUNDTEX DEFAULTHEIGHT DEFAULTWATER DEFAULTWATERHEIGHT seed\nGenerates a fresh maps/foo.hmp file. You may wish to edit it with notepad later to customise it. You will need csaddon.dat in order to edit the actual terrain.\n", Cmd_Argv(0));
		return;
	}

	mapdesc = Cmd_Argv(2); if (!*mapdesc) mapdesc = Cmd_Argv(1);
	skyname = Cmd_Argv(3);
	groundname = Cmd_Argv(4);
	groundheight = Cmd_Argv(5);
	watername = Cmd_Argv(6);
	waterheight = Cmd_Argv(7);
	seed = Cmd_Argv(7);
	Mod_SetEntitiesString(&mod, va(
		"{\n"
			"classname \"worldspawn\"\n"
			"message \"%s\"\n"
			"_sky \"%s\"\n"
			"_fog 0.02\n"
			"_maxdrawdist 0 /*overrides fog distance (if greater)*/\n"
			"_segmentsize 1024 /*how big each section is. this affects texturing and resolutions*/\n"
			"_minxsegment -2048\n"
			"_minysegment -2048\n"
			"_maxxsegment 2048\n"
			"_maxysegment 2048\n"
			"_seed \"%s\" /*for auto-gen plugins*/\n"
			"_exterior solid\n"
			"_defaultgroundtexture \"%s\"\n"
			"_defaultgroundheight \"%s\"\n"
			"_defaultwatertexture \"%s\"\n"
			"_defaultwaterheight \"%s\"\n"	//hurrah, sea level.
//			"_tiles 64 64 8 8\n"
		"}\n"
		"{\n"
			"classname info_player_start\n"
			"origin \"0 0 1024\" /*EDITME*/\n"
		"}\n"
		"/*ADD EXTRA ENTITIES!*/\n"
		, mapdesc
		,*skyname?skyname:"terrsky1", seed
		,*groundname?groundname:"ground1_1"
		,*groundheight?groundheight:"-1024"
		,*watername?watername:"*water2"
		,*waterheight?waterheight:"0"
		), true);

	mod.type = mod_heightmap;
	mod.terrain = hm = Z_Malloc(sizeof(*hm));
	Terr_ParseEntityLump(&mod, hm);
	hm->entitylock = Sys_CreateMutex();
	ClearLink(&hm->recycle);
	Q_strncpyz(hm->path, Cmd_Argv(1), sizeof(hm->path));
	Q_strncpyz(hm->groundshadername, "terrainshader", sizeof(hm->groundshadername));
	hm->exteriorcontents = FTECONTENTS_SOLID;


	for (x = CHUNKBIAS-1; x < CHUNKBIAS+1; x++)
		for (y = CHUNKBIAS-1; y < CHUNKBIAS+1; y++)
			Terr_GetSection(hm, x, y, TGS_TRYLOAD|TGS_DEFAULTONFAIL);

	for (x = CHUNKBIAS-1; x < CHUNKBIAS+1; x++)
		for (y = CHUNKBIAS-1; y < CHUNKBIAS+1; y++)
		{
			s = Terr_GetSection(hm, x, y, TGS_WAITLOAD|TGS_DEFAULTONFAIL);
			if (s && (s->flags & (TSF_EDITED|TSF_DIRTY)))
			{
				Terr_InitLightmap(s, false);
				Terr_SaveSection(hm, s, x, y, true);
			}
		}

	mname = va("maps/%s.hmp", Cmd_Argv(1));
	if (COM_FCheckExists(mname))
	{
		Con_Printf("%s: already exists, not overwriting.\n", mname);
		return;
	}
	FS_CreatePath(mname, FS_GAMEONLY);
	file = FS_OpenVFS(mname, "wb", FS_GAMEONLY);
	if (!file)
		Con_TPrintf("unable to open %s\n", mname);
	else
	{
		Terr_WriteMapFile(file, &mod);
		VFS_CLOSE(file);
		Con_TPrintf("Wrote %s\n", mname);
		FS_FlushFSHashWritten(mname);
	}
	Mod_SetEntitiesString(&mod, NULL, false);
	Terr_FreeModel(&mod);
}
#endif
//reads in the terrain a tile at a time, and writes it out again.
//the new version will match our current format version.
//this is mostly so I can strip out old format revisions...
#ifdef HAVE_CLIENT
void Mod_Terrain_Convert_f(void)
{
	model_t *mod;
	heightmap_t *hm;
	if (Cmd_FromGamecode())
		return;

	if (Cmd_Argc() >= 2)
		mod = Mod_FindName(va("maps/%s.hmp", Cmd_Argv(1)));
	else if (cls.state)
		mod = cl.worldmodel;
	else
		mod = NULL;
	if (!mod || mod->type == mod_dummy)
		return;
	hm = mod->terrain;
	if (!hm)
		return;

	{
		char *texkill = Cmd_Argv(2);
		hmsection_t *s;
		int x, sx;
		int y, sy;

		while(Terr_Collect(hm))	//collect as many as we can now, so when we collect later, the one that's collected is fresh.
			;
		for (y = hm->firstsegy; y < hm->maxsegy; y+=SECTIONSPERBLOCK)
		{
			Sys_Printf("%g%% complete\n", 100 * (y-hm->firstsegy)/(float)(hm->maxsegy-hm->firstsegy));
			for (x = hm->firstsegx; x < hm->maxsegx; x+=SECTIONSPERBLOCK)
			{
				for (sy = y; sy < y+SECTIONSPERBLOCK && sy < hm->maxsegy; sy++)
				{
					for (sx = x; sx < x+SECTIONSPERBLOCK && sx < hm->maxsegx; sx++)
					{
						s = Terr_GetSection(hm, sx, sy, TGS_WAITLOAD|TGS_NODOWNLOAD|TGS_NORENDER);
						if (s)
						{
							if (*texkill)
								ted_texkill(s, texkill);
							s->flags |= TSF_EDITED;
						}
					}
				}
				for (sy = y; sy < y+SECTIONSPERBLOCK && sy < hm->maxsegy; sy++)
				{
					for (sx = x; sx < x+SECTIONSPERBLOCK && sx < hm->maxsegx; sx++)
					{
						s = Terr_GetSection(hm, sx, sy, TGS_WAITLOAD|TGS_NODOWNLOAD|TGS_NORENDER);
						if (s)
						{
							if (s->flags & TSF_EDITED)
							{
								if (Terr_SaveSection(hm, s, sx, sy, true))
								{
									s->flags &= ~TSF_EDITED;
								}
							}
						}
					}
				}
				while(Terr_Collect(hm))
					;
			}
		}
		Sys_Printf("%g%% complete\n", 100.0f);
	}
}
#endif
void Mod_Terrain_Reload_f(void)
{
	model_t *mod;
	heightmap_t *hm;
	if (Cmd_Argc() >= 2)
		mod = Mod_FindName(va("maps/%s.hmp", Cmd_Argv(1)));
#ifdef HAVE_CLIENT
	else if (cls.state)
		mod = cl.worldmodel;
#endif
	else
		mod = NULL;
	if (!mod || mod->type == mod_dummy)
		return;
	hm = mod->terrain;
	if (!hm)
		return;

	if (Cmd_Argc() >= 4)
	{
		hmsection_t *s;
		int sx = atoi(Cmd_Argv(2)) + CHUNKBIAS;
		int sy = atoi(Cmd_Argv(3)) + CHUNKBIAS;
		if (hm)
		{
			s = Terr_GetSection(hm, sx, sy, TGS_NOLOAD);
			if (s)
			{
				s->flags |= TSF_NOTIFY;
			}
		}
	}
	else
		Terr_PurgeTerrainModel(mod, false, true);
}

plugterrainfuncs_t *Terr_GetTerrainFuncs(size_t structsize)
{
	if (structsize != sizeof(plugterrainfuncs_t))
		return NULL;
#ifdef HAVE_CLIENT
	return &terrainfuncs;
#else
	return NULL;	//dedicated server builds have all the visual stuff stripped, which makes APIs too inconsistent. Generate then save. Or fix up the API...
#endif
}

void Terr_Init(void)
{
	terrainfuncs.GenerateWater = Terr_GenerateWater;
	terrainfuncs.InitLightmap = Terr_InitLightmap;
	terrainfuncs.AddMesh = Terr_AddMesh;
	terrainfuncs.GetLightmap = Terr_GetLightmap;
	terrainfuncs.GetSection = Terr_GetSection;
	terrainfuncs.GenerateSections = Terr_GenerateSections;
	terrainfuncs.FinishedSection = Terr_FinishedSection;

	Cvar_Register(&mod_terrain_networked, "Terrain");
	Cvar_Register(&mod_terrain_defaulttexture, "Terrain");
	Cvar_Register(&mod_terrain_savever, "Terrain");
	Cmd_AddCommand("mod_terrain_save", Mod_Terrain_Save_f);
	Cmd_AddCommand("mod_terrain_reload", Mod_Terrain_Reload_f);
#ifdef HAVE_CLIENT
//	Cmd_AddCommandD("mod_terrain_export", Mod_Terrain_Export_f, "Export a raw heightmap");
//	Cmd_AddCommandD("mod_terrain_import", Mod_Terrain_Import_f, "Import a raw heightmap");
	Cmd_AddCommand("mod_terrain_create", Mod_Terrain_Create_f);
	Cmd_AddCommandD("mod_terrain_convert", Mod_Terrain_Convert_f, "mod_terrain_convert [mapname] [texkill]\nConvert a terrain to the current format. If texkill is specified, only tiles with the named texture will be converted, and tiles with that texture will be stripped. This is a slow operation.");

	Cvar_Register(&mod_terrain_sundir, "Terrain");
	Cvar_Register(&mod_terrain_ambient, "Terrain");
	Cvar_Register(&mod_terrain_shadows, "Terrain");
	Cvar_Register(&mod_terrain_shadow_dist, "Terrain");
	Cvar_Register(&mod_terrain_brushlights, "Terrain");
	Cvar_Register(&mod_terrain_brushtexscale, "Terrain");
#endif

	Mod_RegisterModelFormatText(NULL, "FTE Heightmap Map (hmp)", "terrain", Terr_LoadTerrainModel);
	Mod_RegisterModelFormatText(NULL, "Quake Map Format (map)", "{", Terr_LoadTerrainModel);
}
#endif
