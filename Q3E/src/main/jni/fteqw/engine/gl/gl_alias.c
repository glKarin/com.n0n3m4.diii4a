
//a note about dedicated servers:
//In the server-side gamecode, a couple of q1 extensions require knowing something about models.
//So we load models serverside, if required.

//things we need:
//tag/bone names and indexes so we can have reasonable modding with tags. :)
//tag/bone positions so we can shoot from the actual gun or other funky stuff
//vertex positions so we can trace against the mesh rather than the bbox.

//we use the gl renderer's model code because it supports more sorts of models than the sw renderer. Sad but true.




#include "quakedef.h"
#include "glquake.h"
#ifndef SERVERONLY

#include "com_mesh.h"

#if defined(RTLIGHTS)
static int numProjectedShadowVerts;
static vec3_t *ProjectedShadowVerts;
static int numFacing;
static qbyte *triangleFacing;
#endif

//FIXME
typedef struct
{
	float		scale[3];	// multiply qbyte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	dtrivertx_t	verts[1];	// variable sized
} dmd2aliasframe_t;



extern cvar_t gl_part_flame, r_fullbrightSkins, r_fb_models, ruleset_allow_fbmodels, gl_overbright_models;
extern cvar_t r_noaliasshadows;
extern cvar_t r_lodscale, r_lodbias;

extern cvar_t gl_ati_truform;
extern cvar_t r_vertexdlights;
extern cvar_t mod_md3flags;
//extern cvar_t r_skin_overlays;
extern cvar_t r_globalskin_first, r_globalskin_count;

#ifndef SERVERONLY
static hashtable_t skincolourmapped;

//q3 .skin support
static skinfile_t **registeredskins;
static skinid_t numregisteredskins;
struct cctx_s
{
	texid_t diffuse;
	int width;
	int height;
};
void Mod_FlushSkin(skinid_t id)
{
#ifdef QWSKINS
	skinfile_t *sk;
	id--;
	if (id >= numregisteredskins)
		return;	//invalid!
	sk = registeredskins[id];
	if (!sk)
		return;
	sk->qwskin = NULL;
#endif
}
void Mod_WipeSkin(skinid_t id, qboolean force)
{
	//FIXME: skin objects should persist for a frame.
	skinfile_t *sk;
	int i;
	id--;
	if (id >= numregisteredskins)
		return;	//invalid!
	sk = registeredskins[id];
	if (!sk)
		return;
	if (!force && --sk->refcount > 0)
		return; //still in use.
	registeredskins[id] = NULL;

	for (i = 0; i < sk->nummappings; i++)
	{
		if (sk->mappings[i].needsfree)
		{
			Image_UnloadTexture(sk->mappings[i].texnums.base);
			sk->mappings[i].texnums.base = r_nulltex;
		}
		R_UnloadShader(sk->mappings[i].shader);
	}
	Z_Free(sk);
}
static void Mod_WipeAllSkins(qboolean final)
{
	skinid_t id;
	if (final)
	{
		for (id = 0; id < numregisteredskins; )
			Mod_WipeSkin(++id, true);
		Z_Free(registeredskins);
		registeredskins = NULL;
		numregisteredskins = 0;
	}
	else
	{
		for (id = 0; id < numregisteredskins; )
			Mod_FlushSkin(++id);
	}
}
skinfile_t *Mod_LookupSkin(skinid_t id)
{
	id--;
	if (id < numregisteredskins)
		return registeredskins[id];
	return NULL;
}
struct composeline_s
{
	shader_t *sourceimg;

	vec2_t pos, size;
	vec4_t tc;
	vec4_t rgba;
};
static void Mod_ParseComposeLine(char *texture, struct composeline_s *line)
{
	int l;
	char *s, tname[MAX_QPATH];
	for (s = texture; *s; s++)
	{
		if (*s == '@' || *s == ':' || *s == '?' || *s == '*')
			break;
	}

	l = s - texture;
	if (l > MAX_QPATH-1)
		l = MAX_QPATH-1;
	memcpy(tname, texture, l);
	tname[l] = 0;

	//load the image and set some default sizes, etc.
	if (*tname)
		line->sourceimg = R2D_SafeCachePic(tname);
	else
		line->sourceimg = NULL;

	Vector2Set(line->pos, 0, 0);
	Vector2Set(line->size, -1, -1);
	Vector4Set(line->tc, 0, 0, 1, 1);
	Vector4Set(line->rgba, 1, 1, 1, 1);

	while(*s)
	{
		switch(*s)
		{
		case '@':
			line->pos[0] = strtod(s+1, &s); 
			if (*s == ',')
				s++;
			line->pos[1] = strtod(s, &s); 
			break;
		case ':':
			line->size[0] = strtod(s+1, &s); 
			if (*s == ',')
				s++;
			line->size[1] = strtod(s, &s); 
			break;
		case '$':
			line->tc[0] = strtod(s+1, &s); 
			if (*s == ',')
				s++;
			line->tc[1] = strtod(s, &s);
			if (*s == ',')
				s++;
			line->tc[2] = strtod(s, &s); 
			if (*s == ',')
				s++;
			line->tc[3] = strtod(s, &s); 
			break;
		case '?':
			line->rgba[0] = strtod(s+1, &s); 
			if (*s == ',')
				s++;
			line->rgba[1] = strtod(s, &s); 
			if (*s == ',')
				s++;
			line->rgba[2] = strtod(s, &s); 
			if (*s == ',')
				s++;
			line->rgba[3] = strtod(s, &s);
			break;
//		case '*':
//			break;
		default:
			s+=strlen(s);	//some sort of error or other thing we don't support
			break;
		}
	}
}
static void Mod_ComposeSkin(char *texture, struct cctx_s *cctx, struct composeline_s *line)
{
	int iw=0, ih=0;

	if (!line->sourceimg || R_GetShaderSizes(line->sourceimg, &iw, &ih, false) != 1)	//no shader? no point in doing anything.
	{
		iw = ih = 0;
		line->sourceimg = NULL;
	}
	if (line->size[0] < 0)
		line->size[0] = iw;
	if (line->size[1] < 0)
		line->size[1] = ih;

	if (line->size[0]>0 && line->size[1]>0)
	{
		//create a render target if one is not already selected
		if (!TEXVALID(cctx->diffuse))
		{
			if (R2D_Flush)
				R2D_Flush();

			strcpy(r_refdef.rt_destcolour[0].texname, "-");
			cctx->width = line->pos[0]+line->size[0];
			cctx->height = line->pos[1]+line->size[1];
			cctx->diffuse = R2D_RT_Configure(r_refdef.rt_destcolour[0].texname, cctx->width, cctx->height, TF_RGBA32, RT_IMAGEFLAGS);
			BE_RenderToTextureUpdate2d(true);
		}

		if (line->sourceimg)
		{
			R2D_ImageColours(line->rgba[0],line->rgba[1],line->rgba[2],line->rgba[3]);
			R2D_Image(line->pos[0], line->pos[1], line->size[0], line->size[1], line->tc[0], line->tc[1], line->tc[2], line->tc[3], line->sourceimg);
		}
	}

	if (R2D_Flush)
		R2D_Flush();
	R_UnloadShader(line->sourceimg);	//we're done with it now
}
//create a new skin with explicit name and text. even if its already loaded. this means you can free it safely.
skinid_t Mod_ReadSkinFile(const char *skinname, const char *skintext)
{
	skinid_t id;
	char *nl;
	skinfile_t *skin;
	char shadername[MAX_QPATH];

	for (id = 0; id < numregisteredskins; id++)
	{
		if (!registeredskins[id])
			break;
	}
	if (id == numregisteredskins)
	{	
		int newn = numregisteredskins + 64;
		registeredskins = BZ_Realloc(registeredskins, sizeof(*registeredskins) * newn);
		memset(registeredskins + numregisteredskins, 0, (newn-numregisteredskins)*sizeof(*registeredskins));
		numregisteredskins = newn;
	}

	skin = Z_Malloc(sizeof(*skin) - sizeof(skin->mappings) + sizeof(skin->mappings[0])*4);
	skin->refcount++;
	skin->maxmappings = 4;
	Q_strncpyz(skin->skinname, skinname, sizeof(skin->skinname));
#ifdef QWSKINS
	skin->q1lower = Q1UNSPECIFIED;
	skin->q1upper = Q1UNSPECIFIED;
#ifdef HEXEN2
	skin->h2class = Q1UNSPECIFIED;
#endif
#endif


	while(skintext)
	{
		if (skin->nummappings == skin->maxmappings)
		{
			skin->maxmappings += 4;
			skin = BZ_Realloc(skin, sizeof(*skin) - sizeof(skin->mappings) + sizeof(skin->mappings[0])*skin->maxmappings);
		}

		skintext = COM_ParseToken(skintext, ",");
		if (!skintext)
			break;
		if (!strcmp(com_token, "replace"))
		{
			skintext = COM_ParseToken(skintext, NULL);

			if (com_tokentype != TTP_LINEENDING)
			{
				Q_strncpyz(skin->mappings[skin->nummappings].surface, com_token, sizeof(skin->mappings[skin->nummappings].surface));
				skintext = COM_ParseToken(skintext, NULL);
				Q_strncpyz(shadername, com_token, sizeof(shadername));
				skin->mappings[skin->nummappings].shader = R_RegisterSkin(NULL, shadername);
				R_BuildDefaultTexnums(NULL, skin->mappings[skin->nummappings].shader, 0);
				skin->mappings[skin->nummappings].texnums = *skin->mappings[skin->nummappings].shader->defaulttextures;
				skin->mappings[skin->nummappings].needsfree = false;
				skin->nummappings++;
			}
		}
		else if (!strcmp(com_token, "compose"))
		{
			skintext = COM_ParseToken(skintext, NULL);
			//body
			if (com_tokentype != TTP_LINEENDING)
			{
				size_t l,lines;
				struct composeline_s line[64];
				//fixme: this blocks waiting for the textures to load.
				struct cctx_s cctx;
				memset(&cctx, 0, sizeof(cctx));

				Q_strncpyz(skin->mappings[skin->nummappings].surface, com_token, sizeof(skin->mappings[skin->nummappings].surface));
				skintext = COM_ParseToken(skintext, NULL);
				Q_strncpyz(shadername, com_token, sizeof(shadername));
				skin->mappings[skin->nummappings].shader = R_RegisterSkin(NULL, shadername);
				R_BuildDefaultTexnums(NULL, skin->mappings[skin->nummappings].shader, 0);
				skin->mappings[skin->nummappings].texnums = *skin->mappings[skin->nummappings].shader->defaulttextures;

				//parse the lines, and start to load the various shaders.
				for(lines = 0;lines<countof(line);)
				{
					while(*skintext == ' ' || *skintext == '\t')
						skintext++;
					if (*skintext == '+')
						skintext++;
					else
						break;
					skintext = COM_Parse(skintext);
					Mod_ParseComposeLine(com_token, &line[lines++]);
				}
				//all the textures should be loading now... block while waiting for them (sucks)
				for (l = 0; l < lines; l++)
					R_GetShaderSizes(line[l].sourceimg, NULL, NULL, true);
				//okay, they're loaded, do the compose now.
				for (l = 0; l < lines; l++)
					Mod_ComposeSkin(com_token, &cctx, &line[l]);
				*r_refdef.rt_destcolour[0].texname = 0;
				BE_RenderToTextureUpdate2d(true);

				skin->mappings[skin->nummappings].needsfree = 1;
				skin->mappings[skin->nummappings].texnums.base = cctx.diffuse;
				skin->nummappings++;
			}
		}
		else if (!strcmp(com_token, "geomset") || !strcmp(com_token, "meshgroup"))
		{
			unsigned int set;
			skintext = COM_ParseToken(skintext, NULL);
			set = atoi(com_token);

			if (com_tokentype != TTP_LINEENDING)
			{
				skintext = COM_ParseToken(skintext, NULL);
				if (set < MAX_GEOMSETS)
					skin->geomset[set] = atoi(com_token);
			}
		}
		else if (!strncmp(com_token, "tag_", 4))
		{
			//ignore it. matches q3.
		}
#ifdef QWSKINS
		else if (!strcmp(com_token, "qwskin"))
		{
			skintext = COM_ParseToken(skintext, NULL);
			Q_strncpyz(skin->qwskinname, com_token, sizeof(skin->qwskinname));
		}
		else if (!strcmp(com_token, "q1lower"))
		{
			skintext = COM_ParseToken(skintext, NULL);
			if (!strncmp(com_token, "0x", 2))
				skin->q1lower = 0xff000000|strtoul(com_token+2, NULL, 16);
			else
				skin->q1lower = atoi(com_token);
		}
		else if (!strcmp(com_token, "q1upper"))
		{
			skintext = COM_ParseToken(skintext, NULL);
			if (!strncmp(com_token, "0x", 2))
				skin->q1upper = 0xff000000|strtoul(com_token+2, NULL, 16);
			else
				skin->q1upper = atoi(com_token);
		}
#ifdef HEXEN2
		else if (!strcmp(com_token, "h2class"))
		{
			skintext = COM_ParseToken(skintext, NULL);
			skin->h2class = atoi(com_token);
		}
#endif
#endif
		else
		{
			while(*skintext == ' ' || *skintext == '\t')
				skintext++;
			if (*skintext == ',')
			{
				Q_strncpyz(skin->mappings[skin->nummappings].surface, com_token, sizeof(skin->mappings[skin->nummappings].surface));
				skintext = COM_ParseToken(skintext+1, NULL);
				Q_strncpyz(shadername, com_token, sizeof(shadername));
				skin->mappings[skin->nummappings].shader = R_RegisterCustom (NULL, shadername, 0, Shader_DefaultSkin, NULL);
				R_BuildDefaultTexnums(NULL, skin->mappings[skin->nummappings].shader, 0);
				skin->mappings[skin->nummappings].texnums = *skin->mappings[skin->nummappings].shader->defaulttextures;
				skin->mappings[skin->nummappings].needsfree = false;
				skin->nummappings++;
			}
		}

		if (com_tokentype == TTP_LINEENDING || !skintext)
			continue;
		nl = strchr(skintext, '\n');
		if (!nl)
			skintext = skintext+strlen(skintext);
		else
			skintext = nl+1;
		if (!*skintext)
			break;
	}

	registeredskins[id] = skin;

#ifdef QWSKINS
	//warm it up, hopefully before its stictly necessary.
	if (*skin->qwskinname)
		Skin_Lookup (skin->qwskinname);
#endif

	return id+1;
}
//registers a skin. loads it if there's not one with that name already registered.
//returns 0 if it failed.
skinid_t Mod_RegisterSkinFile(const char *skinname)
{
	char *f;
	skinid_t id;
	//block duplicates
	for (id = 0; id < numregisteredskins; id++)
	{
		if (!registeredskins[id])
			continue;
		if (!strcmp(skinname, registeredskins[id]->skinname))
		{
			registeredskins[id]->refcount++;
			return id+1;
		}
	}
	f = FS_LoadMallocFile(skinname, NULL);
	if (!f)
		return 0;
	id = Mod_ReadSkinFile(skinname, f);
	Z_Free(f);
	return id;
}


