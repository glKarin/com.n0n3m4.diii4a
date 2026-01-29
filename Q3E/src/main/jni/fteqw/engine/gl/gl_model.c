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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "fs.h"
#include "com_bih.h"
#if 1//ndef SERVERONLY	//FIXME
#include "glquake.h"
#include "com_mesh.h"

extern cvar_t r_shadow_bumpscale_basetexture;
extern cvar_t r_replacemodels;
extern cvar_t r_lightmap_average;
cvar_t mod_loadentfiles						= CVAR("sv_loadentfiles", "1");
cvar_t mod_loadentfiles_dir					= CVAR("sv_loadentfiles_dir", "");
cvar_t mod_external_vis						= CVARD("mod_external_vis", "1", "Attempt to load .vis patches for quake maps, allowing transparent water to work properly.");
cvar_t mod_warnmodels						= CVARD("mod_warnmodels", "1", "Warn if any models failed to load. Set to 0 if your mod is likely to lack optional models (like its in development).");	//set to 0 for hexen2 and its otherwise-spammy-as-heck demo.
cvar_t mod_litsprites_force					= CVARFD("mod_litsprites_force", "0", CVAR_RENDERERLATCH, "If set to 1, sprites will be lit according to world lighting (including rtlights), like Tenebrae. Ideally use EF_ADDITIVE or EF_FULLBRIGHT to make emissive sprites instead.");
cvar_t mod_loadmappackages					= CVARD ("mod_loadmappackages", "1", "Load additional content embedded within bsp files.");
cvar_t mod_lightscale_broken				= CVARFD("mod_lightscale_broken", "0", CVAR_RENDERERLATCH, "When active, replicates a bug from vanilla - the radius of r_dynamic lights is scaled by per-surface texture scale rather than using actual distance.");
cvar_t mod_lightpoint_distance				= CVARD("mod_lightpoint_distance", "8192", "This is the maximum distance to trace when searching for a ground surface for lighting info on map formats without light more fancy lighting info. Use 2048 for full compat with Quake.");
#ifdef SPRMODELS
cvar_t r_sprite_backfacing					= CVARD	("r_sprite_backfacing", "0", "Make oriented sprites face backwards relative to their orientation, for compat with q1.");
#endif
cvar_t r_nolerp_list						= CVARFD ("r_nolerp_list"/*qs*/, "", CVAR_RENDERERLATCH, "Models in this list will not interpolate. Any models included here should be considered bad.");
#ifdef RTLIGHTS
cvar_t r_noshadow_list						= CVARAFD ("r_noshadow_list"/*qs*/, "r_noEntityCastShadowList", "progs/missile.mdl,progs/flame.mdl,progs/flame2.mdl,progs/lavaball.mdl,progs/grenade.mdl,progs/spike.mdl,progs/s_spike.mdl,progs/laser.mdl,progs/lspike.mdl,progs/candle.mdl", CVAR_RENDERERLATCH, "Models in this list will not cast shadows.");
#endif
#ifdef SERVERONLY
cvar_t gl_overbright, gl_specular, gl_load24bit, r_replacemodels, gl_miptexLevel, r_fb_bmodels;	//all of these can/should default to 0
cvar_t r_noframegrouplerp					= CVARF  ("r_noframegrouplerp", "0", CVAR_ARCHIVE);
cvar_t dpcompat_psa_ungroup					= CVAR  ("dpcompat_psa_ungroup", "0");
texture_t	r_notexture_mip_real;
texture_t	*r_notexture_mip = &r_notexture_mip_real;
#endif

void CM_Init(void);

void Mod_LoadSpriteShaders(model_t *spr);
qboolean QDECL Mod_LoadSpriteModel (model_t *mod, void *buffer, size_t fsize);
qboolean QDECL Mod_LoadSprite2Model (model_t *mod, void *buffer, size_t fsize);
#ifdef Q1BSPS
static qboolean QDECL Mod_LoadBrushModel (model_t *mod, void *buffer, size_t fsize);
#endif
#if defined(Q2BSPS) || defined(Q3BSPS)
qboolean QDECL Mod_LoadQ2BrushModel (model_t *mod, void *buffer, size_t fsize);
#endif
model_t *Mod_LoadModel (model_t *mod, enum mlverbosity_e verbose);
static void Mod_PrintFormats_f(void);
static void Mod_ShowEnt_f(void);
static void Mod_SaveEntFile_f(void);

#ifdef MAP_DOOM
qboolean QDECL Mod_LoadDoomLevel(model_t *mod, void *buffer, size_t fsize);
#endif

#ifdef DSPMODELS
void Mod_LoadDoomSprite (model_t *mod);
#endif

#define	MAX_MOD_KNOWN	8192
model_t	*mod_known;
int		mod_numknown;
char mod_modifier[MAX_QPATH];	//postfix for ent files

extern cvar_t r_loadlits;
#ifdef SPECULAR
extern cvar_t gl_specular;
#endif
extern cvar_t r_fb_bmodels;
void Mod_SortShaders(model_t *mod);
void Mod_LoadAliasShaders(model_t *mod);

#ifdef RUNTIMELIGHTING
extern model_t *lightmodel;
#endif

static void Mod_MemList_f(void)
{
	int m;
	model_t *mod;
	int total = 0;
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (mod->memgroup.totalbytes)
			Con_Printf("%s: %i bytes\n", mod->name, mod->memgroup.totalbytes);
		total += mod->memgroup.totalbytes;
	}
	Con_Printf("Total: %i bytes\n", total);
}
#ifndef SERVERONLY
static void Mod_BatchList_f(void)
{
	int m, i;
	model_t *mod;
	batch_t *batch;
	unsigned int count;
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (mod->type == mod_brush && mod->loadstate == MLS_LOADED)
		{
			Con_Printf("^1%s:\n", mod->name);
			count = 0;
			for (i = 0; i < SHADER_SORT_COUNT; i++)
			{
				for (batch = mod->batches[i]; batch; batch = batch->next)
				{
					char editname[MAX_QPATH];
					char *body = Shader_GetShaderBody(batch->texture->shader, editname, sizeof(editname));
					if (!body)
						body = "SHADER NOT KNOWN";
					else
					{
						char *cr;
						while ((cr = strchr(body, '\r')))
							*cr = ' ';
					}
					Con_Printf("  ^[%s\\tipimg\\%s\\tipimgtype\\%i\\tip\\{%s^]", batch->texture->shader->name, batch->texture->shader->name, batch->texture->shader->usageflags, body);

#if MAXRLIGHTMAPS > 1
					if (batch->lightmap[3] >= 0)
						Con_Printf("^2 lm=(%i:%i %i:%i %i:%i %i:%i)", batch->lightmap[0], batch->lmlightstyle[0], batch->lightmap[1], batch->lmlightstyle[1], batch->lightmap[2], batch->lmlightstyle[2], batch->lightmap[3], batch->lmlightstyle[3]);
					else if (batch->lightmap[2] >= 0)
						Con_Printf("^2 lm=(%i:%i %i:%i %i:%i)", batch->lightmap[0], batch->lmlightstyle[0], batch->lightmap[1], batch->lmlightstyle[1], batch->lightmap[2], batch->lmlightstyle[2]);
					else if (batch->lightmap[1] >= 0)
						Con_Printf("^2 lm=(%i:%i %i:%i)", batch->lightmap[0], batch->lmlightstyle[0], batch->lightmap[1], batch->lmlightstyle[1]);
					else
#endif
						if (batch->lmlightstyle[0] != INVALID_LIGHTSTYLE)
						Con_Printf("^2 lm=(%i:%i)", batch->lightmap[0], batch->lmlightstyle[0]);
					else
						Con_Printf("^2 lm=%i", batch->lightmap[0]);
					count++;

					if (batch->envmap)
						Con_Printf("^3 envmap=%s", batch->envmap->ident);
					Con_Printf(" surfs=%u\n", batch->maxmeshes);
				}
			}
			Con_Printf("^h(%u batches, lm %i*%i, lux %s)\n", count, mod->lightmaps.width, mod->lightmaps.height, mod->lightmaps.deluxemapping?"true":"false");
		}
	}
}

static void Mod_TextureList_f(void)
{
	int m, i;
	texture_t *tx;
	model_t *mod;
	qboolean shownmodelname = false;
	int count = 0;
	char *body;
	char editname[MAX_OSPATH];
	int preview = (Cmd_Argc()==1)?8:atoi(Cmd_Argv(1));

	int s;
	batch_t *batch;
	unsigned int batchcount;

	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (shownmodelname)
			Con_Printf("(%u textures)\n", count);
		shownmodelname = false;

		if (mod->type == mod_brush && mod->loadstate == MLS_LOADED)
		{
			if (*mod->name == '*')
				continue;//	inlines don't count
			count = 0;
			for (i = 0; i < mod->numtextures; i++)
			{
				tx = mod->textures[i];
				if (!tx)
					continue;	//happens on e1m2

				batchcount = 0;
				for (s = 0; s < SHADER_SORT_COUNT; s++)
				{
					for (batch = mod->batches[s]; batch; batch = batch->next)
					{
						if (batch->texture == tx)
							batchcount++;
					}
				}
	//			if (!batchcount)
//					continue; //not actually used...

				if (!shownmodelname)
				{
					shownmodelname = true;
					Con_Printf("%s\n", mod->name);
					count = 0;
				}

				body = Shader_GetShaderBody(tx->shader, editname, sizeof(editname));
				if (!body)
					body = "SHADER NOT KNOWN";
				else
				{
					char *cr;
					while ((cr = strchr(body, '\r')))
						*cr = ' ';
					if (strlen(body) > 3000)
						body[3000] = 0;	//arbitrary cut off, to avoid console glitches with big shaders.
				}

				if (preview)
				{
					if (*editname)
						Con_Printf("^[\\edit\\%s\\img\\%s\\imgtype\\%i\\s\\%i\\tip\\{%s^]", editname, tx->name, tx->shader->usageflags, preview, body);
					else
						Con_Printf("^[\\img\\%s\\imgtype\\%i\\s\\%i\\tip\\{%s^]", tx->shader->name, tx->shader->usageflags, preview, body);
				}
				if (*editname)
					Con_Printf("  ^[%s\\edit\\%s\\tipimg\\%s\\tipimgtype\\%i\\tip\\{%s^] (%u batches)\n", tx->name, editname, tx->name, tx->shader->usageflags, body, batchcount);
				else
					Con_Printf("  ^[%s\\tipimg\\%s\\tipimgtype\\%i\\tip\\{%s^] (%u batches)\n", tx->name, tx->shader->name, tx->shader->usageflags, body, batchcount);
				count++;
			}
		}
	}
	if (shownmodelname)
		Con_Printf("(%u textures)\n", count);
}

static void Mod_BlockTextureColour_f (void)
{
	char texname[64];
	model_t *mod;
	texture_t *tx;
//	shader_t *s;
	char *match = Cmd_Argv(1);

	int i, m;
//	unsigned int colour[8*8];
	unsigned int rgba;

	((qbyte *)&rgba)[0] = atoi(Cmd_Argv(2));
	((qbyte *)&rgba)[1] = atoi(Cmd_Argv(3));
	((qbyte *)&rgba)[2] = atoi(Cmd_Argv(4));
	((qbyte *)&rgba)[3] = 255;

	sprintf(texname, "purergb_%i_%i_%i", (int)((char *)&rgba)[0], (int)((char *)&rgba)[1], (int)((char *)&rgba)[2]);
/*	s = R_RegisterCustom(Cmd_Argv(2), SUF_LIGHTMAP, NULL, NULL);
	if (!s)
	{
		s = R_RegisterCustom (texname, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);

		for (i = 0; i < sizeof(colour)/sizeof(colour[0]); i++)
			colour[i] = rgba;
		s->defaulttextures.base = GL_LoadTexture32(texname, 8, 8, colour, IF_NOMIPMAP);
	}
*/
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (mod->type == mod_brush && mod->loadstate == MLS_LOADED)
		{
			for (i = 0; i < mod->numtextures; i++)
			{
				tx = mod->textures[i];
				if (!tx)
					continue;	//happens on e1m2

				if (!stricmp(tx->name, match))
					tx->shader->defaulttextures->base = Image_GetTexture(texname, NULL, IF_NOMIPMAP|IF_NEAREST, &rgba, NULL, 1, 1, TF_BGRA32);
			}
		}
	}
}
#endif

void Mod_RebuildLightmaps (void)
{
	int i, j;
	msurface_t *surf;
	model_t	*mod;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate != MLS_LOADED)
			continue;

		if (mod->type == mod_brush)
		{
			for (j=0, surf = mod->surfaces; j<mod->numsurfaces ; j++, surf++)
				surf->cached_dlight=-1;//force it
		}
	}
}

#ifdef HAVE_CLIENT
void Mod_ResortShaders(void)
{
	//called when some shader changed its sort key.
	//this means we have to hunt down all models and update their batches.
	//really its only bsps that need this.
	batch_t *oldlists[SHADER_SORT_COUNT], *b;
	int i, j, bs;
	model_t	*mod;
	for (i=0, mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate != MLS_LOADED)
			continue;

		memcpy(oldlists, mod->batches, sizeof(oldlists));
		memset(mod->batches, 0, sizeof(oldlists));
		mod->numbatches = 0;	//this is a bit of a misnomer. clearing this will cause it to be recalculated, with everything renumbered as needed.
	
		for (j = 0; j < SHADER_SORT_COUNT; j++)
		{
			while((b=oldlists[j]))
			{
				oldlists[j] = b->next;
				bs = b->shader?b->shader->sort:j;

				b->next = mod->batches[bs];
				mod->batches[bs] = b;
			}
		}
	}

	Surf_ClearSceneCache();	//make sure their caches are updated.
}
#endif

const char *Mod_GetEntitiesString(model_t *mod)
{
	size_t vl;
	size_t e;
	size_t sz;
	char *o;
	if (!mod)
		return NULL;
	if (mod->entities_raw)	//still cached/correct
		return mod->entities_raw;
	if (!mod->numentityinfo)
	{
		if (mod->loadstate != MLS_LOADED)
			return NULL;
		mod->entities_raw = FS_LoadMallocFile(va("%s.ent", mod->name), NULL);
		if (!mod->entities_raw)
			mod->entities_raw = FS_LoadMallocFile(va("%s.ent", mod->name), NULL);
		return mod->entities_raw;
	}

	//reform the entities back into a full string now that we apparently need it
	//find needed buffer size
	for (e = 0, sz = 0; e < mod->numentityinfo; e++)
	{
		if (!mod->entityinfo[e].keyvals)
			continue;
		sz += 2;
		sz += strlen(mod->entityinfo[e].keyvals);
		sz += 2;
	}
	sz+=1;
	o = BZ_Malloc(sz);

	//splurge it out
	for (e = 0, sz = 0; e < mod->numentityinfo; e++)
	{
		if (!mod->entityinfo[e].keyvals)
			continue;
		o[sz+0] = '{';
		o[sz+1] = '\n';
		sz += 2;
		vl = strlen(mod->entityinfo[e].keyvals);
		memcpy(&o[sz], mod->entityinfo[e].keyvals, vl);
		sz += vl;
		o[sz+0] = '}';
		o[sz+1] = '\n';
		sz += 2;
	}
	o[sz+0] = 0;

	mod->entities_raw = o;
	return mod->entities_raw;
}
void Mod_SetEntitiesString(model_t *mod, const char *str, qboolean docopy)
{
	size_t j;
	for (j = 0; j < mod->numentityinfo; j++)
		Z_Free(mod->entityinfo[j].keyvals);
	mod->numentityinfo = 0;
	Z_Free(mod->entityinfo);
	mod->entityinfo = NULL;
	Z_Free((char*)mod->entities_raw);
	mod->entities_raw = NULL;

	if (str)
	{
		if (docopy)
			str = Z_StrDup(str);
		mod->entities_raw = str;
	}
}

void Mod_SetEntitiesStringLen(model_t *mod, const char *str, size_t strsize)
{
	if (str)
	{
		char *cpy = BZ_Malloc(strsize+1);
		memcpy(cpy, str, strsize);
		cpy[strsize] = 0;
		Mod_SetEntitiesString(mod, cpy, false);
	}
	else
		Mod_SetEntitiesString(mod, str, false);
}

void Mod_ParseEntities(model_t *mod)
{
	char key[1024];
	char value[4096];
	const char *entstart;
	const char *entend;
	const char *entdata;
	size_t c, m;

	c = 0; m = 0;

	while (mod->numentityinfo > 0)
		Z_Free(mod->entityinfo[--mod->numentityinfo].keyvals);
	Z_Free(mod->entityinfo);
	mod->entityinfo = NULL;

	entdata = Mod_GetEntitiesString(mod);
	while(1)
	{
		if (!(entdata=COM_ParseOut(entdata, key, sizeof(key))))
			break;
		if (strcmp(key, "{"))
			break;

		//skip whitespace to save space.
		while (*entdata == ' ' || *entdata == '\r' || *entdata == '\n' || *entdata == '\t')
			entdata++;

		entstart = entdata;

		while(1)
		{
			entend = entdata;
			entdata=COM_ParseOut(entdata, key, sizeof(key));
			if (!strcmp(key, "}"))
				break;
			entdata=COM_ParseOut(entdata, value, sizeof(value));
		}
		if (!entdata)
			break;	//erk. eof

		if (c == m)
		{
			if (!m)
				m = 64;
			else
				m *= 2;
			mod->entityinfo = BZ_Realloc(mod->entityinfo, sizeof(*mod->entityinfo) * m);
		}
		mod->entityinfo[c].id = c+1;
		mod->entityinfo[c].keyvals = BZ_Malloc(entend-entstart + 1);
		memcpy(mod->entityinfo[c].keyvals, entstart, entend-entstart);
		mod->entityinfo[c].keyvals[entend-entstart] = 0;
		c++;
	}
	mod->numentityinfo = c;
}


void Mod_LoadMapArchive(model_t *mod, void *archivedata, size_t archivesize)
{
	if (archivesize && mod && !mod->archive && mod_loadmappackages.ival)
	{
		vfsfile_t *f = VFSPIPE_Open(1,true);
		if (f)
		{
			VFS_WRITE(f, archivedata, archivesize);
			mod->archive = FSZIP_LoadArchive(f, NULL, mod->name, mod->name, NULL);
			if (mod->archive)
				FS_LoadMapPackFile(mod->name, mod->archive);	//give it to the filesystem to use.
			else
				VFS_CLOSE(f);	//give up.
		}
	}
}

/*
===================
Mod_ClearAll
===================

called before new content is loaded.
*/
static int mod_datasequence;
void Mod_ClearAll (void)
{
#ifdef RUNTIMELIGHTING
	RelightTerminate(NULL);
#endif

	mod_datasequence++;
}

qboolean Mod_PurgeModel(model_t	*mod, enum mod_purge_e ptype)
{
	if (mod->loadstate == MLS_LOADING)
	{
		if (ptype == MP_MAPCHANGED && !mod->submodelof)
			return false;	//don't bother waiting for it on map changes.
		COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
	}

#ifdef RUNTIMELIGHTING
	RelightTerminate(mod);
#endif

#ifdef TERRAIN
	//we can safely flush all terrain sections at any time
	if (mod->terrain)
	{
		if (ptype == MP_MAPCHANGED)
			return false;	//don't destroy any data there that the user might want to save. FIXME: handle better.
		Terr_PurgeTerrainModel(mod, false, true);
	}
#endif

	//purge any vbos
	if (mod->type == mod_brush)
	{
		if (ptype == MP_FLUSH)
			return false;
#ifndef SERVERONLY
		Surf_Clear(mod);
#endif
	}

#ifdef TERRAIN
	if (mod->type == mod_brush || mod->type == mod_heightmap)
		Terr_FreeModel(mod);
#endif
	if (mod->type == mod_alias)
	{
		Mod_DestroyMesh(mod->meshinfo);
		mod->meshinfo = NULL;
	}

	Mod_SetEntitiesString(mod, NULL, false);

#ifdef PSET_SCRIPT
	PScript_ClearSurfaceParticles(mod);
#endif

	//and obliterate anything else remaining in memory.
	mod->meshinfo = NULL;
	if (mod->archive)
	{
		FS_CloseMapPackFile(mod->archive);
		mod->archive = NULL;
	}
	ZG_FreeGroup(&mod->memgroup);
	mod->loadstate = MLS_NOTLOADED;

	mod->submodelof = NULL;
	mod->pvs = NULL;
	mod->phs = NULL;

#ifndef CLIENTONLY
	sv.world.lastcheckpvs = NULL;	//if the server has that cached, flush it just in case.
#endif

	return true;
}

//can be called in one of two ways.
//force=true: explicit flush. everything goes, even if its still in use.
//force=false: map change. lots of stuff is no longer in use and can be freely flushed.
//certain models cannot be safely flushed while still in use. such models will not be flushed even if forced (they may still be partially flushed).
void Mod_Purge(enum mod_purge_e ptype)
{
	int		i;
	model_t	*mod;
	qboolean unused;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		unused = mod->datasequence != mod_datasequence;

		if (mod->loadstate == MLS_NOTLOADED)
			continue;

		//this model isn't active any more.
		if (unused || ptype != MP_MAPCHANGED)
		{
			if (unused)
				Con_DLPrintf(2, "model \"%s\" no longer needed\n", mod->name);
			Mod_PurgeModel(mod, (ptype==MP_FLUSH && unused)?MP_RESET:ptype);
		}
#if defined(HALFLIFEMODELS) && defined(HAVE_CLIENT)
		else if (mod->type == mod_halflife)
			R_HalfLife_TouchTextures(mod);
#endif
	}
}

void Mod_SetModifier(const char *modifier)
{
	if (!modifier || strlen(modifier) >= sizeof(mod_modifier)) modifier = "";
	if (strcmp(modifier, mod_modifier))
	{	//if the modifier changed, force all models to reset.
		COM_WorkerFullSync();	//sync all the workers, just in case.
		strcpy(mod_modifier, modifier);
		Mod_Purge(MP_RESET);	//nuke it now
	}
}

#ifndef SERVERONLY
void Mod_BSPX_Init(void);
#endif