//changes vertex lighting values
#if 0
static void R_GAliasApplyLighting(mesh_t *mesh, vec3_t org, vec3_t angles, float *colormod)
{
	int l, v;
	vec3_t rel;
	vec3_t dir;
	float dot, d, a;

	if (mesh->colors4f_array)
	{
		float l;
		int temp;
		int i;
		avec4_t *colours = mesh->colors4f_array;
		vec3_t *normals = mesh->normals_array;
		vec3_t ambient, shade;
		qbyte alphab = bound(0, colormod[3], 1);
		if (!mesh->normals_array)
		{
			mesh->colors4f_array = NULL;
			return;
		}

		VectorCopy(ambientlight, ambient);
		VectorCopy(shadelight, shade);

		for (i = 0; i < 3; i++)
		{
			ambient[i] *= colormod[i];
			shade[i] *= colormod[i];
		}


		for (i = mesh->numvertexes-1; i >= 0; i--)
		{
			l = DotProduct(normals[i], shadevector);

			temp = l*ambient[0]+shade[0];
			colours[i][0] = temp;

			temp = l*ambient[1]+shade[1];
			colours[i][1] = temp;

			temp = l*ambient[2]+shade[2];
			colours[i][2] = temp;

			colours[i][3] = alphab;
		}
	}

	if (r_vertexdlights.value && mesh->colors4f_array)
	{
		//don't include world lights
		for (l=rtlights_first ; l<RTL_FIRST; l++)
		{
			if (cl_dlights[l].radius)
			{
				VectorSubtract (cl_dlights[l].origin,
								org,
								dir);
				if (Length(dir)>cl_dlights[l].radius+mesh->radius)	//far out man!
					continue;

				rel[0] = -DotProduct(dir, currententity->axis[0]);
				rel[1] = -DotProduct(dir, currententity->axis[1]);	//quake's crazy.
				rel[2] = -DotProduct(dir, currententity->axis[2]);
	/*
				glBegin(GL_LINES);
				glVertex3f(0,0,0);
				glVertex3f(rel[0],rel[1],rel[2]);
				glEnd();
	*/
				for (v = 0; v < mesh->numvertexes; v++)
				{
					VectorSubtract(mesh->xyz_array[v], rel, dir);
					dot = DotProduct(dir, mesh->normals_array[v]);
					if (dot>0)
					{
						d = DotProduct(dir, dir);
						a = 1/d;
						if (a>0)
						{
							a *= 10000000*dot/sqrt(d);
							mesh->colors4f_array[v][0] += a*cl_dlights[l].color[0];
							mesh->colors4f_array[v][1] += a*cl_dlights[l].color[1];
							mesh->colors4f_array[v][2] += a*cl_dlights[l].color[2];
						}
	//					else
	//						mesh->colors4f_array[v][1] = 1;
					}
	//				else
	//					mesh->colors4f_array[v][2] = 1;
				}
			}
		}
	}
}
#endif
/*
void GL_GAliasFlushOneSkin(char *skinname)
{
	int i;
	bucket_t **l;
	galiascolourmapped_t *cm;
	for (i = 0; i < skincolourmapped.numbuckets; i++)
	{
		for(l = &skincolourmapped.bucket[i]; *l; )
		{
			cm = (*l)->data;
			if (strstr(cm->name, skinname))
			{
				*l = cm->bucket.next;
				BZ_Free(cm);
				continue;
			}
			l = &(*l)->next;
		}
	}
}*/

//final is set when the renderer is going down.
//if not set, this is mid-map, and everything should be regeneratable.
void R_GAliasFlushSkinCache(qboolean final)
{
	int i;
	bucket_t *b;
	for (i = 0; i < skincolourmapped.numbuckets; i++)
	{
		while((b = skincolourmapped.bucket[i]))
		{
			skincolourmapped.bucket[i] = b->next;
			BZ_Free(b->data);
		}
	}
	if (skincolourmapped.bucket)
		BZ_Free(skincolourmapped.bucket);
	skincolourmapped.bucket = NULL;
	skincolourmapped.numbuckets = 0;

#ifdef RTLIGHTS
	BZ_Free(ProjectedShadowVerts);
	ProjectedShadowVerts = NULL;
	numProjectedShadowVerts = 0;
	
	BZ_Free(triangleFacing);
	triangleFacing = NULL;
	numFacing = 0;
#endif

	Mod_WipeAllSkins(final);
}

static shader_t *GL_ChooseSkin(galiasinfo_t *inf, model_t *model, int surfnum, entity_t *e, texnums_t **forcedtex)
{
	galiasskin_t *skins;
#ifdef QWSKINS
	shader_t *shader;
	qwskin_t *plskin = NULL;
	unsigned int subframe;
	unsigned int tc = e->topcolour, bc = e->bottomcolour;
#ifdef HEXEN2
	unsigned int pc = e->h2playerclass;
#else
	unsigned int pc = 0;
#endif
	qboolean generateupperlower = false;
	qboolean forced;
	extern int cl_playerindex;	//so I don't have to strcmp
#endif
	int frame;

	*forcedtex = NULL;
	/*q3 .skin files*/
	if (e->customskin)
	{
		skinfile_t *sk = Mod_LookupSkin(e->customskin);
		if (sk)
		{
			int i, fallback=-1;
			if (inf->geomset < MAX_GEOMSETS && sk->geomset[inf->geomset] != inf->geomid)
				return NULL;	//don't allow this surface to be drawn.
			for (i = 0; i < sk->nummappings; i++)
			{
				if (!strcmp(sk->mappings[i].surface, inf->surfacename))
				{
					*forcedtex = &sk->mappings[i].texnums;
					return sk->mappings[i].shader;
				}
				if (!*sk->mappings[i].surface)
					fallback = i;
			}
			if (fallback >= 0)
			{
				*forcedtex = &sk->mappings[fallback].texnums;
				return sk->mappings[fallback].shader;
			}
#ifdef QWSKINS
			if (sk->q1lower != Q1UNSPECIFIED)
				bc = e->bottomcolour = sk->q1lower;
			if (sk->q1upper != Q1UNSPECIFIED)
				tc = e->topcolour = sk->q1upper;
#ifdef HEXEN2
			if (sk->h2class != Q1UNSPECIFIED)
				pc = sk->h2class;
#endif
			if (!sk->qwskin && *sk->qwskinname)
				sk->qwskin = Skin_Lookup(sk->qwskinname);
			plskin = sk->qwskin;
#endif
		}
	}
	else if (inf->geomset < MAX_GEOMSETS && 0 != inf->geomid)
		return NULL;


	/*hexen2 feature: global skins */
	if (inf->numskins < e->skinnum && e->skinnum >= r_globalskin_first.ival && e->skinnum < r_globalskin_first.ival+r_globalskin_count.ival)
	{
		shader_t *s;
		s = R_RegisterSkin(NULL, va("gfx/skin%d.lmp", e->skinnum));
		if (s)
		{
			if (!TEXVALID(s->defaulttextures->base))
				R_BuildDefaultTexnums(NULL, s, 0);
			return s;
		}
	}

#ifdef QWSKINS
	if ((e->model->engineflags & MDLF_NOTREPLACEMENTS) && !ruleset_allow_sensitive_texture_replacements.ival)
		forced = true;
	else
		forced = false;

	if (!gl_nocolors.ival || forced)
	{
		if (plskin && plskin->loadstate == SKIN_LOADING)
			plskin = NULL;
		else if (!plskin || plskin->loadstate != SKIN_LOADED)
		{
			if (e->playerindex >= 0 && e->playerindex <= MAX_CLIENTS)
			{
				//heads don't get skinned, only players (and weaponless players), they do still get recoloured.
				if (model==cl.model_precache[cl_playerindex]
#ifdef HAVE_LEGACY
					|| model==cl.model_precache_vwep[0]
#endif
				)
				{
					if (!cl.players[e->playerindex].qwskin)
						Skin_Find(&cl.players[e->playerindex]);
					plskin = cl.players[e->playerindex].qwskin;
				}
				else
					plskin = NULL;

				if (plskin && plskin->loadstate < SKIN_LOADED)
				{
					Skin_TryCache8(plskin);	//we're not going to use it, but make sure its status is updated when it is finally loaded..
					if (plskin->loadstate < SKIN_LOADED)
					{
						plskin = cl.players[e->playerindex].lastskin;
						if (!plskin || plskin->loadstate < SKIN_LOADED)
							return NULL;	//just wait for it to finish loading so we don't generate pointless skins.
					}
				}
				else
					cl.players[e->playerindex].lastskin = plskin;
			}
			else
				plskin = NULL;
		}

		if (forced || tc != TOP_DEFAULT || bc != BOTTOM_DEFAULT || plskin)
		{
			int			inwidth, inheight;
			int			tinwidth, tinheight;
			char *skinname;
			qbyte	*original;
			galiascolourmapped_t *cm;
			char hashname[512];

			if (!skincolourmapped.numbuckets)
			{
				int bucketcount = 256;
				void *buckets = BZ_Malloc(Hash_BytesForBuckets(bucketcount));
				memset(buckets, 0, Hash_BytesForBuckets(bucketcount));
				Hash_InitTable(&skincolourmapped, bucketcount, buckets);
			}

			if (!inf->numskins)
			{
				/*model has no skins*/
				skins = NULL;
				subframe = 0;
				shader = NULL;
			}
			else
			{
				skins = inf->ofsskins;
				if (e->skinnum >= 0 && e->skinnum < inf->numskins)
					skins += e->skinnum;

				if (!skins->numframes)
				{
					/*model has a skin, but has no framegroups*/
					skins = NULL;
					subframe = 0;
					shader = NULL;
				}
				else
				{
					subframe = cl.time*skins->skinspeed;
					subframe = subframe%skins->numframes;

					shader = skins->frame[subframe].shader;
				}
			}
			if (shader)
			{
				if (!plskin)
				{
					texnums_t *tex = shader->defaulttextures;
					//do this for the loading case too, in the hope that it'll avoid generating a per-player skin at all
					if ((tex->base && (tex->base->status == TEX_LOADING)) ||
						(tex->loweroverlay && (tex->loweroverlay->status == TEX_LOADING || tex->loweroverlay->status == TEX_LOADED)) ||
						(tex->upperoverlay && (tex->upperoverlay->status == TEX_LOADING || tex->upperoverlay->status == TEX_LOADED)))
						return shader;
					//if we've got a replacement texture (read: its size differs from the proper skin size) then don't use the base texels for colourmapping.
					if (tex->base && skins &&
						(tex->base->width!=skins->skinwidth || tex->base->height!=skins->skinheight))
						return shader;
				}
				if ((shader->flags & SHADER_HASTOPBOTTOM) && !h2playertranslations)
				{	//this shader will try to do top+bottom colours. this means we can generate only a black image, with separate top+bottom textures.
					tc = 0xfe000000;
					bc = 0xfe000000;
					generateupperlower = true;
				}
				if (!skins || !skins->numframes || !skins->frame[subframe].texels)
				{	//no top/bottom colourmapping possible here. don't cache a million different values.
					tc = 0xfe000000;
					bc = 0xfe000000;
					generateupperlower = true;
				}
			}

			if (plskin)
			{
				snprintf(hashname, sizeof(hashname), "%s$%s$%i", model->name, plskin->name, surfnum);
				skinname = hashname;
			}
			else if (surfnum)
			{
				snprintf(hashname, sizeof(hashname), "%s$%i", model->name, surfnum);
				skinname = hashname;
			}
			else
				skinname = model->name;

			for (cm = Hash_Get(&skincolourmapped, skinname); cm; cm = Hash_GetNext(&skincolourmapped, skinname, cm))
			{
				if (cm->tcolour == tc && cm->bcolour == bc && cm->skinnum == e->skinnum && cm->subframe == subframe && cm->pclass == pc)
				{
					*forcedtex = &cm->texnum;
					if (!shader)
						shader = R_RegisterSkin(model, skinname);
					return shader;
				}
			}

			if (plskin)
			{
				original = Skin_TryCache8(plskin);	//will start it loading if not already loaded.
				if (plskin->loadstate == SKIN_LOADING)
					return shader;
				inwidth = plskin->width;
				inheight = plskin->height;
			}
			else
			{
				original = NULL;
				inwidth = 0;
				inheight = 0;
			}

			//colourmap isn't present yet.
			cm = Z_Malloc(sizeof(*cm));
			*forcedtex = &cm->texnum;
			Q_strncpyz(cm->name, skinname, sizeof(cm->name));
			Hash_Add(&skincolourmapped, cm->name, cm, &cm->bucket);
			cm->tcolour = tc;
			cm->bcolour = bc;
			cm->pclass = pc;	//is this needed? surely it'll be baked as part of the modelname?
			cm->skinnum = e->skinnum;
			cm->subframe = subframe;

			//q2 has no surfaces in its player models, so don't crash from that
			//note that q2 should also always have a custom skin set. its not our problem (here) if it doesn't.
			if (!shader)
				shader = R_RegisterSkin(model, skinname);

			cm->texnum.bump = shader->defaulttextures->bump;	//can't colour bumpmapping
			if (plskin)
			{
				if (TEXLOADED(plskin->textures.base))
				{
					cm->texnum.loweroverlay = plskin->textures.loweroverlay;
					cm->texnum.upperoverlay = plskin->textures.upperoverlay;
					cm->texnum.base = plskin->textures.base;
					cm->texnum.fullbright = plskin->textures.fullbright;
					cm->texnum.specular = plskin->textures.specular;
					cm->texnum.paletted = r_nulltex;
					cm->texnum.reflectcube = r_nulltex;
					cm->texnum.reflectmask = r_nulltex;
					cm->texnum.displacement = r_nulltex;
					return shader;
				}
			}
			if (!original)
			{
				if (skins && skins->numframes && skins->frame[subframe].texels)
				{
					original = skins->frame[subframe].texels;
					inwidth = skins->skinwidth;
					inheight = skins->skinheight;
				}
				else
				{
					original = NULL;
					inwidth = 0;
					inheight = 0;
				}
			}
			if (skins)
			{
				tinwidth = skins->skinwidth;
				tinheight = skins->skinheight;
			}
			else
			{
				tinwidth = inwidth;
				tinheight = inheight;
			}
			if (original)
			{
				int i, j;
				unsigned translate32[256];
				unsigned	*pixels;
				unsigned	*out;
				unsigned	frac, fracstep;

				unsigned	scaled_width, scaled_height;
				qbyte		*inrow;

				cm->texnum.base = r_nulltex;
				cm->texnum.fullbright = r_nulltex;

				scaled_width = gl_max_size.value < 512 ? gl_max_size.value : 512;
				scaled_height = gl_max_size.value < 512 ? gl_max_size.value : 512;

				//handle the case of an external skin being smaller than the texture that its meant to replace
				//(to support the evil hackage of the padding on the outside of common qw skins)
				if (tinwidth > inwidth)
					tinwidth = inwidth;
				if (tinheight > inheight)
					tinheight = inheight;

				//don't make scaled width any larger than it needs to be
				if (sh_config.texture_non_power_of_two)
				{
					scaled_width = tinwidth;
					scaled_height = tinheight;
				}
				else
				{
					for (i = 0; i < 10; i++)
					{
						scaled_width = (1<<i);
						if (scaled_width >= tinwidth)
							break;	//its covered
					}
					for (i = 0; i < 10; i++)
					{
						scaled_height = (1<<i);
						if (scaled_height >= tinheight)
							break;	//its covered
					}
				}

				if (scaled_width > gl_max_size.value)
					scaled_width = gl_max_size.value;	//whoops, we made it too big
				if (scaled_height > gl_max_size.value)
					scaled_height = gl_max_size.value;	//whoops, we made it too big

				if (scaled_width < 4)
					scaled_width = 4;
				if (scaled_height < 4)
					scaled_height = 4;

#ifdef HEXEN2
				if (h2playertranslations && pc && pc >= 1 && pc < 6)
				{
					unsigned int color_offsets[5] = {2*14*256,0,1*14*256,2*14*256,2*14*256};
					unsigned char *colorA, *colorB, *sourceA, *sourceB;
					colorA = h2playertranslations + 256 + color_offsets[pc-1];
					colorB = colorA + 256;
					sourceA = colorB + ((tc>10)?0:(tc * 256)); //hexen2 allows only colours 0..10 inclusive.
					sourceB = colorB + ((bc>10)?0:(bc * 256));
					for(i=0;i<256;i++)
					{
						translate32[i] = d_8to24rgbtable[i];
						if (tc > 0 && (colorA[i] != 255))
						{
							if (tc >= 16)
							{
								unsigned int v = d_8to24rgbtable[colorA[i]];
								v = max(max((v>>0)&0xff, (v>>8)&0xff), (v>>16)&0xff);
								*((unsigned char*)&translate32[i]+0) = (((tc&0xff0000)>>16)*v)>>8;
								*((unsigned char*)&translate32[i]+1) = (((tc&0x00ff00)>> 8)*v)>>8;
								*((unsigned char*)&translate32[i]+2) = (((tc&0x0000ff)>> 0)*v)>>8;
								*((unsigned char*)&translate32[i]+3) = 0xff;
							}
							else
								translate32[i] = d_8to24rgbtable[sourceA[i]];
						}
						if (bc > 0 && (colorB[i] != 255))
						{
							if (bc >= 16)
							{
								unsigned int v = d_8to24rgbtable[colorB[i]];
								v = max(max((v>>0)&0xff, (v>>8)&0xff), (v>>16)&0xff);
								*((unsigned char*)&translate32[i]+0) = (((bc&0xff0000)>>16)*v)>>8;
								*((unsigned char*)&translate32[i]+1) = (((bc&0x00ff00)>> 8)*v)>>8;
								*((unsigned char*)&translate32[i]+2) = (((bc&0x0000ff)>> 0)*v)>>8;
								*((unsigned char*)&translate32[i]+3) = 0xff;
							}
							else
								translate32[i] = d_8to24rgbtable[sourceB[i]];
						}
					}
					translate32[0] = 0;
				}
				else
#endif
				{
					for (i=0 ; i<256 ; i++)
						translate32[i] = d_8to24rgbtable[i];

					for (i = 0; i < 16; i++)
					{
						if (tc >= 16)
						{
							//assumption: row 0 is pure white.
							*((unsigned char*)&translate32[TOP_RANGE+i]+0) = (((tc&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[i]+0))>>8;
							*((unsigned char*)&translate32[TOP_RANGE+i]+1) = (((tc&0x00ff00)>> 8)**((unsigned char*)&d_8to24rgbtable[i]+1))>>8;
							*((unsigned char*)&translate32[TOP_RANGE+i]+2) = (((tc&0x0000ff)>> 0)**((unsigned char*)&d_8to24rgbtable[i]+2))>>8;
							*((unsigned char*)&translate32[TOP_RANGE+i]+3) = 0xff;
						}
						else
						{
							if (tc < 8)
								translate32[TOP_RANGE+i] = d_8to24rgbtable[(tc<<4)+i];
							else
								translate32[TOP_RANGE+i] = d_8to24rgbtable[(tc<<4)+15-i];
						}
						if (bc >= 16)
						{
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+0) = (((bc&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[i]+0))>>8;
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+1) = (((bc&0x00ff00)>> 8)**((unsigned char*)&d_8to24rgbtable[i]+1))>>8;
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+2) = (((bc&0x0000ff)>> 0)**((unsigned char*)&d_8to24rgbtable[i]+2))>>8;
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+3) = 0xff;
						}
						else
						{
							if (bc < 8)
								translate32[BOTTOM_RANGE+i] = d_8to24rgbtable[(bc<<4)+i];
							else
								translate32[BOTTOM_RANGE+i] = d_8to24rgbtable[(bc<<4)+15-i];
						}
					}
				}

				pixels = malloc(sizeof(*pixels) * scaled_height*scaled_width);
				out = pixels;
				fracstep = tinwidth*0x10000/scaled_width;
				for (i=0 ; i<scaled_height ; i++, out += scaled_width)
				{
					inrow = original + inwidth*(i*inheight/scaled_height);
					frac = fracstep >> 1;
					for (j=0 ; j<scaled_width ; j+=4)
					{
						out[j] = translate32[inrow[frac>>16]];
						frac += fracstep;
						out[j+1] = translate32[inrow[frac>>16]];
						frac += fracstep;
						out[j+2] = translate32[inrow[frac>>16]];
						frac += fracstep;
						out[j+3] = translate32[inrow[frac>>16]];
						frac += fracstep;
					}
				}
				cm->texnum.base = R_LoadTexture(va("base$%x$%x$%i$%i$%i$%s", tc, bc, cm->skinnum, subframe, pc, cm->name),
								scaled_width, scaled_height, h2playertranslations?TF_RGBA32:TF_RGBX32, pixels, IF_NOMIPMAP);

				cm->texnum.bump = shader->defaulttextures->bump;
				cm->texnum.fullbright = shader->defaulttextures->fullbright;
				cm->texnum.specular = shader->defaulttextures->specular;
				cm->texnum.reflectcube = shader->defaulttextures->reflectcube;
				cm->texnum.reflectmask = shader->defaulttextures->reflectmask;
				cm->texnum.paletted = shader->defaulttextures->paletted;
				cm->texnum.displacement = shader->defaulttextures->displacement;

#ifdef HEXEN2	//too lazy to do this
				if (h2playertranslations && pc)
					;
				else
#endif
				if (r_softwarebanding)
				{
					qbyte *pixels8 = (void*)pixels;
					qbyte *out8;
					for (i=0 ; i<256 ; i++)
						translate32[i] = i;

					//fancy colours are not supported here. try to aproximate them.
					if (tc >= 16)
						tc = GetPaletteIndexNoFB((tc>>16)&0xff, (tc>>8)&0xff, (tc>>0)&0xff)/16;
					if (bc >= 16)
						bc = GetPaletteIndexNoFB((bc>>16)&0xff, (bc>>8)&0xff, (bc>>0)&0xff)/16;

					for (i = 0; i < 16; i++)
					{
						if (tc < 8)
							translate32[TOP_RANGE+i] = (tc<<4)+i;
						else
							translate32[TOP_RANGE+i] = (tc<<4)+15-i;

						if (bc < 8)
							translate32[BOTTOM_RANGE+i] = (bc<<4)+i;
						else
							translate32[BOTTOM_RANGE+i] = (bc<<4)+15-i;
					}

					fracstep = tinwidth*0x10000/scaled_width;
					for (i=0, out8=pixels8 ; i<scaled_height ; i++, out8 += scaled_width)
					{
						inrow = original + inwidth*(i*inheight/scaled_height);
						frac = fracstep >> 1;
						for (j=0 ; j<scaled_width ; j+=4)
						{
							out8[j] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out8[j+1] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out8[j+2] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out8[j+3] = translate32[inrow[frac>>16]];
							frac += fracstep;
						}
					}


					cm->texnum.paletted = R_LoadTexture(va("paletted$%x$%x$%i$%i$%i$%s", tc, bc, cm->skinnum, subframe, pc, cm->name),
								scaled_width, scaled_height, PTI_P8, pixels8, IF_PALETTIZE|IF_NEAREST|IF_NOMIPMAP);

				}

				/*if (!h2playertranslations)
				{
					qboolean valid = false;
					//now do the fullbrights.
					out = pixels;
					fracstep = tinwidth*0x10000/scaled_width;
					for (i=0 ; i<scaled_height ; i++, out += scaled_width)
					{
						inrow = original + inwidth*(i*inheight/scaled_height);
						frac = fracstep >> 1;
						for (j=0 ; j<scaled_width ; j+=1)
						{
							if (inrow[frac>>16] < 255-vid.fullbright)
								((char *) (&out[j]))[3] = 0;	//alpha 0
							else
								valid = true;
							frac += fracstep;
						}
					}
					if (valid)
						cm->texnum.fullbright = R_LoadTexture(va("fb$%x$%x$%i$%i$%i$%s", tc, bc, cm->skinnum, subframe, pc, cm->name),
										scaled_width, scaled_height, TF_RGBA32, pixels, IF_NOMIPMAP);
				}*/

				if (generateupperlower)
				{
					for (i=0 ; i<256 ; i++)
						translate32[i] = 0xff000000;
					for (i = 0; i < 16; i++)
						translate32[TOP_RANGE+i] = d_8to24rgbtable[i];
					out = pixels;
					fracstep = tinwidth*0x10000/scaled_width;
					for (i=0 ; i<scaled_height ; i++, out += scaled_width)
					{
						inrow = original + inwidth*(i*inheight/scaled_height);
						frac = fracstep >> 1;
						for (j=0 ; j<scaled_width ; j+=4)
						{
							out[j] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out[j+1] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out[j+2] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out[j+3] = translate32[inrow[frac>>16]];
							frac += fracstep;
						}
					}
					cm->texnum.upperoverlay = R_LoadTexture(va("up$%i$%i$%i$%s", cm->skinnum, subframe, pc, cm->name),
									scaled_width, scaled_height, TF_RGBA32, pixels, IF_NOMIPMAP);

					for (i=0 ; i<256 ; i++)
						translate32[i] = 0xff000000;
					for (i = 0; i < 16; i++)
						translate32[BOTTOM_RANGE+i] = d_8to24rgbtable[i];
					out = pixels;
					fracstep = tinwidth*0x10000/scaled_width;
					for (i=0 ; i<scaled_height ; i++, out += scaled_width)
					{
						inrow = original + inwidth*(i*inheight/scaled_height);
						frac = fracstep >> 1;
						for (j=0 ; j<scaled_width ; j+=4)
						{
							out[j] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out[j+1] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out[j+2] = translate32[inrow[frac>>16]];
							frac += fracstep;
							out[j+3] = translate32[inrow[frac>>16]];
							frac += fracstep;
						}
					}
					cm->texnum.loweroverlay = R_LoadTexture(va("lo$%i$%i$%i$%s", cm->skinnum, subframe, pc, cm->name),
									scaled_width, scaled_height, TF_RGBA32, pixels, IF_NOMIPMAP);

				}
				free(pixels);
			}
			else
			{
				/*model has no original skin info and thus cannot be reskinned, copy over the default textures so that the skincache doesn't break things when it gets reused*/
				cm->texnum = *shader->defaulttextures;
			}
			return shader;
		}
	}