/*
===============
Mod_Init
===============
*/
void Mod_Init (qboolean initial)
{
	if (!mod_known)
		mod_known = malloc(MAX_MOD_KNOWN * sizeof(*mod_known));
	if (!initial)
	{
		Mod_ClearAll();	//shouldn't be needed
		Mod_Purge(MP_RESET);//shouldn't be needed
		mod_numknown = 0;
#ifdef Q1BSPS
		Q1BSP_Init();
#endif

		Cmd_AddCommand("mod_memlist", Mod_MemList_f);
#ifndef SERVERONLY
		Cmd_AddCommand("mod_batchlist", Mod_BatchList_f);
		Cmd_AddCommand("mod_texturelist", Mod_TextureList_f);
		Cmd_AddCommand("mod_usetexture", Mod_BlockTextureColour_f);
#endif
	}
	else
	{
		Cvar_Register(&mod_external_vis, "Graphical Nicaties");
		Cvar_Register(&mod_warnmodels, "Graphical Nicaties");
		Cvar_Register(&mod_litsprites_force, "Graphical Nicaties");
		Cvar_Register(&mod_loadentfiles, NULL);
		Cvar_Register(&mod_loadentfiles_dir, NULL);
		Cvar_Register(&mod_loadmappackages, NULL);
		Cvar_Register(&mod_lightscale_broken, NULL);
		Cvar_Register(&mod_lightpoint_distance, NULL);
		Cvar_Register (&r_meshpitch, "Gamecode");
		Cvar_Register (&r_meshroll, "Gamecode");
		Cvar_Register(&r_nolerp_list, "Graphical Nicaties");
#ifdef RTLIGHTS
		Cvar_Register(&r_noshadow_list, "Graphical Nicaties");
#endif
		Cmd_AddCommandD("sv_saveentfile", Mod_SaveEntFile_f, "Dumps a copy of the map's entities to disk, so that it can be edited and used as a replacement for slightly customised maps.");
		Cmd_AddCommandD("mod_showent", Mod_ShowEnt_f, "Allows you to quickly search through a map's entities.");
		Cmd_AddCommand("version_modelformats", Mod_PrintFormats_f);

#ifdef SPRMODELS
		Cvar_Register (&r_sprite_backfacing, NULL);
#endif
#ifndef SERVERONLY
		Mod_BSPX_Init();
#endif
	}

	if (initial)
	{
		Alias_Register();

#ifdef SPRMODELS
		Mod_RegisterModelFormatMagic(NULL, "Quake1 Sprite (spr)",			IDSPRITEHEADER,							Mod_LoadSpriteModel);
#endif
#ifdef SP2MODELS
		Mod_RegisterModelFormatMagic(NULL, "Quake2 Sprite (sp2)",			IDSPRITE2HEADER,						Mod_LoadSprite2Model);
#endif

		//q2/q3bsps
#ifdef Q3BSPS
		Mod_RegisterModelFormatMagic(NULL, "RTCW Map (bsp)",				"IBSP\57\0\0\0",8,						Mod_LoadQ2BrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake3 Map (bsp)",				"IBSP\56\0\0\0",8,						Mod_LoadQ2BrushModel);
#endif
#ifdef Q2BSPS
		Mod_RegisterModelFormatMagic(NULL, "Quake2 Map (bsp)",				"IBSP\46\0\0\0",8,						Mod_LoadQ2BrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake2World Map (bsp)",			"IBSP\105\0\0\0",8,						Mod_LoadQ2BrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Qbism (Quake2) Map (bsp)",		"QBSP\46\0\0\0",8,						Mod_LoadQ2BrushModel);
#endif
#ifdef RFBSPS
		Mod_RegisterModelFormatMagic(NULL, "Raven Map (bsp)",				"RBSP\1\0\0\0",8,	Mod_LoadQ2BrushModel);
		Mod_RegisterModelFormatMagic(NULL, "QFusion Map (bsp)",				"FBSP\1\0\0\0",8,	Mod_LoadQ2BrushModel);
#endif

		//doom maps
#ifdef MAP_DOOM
		Mod_RegisterModelFormatMagic(NULL, "Doom IWad Map",					"IWAD",4,		Mod_LoadDoomLevel);
		Mod_RegisterModelFormatMagic(NULL, "Doom PWad Map",					"PWAD",4,		Mod_LoadDoomLevel);
#endif

#ifdef MAP_PROC
		Mod_RegisterModelFormatText(NULL, "Doom3 (cm)",						"CM",									D3_LoadMap_CollisionMap);
#endif

#ifdef Q1BSPS
		//q1-based formats
		Mod_RegisterModelFormatMagic(NULL, "Quake1 2PSB Map (bsp)",			BSPVERSION_LONG1,						Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake1 BSP2 Map (bsp)",			BSPVERSION_LONG2,						Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Half-Life Map (bsp)",			BSPVERSIONHL,							Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake1 Map (bsp)",				BSPVERSION,								Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake1 Prerelease Map (bsp)",	BSPVERSIONPREREL,						Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake 64 Remastered Map (bsp)", BSPVERSIONQ64,							Mod_LoadBrushModel);
#endif
	}
}

void Mod_Shutdown (qboolean final)
{
	if (final)
	{
		Mod_ClearAll();
		Mod_Purge(MP_RESET);

		Mod_UnRegisterAllModelFormats(NULL);
	}
	else
	{
		Mod_ClearAll();
		Mod_Purge(MP_RESET);

		Cmd_RemoveCommand("mod_memlist");
		Cmd_RemoveCommand("mod_batchlist");
		Cmd_RemoveCommand("mod_texturelist");
		Cmd_RemoveCommand("mod_usetexture");
	}
	free(mod_known);
	mod_known = NULL;
	mod_numknown = 0;

#ifndef SERVERONLY
	r_worldentity.model = NULL;	//just in case.
	cl_numvisedicts = 0;	//make sure nothing gets cached.
#endif
}

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	void	*r;
	
	r = mod->meshinfo;
	if (r)
		return r;

	Mod_LoadModel (mod, MLV_ERROR);
	
	if (!mod->meshinfo)
		Sys_Error ("Mod_Extradata: caching failed");
	return mod->meshinfo;
}

const char *Mod_FixName(const char *modname, const char *worldname)
{
	if (*modname == '*' && worldname && *worldname)
	{
		//make sure that the value is an inline value with no existing extra postfix or anything.
		char *e;
		if (strtoul(modname+1, &e, 10) != 0)
			if (!*e)
				return va("%s:%s", modname, worldname);
	}
	return modname;
}

//Called when the given file was (re)written.
//
void Mod_FileWritten (const char *filename)
{
	int		i;
	model_t	*mod;
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
		if (!strcmp (mod->publicname, filename) )
		{
			if (mod->loadstate != MLS_NOTLOADED)
				Mod_PurgeModel(mod, MP_RESET);
		}
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (const char *name)
{
	int		i;
	model_t	*mod;
	
//	if (!name[0])
//		Sys_Error ("Mod_ForName: NULL name");
		
//
// search the currently loaded models
//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
		if (!strcmp (mod->publicname, name) )
			break;
			
	if (i == mod_numknown)
	{
#ifdef LOADERTHREAD
		Sys_LockMutex(com_resourcemutex);
		for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
			if (!strcmp (mod->publicname, name) )
				break;
		if (i == mod_numknown)
		{
#endif
			if (mod_numknown == MAX_MOD_KNOWN)
			{
#ifdef LOADERTHREAD
				Sys_UnlockMutex(com_resourcemutex);
#endif
				Sys_Error ("mod_numknown == MAX_MOD_KNOWN");
				return NULL;
			}
			if (strlen(name) >= sizeof(mod->publicname))
			{
#ifdef LOADERTHREAD
				Sys_UnlockMutex(com_resourcemutex);
#endif
				Sys_Error ("model name is too long: %s", name);
				return NULL;
			}
			memset(mod, 0, sizeof(model_t));	//clear the old model as the renderers use the same globals
			Q_strncpyz (mod->publicname, name, sizeof(mod->publicname));
			Q_strncpyz (mod->name, name, sizeof(mod->name));
			mod->loadstate = MLS_NOTLOADED;
			mod_numknown++;
			mod->particleeffect = -1;
			mod->particletrail = -1;
#ifdef LOADERTHREAD
		}
		Sys_UnlockMutex(com_resourcemutex);
#endif
	}

//	if (mod->loadstate == MLS_FAILED)
//		mod->loadstate = MLS_NOTLOADED;

	//mark it as active, so it doesn't get flushed prematurely
	mod->datasequence = mod_datasequence;
	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel (const char *name)
{
	//findname does this anyway.
	Mod_FindName (name);
}

static struct
{
	void *module;
	char *formatname;
	char *ident;
	qbyte *magic;
	size_t magicsize;
	qboolean (QDECL *load) (model_t *mod, void *buffer, size_t buffersize);
} modelloaders[64];

static void Mod_PrintFormats_f(void)
{
	int i;
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].load && modelloaders[i].formatname)
			Con_Printf("%s\n", modelloaders[i].formatname);
	}
}

int Mod_RegisterModelFormatText(void *module, const char *formatname, char *magictext, qboolean (QDECL *load) (model_t *mod, void *buffer, size_t fsize))
{
	int i, free = -1;
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].ident && !strcmp(modelloaders[i].ident, magictext))
		{
			free = i;
			break;	//extension match always replaces
		}
		else if (!modelloaders[i].load && free < 0)
			free = i;
	}
	if (free < 0)
		return 0;

	modelloaders[free].module = module;
	modelloaders[free].formatname = Z_StrDup(formatname);
	modelloaders[free].magic = NULL;
	modelloaders[free].magicsize = 0;
	modelloaders[free].ident = Z_StrDup(magictext);
	modelloaders[free].load = load;

	return free+1;
}
int Mod_RegisterModelFormatMagic(void *module, const char *formatname, qbyte *magic, size_t magicsize, qboolean (QDECL *load) (model_t *mod, void *buffer, size_t fsize))
{
	int i, free = -1;
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].magic && modelloaders[i].magicsize == magicsize && !memcmp(modelloaders[i].magic, magic, magicsize))
		{
			free = i;
			break;	//extension match always replaces
		}
		else if (!modelloaders[i].load && free < 0)
			free = i;
	}
	if (free < 0)
		return 0;

	modelloaders[free].module = module;
	if (modelloaders[free].formatname)
		Z_Free(modelloaders[free].formatname);
	modelloaders[free].formatname = Z_StrDup(formatname);
	modelloaders[free].magic = magic;
	modelloaders[free].magicsize = magicsize;
	modelloaders[free].ident = NULL;
	modelloaders[free].load = load;

	return free+1;
}

void Mod_UnRegisterModelFormat(void *module, int idx)
{
	
	idx--;
	if ((unsigned int)(idx) >= sizeof(modelloaders)/sizeof(modelloaders[0]))
		return;
	if (modelloaders[idx].module != module)
		return;

	COM_WorkerFullSync();
	Z_Free(modelloaders[idx].ident);
	modelloaders[idx].ident = NULL;
	Z_Free(modelloaders[idx].formatname);
	modelloaders[idx].formatname = NULL;
	modelloaders[idx].magic = 0;
	modelloaders[idx].load = NULL;
	modelloaders[idx].module = NULL;

	//FS_Restart will be needed
}

void Mod_UnRegisterAllModelFormats(void *module)
{
	int i;
	COM_WorkerFullSync();
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].module == module)
			Mod_UnRegisterModelFormat(module, i+1);
	}
}