#endif
	if (!inf->numskins)
		return NULL;

	skins = inf->ofsskins;
	if (e->skinnum >= 0 && e->skinnum < inf->numskins)
		skins += e->skinnum;
	else
	{
		static float timer;
		Con_ThrottlePrintf(&timer, 1, "Skin number out of range (%u >= %u - %s)\n", e->skinnum, inf->numskins, model->name);
		if (!inf->numskins)
			return NULL;
	}

	if (!skins->numframes)
		return NULL;

	frame = cl.time*skins->skinspeed;
	frame = frame%skins->numframes;
	return skins->frame[frame].shader;
}

#if defined(RTLIGHTS)
static void R_CalcFacing(mesh_t *mesh, vec3_t lightpos)
{
	float *v1, *v2, *v3;
	vec3_t d1, d2, norm;

	int i;

	index_t *indexes = mesh->indexes;
	int numtris = mesh->numindexes/3;


	if (numFacing < numtris)
	{
		if (triangleFacing)
			BZ_Free(triangleFacing);
		triangleFacing = BZ_Malloc(sizeof(*triangleFacing)*numtris);
		numFacing = numtris;
	}

	for (i = 0; i < numtris; i++, indexes+=3)
	{
		v1 = (float *)(mesh->xyz_array + indexes[0]);
		v2 = (float *)(mesh->xyz_array + indexes[1]);
		v3 = (float *)(mesh->xyz_array + indexes[2]);

		VectorSubtract(v1, v2, d1);
		VectorSubtract(v3, v2, d2);
		CrossProduct(d1, d2, norm);

		triangleFacing[i] = (( lightpos[0] - v1[0] ) * norm[0] + ( lightpos[1] - v1[1] ) * norm[1] + ( lightpos[2] - v1[2] ) * norm[2]) > 0;
	}
}

#define PROJECTION_DISTANCE 30000
static void R_ProjectShadowVolume(mesh_t *mesh, vec3_t lightpos)
{
	int numverts = mesh->numvertexes;
	int i;
	vecV_t *input = mesh->xyz_array;
	vec3_t *projected;
	if (numProjectedShadowVerts < numverts)
	{
		if (ProjectedShadowVerts)
			BZ_Free(ProjectedShadowVerts);
		ProjectedShadowVerts = BZ_Malloc(sizeof(*ProjectedShadowVerts)*numverts);
		numProjectedShadowVerts = numverts;
	}
	projected = ProjectedShadowVerts;
	for (i = 0; i < numverts; i++)
	{
		projected[i][0] = input[i][0] + (input[i][0]-lightpos[0])*PROJECTION_DISTANCE;
		projected[i][1] = input[i][1] + (input[i][1]-lightpos[1])*PROJECTION_DISTANCE;
		projected[i][2] = input[i][2] + (input[i][2]-lightpos[2])*PROJECTION_DISTANCE;
	}
}

static void R_DrawShadowVolume(mesh_t *mesh)
{
#ifdef GLQUAKE
	int t;
	vec3_t *proj = ProjectedShadowVerts;
	vecV_t *verts = mesh->xyz_array;
	index_t *indexes = mesh->indexes;
	int *neighbours = mesh->trneighbors;
	int numtris = mesh->numindexes/3;

	qglBegin(GL_TRIANGLES);
	for (t = 0; t < numtris; t++)
	{
		if (triangleFacing[t])
		{
			//draw front
			qglVertex3fv(verts[indexes[t*3+0]]);
			qglVertex3fv(verts[indexes[t*3+1]]);
			qglVertex3fv(verts[indexes[t*3+2]]);

			//draw back
			qglVertex3fv(proj[indexes[t*3+1]]);
			qglVertex3fv(proj[indexes[t*3+0]]);
			qglVertex3fv(proj[indexes[t*3+2]]);

			//draw side caps
			if (neighbours[t*3+0] < 0 || !triangleFacing[neighbours[t*3+0]])
			{
				qglVertex3fv(verts[indexes[t*3+1]]);
				qglVertex3fv(verts[indexes[t*3+0]]);
				qglVertex3fv(proj [indexes[t*3+0]]);
				qglVertex3fv(verts[indexes[t*3+1]]);
				qglVertex3fv(proj [indexes[t*3+0]]);
				qglVertex3fv(proj [indexes[t*3+1]]);
			}

			if (neighbours[t*3+1] < 0 || !triangleFacing[neighbours[t*3+1]])
			{
				qglVertex3fv(verts[indexes[t*3+2]]);
				qglVertex3fv(verts[indexes[t*3+1]]);
				qglVertex3fv(proj [indexes[t*3+1]]);
				qglVertex3fv(verts[indexes[t*3+2]]);
				qglVertex3fv(proj [indexes[t*3+1]]);
				qglVertex3fv(proj [indexes[t*3+2]]);
			}

			if (neighbours[t*3+2] < 0 || !triangleFacing[neighbours[t*3+2]])
			{
				qglVertex3fv(verts[indexes[t*3+0]]);
				qglVertex3fv(verts[indexes[t*3+2]]);
				qglVertex3fv(proj [indexes[t*3+2]]);
				qglVertex3fv(verts[indexes[t*3+0]]);
				qglVertex3fv(proj [indexes[t*3+2]]);
				qglVertex3fv(proj [indexes[t*3+0]]);
			}
		}
	}
	qglEnd();
#endif
}
#endif

//true if no shading is to be used.
qboolean R_CalcModelLighting(entity_t *e, model_t *clmodel)
{
	vec3_t lightdir;
	int i;
	vec3_t dist;
	float add, m;
	vec3_t shadelight, ambientlight;

	if (e->light_known)
		return e->light_known-1;

	e->light_dir[0] = 0; e->light_dir[1] = 1; e->light_dir[2] = 0;
#ifdef HEXEN2
	if ((e->drawflags & MLS_MASK) == MLS_ABSLIGHT)
	{	//per-entity fixed lighting
		e->light_range[0] = e->light_range[1] = e->light_range[2] =
		e->light_avg[0] = e->light_avg[1] = e->light_avg[2] = e->abslight/255.f;
		e->light_known = 2;
		return e->light_known-1;
	}
	if ((e->drawflags & MLS_MASK) && (e->drawflags & MLS_MASK) != MLS_ADDLIGHT)
	{	//these use some qc-defined lightstyles.
		i = 24 + (e->drawflags & MLS_MASK); //saves knowing the proper patterns at least.
		VectorScale(cl_lightstyle[i].colours, d_lightstylevalue[i]/255.f, e->light_range);
		VectorScale(cl_lightstyle[i].colours, d_lightstylevalue[i]/255.f, e->light_avg);
		e->light_known = 2;
		return e->light_known-1;
	}
#endif
	if ((clmodel->engineflags & MDLF_FLAME) ||	//stuff on fire should generally have enough light...
		r_fullbright.ival ||	//vanila cheat
		(e->flags & RF_FULLBRIGHT) ||	//DP feature
		(r_fb_models.ival == 1 && ruleset_allow_fbmodels.ival && (clmodel->engineflags & MDLF_EZQUAKEFBCHEAT) && cls.protocol == CP_QUAKEWORLD && cl.deathmatch && cls.allow_fbskins>0))	//ezquake cheat
	{
		e->light_avg[0] = e->light_avg[1] = e->light_avg[2] = 1;
		e->light_range[0] = e->light_range[1] = e->light_range[2] = 0;
		e->light_known = 2;
		return e->light_known-1;
	}

	if (!(r_refdef.flags & RDF_NOWORLDMODEL) && cl.worldmodel)
	{
		if (e->flags & RF_WEAPONMODEL)
		{
			cl.worldmodel->funcs.LightPointValues(cl.worldmodel, r_refdef.vieworg, shadelight, ambientlight, lightdir);
			for (i = 0; i < 3; i++)
			{	/*viewmodels may not be pure black*/
				if (ambientlight[i] < 24)
					ambientlight[i] = 24;
			}
		}
		else
		{
			vec3_t center;
			#if 0 /*hexen2*/
			VectorAvg(clmodel->mins, clmodel->maxs, center);
			VectorAdd(e->origin, center, center);
			#else
			VectorCopy(e->origin, center);
			center[2] += 24;
			#endif
			cl.worldmodel->funcs.LightPointValues(cl.worldmodel, center, shadelight, ambientlight, lightdir);
		}
	}
	else
	{
		ambientlight[0] = ambientlight[1] = ambientlight[2] = shadelight[0] = shadelight[1] = shadelight[2] = 128;
		lightdir[0] = 0;
		lightdir[1] = 1;
		lightdir[2] = 1;
	}

#ifdef HEXEN2
	if ((e->drawflags & MLS_MASK) == MLS_ADDLIGHT)
	{
		ambientlight[0] += e->abslight;
		ambientlight[1] += e->abslight;
		ambientlight[2] += e->abslight;
		shadelight[0] += e->abslight;
		shadelight[1] += e->abslight;
		shadelight[2] += e->abslight;
	}
#endif

	if (r_softwarebanding)
	{
		//mimic software rendering as closely as possible
		lightdir[2] = 0;	//horizontal light only.

//		VectorMA(vec3_origin, 0.5, shadelight, ambientlight);
//		VectorCopy(ambientlight, shadelight);

		if (!r_vertexdlights.ival && r_dlightlightmaps)
		{
			float *org = e->origin;
			if (e->flags & RF_WEAPONMODEL)
				org = r_refdef.vieworg;
			//don't do world lights, although that might be funny
			for (i=rtlights_first; i<RTL_FIRST; i++)
			{
				if (!(*cl_dlights[i].cubemapname) && cl_dlights[i].radius)
				{
					VectorSubtract (org,
									cl_dlights[i].origin,
									dist);
					add = cl_dlights[i].radius - Length(dist);
#ifdef RTLIGHTS
					if (r_shadow_realtime_world.ival)	//if world lighting is on, there may be no lightmap influence even if r_dynamic is on.
						add *= r_shadow_realtime_world_lightmaps.value;
#endif
					if (add > 0)
					{
						if (r_dynamic.ival == 2)
						{
							ambientlight[0] += add * 2;
							ambientlight[1] += add * 2;
							ambientlight[2] += add * 2;
						}
						else
						{
							ambientlight[0] += add * cl_dlights[i].color[0];
							ambientlight[1] += add * cl_dlights[i].color[1];
							ambientlight[2] += add * cl_dlights[i].color[2];
						}
					}
				}
			}
		}

		for (i = 0; i < 3; i++)
		{
			if (ambientlight[i] > 128)
				ambientlight[i] = 128;
			if (ambientlight[i] + shadelight[i] > 192)
				shadelight[i] = 192 - ambientlight[i];
		}
	}
	else
	{
		if (!r_vertexdlights.ival && r_dlightlightmaps)
		{
			float *org = e->origin;
			if (e->flags & RF_WEAPONMODEL)
				org = r_refdef.vieworg;

			//don't do world lights, although that might be funny
			for (i=rtlights_first; i<RTL_FIRST; i++)
			{
				if (!(*cl_dlights[i].cubemapname) && cl_dlights[i].radius)
				{
					VectorSubtract (org,
									cl_dlights[i].origin,
									dist);
					add = cl_dlights[i].radius - Length(dist);
#ifdef RTLIGHTS
					if (r_shadow_realtime_world.ival)	//if world lighting is on, there may be no lightmap influence even if r_dynamic is on.
						add *= r_shadow_realtime_world_lightmaps.value;
#endif

					if (add > 0)
					{
						if (r_dynamic.ival == 2)
						{
							ambientlight[0] += add * 2;
							ambientlight[1] += add * 2;
							ambientlight[2] += add * 2;
							//ZOID models should be affected by dlights as well
							shadelight[0] += add * 2;
							shadelight[1] += add * 2;
							shadelight[2] += add * 2;
						}
						else
						{
							ambientlight[0] += add * cl_dlights[i].color[0];
							ambientlight[1] += add * cl_dlights[i].color[1];
							ambientlight[2] += add * cl_dlights[i].color[2];
							//ZOID models should be affected by dlights as well
							shadelight[0] += add * cl_dlights[i].color[0];
							shadelight[1] += add * cl_dlights[i].color[1];
							shadelight[2] += add * cl_dlights[i].color[2];
						}
					}
				}
			}
		}

		switch(PTI_E5BGR9)//lightmap_fmt)
		{
		//don't clamp model lighting if we're not clamping world lighting either.
		case PTI_E5BGR9:
		case PTI_RGBA16F:
		case PTI_RGBA32F:
			break;
		default:	//non-hdr lightmap format. clamp model lighting to match the lightmap's clamps.
			m = max(max(ambientlight[0], ambientlight[1]), ambientlight[2]);
			if (m > 255)
			{
				ambientlight[0] *= 255.0/m;
				ambientlight[1] *= 255.0/m;
				ambientlight[2] *= 255.0/m;
			}
			m = max(max(shadelight[0], shadelight[1]), shadelight[2]);
			if (m > 128)
			{
				shadelight[0] *= 128.0/m;
				shadelight[1] *= 128.0/m;
				shadelight[2] *= 128.0/m;
			}
			break;
		}

		//MORE HUGE HACKS! WHEN WILL THEY CEASE!
		// clamp lighting so it doesn't overbright as much
		// ZOID: never allow players to go totally black
		if (e->playerindex >= 0 && !(e->flags & (RF_WEAPONMODEL|RF_WEAPONMODELNOBOB|RF_DEPTHHACK)))
		{
			float fb = r_fullbrightSkins.value;
			if (fb > cls.allow_fbskins)
				fb = cls.allow_fbskins;
			if (fb < 0)
				fb = 0;
			if (fb)
			{
				extern cvar_t r_fb_models;

				if (fb >= 1 && r_fb_models.value)
				{
					ambientlight[0] = ambientlight[1] = ambientlight[2] = 1;
					shadelight[0] = shadelight[1] = shadelight[2] = 1;

					VectorSet(e->light_dir, 1, 0, 0);
					VectorClear(e->light_range);
					VectorScale(shadelight, fb, e->light_avg);

					e->light_known = 2;
					return e->light_known-1;
				}
				else
				{
					for (i = 0; i < 3; i++)
					{
						ambientlight[i] = max(ambientlight[i], 8 + fb * 120);
						shadelight[i] = max(shadelight[i], 8 + fb * 120);
					}
				}
			}
			for (i = 0; i < 3; i++)
			{
				if (ambientlight[i] < 8)
					ambientlight[i] = 8;
			}
		}


#if 1	//match quakespasm here.
		if (gl_overbright_models.value > 0.f && ruleset_allow_fbmodels.ival)
		{
			m = (96*6) / (shadelight[0]+shadelight[1]+shadelight[2]+ambientlight[0]+ambientlight[1]+ambientlight[2]);
			if (m > 1.0)
				m = 1;	//we only want to let it darken here.
			m *= 1 + min(1,gl_overbright_models.value);
		}
		else
			m = 1;
		m /= 200.0/255;	//a legacy quake fudge-factor.
		VectorScale(shadelight, m, shadelight);
		VectorScale(ambientlight, m, ambientlight);
#else
		for (i = 0; i < 3; i++)
		{
			if (ambientlight[i] > 128)
				ambientlight[i] = 128;

			shadelight[i] /= 200.0/255;
			ambientlight[i] /= 200.0/255;
		}
#endif

		if ((e->model->flags & MF_ROTATE) && cl.hexen2pickups)
		{
			shadelight[0] = shadelight[1] = shadelight[2] =
			ambientlight[0] = ambientlight[1] = ambientlight[2] = 128+sin(cl.servertime*4)*64;
		}
	}

	if (e->flags & RF_WEAPONMODEL)
	{
		vec3_t temp;
		temp[0] = DotProduct(lightdir, vpn);
		temp[1] = -DotProduct(lightdir, vright);
		temp[2] = DotProduct(lightdir, vup);

		e->light_dir[0] = DotProduct(temp, e->axis[0]);
		e->light_dir[1] = DotProduct(temp, e->axis[1]);
		e->light_dir[2] = DotProduct(temp, e->axis[2]);
	}
	else
	{
		e->light_dir[0] = DotProduct(lightdir, e->axis[0]);
		e->light_dir[1] = DotProduct(lightdir, e->axis[1]);
		e->light_dir[2] = DotProduct(lightdir, e->axis[2]);
	}
	VectorNormalize(e->light_dir);

	shadelight[0] *= 1/255.0f;
	shadelight[1] *= 1/255.0f;
	shadelight[2] *= 1/255.0f;
	ambientlight[0] *= 1/255.0f;
	ambientlight[1] *= 1/255.0f;
	ambientlight[2] *= 1/255.0f;

	if (e->flags & Q2RF_GLOW)
	{
		float scale = 1 + 0.2 * sin(cl.time*7);
		VectorScale(ambientlight, scale, ambientlight);
		VectorScale(shadelight, scale, shadelight);
	}

	if (r_softwarebanding)
	{	//overbright the models.
		VectorScale(ambientlight, 2, e->light_avg);
		VectorScale(shadelight, 2, e->light_range);
	}
	else if (1)
	{	//calculate average and range, to allow for negative lighting dotproducts
		VectorCopy(shadelight, e->light_avg);
		VectorCopy(ambientlight, e->light_range);
	}
	else
	{	//calculate average and range, to allow for negative lighting dotproducts
		VectorMA(ambientlight, 0.5, shadelight, e->light_avg);
		VectorSubtract(shadelight, ambientlight, e->light_range);
	}

	e->light_known = 1;
	return e->light_known-1;
}

void R_GAlias_DrawBatch(batch_t *batch)
{
	entity_t *e;

	galiasinfo_t *inf;
	model_t *clmodel;
	unsigned int surfnum;

	static mesh_t mesh;
	static mesh_t *meshl = &mesh;

//	qboolean needrecolour;
//	qboolean nolightdir;

	e = batch->ent;
	clmodel = e->model;

	currententity = e;
	/*nolightdir =*/ R_CalcModelLighting(e, clmodel);

	inf = Mod_Extradata (clmodel);
	if (inf)
	{
		memset(&mesh, 0, sizeof(mesh));
		for(surfnum=0; inf; inf=inf->nextsurf, surfnum++)
		{
			if (batch->user.alias.surfrefs[0] == surfnum)
			{
				/*needrecolour =*/ Alias_GAliasBuildMesh(&mesh, &batch->vbo, inf, surfnum, e, batch->shader->prog && (batch->shader->prog->supportedpermutations & PERMUTATION_SKELETAL));
				batch->mesh = &meshl;
				if (!mesh.numindexes)
				{
					batch->meshes = 0;	//something went screwy
					batch->mesh = NULL;
				}
				return;
			}
		}
	}
	batch->meshes = 0;
	Con_Printf("Broken model surfaces mid-frame\n");
	return;
}

void R_GAlias_GenerateBatches(entity_t *e, batch_t **batches)
{
	galiasinfo_t *inf;
	model_t *clmodel;
	shader_t *shader, *regshader;
	batch_t *b;
	int surfnum, j;
	shadersort_t sort;
	float lod;

	texnums_t *skin;

	if ((r_refdef.externalview || r_refdef.recurse) && (e->flags & RF_WEAPONMODEL))
		return;

	clmodel = e->model;
#ifdef QWSKINS
	/*switch model if we're the player model, and the player skin says a new model*/
	{
		extern int cl_playerindex;
		if (e->playerindex >= 0 && e->model == cl.model_precache[cl_playerindex])
		{
			clmodel = cl.players[e->playerindex].model;
			if (!clmodel || clmodel->type != mod_alias)
				clmodel = e->model;	//oops, never mind
		}
	}
#endif

	if (!(e->flags & RF_WEAPONMODEL)
#ifdef SKELETALMODELS
		&& !e->framestate.bonestate
#endif
		)
	{
		if (R_CullEntityBox (e, clmodel->mins, clmodel->maxs))
			return;
#ifdef RTLIGHTS
		if (BE_LightCullModel(e->origin, clmodel))
			return;
	}
	else
	{
		if (BE_LightCullModel(r_origin, clmodel))
			return;
#endif
	}

	if (clmodel->tainted)
	{
		if (!ruleset_allow_modified_eyes.ival && !strcmp(clmodel->name, "progs/eyes.mdl"))
			return;
	}

	inf = Mod_Extradata (clmodel);

	if (clmodel->maxlod)
	{
		vec3_t v;
		float z;

		VectorSubtract(e->origin, r_refdef.vieworg, v);
		z = DotProduct(v, vpn);
		if (z < -clmodel->radius)
			return;		//furthest extent of bounding sphere is nearer than the near clip plane, and thus completely invisible
		else if (z < 0)
			lod = 0;	//nearer than the camera, use the highest lod
		else
		{
			//if the ent is in the middle of the screen, then the right edge of its sphere is at what percentage of the width of the screen...?

			//simplified Matrix4x4_CM_Transform4
			float coverage = (r_refdef.m_projection_std[5]*clmodel->radius + r_refdef.m_projection_std[ 9]*-z + r_refdef.m_projection_std[13]) /
							 (r_refdef.m_projection_std[7]*clmodel->radius + r_refdef.m_projection_std[11]*-z + r_refdef.m_projection_std[15]);
			lod = 1-(coverage*r_lodscale.value);
			lod = bound(0, lod, 1);	//so lodbias is a little more reliable.
			lod *= clmodel->maxlod;
			lod += r_lodbias.value;
			lod = max(0, lod);	//never nearer than 0, the min value check wouldn't cope.
		}
	}
	else
		lod = 0;

	for(surfnum=0; inf; inf=inf->nextsurf, surfnum++)
	{
		if (!inf->numindexes)
			continue;
		if (lod < inf->mindist || (inf->maxdist && lod >= inf->maxdist))
			continue;

		regshader = GL_ChooseSkin(inf, clmodel, surfnum, e, &skin);
		if (!regshader)
			continue;
		skin = skin?skin:NULL;
		shader = e->forcedshader?e->forcedshader:regshader;
		if (shader && !(shader->flags & SHADER_NODRAW))
		{
			b = BE_GetTempBatch();
			if (!b)
				break;

			b->buildmeshes = R_GAlias_DrawBatch;
			b->ent = e;
			b->envmap = Mod_CubemapForOrigin(cl.worldmodel, e->origin);
#if defined(Q3BSPS) || defined(RFBSPS)
			b->fog = Mod_FogForOrigin(cl.worldmodel, e->origin);
#endif
			b->mesh = NULL;
			b->firstmesh = 0;
			b->meshes = 1;
			b->skin = skin;
			b->texture = NULL;
			b->shader = shader;
			for (j = 0; j < MAXRLIGHTMAPS; j++)
			{
				b->lightmap[j] = -1;
				b->lmlightstyle[j] = INVALID_LIGHTSTYLE;
			}
			b->user.alias.surfrefs[0] = surfnum;
			b->flags = 0;
			sort = shader->sort;
			if (e->flags & RF_FORCECOLOURMOD)
				b->flags |= BEF_FORCECOLOURMOD;
			if (e->flags & RF_ADDITIVE)
			{
				b->flags |= BEF_FORCEADDITIVE;
				if (sort < SHADER_SORT_ADDITIVE)
					sort = SHADER_SORT_ADDITIVE;
			}
			if (e->flags & RF_TRANSLUCENT)
			{
				b->flags |= BEF_FORCETRANSPARENT;
				if (SHADER_SORT_PORTAL < sort && sort < SHADER_SORT_BLEND)
					sort = SHADER_SORT_BLEND;
			}
#ifdef HEXEN2
			else if (e->drawflags & DRF_TRANSLUCENT)
			{
				b->flags |= BEF_FORCETRANSPARENT;
				if (SHADER_SORT_PORTAL < sort && sort < SHADER_SORT_BLEND)
					sort = SHADER_SORT_BLEND;
				e->shaderRGBAf[3] = r_wateralpha.value;
			}
#endif
			if (e->flags & RF_NODEPTHTEST)
			{
				b->flags |= BEF_FORCENODEPTH;
				if (sort < SHADER_SORT_NEAREST)
					sort = SHADER_SORT_NEAREST;
			}
			if ((e->flags & RF_NOSHADOW) || (clmodel->engineflags & MDLF_NOSHADOWS))
				b->flags |= BEF_NOSHADOWS;
			b->vbo = NULL;
			b->next = batches[sort];
			batches[sort] = b;
		}
	}
}