//main thread. :(
void Mod_ModelLoaded(void *ctx, void *data, size_t a, size_t b)
{
	qboolean previouslyfailed;
	model_t *mod = ctx;
	enum mlverbosity_e verbose = b;
#ifndef SERVERONLY
	P_LoadedModel(mod);
#endif

	previouslyfailed = mod->loadstate == MLS_FAILED;
	mod->loadstate = a;

#ifdef TERRAIN
	if (mod->terrain)
		Terr_FinishTerrain(mod);
#endif
#ifndef SERVERONLY
	if (mod->type == mod_brush)
		Surf_BuildModelLightmaps(mod);
	if (mod->type == mod_sprite)
	{
		Mod_LoadSpriteShaders(mod);
	}
	if (mod->type == mod_alias)
	{
		if (qrenderer != QR_NONE)
			Mod_LoadAliasShaders(mod);
	}

#ifdef RAGDOLL
	if (mod->type == mod_alias || mod->type == mod_halflife)
	{
		int numbones = Mod_GetNumBones(mod, false);
		if (numbones)
		{
			size_t filesize;
			char *buf;
			char dollname[MAX_QPATH];
			Q_snprintfz(dollname, sizeof(dollname), "%s.doll", mod->name);
			buf = FS_LoadMallocFile(dollname, &filesize);
			if (buf)
			{
				mod->dollinfo = rag_createdollfromstring(mod, dollname, numbones, buf);
				BZ_Free(buf);
			}
		}
	}
#endif

#endif

	switch(verbose)
	{
	default:
	case MLV_ERROR:
		Host_EndGame ("Mod_NumForName: %s not found or couldn't load", mod->name);
		break;
	case MLV_WARNSYNC:
	case MLV_WARN:
		if (*mod->name != '*' && strcmp(mod->name, "null") && mod_warnmodels.ival && !previouslyfailed)
			Con_Printf(CON_ERROR "Unable to load %s\n", mod->name);
		break;
	case MLV_SILENT:
	case MLV_SILENTSYNC:
		break;
	}
}
void Mod_SubmodelLoaded(model_t *mod, int state)
{
	COM_AddWork(WG_MAIN, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
}
/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
static void Mod_LoadModelWorker (void *ctx, void *data, size_t a, size_t b)
{
	model_t *mod = ctx;
	enum mlverbosity_e verbose = a;
	unsigned *buf = NULL;
	char mdlbase[MAX_QPATH];
	char *replstr;
#ifdef DSPMODELS
	qboolean doomsprite = false;
#endif
	unsigned int i;
	size_t filesize;
	char ext[8];
	int basedepth;

	//clear out any old state.
	memset(&mod->loadstate+1, 0, sizeof(*mod) - (qintptr_t)(&((model_t*)NULL)->loadstate+1));

	if (!*mod->publicname)
	{
		mod->type = mod_dummy;
		mod->mins[0] = -16;
		mod->mins[1] = -16;
		mod->mins[2] = -16;
		mod->maxs[0] = 16;
		mod->maxs[1] = 16;
		mod->maxs[2] = 16;
		mod->engineflags = 0;
		COM_AddWork(WG_MAIN, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
		return;
	}
	
#ifdef RAGDOLL
	if (mod->dollinfo)
	{
		rag_freedoll(mod->dollinfo);
		mod->dollinfo = NULL;
	}
#endif

	if (mod->loadstate == MLS_FAILED)
		return;

//
// load the file
//
	// set necessary engine flags for loading purposes
	if (!strcmp(mod->publicname, "progs/player.mdl"))
		mod->engineflags |= MDLF_PLAYER | MDLF_DOCRC;
	else if (!strcmp(mod->publicname, "progs/flame.mdl")
		|| !strcmp(mod->publicname, "progs/flame2.mdl")
#ifdef HEXEN2
		|| !strcmp(mod->publicname, "models/flame1.mdl")	//hexen2 small standing flame
		|| !strcmp(mod->publicname, "models/flame2.mdl")	//hexen2 large standing flame
		|| !strcmp(mod->publicname, "models/cflmtrch.mdl")	//hexen2 wall torch
#endif
			)
		mod->engineflags |= MDLF_FLAME|MDLF_NOSHADOWS;
	else if (!strcmp(mod->publicname, "progs/bolt.mdl")
		|| !strcmp(mod->publicname, "progs/bolt2.mdl")
		|| !strcmp(mod->publicname, "progs/bolt3.mdl")
		|| !strcmp(mod->publicname, "progs/beam.mdl")
#ifdef HEXEN2
		|| !strcmp(mod->publicname, "models/stsunsf2.mdl")
		|| !strcmp(mod->publicname, "models/stsunsf1.mdl")
		|| !strcmp(mod->publicname, "models/stice.mdl")
#endif
			 )
		mod->engineflags |= MDLF_NOSHADOWS;
	else if (!strcmp(mod->publicname, "progs/backpack.mdl"))
		mod->engineflags |= MDLF_NOTREPLACEMENTS;
	else if (!strcmp(mod->publicname, "progs/eyes.mdl"))
		mod->engineflags |= MDLF_NOTREPLACEMENTS|MDLF_DOCRC;

	/*handle ezquake-originated cheats that would feck over fte users if fte didn't support
	these are the conditions required for r_fb_models on non-players*/
	mod->engineflags |= MDLF_EZQUAKEFBCHEAT;
	if ((mod->engineflags & MDLF_DOCRC) ||
		!strcmp(mod->publicname, "progs/backpack.mdl") ||
		!strcmp(mod->publicname, "progs/gib1.mdl") ||
		!strcmp(mod->publicname, "progs/gib2.mdl") ||
		!strcmp(mod->publicname, "progs/gib3.mdl") ||
		!strcmp(mod->publicname, "progs/h_player.mdl") ||
		!strncmp(mod->publicname, "progs/v_", 8))
		mod->engineflags &= ~MDLF_EZQUAKEFBCHEAT;

	mod->engineflags |= MDLF_RECALCULATERAIN;

	// get string used for replacement tokens
	COM_FileExtension(mod->publicname, ext, sizeof(ext));
	if (!Q_strcasecmp(ext, "spr") || !Q_strcasecmp(ext, "sp2"))
		replstr = ""; // sprite
#ifdef DSPMODELS
	else if (!Q_strcasecmp(ext, "dsp")) // doom sprite
	{
		replstr = "";
		doomsprite = true;
	}
#endif
	else // assume models
		replstr = r_replacemodels.string;

	// gl_load24bit 0 disables all replacements
	if (!gl_load24bit.value)
	{
		replstr = "";
		basedepth = FDEPTH_MISSING;
	}
	else
		basedepth = COM_FDepthFile(mod->publicname, true);

	COM_StripExtension(mod->publicname, mdlbase, sizeof(mdlbase));

	while (replstr)
	{
		char token[256];
		replstr = COM_ParseStringSet(replstr, token, sizeof(token));

		if (replstr)
		{
			char altname[MAX_QPATH], *sl;
			sl = strchr(token, '/');
			if (sl)
			{	//models/name.mdl -> path/preslash/name.postslash
				char *p = COM_SkipPath(mdlbase);
				size_t ofs = (p-mdlbase) + (sl+1-token);
				memcpy(altname, mdlbase, p-mdlbase);
				memcpy(altname+(p-mdlbase), token, sl+1-token);
				if (ofs + strlen(p)+strlen(sl+1)+2 > sizeof(altname))
					continue;	//erk
				Q_snprintfz(altname + ofs, sizeof(altname)-ofs, "%s.%s", p, sl+1);
			}
			else
				Q_snprintfz(altname, sizeof(altname), "%s.%s", mdlbase, token);

			if (COM_FDepthFile(altname, true) > basedepth)
				continue;

			TRACE(("Mod_LoadModel: Trying to load (replacement) model \"%s\"\n", altname));
			buf = (unsigned *)FS_LoadMallocGroupFile(NULL, altname, &filesize, true);

			if (buf)
				Q_strncpyz(mod->name, altname, sizeof(mod->name));
		}
		else
		{
			TRACE(("Mod_LoadModel: Trying to load model \"%s\"\n", mod->publicname));
			buf = (unsigned *)FS_LoadMallocGroupFile(NULL, mod->publicname, &filesize, true);
			if (buf)
				Q_strncpyz(mod->name, mod->publicname, sizeof(mod->name));
			else if (!buf)
			{
#ifdef DSPMODELS
				if (doomsprite) // special case needed for doom sprites
				{
					TRACE(("Mod_LoadModel: doomsprite: \"%s\"\n", mod->name));
					Mod_LoadDoomSprite(mod);
					BZ_Free(buf);
					COM_AddWork(WG_MAIN, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
					return;
				}
#endif
#ifdef TERRAIN
				if (!Q_strcasecmp(ext, "map"))
				{
					const char *dummymap =
						"{\n"
							"classname worldspawn\n"
							"wad \"base.wad\"\n"	//we ARE a quake engine after all, and default.wad is generally wrong
							"message \"Unnamed map\"\n"
							"{\n"
								"(-128  128 0)	( 128  128 0)	( 128 -128 0)	\"WBRICK1_5\" 0 0 0 1 1\n"
								"( 128 -128 -16)( 128  128 -16)	(-128  128 -16)	\"WBRICK1_5\" 0 0 0 1 1\n"
								"( 128  128 0)	(-128  128 0)	(-128  128 -16)	\"WBRICK1_5\" 0 0 0 1 1\n"
								"(-128 -128 0)	( 128 -128 0)	( 128 -128 -16)	\"WBRICK1_5\" 0 0 0 1 1\n"
								"(-128  128 0)	(-128 -128 0)	(-128 -128 -16)	\"WBRICK1_5\" 0 0 0 1 1\n"
								"( 128 -128 0)	( 128  128 0)	( 128  128 -16)	\"WBRICK1_5\" 0 0 0 1 1\n"
							"}\n"
						"}\n"
						"{\n"
							"classname info_player_start\n"
							"origin \"0 0 24\"\n"
						"}\n"
						"{\n"
							"classname light\n"
							"origin \"0 0 64\"\n"
						"}\n";
					buf = (unsigned*)Z_StrDup(dummymap);
					filesize = strlen(dummymap);
				}
				else
#endif
					break; // failed to load unreplaced file and nothing left
			}
		}
		if (!buf)
			continue;
		if (filesize < 4)
		{
			BZ_Free(buf);
			continue;
		}

//
// fill it in
//
		if (!Mod_DoCRC(mod, (char*)buf, filesize))
		{
			BZ_Free(buf);
			continue;
		}

		memset(&mod->funcs, 0, sizeof(mod->funcs));	//just in case...

		//look for known extensions first, to try to avoid issues with specific formats
		for(i = 0; i < countof(modelloaders); i++)
		{
			if (modelloaders[i].load && modelloaders[i].ident && *modelloaders[i].ident == '.' && !Q_strcasecmp(modelloaders[i].ident, COM_GetFileExtension(mod->name, NULL)))
				break;
		}
		//now look to see if we can find one with the right magic header
		if (i == countof(modelloaders))
		{
			for(i = 0; i < countof(modelloaders); i++)
			{
				if (modelloaders[i].load && modelloaders[i].magic && filesize >= modelloaders[i].magicsize && !memcmp(buf, modelloaders[i].magic, modelloaders[i].magicsize) && !modelloaders[i].ident)
					break;
			}
		}
		if (i < countof(modelloaders))
		{
			if (!modelloaders[i].load(mod, buf, filesize))
			{
				BZ_Free(buf);
				continue;
			}
		}
		else
		{
			COM_ParseOut((char*)buf, token, sizeof(token));
			for(i = 0; i < sizeof(modelloaders) / sizeof(modelloaders[0]); i++)
			{
				if (modelloaders[i].load && modelloaders[i].ident && !strcmp(modelloaders[i].ident, token))
					break;
			}
			if (i < sizeof(modelloaders) / sizeof(modelloaders[0]))
			{
				if (!modelloaders[i].load(mod, buf, filesize))
				{
					BZ_Free(buf);
					continue;
				}
			}
			else
			{
				Con_Printf(CON_WARNING "Unrecognised model format %c%c%c%c\n", ((char*)buf)[0], ((char*)buf)[1], ((char*)buf)[2], ((char*)buf)[3]);
				BZ_Free(buf);
				continue;
			}
		}

		TRACE(("Mod_LoadModel: Loaded\n"));

		BZ_Free(buf);

		COM_AddWork(WG_MAIN, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
		return;
	}

	mod->type = mod_dummy;
	mod->mins[0] = -16;
	mod->mins[1] = -16;
	mod->mins[2] = -16;
	mod->maxs[0] = 16;
	mod->maxs[1] = 16;
	mod->maxs[2] = 16;
	mod->engineflags = 0;
	COM_AddWork(WG_MAIN, Mod_ModelLoaded, mod, NULL, MLS_FAILED, verbose);
}


model_t *Mod_LoadModel (model_t *mod, enum mlverbosity_e verbose)
{
	if (mod->loadstate == MLS_NOTLOADED && *mod->name != '*')
	{
		const char *s = strstr(r_nolerp_list.string, mod->publicname);
		COM_AssertMainThread("Mod_LoadModel");
		if (s)
		{
			size_t l = strlen(mod->publicname);
			if ((s == r_nolerp_list.string || s[-1]==',') && (s[l] == 0 || s[l] == ','))
				mod->engineflags |= MDLF_NOLERP;
		}
#ifdef RTLIGHTS
		s = strstr(r_noshadow_list.string, mod->publicname);
		if (s)
		{
			size_t l = strlen(mod->publicname);
			if ((s == r_noshadow_list.string || s[-1]==',') && (s[l] == 0 || s[l] == ','))
				mod->engineflags |= MDLF_NOSHADOWS;
		}
#endif

		mod->loadstate = MLS_LOADING;
		if (verbose == MLV_ERROR || verbose == MLV_WARNSYNC)
			COM_InsertWork(WG_LOADER, Mod_LoadModelWorker, mod, NULL, verbose, 0);
		else
			COM_AddWork(WG_LOADER, Mod_LoadModelWorker, mod, NULL, verbose, 0);
	}

	//block until its loaded, if we care.
	if (mod->loadstate == MLS_LOADING && (verbose == MLV_ERROR || verbose == MLV_WARNSYNC || verbose == MLV_SILENTSYNC))
		COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);

	if (verbose == MLV_ERROR)
	{
		//someone already tried to load it without caring if it failed or not. make sure its loaded.
		//fixme: this is a spinloop.

		if (mod->loadstate != MLS_LOADED)
			Host_EndGame ("Mod_NumForName: %s not found or couldn't load", mod->name);
	}
	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (const char *name, enum mlverbosity_e verbosity)
{
	model_t	*mod;
	
	mod = Mod_FindName (name);
	
	return Mod_LoadModel (mod, verbosity);
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

#if !defined(SERVERONLY)
static const struct
{
	const char *oldname;
	unsigned int chksum;	//xor-compacted md4
	const char *newname;
} buggytextures[] =
{
	//FIXME: we should load this table from disk or something.
	//old			sum			new
	{"metal5_2",	0x45d110ec,	"metal5_2_arc"},
	{"metal5_2",	0x0d275f87,	"metal5_2_x"},
	{"metal5_4",	0xf8e27da8,	"metal5_4_arc"},
	{"metal5_4",	0xa301c52e,	"metal5_4_double"},
	{"metal5_8",	0xfaa8bf77,	"metal5_8_back"},
	{"metal5_8",	0x88792923,	"metal5_8_rune"},
	{"plat_top1",	0xfe4f9f5a,	"plat_top1_bolt"},
	{"plat_top1",	0x9ac3fccf,	"plat_top1_cable"},
	{"sky4",		0xde688b77,	"sky1"},
//	{"sky4",		0x8a010dc0,	"sky4"},
//	{"window03",	?,			"window03_?"},
//	{"window03",	?,			"window03_?"},


	//FIXME: hexen2 has the same issue.
};
static const char *Mod_RemapBuggyTexture(const char *name, const qbyte *data, unsigned int datalen)
{
	unsigned int i;
	if (!data)
		return NULL;
	for (i = 0; i < sizeof(buggytextures)/sizeof(buggytextures[0]); i++)
	{
		if (!strcmp(name, buggytextures[i].oldname))
		{
			unsigned int sum = CalcHashInt(&hash_md4, data, datalen);
			for (; i < sizeof(buggytextures)/sizeof(buggytextures[0]); i++)
			{
				if (strcmp(name, buggytextures[i].oldname))
					break;
				if (sum == buggytextures[i].chksum)
					return buggytextures[i].newname;
			}
			break;
		}
	}
	return NULL;
}

static void Mod_FinishTexture(model_t *mod, texture_t *tx, const char *loadname, qboolean safetoloadfromwads)
{
	extern cvar_t gl_shadeq1_name;
	char altname[MAX_QPATH];
	char *star;
	const char *origname = NULL;
	const char *shadername = tx->name;

	if (!safetoloadfromwads || !tx->shader)
	{
		//remap to avoid bugging out on textures with the same name and different images (vanilla content sucks)
		shadername = Mod_RemapBuggyTexture(shadername, tx->srcdata, tx->srcwidth*tx->srcheight);
		if (shadername)
			origname = tx->name;
		else
			shadername = tx->name;

		//find the *
		if (!*gl_shadeq1_name.string || !strcmp(gl_shadeq1_name.string, "*"))
			;
		else if (!(star = strchr(gl_shadeq1_name.string, '*')) || (strlen(gl_shadeq1_name.string)+strlen(tx->name)+1>=sizeof(altname)))	//it's got to fit.
			shadername = gl_shadeq1_name.string;
		else
		{
			strncpy(altname, gl_shadeq1_name.string, star-gl_shadeq1_name.string);	//copy the left
			altname[star-gl_shadeq1_name.string] = '\0';
			strcat(altname, shadername);	//insert the *
			strcat(altname, star+1);	//add any final text.
			shadername = altname;
		}

		tx->shader = R_RegisterCustom (mod, shadername, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);

		if (!tx->srcdata && !safetoloadfromwads)
			return;
	}
	else
	{	//already loaded. don't waste time / crash (this will be a dead pointer).
		if (tx->srcdata)
			return;
	}

	if (!strncmp(tx->name, "sky", 3))
		R_InitSky (tx->shader, shadername, tx->srcfmt, tx->srcdata, tx->srcwidth, tx->srcheight);
	else
	{
		unsigned int maps = 0;
		maps |= SHADER_HASPALETTED;
		maps |= SHADER_HASDIFFUSE;
		if (r_fb_bmodels.ival)
			maps |= SHADER_HASFULLBRIGHT;
		if (r_loadbumpmapping || ((r_waterstyle.ival > 1 || r_telestyle.ival > 1) && *tx->name == '*') || tx->shader->defaulttextures->reflectcube)
			maps |= SHADER_HASNORMALMAP;
		if (gl_specular.ival)
			maps |= SHADER_HASGLOSS;

		R_BuildLegacyTexnums(tx->shader, origname, loadname, maps, 0, tx->srcfmt, tx->srcwidth, tx->srcheight, tx->srcdata, tx->palette);
	}
	BZ_Free(tx->srcdata);
	tx->srcdata = NULL;
}
#endif

void Mod_NowLoadExternal(model_t *loadmodel)
{
	//for halflife bsps where wads are loaded after the map.
#if !defined(SERVERONLY)
	int i;
	texture_t	*tx;
	char loadname[32];
	COM_FileBase (cl.worldmodel->name, loadname, sizeof(loadname));
	
	if (!strncmp(loadname, "b_", 2))
		Q_strncpyz(loadname, "bmodels", sizeof(loadname));

	for (i=0 ; i<loadmodel->numtextures ; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx)	//e1m2, this happens
			continue;

		if (tx->srcdata)
			continue;

		Mod_FinishTexture(loadmodel, tx, loadname, true);
	}
#endif
}

qbyte lmgamma[256];
void BuildLightMapGammaTable (float g, float c)
{
	int i, inf;

//	g = bound (0.1, g, 3);
//	c = bound (1, c, 3);

	if (g == 1 && c == 1)
	{
		for (i = 0; i < 256; i++)
			lmgamma[i] = i;
		return;
	}

	for (i = 0; i < 256; i++)
	{
		inf = 255 * pow ((i + 0.5) / 255.5 * c, g) + 0.5;
		if (inf < 0)
			inf = 0;
		else if (inf > 255)
			inf = 255;		
		lmgamma[i] = inf;
	}
}

typedef struct
{
	unsigned int magic; //"QLIT"
	unsigned int version; //2
	unsigned int numsurfs;
	unsigned int lmsize;	//samples, not bytes (same size as vanilla lighting lump in a q1 bsp).

	//uint		lmoffsets[numsurfs];	//completely overrides the bsp lightmap info
	//ushort	lmextents[numsurfs*2];	//only to avoid precision issues. width+height pairs, actual lightmap sizes on disk (so +1).
	//byte		lmstyles[numsurfs*4];	//completely overrides the bsp lightmap info
	//byte		lmshifts[numsurfs];		//default is 4 (1<<4=16), for 1/16th lightmap-to-texel ratio
	//byte		litdata[lmsize*3];		//rgb data
	//byte		luxdata[lmsize*3];		//stn light dirs (unsigned bytes
} qlit2_t;

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (model_t *loadmodel, bspx_header_t *bspx, qbyte *mod_base, lump_t *l, qboolean interleaveddeluxe, lightmapoverrides_t *overrides, subbsp_t subbsp)
{
	qboolean luxtmp = true;
	qboolean exptmp = true;
	qboolean littmp = true;
	qbyte *luxdata = NULL; //rgb8
	qbyte *expdata = NULL; //e5bgr9 (hdr!)
	qbyte *litdata = NULL; //xyz8
	qbyte *lumdata = NULL; //l8
	qbyte *out;
	size_t samples;
#ifdef RUNTIMELIGHTING
	qboolean relighting = false;
#endif

	extern cvar_t gl_overbright;

#ifdef HAVE_CLIENT
	BSPX_LightGridLoad(loadmodel, bspx, mod_base);
#endif

	loadmodel->lightmaps.fmt = LM_L8;

	//q3 maps have built in 4-fold overbright.
	//if we're not rendering with that, we need to brighten the lightmaps in order to keep the darker parts the same brightness. we loose the 2 upper bits. those bright areas become uniform and indistinct.
	if (loadmodel->fromgame == fg_quake3)
	{
		gl_overbright.flags |= CVAR_RENDERERLATCH;
		BuildLightMapGammaTable(1, (1<<(2-gl_overbright.ival)));
	}
	else //lit file light intensity is made to match the world's light intensity.
		BuildLightMapGammaTable(1, 1);

	loadmodel->lightdata = NULL;
	loadmodel->deluxdata = NULL;
	if (loadmodel->fromgame == fg_halflife || loadmodel->fromgame == fg_quake2 || loadmodel->fromgame == fg_quake3)
	{
		litdata = mod_base + l->fileofs;
		samples = l->filelen/3;
	}
	else if (subbsp == sb_quake64)
	{
		qbyte *q64l = mod_base + l->fileofs;
		qbyte* newl;
		int i;

		samples = l->filelen / 2;
		litdata = ZG_Malloc(&loadmodel->memgroup, samples * 3);
		littmp = false;

		// q64 lightmap format: byte 0 RRRR RGGG byte 1 GGBB BBBA
		for (i = 0, newl = litdata; i < samples; ++i, q64l += 2, newl += 3)
		{
			newl[0] = (q64l[0] & 0xF8) | ((q64l[0] & 0xF8) >> 5);
			newl[1] = ((q64l[0] & 0x07) << 5) | ((q64l[1] & 0xC0) >> 3) | (q64l[0] & 0x07);
			newl[2] = ((q64l[1] & 0x3E) << 2) | ((q64l[1] & 0x3E) >> 3);
		}
	} 
	else
	{
		lumdata = mod_base + l->fileofs;
		samples = l->filelen;
	}
	if (interleaveddeluxe)
		samples >>= 1;
	if (!samples)
	{
		expdata = BSPX_FindLump(bspx, mod_base, "LIGHTING_E5BGR9", &samples);	//expressed as a big-endian packed int - 0xEBGR type thing, except misaligned and 32bit.
		samples /= 4;
		if (!samples)
		{
			litdata = BSPX_FindLump(bspx, mod_base, "RGBLIGHTING", &samples); //RGB packed data
			samples /= 3;
			if (!samples)
				return;
		}
	}

#ifndef SERVERONLY
#if 0//def Q2BSPS //Q2XP's alternative to lit files, for higher res lightmaps (that seem to have light coming from the wrong directions...)
	if (loadmodel->fromgame == fg_quake2 && overrides && !interleaveddeluxe)
	{
		char litname[MAX_QPATH];
		size_t litsize;
		qbyte *xplm;
		COM_StripExtension(loadmodel->name, litname, sizeof(litname));
		Q_strncatz(litname, ".xplm", sizeof(litname));
		xplm = FS_LoadMallocGroupFile(&loadmodel->memgroup, litname, &litsize);

		if (litdata)
		{
			int scale;
			size_t numsurfs = LittleLong(*(int *)&xplm[0]);
			unsigned int *offsets = (unsigned int*)(xplm+4);
			scale = xplm[(numsurfs+1)*4];

			for (overrides->defaultshift=0; scale && !(scale&1); scale>>=1)
				overrides->defaultshift++;
			if (scale == 1)
			{	//its a supported shift
				litdata = xplm+(numsurfs+1)*4+1;
				samples = (litsize-(numsurfs+1)*4+1)/3;
				overrides->offsets = offsets;
			}
		}
	}
#endif
	if (!expdata && !litdata && r_loadlits.value)
	{
		struct
		{
			char *pattern;
		} litnames[] = {
			{"%s.hdr"},
			{"%s.lit"},
#ifdef HAVE_LEGACY
			{"lits/%s.lit"},
#endif
		};
		char litbasep[MAX_QPATH];
		char litbase[MAX_QPATH];
		int depth;
		int bestdepth = 0x7fffffff;
		int best = -1;
		int i;
		char litname[MAX_QPATH];
		size_t litsize;
		qboolean inhibitvalidation = false;

		COM_StripExtension(loadmodel->name, litbasep, sizeof(litbasep));
		COM_FileBase(loadmodel->name, litbase, sizeof(litbase));
		for (i = 0; i < countof(litnames); i++)
		{
			if (strchr(litnames[i].pattern, '/'))
				Q_snprintfz(litname, sizeof(litname), litnames[i].pattern, litbase);
			else
				Q_snprintfz(litname, sizeof(litname), litnames[i].pattern, litbasep);
			depth = COM_FDepthFile(litname, false);
			if (depth < bestdepth)
			{
				bestdepth = depth;
				best = i;
			}
		}
		if (best >= 0)
		{
			if (strchr(litnames[best].pattern, '/'))
				Q_snprintfz(litname, sizeof(litname), litnames[best].pattern, litbase);
			else
				Q_snprintfz(litname, sizeof(litname), litnames[best].pattern, litbasep);
			litdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, litname, &litsize, false);
		}
		else
		{
			litdata = NULL;
			litsize = 0;
		}

		if (litdata)
		{	//validate it, if we loaded one.
			int litver = LittleLong(*(int *)&litdata[4]);
			if (litsize < 8 || litdata[0] != 'Q' || litdata[1] != 'L' || litdata[2] != 'I' || litdata[3] != 'T')
			{
				litdata = NULL;
				Con_Printf("lit \"%s\" isn't a lit\n", litname);
			}
			else if (litver == 1)
			{
				if (l->filelen && samples*3 != (litsize-8))
				{
					litdata = NULL;
					Con_Printf("lit \"%s\" doesn't match level. Ignored.\n", litname);
				}
				else	
					litdata += 8;	//header+version
			}
			else if (litver == 0x10001)
			{	//hdr lighting, e5bgr9 format
				if (l->filelen && samples*4 != (litsize-8))
					Con_Printf("lit \"%s\" doesn't match level. Ignored.\n", litname);
				else	
					expdata = litdata+8;	//header+version
				litdata = NULL;
			}
			else if (litver == 2 && overrides && litsize > sizeof(qlit2_t))
			{
				qlit2_t *ql2 = (qlit2_t*)litdata;
				unsigned int *offsets = (unsigned int*)(ql2+1);
				unsigned short *extents = (unsigned short*)(offsets+ql2->numsurfs);
				unsigned char *styles = (unsigned char*)(extents+ql2->numsurfs*2);
				unsigned char *shifts = (unsigned char*)(styles+ql2->numsurfs*4);
				if (loadmodel->numsurfaces != ql2->numsurfs)
				{
					litdata = NULL;
					Con_Printf("lit \"%s\" doesn't match level. Ignored.\n", litname);
				}
				else if (litsize != sizeof(qlit2_t)+ql2->numsurfs*4+ql2->lmsize*6)
				{
					litdata = NULL;
					Con_Printf("lit \"%s\" is truncated. Ignored.\n", litname);
				}
				else
				{
					Con_Printf("%s: lit2 support is unstandardised and may change in future.\n", litname);

					inhibitvalidation = true;

					//surface code needs to know the overrides.
					overrides->offsets = offsets;
					overrides->extents = extents;
					overrides->styles8 = styles;
					overrides->stylesperface = 4;
					overrides->shifts = shifts;

					//we're now using this amount of data.
					samples = ql2->lmsize;

					litdata = shifts+ql2->numsurfs;
					if (r_deluxemapping)
						luxdata = litdata+samples*3;
				}
			}
			else
			{
				Con_Printf("lit \"%s\" isn't version 1 or 2.\n", litname);
				litdata = NULL;
			}
		}

		exptmp = littmp = false;
		if (!litdata && !expdata)
		{
			size_t size;
			/*FIXME: bspx support for extents+lmscale, may require style+offset lumps too, not sure what to do here*/
			expdata = BSPX_FindLump(bspx, mod_base, "LIGHTING_E5BGR9", &size);
			exptmp = true;
			if (size != samples*4)
			{
				expdata = NULL;

				litdata = BSPX_FindLump(bspx, mod_base, "RGBLIGHTING", &size);
				littmp = true;
				if (size != samples*3)
					litdata = NULL;
			}
		}
		else if (!inhibitvalidation)
		{
			if (lumdata && litdata)
			{
				float prop;
				int i;
				qbyte *lum;
				qbyte *lit;

				//now some cheat protection.
				lum = lumdata;
				lit = litdata;

				for (i = 0; i < samples; i++)	//force it to the same intensity. (or less, depending on how you see it...)
				{
#define m(a, b, c) (a>(b>c?b:c)?a:(b>c?b:c))
					prop = (float)m(lit[0],  lit[1], lit[2]);

					if (!prop)
					{
						lit[0] = *lum;
						lit[1] = *lum;
						lit[2] = *lum;
					}
					else
					{
						prop = *lum / prop;
						lit[0] *= prop;
						lit[1] *= prop;
						lit[2] *= prop;
					}

					lum++;
					lit+=3;
				}
				//end anti-cheat
			}
		}
	}


	if (!luxdata && r_loadlits.ival && r_deluxemapping)
	{	//the map util has a '-scalecos X' parameter. use 0 if you're going to use only just lux. without lux scalecos 0 is hideous.
		char luxname[MAX_QPATH];
		size_t luxsz = 0;
		*luxname = 0;
		if (!luxdata)
		{							
			Q_strncpyz(luxname, loadmodel->name, sizeof(luxname));
			COM_StripExtension(loadmodel->name, luxname, sizeof(luxname));
			COM_DefaultExtension(luxname, ".lux", sizeof(luxname));
			luxdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, luxname, &luxsz, false);
			luxtmp = false;
		}
		if (!luxdata)
		{
			Q_strncpyz(luxname, "luxs/", sizeof(luxname));
			COM_StripExtension(COM_SkipPath(loadmodel->name), luxname+5, sizeof(luxname)-5);
			Q_strncatz(luxname, ".lux", sizeof(luxname));

			luxdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, luxname, &luxsz, false);
			luxtmp = false;
		}
		if (!luxdata) //dp...
		{
			COM_StripExtension(loadmodel->name, luxname, sizeof(luxname));
			COM_DefaultExtension(luxname, ".dlit", sizeof(luxname));
			luxdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, luxname, &luxsz, false);
			luxtmp = false;
		}
		//make sure the .lux has the correct size
		if (luxdata && l->filelen && l->filelen != (luxsz-8)/3)
		{
			Con_Printf("deluxmap \"%s\" doesn't match level. Ignored.\n", luxname);
			luxdata=NULL;
		}
		if (!luxdata)
		{
			size_t size;
			luxdata = BSPX_FindLump(bspx, mod_base, "LIGHTINGDIR", &size);
			if (size != samples*3)
				luxdata = NULL;
			luxtmp = true;
		}
		else
		{
			if (luxsz < 8 || (luxdata[0] == 'Q' && luxdata[1] == 'L' && luxdata[2] == 'I' && luxdata[3] == 'T'))
			{
				if (LittleLong(*(int *)&luxdata[4]) == 1)
					luxdata+=8;
				else
				{
					Con_Printf("\"%s\" isn't a version 1 deluxmap\n", luxname);
					luxdata=NULL;
				}
			}
			else
			{
				Con_Printf("lit \"%s\" isn't a deluxmap\n", luxname);
				luxdata=NULL;
			}
		}	
	}
#endif

#ifdef RUNTIMELIGHTING
	if ((loadmodel->type == mod_brush && loadmodel->fromgame == fg_quake) || loadmodel->type == mod_heightmap)
	{	//we only support a couple of formats. :(
		if (r_loadlits.value >= 2 && ((!litdata&&!expdata) || (!luxdata && r_deluxemapping)))
		{
			relighting = RelightSetup(loadmodel, l->filelen, !litdata&&!expdata);
		}
		else if (r_deluxemapping_cvar.value>1 && r_deluxemapping && !luxdata
#ifdef RTLIGHTS
			&& !(r_shadow_realtime_world.ival && r_shadow_realtime_world_lightmaps.value<=0)
#endif
			)
		{	//if deluxemapping is on, generate missing lux files a little more often, but don't bother if we have rtlights on anyway.

			relighting = RelightSetup(loadmodel, l->filelen, false);
		}
	}

	/*if we're relighting, make sure there's the proper lit data to be updated*/
	if (relighting && !litdata && !expdata)
	{
		int i;
		unsigned int *ergb;

		if (r_loadlits.ival >= 3)
		{
			ergb = ZG_Malloc(&loadmodel->memgroup, samples*4);
			expdata = (qbyte*)ergb;
			littmp = false;
			if (lumdata)
			{
				for (i = 0; i < samples; i++)
					ergb[i] = (17u<<27) | (lumdata[i]<<18) | (lumdata[i]<<9) | (lumdata[i]<<0);
				lumdata = NULL;
			}
		}
		else
		{
			litdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
			littmp = false;
			if (lumdata)
			{
				for (i = 0; i < samples; i++)
				{
					litdata[i*3+0] = lumdata[i];
					litdata[i*3+1] = lumdata[i];
					litdata[i*3+2] = lumdata[i];
				}
				lumdata = NULL;
			}
		}
	}
	/*if we're relighting, make sure there's the proper lux data to be updated*/
	if (relighting && r_deluxemapping && !luxdata)
	{
		int i;
		luxdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
		for (i = 0; i < samples; i++)
		{
			luxdata[i*3+0] = 0.5f*255;
			luxdata[i*3+1] = 0.5f*255;
			luxdata[i*3+2] = 255;
		}
	}
#endif

	if (overrides && !overrides->shifts)
	{
		size_t size;
		//if we have shifts, then we probably also have legacy data in the surfaces that we want to override
		if (!overrides->offsets)
		{
			size_t size;
			overrides->offsets = BSPX_FindLump(bspx, mod_base, "LMOFFSET", &size);
			if (size != loadmodel->numsurfaces * sizeof(int))
			{
				if (size)
					Con_Printf(CON_ERROR"BSPX LMOFFSET lump is wrong size, expected %u entries, found %"PRIuSIZE"\n", loadmodel->numsurfaces, size/(unsigned int)sizeof(int));
				overrides->offsets = NULL;
			}
		}
		if (!overrides->styles8 && !overrides->styles16)
		{	//16bit per-face lightmap styles index
			size_t size;
			overrides->styles16 = BSPX_FindLump(bspx, mod_base, "LMSTYLE16", &size);
			overrides->stylesperface = size / (sizeof(*overrides->styles16)*loadmodel->numsurfaces); //rounding issues will be caught on the next line...
			if (!overrides->stylesperface || size != loadmodel->numsurfaces * sizeof(*overrides->styles16)*overrides->stylesperface)
			{
				if (size)
					Con_Printf(CON_ERROR"BSPX LMSTYLE16 lump is wrong size, expected %u*%u entries, found %"PRIuSIZE"\n", loadmodel->numsurfaces, overrides->stylesperface, size/(unsigned int)sizeof(*overrides->styles16));
				overrides->styles16 = NULL;
			}
			else if (overrides->stylesperface > MAXCPULIGHTMAPS)
				Con_Printf(CON_WARNING "LMSTYLE16 lump provides %i styles, only the first %i will be used.\n", overrides->stylesperface, MAXCPULIGHTMAPS);
		}
		if (!overrides->styles8 && !overrides->styles16)
		{	//16bit per-face lightmap styles index
			size_t size;
			overrides->styles8 = BSPX_FindLump(bspx, mod_base, "LMSTYLE", &size);
			overrides->stylesperface = size / (sizeof(*overrides->styles8)*loadmodel->numsurfaces); //rounding issues will be caught on the next line...
			if (!overrides->stylesperface || size != loadmodel->numsurfaces * sizeof(*overrides->styles8)*overrides->stylesperface)
			{
				if (size)
					Con_Printf(CON_ERROR"BSPX LMSTYLE16 lump is wrong size, expected %u*%u entries, found %"PRIuSIZE"\n", loadmodel->numsurfaces, overrides->stylesperface, size/(unsigned int)sizeof(*overrides->styles8));
				overrides->styles8 = NULL;
			}
			else if (overrides->stylesperface > MAXCPULIGHTMAPS)
				Con_Printf(CON_WARNING "LMSTYLE lump provides %i styles, only the first %i will be used.\n", overrides->stylesperface, MAXCPULIGHTMAPS);
		}

		overrides->shifts = BSPX_FindLump(bspx, mod_base, "LMSHIFT", &size);
		if (size != loadmodel->numsurfaces)
		{
			if (size)
			{	//ericw-tools is screwing up again. don't leave things screwed.
				Con_Printf(CON_ERROR"BSPX LMSHIFT lump is wrong size, expected %u entries, found %"PRIuSIZE"\n", loadmodel->numsurfaces, size);
				overrides->styles16 = NULL;
				overrides->styles8 = NULL;
				overrides->offsets = NULL;
			}
			overrides->shifts = NULL;
		}
	}

	if (luxdata && luxtmp)
	{
		loadmodel->deluxdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
		memcpy(loadmodel->deluxdata, luxdata, samples*3);
	}
	else if (luxdata)
		loadmodel->deluxdata = luxdata;
	else if (interleaveddeluxe)
		loadmodel->deluxdata = ZG_Malloc(&loadmodel->memgroup, samples*3);

	if (expdata)
	{
		loadmodel->lightmaps.fmt = LM_E5BGR9;
		loadmodel->lightdatasize = samples*4;
		if (exptmp)
		{
			loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, samples*4);
			memcpy(loadmodel->lightdata, expdata, samples*4);
		}
		else
			loadmodel->lightdata = expdata;

		//FIXME: no desaturation/gamma logic.
		return;
	}
	else if (litdata)
	{
		loadmodel->lightmaps.fmt = LM_RGB8;
		if (littmp)
			loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, samples*3);	/*the memcpy is below*/
		else
			loadmodel->lightdata = litdata;
		samples*=3;
	}
	else if (lumdata)
	{
		loadmodel->lightmaps.fmt = LM_L8;
		loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, samples);
		litdata = lumdata;
	}

	/*apply lightmap gamma to the entire lightmap*/
	loadmodel->lightdatasize = samples;
	out = loadmodel->lightdata;
	if (interleaveddeluxe)
	{
		qbyte *luxout = loadmodel->deluxdata;
		samples /= 3;
		while(samples-- > 0)
		{
			*out++ = lmgamma[*litdata++];
			*out++ = lmgamma[*litdata++];
			*out++ = lmgamma[*litdata++];
			*luxout++ = *litdata++;
			*luxout++ = *litdata++;
			*luxout++ = *litdata++;
		}
	}
	else
	{
		while(samples-- > 0)
		{
			*out++ = lmgamma[*litdata++];
		}
	}

#ifndef SERVERONLY
	if ((loadmodel->lightmaps.fmt == LM_RGB8) && r_lightmap_saturation.value != 1.0f)
		SaturateR8G8B8(loadmodel->lightdata, l->filelen, r_lightmap_saturation.value);
#endif
}

//scans through the worldspawn for a single specific key.
const char *Mod_ParseWorldspawnKey(model_t *mod, const char *key, char *buffer, size_t sizeofbuffer)
{
	char keyname[64];
	char value[1024];
	const char *ents = Mod_GetEntitiesString(mod);
	while(ents && *ents)
	{
		ents = COM_ParseOut(ents, keyname, sizeof(keyname));
		if (*keyname == '{')	//an entity
		{
			while (ents && *ents)
			{
				ents = COM_ParseOut(ents, keyname, sizeof(keyname));
				if (*keyname == '}')
					break;
				ents = COM_ParseOut(ents, value, sizeof(value));
				if (!strcmp(keyname, key) || (*keyname == '_' && !strcmp(keyname+1, key)))
				{
					Q_strncpyz(buffer, value, sizeofbuffer);
					return buffer;
				}
			}
			return "";	//worldspawn only.
		}
	}
	return "";	//err...
}

static void Mod_ShowEnt_f(void)
{
	model_t *mod = NULL;
	size_t idx = atoi(Cmd_Argv(1));
	char *n = Cmd_Argv(2);

	if (*n)
		mod = Mod_ForName(n, MLV_WARN);
#ifndef CLIENTONLY
	if (sv.state && !mod)
		mod = sv.world.worldmodel;
#endif
#ifndef SERVERONLY
	if (cls.state && !mod)
		mod = cl.worldmodel;
#endif
	if (mod && mod->loadstate == MLS_LOADING)
		COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
	if (!mod || mod->loadstate != MLS_LOADED)
	{
		Con_Printf("Map not loaded\n");
		return;
	}

	if (!mod->numentityinfo)
		Mod_ParseEntities(mod);
	if (!idx && strcmp(Cmd_Argv(1), "0"))
	{
		char *match = Cmd_Argv(1);
		unsigned count = 0;
		for (idx = 0; idx < mod->numentityinfo; idx++)
		{
			if (strstr(mod->entityinfo[idx].keyvals, match))
			{
				Con_Printf("{\t//%u\n%s\n}\n", (unsigned)idx, mod->entityinfo[idx].keyvals);
				count++;
			}
		}
		Con_Printf("%u of %u ents match\n", (unsigned)count, (unsigned)mod->numentityinfo);
	}
	else if (idx >= mod->numentityinfo)
		Con_Printf("Invalid entity index (max %u).\n", (unsigned)mod->numentityinfo);
	else if (!mod->entityinfo[idx].keyvals)
		Con_Printf("Entity index was cleared...\n");
	else
		Con_Printf("{\n%s\n}\n", mod->entityinfo[idx].keyvals);
}