#if 0
void R_Sprite_GenerateBatches(entity_t *e, batch_t **batches)
{
	galiasinfo_t *inf;
	model_t *clmodel;
	shader_t *shader;
	batch_t *b;
	int surfnum;

	texnums_t *skin;

	if (r_refdef.externalview && e->flags & Q2RF_WEAPONMODEL)
		return;

	clmodel = e->model;

	if (!(e->flags & Q2RF_WEAPONMODEL))
	{
		if (R_CullEntityBox (e, clmodel->mins, clmodel->maxs))
			return;
#ifdef RTLIGHTS
		if (BE_LightCullModel(e->origin, clmodel))
			return;
	}
	else
	{
		if (BE_LightCullModel(r_origin, clmodel))
			return;
#endif
	}

	if (clmodel->tainted)
	{
		if (!ruleset_allow_modified_eyes.ival && !strcmp(clmodel->name, "progs/eyes.mdl"))
			return;
	}

	inf = RMod_Extradata (clmodel);

	if (!e->model || e->forcedshader)
	{
		//fixme
		return;
	}
	else
	{
		frame = R_GetSpriteFrame (e);
		psprite = e->model->cache.data;
		sprtype = psprite->type;
		shader = frame->shader;
	}

	if (shader)
	{
		b = BE_GetTempBatch();
		if (!b)
			break;

		b->buildmeshes = R_Sprite_DrawBatch;
		b->ent = e;
		b->mesh = NULL;
		b->firstmesh = 0;
		b->meshes = 1;
		b->skin = frame-;
		b->texture = NULL;
		b->shader = frame->shader;
		for (j = 0; j < MAXRLIGHTMAPS; j++)
			b->lightmap[j] = -1;
		b->surf_first = surfnum;
		b->flags = 0;
		b->vbo = NULL;
		b->next = batches[shader->sort];
		batches[shader->sort] = b;
	}
}
#endif

//returns the rotated offset of the two points in result
void RotateLightVector(const vec3_t *axis, const vec3_t origin, const vec3_t lightpoint, vec3_t result)
{
	vec3_t offs;

	offs[0] = lightpoint[0] - origin[0];
	offs[1] = lightpoint[1] - origin[1];
	offs[2] = lightpoint[2] - origin[2];

	result[0] = DotProduct (offs, axis[0]);
	result[1] = DotProduct (offs, axis[1]);
	result[2] = DotProduct (offs, axis[2]);
}

#if defined(RTLIGHTS)
/*
static void GL_LightMesh (mesh_t *mesh, vec3_t lightpos, vec3_t colours, float radius)
{
	vec3_t dir;
	int i;
	float dot, d, f, a;

	vecV_t *xyz = mesh->xyz_array;
	vec3_t *normals = mesh->normals_array;
	vec4_t *out = mesh->colors4f_array;

	if (!out)
		return;	//urm..

	if (normals)
	{
		for (i = 0; i < mesh->numvertexes; i++)
		{
			VectorSubtract(lightpos, xyz[i], dir);
			dot = DotProduct(dir, normals[i]);
			if (dot > 0)
			{
				d = DotProduct(dir, dir)/radius;
				a = 1/d;
				if (a>0)
				{
					a *= dot/sqrt(d);
					f = a*colours[0];
					out[i][0] = f;

					f = a*colours[1];
					out[i][1] = f;

					f = a*colours[2];
					out[i][2] = f;
				}
				else
				{
					out[i][0] = 0;
					out[i][1] = 0;
					out[i][2] = 0;
				}
			}
			else
			{
				out[i][0] = 0;
				out[i][1] = 0;
				out[i][2] = 0;
			}
			out[i][3] = 1;
		}
	}
	else
	{
		for (i = 0; i < mesh->numvertexes; i++)
		{
			VectorSubtract(lightpos, xyz[i], dir);
			out[i][0] = colours[0];
			out[i][1] = colours[1];
			out[i][2] = colours[2];
			out[i][3] = 1;
		}
	}
}
*/

//courtesy of DP
static void R_BuildBumpVectors(const float *v0, const float *v1, const float *v2, const float *tc0, const float *tc1, const float *tc2, float *fte_restrict svector3f, float *fte_restrict tvector3f, float *fte_restrict normal3f)
{
	float f, tangentcross[3], v10[3], v20[3], tc10[2], tc20[2];
	// 79 add/sub/negate/multiply (1 cycle), 1 compare (3 cycle?), total cycles not counting load/store/exchange roughly 82 cycles
	// 6 add, 28 subtract, 39 multiply, 1 compare, 50% chance of 6 negates

	// 6 multiply, 9 subtract
	VectorSubtract(v1, v0, v10);
	VectorSubtract(v2, v0, v20);
	normal3f[0] = v10[1] * v20[2] - v10[2] * v20[1];
	normal3f[1] = v10[2] * v20[0] - v10[0] * v20[2];
	normal3f[2] = v10[0] * v20[1] - v10[1] * v20[0];
	// 12 multiply, 10 subtract
	tc10[1] = tc1[1] - tc0[1];
	tc20[1] = tc2[1] - tc0[1];
	svector3f[0] = tc10[1] * v20[0] - tc20[1] * v10[0];
	svector3f[1] = tc10[1] * v20[1] - tc20[1] * v10[1];
	svector3f[2] = tc10[1] * v20[2] - tc20[1] * v10[2];
	tc10[0] = tc1[0] - tc0[0];
	tc20[0] = tc2[0] - tc0[0];
	tvector3f[0] = tc10[0] * v20[0] - tc20[0] * v10[0];
	tvector3f[1] = tc10[0] * v20[1] - tc20[0] * v10[1];
	tvector3f[2] = tc10[0] * v20[2] - tc20[0] * v10[2];
	// 12 multiply, 4 add, 6 subtract
	f = DotProduct(svector3f, normal3f);
	svector3f[0] -= f * normal3f[0];
	svector3f[1] -= f * normal3f[1];
	svector3f[2] -= f * normal3f[2];
	f = DotProduct(tvector3f, normal3f);
	tvector3f[0] -= f * normal3f[0];
	tvector3f[1] -= f * normal3f[1];
	tvector3f[2] -= f * normal3f[2];
	// if texture is mapped the wrong way (counterclockwise), the tangents
	// have to be flipped, this is detected by calculating a normal from the
	// two tangents, and seeing if it is opposite the surface normal
	// 9 multiply, 2 add, 3 subtract, 1 compare, 50% chance of: 6 negates
	CrossProduct(tvector3f, svector3f, tangentcross);
	if (DotProduct(tangentcross, normal3f) < 0)
	{
		VectorNegate(svector3f, svector3f);
		VectorNegate(tvector3f, tvector3f);
	}
}

#if 0
//courtesy of DP
void R_AliasGenerateTextureVectors(mesh_t *mesh, float *fte_restrict normal3f, float *fte_restrict svector3f, float *fte_restrict tvector3f)
{
	int i;
	float sdir[3], tdir[3], normal[3], *v;
	index_t *e;
	float *vertex3f = (float*)mesh->xyz_array;
	float *texcoord2f = (float*)mesh->st_array;
	// clear the vectors
//	if (svector3f)
		memset(svector3f, 0, mesh->numvertexes * sizeof(float[3]));
//	if (tvector3f)
		memset(tvector3f, 0, mesh->numvertexes * sizeof(float[3]));
//	if (normal3f)
		memset(normal3f, 0, mesh->numvertexes * sizeof(float[3]));
	// process each vertex of each triangle and accumulate the results
	for (e = mesh->indexes; e < mesh->indexes+mesh->numindexes; e += 3)
	{
		R_BuildBumpVectors(vertex3f + e[0] * 3, vertex3f + e[1] * 3, vertex3f + e[2] * 3, texcoord2f + e[0] * 2, texcoord2f + e[1] * 2, texcoord2f + e[2] * 2, sdir, tdir, normal);
//		if (!areaweighting)
//		{
//			VectorNormalize(sdir);
//			VectorNormalize(tdir);
//			VectorNormalize(normal);
//		}
//		if (svector3f)
			for (i = 0;i < 3;i++)
				VectorAdd(svector3f + e[i]*3, sdir, svector3f + e[i]*3);
//		if (tvector3f)
			for (i = 0;i < 3;i++)
				VectorAdd(tvector3f + e[i]*3, tdir, tvector3f + e[i]*3);
//		if (normal3f)
			for (i = 0;i < 3;i++)
				VectorAdd(normal3f + e[i]*3, normal, normal3f + e[i]*3);
	}
	// now we could divide the vectors by the number of averaged values on
	// each vertex...  but instead normalize them
	// 4 assignments, 1 divide, 1 sqrt, 2 adds, 6 multiplies
	if (svector3f)
		for (i = 0, v = svector3f;i < mesh->numvertexes;i++, v += 3)
			VectorNormalize(v);
	// 4 assignments, 1 divide, 1 sqrt, 2 adds, 6 multiplies
	if (tvector3f)
		for (i = 0, v = tvector3f;i < mesh->numvertexes;i++, v += 3)
			VectorNormalize(v);
	// 4 assignments, 1 divide, 1 sqrt, 2 adds, 6 multiplies
	if (normal3f)
		for (i = 0, v = normal3f;i < mesh->numvertexes;i++, v += 3)
			VectorNormalize(v);
}
#endif

//calculate S+T vectors without also breaking the normals
void R_Generate_Mesh_ST_Vectors(mesh_t *mesh)
{
	int i;
	vec3_t sdir, tdir, normal, *s, *t, *n;
	index_t *e;
	vecV_t *vertex3f = mesh->xyz_array;
	vec2_t *texcoord2f = mesh->st_array;
	vec3_t *normal3f = mesh->normals_array;
	vec3_t *fte_restrict svector3f = mesh->snormals_array;
	vec3_t *fte_restrict tvector3f = mesh->tnormals_array;
	float frac;
	// clear the vectors
	memset(svector3f, 0, mesh->numvertexes * sizeof(float[3]));
	memset(tvector3f, 0, mesh->numvertexes * sizeof(float[3]));
	// process each vertex of each triangle and accumulate the results
	for (e = mesh->indexes; e < mesh->indexes+mesh->numindexes; e += 3)
	{
		R_BuildBumpVectors(vertex3f[e[0]], vertex3f[e[1]], vertex3f[e[2]], texcoord2f[e[0]], texcoord2f[e[1]], texcoord2f[e[2]], sdir, tdir, normal);
//		if (!areaweighting)
//		{
//			VectorNormalize(sdir);
//			VectorNormalize(tdir);
//		}
		for (i = 0;i < 3;i++)
			VectorAdd(svector3f[e[i]], sdir, svector3f[e[i]]);
		for (i = 0;i < 3;i++)
			VectorAdd(tvector3f[e[i]], tdir, tvector3f[e[i]]);
	}
	for (i = 0, s = svector3f, t = tvector3f, n = normal3f;i < mesh->numvertexes;i++, s++, t++, n++)
	{
		frac = -DotProduct(*s, *n);
		VectorMA(*s, frac, *n, *s);
		VectorNormalize(*s);

		frac = -DotProduct(*t, *n);
		VectorMA(*t, frac, *n, *t);
		VectorNormalize(*t);
	}
}

//FIXME: Be less agressive.
//This function will have to be called twice (for geforce cards), with the same data, so do the building once and rendering twice.
void R_DrawGAliasShadowVolume(entity_t *e, vec3_t lightpos, float radius)
{
	model_t *clmodel = e->model;
	galiasinfo_t *inf;
	mesh_t mesh;
	vec3_t lightorg;
	int surfnum = 0;

	if (qrenderer != QR_OPENGL)
		return;

	if (clmodel->engineflags & MDLF_NOSHADOWS)
		return;
	if (r_noaliasshadows.ival)
		return;

//	if (e->shaderRGBAf[3] < 0.5)
//		return;

	RotateLightVector((void *)e->axis, e->origin, lightpos, lightorg);

	if (Length(lightorg) > radius + clmodel->radius)
		return;

	BE_SelectEntity(e);

	inf = Mod_Extradata (clmodel);
	while(inf)
	{
		if (inf->ofs_trineighbours)
		{
			Alias_GAliasBuildMesh(&mesh, NULL, inf, surfnum, e, false);
			R_CalcFacing(&mesh, lightorg);
			R_ProjectShadowVolume(&mesh, lightorg);
			R_DrawShadowVolume(&mesh);
		}

		inf = inf->nextsurf;

		surfnum++;
	}
}
#endif