static void Mod_SaveEntFile_f(void)
{
	char fname[MAX_QPATH];
	char nname[MAX_OSPATH];
	model_t *mod = NULL;
	char *n = Cmd_Argv(1);
	const char *ents;
	if (*n)
		mod = Mod_ForName(n, MLV_WARN);
#ifndef CLIENTONLY
	if (sv.state && !mod)
		mod = sv.world.worldmodel;
#endif
#ifndef SERVERONLY
	if (cls.state && !mod)
		mod = cl.worldmodel;
#endif
	if (mod && mod->loadstate == MLS_LOADING)
		COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
	if (!mod || mod->loadstate != MLS_LOADED)
	{
		Con_Printf("Map not loaded\n");
		return;
	}
	ents = Mod_GetEntitiesString(mod);
	if (!ents)
	{
		Con_Printf("Map is not a map, and has no entities\n");
		return;
	}

	if (*mod_loadentfiles_dir.string && !strncmp(mod->name, "maps/", 5))
	{
		Q_snprintfz(fname, sizeof(fname), "maps/%s/%s", mod_loadentfiles_dir.string, mod->name+5);
		COM_StripExtension(fname, fname, sizeof(fname));
		Q_strncatz(fname, mod_modifier, sizeof(fname));
		Q_strncatz(fname, ".ent", sizeof(fname));
	}
	else
	{
		COM_StripExtension(mod->name, fname, sizeof(fname));
		Q_strncatz(fname, mod_modifier, sizeof(fname));
		Q_strncatz(fname, ".ent", sizeof(fname));
	}

	if (COM_WriteFile(fname, FS_GAMEONLY, ents, strlen(ents)))
	{
		if (FS_DisplayPath(fname, FS_GAMEONLY, nname, sizeof(nname)))
			Con_Printf("Wrote %s\n", nname);
	}
	else
		Con_Printf("Write failed\n");
}

/*
=================
Mod_LoadEntities
=================
*/
qboolean Mod_LoadEntitiesBlob(struct model_s *mod, const char *entdata, size_t entdatasize)
{
	char fname[MAX_QPATH];
	size_t sz;
	char keyname[64];
	char value[1024];
	char *ents = NULL, *k;
	int t;

	Mod_SetEntitiesString(mod, NULL, false);
	if (!entdatasize)
		return false;

	if (mod_loadentfiles.value && !ents && *mod_loadentfiles_dir.string)
	{
		if (!strncmp(mod->name, "maps/", 5))
		{
			Q_snprintfz(fname, sizeof(fname), "maps/%s/%s", mod_loadentfiles_dir.string, mod->name+5);
			COM_StripExtension(fname, fname, sizeof(fname));
			Q_strncatz(fname, mod_modifier, sizeof(fname));
			Q_strncatz(fname, ".ent", sizeof(fname));
			ents = FS_LoadMallocFile(fname, &sz);
		}
	}
	if (mod_loadentfiles.value && !ents)
	{
		COM_StripExtension(mod->name, fname, sizeof(fname));
		Q_strncatz(fname, mod_modifier, sizeof(fname));
		Q_strncatz(fname, ".ent", sizeof(fname));
		ents = FS_LoadMallocFile(fname, &sz);
	}
	if (mod_loadentfiles.value && !ents)
	{	//tenebrae compat
		COM_StripExtension(mod->name, fname, sizeof(fname));
		Q_strncatz(fname, mod_modifier, sizeof(fname));
		Q_strncatz(fname, ".edo", sizeof(fname));
		ents = FS_LoadMallocFile(fname, &sz);
	}
	if (!ents)
	{
		ents = Z_Malloc(entdatasize + 1);
		memcpy (ents, entdata, entdatasize);
		ents[entdatasize] = 0;
		mod->entitiescrc = 0;
	}
	else
		mod->entitiescrc = CalcHashInt(&hash_crc16, ents, strlen(ents));

	Mod_SetEntitiesString(mod, ents, false);

	while(ents && *ents)
	{
		ents = COM_ParseOut(ents, keyname, sizeof(keyname));
		if (*keyname == '{')	//an entity
		{
			while (ents && *ents)
			{
				ents = COM_ParseOut(ents, keyname, sizeof(keyname));
				if (*keyname == '}')
					break;
				ents = COM_ParseOut(ents, value, sizeof(value));
				if (!strncmp(keyname, "_texpart_", 9) || !strncmp(keyname, "texpart_", 8))
				{
					k = keyname + ((*keyname=='_')?9:8);
					for (t = 0; t < mod->numtextures; t++)
					{
						if (!strcmp(k, mod->textures[t]->name))
						{
							mod->textures[t]->partname = ZG_Malloc(&mod->memgroup, strlen(value)+1);
							strcpy(mod->textures[t]->partname, value);
							break;
						}
					}
					if (t == mod->numtextures)
						Con_Printf("\"%s\" is not valid for %s\n", keyname, mod->name);
				}
			}
		}
	}
	return true;
}
void Mod_LoadEntities (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	Mod_LoadEntitiesBlob(loadmodel, mod_base+l->fileofs, l->filelen);
}

/*
=================
Mod_LoadVertexes
=================
*/
qboolean Mod_LoadVertexes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
		return false;
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));	

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}

	return true;
}

qboolean Mod_LoadVertexNormals (model_t *loadmodel, bspx_header_t *bspx, qbyte *mod_base, lump_t *l)
{
	float	*in;
	float	*out;
	size_t	i, count;

	if (l)
	{
		in = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(vec3_t);
		if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(vec3_t))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
			return false;
		}

		if (count != loadmodel->numvertexes)
			return false;	//invalid number of verts there, can't use this.
	}
	else
	{	//ericw's thing
		size_t size;
		quint32_t t;
		int *normcount;
		struct surfedgenormals_s *sen;
		normcount = BSPX_FindLump(bspx, mod_base, "FACENORMALS", &size);
		if (normcount && size >= sizeof(*normcount))
		{
			count = LittleLong(*normcount);
			if (count < 1)
				return false;
			in = (void*)(normcount+1);	//now the normals table.
			sen = (void*)(in + count*3);
			if ((qbyte*)(sen + loadmodel->numsurfedges)-(qbyte*)normcount != size)
				return false;	//bad size.

			loadmodel->surfedgenormals = ZG_Malloc(&loadmodel->memgroup, loadmodel->numsurfedges*sizeof(*loadmodel->surfedgenormals));
			for ( i=0 ; i<loadmodel->numsurfedges ; i++, sen++)
			{
				t = LittleLong(sen->n); loadmodel->surfedgenormals[i].n = bound(0, t, count-1);
				t = LittleLong(sen->s); loadmodel->surfedgenormals[i].s = bound(0, t, count-1);
				t = LittleLong(sen->t); loadmodel->surfedgenormals[i].t = bound(0, t, count-1);
			}
		}
		else
		{
			//quake2world's thing
			in = BSPX_FindLump(bspx, mod_base, "VERTEXNORMALS", &count);
			if (in)
				count /= sizeof(vec3_t);
			else
				count = 0;
			if (count != loadmodel->numvertexes)
				return false;	//invalid number of verts there, can't use this.
		}
	}

	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(vec3_t));	
	loadmodel->normals = (vec3_t*)out;
	for ( i=0 ; i<count ; i++, in+=3, out+=3)
	{
		out[0] = LittleFloat (in[0]);
		out[1] = LittleFloat (in[1]);
		out[2] = LittleFloat (in[2]);
	}
	return true;
}