#if 0
static int R_FindTriangleWithEdge ( index_t *indexes, int numtris, index_t start, index_t end, int ignore)
{
	int i;
	int match, count;

	count = 0;
	match = -1;

	for (i = 0; i < numtris; i++, indexes += 3)
	{
		if ( (indexes[0] == start && indexes[1] == end)
			|| (indexes[1] == start && indexes[2] == end)
			|| (indexes[2] == start && indexes[0] == end) ) {
			if (i != ignore)
				match = i;
			count++;
		} else if ( (indexes[1] == start && indexes[0] == end)
			|| (indexes[2] == start && indexes[1] == end)
			|| (indexes[0] == start && indexes[2] == end) ) {
			count++;
		}
	}

	// detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}
#endif

#if 0
static void R_BuildTriangleNeighbours ( int *neighbours, index_t *indexes, int numtris )
{
	int i, *n;
	index_t *index;

	for (i = 0, index = indexes, n = neighbours; i < numtris; i++, index += 3, n += 3)
	{
		n[0] = R_FindTriangleWithEdge (indexes, numtris, index[1], index[0], i);
		n[1] = R_FindTriangleWithEdge (indexes, numtris, index[2], index[1], i);
		n[2] = R_FindTriangleWithEdge (indexes, numtris, index[0], index[2], i);
	}
}
#endif




#if 0
void GL_GenerateNormals(float *orgs, float *normals, int *indicies, int numtris, int numverts)
{
	vec3_t d1, d2;
	vec3_t norm;
	int t, i, v1, v2, v3;
	int tricounts[MD2MAX_VERTS];
	vec3_t combined[MD2MAX_VERTS];
	int triremap[MD2MAX_VERTS];
	if (numverts > MD2MAX_VERTS)
		return;	//not an issue, you just loose the normals.

	memset(triremap, 0, numverts*sizeof(triremap[0]));

	v2=0;
	for (i = 0; i < numverts; i++)	//weld points
	{
		for (v1 = 0; v1 < v2; v1++)
		{
			if (orgs[i*3+0] == combined[v1][0] &&
				orgs[i*3+1] == combined[v1][1] &&
				orgs[i*3+2] == combined[v1][2])
			{
				triremap[i] = v1;
				break;
			}
		}
		if (v1 == v2)
		{
			combined[v1][0] = orgs[i*3+0];
			combined[v1][1] = orgs[i*3+1];
			combined[v1][2] = orgs[i*3+2];
			v2++;

			triremap[i] = v1;
		}
	}
	memset(tricounts, 0, v2*sizeof(tricounts[0]));
	memset(combined, 0, v2*sizeof(*combined));

	for (t = 0; t < numtris; t++)
	{
		v1 = triremap[indicies[t*3]];
		v2 = triremap[indicies[t*3+1]];
		v3 = triremap[indicies[t*3+2]];

		VectorSubtract((orgs+v2*3), (orgs+v1*3), d1);
		VectorSubtract((orgs+v3*3), (orgs+v1*3), d2);
		CrossProduct(d1, d2, norm);
		VectorNormalize(norm);

		VectorAdd(norm, combined[v1], combined[v1]);
		VectorAdd(norm, combined[v2], combined[v2]);
		VectorAdd(norm, combined[v3], combined[v3]);

		tricounts[v1]++;
		tricounts[v2]++;
		tricounts[v3]++;
	}

	for (i = 0; i < numverts; i++)
	{
		if (tricounts[triremap[i]])
		{
			VectorScale(combined[triremap[i]], 1.0f/tricounts[triremap[i]], normals+i*3);
		}
	}
}
#endif
#endif







#if defined(Q2CLIENT) || defined(Q3CLIENT)
//q3 lightning gun / q3 railgun / q2 beams
static void R_Beam_GenerateTrisoup(entity_t *e, int bemode)
{
	float lightmap;
	unsigned int batchflags = 0;
	vecV_t *xyz;
	vec2_t *st;
	vec4_t *rgba;
	scenetris_t *t;
	shader_t *shader = NULL;
	float scale, length;
	vec3_t dir, v, cr;

	shader = e->forcedshader;
	if (!shader)
		shader = R_RegisterShader("q2beam", SUF_NONE,
			"{\n"
				"{\n"
					"map $whiteimage\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
					"blendfunc blend\n"
				"}\n"
			"}\n"
			);

	batchflags = 0;
//	if (e->flags & RF_NOSHADOW)
		batchflags |= BEF_NOSHADOWS;
	if (e->flags & RF_ADDITIVE)
		batchflags |= BEF_FORCEADDITIVE;
	if (e->flags & RF_TRANSLUCENT)
		batchflags |= BEF_FORCETRANSPARENT;
	if (e->flags & RF_NODEPTHTEST)
		batchflags |= BEF_FORCENODEPTH;
	if (e->flags & RF_FORCECOLOURMOD)
		batchflags |= BEF_FORCECOLOURMOD;
	if (shader->flags & SHADER_NODLIGHT)
		batchflags |= BEF_NODLIGHT;

	if ((batchflags & BEF_NODLIGHT) || (shader->flags & SHADER_NODLIGHT) || bemode != BEM_STANDARD)
	{
		//unlit sprites are just fullbright
		lightmap = 1;
	}
	else
	{
#ifdef RTLIGHTS
		extern cvar_t r_shadow_realtime_world_lightmaps;
		//lit sprites need to sample the world lighting. with rtlights that generally means they're 0.
		if (r_shadow_realtime_world.ival)
			lightmap = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			lightmap = 1;
	}

	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == batchflags)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->flags = batchflags;
	}

	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx=cl_numstrisidx+6 + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert+64);

	xyz = &cl_strisvertv[cl_numstrisvert];
	st = &cl_strisvertt[cl_numstrisvert];
	rgba = &cl_strisvertc[cl_numstrisvert];

	cl_strisidx[cl_numstrisidx++] = t->numvert+0;
	cl_strisidx[cl_numstrisidx++] = t->numvert+1;
	cl_strisidx[cl_numstrisidx++] = t->numvert+2;
	cl_strisidx[cl_numstrisidx++] = t->numvert+0;
	cl_strisidx[cl_numstrisidx++] = t->numvert+2;
	cl_strisidx[cl_numstrisidx++] = t->numvert+3;
	t->numidx += 6;
	t->numvert += 4;
	cl_numstrisvert += 4;

	scale = e->scale*5;
	if (!scale)
		scale = 5;

	if (shader->flags & SHADER_CULL_FRONT)
		scale *= -1;

	VectorSubtract(e->origin, e->oldorigin, dir);
	length = Length(dir);

	Vector2Set(st[0], 0, 1);
	Vector2Set(st[1], 0, 0);
	Vector2Set(st[2], length/128, 0);
	Vector2Set(st[3], length/128, 1);

	VectorSubtract(r_refdef.vieworg, e->origin, v);
	CrossProduct(v, dir, cr);
	VectorNormalize(cr);

	VectorMA(e->origin, -scale/2, cr, xyz[0]);
	VectorMA(e->origin, scale/2, cr, xyz[1]);

	VectorSubtract(r_refdef.vieworg, e->oldorigin, v);
	CrossProduct(v, dir, cr);
	VectorNormalize(cr);

	VectorMA(e->oldorigin, scale/2, cr, xyz[2]);
	VectorMA(e->oldorigin, -scale/2, cr, xyz[3]);

	if (e->shaderRGBAf[0] != 0 || e->shaderRGBAf[1] != 0 || e->shaderRGBAf[2] != 0 || (batchflags & BEF_FORCECOLOURMOD))
	{
		if (e->shaderRGBAf[0] > 1)
			e->shaderRGBAf[0] = 1;
		if (e->shaderRGBAf[1] > 1)
			e->shaderRGBAf[1] = 1;
		if (e->shaderRGBAf[2] > 1)
			e->shaderRGBAf[2] = 1;
	}
	else
	{
		e->shaderRGBAf[0] = 1;
		e->shaderRGBAf[1] = 1;
		e->shaderRGBAf[2] = 1;
	}

	VectorScale(e->shaderRGBAf, lightmap, rgba[0]);
	rgba[0][3] = e->shaderRGBAf[3];
	Vector4Copy(rgba[0], rgba[1]);
	Vector4Copy(rgba[0], rgba[2]);
	Vector4Copy(rgba[0], rgba[3]);
}
#endif

static void R_Sprite_GenerateTrisoup(entity_t *e, int bemode)
{
	vec3_t	point;
	mspriteframe_t	genframe;
	vec3_t		spraxis[3];
	msprite_t		*psprite;
	vec3_t sprorigin;
	unsigned int sprtype;
	float lightmap;
	unsigned int batchflags = 0;
	vecV_t *xyz;
	vec2_t *st;
	vec4_t *rgba;
	scenetris_t *t;

	shader_t *shader = NULL;
	mspriteframe_t *frame;

	if (!e->model || e->model->type != mod_sprite || e->forcedshader)
	{
		frame = NULL;
		shader = e->forcedshader;
		if (!shader)
			shader = R_RegisterShader("q2beam", SUF_NONE,
				"{\n"
					"{\n"
						"map $whiteimage\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"blendfunc blend\n"
					"}\n"
				"}\n"
				);
	}
	else
	{
		if (!(e->flags & RF_WEAPONMODEL))
		{
			if (R_CullEntityBox (e, e->model->mins, e->model->maxs))
				return;
	#ifdef RTLIGHTS
			if (BE_LightCullModel(e->origin, e->model))
				return;
		}
		else
		{
			if (BE_LightCullModel(r_origin, e->model))
				return;
	#endif
		}

		// don't even bother culling, because it's just a single
		// polygon without a surface cache
		frame = R_GetSpriteFrame(e);
		shader = frame->shader;
	}

	batchflags = 0;
//	if (e->flags & RF_NOSHADOW)
		batchflags |= BEF_NOSHADOWS;
	if (e->flags & RF_ADDITIVE)
		batchflags |= BEF_FORCEADDITIVE;
	if (e->flags & RF_TRANSLUCENT)
		batchflags |= BEF_FORCETRANSPARENT;
	if (e->flags & RF_NODEPTHTEST)
		batchflags |= BEF_FORCENODEPTH;
	if (e->flags & RF_FORCECOLOURMOD)
		batchflags |= BEF_FORCECOLOURMOD;
	if (shader->flags & SHADER_NODLIGHT)
		batchflags |= BEF_NODLIGHT;
//	if (shader->flags & RF_TWOSIDED)
//		batchflags |= BEF_FORCETWOSIDED;

	if ((batchflags & BEF_NODLIGHT) || (shader->flags & SHADER_NODLIGHT) || bemode != BEM_STANDARD)
	{
		//unlit sprites are just fullbright
		lightmap = 1;
	}
	else
	{
#ifdef RTLIGHTS
		extern cvar_t r_shadow_realtime_world_lightmaps;
		//lit sprites need to sample the world lighting. with rtlights that generally means they're 0.
		if (r_shadow_realtime_world.ival)
			lightmap = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			lightmap = 1;
	}

	if (e->flags & RF_WEAPONMODELNOBOB)
	{
		sprorigin[0] = r_refdef.weaponmatrix[3][0];
		sprorigin[1] = r_refdef.weaponmatrix[3][1];
		sprorigin[2] = r_refdef.weaponmatrix[3][2];
		VectorMA(sprorigin, e->origin[0], r_refdef.weaponmatrix[0], sprorigin);
		VectorMA(sprorigin, e->origin[1], r_refdef.weaponmatrix[1], sprorigin);
		VectorMA(sprorigin, e->origin[2], r_refdef.weaponmatrix[2], sprorigin);
//		VectorMA(sprorigin, 12, vpn, sprorigin);

//		batchflags |= BEF_FORCENODEPTH;
	}
	else if (e->flags & RF_WEAPONMODEL)
	{
		sprorigin[0] = r_refdef.weaponmatrix_bob[3][0];
		sprorigin[1] = r_refdef.weaponmatrix_bob[3][1];
		sprorigin[2] = r_refdef.weaponmatrix_bob[3][2];
		VectorMA(sprorigin, e->origin[0], r_refdef.weaponmatrix_bob[0], sprorigin);
		VectorMA(sprorigin, e->origin[1], r_refdef.weaponmatrix_bob[1], sprorigin);
		VectorMA(sprorigin, e->origin[2], r_refdef.weaponmatrix_bob[2], sprorigin);
//		VectorMA(sprorigin, 12, vpn, sprorigin);

//		batchflags |= BEF_FORCENODEPTH;
	}
	else
		VectorCopy(e->origin, sprorigin);


	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == batchflags)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->flags = batchflags;
	}

	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx=cl_numstrisidx+6 + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert+64);

	xyz = &cl_strisvertv[cl_numstrisvert];
	st = &cl_strisvertt[cl_numstrisvert];
	rgba = &cl_strisvertc[cl_numstrisvert];

	cl_strisidx[cl_numstrisidx++] = t->numvert+0;
	cl_strisidx[cl_numstrisidx++] = t->numvert+1;
	cl_strisidx[cl_numstrisidx++] = t->numvert+2;
	cl_strisidx[cl_numstrisidx++] = t->numvert+0;
	cl_strisidx[cl_numstrisidx++] = t->numvert+2;
	cl_strisidx[cl_numstrisidx++] = t->numvert+3;
	t->numidx += 6;
	t->numvert += 4;
	cl_numstrisvert += 4;

	if (!frame)
	{
		genframe.down = genframe.left = -1;
		genframe.up = genframe.right = 1;
		genframe.xmirror = false;
		genframe.lit = false;
		sprtype = SPR_VP_PARALLEL;
		frame = &genframe;
	}
	else
	{
		// don't even bother culling, because it's just a single
		// polygon without a surface cache
		psprite = e->model->meshinfo;
		sprtype = psprite->type;
	}

	safeswitch(sprtype)
	{
	case SPR_ORIENTED:
		// bullet marks on walls
		if (e->flags & RF_WEAPONMODELNOBOB)
			Matrix3_Multiply(e->axis, r_refdef.weaponmatrix, spraxis);
		else if (e->flags & RF_WEAPONMODEL)
			Matrix3_Multiply(e->axis, r_refdef.weaponmatrix_bob, spraxis);
		else
			memcpy(spraxis, e->axis, sizeof(spraxis));
		break;
	case SPR_ORIENTED_BACKFACE:
		// bullet marks on walls, invisible to anyone in the direction that its facing...
		if (e->flags & RF_WEAPONMODELNOBOB)
			Matrix3_Multiply(e->axis, r_refdef.weaponmatrix, spraxis);
		else  if ((e->flags & RF_WEAPONMODEL) && r_refdef.playerview->viewentity > 0)
			Matrix3_Multiply(e->axis, r_refdef.weaponmatrix_bob, spraxis);
		else
			memcpy(spraxis, e->axis, sizeof(spraxis));
		VectorNegate(spraxis[1], spraxis[1]);
		break;

	case SPR_FACING_UPRIGHT:
		//up vector is worldspace up
		//side is crossproduct of (org-vieworg),up
		spraxis[2][0] = 0;spraxis[2][1] = 0;spraxis[2][2]=1;
		spraxis[1][0] = sprorigin[1] - r_origin[1];
		spraxis[1][1] = -(sprorigin[0] - r_origin[0]);
		spraxis[1][2] = 0;
		VectorNormalize (spraxis[1]);
		break;
	case SPR_VP_PARALLEL_UPRIGHT:
		//up vector is worldspace up
		//side vector matches view
		spraxis[2][0] = 0;spraxis[2][1] = 0;spraxis[2][2]=1;
		VectorCopy (vright, spraxis[1]);
		break;

	case SPR_VP_PARALLEL_ORIENTED:
		//normal sprite, except rotating with roll angles
		{
			vec3_t ang;
			int i;
			float cr,sr;
			VectorAngles(e->axis[0], e->axis[2], ang, false);	//bah, slow.
			cr = cos(ang[2] * M_PI/180);
			sr = sin(ang[2]);
			for (i=0 ; i<3 ; i++)
			{
				spraxis[1][i] = vright[i] * cr + vup[i] * sr;
				spraxis[2][i] = vright[i] * -sr + vup[i] * cr;
			}
        }
		break;
	case SPR_VP_PARALLEL:
		//normal sprite
	safedefault:
		VectorCopy(vup, spraxis[2]);
		VectorCopy(vright, spraxis[1]);
		break;
	}
	if (e->scale)
	{
		spraxis[2][0]*=e->scale;
		spraxis[2][1]*=e->scale;
		spraxis[2][2]*=e->scale;
		spraxis[1][0]*=e->scale;
		spraxis[1][1]*=e->scale;
		spraxis[1][2]*=e->scale;
	}

	if (e->shaderRGBAf[0] != 0 || e->shaderRGBAf[1] != 0 || e->shaderRGBAf[2] != 0 || (batchflags & BEF_FORCECOLOURMOD))
	{
//		if (e->shaderRGBAf[0] > 1)
//			e->shaderRGBAf[0] = 1;
//		if (e->shaderRGBAf[1] > 1)
//			e->shaderRGBAf[1] = 1;
//		if (e->shaderRGBAf[2] > 1)
//			e->shaderRGBAf[2] = 1;
	}
	else
	{
		e->shaderRGBAf[0] = 1;
		e->shaderRGBAf[1] = 1;
		e->shaderRGBAf[2] = 1;
	}

	VectorScale(e->shaderRGBAf, lightmap, rgba[0]);
	if (frame && frame->lit && !(r_refdef.flags & RDF_NOWORLDMODEL) && cl.worldmodel && cl.worldmodel->funcs.LightPointValues)
	{
		R_CalcModelLighting(e, e->model);
		VectorMul(rgba[0], e->light_avg, rgba[0]);
		VectorMA(rgba[0], 0.5, e->light_range, rgba[0]);
	}
	rgba[0][3] = e->shaderRGBAf[3];
	Vector4Copy(rgba[0], rgba[1]);
	Vector4Copy(rgba[0], rgba[2]);
	Vector4Copy(rgba[0], rgba[3]);

	if (frame->xmirror)
	{
		Vector2Set(st[0], 1, 1);
		Vector2Set(st[1], 1, 0);
		Vector2Set(st[2], 0, 0);
		Vector2Set(st[3], 0, 1);
	}
	else
	{
		Vector2Set(st[0], 0, 1);
		Vector2Set(st[1], 0, 0);
		Vector2Set(st[2], 1, 0);
		Vector2Set(st[3], 1, 1);
	}

	VectorMA (sprorigin, frame->down, spraxis[2], point);
	VectorMA (point, frame->left, spraxis[1], xyz[0]);

	VectorMA (sprorigin, frame->up, spraxis[2], point);
	VectorMA (point, frame->left, spraxis[1], xyz[1]);

	VectorMA (sprorigin, frame->up, spraxis[2], point);
	VectorMA (point, frame->right, spraxis[1], xyz[2]);

	VectorMA (sprorigin, frame->down, spraxis[2], point);
	VectorMA (point, frame->right, spraxis[1], xyz[3]);
}