#if defined(Q1BSPS) || defined(Q2BSPS)
void ModQ1_Batches_BuildQ1Q2Poly(model_t *mod, msurface_t *surf, builddata_t *cookie)
{
	unsigned int vertidx;
	int i, lindex, edgevert;
	mesh_t *mesh = surf->mesh;
	float *vec;
	float s, t, d;
	int sty;
//	int w,h;
	struct facelmvecs_s *flmv = mod->facelmvecs?mod->facelmvecs + (surf-mod->surfaces):NULL;

	if (!mesh)
	{
		mesh = surf->mesh = ZG_Malloc(&mod->memgroup, sizeof(mesh_t) + (sizeof(vecV_t)+sizeof(vec2_t)*(1+1)+sizeof(vec3_t)*3+sizeof(vec4_t)*1)* surf->numedges + sizeof(index_t)*(surf->numedges-2)*3);
		mesh->numvertexes = surf->numedges;
		mesh->numindexes = (mesh->numvertexes-2)*3;
		mesh->xyz_array = (vecV_t*)(mesh+1);
		mesh->st_array = (vec2_t*)(mesh->xyz_array+mesh->numvertexes);
		mesh->lmst_array[0] = (vec2_t*)(mesh->st_array+mesh->numvertexes);
		mesh->normals_array = (vec3_t*)(mesh->lmst_array[0]+mesh->numvertexes);
		mesh->snormals_array = (vec3_t*)(mesh->normals_array+mesh->numvertexes);
		mesh->tnormals_array = (vec3_t*)(mesh->snormals_array+mesh->numvertexes);
		mesh->colors4f_array[0] = (vec4_t*)(mesh->tnormals_array+mesh->numvertexes);
		mesh->indexes = (index_t*)(mesh->colors4f_array[0]+mesh->numvertexes);
	}
	mesh->istrifan = true;

	//output the mesh's indicies
	for (i=0 ; i<mesh->numvertexes-2 ; i++)
	{
		mesh->indexes[i*3] = 0;
		mesh->indexes[i*3+1] = i+1;
		mesh->indexes[i*3+2] = i+2;
	}
	//output the renderable verticies
	for (i=0 ; i<mesh->numvertexes ; i++)
	{
		lindex = mod->surfedges[surf->firstedge + i];
		edgevert = lindex <= 0;
		if (edgevert)
			lindex = -lindex;
		if (lindex < 0 || lindex >= mod->numedges)
			vertidx = 0;
		else
			vertidx = mod->edges[lindex].v[edgevert];
		vec = mod->vertexes[vertidx].position;

		s = DotProduct (vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
		t = DotProduct (vec, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];

		VectorCopy (vec, mesh->xyz_array[i]);

/*		if (R_GetShaderSizes(surf->texinfo->texture->shader, &w, &h, false) > 0)
		{
			mesh->st_array[i][0] = s/w;
			mesh->st_array[i][1] = t/h;
		}
		else
*/
		{
			mesh->st_array[i][0] = s;
			mesh->st_array[i][1] = t;
			if (surf->texinfo->texture->vwidth)
				mesh->st_array[i][0] /= surf->texinfo->texture->vwidth;
			if (surf->texinfo->texture->vheight)
				mesh->st_array[i][1] /= surf->texinfo->texture->vheight;

			if (surf->texinfo->flags & TI_N64_UV)
			{
				mesh->st_array[i][0] /= 2;
				mesh->st_array[i][1] /= 2;
			}
		}

		if (flmv)
		{
			s = DotProduct (vec, flmv->lmvecs[0]) + flmv->lmvecs[0][3];
			t = DotProduct (vec, flmv->lmvecs[1]) + flmv->lmvecs[1][3];
#ifndef SERVERONLY
			if (r_lightmap_average.ival)
				s = surf->extents[0]*0.5, t = surf->extents[1]*0.5;
#endif
			//s+t are now in luxels... need to convert those to normalised texcoords though.
			for (sty = 0; sty < 1; sty++)
			{
				mesh->lmst_array[sty][i][0] = (surf->light_s[sty] + s) / mod->lightmaps.width;
				mesh->lmst_array[sty][i][1] = (surf->light_t[sty] + t) / mod->lightmaps.height;
			}
		}
		else
		{
#ifndef SERVERONLY
			if (r_lightmap_average.ival)
			{
				for (sty = 0; sty < 1; sty++)
				{
					mesh->lmst_array[sty][i][0] = (surf->extents[0]*0.5 + (surf->light_s[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.width<<surf->lmshift);
					mesh->lmst_array[sty][i][1] = (surf->extents[1]*0.5 + (surf->light_t[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.height<<surf->lmshift);
				}
			}
			else
#endif
			{
				for (sty = 0; sty < 1; sty++)
				{
					mesh->lmst_array[sty][i][0] = (s - surf->texturemins[0] + (surf->light_s[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.width<<surf->lmshift);
					mesh->lmst_array[sty][i][1] = (t - surf->texturemins[1] + (surf->light_t[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.height<<surf->lmshift);
				}
			}
		}

		if (mod->surfedgenormals)
		{
			struct surfedgenormals_s *pv = mod->surfedgenormals + surf->firstedge + i;
			VectorCopy(mod->normals[pv->n], mesh->normals_array[i]);
			VectorCopy(mod->normals[pv->s], mesh->snormals_array[i]);
			VectorCopy(mod->normals[pv->t], mesh->tnormals_array[i]);
		}
		else
		{
			//figure out the texture directions, for bumpmapping and stuff
			if (mod->normals && (surf->texinfo->flags & 0x800) && (mod->normals[vertidx][0] || mod->normals[vertidx][1] || mod->normals[vertidx][2]))
			{
				//per-vertex normals - used for smoothing groups and stuff.
				VectorCopy(mod->normals[vertidx], mesh->normals_array[i]);
			}
			else
			{
				if (surf->flags & SURF_PLANEBACK)
					VectorNegate(surf->plane->normal, mesh->normals_array[i]);
				else
					VectorCopy(surf->plane->normal, mesh->normals_array[i]);
			}
			VectorCopy(surf->texinfo->vecs[0], mesh->snormals_array[i]);
			VectorNegate(surf->texinfo->vecs[1], mesh->tnormals_array[i]);
			//for q1bsp the s+t vectors are usually axis-aligned, so fiddle them so they're normal aligned instead
			d = -DotProduct(mesh->normals_array[i], mesh->snormals_array[i]);
			VectorMA(mesh->snormals_array[i], d, mesh->normals_array[i], mesh->snormals_array[i]);
			d = -DotProduct(mesh->normals_array[i], mesh->tnormals_array[i]);
			VectorMA(mesh->tnormals_array[i], d, mesh->normals_array[i], mesh->tnormals_array[i]);
			VectorNormalize(mesh->snormals_array[i]);
			VectorNormalize(mesh->tnormals_array[i]);
		}

		//q1bsp has no colour information (fixme: sample from the lightmap?)
		for (sty = 0; sty < 1; sty++)
		{
			mesh->colors4f_array[sty][i][0] = 1;
			mesh->colors4f_array[sty][i][1] = 1;
			mesh->colors4f_array[sty][i][2] = 1;
			mesh->colors4f_array[sty][i][3] = 1;
		}
	}
}
#endif

#ifndef SERVERONLY
static void Mod_Batches_BuildModelMeshes(model_t *mod, int maxverts, int maxindicies, void (*build)(model_t *mod, msurface_t *surf, builddata_t *bd), builddata_t *bd, int lmmerge)
{
	batch_t *batch;
	msurface_t *surf;
	mesh_t *mesh;
	int numverts = 0;
	int numindicies = 0;
	int j, i;
	int sortid;
	int sty;
	vbo_t vbo;
	int styles = mod->lightmaps.surfstyles;
	char *ptr;

	memset(&vbo, 0, sizeof(vbo));
	vbo.indicies.sysptr = ZG_Malloc(&mod->memgroup, sizeof(index_t) * maxindicies);
	ptr = ZG_Malloc(&mod->memgroup, (sizeof(vecV_t)+sizeof(vec2_t)*(1+styles)+sizeof(vec3_t)*3+sizeof(vec4_t)*styles)* maxverts);

	vbo.coord.sysptr = ptr;
	ptr += sizeof(vecV_t)*maxverts;
	for (sty = 0; sty < styles; sty++)
	{
		vbo.colours[sty].sysptr = ptr;
		ptr += sizeof(vec4_t)*maxverts;
	}
	for (; sty < MAXRLIGHTMAPS; sty++)
		vbo.colours[sty].sysptr = NULL;
	vbo.texcoord.sysptr = ptr;
	ptr += sizeof(vec2_t)*maxverts;
	sty = 0;
	for (; sty < styles; sty++)
	{
		vbo.lmcoord[sty].sysptr = ptr;
		ptr += sizeof(vec2_t)*maxverts;
	}
	for (; sty < MAXRLIGHTMAPS; sty++)
		vbo.lmcoord[sty].sysptr = NULL;
	vbo.normals.sysptr = ptr;
	ptr += sizeof(vec3_t)*maxverts;
	vbo.svector.sysptr = ptr;
	ptr += sizeof(vec3_t)*maxverts;
	vbo.tvector.sysptr = ptr;
	ptr += sizeof(vec3_t)*maxverts;

	numindicies = 0;
	numverts = 0;

	//build each mesh
	for (sortid=0; sortid<SHADER_SORT_COUNT; sortid++)
	{
		for (batch = mod->batches[sortid]; batch; batch = batch->next)
		{
			for (j = 0; j < batch->maxmeshes; j++)
			{
				surf = (msurface_t*)batch->mesh[j];
				mesh = surf->mesh;
				batch->mesh[j] = mesh;

				mesh->vbofirstvert = numverts;
				mesh->vbofirstelement = numindicies;
				numverts += mesh->numvertexes;
				numindicies += mesh->numindexes;

				//set up the arrays. the arrangement is required for the backend to optimise vbos
				mesh->xyz_array = (vecV_t*)vbo.coord.sysptr + mesh->vbofirstvert;
				mesh->st_array = (vec2_t*)vbo.texcoord.sysptr + mesh->vbofirstvert;
				for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
				{
					if (vbo.lmcoord[sty].sysptr)
						mesh->lmst_array[sty] = (vec2_t*)vbo.lmcoord[sty].sysptr + mesh->vbofirstvert;
					else
						mesh->lmst_array[sty] = NULL;
					if (vbo.colours[sty].sysptr)
						mesh->colors4f_array[sty] = (vec4_t*)vbo.colours[sty].sysptr + mesh->vbofirstvert;
					else
						mesh->colors4f_array[sty] = NULL;
				}
				mesh->normals_array = (vec3_t*)vbo.normals.sysptr + mesh->vbofirstvert;
				mesh->snormals_array = (vec3_t*)vbo.svector.sysptr + mesh->vbofirstvert;
				mesh->tnormals_array = (vec3_t*)vbo.tvector.sysptr + mesh->vbofirstvert;
				mesh->indexes = (index_t*)vbo.indicies.sysptr + mesh->vbofirstelement;

				mesh->vbofirstvert = 0;
				mesh->vbofirstelement = 0;

				build(mod, surf, bd);

				if (lmmerge != 1)
				{
					for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
					{
						if (surf->lightmaptexturenums[sty] >= 0)
						{
							if (mod->lightmaps.deluxemapping)
								surf->lightmaptexturenums[sty] /= 2;
							if (mesh->lmst_array[sty])
							{
								int soffset = surf->lightmaptexturenums[sty] % mod->lightmaps.mergew;
								int toffset = surf->lightmaptexturenums[sty] / mod->lightmaps.mergew;
								float smul = 1.0/mod->lightmaps.mergew;
								float tmul = 1.0/mod->lightmaps.mergeh;
								for (i = 0; i < mesh->numvertexes; i++)
								{
									mesh->lmst_array[sty][i][0] += soffset;
									mesh->lmst_array[sty][i][0] *= smul;
									mesh->lmst_array[sty][i][1] += toffset;
									mesh->lmst_array[sty][i][1] *= tmul;
								}
							}
							surf->lightmaptexturenums[sty] /= lmmerge;
							if (mod->lightmaps.deluxemapping)
								surf->lightmaptexturenums[sty] *= 2;
						}
					}
				}
			}
			batch->meshes = 0;
			batch->firstmesh = 0;
		}
	}
}

#ifdef Q1BSPS
//q1 autoanimates. if the frame is set, it uses the alternate animation.
static void Mod_UpdateBatchShader_Q1 (struct batch_s *batch)
{
	texture_t *base = batch->texture;
	unsigned int	relative;
	int				count;

	if (batch->ent->framestate.g[FS_REG].frame[0])
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (base->anim_total)
	{
		relative = (unsigned int)(cl.time*10) % base->anim_total;

		count = 0;
		while (base->anim_min > relative || base->anim_max <= relative)
		{
			base = base->anim_next;
			if (!base)
				Sys_Error ("R_TextureAnimation: broken cycle");
			if (++count > 100)
				Sys_Error ("R_TextureAnimation: infinite cycle");
		}
	}

	batch->shader = base->shader;
}

// copy of Q1s, but with a different framerate
static void Mod_UpdateBatchShader_HL (struct batch_s *batch)
{
	texture_t *base = batch->texture;
	unsigned int	relative;
	int				count;

	if (batch->ent->framestate.g[FS_REG].frame[0])
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (base->anim_total)
	{
		relative = (unsigned int)(cl.time*20) % base->anim_total;

		count = 0;
		while (base->anim_min > relative || base->anim_max <= relative)
		{
			base = base->anim_next;
			if (!base)
				Sys_Error ("R_TextureAnimation: broken cycle");
			if (++count > 100)
				Sys_Error ("R_TextureAnimation: infinite cycle");
		}
	}

	batch->shader = base->shader;
}
#endif

#ifdef Q2BSPS
//q2 has direct control over the texture frames used, but typically has the client generate the frame (different flags autogenerate different ranges).
static void Mod_UpdateBatchShader_Q2 (struct batch_s *batch)
{
	texture_t *base = batch->texture;
	int		reletive;
	int frame = batch->ent->framestate.g[FS_REG].frame[0];
	if (batch->ent == &r_worldentity)
		frame = cl.time*2;

	if (base->anim_total)
	{
		reletive = frame % base->anim_total;
		while (reletive --> 0)
		{
			base = base->anim_next;
			if (!base)
				Sys_Error ("R_TextureAnimation: broken cycle");
		}
	}

	batch->shader = base->shader;
}
#endif

#define lmmerge(i) ((i>=0)?i/merge:i)
/*
batch->firstmesh is set only in and for this function, its cleared out elsewhere
*/
static int Mod_Batches_Generate(model_t *mod)
{
//#define NOBATCH	//define this to force each surface into its own batch...

	int i;
	msurface_t *surf;
	shader_t *shader;
	int sortid;
	batch_t *batch, *lbatch = NULL;
	vec4_t plane;
	image_t *envmap;

	int merge = mod->lightmaps.mergew*mod->lightmaps.mergeh;
	if (!merge)
		merge = mod->lightmaps.mergew = mod->lightmaps.mergeh = 1;	//no division by 0 please...
	if (mod->lightmaps.deluxemapping)
	{
		mod->lightmaps.count = ((mod->lightmaps.count+1)/2+merge-1) & ~(merge-1);
		mod->lightmaps.count /= merge;
		mod->lightmaps.count *= 2;
	}
	else
	{
		mod->lightmaps.count = (mod->lightmaps.count+merge-1) & ~(merge-1);
		mod->lightmaps.count /= merge;
	}
	mod->lightmaps.width *= mod->lightmaps.mergew;
	mod->lightmaps.height *= mod->lightmaps.mergeh;

	mod->numbatches = 0;

	//for each surface, find a suitable batch to insert it into.
	//we use 'firstmesh' to avoid chucking out too many verts in a single vbo (gl2 hardware tends to have a 16bit limit)
	for (i=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + mod->firstmodelsurface + i;
		shader = surf->texinfo->texture->shader;
		envmap = surf->envmap;

		if (surf->flags & SURF_NODRAW)
		{
			shader = R_RegisterShader("nodraw", SUF_NONE, "{\nsurfaceparm nodraw\n}");
			sortid = shader->sort;
			VectorClear(plane);
			plane[3] = 0;
			envmap = NULL;
		}
		else if (shader)
		{
			sortid = shader->sort;

			//shaders that are portals need to be split into separate batches to have the same surface planes
			if (sortid == SHADER_SORT_PORTAL || (shader->flags & (SHADER_HASREFLECT | SHADER_HASREFRACT)))
			{
				if (surf->flags & SURF_PLANEBACK)
				{
					VectorNegate(surf->plane->normal, plane);
					plane[3] = -surf->plane->dist;
				}
				else
				{
					VectorCopy(surf->plane->normal, plane);
					plane[3] = surf->plane->dist;
				}
			}
			else
			{
				VectorClear(plane);
				plane[3] = 0;
			}

			if (!(shader->flags & SHADER_HASREFLECTCUBE))
				envmap = NULL;
		}
		else
		{
			sortid = SHADER_SORT_OPAQUE;
			VectorClear(plane);
			plane[3] = 0;
		}

#ifdef NOBATCH
		batch = NULL;
		(void)lbatch;
#else
		if (lbatch && (
					lbatch->texture == surf->texinfo->texture &&
					lbatch->shader == shader &&
					lbatch->lightmap[0] == lmmerge(surf->lightmaptexturenums[0]) &&
					Vector4Compare(plane, lbatch->user.bmodel.plane) &&
					lbatch->firstmesh + surf->mesh->numvertexes <= MAX_INDICIES &&
	#if MAXRLIGHTMAPS > 1
					lbatch->lightmap[1] == lmmerge(surf->lightmaptexturenums[1]) &&
					lbatch->lightmap[2] == lmmerge(surf->lightmaptexturenums[2]) &&
					lbatch->lightmap[3] == lmmerge(surf->lightmaptexturenums[3]) &&
	#endif
					lbatch->fog == surf->fog &&
					lbatch->envmap == envmap))
			batch = lbatch;
		else
		{
			for (batch = mod->batches[sortid]; batch; batch = batch->next)
			{
				if (
							batch->texture == surf->texinfo->texture &&
							batch->shader == shader &&
							batch->lightmap[0] == lmmerge(surf->lightmaptexturenums[0]) &&
							Vector4Compare(plane, batch->user.bmodel.plane) &&
							batch->firstmesh + surf->mesh->numvertexes <= MAX_INDICIES &&
	#if MAXRLIGHTMAPS > 1
							batch->lightmap[1] == lmmerge(surf->lightmaptexturenums[1]) &&
							batch->lightmap[2] == lmmerge(surf->lightmaptexturenums[2]) &&
							batch->lightmap[3] == lmmerge(surf->lightmaptexturenums[3]) &&
	#endif
							batch->fog == surf->fog &&
							batch->envmap == envmap)
					break;
			}
		}
#endif
		if (!batch)
		{
			batch = ZG_Malloc(&mod->memgroup, sizeof(*batch));
			batch->lightmap[0] = lmmerge(surf->lightmaptexturenums[0]);
#if MAXRLIGHTMAPS > 1
			batch->lightmap[1] = lmmerge(surf->lightmaptexturenums[1]);
			batch->lightmap[2] = lmmerge(surf->lightmaptexturenums[2]);
			batch->lightmap[3] = lmmerge(surf->lightmaptexturenums[3]);
#endif
			batch->texture = surf->texinfo->texture;
			batch->shader = shader;
			if (surf->texinfo->texture->alternate_anims || surf->texinfo->texture->anim_total)
			{
				switch (mod->fromgame)
				{
#ifdef Q2BSPS
				case fg_quake2:
					batch->buildmeshes = Mod_UpdateBatchShader_Q2;
					break;
#endif
#ifdef Q1BSPS
				case fg_quake:
					batch->buildmeshes = Mod_UpdateBatchShader_Q1;
					break;
				case fg_halflife:
					batch->buildmeshes = Mod_UpdateBatchShader_HL;
					break;
#endif
				default:
					break;
				}
			}
			batch->next = mod->batches[sortid];
			batch->ent = &r_worldentity;
			batch->fog = surf->fog;
			batch->envmap = envmap;
			Vector4Copy(plane, batch->user.bmodel.plane);

			mod->batches[sortid] = batch;
		}
		batch->user.bmodel.ebobatch = -1;

		surf->sbatch = batch;	//let the surface know which batch its in
		batch->maxmeshes++;
		batch->firstmesh += surf->mesh->numvertexes;

		lbatch = batch;
	}
	return merge;
#undef lmmerge
}

void Mod_LightmapAllocInit(lmalloc_t *lmallocator, qboolean hasdeluxe, unsigned int width, unsigned int height, int firstlm)
{
	memset(lmallocator, 0, sizeof(*lmallocator));
	lmallocator->deluxe = hasdeluxe;
	lmallocator->lmnum = firstlm;
	lmallocator->firstlm = firstlm;

	lmallocator->width = min(LMBLOCK_SIZE_MAX, width);
	lmallocator->height = min(LMBLOCK_SIZE_MAX, height);
}
void Mod_LightmapAllocDone(lmalloc_t *lmallocator, model_t *mod)
{
	mod->lightmaps.first = lmallocator->firstlm;
	mod->lightmaps.count = (lmallocator->lmnum - lmallocator->firstlm);
	if (lmallocator->allocated[0])	//lmnum was only *COMPLETE* lightmaps that we allocated, and does not include the one we're currently building.
		mod->lightmaps.count++;

	if (lmallocator->deluxe)
	{
		mod->lightmaps.first*=2;
		mod->lightmaps.count*=2;
		mod->lightmaps.deluxemapping = true;
	}
	else
		mod->lightmaps.deluxemapping = false;
}
void Mod_LightmapAllocBlock(lmalloc_t *lmallocator, int w, int h, unsigned short *x, unsigned short *y, int *tnum)
{
	int best, best2;
	int i, j;

	for(;;)
	{
		best = lmallocator->height;

		for (i = 0; i <= lmallocator->width - w; i++)
		{
			best2 = 0;

			for (j=0; j < w; j++)
			{
				if (lmallocator->allocated[i+j] >= best)
					break;
				if (lmallocator->allocated[i+j] > best2)
					best2 = lmallocator->allocated[i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > lmallocator->height)
		{
			memset(lmallocator->allocated, 0, sizeof(lmallocator->allocated));
			lmallocator->lmnum++;
			continue;
		}

		for (i=0; i < w; i++)
			lmallocator->allocated[*x + i] = best + h;

		if (lmallocator->deluxe)
			*tnum = lmallocator->lmnum*2;
		else
			*tnum = lmallocator->lmnum;
		break;
	}
}

#ifdef Q3BSPS
static void Mod_Batches_SplitLightmaps(model_t *mod, int lmmerge)
{
	batch_t *batch;
	batch_t *nb;
	int i, j, sortid;
	msurface_t *surf;
	int sty;
	int lmscale = 1;

	if (mod->lightmaps.deluxemapping)
	{
		lmmerge *= 2;
		lmscale *= 2;
	}

	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	for (batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
	{
		surf = (msurface_t*)batch->mesh[0];
		for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
		{
			batch->lightmap[sty] = (surf->lightmaptexturenums[sty]>=0)?lmscale*(surf->lightmaptexturenums[sty]/lmmerge):surf->lightmaptexturenums[sty];
			batch->lmlightstyle[sty] = surf->styles[sty];
			batch->vtlightstyle[sty] = surf->vlstyles[sty];
		}

		for (j = 1; j < batch->maxmeshes; j++)
		{
			surf = (msurface_t*)batch->mesh[j];
			for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
			{
				int lm = (surf->lightmaptexturenums[sty]>=0)?lmscale*(surf->lightmaptexturenums[sty]/lmmerge):surf->lightmaptexturenums[sty];
				if (lm != batch->lightmap[sty] ||
					//fixme: we should merge later (reverted matching) surfaces into the prior batch
					surf->styles[sty] != batch->lmlightstyle[sty] ||
					surf->vlstyles[sty] != batch->vtlightstyle[sty])
					break;
			}
			if (sty < MAXRLIGHTMAPS)
			{
				nb = ZG_Malloc(&mod->memgroup, sizeof(*batch));
				*nb = *batch;
				batch->next = nb;

				nb->mesh = batch->mesh + j*2;
				nb->maxmeshes = batch->maxmeshes - j;
				batch->maxmeshes = j;
				for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
				{
					int lm = (surf->lightmaptexturenums[sty]>=0)?lmscale*(surf->lightmaptexturenums[sty]/lmmerge):surf->lightmaptexturenums[sty];
					nb->lightmap[sty] = lm;
					nb->lmlightstyle[sty] = surf->styles[sty];
					nb->vtlightstyle[sty] = surf->vlstyles[sty];
				}

				memmove(nb->mesh, batch->mesh+j, sizeof(msurface_t*)*nb->maxmeshes);

				for (i = 0; i < nb->maxmeshes; i++)
				{
					surf = (msurface_t*)nb->mesh[i];
					surf->sbatch = nb;
				}

				batch = nb;
				j = 1;
			}
		}
	}
}
#endif

#if defined(Q1BSPS) || defined(Q2BSPS)
static void Mod_LightmapAllocSurf(lmalloc_t *lmallocator, msurface_t *surf, int surfstyle)
{
	int smax, tmax;
	smax = (surf->extents[0]>>surf->lmshift)+1;
	tmax = (surf->extents[1]>>surf->lmshift)+1;

	if (isDedicated ||
		(surf->texinfo->texture->shader && !(surf->texinfo->texture->shader->flags & SHADER_HASLIGHTMAP)) || //fte
		(surf->flags & (SURF_DRAWSKY|SURF_DRAWTILED)) ||	//q1
		(surf->texinfo->flags & TEX_SPECIAL) ||	//the original 'no lightmap'
		smax > lmallocator->width || tmax > lmallocator->height || smax < 0 || tmax < 0)	//bugs/bounds/etc
	{
		surf->lightmaptexturenums[surfstyle] = -1;
		return;
	}

	Mod_LightmapAllocBlock (lmallocator, smax, tmax, &surf->light_s[surfstyle], &surf->light_t[surfstyle], &surf->lightmaptexturenums[surfstyle]);
}

/*
allocates lightmaps and splits batches upon lightmap boundaries
*/
static void Mod_Batches_AllocLightmaps(model_t *mod)
{
	batch_t *batch;
	batch_t *nb;
	lmalloc_t lmallocator;
	int i, j, sortid;
	msurface_t *surf;
	int sty;

	size_t samps = 0;

	//small models don't have many surfaces, don't allocate a smegging huge lightmap that simply won't be used.
	for (i=0, j=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + mod->firstmodelsurface + i;
		if (surf->texinfo->flags & TEX_SPECIAL)
			continue;	//surfaces with no lightmap should not count torwards anything.
		samps += ((surf->extents[0]>>surf->lmshift)+1) * ((surf->extents[1]>>surf->lmshift)+1);

		if (j < (surf->extents[0]>>surf->lmshift)+1)
			j = (surf->extents[0]>>surf->lmshift)+1;
		if (j < (surf->extents[1]>>surf->lmshift)+1)
			j = (surf->extents[1]>>surf->lmshift)+1;
	}
	samps /= 4;
	samps = sqrt(samps);
	if (j > 128 || r_dynamic.ival <= 0)
		samps *= 2;
	mod->lightmaps.width = bound(j, samps, LMBLOCK_SIZE_MAX);
	mod->lightmaps.height = bound(j, samps, LMBLOCK_SIZE_MAX);
	for (i = 0; (1<<i) < mod->lightmaps.width; i++);
	mod->lightmaps.width = 1<<i;
	for (i = 0; (1<<i) < mod->lightmaps.height; i++);
	mod->lightmaps.height = 1<<i;
	mod->lightmaps.width = bound(64, mod->lightmaps.width, sh_config.texture2d_maxsize);
	mod->lightmaps.height = bound(64, mod->lightmaps.height, sh_config.texture2d_maxsize);

	Mod_LightmapAllocInit(&lmallocator, mod->deluxdata != NULL, mod->lightmaps.width, mod->lightmaps.height, 0x50);

	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	for (batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
	{
		surf = (msurface_t*)batch->mesh[0];
		Mod_LightmapAllocSurf (&lmallocator, surf, 0);
		for (sty = 1; sty < MAXRLIGHTMAPS; sty++)
			surf->lightmaptexturenums[sty] = -1;
		for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
		{
			batch->lightmap[sty] = surf->lightmaptexturenums[sty];
			batch->lmlightstyle[sty] = INVALID_LIGHTSTYLE;//don't do special backend rendering of lightstyles.
			batch->vtlightstyle[sty] = 255;//don't do special backend rendering of lightstyles.
		}

		for (j = 1; j < batch->maxmeshes; j++)
		{
			surf = (msurface_t*)batch->mesh[j];
			Mod_LightmapAllocSurf (&lmallocator, surf, 0);
			for (sty = 1; sty < MAXRLIGHTMAPS; sty++)
				surf->lightmaptexturenums[sty] = -1;
			if (surf->lightmaptexturenums[0] != batch->lightmap[0])
			{
				nb = ZG_Malloc(&mod->memgroup, sizeof(*batch));
				*nb = *batch;
				batch->next = nb;

				nb->mesh = batch->mesh + j*2;
				nb->maxmeshes = batch->maxmeshes - j;
				batch->maxmeshes = j;
				for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
					nb->lightmap[sty] = surf->lightmaptexturenums[sty];

				memmove(nb->mesh, batch->mesh+j, sizeof(msurface_t*)*nb->maxmeshes);

				for (i = 0; i < nb->maxmeshes; i++)
				{
					surf = (msurface_t*)nb->mesh[i];
					surf->sbatch = nb;
				}

				batch = nb;
				j = 0;
			}
		}
	}

	Mod_LightmapAllocDone(&lmallocator, mod);
}
#endif

extern void Surf_CreateSurfaceLightmap (msurface_t *surf, int shift);
//if build is NULL, uses q1/q2 surf generation, and allocates lightmaps
void Mod_Batches_Build(model_t *mod, builddata_t *bd)
{
	int i;
	int numverts = 0, numindicies=0;
	msurface_t *surf;
	mesh_t *mesh;
	mesh_t **bmeshes;
	int sortid;
	batch_t *batch;
	mesh_t *meshlist;
	int merge = 1;

	if (!mod->textures)
		return;

	if (mod->firstmodelsurface + mod->nummodelsurfaces > mod->numsurfaces)
		Sys_Error("submodel %s surface range is out of bounds\n", mod->name);

	if (bd)
		meshlist = NULL;
	else
		meshlist = ZG_Malloc(&mod->memgroup, sizeof(mesh_t) * mod->nummodelsurfaces);

	for (i=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + i + mod->firstmodelsurface;
		if (meshlist)
		{
			mesh = surf->mesh = &meshlist[i];
			mesh->numvertexes = surf->numedges;
			mesh->numindexes = (surf->numedges-2)*3;
		}
		else
			mesh = surf->mesh;

		if (mesh->numindexes <= 0 || mesh->numvertexes < 1)
		{
			mesh->numindexes = 0;
			mesh->numvertexes = 0;
		}

		numverts += mesh->numvertexes;
		numindicies += mesh->numindexes;
//		surf->lightmaptexturenum = -1;
	}

	/*assign each mesh to a batch, generating as needed*/
	merge = Mod_Batches_Generate(mod);

	bmeshes = ZG_Malloc(&mod->memgroup, sizeof(*bmeshes)*mod->nummodelsurfaces*R_MAX_RECURSE);

	//we now know which batch each surface is in, and how many meshes there are in each batch.
	//allocate the mesh-pointer-lists for each batch. *2 for recursion.
	for (i = 0, sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	for (batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
	{
		batch->mesh = bmeshes + i;
		i += batch->maxmeshes*R_MAX_RECURSE;
	}
	//store the *surface* into the batch's mesh list (yes, this is an evil cast hack, but at least both are pointers)
	for (i=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + mod->firstmodelsurface + i;
		surf->sbatch->mesh[surf->sbatch->meshes++] = (mesh_t*)surf;
	}

#if defined(Q1BSPS) || defined(Q2BSPS)
	if (!bd)
	{
		Mod_Batches_AllocLightmaps(mod);

		mod->lightmaps.surfstyles = 1;
		Mod_Batches_BuildModelMeshes(mod, numverts, numindicies, ModQ1_Batches_BuildQ1Q2Poly, bd, merge);
	}
#endif
	if (bd)
	{
		if (bd->paintlightmaps)
			Mod_Batches_AllocLightmaps(mod);
		else
			Mod_Batches_SplitLightmaps(mod, merge);
		Mod_Batches_BuildModelMeshes(mod, numverts, numindicies, bd->buildfunc, bd, merge);
	}

	if (BE_GenBrushModelVBO)
		BE_GenBrushModelVBO(mod);
}
#endif


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	if (!node)
		return;
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

#if defined(Q1BSPS) || defined(Q2BSPS)
/*
=================
Mod_LoadEdges
=================
*/
qboolean Mod_LoadEdges (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t subbsp)
{
	medge_t *out;
	int 	i, count;
	
	if (subbsp == sb_long1 || subbsp == sb_long2)
	{
		dledge_t *in = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*in);
		if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, (count + 1) * sizeof(*out));	

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			out->v[0] = LittleLong(in->v[0]);
			out->v[1] = LittleLong(in->v[1]);
		}
	}
	else
	{
		dsedge_t *in = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*in);
		if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, (count + 1) * sizeof(*out));

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			out->v[0] = (unsigned short)LittleShort(in->v[0]);
			out->v[1] = (unsigned short)LittleShort(in->v[1]);
		}
	}

	return true;
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
qboolean Mod_LoadMarksurfaces (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t subbsp)
{	
	int		i, j, count;
	msurface_t **out;

	if (subbsp == sb_long1 || subbsp == sb_long2)
	{
		int		*inl;
		inl = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*inl);
		if (l->filelen % sizeof(*inl) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		for ( i=0 ; i<count ; i++)
		{
			j = (unsigned int)LittleLong(inl[i]);
			if (j >= loadmodel->numsurfaces)
			{
				Con_Printf (CON_ERROR "Mod_ParseMarksurfaces: bad surface number\n");
				return false;
			}
			out[i] = loadmodel->surfaces + j;
		}
	}
	else
	{
		short		*ins;
		ins = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*ins);
		if (l->filelen % sizeof(*ins) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		for ( i=0 ; i<count ; i++)
		{
			j = (unsigned short)LittleShort(ins[i]);
			if (j >= loadmodel->numsurfaces)
			{
				Con_Printf (CON_ERROR "Mod_ParseMarksurfaces: bad surface number\n");
				return false;
			}
			out[i] = loadmodel->surfaces + j;
		}
	}

	return true;
}

/*
=================
Mod_LoadSurfedges
=================
*/
qboolean Mod_LoadSurfedges (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);

	return true;
}
#endif
#ifdef Q1BSPS
/*
=================
Mod_LoadVisibility
=================
*/
static void Mod_LoadVisibility (model_t *loadmodel, qbyte *mod_base, lump_t *l, qbyte *ptr, size_t len)
{
	if (!ptr)
	{
		ptr = mod_base + l->fileofs;
		len = l->filelen;
	}
	if (!len)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = ZG_Malloc(&loadmodel->memgroup, len);	
	memcpy (loadmodel->visdata, ptr, len);
}

#ifndef SERVERONLY
static void Mod_LoadMiptex(model_t *loadmodel, texture_t *tx, miptex_t *mt, int mtsize, qbyte *ptr, size_t miptexsize)
{
	unsigned int legacysize =
		(mt->width>>0)*(mt->height>>0) +
		(mt->width>>1)*(mt->height>>1) +
		(mt->width>>2)*(mt->height>>2) +
		(mt->width>>3)*(mt->height>>3);

	uploadfmt_t newfmt = PTI_INVALID;
	size_t neww=0, newh=0;
	qbyte *newdata=NULL;
	qbyte *pal = NULL;
	int m;

	//bug: vanilla quake ignored offsets and just made assumptions.
	//this means we can't just play with offsets to hide stuff, we have to postfix it (which requires guessing lump sizes)
	//issue: halflife textures have (leshort)256,(byte)pal[256*3] stuck on the end
	//we signal the presence of our extended data using 0x00,0xfb,0x2b,0xaf (this should be uncommon as the next mip's name shouldn't normally be empty, nor a weird char (which should hopefully also not come from random stack junk in the wad tool)
	//each extended block of data then has a size value, followed by a block name.
	//compressed formats then contain a width+height value, and then a FULL (round-down) mip chain.
	//if the gpu doesn't support npot, or its too big, or can't use the pixelformat then the engine will simply have to fall back on the paletted data. lets hope it was present.
	size_t extofs;
	if (!mt->offsets[0])
		extofs = mtsize;
	else if (mt->offsets[0] == mtsize &&
			 mt->offsets[1] == mt->offsets[0]+(mt->width>>0)*(mt->height>>0) &&
			 mt->offsets[2] == mt->offsets[1]+(mt->width>>1)*(mt->height>>1) &&
			 mt->offsets[3] == mt->offsets[2]+(mt->width>>2)*(mt->height>>2))
	{
		extofs = mt->offsets[3]+(mt->width>>3)*(mt->height>>3);
		if (extofs + 2+256*3 <= miptexsize && *(short*)(ptr + extofs) == 256)
		{	//space for a halflife paletted texture, with the right signature (note: usually padded).
			pal = ptr + extofs+2;
			extofs += 2+256*3;
		}
	}
	else
		extofs = miptexsize;	//the numbers don't match what we expect... something weird is going on here... don't misinterpret it.
	if (extofs+4 <= miptexsize && ptr[extofs+0] == 0 && ptr[extofs+1]==0xfb && ptr[extofs+2]==0x2b && ptr[extofs+3]==0xaf)
	{
		unsigned int extsize;
		extofs += 4;
		for (; extofs < miptexsize; extofs += extsize)
		{
			size_t sz, w, h;
			unsigned int bb,bw,bh,bd;
			int mip;
			qbyte *extdata = (void*)(ptr+extofs);
			char *extfmt = (char*)(extdata+4);
			extsize = (extdata[0]<<0)|(extdata[1]<<8)|(extdata[2]<<16)|(extdata[3]<<24);
			if (extsize<8 || extofs+extsize>miptexsize) break;	//not a valid entry... something weird is happening here
			else if (!strncmp(extfmt, "NAME", 4))
			{	//replacement name, for longer shader/external names
				size_t sz = extsize-8;
				if (sz >= sizeof(tx->name))
					continue;
				memcpy(tx->name, (qbyte*)extdata+8, sz);
				tx->name[sz] = 0;
			}
			else if (!strncmp(extfmt, "LPAL", 4) && extsize == 8+256*3)
			{	//replacement palette for the 8bit data, for feature parity with halflife, but with extra markup so we know its actually meant to be a replacement palette.
				pal = extdata+8;
				continue;
			}
			else if (extsize <= 16) continue;										//too small for an altformat lump
			else if (newfmt != PTI_INVALID)	continue;								//only accept the first accepted format (allowing for eg astc+bc1 fallbacks)
			else if (!strncmp(extfmt, "RGBA", 4))	newfmt = PTI_RGBA8;				//32bpp, we don't normally need this alpha precision (padding can be handy though, for the lazy).
			else if (!strncmp(extfmt, "RGBX", 4))	newfmt = PTI_RGBX8;				//32bpp, we don't normally need this alpha precision (padding can be handy though, for the lazy).
			else if (!strncmp(extfmt, "RGB", 4))	newfmt = PTI_RGB8;				//24bpp
			else if (!strncmp(extfmt, "565", 4))	newfmt = PTI_RGB565;			//16bpp
			else if (!strncmp(extfmt, "4444", 4))	newfmt = PTI_RGBA4444;			//16bpp
			else if (!strncmp(extfmt, "5551", 4))	newfmt = PTI_RGBA5551;			//16bpp
			else if (!strncmp(extfmt, "LUM8", 4))	newfmt = PTI_L8;			//8bpp
			else if (!strncmp(extfmt, "EXP5", 4))	newfmt = PTI_E5BGR9;			//32bpp, we don't normally need this alpha precision...
			else if (!strncmp(extfmt, "BC1", 4))	newfmt = PTI_BC1_RGBA;			//4bpp
			else if (!strncmp(extfmt, "BC2", 4))	newfmt = PTI_BC2_RGBA;			//8bpp, we don't normally need this alpha precision...
			else if (!strncmp(extfmt, "BC3", 4))	newfmt = PTI_BC3_RGBA;			//8bpp, we don't normally need this alpha precision...
			else if (!strncmp(extfmt, "BC4", 4))	newfmt = PTI_BC4_R;				//4bpp, wtf
			else if (!strncmp(extfmt, "BC5", 4))	newfmt = PTI_BC5_RG;			//8bpp, wtf
			else if (!strncmp(extfmt, "BC6", 4))	newfmt = PTI_BC6_RGB_UFLOAT;	//8bpp, weird
			else if (!strncmp(extfmt, "BC7", 4))	newfmt = PTI_BC7_RGBA;			//8bpp
			else if (!strncmp(extfmt, "AST4", 4))	newfmt = PTI_ASTC_4X4_LDR;		//8 bpp
			else if (!strncmp(extfmt, "AS54", 4))	newfmt = PTI_ASTC_5X4_LDR;		//6.40bpp
			else if (!strncmp(extfmt, "AST5", 4))	newfmt = PTI_ASTC_5X5_LDR;		//5.12bpp
			else if (!strncmp(extfmt, "AS65", 4))	newfmt = PTI_ASTC_6X5_LDR;		//4.17bpp
			else if (!strncmp(extfmt, "AST6", 4))	newfmt = PTI_ASTC_6X6_LDR;		//3.56bpp
			else if (!strncmp(extfmt, "AS85", 4))	newfmt = PTI_ASTC_8X5_LDR;		//3.20bpp
			else if (!strncmp(extfmt, "AS86", 4))	newfmt = PTI_ASTC_8X6_LDR;		//2.67bpp
			else if (!strncmp(extfmt, "AS05", 4))	newfmt = PTI_ASTC_10X5_LDR;		//2.56bpp
			else if (!strncmp(extfmt, "AS06", 4))	newfmt = PTI_ASTC_10X6_LDR;		//2.13bpp
			else if (!strncmp(extfmt, "AST8", 4))	newfmt = PTI_ASTC_8X8_LDR;		//2 bpp
			else if (!strncmp(extfmt, "AS08", 4))	newfmt = PTI_ASTC_10X8_LDR;		//1.60bpp
			else if (!strncmp(extfmt, "AS00", 4))	newfmt = PTI_ASTC_10X10_LDR;	//1.28bpp
			else if (!strncmp(extfmt, "AS20", 4))	newfmt = PTI_ASTC_12X10_LDR;	//1.07bpp
			else if (!strncmp(extfmt, "AST2", 4))	newfmt = PTI_ASTC_12X12_LDR;	//0.89bpp
			else if (!strncmp(extfmt, "ETC1", 4))	newfmt = PTI_ETC1_RGB8;			//4bpp
			else if (!strncmp(extfmt, "ETC2", 4))	newfmt = PTI_ETC2_RGB8;			//4bpp
			else if (!strncmp(extfmt, "ETCP", 4))	newfmt = PTI_ETC2_RGB8A1;		//4bpp
			else if (!strncmp(extfmt, "ETCA", 4))	newfmt = PTI_ETC2_RGB8A8;		//8bpp, we don't normally need this alpha precision...
			else continue;																//dunno what that is, ignore it

			//alternative textures are usually compressed
			//this means we insist on a FULL mip chain
			//npot mips are explicitly round-down (but don't drop to 0 with non-square).
			Image_BlockSizeForEncoding(newfmt, &bb, &bw, &bh, &bd);
			neww = (extdata[8]<<0)|(extdata[9]<<8)|(extdata[10]<<16)|(extdata[11]<<24);
			newh = (extdata[12]<<0)|(extdata[13]<<8)|(extdata[14]<<16)|(extdata[15]<<24);
			for (mip = 0, w=neww, h=newh, sz=0; w || h; mip++, w>>=1,h>>=1)
			{
				w = max(1, w);
				h = max(1, h);
				sz +=	bb *
						((w+bw-1)/bw) *
						((h+bh-1)/bh);
				//Support truncation to top-mip only? tempting...
			}
			if (extsize != 16+sz)
			{
				Con_Printf(CON_WARNING"miptex %s (%s) has incomplete mipchain\n", tx->name, Image_FormatName(newfmt));
				continue;
			}

			//make sure we're not going to need to rescale compressed formats.
			//gles<3 or gl<2 requires npot inputs for this to work. I guess that means dx9.3+ gpus, so all astc+bc7 but not necessarily all bc1+etc2. oh well.
			if (!sh_config.texture_non_power_of_two)
			{
				if (neww & (neww - 1))
					continue;
				if (newh & (newh - 1))
					continue;
			}
			//make sure its within our limits
			if (!neww || !newh || neww > sh_config.texture2d_maxsize || newh > sh_config.texture2d_maxsize)
				continue;
			//that our hardware supports it... (Note: FTE can soft-decompress all of the above so this doesn't make too much sense if there's only one)
			//if (!sh_config.texfmt[newfmt])
			//	continue;
			//that we can actually use non-paletted data...
			if (r_softwarebanding && mt->offsets[0])
				continue;

			newdata = BZ_Malloc(sz);
			memcpy(newdata, extdata+16, sz);
		}
	}

	if (newdata)
	{
		tx->srcfmt = newfmt|PTI_FULLMIPCHAIN;
		tx->srcwidth = neww;
		tx->srcheight = newh;
		tx->srcdata = newdata;
		tx->palette = NULL;
		return;
	}

	if (!mt->offsets[0])
	{
		tx->srcfmt = PTI_INVALID;
		tx->srcwidth = mt->width;
		tx->srcheight = mt->height;
		tx->srcdata = NULL;
		tx->palette = NULL;
		return;
	}

	if (pal)
	{	//mostly identical, just a specific palette hidden at the end. handle fences elsewhere.
		tx->srcdata = BZ_Malloc(legacysize + 768);
		tx->palette = tx->srcdata + legacysize;
		memcpy(tx->palette, pal, 768);
	}
	else
	{
		tx->srcdata = BZ_Malloc(legacysize);
		tx->palette = NULL;
	}

	if (tx->palette)
	{	//halflife, probably...
		if (*tx->name == '{')
			tx->srcfmt = TF_MIP4_8PAL24_T255;
		else
			tx->srcfmt = TF_MIP4_8PAL24;
	}
	else
	{
		if (*tx->name == '{')
			tx->srcfmt = TF_TRANS8;
		else
			tx->srcfmt = TF_MIP4_SOLID8;
	}
	tx->srcwidth = mt->width;
	tx->srcheight = mt->height;

	legacysize = 0;
	for (m = 0; m < 4; m++)
	{
		if (mt->offsets[m] && (mt->offsets[m]+(mt->width>>m)*(mt->height>>m)<=miptexsize))
			memcpy(tx->srcdata+legacysize, ptr + mt->offsets[m], (mt->width>>m)*(mt->height>>m));
		else
			memset(tx->srcdata+legacysize, 0, (mt->width>>m)*(mt->height>>m));
		legacysize += (mt->width>>m)*(mt->height>>m);
	}
}
#endif

/*
=================
Mod_LoadTextures
=================
*/
static qboolean Mod_LoadTextures (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t subbsp)
{
	int		i, j, num, max, altmax;
	texture_t	*tx, *tx2;
	texture_t	*anims[10];
	texture_t	*altanims[10];
	dmiptexlump_t *m;
	unsigned int *sizes;
	unsigned int e, o;
	int mtsize;

TRACE(("dbg: Mod_LoadTextures: inittexturedescs\n"));

//	Mod_InitTextureDescs(loadname);

	if (!l->filelen)
	{
		Con_Printf(CON_WARNING "warning: %s contains no texture data\n", loadmodel->name);

		loadmodel->numtextures = 1;
		loadmodel->textures = ZG_Malloc(&loadmodel->memgroup, 1 * sizeof(*loadmodel->textures));

		i = 0;
		tx = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));
		memcpy(tx, r_notexture_mip, sizeof(texture_t));
		sprintf(tx->name, "unnamed%i", i);
		loadmodel->textures[i] = tx;

		return true;
	}
	m = (dmiptexlump_t *)(mod_base + l->fileofs);

	m->nummiptex = LittleLong (m->nummiptex);

	if ((1+m->nummiptex)*sizeof(int) > l->filelen)
	{
		Con_Printf(CON_WARNING "warning: %s contains corrupt texture lump\n", loadmodel->name);
		return false;
	}

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = ZG_Malloc(&loadmodel->memgroup, m->nummiptex * sizeof(*loadmodel->textures));
	sizes = alloca(sizeof(*sizes)*m->nummiptex);

	mtsize = subbsp == sb_quake64 ? sizeof(q64miptex_t) : sizeof(miptex_t);

	for (i=m->nummiptex, e = l->filelen; i-->0; )
	{
		qbyte* ptr;
		miptex_t* mt;
		miptex_t tmp;
		int scale;

		o = LittleLong(m->dataofs[i]);
		if (o >= l->filelen)	//e1m2, this happens
		{
badmip:
			tx = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));
			memcpy(tx, r_notexture_mip, sizeof(texture_t));
			sprintf(tx->name, "unnamed%i", i);
			loadmodel->textures[i] = tx;
			continue;
		}
		if (o >= e)
			e = l->filelen; //something doesn't make sense. try to avoid making too many assumptions.

		ptr = (qbyte*)m + o;

		if (subbsp == sb_quake64)
		{
			q64miptex_t *q64mt = (q64miptex_t*)ptr;
			memcpy(tmp.name, q64mt->name, sizeof(tmp.name));
			mt = &tmp;
			mt->width = LittleLong(q64mt->width);
			mt->height = LittleLong(q64mt->height);
			for (j = 0; j < MIPLEVELS; j++)
				mt->offsets[j] = LittleLong(q64mt->offsets[j]);
			scale = LittleLong (q64mt->scale);
		} 
		else 
		{
			mt = (miptex_t*)ptr;
			mt->width = LittleLong(mt->width);
			mt->height = LittleLong(mt->height);
			for (j = 0; j < MIPLEVELS; j++)
				mt->offsets[j] = LittleLong(mt->offsets[j]);
		}

		TRACE(("dbg: Mod_LoadTextures: texture %s\n", mt->name));

		if (mt->offsets[0] && (mt->width > 0xffff|| mt->height > 0xffff))
		{
			Con_Printf(CON_WARNING "%s: miptex %i is excessively large. probably corrupt\n", loadmodel->name, i);
			goto badmip;
		}

		if (!*mt->name)	//I HATE MAPPERS!
		{
			Q_snprintfz(mt->name, sizeof(mt->name), "unnamed%i", i);
			Con_DPrintf(CON_WARNING "warning: unnamed texture in %s, renaming to %s\n", loadmodel->name, mt->name);
		}

		if ( (mt->width & 15) || (mt->height & 15) )
			Con_DPrintf (CON_WARNING "Warning: Texture %s is not 16 aligned", mt->name);
		if (mt->width < 1 || mt->height < 1)
			Con_Printf (CON_WARNING "Warning: Texture %s has no size", mt->name);
		tx = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));
		loadmodel->textures[i] = tx;

		Q_strncpyz(tx->name, mt->name, min(sizeof(mt->name)+1, sizeof(tx->name)));
		tx->vwidth = mt->width;
		tx->vheight = mt->height;
		if (subbsp == sb_quake64)
		{
			tx->vwidth <<= scale;
			tx->vheight <<= scale;
		}

#ifndef SERVERONLY
		Mod_LoadMiptex(loadmodel, tx, mt, mtsize, ptr, e-o);
#else
		(void)e;
		(void)mtsize;
#endif

		e = o;
	}
//
// sequence the animations
//
	for (i=0 ; i<m->nummiptex ; i++)
	{
		qboolean animvalid = true;

		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// already sequenced

	// find the number of frames in the animation
		memset (anims, 0, sizeof(anims));
		memset (altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
		{
			Con_Printf (CON_WARNING "Bad animating texture name %s\n", tx->name);
			continue;
		}

		for (j=i+1 ; j<m->nummiptex ; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp (tx2->name+2, tx->name+2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			else
			{
				continue;
			}
		}

#define	ANIM_CYCLE	2
		// validate
		for (j = 0; j < max; j++)
		{
			if (!anims[j])
			{
				Con_Printf(CON_WARNING "Missing frame %i of %s\n", j, tx->name);
				animvalid = false;
				break;
			}
		}
		for (j = 0; j < altmax && animvalid; j++)
		{
			if (!altanims[j])
			{
				Con_Printf(CON_WARNING "Missing alt frame %i of %s\n", j, tx->name);
				animvalid = false;
				break;
			}
		}

		// link them all together
		if (animvalid)
		{
			for (j = 0; j < max; j++)
			{
				tx2 = anims[j];
				tx2->anim_total = max * ANIM_CYCLE;
				tx2->anim_min = j * ANIM_CYCLE;
				tx2->anim_max = (j + 1) * ANIM_CYCLE;
				tx2->anim_next = anims[(j + 1) % max];
				if (altmax)
					tx2->alternate_anims = altanims[0];
			}
			for (j = 0; j < altmax; j++)
			{
				tx2 = altanims[j];
				tx2->anim_total = altmax * ANIM_CYCLE;
				tx2->anim_min = j * ANIM_CYCLE;
				tx2->anim_max = (j + 1) * ANIM_CYCLE;
				tx2->anim_next = altanims[(j + 1) % altmax];
				if (max)
					tx2->alternate_anims = anims[0];
			}
		}
	}

	return true;
}

/*
=================
Mod_LoadSubmodels
=================
*/
static qboolean Mod_LoadSubmodels (model_t *loadmodel, qbyte *mod_base, lump_t *l, qboolean *hexen2map)
{
	dq1model_t	*inq;
	dh2model_t	*inh;
	mmodel_t	*out;
	int			i, j, count;

	//this is crazy!

	inq = (void *)(mod_base + l->fileofs);
	inh = (void *)(mod_base + l->fileofs);
	if (!inq->numfaces)
	{
		*hexen2map = true;
		count = l->filelen / sizeof(*inh);
		if (l->filelen % sizeof(*inh) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->submodels = out;
		loadmodel->numsubmodels = count;

		for ( i=0 ; i<count ; i++, inh++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{	// spread the mins / maxs by a pixel
				out->mins[j] = LittleFloat (inh->mins[j]) - 1;
				out->maxs[j] = LittleFloat (inh->maxs[j]) + 1;
				out->origin[j] = LittleFloat (inh->origin[j]);
			}
			for (j=0 ; j<MAX_MAP_HULLSDH2 ; j++)
			{
				out->headnode[j] = LittleLong (inh->headnode[j]);
			}
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->headnode[j] = 0;
			for (j=0 ; j<MAX_MAP_HULLSDH2 ; j++)
				out->hullavailable[j] = true;
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->hullavailable[j] = false;
			out->visleafs = LittleLong (inh->visleafs);
			out->firstface = LittleLong (inh->firstface);
			out->numfaces = LittleLong (inh->numfaces);
		}

	}
	else
	{
		*hexen2map = false;
		count = l->filelen / sizeof(*inq);
		if (l->filelen % sizeof(*inq) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));	

		loadmodel->submodels = out;
		loadmodel->numsubmodels = count;

		for ( i=0 ; i<count ; i++, inq++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{	// spread the mins / maxs by a pixel
				out->mins[j] = LittleFloat (inq->mins[j]) - 1;
				out->maxs[j] = LittleFloat (inq->maxs[j]) + 1;
				out->origin[j] = LittleFloat (inq->origin[j]);
			}
			for (j=0 ; j<MAX_MAP_HULLSDQ1 ; j++)
			{
				out->headnode[j] = LittleLong (inq->headnode[j]);
			}
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->headnode[j] = 0;
			for (j=0 ; j<4 ; j++)
				out->hullavailable[j] = true;
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->hullavailable[j] = false;
			out->visleafs = LittleLong (inq->visleafs);
			out->firstface = LittleLong (inq->firstface);
			out->numfaces = LittleLong (inq->numfaces);
		}
	}

	return true;
}

/*
=================
Mod_LoadTexinfo
=================
*/
static qboolean Mod_LoadTexinfo (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	int		miptex;

	in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<4 ; j++)
		{
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat (in->vecs[1][j]);
		}
		if (mod_lightscale_broken.ival)
		{
			out->vecscale[0] = 1.0;
			out->vecscale[1] = 1.0;
		}
		else
		{
			out->vecscale[0] = 1.0/Length (out->vecs[0]);
			out->vecscale[1] = 1.0/Length (out->vecs[1]);
		}

		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);
	
		if (loadmodel->numtextures)
			out->texture = loadmodel->textures[miptex % loadmodel->numtextures];
		else
			out->texture = NULL;
		if (!out->texture)
		{
			out->texture = r_notexture_mip; // texture not found
			out->flags = 0;
		}
		else
		{
			if (*out->texture->name == '*' || (*out->texture->name == '!' && loadmodel->fromgame == fg_halflife))		// turbulent
			{
				if (!(out->flags & TEX_SPECIAL) && !strchr(out->texture->name, '#'))
					Q_strncatz(out->texture->name, "#LIT", sizeof(out->texture->name));
			}

			if (!strncmp(out->texture->name, "scroll", 6) || ((*out->texture->name == '*' || *out->texture->name == '{' || *out->texture->name == '!') && !strncmp(out->texture->name+1, "scroll", 6)))
				out->flags |= TI_FLOWING;

		}
	}

	return true;
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/

void CalcSurfaceExtents (model_t *mod, msurface_t *s);
/*
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{	
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i];
		s->extents[i] = (bmaxs[i] - bmins[i]);

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 )	//q2 uses 512.
//			Sys_Error ("Bad surface extents");
	}
}
*/

/*
=================
Mod_LoadFaces
=================
*/
static qboolean Mod_LoadFaces (model_t *loadmodel, bspx_header_t *bspx, qbyte *mod_base, lump_t *l, lump_t *lightlump, subbsp_t subbsp)
{
	dsface_t		*ins;
	dlface_t		*inl;
	msurface_t 	*out;
	int			count, surfnum;
	int			i, planenum, side;
	int tn;
	unsigned int lofs, lend;

	unsigned short lmshift, lmscale;
	char buf[64];
	lightmapoverrides_t overrides;

	int lofsscale = 1;
	qboolean lightmapusable = false;

	struct decoupled_lm_info_s *decoupledlm;
	size_t dcsize;

	memset(&overrides, 0, sizeof(overrides));

	lmscale = atoi(Mod_ParseWorldspawnKey(loadmodel, "lightmap_scale", buf, sizeof(buf)));
	if (!lmscale)
		lmshift = LMSHIFT_DEFAULT;
	else
	{
		for(lmshift = 0; lmscale > 1; lmshift++)
			lmscale >>= 1;
	}

	if (subbsp == sb_long1 || subbsp == sb_long2)
	{
		ins = NULL;
		inl = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*inl);
		if (l->filelen % sizeof(*inl) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		inl = NULL;
		count = l->filelen / sizeof(*ins);
		if (l->filelen % sizeof(*ins) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

//	*meshlist = ZG_Malloc(&loadmodel->memgroup, count*sizeof(**meshlist));
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	//dodgy guesses time...
	if (loadmodel->fromgame == fg_quake //some halflife maps are misidentified as quake...
		&& loadmodel->submodels[0].headnode[3] /*these do have crouch hulls. this'll save a LOT of modulo expense*/
		&& subbsp == sb_none/*don't bother with bsp2... maybe halflife will get a remaster that uses/supports it?*/
		&& ins && count /*yeah... just in case*/
		&& !overrides.shifts /*would break expectations. fix your maps.*/
		&& lightlump->filelen%3==0	/*hlbsp has rgb lighting so MUST be a multiple of 3*/
		)
	{
		for (surfnum=0; surfnum<count; surfnum++)
		{
			lofs = LittleLong(ins[surfnum].lightofs);
			if (lofs%3)
				break;	//not a byte offset within rgb data
			if (lofs != (unsigned int)-1 && ins[surfnum].styles[0]!=255)
			{
				//count styles
				for (i = 0; i < countof(ins[surfnum].styles); i++)
					if (ins[surfnum].styles[i] == 255)
						break;
				if (!i)
					continue;	//no lightmap data here...

				tn = LittleShort (ins->texinfo);
				if (tn < 0 || tn >= loadmodel->numtexinfo)
					break;
				out->texinfo = loadmodel->texinfo + tn;
				out->firstedge = LittleLong(ins->firstedge);
				out->numedges = LittleShort(ins->numedges);
				out->lmshift = lmshift;
				CalcSurfaceExtents (loadmodel, out);
				i *= (out->extents[0]>>out->lmshift)+1;	//width
				i *= (out->extents[1]>>out->lmshift)+1;	//height
				i *= 3; //for rgb
				//'i' is now the size of our lightmap data, in bytes. phew.
				lend = lofs + i;

				//we now have a reference surface.
				for (surfnum++; surfnum<count; surfnum++)
				{
					unsigned int checklofs = LittleLong(ins[surfnum].lightofs);
					if (checklofs%3)
						break;	//can't be hl
					if (checklofs > lofs && checklofs < lend)
						break;	//started before reference surf ended... reference surface can't have been using RGB lighting. so not a mislabled hlbsp.
				}
				break;
			}
		}
		if (surfnum==count)
			loadmodel->fromgame = fg_halflife;
	}

	Mod_LoadVertexNormals(loadmodel, bspx, mod_base, NULL);
	Mod_LoadLighting (loadmodel, bspx, mod_base, lightlump, false, &overrides, subbsp);

	decoupledlm = BSPX_FindLump(bspx, mod_base, "DECOUPLED_LM", &dcsize); //RGB packed data
	if (dcsize == count*sizeof(*decoupledlm))
		loadmodel->facelmvecs = ZG_Malloc(&loadmodel->memgroup, count * sizeof(*loadmodel->facelmvecs));	//seems good.
	else
		decoupledlm	= NULL;	//wrong size somehow... discard it.

	switch(loadmodel->lightmaps.fmt)
	{
	case LM_E5BGR9:
		lofsscale = 4;
		break;
	case LM_RGB8:
		lofsscale = 3;
		break;
	default:
	case LM_L8:
		lofsscale = 1;
		break;
	}
	if (loadmodel->fromgame == fg_halflife)
		lofsscale /= 3;	//halflife has rgb offsets already (this should drop to 1, preserving any misaligned offsets...

	for ( surfnum=0 ; surfnum<count ; surfnum++, out++)
	{
		if (subbsp == sb_long1 || subbsp == sb_long2)
		{
			planenum = LittleLong(inl->planenum);
			side = LittleLong(inl->side);
			out->firstedge = LittleLong(inl->firstedge);
			out->numedges = LittleLong(inl->numedges);
			tn = LittleLong (inl->texinfo);
			for (i=0 ; i<countof(out->styles) ; i++)
				out->styles[i] = (i >= countof(inl->styles) || (lightstyleindex_t)inl->styles[i]>=INVALID_LIGHTSTYLE|| inl->styles[i]==255)?INVALID_LIGHTSTYLE:inl->styles[i];
			lofs = LittleLong(inl->lightofs);
			inl++;
		}
		else
		{
			planenum = LittleShort(ins->planenum);
			side = LittleShort(ins->side);
			out->firstedge = LittleLong(ins->firstedge);
			out->numedges = LittleShort(ins->numedges);
			tn = LittleShort (ins->texinfo);
			for (i=0 ; i<countof(out->styles) ; i++)
				out->styles[i] = (i >= countof(ins->styles) || (lightstyleindex_t)ins->styles[i]>=INVALID_LIGHTSTYLE || ins->styles[i]==255)?INVALID_LIGHTSTYLE:ins->styles[i];
			lofs = LittleLong(ins->lightofs);
			if (subbsp == sb_quake64)
			{
				lofs >>= 1;
			}
			ins++;
		}
//		(*meshlist)[surfnum].vbofirstvert = out->firstedge;
//		(*meshlist)[surfnum].numvertexes = out->numedges;
		out->flags = 0;

		if (side)
			out->flags |= SURF_PLANEBACK;			

		out->plane = loadmodel->planes + planenum;

		if (tn < 0 || tn >= loadmodel->numtexinfo)
		{
			Con_Printf("texinfo 0 <= %i < %i\n", tn, loadmodel->numtexinfo);
			return false;
		}
		out->texinfo = loadmodel->texinfo + tn;

		if (overrides.shifts)
			out->lmshift = overrides.shifts[surfnum];
		else
			out->lmshift = lmshift;
		if (overrides.offsets)
			lofs = overrides.offsets[surfnum];
		if (overrides.styles16)
		{
			for (i=0 ; i<countof(out->styles) ; i++)
				out->styles[i] = (i>=overrides.stylesperface)?INVALID_LIGHTSTYLE:overrides.styles16[surfnum*overrides.stylesperface+i];
		}
		else if (overrides.styles8)
		{
			for (i=0 ; i<countof(out->styles) ; i++)
				out->styles[i] = (i>=overrides.stylesperface)?INVALID_LIGHTSTYLE:((overrides.styles8[surfnum*overrides.stylesperface+i]==255)?INVALID_LIGHTSTYLE:overrides.styles8[surfnum*overrides.stylesperface+i]);
		}
		for (i=0 ; i<countof(out->styles) && out->styles[i] != INVALID_LIGHTSTYLE; i++)
			if (loadmodel->lightmaps.maxstyle < out->styles[i])
				loadmodel->lightmaps.maxstyle = out->styles[i];

		if (decoupledlm)
		{
			lofs = LittleLong(decoupledlm->lmoffset);
			out->texturemins[0] = out->texturemins[1] = 0; // should be handled by the now-per-surface vecs[][3] value.
			out->lmshift = 0;	//redundant.
			if (!decoupledlm->lmsize[0] || !decoupledlm->lmsize[1])
			{
				decoupledlm->lmsize[0] = decoupledlm->lmsize[1] = 0;
				if (lofs != (unsigned int)-1)
				{	//we'll silently allow these buggy surfaces for now... but only if they've got no lightmap data at all. unsafe if they're the last otherwise.
					lofs = -1;
					Con_Printf(CON_WARNING"%s: Face %i has invalid extents\n", loadmodel->name, surfnum);
				}
			}
			else
			{
				out->extents[0] = (unsigned short)LittleShort(decoupledlm->lmsize[0]) - 1;	//surfaces should NEVER have an extent of 0. even if the surface is omitted it should still have some padding...
				out->extents[1] = (unsigned short)LittleShort(decoupledlm->lmsize[1]) - 1;
			}
			loadmodel->facelmvecs[surfnum].lmvecs[0][0] = LittleFloat(decoupledlm->lmvecs[0][0]);
			loadmodel->facelmvecs[surfnum].lmvecs[0][1] = LittleFloat(decoupledlm->lmvecs[0][1]);
			loadmodel->facelmvecs[surfnum].lmvecs[0][2] = LittleFloat(decoupledlm->lmvecs[0][2]);
			loadmodel->facelmvecs[surfnum].lmvecs[0][3] = LittleFloat(decoupledlm->lmvecs[0][3]) + 0.5f; //sigh
			loadmodel->facelmvecs[surfnum].lmvecs[1][0] = LittleFloat(decoupledlm->lmvecs[1][0]);
			loadmodel->facelmvecs[surfnum].lmvecs[1][1] = LittleFloat(decoupledlm->lmvecs[1][1]);
			loadmodel->facelmvecs[surfnum].lmvecs[1][2] = LittleFloat(decoupledlm->lmvecs[1][2]);
			loadmodel->facelmvecs[surfnum].lmvecs[1][3] = LittleFloat(decoupledlm->lmvecs[1][3]) + 0.5f; //sigh
			loadmodel->facelmvecs[surfnum].lmvecscale[0] = 1.0f/Length(loadmodel->facelmvecs[surfnum].lmvecs[0]);	//luxels->qu
			loadmodel->facelmvecs[surfnum].lmvecscale[1] = 1.0f/Length(loadmodel->facelmvecs[surfnum].lmvecs[1]);
			decoupledlm++;
		}
		else
			CalcSurfaceExtents (loadmodel, out);
		if (lofs != (unsigned int)-1)
			lofs *= lofsscale;
		lend = lofs+(out->extents[0]+1)*(out->extents[1]+1) /*FIXME: mul by numstyles */;
		if (lofs > loadmodel->lightdatasize || lend < lofs)
			out->samples = NULL;	//should includes -1
		else
		{
			out->samples = loadmodel->lightdata + lofs;
			lightmapusable = true;	//something has a valid offset.
		}

		if (!out->texinfo->texture)
			continue;

		if (out->numedges < 3)
			Con_Printf(CON_WARNING"%s: Face %i has only %i edge(s) - \"%s\".\n", loadmodel->name, surfnum, out->numedges, out->texinfo->texture->name);

		
	// set the drawing flags flag		
		if (!Q_strncmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}
		if (*out->texinfo->texture->name == '*' || (*out->texinfo->texture->name == '!' && loadmodel->fromgame == fg_halflife))		// turbulent
		{
			out->flags |= SURF_DRAWTURB;
			if (out->texinfo->flags & TEX_SPECIAL)
			{
				out->flags |= SURF_DRAWTILED;
				for (i=0 ; i<2 ; i++)
				{
					out->extents[i] = 16384;
					out->texturemins[i] = -8192;
				}
			}
			continue;
		}

		/*if (*out->texinfo->texture->name == '~')
		{
			out->texinfo->flags |= SURF_BLENDED;
			continue;
		}*/
		if (!Q_strncmp(out->texinfo->texture->name,"{",1))		// alpha
		{
			out->flags |= (SURF_DRAWALPHA);
			continue;
		}

		if (out->flags & SURF_DRAWALPHA)
			out->flags &= ~SURF_DRAWALPHA;
	}

	if (!lightmapusable)
	{
		Con_Printf("no valid lightmap offsets in map\n");
#ifdef RUNTIMELIGHTING
		RelightTerminate(loadmodel);	//not gonna work...
#endif
		loadmodel->lightdata = NULL;
		loadmodel->deluxdata = NULL;
	}
	return true;
}

/*
=================
Mod_LoadNodes
=================
*/
static qboolean Mod_LoadNodes (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t subbsp)
{
	int			i, j, count, p;
	mnode_t 	*out;

	if (subbsp == sb_long2)
	{
		dl2node_t		*in;
		in = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*in);
		if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleFloat (in->mins[j]);
				out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
			}
		
			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = LittleLong (in->firstface);
			out->numsurfaces = LittleLong (in->numfaces);
			
			for (j=0 ; j<2 ; j++)
			{
				p = LittleLong (in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	else if (subbsp == sb_long1)
	{
		dl1node_t		*in;
		in = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*in);
		if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}
		
			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = LittleLong (in->firstface);
			out->numsurfaces = LittleLong (in->numfaces);

			for (j=0 ; j<2 ; j++)
			{
				p = LittleLong (in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	else
	{
		dsnode_t		*in;
		in = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*in);
		if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}
		
			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = (unsigned short)LittleShort (in->firstface);
			out->numsurfaces = (unsigned short)LittleShort (in->numfaces);

			for (j=0 ; j<2 ; j++)
			{
				p = (unsigned short)LittleShort (in->children[j]);

				if (p >= 0 && p < loadmodel->numnodes)
					out->children[j] = loadmodel->nodes + p;
				else
				{
					p = (-1 - (signed)(0xffff0000|p));
					if (p >= 0 && p < loadmodel->numleafs)
						out->children[j] = (mnode_t *)(loadmodel->leafs + p);
					else
					{
						Con_Printf (CON_ERROR "MOD_LoadBmodel: invalid node child %i in %s\n", LittleShort (in->children[j]), loadmodel->name);
						return false;
					}
				}
			}
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
	return true;
}

/*
=================
Mod_LoadLeafs
=================
*/
static qboolean Mod_LoadLeafs (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t subbsp, qboolean isnotmap, qbyte *ptr, size_t len)
{
	mleaf_t 	*out;
	int			i, j, count, p;

	if (!ptr)
	{
		ptr = mod_base + l->fileofs;
		len = l->filelen;
	}

	if (subbsp == sb_long2)
	{
		dl2leaf_t 	*in;
		in = (void *)ptr;
		count = len / sizeof(*in);
		if (len % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->leafs = out;
		loadmodel->numleafs = count;
		loadmodel->numclusters = count-1;
		loadmodel->pvsbytes = ((loadmodel->numclusters+31)>>3)&~3;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleFloat (in->mins[j]);
				out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces +
				LittleLong(in->firstmarksurface);
			out->nummarksurfaces = LittleLong(in->nummarksurfaces);
			
			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			
			for (j=0 ; j<4 ; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

	#ifndef CLIENTONLY
			if (!isDedicated)
	#endif
			{
				// gl underwater warp
				if (out->contents != Q1CONTENTS_EMPTY)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				}
				if (isnotmap)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_DONTWARP;
				}
			}
		}
	}
	else if (subbsp == sb_long1)
	{
		dl1leaf_t 	*in;
		in = (void *)(ptr);
		count = len / sizeof(*in);
		if (len % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->leafs = out;
		loadmodel->numleafs = count;
		loadmodel->numclusters = count-1;
		loadmodel->pvsbytes = ((loadmodel->numclusters+31)>>3)&~3;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces +
				LittleLong(in->firstmarksurface);
			out->nummarksurfaces = LittleLong(in->nummarksurfaces);
			
			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			
			for (j=0 ; j<4 ; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

	#ifndef CLIENTONLY
			if (!isDedicated)
	#endif
			{
				// gl underwater warp
				if (out->contents != Q1CONTENTS_EMPTY)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				}
				if (isnotmap)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_DONTWARP;
				}
			}
		}
	}
	else
	{
		dsleaf_t 	*in;
		in = (void *)(ptr);
		count = len / sizeof(*in);
		if (len % sizeof(*in) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->leafs = out;
		loadmodel->numleafs = count;
		loadmodel->numclusters = count-1;
		loadmodel->pvsbytes = ((loadmodel->numclusters+31)>>3)&~3;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces + (unsigned short)LittleShort(in->firstmarksurface);
			out->nummarksurfaces = (unsigned short)LittleShort(in->nummarksurfaces);
			
			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			
			for (j=0 ; j<4 ; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

	#ifndef CLIENTONLY
			if (!isDedicated)
	#endif
			{
				// gl underwater warp
				if (out->contents != Q1CONTENTS_EMPTY)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				}
				if (isnotmap)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_DONTWARP;
				}
			}
		}
	}

	return true;
}




/*
=================
Mod_LoadClipnodes
=================
*/
static qboolean Mod_LoadClipnodes (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t subbsp, qboolean hexen2map)
{
	dsclipnode_t *ins;
	dlclipnode_t *inl;
	mclipnode_t *out;
	int			i, count;
	hull_t		*hull;

	if (subbsp == sb_long1 || subbsp == sb_long2)
	{
		ins = NULL;
		inl = (void *)(mod_base + l->fileofs);
		count = l->filelen / sizeof(*inl);
		if (l->filelen % sizeof(*inl) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		inl = NULL;
		count = l->filelen / sizeof(*ins);
		if (l->filelen % sizeof(*ins) || count > SANITY_LIMIT(*out))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		if (count > (1u<<16))
		{
			Con_Printf (CON_ERROR "%s: clipnode count exceeds 16bit limit (%u). Try bsp2.\n", loadmodel->name, count);
			return false;
		}
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));//space for both

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;


	if (hexen2map)
	{	//hexen2.
		//compatible with Q1.
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 32;
		hull->available = true;

		//NOT compatible with Q1
		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -24;
		hull->clip_mins[1] = -24;
		hull->clip_mins[2] = -20;
		hull->clip_maxs[0] = 24;
		hull->clip_maxs[1] = 24;
		hull->clip_maxs[2] = 20;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -12;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 16;
		hull->available = true;

		/*
		There is some mission-pack weirdness here
		in the missionpack, hull 4 is meant to be '-8 -8 -8' '8 8 8'
		in the original game, hull 4 is '-40 -40 -42' '40 40 42'
		*/
		hull = &loadmodel->hulls[4];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -8;
		hull->clip_mins[1] = -8;
		hull->clip_mins[2] = -8;
		hull->clip_maxs[0] = 8;
		hull->clip_maxs[1] = 8;
		hull->clip_maxs[2] = 8;
		hull->available = true;

		hull = &loadmodel->hulls[5];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -48;
		hull->clip_mins[1] = -48;
		hull->clip_mins[2] = -50;
		hull->clip_maxs[0] = 48;
		hull->clip_maxs[1] = 48;
		hull->clip_maxs[2] = 50;
		hull->available = true;

		//6 isn't used.
		//7 isn't used.
	}
	else if (loadmodel->fromgame == fg_halflife)
	{
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -36;//-36 is correct here, but mvdsv uses -32 instead. This breaks prediction between the two
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = hull->clip_mins[2]+72;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -32;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = hull->clip_mins[2]+64;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -18;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = hull->clip_mins[2]+36;
		hull->available = true;
	}
	else
	{
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 32;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = 64;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -6;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 30;
		hull->available = false;
	}

	if (subbsp == sb_long1 || subbsp == sb_long2)
	{
		for (i=0 ; i<count ; i++, out++, inl++)
		{
			out->planenum = LittleLong(inl->planenum);
			out->children[0] = LittleLong(inl->children[0]);
			out->children[1] = LittleLong(inl->children[1]);
		}
	}
	else
	{
		for (i=0 ; i<count ; i++, out++, ins++)
		{
			out->planenum = LittleLong(ins->planenum);
			out->children[0] = (unsigned short)LittleShort(ins->children[0]);
			out->children[1] = (unsigned short)LittleShort(ins->children[1]);

			//if these 'overflow', then they're meant to refer to contents instead, and should be negative
			if (out->children[0] >= count)
				out->children[0] -= 0x10000;
			if (out->children[1] >= count)
				out->children[1] -= 0x10000;
		}
	}

	return true;
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
static void Mod_MakeHull0 (model_t *loadmodel)
{
	mnode_t		*in, *child;
	mclipnode_t *out;
	int			i, j, count;
	hull_t		*hull;

	hull = &loadmodel->hulls[0];	

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadPlanes
=================
*/
static qboolean Mod_LoadPlanes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*2*sizeof(*out));
	
	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
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

	return true;
}

static void Mod_FixupNodeMinsMaxs (mnode_t *node, mnode_t *parent)
{
	if (!node)
		return;

	if (node->contents >= 0)
	{
		Mod_FixupNodeMinsMaxs (node->children[0], node);
		Mod_FixupNodeMinsMaxs (node->children[1], node);
	}

	if (parent)
	{
		if (parent->minmaxs[0] > node->minmaxs[0])
			parent->minmaxs[0] = node->minmaxs[0];
		if (parent->minmaxs[1] > node->minmaxs[1])
			parent->minmaxs[1] = node->minmaxs[1];
		if (parent->minmaxs[2] > node->minmaxs[2])
			parent->minmaxs[2] = node->minmaxs[2];

		if (parent->minmaxs[3] < node->minmaxs[3])
			parent->minmaxs[3] = node->minmaxs[3];
		if (parent->minmaxs[4] < node->minmaxs[4])
			parent->minmaxs[4] = node->minmaxs[4];
		if (parent->minmaxs[5] < node->minmaxs[5])
			parent->minmaxs[5] = node->minmaxs[5];
	}

}

static void Mod_FixupMinsMaxs(model_t *loadmodel)
{
	//q1 bsps are capped to +/- 32767 by the nodes/leafs
	//verts arn't though
	//so if the map is too big, let's figure out what they should be
	float *v;
	msurface_t **mark, *surf;
	mleaf_t *pleaf;
	medge_t *e, *pedges;
	int en, lindex;
	int i, c, lnumverts;
	qboolean needsfixup = false;

	if (loadmodel->mins[0] < -32768)
		needsfixup = true;
	if (loadmodel->mins[1] < -32768)
		needsfixup = true;
	if (loadmodel->mins[2] < -32768)
		needsfixup = true;

	if (loadmodel->maxs[0] > 32767)
		needsfixup = true;
	if (loadmodel->maxs[1] > 32767)
		needsfixup = true;
	if (loadmodel->maxs[2] > 32767)
		needsfixup = true;

	if (!needsfixup)
		return;

	//this is insane.
	//why am I writing this?
	//by the time the world actually gets this large, the floating point errors are going to be so immensly crazy that it's just not worth it.

	pedges = loadmodel->edges;

	for (i = 0; i < loadmodel->numleafs; i++)
	{
		pleaf = &loadmodel->leafs[i];

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				surf = (*mark++);

				lnumverts = surf->numedges;
				for (en=0 ; en<lnumverts ; en++)
				{
					lindex = loadmodel->surfedges[surf->firstedge + en];

					if (lindex > 0)
					{
						e = &pedges[lindex];
						v = loadmodel->vertexes[e->v[0]].position;
					}
					else
					{
						e = &pedges[-lindex];
						v = loadmodel->vertexes[e->v[1]].position;
					}

					if (pleaf->minmaxs[0] > v[0])
						pleaf->minmaxs[0] = v[0];
					if (pleaf->minmaxs[1] > v[1])
						pleaf->minmaxs[1] = v[1];
					if (pleaf->minmaxs[2] > v[2])
						pleaf->minmaxs[2] = v[2];

					if (pleaf->minmaxs[3] < v[0])
						pleaf->minmaxs[3] = v[0];
					if (pleaf->minmaxs[4] < v[1])
						pleaf->minmaxs[4] = v[1];
					if (pleaf->minmaxs[5] < v[2])
						pleaf->minmaxs[5] = v[2];

				}
			} while (--c);
		}
	}
	Mod_FixupNodeMinsMaxs (loadmodel->nodes, NULL);	// sets nodes and leafs
}

#endif

void ModBrush_LoadGLStuff(void *ctx, void *data, size_t a, size_t b)
{
#ifndef SERVERONLY
	model_t *mod = ctx;
	char loadname[MAX_QPATH];

	if (!a)
	{	//submodels share textures, so only do this if 'a' is 0 (inline index, 0 = world).
		for (a = 0; a < mod->numfogs; a++)
		{
			mod->fogs[a].shader = R_RegisterShader_Lightmap(mod, mod->fogs[a].shadername);
			R_BuildDefaultTexnums(NULL, mod->fogs[a].shader, IF_WORLDTEX);
			if (!mod->fogs[a].shader->fog_dist)
			{
				//invalid fog shader, don't use.
				mod->fogs[a].shader = NULL;
				mod->fogs[a].numplanes = 0;
			}
		}

#if defined(Q3BSPS) || defined(RFBSPS)
		if (mod->fromgame == fg_quake3)
		{
			if (mod->lightmaps.deluxemapping && mod->lightmaps.deluxemapping_modelspace)
			{
				for(a = 0; a < mod->numtexinfo; a++)
				{
					mod->textures[a]->shader = R_RegisterShader_Lightmap(mod, va("%s#BUMPMODELSPACE", mod->textures[a]->name));
					R_BuildDefaultTexnums(NULL, mod->textures[a]->shader, IF_WORLDTEX);

					mod->textures[a+mod->numtexinfo]->shader = R_RegisterShader_Vertex (mod, va("%s#VERTEXLIT", mod->textures[a+mod->numtexinfo]->name));
					R_BuildDefaultTexnums(NULL, mod->textures[a+mod->numtexinfo]->shader, IF_WORLDTEX);
				}
			}
			else
			{
				for(a = 0; a < mod->numtexinfo; a++)
				{
					mod->textures[a]->shader = R_RegisterShader_Lightmap(mod, mod->textures[a]->name);
					R_BuildDefaultTexnums(NULL, mod->textures[a]->shader, IF_WORLDTEX);

					mod->textures[a+mod->numtexinfo]->shader = R_RegisterShader_Vertex (mod, va("%s#VERTEXLIT", mod->textures[a+mod->numtexinfo]->name));
					R_BuildDefaultTexnums(NULL, mod->textures[a+mod->numtexinfo]->shader, IF_WORLDTEX);
				}
			}
			mod->textures[2*mod->numtexinfo]->shader = R_RegisterShader_Flare(mod, "noshader");
		}
		else
#endif
#ifdef Q2BSPS
		if (mod->fromgame == fg_quake2)
		{
			COM_FileBase (mod->name, loadname, sizeof(loadname));
			for(a = 0; a < mod->numtextures; a++)
			{
				unsigned int maps = 0;
				mod->textures[a]->shader = R_RegisterCustom (mod, mod->textures[a]->name, SUF_LIGHTMAP, Shader_DefaultBSPQ2, NULL);

				maps |= SHADER_HASPALETTED;
				maps |= SHADER_HASDIFFUSE;
				if (r_fb_bmodels.ival)
					maps |= SHADER_HASFULLBRIGHT;
//				if (r_loadbumpmapping || (r_waterstyle.ival > 1 && *tx->name == '*'))
//					maps |= SHADER_HASNORMALMAP;
				if (gl_specular.ival)
					maps |= SHADER_HASGLOSS;
				R_BuildLegacyTexnums(mod->textures[a]->shader, mod->textures[a]->name, loadname, maps, IF_WORLDTEX, mod->textures[a]->srcfmt, mod->textures[a]->srcwidth, mod->textures[a]->srcheight, mod->textures[a]->srcdata, mod->textures[a]->palette);
				BZ_Free(mod->textures[a]->srcdata);
				mod->textures[a]->srcdata = NULL;
			}
		}
		else
#endif
		{
			COM_FileBase (mod->name, loadname, sizeof(loadname));
			if (!strncmp(loadname, "b_", 2))
				Q_strncpyz(loadname, "bmodels", sizeof(loadname));
			for(a = 0; a < mod->numtextures; a++)
				Mod_FinishTexture(mod, mod->textures[a], loadname, false);
		}
	}
	Mod_Batches_Build(mod, data);
	if (data)
		BZ_Free(data);
#endif
}

#ifdef Q1BSPS

struct vispatch_s
{
	void *fileptr;
	size_t filelen;

	void *visptr;
	int vislen;

	void *leafptr;
	int leaflen;
};

static void Mod_FindVisPatch(struct vispatch_s *patch, model_t *mod, size_t leaflumpsize)
{
	char patchname[MAX_QPATH];
	int *lenptr, len;
	int ofs;
	qbyte *file;
	char *mapname;
	memset(patch, 0, sizeof(*patch));

	if (!mod_external_vis.ival)
		return;

	mapname = COM_SkipPath(mod->name);

	COM_StripExtension(mod->name, patchname, sizeof(patchname));
	Q_strncatz(patchname, ".vis", sizeof(patchname));

	//ignore the patch file if its in a different gamedir.
	//this file format sucks too much for other verification.
	if (FS_FLocateFile(mod->name,FSLF_DEEPONFAILURE, NULL) != FS_FLocateFile(patchname,FSLF_DEEPONFAILURE, NULL))
		return;

	patch->filelen = FS_LoadFile(patchname, &patch->fileptr);
	if (!patch->fileptr)
		return;

	ofs = 0;
	while (ofs+36 <= patch->filelen)
	{
		file = patch->fileptr;
		file += ofs;
		memcpy(patchname, file, 32);
		patchname[32] = 0;
		file += 32;
		lenptr = (int*)file;
		file += sizeof(int);
		len = LittleLong(*lenptr);
		if (ofs+36+len > patch->filelen)
			break;

		if (!Q_strcasecmp(patchname, mapname))
		{
			lenptr = (int*)file;
			patch->vislen = LittleLong(*lenptr);
			file += sizeof(int);
			patch->visptr = file;
			file += patch->vislen;

			lenptr = (int*)file;
			patch->leaflen = LittleLong(*lenptr);
			file += sizeof(int);
			patch->leafptr = file;
			file += patch->leaflen;

			if (sizeof(int)*2 + patch->vislen + patch->leaflen != len || patch->leaflen != leaflumpsize)
			{
				patch->visptr = NULL;
				patch->leafptr = NULL;
			}
			else
				break;
		}
		ofs += 36+len;
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
static qboolean QDECL Mod_LoadBrushModel (model_t *mod, void *buffer, size_t fsize)
{
	struct vispatch_s vispatch;
	int			i, j;
	dheader_t	header;
	mmodel_t 	*bm;
	model_t *submod;
	unsigned int chksum;
	qboolean noerrors;
	char loadname[32];
	qbyte *mod_base = buffer;
	qboolean hexen2map = false;
	qboolean isnotmap;
	qboolean using_rbe = true;
	qboolean misaligned = false;
	bspx_header_t *bspx;
	subbsp_t subbsp = sb_none;

	COM_FileBase (mod->name, loadname, sizeof(loadname));
	mod->type = mod_brush;
	
	if (fsize < sizeof(header))
		return false;

	mod_base = (qbyte *)buffer;
	memcpy(&header, mod_base, sizeof(header));
	for (i=0 ; i<countof(header.lumps)/4 ; i++)
	{
		header.lumps[i].filelen = LittleLong(header.lumps[i].filelen);
		header.lumps[i].fileofs = LittleLong(header.lumps[i].fileofs);
	}

#ifdef SERVERONLY
	isnotmap = !!sv.world.worldmodel;
#else
	if ((!cl.worldmodel && cls.state>=ca_connected)
#ifndef CLIENTONLY
		|| (!sv.world.worldmodel && sv.state)
#endif
		)
		isnotmap = false;
	else
		isnotmap = true;
#endif

	mod->fromgame = fg_quake;
	if (!memcmp(&header.version,  BSPVERSION))
		mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	else if (!memcmp(&header.version,  BSPVERSIONQ64))
		mod->engineflags |= MDLF_NEEDOVERBRIGHT, subbsp = sb_quake64;
	else if (!memcmp(&header.version,  BSPVERSIONPREREL))
		mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	else if (!memcmp(&header.version,  BSPVERSION_LONG1))
		mod->engineflags |= MDLF_NEEDOVERBRIGHT, subbsp = sb_long1;
	else if (!memcmp(&header.version,  BSPVERSION_LONG2))
		mod->engineflags |= MDLF_NEEDOVERBRIGHT, subbsp = sb_long2;
	else if (!memcmp(&header.version,  BSPVERSIONHL))
	{
		char tmp[64];
		mod->fromgame = fg_halflife;

		//special hack to work around blueshit bugs - we need to swap LUMP_ENTITIES and LUMP_PLANES over
		if (COM_ParseOut(mod_base + header.lumps[LUMP_PLANES].fileofs, tmp, sizeof(tmp)) && !strcmp(tmp, "{"))
		{
			COM_ParseOut(mod_base + header.lumps[LUMP_ENTITIES].fileofs, tmp, sizeof(tmp));
			if (strcmp(tmp, "{"))
			{
				int i;
				for (i = 0; i < header.lumps[LUMP_ENTITIES].filelen && i < sizeof(dplane_t); i++)
					if (mod_base[header.lumps[LUMP_ENTITIES].fileofs + i] == 0)
					{	//yeah, looks screwy in the way we expect. swap em over.
						lump_t tmp = header.lumps[LUMP_ENTITIES];
						header.lumps[LUMP_ENTITIES] = header.lumps[LUMP_PLANES];
						header.lumps[LUMP_PLANES] = tmp;
						break;
					}
			}
		}
	}
	else
	{
		Con_Printf (CON_ERROR "Mod_LoadBrushModel: %s has wrong version number (%i)\n", mod->name, i);
		return false;
	}
	header.version = LittleLong(header.version);

	mod->lightmaps.width = 128;//LMBLOCK_WIDTH;
	mod->lightmaps.height = 128;//LMBLOCK_HEIGHT; 

// checksum all of the map, except for entities
	mod->checksum = 0;
	mod->checksum2 = 0;

	for (i = 0; i < HEADER_LUMPS; i++)
	{
		if ((header.lumps[i].fileofs & 3) && header.lumps[i].filelen)
			misaligned = true;

		if ((unsigned)header.lumps[i].fileofs + (unsigned)header.lumps[i].filelen > fsize)
		{
			Con_Printf (CON_ERROR "Mod_LoadBrushModel: %s appears truncated\n", mod->name);
			return false;
		}
		if (i == LUMP_ENTITIES)
			continue;
		chksum = CalcHashInt(&hash_md4, mod_base + header.lumps[i].fileofs, header.lumps[i].filelen);
		mod->checksum ^= chksum;

		if (i == LUMP_VISIBILITY || i == LUMP_LEAFS || i == LUMP_NODES)
			continue;
		mod->checksum2 ^= chksum;
	}

	if (misaligned)
	{	//pre-phong versions of tyrutils wrote misaligned lumps. These crash on arm/etc.
		char *tmp;
		unsigned int ofs = 0;
		Con_DPrintf(CON_WARNING"%s: Misaligned lumps detected\n", mod->name);
		tmp = BZ_Malloc(fsize);
		memcpy(tmp, mod_base, fsize);
		for (i = 0; i < HEADER_LUMPS; i++)
		{
			if (ofs + header.lumps[i].filelen > fsize)
			{	//can happen if two lumps overlap... otherwise impossible.
				Con_Printf(CON_ERROR"%s: Realignment failed\n", mod->name);
				BZ_Free(tmp);
				return false;
			}
			memcpy(mod_base + ofs, tmp+header.lumps[i].fileofs, header.lumps[i].filelen);
			header.lumps[i].fileofs = ofs;
			ofs += header.lumps[i].filelen;
			ofs = (ofs + 3) & ~3u;
		}
		BZ_Free(tmp);
		bspx = NULL;
	}
	else
	{
		bspx = BSPX_Setup(mod, mod_base, fsize, header.lumps, HEADER_LUMPS);

		/*if (1)//mod_ebfs.value)
		{
			char *id;
			id = (char *)mod_base + sizeof(dheader_t);
			if (id[0]=='P' && id[1]=='A' && id[2]=='C' && id[3]=='K')
			{	//EBFS detected.
				COM_LoadMapPackFile(mod->name, sizeof(dheader_t));
			}
		}*/
	}
		
	noerrors = true;

	Mod_FindVisPatch(&vispatch, mod, header.lumps[LUMP_LEAFS].filelen);

// load into heap
	if (!isDedicated || using_rbe)
	{
		TRACE(("Loading verts\n"));
		noerrors = noerrors && Mod_LoadVertexes (mod, mod_base, &header.lumps[LUMP_VERTEXES]);
		TRACE(("Loading edges\n"));
		noerrors = noerrors && Mod_LoadEdges (mod, mod_base, &header.lumps[LUMP_EDGES], subbsp);
		TRACE(("Loading Surfedges\n"));
		noerrors = noerrors && Mod_LoadSurfedges (mod, mod_base, &header.lumps[LUMP_SURFEDGES]);
	}
	if (!isDedicated)
	{
		TRACE(("Loading Textures\n"));
		noerrors = noerrors && Mod_LoadTextures (mod, mod_base, &header.lumps[LUMP_TEXTURES], subbsp);
	}
	TRACE(("Loading Submodels\n"));
	noerrors = noerrors && Mod_LoadSubmodels (mod, mod_base, &header.lumps[LUMP_MODELS], &hexen2map);
	TRACE(("Loading Planes\n"));
	noerrors = noerrors && Mod_LoadPlanes (mod, mod_base, &header.lumps[LUMP_PLANES]);
	TRACE(("Loading Entities\n"));
	Mod_LoadEntities (mod, mod_base, &header.lumps[LUMP_ENTITIES]);
	if (!isDedicated || using_rbe)
	{
		TRACE(("Loading Texinfo\n"));
		noerrors = noerrors && Mod_LoadTexinfo (mod, mod_base, &header.lumps[LUMP_TEXINFO]);
		TRACE(("Loading Faces\n"));
		noerrors = noerrors && Mod_LoadFaces (mod, bspx, mod_base, &header.lumps[LUMP_FACES], &header.lumps[LUMP_LIGHTING], subbsp);
	}
	if (!isDedicated)
	{
		TRACE(("Loading MarkSurfaces\n"));
		noerrors = noerrors && Mod_LoadMarksurfaces (mod, mod_base, &header.lumps[LUMP_MARKSURFACES], subbsp);
	}
	if (noerrors)
	{
		TRACE(("Loading Vis\n"));
		Mod_LoadVisibility (mod, mod_base, &header.lumps[LUMP_VISIBILITY], vispatch.visptr, vispatch.vislen);
	}
	noerrors = noerrors && Mod_LoadLeafs (mod, mod_base, &header.lumps[LUMP_LEAFS], subbsp, isnotmap, vispatch.leafptr, vispatch.leaflen);
	TRACE(("Loading Nodes\n"));
	noerrors = noerrors && Mod_LoadNodes (mod, mod_base, &header.lumps[LUMP_NODES], subbsp);
	TRACE(("Loading Clipnodes\n"));
	noerrors = noerrors && Mod_LoadClipnodes (mod, mod_base, &header.lumps[LUMP_CLIPNODES], subbsp, hexen2map);
	if (noerrors)
	{
		TRACE(("Loading hull 0\n"));
		Mod_MakeHull0 (mod);
	}

	TRACE(("sorting shaders\n"));
	if (!isDedicated && noerrors)
		Mod_SortShaders(mod);

	BZ_Free(vispatch.fileptr);

	if (!noerrors)
	{
		return false;
	}

	TRACE(("LoadBrushModel %i\n", __LINE__));
	Q1BSP_LoadBrushes(mod, bspx, mod_base);
	TRACE(("LoadBrushModel %i\n", __LINE__));

	mod->numframes = 2;		// regular and alternate animation
	

//
// set up the submodels (FIXME: this is confusing)
//

	for (j=0 ; j<2 ; j++)
		Q1BSP_CheckHullNodes(&mod->hulls[j]);

	for (i=0, submod = mod; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		submod->rootnode = submod->nodes + bm->headnode[0];
		submod->hulls[0].firstclipnode = bm->headnode[0];
		submod->hulls[0].available = true;
//		Q1BSP_CheckHullNodes(&submod->hulls[0]);

TRACE(("LoadBrushModel %i\n", __LINE__));
		for (j=1 ; j<MAX_MAP_HULLSM ; j++)
		{
			submod->hulls[j].firstclipnode = bm->headnode[j];
			submod->hulls[j].lastclipnode = submod->numclipnodes-1;

			submod->hulls[j].available &= bm->hullavailable[j];
			if (submod->hulls[j].firstclipnode > submod->hulls[j].lastclipnode)
				submod->hulls[j].available = false;

//			if (submod->hulls[j].available)
//				Q1BSP_CheckHullNodes(&submod->hulls[j]);
		}

		if (mod->fromgame == fg_halflife && i)
		{
			for (j=bm->firstface ; j<bm->firstface+bm->numfaces ; j++)
			{
				if (mod->surfaces[j].flags & SURF_DRAWTURB)
				{
					float mid = bm->mins[2] + (0.5 * (bm->maxs[2] - bm->mins[2]));
					if (mod->surfaces[j].plane->type == PLANE_Z && mod->surfaces[j].plane->dist >= mid) {
						continue;
					}
					mod->surfaces[j].flags |= SURF_NODRAW;
				}
			}
		}
		
		submod->firstmodelsurface = bm->firstface;
		submod->nummodelsurfaces = bm->numfaces;
		
		VectorCopy (bm->maxs, submod->maxs);
		VectorCopy (bm->mins, submod->mins);

		submod->radius = RadiusFromBounds (submod->mins, submod->maxs);

		submod->numclusters = (i==0)?bm->visleafs:0;
		submod->pvsbytes = ((submod->numclusters+31)>>3)&~3;

		if (i)
		{
			submod->entities_raw = NULL;
			submod->archive = NULL;
		}

		memset(&submod->batches, 0, sizeof(submod->batches));
		submod->vbos = NULL;
		TRACE(("LoadBrushModel %i\n", __LINE__));
		if (!isDedicated || using_rbe)
		{
			COM_AddWork(WG_MAIN, ModBrush_LoadGLStuff, submod, NULL, i, 0);
		}
		TRACE(("LoadBrushModel %i\n", __LINE__));

		submod->cnodes = NULL;
		Q1BSP_SetModelFuncs(submod);
#ifdef Q2BSPS
		if (bm->brushes)
		{
			struct bihleaf_s *leafs, *l;
			size_t i;
			leafs = l = BZ_Malloc(sizeof(*leafs)*bm->numbrushes);
			for (i = 0; i < bm->numbrushes; i++)
			{
				struct q2cbrush_s *b = &bm->brushes[i];
				l->type = BIH_BRUSH;
				l->data.brush = b;
				l->data.contents = b->contents;
				VectorCopy(b->absmins, l->mins);
				VectorCopy(b->absmaxs, l->maxs);
				l++;
			}
			BIH_Build(submod, leafs, l-leafs);
			BZ_Free(leafs);
		}
#endif

		if (i)
			COM_AddWork(WG_MAIN, Mod_ModelLoaded, submod, NULL, MLS_LOADED, 0);
		if (i < submod->numsubmodels-1)
		{	// duplicate the basic information
			char	name[MAX_QPATH];
			model_t *nextmod;

			Q_snprintfz (name, sizeof(name), "*%i:%s", i+1, mod->publicname);
			nextmod = Mod_FindName (name);
			*nextmod = *submod;
			nextmod->submodelof = mod;
			Q_strncpyz(nextmod->publicname, name, sizeof(nextmod->publicname));
			Q_snprintfz (nextmod->name, sizeof(nextmod->publicname), "*%i:%s", i+1, mod->publicname);
			submod = nextmod;
			memset(&submod->memgroup, 0, sizeof(submod->memgroup));
		}
		TRACE(("LoadBrushModel %i\n", __LINE__));
	}
TRACE(("LoadBrushModel %i\n", __LINE__));
	if (!isDedicated)
		Mod_FixupMinsMaxs(mod);
TRACE(("LoadBrushModel %i\n", __LINE__));

#ifdef TERRAIN
	mod->terrain = Mod_LoadTerrainInfo(mod, loadname, false);
#endif
	return true;
}
#endif

/*
==============================================================================

SPRITES

==============================================================================
*/

//=========================================================

#ifdef SERVERONLY
//dedicated servers should not need to load sprites.
//dedicated servers need *.bsp to be loaded for setmodel to get the correct size (or all model types with sv_gameplayfix_setmodelrealbox).
//otherwise other model types(actually: names) only need to be loaded once reflection or hitmodel is used.
//for sprites we don't really care ever.
qboolean QDECL Mod_LoadSpriteModel (model_t *mod, void *buffer, size_t fsize)
{
	mod->type = mod_dummy;
	return true;
}
qboolean QDECL Mod_LoadSprite2Model (model_t *mod, void *buffer, size_t fsize)
{
	return Mod_LoadSpriteModel(mod, buffer, fsize);
}
void Mod_LoadDoomSprite (model_t *mod)
{
	mod->type = mod_dummy;
}
#else

//we need to override the rtlight shader for sprites so they get lit properly ignoring n+s+t dirs
//so lets split the shader into parts to avoid too many dupes
#define SPRITE_SHADER_MAIN(extra)			\
			"{\n"											\
				"if gl_blendsprites\n"						\
					"program defaultsprite\n"				\
				"else\n"									\
					"program defaultsprite#MASK=0.666\n"	\
				"endif\n"									\
				"{\n"										\
					"map $diffuse\n"						\
					"if gl_blendsprites == 2\n"				\
						"blendfunc GL_ONE GL_ONE\n"			\
					"elif gl_blendsprites\n"				\
						"blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA\n"	\
					"else\n"								\
						"alphafunc ge128\n"					\
						"depthwrite\n"						\
					"endif\n"								\
					"rgbgen vertex\n"						\
					"alphagen vertex\n"						\
				"}\n"										\
				"surfaceparm noshadows\n"					\
				extra										\
			"}\n"
#define SPRITE_SHADER_UNLIT	SPRITE_SHADER_MAIN(			\
				"surfaceparm nodlight\n")
#define SPRITE_SHADER_LIT	SPRITE_SHADER_MAIN(			\
				"sort seethrough\n"						\
				"bemode rtlight\n"						\
				"{\n"									\
					"program rtlight#NOBUMP\n"			\
					"{\n"								\
						"map $diffuse\n"				\
						"blendfunc add\n"				\
					"}\n"								\
				"}\n")

void Mod_LoadSpriteFrameShader(model_t *spr, int frame, int subframe, mspriteframe_t *frameinfo)
{
#ifndef SERVERONLY
	char *shadertext;
	char name[MAX_QPATH];
	qboolean litsprite = false;

	if (qrenderer == QR_NONE)
		return;

	if (subframe == -1)
		Q_snprintfz(name, sizeof(name), "%s_%i.tga", spr->name, frame);
	else
		Q_snprintfz(name, sizeof(name), "%s_%i_%i.tga", spr->name, frame, subframe);

	if (mod_litsprites_force.ival || strchr(spr->publicname, '!'))
		litsprite = true;
#ifdef HAVE_LEGACY
	else
	{
		int i;
		/*
		A quick note on tenebrae and sprites: In tenebrae, sprites are always additive, unless the light_lev field is set (which makes it fullbright).
		While its generally preferable and more consistent to assume lit sprites, this is incompatible with vanilla quake and thus unacceptable to us, but you can set the mod_assumelitsprites cvar if you want it.
		So for better compatibility, we have a whitelist of 'well-known' sprites that tenebrae uses in this way, which we do lighting on.
		You should still be able to use EF_FULLBRIGHT on these, but light_lev is an imprecise setting and will result in issues. Just be specific about fullbright or additive.
		DP on the other hand, supports lit sprites only when the sprite contains a ! in its name. We support that too.
		*/
		static char *forcelitsprites[] =
		{
			"progs/smokepuff.spr",
			NULL
		};

		for (i = 0; forcelitsprites[i]; i++)
			if (!strcmp(spr->publicname, forcelitsprites[i]))
			{
				litsprite = true;
				break;
			}
	}
#endif

	if (litsprite)	// a ! in the filename makes it non-fullbright (and can also be lit by rtlights too).
		shadertext = SPRITE_SHADER_LIT;
	else
		shadertext = SPRITE_SHADER_UNLIT;
	frameinfo->lit = litsprite;
	frameinfo->shader = R_RegisterShader(name, SUF_NONE, shadertext);
	frameinfo->shader->defaulttextures->base = frameinfo->image;
	frameinfo->shader->width = frameinfo->right-frameinfo->left;
	frameinfo->shader->height = frameinfo->up-frameinfo->down;
#endif
}
void Mod_LoadSpriteShaders(model_t *spr)
{
	msprite_t *psprite = spr->meshinfo;
	int i, j;
	mspritegroup_t *group;

	for (i = 0; i < psprite->numframes; i++)
	{
		switch (psprite->frames[i].type)
		{
		case SPR_SINGLE:
			Mod_LoadSpriteFrameShader(spr, i, -1, psprite->frames[i].frameptr);
			break;
		case SPR_ANGLED:
		case SPR_GROUP:
			group = (mspritegroup_t *)psprite->frames[i].frameptr;
			for (j = 0; j < group->numframes; j++)
				Mod_LoadSpriteFrameShader(spr, i, j, group->frames[j]);
			break;
		}
	}
}

#ifdef SPRMODELS
/*
=================
Mod_LoadSpriteFrame
=================
*/
static void * Mod_LoadSpriteFrame (model_t *mod, void *pin, void *pend, mspriteframe_t **ppframe, int framenum, int subframe, int version, unsigned char *palette)
{
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	int					width, height, size, origin[2];
	char				name[64];
	uploadfmt_t			lowresfmt;
	void				*dataptr;

	pinframe = (dspriteframe_t *)pin;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);
	size = width * height;

	pspriteframe = ZG_Malloc(&mod->memgroup, sizeof (mspriteframe_t));

	Q_memset (pspriteframe, 0, sizeof (mspriteframe_t));

	*ppframe = pspriteframe;

	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];
	pspriteframe->xmirror = false;

	dataptr = (pinframe + 1);

	if (version == SPRITE32_VERSION)
	{
		size *= 4;
		lowresfmt = TF_RGBA32;
	}
	else if (version == SPRITEHL_VERSION)
		lowresfmt = TF_8PAL32;
	else
		lowresfmt = TF_TRANS8;

	if ((qbyte*)dataptr + size > (qbyte*)pend)
	{
		//tenebrae has a couple of dodgy truncated sprites. yay for replacement textures.
		dataptr = NULL;
		lowresfmt = TF_INVALID;
	}

	if (subframe == -1)
		Q_snprintfz(name, sizeof(name), "%s_%i.tga", mod->name, framenum);
	else
		Q_snprintfz(name, sizeof(name), "%s_%i_%i.tga", mod->name, framenum, subframe);
	pspriteframe->image = Image_GetTexture(name, "sprites", IF_NOMIPMAP|IF_NOGAMMA|IF_CLAMP|IF_PREMULTIPLYALPHA, dataptr, palette, width, height, lowresfmt);

	return (void *)((qbyte *)(pinframe+1) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
static void * Mod_LoadSpriteGroup (model_t *mod, void * pin, void *pend, mspriteframe_t **ppframe, int framenum, int version, unsigned char *palette)
{
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	int					i, numframes;
	dspriteinterval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;
	float				prevtime;

	pingroup = (dspritegroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);
	if (numframes <= 0)
	{
		Con_Printf (CON_ERROR "Mod_LoadSpriteGroup: invalid frame count\n");
		return NULL;
	}

	pspritegroup = ZG_Malloc(&mod->memgroup, sizeof (mspritegroup_t) + (numframes - 1) * sizeof (pspritegroup->frames[0]));

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = ZG_Malloc(&mod->memgroup, numframes * sizeof (float));

	pspritegroup->intervals = poutintervals;

	for (i=0, prevtime=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
		{
			Con_Printf (CON_ERROR "Mod_LoadSpriteGroup: interval<=0\n");
			return NULL;
		}
		prevtime = *poutintervals = prevtime+*poutintervals;

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadSpriteFrame (mod, ptemp, pend, &pspritegroup->frames[i], framenum, i, version, palette);
	}

	return ptemp;
}

/*
=================
Mod_LoadSpriteModel
=================
*/
qboolean QDECL Mod_LoadSpriteModel (model_t *mod, void *buffer, size_t fsize)
{
	int					i;
	int					version;
	dsprite_t			*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dspriteframetype_t	*pframetype;
	int rendertype=SPRHL_ALPHATEST;
	unsigned char pal[256*4];
	int sptype;
	
	pin = (dsprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE_VERSION)
	if (version != SPRITE32_VERSION)
	if (version != SPRITEHL_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number "
				 "(%i should be %i)\n", mod->name, version, SPRITE_VERSION);
		return false;
	}

	sptype = LittleLong (pin->type);

	if (LittleLong(pin->version) == SPRITEHL_VERSION)
	{
		pin = (dsprite_t*)((char*)pin + 4);
		rendertype = LittleLong (pin->type);	//not sure what the values mean.
	}

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = ZG_Malloc(&mod->memgroup, size);

	mod->meshinfo = psprite;
	switch(sptype)
	{
	case SPR_ORIENTED:
		if (r_sprite_backfacing.ival)
			sptype = SPR_ORIENTED_BACKFACE;
		break;
	case SPR_VP_PARALLEL_UPRIGHT:
	case SPR_FACING_UPRIGHT:
	case SPR_VP_PARALLEL:
	case SPR_VP_PARALLEL_ORIENTED:
//	case SPRDP_LABEL:
//	case SPRDP_LABEL_SCALE:
//	case SPRDP_OVERHEAD:
		break;
	default:
		Con_DPrintf(CON_ERROR "%s has unsupported sprite type %i\n", mod->name, sptype);
		sptype = SPR_VP_PARALLEL;
		break;
	}
	psprite->type = sptype;

	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	mod->synctype = LittleLong (pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	if (qrenderer == QR_NONE)
	{
		mod->type = mod_dummy;
		return true;
	}

	if (version == SPRITEHL_VERSION)
	{
		int i;
		short *numi = (short*)(pin+1);
		unsigned char *src = (unsigned char *)(numi+1);
		if (LittleShort(*numi) != 256)
		{
			Con_Printf(CON_ERROR "%s has wrong number of palette indexes (we only support 256)\n", mod->name);
			return false;
		}

		if (rendertype == SPRHL_INDEXALPHA)
		{
			/* alpha value is equivalent to palette index - eukara */
			for (i = 0; i < 256; i++)
			{
				pal[i*4+0] = *src++;
				pal[i*4+1] = *src++;
				pal[i*4+2] = *src++;
				pal[i*4+3] = i;
			}
		}
		else
		{
			for (i = 0; i < 256; i++)
			{//FIXME: bgr?
				pal[i*4+0] = *src++;
				pal[i*4+1] = *src++;
				pal[i*4+2] = *src++;
				pal[i*4+3] = 255;
			}
			if (rendertype == SPRHL_ALPHATEST)
			{
				pal[255*4+0] = 0;
				pal[255*4+1] = 0;
				pal[255*4+2] = 0;
				pal[255*4+3] = 0;
			}
		}

		pframetype = (dspriteframetype_t *)(src);
	}
	else
		pframetype = (dspriteframetype_t *)(pin + 1);

//
// load the frames
//
	if (numframes < 1)
	{
		Con_Printf (CON_ERROR "Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);
		return false;
	}

	mod->numframes = numframes;

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = LittleLong (pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t *)
					Mod_LoadSpriteFrame (mod, pframetype + 1, (qbyte*)buffer + fsize,
										 &psprite->frames[i].frameptr, i, -1, version, pal);
		}
		else
		{
			pframetype = (dspriteframetype_t *)
					Mod_LoadSpriteGroup (mod, pframetype + 1, (qbyte*)buffer + fsize,
										 &psprite->frames[i].frameptr, i, version, pal);
		}
		if (pframetype == NULL)
		{
			return false;
		}
	}

	mod->type = mod_sprite;

	return true;
}
#endif