static void R_DB_Poly(batch_t *batch)
{
	static mesh_t mesh;
	static mesh_t *meshptr = &mesh;
	unsigned int i = batch->user.poly.surface;

	batch->mesh = &meshptr;

	mesh.xyz_array = cl_strisvertv + cl_stris[i].firstvert;
	mesh.st_array = cl_strisvertt + cl_stris[i].firstvert;
//	mesh.normals_array = cl_strisvertn[0];// + cl_stris[i].firstvert;
//	mesh.snormals_array = cl_strisvertn[1];// + cl_stris[i].firstvert;
//	mesh.tnormals_array = cl_strisvertn[2];// + cl_stris[i].firstvert;
	mesh.colors4f_array[0] = cl_strisvertc + cl_stris[i].firstvert;
	mesh.indexes = cl_strisidx + cl_stris[i].firstidx;
	mesh.numindexes = cl_stris[i].numidx;
	mesh.numvertexes = cl_stris[i].numvert;
}
static void BE_GenPolyBatches(batch_t **batches)
{
	shader_t *shader = NULL;
	batch_t *b;
	unsigned int i = cl_numstris, j;
	unsigned int sort;

	while (i-- > 0)
	{
		if (!cl_stris[i].numidx)
			continue;

		b = BE_GetTempBatch();
		if (!b)
			return;

		shader = cl_stris[i].shader;
		if (!shader)
			continue;

		b->buildmeshes = R_DB_Poly;
		b->ent = &r_worldentity;
		b->mesh = NULL;
		b->firstmesh = 0;
		b->meshes = 1;
		b->skin = NULL;
		b->texture = NULL;
		b->shader = shader;
		for (j = 0; j < MAXRLIGHTMAPS; j++)
			b->lightmap[j] = -1;
		b->user.poly.surface = i;
		b->flags = cl_stris[i].flags;
		b->vbo = 0;

		sort = shader->sort;
		if ((b->flags & BEF_FORCEADDITIVE) && sort < SHADER_SORT_ADDITIVE)
			sort = SHADER_SORT_ADDITIVE;
		if ((b->flags & BEF_FORCETRANSPARENT) && SHADER_SORT_PORTAL < sort && sort < SHADER_SORT_BLEND)
			sort = SHADER_SORT_BLEND;
		if ((b->flags & BEF_FORCENODEPTH) && sort < SHADER_SORT_BANNER)
			sort = SHADER_SORT_BANNER;

		b->next = batches[sort];
		batches[sort] = b;
	}
}
void PR_Route_Visualise(void);
void BE_GenModelBatches(batch_t **batches, const dlight_t *dl, unsigned int bemode, const qbyte *worldpvs, const int *worldareas)
{
	int		i;
	entity_t *ent;
	model_t *emodel;
	unsigned int orig_numstris = cl_numstris;
	unsigned int orig_numvisedicts = cl_numvisedicts;
//	unsigned int orig_numstrisidx = cl_numstrisidx;
//	unsigned int orig_numstrisvert = cl_numstrisvert;
	extern cvar_t r_ignoreentpvs; //legacy value is 1...

	if (r_ignoreentpvs.ival)
	{
		worldpvs = NULL;
		worldareas = NULL;
	}

	/*clear the batch list*/
	for (i = 0; i < SHADER_SORT_COUNT; i++)
		batches[i] = NULL;

	if (cl.worldmodel && !(r_refdef.flags & RDF_NOWORLDMODEL))
	{
		if (cl.worldmodel->terrain)
#if defined(TERRAIN)
			Terr_DrawTerrainModel(batches, &r_worldentity);
#endif
		if (cl.worldmodel->type == mod_alias)
		{
			r_worldentity.framestate.g[FS_REG].lerpweight[0] = 1;
			r_worldentity.scale = 1;

			VectorSet(r_worldentity.light_avg,   1.0, 1.0, 1.0);
			VectorSet(r_worldentity.light_range, 0.5, 0.5, 0.5);
			VectorSet(r_worldentity.light_dir,   0.0, 0.196, 0.98);
			r_worldentity.light_known = 1;

			R_GAlias_GenerateBatches(&r_worldentity, batches);
		}
	}

	R_Clutter_Emit(batches);

	if (!r_drawentities.ival)
		return;

	if (bemode == BEM_STANDARD)
	{
#ifndef CLIENTONLY
		SV_AddDebugPolygons();
#endif
#ifdef ENGINE_ROUTING
		PR_Route_Visualise();
#endif

		//the alias cache is a backend thing that provides support for multiple entities using the same skeleton.
		//thus it needs to be cleared so that it won't reuse the cache over multiple frames.
		Alias_FlushCache();
	}

	// draw sprites seperately, because of alpha blending
	for (i=r_refdef.firstvisedict ; i<cl_numvisedicts ; i++)
	{
		ent = &cl_visedicts[i];

		if ((r_refdef.externalview || chase_active.ival) && (ent->flags & RF_FIRSTPERSON))
			continue;
		if (!r_refdef.externalview && (ent->flags & RF_EXTERNALMODEL) && !chase_active.ival)
			continue;

#ifdef RTLIGHTS
		if (bemode == BEM_STENCIL || bemode == BEM_DEPTHONLY)
		{
			if (ent->flags & (RF_NOSHADOW | RF_ADDITIVE | RF_NODEPTHTEST | RF_TRANSLUCENT))	//noshadow often isn't enough for legacy content.
				continue;
			if (ent->flags & RF_EXTERNALMODEL && !r_shadow_playershadows.ival)	//noshadow often isn't enough for legacy content.
				continue;
			if (ent->keynum == dl->key && ent->keynum)	//shadows are not cast from the entity that owns the light. it is expected to be inside.
				continue;
			if (ent->model && ent->model->engineflags & MDLF_FLAME)
				continue;
		}
#endif

		if (worldpvs && !cl.worldmodel->funcs.EdictInFatPVS(cl.worldmodel, &ent->pvscache, worldpvs, worldareas))
			continue;

		switch(ent->rtype)
		{
		case RT_MODEL:
		default:
			emodel = ent->model;
			if (!emodel)
				continue;
			if (emodel->loadstate == MLS_NOTLOADED)
			{
				if (!Mod_LoadModel(emodel, MLV_WARN))
					continue;
			}
			if (emodel->loadstate != MLS_LOADED)
				continue;

			if (cl.lerpents && (cls.allow_anyparticles))	//allowed or static
			{
				if (gl_part_flame.value)
				{
					if (emodel->engineflags & MDLF_EMITREPLACE)
						continue;
				}
			}

			if (emodel->engineflags & MDLF_NOTREPLACEMENTS)
			{
				if (emodel->fromgame != fg_quake || emodel->type != mod_alias)
					if (!ruleset_allow_sensitive_texture_replacements.value)
						continue;
			}

			safeswitch(emodel->type)
			{
			case mod_brush:
				if (r_drawentities.ival == 2 && cls.allow_cheats)	//2 is considered a cheat, because it can be used as a wallhack (whereas mdls are not normally considered as occluding).
					continue;
				if (emodel->lightmaps.maxstyle >= cl_max_lightstyles)
					R_BumpLightstyles(emodel->lightmaps.maxstyle);
				Surf_GenBrushBatches(batches, ent);
				break;
			case mod_alias:
				if (r_drawentities.ival == 3)
					continue;
				R_GAlias_GenerateBatches(ent, batches);
				break;
			case mod_sprite:
				R_Sprite_GenerateTrisoup(ent, bemode);
				break;
			case mod_halflife:
#ifdef HALFLIFEMODELS
				R_HalfLife_GenerateBatches(ent, batches);
#endif
				break;
			case mod_dummy:
			case mod_heightmap:
#if defined(TERRAIN)
				if (emodel->terrain && !(r_refdef.flags & RDF_NOWORLDMODEL))
					Terr_DrawTerrainModel(batches, ent);
#endif
				break;
			safedefault:
				break;
			}
			break;
		case RT_SPRITE:
			R_Sprite_GenerateTrisoup(ent, bemode);
			break;

		case RT_BEAM:
		case RT_RAIL_RINGS:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
#if defined(Q2CLIENT) || defined(Q3CLIENT)
			R_Beam_GenerateTrisoup(ent, bemode);
#endif
			break;

		case RT_POLY:
			/*not implemented*/
			break;
		case RT_PORTALSURFACE:
			/*nothing*/
			break;
		}
	}

	if (cl_numstris && !(r_refdef.flags & RDF_DISABLEPARTICLES))
		BE_GenPolyBatches(batches);

	while(orig_numstris < cl_numstris)
		cl_stris[orig_numstris++].shader = NULL;
	cl_numstris = orig_numstris;
/*	cl_numstrisidx = orig_numstrisidx;
	cl_numstrisvert = orig_numstrisvert;
	if (cl_numstris)
	{	//fix this up, in case they got merged.
		cl_stris[cl_numstris-1].numvert = cl_numstrisvert - cl_stris[cl_numstris-1].firstvert;
		cl_stris[cl_numstris-1].numidx = cl_numstrisidx - cl_stris[cl_numstris-1].firstidx;
	}
*/	cl_numvisedicts = orig_numvisedicts;
}

#endif	// defined(GLQUAKE)