#ifdef SP2MODELS
qboolean QDECL Mod_LoadSprite2Model (model_t *mod, void *buffer, size_t fsize)
{
	int					i;
	int					version;
	dmd2sprite_t		*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dmd2sprframe_t		*pframetype;
	mspriteframe_t		*frame;
	int w, h;
	float origin[2];

	
	pin = (dmd2sprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE2_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number "
				 "(%i should be %i)", mod->name, version, SPRITE2_VERSION);
		return false;
	}

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = ZG_Malloc(&mod->memgroup, size);

	mod->meshinfo = psprite;

	psprite->type = SPR_VP_PARALLEL;
	psprite->maxwidth = 1;
	psprite->maxheight = 1;
	psprite->beamlength = 1;
	mod->synctype = 0;
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	
//
// load the frames
//
	if (numframes < 1)
	{
		Con_Printf (CON_ERROR "Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);
		return false;
	}

	mod->numframes = numframes;

	pframetype = pin->frames;

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = SPR_SINGLE;
		psprite->frames[i].type = frametype;

		frame = psprite->frames[i].frameptr = ZG_Malloc(&mod->memgroup, sizeof(mspriteframe_t));

		frame->image = Image_GetTexture(pframetype->name, NULL, IF_NOMIPMAP|IF_NOGAMMA|IF_CLAMP, NULL, NULL, 0, 0, TF_INVALID);

		w = LittleLong(pframetype->width);
		h = LittleLong(pframetype->height);
		origin[0] = LittleLong (pframetype->origin_x);
		origin[1] = LittleLong (pframetype->origin_y);

		frame->down = -origin[1];
		frame->up = h - origin[1];
		frame->left = -origin[0];
		frame->right = w - origin[0];
		frame->xmirror = false;

		pframetype++;
	}

	mod->type = mod_sprite;

	return true;
}
#endif

#ifdef DSPMODELS

typedef struct {
	short width;
	short height;
	short xpos;
	short ypos;
} doomimage_t;
static int QDECL FindDoomSprites(const char *name, qofs_t size, time_t mtime, void *param, searchpathfuncs_t *spath)
{
	if (*(int *)param + strlen(name)+1 > 16000)
		Sys_Error("Too many doom sprites\n");

	strcpy((char *)param + *(int *)param, name);
	*(int *)param += strlen(name)+1;

	return true;
}


static void LoadDoomSpriteFrame(model_t *mod, mspriteframe_t frame, mspriteframedesc_t *pdesc, int anglenum)
{
	mspriteframe_t *pframe;

	if (!anglenum)
	{
		pdesc->type = SPR_SINGLE;
		pdesc->frameptr = pframe = ZG_Malloc(&mod->memgroup, sizeof(*pframe));
	}
	else
	{
		mspritegroup_t *group;

		if (!pdesc->frameptr || pdesc->type != SPR_ANGLED)
		{
			pdesc->type = SPR_ANGLED;
			group = ZG_Malloc(&mod->memgroup, sizeof(*group)+sizeof(mspriteframe_t *)*(8-1));
			pdesc->frameptr = (mspriteframe_t *)group;
			group->numframes = 8;
		}
		else
			group = (mspritegroup_t *)pdesc->frameptr;

		pframe = ZG_Malloc(&mod->memgroup, sizeof(*pframe));
		group->frames[anglenum-1] = pframe;
	}

	*pframe = frame;
}

/*
=================
Doom Sprites
=================
*/
void Mod_LoadDoomSprite (model_t *mod)
{
	char files[16384];
	char basename[MAX_QPATH];
	int baselen;
	char *name;

	int numframes=0;
	int ofs;

	int size;

	int elements=0;

	int framenum;
	int anglenum;

	msprite_t *psprite;
	unsigned int image[256*256];
	size_t fsize;
	qbyte palette[256*4];
	doomimage_t *header;
	qbyte *coldata, fr, rc;
	mspriteframe_t frame;
	size_t c;
	unsigned int *colpointers;
	vec3_t t;

	mod->type = mod_dummy;


	COM_StripExtension(mod->name, basename, sizeof(basename));
	baselen = strlen(basename);
	strcat(basename, "*");
	*(int *)files=4;
	COM_EnumerateFiles(basename, FindDoomSprites, files);

	//find maxframes and validate the rest.
	for (ofs = 4; ofs < *(int*)files; ofs+=strlen(files+ofs)+1)
	{
		name = files+ofs+baselen;

		if (!*name)
			Host_Error("Doom sprite componant lacks frame name");
		if (*name - 'a'+1 > numframes)
			numframes = *name - 'a'+1;
		if (name[1] < '0' || name[1] > '8')
			Host_Error("Doom sprite componant has bad angle number");
		if (name[1] == '0')
			elements+=8;
		else
			elements++;
		if (name[2])	//is there a second element?
		{
			if (name[2] - 'a'+1 > numframes)
				numframes = name[2] - 'a'+1;
			if (name[3] < '0' || name[3] > '8')
				Host_Error("Doom sprite componant has bad angle number");

			if (name[3] == '0')
				elements+=8;
			else
				elements++;
		}
	}
	if (elements != numframes*8)
		Con_Printf("Doom sprite %s has wrong componant count", mod->name);
	else if (numframes)
	{
		size = sizeof (msprite_t) +	(elements - 1) * sizeof (psprite->frames);
		psprite = ZG_Malloc(&mod->memgroup, size);

		psprite->numframes = numframes;

		memset(&frame, 0, sizeof(frame));
		coldata = FS_LoadMallocFile("wad/playpal", &fsize);
		if (coldata && fsize >= 256*3)
		{	//expand to 32bit.
			for (ofs = 0; ofs < 256; ofs++)
			{
				palette[ofs*4+0] = coldata[ofs*3+0];
				palette[ofs*4+1] = coldata[ofs*3+1];
				palette[ofs*4+2] = coldata[ofs*3+2];
				palette[ofs*4+3] = 255;
			}
		}
		Z_Free(coldata);

		ClearBounds(mod->mins, mod->maxs);

		//do the actual loading.
		for (ofs = 4; ofs < *(int*)files; ofs+=strlen(files+ofs)+1)
		{
			name = files+ofs;

			header = (doomimage_t *)FS_LoadMallocFile(name, &fsize);

			//the 5 is because doom likes drawing sprites slightly downwards, in the floor.
			frame.up = header->ypos + 5;
			frame.down = header->ypos-header->height + 5;
			frame.left = -header->xpos;
			frame.right = header->width - header->xpos;

			t[0] = t[1] = max(fabs(frame.left),fabs(frame.right));
			t[2] = frame.up;
			AddPointToBounds(t, mod->mins, mod->maxs);
			t[0] *= -1;
			t[1] *= -1;
			t[2] = frame.down;

			if (header->width*header->height <= sizeof(image))
			{
				//anything not written will be transparent.
				memset(image, 0, header->width*header->height*4);
				colpointers = (unsigned int*)(header+1);
				for (c = 0; c < header->width; c++)
				{
					coldata = (qbyte *)header + colpointers[c];
					while(1)
					{
						fr = *coldata++;
						if (fr == 255)
							break;
						rc = *coldata++;
						coldata++;
						if ((fr+rc) > header->height)
							break;
						while(rc)
						{
							image[c + fr*header->width] = ((unsigned int*)palette)[*coldata++];
							fr++;
							rc--;
						}
						coldata++;
					}
				}
				frame.image = Image_GetTexture(name, NULL, IF_CLAMP|IF_NOREPLACE, image, palette, header->width, header->height, TF_RGBA32);
				Z_Free(header);
			}


			framenum = name[baselen+0] - 'a';
			anglenum = name[baselen+1] - '0';
			frame.xmirror = false;
			LoadDoomSpriteFrame(mod, frame, &psprite->frames[framenum], anglenum);

			if (name[baselen+2])	//is there a second element?
			{
				framenum = name[baselen+2] - 'a';
				anglenum = name[baselen+3] - '0';
				frame.xmirror = true;
				LoadDoomSpriteFrame(mod, frame, &psprite->frames[framenum], anglenum);
			}
		}


		psprite->type = SPR_FACING_UPRIGHT;
		mod->numframes = numframes;
		mod->type = mod_sprite;

		mod->meshinfo = psprite;
	}
}
#endif

#endif

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		Con_Printf ("%8p : %s\n", mod->meshinfo, mod->name);
	}
}


#endif
