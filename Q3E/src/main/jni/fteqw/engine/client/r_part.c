/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

#define NUMVERTEXNORMALS	162
float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};


#include "shader.h"
#include "com_mesh.h"
//FIXME: we're likely going to want to thread the building routine at some point.
//the alias mesh stuff will need some rework as it uses statics inside.
#define DESCSPERSHADER 8
typedef struct
{
	int x, y, z;	//rebuilt if changed
	int key;

	model_t *loadingmodel;	//needs rebuilding, but wait till this is loaded.

	struct
	{
		shader_t *shader;
		mesh_t mesh;
		mesh_t *pmesh;
		vbo_t vbo;
	} soups[64];
	size_t numsoups;
} cluttersector_t;
static cluttersector_t cluttersector[3*3*3];
cvar_t r_clutter_density		= CVARD("r_clutter_density", "0", "Scaler for clutter counts. 0 disables clutter completely.\nClutter requires shaders with 'fte_clutter MODEL SPACING SCALEMIN SCALEMAX ZOFS ANGLEMIN ANGLEMAX' terms");
cvar_t r_clutter_distance		= CVARD("r_clutter_distance", "1024", "Distance at which clutter will become invisible.");	//should be used by various shaders to fade it out by here
void R_Clutter_Init(void)
{
	Cvar_Register(&r_clutter_density, "Ground Clutter");
	Cvar_Register(&r_clutter_distance, "Ground Clutter");
}
typedef struct
{
	model_t *loadingmodel;
	struct clutter_build_ctx_soup_s
	{
		shader_t *shader;
		vecV_t *coord;
		vec2_t *texcoord;
		vec4_t *colour;
		vec3_t *normal;
		vec3_t *sdir;
		vec3_t *tdir;
		index_t *idx;
		size_t numverts;
		size_t numidx;
		size_t maxverts;
		size_t maxidx;
	} soups[64];
	unsigned int numsoups;
	float area[DESCSPERSHADER];	//here so it can overflow, so large values with small surfaces actually does something. not evenly perhaps, but not much else we can do

	unsigned int x, y, z, w;
} clutter_build_ctx_t;

//to make things repeatable so that people can depend upon placement.
unsigned int R_Clutter_Random(clutter_build_ctx_t *ctx)
{	//ripped from wikipedia (originally called xorshift128)
	unsigned int t = ctx->x ^ (ctx->x << 11);
	ctx->x = ctx->y; ctx->y = ctx->z; ctx->z = ctx->w;
	return ctx->w = ctx->w ^ (ctx->w >> 19) ^ t ^ (t >> 8);
}
float R_Clutter_FRandom(clutter_build_ctx_t *ctx)
{
	unsigned int r = R_Clutter_Random(ctx);
	return (r & 0xffffff) / (float)0xffffff;
}

static void R_Clutter_Insert_Soup(clutter_build_ctx_t *ctx, shader_t *shader, vecV_t *fte_restrict coord, vec2_t *fte_restrict texcoord, vec3_t *fte_restrict normal, vec3_t *fte_restrict sdir, vec3_t *fte_restrict tdir, vec4_t *fte_restrict colours, size_t numverts, index_t *fte_restrict index, size_t numidx, float scale, vec3_t origin, vec3_t axis[])
{
	vec3_t diffuse, ambient, ldir;
	float dot;
	struct clutter_build_ctx_soup_s *soup = NULL;
	size_t i;

	/* useful for debugging mayhaps --eukara */
	/*shader_t *os = shader;
	shader = R_RegisterShader(va("clutter#replace=%s", os->name), SUF_NONE,
			"{\n"
				"program defaultsprite#MASK=0.666\n"
//				"surfaceparm nodlight\n"
				"surfaceparm noshadows\n"
//				"cull disable\n"
				"{\n"
					"map $diffuse\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
//					"alphafunc ge128\n"
				"}\n"
			"}\n"
		);
	*shader->defaulttextures = *os->defaulttextures;*/


	for (i = 0, soup = ctx->soups; i < ctx->numsoups; i++, soup++)
	{
		if (soup->shader == shader)
			if (soup->numverts + numverts <= MAX_INDICIES)
				break;
	}
	if (i == ctx->numsoups)
	{
		if (i == sizeof(ctx->soups)/sizeof(ctx->soups[0]))
			return;	//too many different shaders or something
		soup->shader = shader;
		ctx->numsoups++;
	}

	//inject the indicies
	if (soup->numidx + numidx > soup->maxidx)
	{
		soup->maxidx = (soup->numidx + numidx) * 2;
		soup->idx = BZ_Realloc(soup->idx, sizeof(*soup->idx) * soup->maxidx);
	}
	for (i = 0; i < numidx; i++)
		soup->idx[soup->numidx++] = soup->numverts+*index++;


	cl.worldmodel->funcs.LightPointValues(cl.worldmodel, origin, diffuse, ambient, ldir);
	VectorScale(ambient, 1/255.0, ambient);
	VectorScale(diffuse, 1/255.0, diffuse);

	//inject the verts
	if (soup->numverts + numverts > soup->maxverts)
	{
		soup->maxverts = (soup->numverts + numverts) * 2;
		soup->coord = BZ_Realloc(soup->coord, sizeof(*soup->coord) * soup->maxverts);
		soup->texcoord = BZ_Realloc(soup->texcoord, sizeof(*soup->texcoord) * soup->maxverts);
		soup->colour = BZ_Realloc(soup->colour, sizeof(*soup->colour) * soup->maxverts);
		soup->normal = BZ_Realloc(soup->normal, sizeof(*soup->normal) * soup->maxverts);
		soup->sdir = BZ_Realloc(soup->sdir, sizeof(*soup->sdir) * soup->maxverts);
		soup->tdir = BZ_Realloc(soup->tdir, sizeof(*soup->tdir) * soup->maxverts);
	}
	for (i = 0; i < numverts; i++)
	{
		VectorMA(origin,						scale*coord[i][0], axis[0], soup->coord[soup->numverts]);
		VectorMA(soup->coord[soup->numverts],	scale*coord[i][1], axis[1], soup->coord[soup->numverts]);
		VectorMA(soup->coord[soup->numverts],	scale*coord[i][2], axis[2], soup->coord[soup->numverts]);
		Vector2Copy(texcoord[i], soup->texcoord[soup->numverts]);

		VectorMA(vec3_origin,					normal[i][0], axis[0], soup->normal[soup->numverts]);
		VectorMA(soup->normal[soup->numverts],	normal[i][1], axis[1], soup->normal[soup->numverts]);
		VectorMA(soup->normal[soup->numverts],	normal[i][2], axis[2], soup->normal[soup->numverts]);
		
		VectorMA(vec3_origin,					sdir[i][0], axis[0], soup->sdir[soup->numverts]);
		VectorMA(soup->sdir[soup->numverts],	sdir[i][1], axis[1], soup->sdir[soup->numverts]);
		VectorMA(soup->sdir[soup->numverts],	sdir[i][2], axis[2], soup->sdir[soup->numverts]);

		VectorMA(vec3_origin,					tdir[i][0], axis[0], soup->tdir[soup->numverts]);
		VectorMA(soup->tdir[soup->numverts],	tdir[i][1], axis[1], soup->tdir[soup->numverts]);
		VectorMA(soup->tdir[soup->numverts],	tdir[i][2], axis[2], soup->tdir[soup->numverts]);

//		VectorCopy(ambient, soup->colour[soup->numverts]);
		dot = DotProduct(ldir, soup->normal[soup->numverts]);
		if (dot < 0)
			dot = 0;
		VectorMA(ambient, dot, diffuse, soup->colour[soup->numverts]);
		if (colours)	//most model formats don't have vertex colours
			soup->colour[soup->numverts][3] = colours[i][3];
		else
			soup->colour[soup->numverts][3] = 1;

		soup->numverts++;
	}
}
static void R_Clutter_Insert_Mesh(clutter_build_ctx_t *ctx, model_t *mod, float scale, vec3_t origin, vec3_t axis[3])
{
	mesh_t mesh;
	galiasinfo_t *inf;
	unsigned int surfnum = 0;
	entity_t re;
	unsigned int randanim = R_Clutter_Random(ctx);
	unsigned int randskin = R_Clutter_Random(ctx);

	if (!mod)
		return;

	if (mod->type == mod_alias)
	{
		//fill in the parts of the entity_t that Alias_GAliasBuildMesh needs.
		memset(&re, 0, sizeof(re));
		re.framestate.g[FS_REG].lerpweight[0] = 1;
		re.model = mod;

		inf = (galiasinfo_t*)Mod_Extradata (mod);
		while(inf)
		{
			galiasskin_t *skins = inf->ofsskins;
			if (inf->numanimations >= 1)
				re.framestate.g[FS_REG].frame[0] = randanim%inf->numanimations;
			else
				re.framestate.g[FS_REG].frame[0] = 0;
			if (skins->numframes)
			{
				unsigned int frame = randskin%skins->numframes;
				Alias_GAliasBuildMesh(&mesh, NULL, inf, surfnum, &re, false);
				surfnum++;
				//fixme: if shares verts, rewind the verts and don't add more somehow, while being careful with shaders
				R_Clutter_Insert_Soup(ctx, skins->frame[frame].shader, mesh.xyz_array, mesh.st_array, mesh.normals_array, mesh.snormals_array, mesh.tnormals_array, mesh.colors4f_array[0], mesh.numvertexes, mesh.indexes, mesh.numindexes, scale, origin, axis);
			}
			inf = inf->nextsurf;
		}
		Alias_FlushCache();	//it got built using an entity on the stack, make sure other stuff doesn't get hurt.
	}
}
static void R_Clutter_Insert(void *vctx, vec3_t *fte_restrict points, size_t numtris, shader_t *surface)
{
	struct shader_clutter_s *clut;
	unsigned int obj;
	clutter_build_ctx_t *ctx = vctx;
	model_t *mod[DESCSPERSHADER];
	if (!surface || !surface->clutter)
		return;	//nothing to do.

	//avoid returning on error, so the randomization is dependable when content is still loading.
	for (clut = surface->clutter, obj = 0; clut && obj <= DESCSPERSHADER; clut = clut->next, obj++)
	{
		mod[obj] = Mod_ForName(clut->modelname, MLV_WARN);
		if (mod[obj]->loadstate == MLS_LOADING)
		{
			if (!ctx->loadingmodel)
				ctx->loadingmodel = mod[obj];
			mod[obj] = NULL;
		}
		else if (mod[obj]->type != mod_alias)
			mod[obj] = NULL;
	}

	while(numtris-->0)
	{
		vec3_t xd;
		vec3_t yd;
		vec3_t zd;
		vec3_t norm;
		vec3_t axis[3];
		vec3_t org, dir;
		float dot;
		float triarea;
//		vec3_t discard;
//		unsigned int subimage;
		vec_t xm, ym, zm, s;
		VectorSubtract(points[1], points[0], xd);
		VectorSubtract(points[2], points[0], yd);
		VectorSubtract(points[2], points[1], zd);
		CrossProduct(yd, xd, norm);
		VectorNormalize(norm);
		if (norm[2] >= 0.7)
		{
			//determine area of triangle
			xm = Length(xd);
			ym = Length(yd);
			zm = Length(zd);
			s = (xm+ym+zm)/2;
			triarea = sqrt(s*(s-xm)*(s-ym)*(s-zm));

			for (clut = surface->clutter, obj = 0; clut && obj <= DESCSPERSHADER; clut = clut->next, obj++)
			{
				float spacing = clut->spacing / r_clutter_density.value;
				if (spacing < 1)
					spacing = 1;
				ctx->area[obj] += triarea;
				while (ctx->area[obj] >= spacing)
				{
					float scale = clut->scalemin + R_Clutter_FRandom(ctx) * (clut->scalemax-clut->scalemin);
					ctx->area[obj] -= spacing;

					//pick a random spot
					xm = R_Clutter_FRandom(ctx)*R_Clutter_FRandom(ctx);
					ym = R_Clutter_FRandom(ctx) * (1-xm);
					VectorMA(points[0], xm, xd, org);
					VectorMA(org, ym, yd, org);

					//randomize the direction
					dot = clut->anglemin + R_Clutter_FRandom(ctx) * (clut->anglemax-clut->anglemin);
					dir[0] = cos(dot);
					dir[1] = sin(dot);
					dir[2] = 0;
					//figure out various directions
					dot = -DotProduct(dir, norm);
					VectorMA(dir, dot, norm, dir);
					VectorNormalize(dir);
					VectorCopy(norm, axis[2]);
					CrossProduct(axis[2], dir, axis[1]);
					CrossProduct(axis[1], axis[2], axis[0]);
					VectorMA(org, clut->zofs*scale, axis[2], org);
					R_Clutter_Insert_Mesh(ctx, mod[obj], scale, org, axis);

/*
					VectorMA(org, r_clutter_size.value/2, dir, vertcoord[numverts]);
					VectorMA(org, -(r_clutter_size.value/2), dir, vertcoord[numverts+1]);
					VectorMA(vertcoord[numverts], r_clutter_height.value, norm, vertcoord[numverts+2]);
					VectorMA(vertcoord[numverts+1], r_clutter_height.value, norm, vertcoord[numverts+3]);
					subimage = R_Clutter_Random(ctx);
					Vector2Set(texcoord[numverts], subimage%r_clutter_atlaswidth.ival, (subimage/r_clutter_atlaswidth.ival)%r_clutter_atlasheight.ival);
					texcoord[numverts][0] *= 1/r_clutter_atlaswidth.value;
					texcoord[numverts][1] *= 1/r_clutter_atlasheight.value;
					Vector2Set(texcoord[numverts+1], texcoord[numverts][0]+(1/r_clutter_atlaswidth.value), texcoord[numverts][1]);
					Vector2Set(texcoord[numverts+2], texcoord[numverts][0]								 , texcoord[numverts][1]);
					Vector2Set(texcoord[numverts+3], texcoord[numverts][0]+(1/r_clutter_atlaswidth.value), texcoord[numverts][1]);
					texcoord[numverts+0][1]+=(1/r_clutter_atlasheight.value);
					texcoord[numverts+1][1]+=(1/r_clutter_atlasheight.value);
					Vector4Set(colours[numverts+0], 1, 1, 1, 1);
					VectorMA(org, 1/8.0, norm, org);//push away from the surface to avoid precision issues with lighting on slopes
					cl.worldmodel->funcs.LightPointValues(cl.worldmodel, org, colours[numverts+0], discard, discard);
					VectorScale(colours[numverts+0], 1/512.0, colours[numverts+0]);
					Vector4Copy(colours[numverts+0], colours[numverts+1]);
					Vector4Copy(colours[numverts+0], colours[numverts+2]);
					Vector4Copy(colours[numverts+1], colours[numverts+3]);
					indexes[numidx+0] = numverts+0;
					indexes[numidx+1] = numverts+2;
					indexes[numidx+2] = numverts+1;
					indexes[numidx+3] = numverts+2;
					indexes[numidx+4] = numverts+3;
					indexes[numidx+5] = numverts+1;
					numverts += 4;
					numidx += 6;
*/
				}
			}
		}
		points += 3;
	}
}
void R_Clutter_Emit(batch_t **batches)
{
	const float cluttersize = r_clutter_distance.value;
	int vx, vy, vz;
	int x, y, z, key, i, j;
	cluttersector_t *sect;
	batch_t *b;
	qboolean rebuildlimit = false;

	if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED || r_clutter_density.value <= 0 || (r_refdef.flags & RDF_NOWORLDMODEL))
		return;

	if (qrenderer != QR_OPENGL && qrenderer != QR_VULKAN)	//vbo only!
		return;

	//rebuild if any of the cvars changes.
	key =	r_clutter_density.modified + r_clutter_distance.modified;

	vx = floor((r_refdef.vieworg[0] / cluttersize));
	vy = floor((r_refdef.vieworg[1] / cluttersize));
	vz = floor((r_refdef.vieworg[2] / cluttersize));

	for (z = vz-1; z <= vz+1; z++)
	for (y = vy-1; y <= vy+1; y++)
	for (x = vx-1; x <= vx+1; x++)
	{
		int ix = x%3;
		int iy = y%3;
		int iz = z%3;
		if (ix < 0)
			ix += 3;
		if (iy < 0)
			iy += 3;
		if (iz < 0)
			iz += 3;
		sect = &cluttersector[ix + (iy*3) + (iz*3*3)];
		if (sect->loadingmodel && sect->loadingmodel->loadstate != MLS_LOADING)
		{
			sect->loadingmodel = NULL;
			sect->key-=1;	//rebuild even if failed, this covers multiple models.
		}
		if (sect->x != x || sect->y != y || sect->z != z || sect->key != key)
		{
			vbobctx_t vctx;
			clutter_build_ctx_t cctx;
			vec3_t org = {x*cluttersize+(cluttersize/2),y*cluttersize+(cluttersize/2),z*cluttersize+(cluttersize/2)};
			vec3_t down = {0, 0, -1};
			vec3_t forward = {1, 0, 0};
			vec3_t right = {0, 1, 0};
			if (r_refdef.recurse)	//FIXME
				continue;
			if (rebuildlimit)
				continue;
			rebuildlimit = true;
			sect->x = x;
			sect->y = y;
			sect->z = z;
			sect->key = key;

			//make sure any old state is gone
			for (i = 0; i < sect->numsoups; i++)
			{
				BE_VBO_Destroy(&sect->soups[i].vbo.coord, sect->soups[i].vbo.vbomem);
				BE_VBO_Destroy(&sect->soups[i].vbo.indicies, sect->soups[i].vbo.ebomem);
			}
			sect->numsoups = 0;
			memset(&cctx, 0, sizeof(cctx));
			cctx.x = x;
			cctx.y = y;
			cctx.z = z;
			cctx.w = (sect-cluttersector)+1;
			Mod_ClipDecal(cl.worldmodel, org, down, forward, right, cluttersize, 0, 0, R_Clutter_Insert, &cctx);
			sect->loadingmodel = cctx.loadingmodel;

			for (i = 0; i < cctx.numsoups; i++)
			{
				if (cctx.soups[i].numverts)
				{
					sect->soups[sect->numsoups].shader = cctx.soups[i].shader;
					sect->soups[sect->numsoups].pmesh = &sect->soups[sect->numsoups].mesh;
			
					BE_VBO_Begin(&vctx, (sizeof(cctx.soups[i].coord[0]) + sizeof(cctx.soups[i].texcoord[0]) + sizeof(cctx.soups[i].colour[0]) + 3*sizeof(vec3_t))*cctx.soups[i].numverts);
					BE_VBO_Data(&vctx, cctx.soups[i].coord, sizeof(cctx.soups[i].coord[0])*cctx.soups[i].numverts, &sect->soups[sect->numsoups].vbo.coord);
					BE_VBO_Data(&vctx, cctx.soups[i].texcoord, sizeof(cctx.soups[i].texcoord[0])*cctx.soups[i].numverts, &sect->soups[sect->numsoups].vbo.texcoord);
					BE_VBO_Data(&vctx, cctx.soups[i].colour, sizeof(cctx.soups[i].colour[0])*cctx.soups[i].numverts, &sect->soups[sect->numsoups].vbo.colours[0]);
					BE_VBO_Data(&vctx, cctx.soups[i].normal, sizeof(cctx.soups[i].normal[0])*cctx.soups[i].numverts, &sect->soups[sect->numsoups].vbo.normals);
					BE_VBO_Data(&vctx, cctx.soups[i].sdir, sizeof(cctx.soups[i].sdir[0])*cctx.soups[i].numverts, &sect->soups[sect->numsoups].vbo.svector);
					BE_VBO_Data(&vctx, cctx.soups[i].tdir, sizeof(cctx.soups[i].tdir[0])*cctx.soups[i].numverts, &sect->soups[sect->numsoups].vbo.tvector);
					BE_VBO_Finish(&vctx, cctx.soups[i].idx, sizeof(cctx.soups[i].idx[0])*cctx.soups[i].numidx, &sect->soups[sect->numsoups].vbo.indicies, &sect->soups[sect->numsoups].vbo.vbomem, &sect->soups[sect->numsoups].vbo.ebomem);
					sect->soups[sect->numsoups].vbo.colours_bytes = false;

					sect->soups[sect->numsoups].mesh.numindexes = sect->soups[sect->numsoups].vbo.indexcount = cctx.soups[i].numidx;
					sect->soups[sect->numsoups].mesh.numvertexes = sect->soups[sect->numsoups].vbo.vertcount = cctx.soups[i].numverts;
					sect->numsoups++;
				}
				BZ_Free(cctx.soups[i].coord);
				BZ_Free(cctx.soups[i].texcoord);
				BZ_Free(cctx.soups[i].colour);
				BZ_Free(cctx.soups[i].normal);
				BZ_Free(cctx.soups[i].sdir);
				BZ_Free(cctx.soups[i].tdir);
				BZ_Free(cctx.soups[i].idx);
			}
		}

		//emit a batch if we have grassy surfaces in this block
		for (i = 0; i < sect->numsoups; i++)
		{
			b = BE_GetTempBatch();
			if (!b)
				return;
			memset(b, 0, sizeof(*b));
			for (j = 0; j < MAXRLIGHTMAPS; j++)
				b->lightmap[j] = -1;
			b->ent = &r_worldentity;
			b->meshes = 1;
			b->mesh = &sect->soups[i].pmesh;
			b->vbo = &sect->soups[i].vbo;
			b->shader = sect->soups[i].shader;
			b->next = batches[b->shader->sort];
			batches[b->shader->sort] = b;
		}
	}
}

void R_Clutter_Purge(void)
{
	size_t i, j;
	cluttersector_t *sect;
	if (!qrenderer)
		return;
	for (i = 0; i < sizeof(cluttersector)/sizeof(cluttersector[0]); i++)
	{
		sect = &cluttersector[i];
		for (j = 0; j < sect->numsoups; j++)
		{
			BE_VBO_Destroy(&sect->soups[j].vbo.coord, sect->soups[j].vbo.vbomem);
			BE_VBO_Destroy(&sect->soups[j].vbo.indicies, sect->soups[j].vbo.ebomem);
		}
		memset(sect, 0, sizeof(*sect));
	}
}




static void QDECL R_Rockettrail_Callback(struct cvar_s *var, char *oldvalue)
{
	int i;
	model_t *mod;
	extern model_t	*mod_known;
	extern int		mod_numknown;

	if (cls.state == ca_disconnected)
		return; // don't bother parsing while disconnected

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate == MLS_LOADED)
			if (mod->flags & MF_ROCKET)
				P_LoadedModel(mod);
	}
}

static void QDECL R_Grenadetrail_Callback(struct cvar_s *var, char *oldvalue)
{
	int i;
	model_t *mod;
	extern model_t	*mod_known;
	extern int		mod_numknown;

	if (cls.state == ca_disconnected)
		return; // don't bother parsing while disconnected

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate == MLS_LOADED)
			if (mod->flags & MF_GRENADE)
				P_LoadedModel(mod);
	}
}

extern particleengine_t pe_null;
#ifdef PSET_CLASSIC
extern particleengine_t pe_classic;
#endif
particleengine_t pe_darkplaces;
particleengine_t pe_qmb;
#ifdef PSET_SCRIPT
extern particleengine_t pe_script;
#endif

particleengine_t *particlesystem[] =
{
#ifdef PSET_SCRIPT
	&pe_script,
#endif
	&pe_darkplaces,
	&pe_qmb,
#ifdef PSET_CLASSIC
	&pe_classic,
#endif
	&pe_null,
	NULL,
};

static void QDECL R_ParticleSystem_Callback(struct cvar_s *var, char *oldvalue)
{
	int i;
	if (pe)
	{
		CL_ClearTEntParticleState();
		CL_ClearLerpEntsParticleState();
#ifdef Q2CLIENT
		CLQ2_ClearParticleState();
#endif

		pe->ShutdownParticles();
	}

	if (!qrenderer)
	{
		pe = &pe_null;
	}
	else
	{
		pe = NULL;
		for (i = 0; particlesystem[i]; i++)
		{
			if (   (particlesystem[i]->name1 && !stricmp(var->string, particlesystem[i]->name1))
				|| (particlesystem[i]->name2 && !stricmp(var->string, particlesystem[i]->name2)))
			{
				pe = particlesystem[i];
				break;
			}
			if (!pe)
				if (particlesystem[i]->name1)
					pe = particlesystem[i];
		}
	}
	if (!pe)
		Sys_Error("No particle system available. Please recompile.");

	if (!pe->InitParticles())
	{
		Con_Printf("Particlesystem %s failed to init\n", pe->name1);
		pe = &pe_null;
		pe->InitParticles();
	}
	pe->ClearParticles();
	CL_RegisterParticles();
}

cvar_t r_decal_noperpendicular = CVARD("r_decal_noperpendicular", "1", "When enabled, decals will not be generated on planes at a steep angle from clipped decal orientation.");
cvar_t r_rockettrail = CVARFC("r_rockettrail", "1", CVAR_SEMICHEAT, R_Rockettrail_Callback);
cvar_t r_grenadetrail = CVARFC("r_grenadetrail", "1", CVAR_SEMICHEAT, R_Grenadetrail_Callback);
#ifndef PSET_CLASSIC
cvar_t r_particlesystem	= CVARFC("r_particlesystem",	"script",						CVAR_SEMICHEAT|CVAR_ARCHIVE|CVAR_NOSET, R_ParticleSystem_Callback);
cvar_t r_particledesc	= CVARAF("r_particledesc",		"",			"r_particlesdesc",	CVAR_SEMICHEAT|CVAR_ARCHIVE);
#else
cvar_t r_particlesystem = CVARFC("r_particlesystem",	IFMINIMAL("classic", "script"), CVAR_SEMICHEAT|CVAR_ARCHIVE, R_ParticleSystem_Callback);
cvar_t r_particledesc = CVARAF("r_particledesc",		"classic",	"r_particlesdesc", CVAR_SEMICHEAT|CVAR_ARCHIVE);
#endif
extern cvar_t r_bouncysparks;
extern cvar_t r_part_rain;
extern cvar_t r_bloodstains;
extern cvar_t gl_part_flame;
cvar_t r_part_rain_quantity = CVARF("r_part_rain_quantity", "1", CVAR_ARCHIVE);

cvar_t r_particle_tracelimit = CVARFD("r_particle_tracelimit", "0x7fffffff", CVAR_ARCHIVE, "Number of traces to allow per frame for particle physics.");
cvar_t r_part_sparks = CVAR("r_part_sparks", "1");
cvar_t r_part_sparks_trifan = CVAR("r_part_sparks_trifan", "1");
cvar_t r_part_sparks_textured = CVAR("r_part_sparks_textured", "1");
cvar_t r_part_beams = CVAR("r_part_beams", "1");
cvar_t r_part_contentswitch = CVARFD("r_part_contentswitch", "1", CVAR_ARCHIVE, "Enable particle effects to change based on content (ex. water).");
cvar_t r_part_density = CVARF("r_part_density", "1", CVAR_ARCHIVE);
cvar_t r_part_classic_expgrav = CVARFD("r_part_classic_expgrav", "10", CVAR_ARCHIVE, "Scaler for how fast classic explosion particles should accelerate due to gravity. 1 for like vanilla, 10 for like zquake.");
cvar_t r_part_classic_opaque = CVARFD("r_part_classic_opaque", "0", CVAR_ARCHIVE, "Disables transparency on classic particles, for the oldskool look.");
cvar_t r_part_classic_square = CVARFD("r_part_classic_square", "0", CVAR_ARCHIVE, "Enables square particles, for the oldskool look.");

cvar_t r_part_maxparticles = CVAR("r_part_maxparticles", "65536");
cvar_t r_part_maxdecals = CVAR("r_part_maxdecals", "8192");


particleengine_t *pe;

static struct partalias_s
{
	struct partalias_s *next;
	const char *from;
	const char *to;
} *partaliaslist;

void P_ParticleEffect_f(void);
static void P_ParticleEffectAlias_f(void);

void P_InitParticleSystem(void)
{
	char *particlecvargroupname = "Particle effects";

	Cvar_Register(&r_decal_noperpendicular, particlecvargroupname);	//decals might actually be used for more than just particles, but oh well.

	Cvar_Register(&r_particlesystem, particlecvargroupname);

	//particles
	Cvar_Register(&r_particledesc, particlecvargroupname);
	Cvar_Register(&r_bouncysparks, particlecvargroupname);
	Cvar_Register(&r_part_rain, particlecvargroupname);

	Cvar_Register(&r_part_rain_quantity, particlecvargroupname);

	Cvar_Register(&r_particle_tracelimit, particlecvargroupname);

	Cvar_Register(&r_part_maxparticles, particlecvargroupname);
	Cvar_Register(&r_part_maxdecals, particlecvargroupname);

	Cvar_Register(&r_part_sparks, particlecvargroupname);
	Cvar_Register(&r_part_sparks_trifan, particlecvargroupname);
	Cvar_Register(&r_part_sparks_textured, particlecvargroupname);
	Cvar_Register(&r_part_beams, particlecvargroupname);
	Cvar_Register(&r_part_contentswitch, particlecvargroupname);
	Cvar_Register(&r_part_density, particlecvargroupname);
	Cvar_Register(&r_part_classic_expgrav, particlecvargroupname);
	Cvar_Register(&r_part_classic_opaque, particlecvargroupname);
	Cvar_Register(&r_part_classic_square, particlecvargroupname);

	Cvar_Register (&gl_part_flame, particlecvargroupname);

	Cvar_Register (&r_rockettrail, particlecvargroupname);
	Cvar_Register (&r_grenadetrail, particlecvargroupname);

	//always registered to suck up stray r_part commands even when the scripted system is not active.
#ifdef PSET_SCRIPT
	Cmd_AddCommand("r_part", P_ParticleEffect_f);
#endif
	Cmd_AddCommand("r_partredirect", P_ParticleEffectAlias_f);

	R_Clutter_Init();
}

void P_ShutdownParticleSystem(void)
{
	struct partalias_s *l;

	while (partaliaslist)
	{
		l = partaliaslist;
		partaliaslist = l->next;
		Z_Free(l);
	}
}

static void P_ParticleEffectAlias_f(void)
{
	struct partalias_s **link, *l;
	char *from = Cmd_Argv(1);
	char *to = Cmd_Argv(2);

	//user wants to list all
	if (!*from)
	{
		for (l = partaliaslist; l; l = l->next)
		{
			Con_Printf("%s -> %s\n", l->from, l->to);
		}
		return;
	}

	//unlink the current value
	for (link = &partaliaslist; (l=*link); link = &(*link)->next)
	{
		if (!Q_strcasecmp(l->from, from))
		{
			//they didn't specify a to, so just print out this one effect without removing it.
			if (Cmd_Argc() == 2)
			{
				Con_Printf("particle %s is currently remapped to %s\n", l->from, l->to);
				return;
			}
			*link = l->next;
			Z_Free(l);
			break;
		}
	}

	//create a new entry.
	if (*to && Q_strcasecmp(from, to))
	{
		l = Z_Malloc(sizeof(*l) + strlen(from) + strlen(to) + 2);
		l->from = (char*)(l + 1);
		strcpy((char*)l->from, from);
		l->to = l->from + strlen(l->from)+1;
		strcpy((char*)l->to, to);
		l->next = partaliaslist;
		partaliaslist = l;
	}

	CL_RegisterParticles();
}
int P_FindParticleType(const char *efname)
{
	struct partalias_s *l;
	int recurselimit = 5;
	if (!pe)
		return P_INVALID;
	for (l = partaliaslist; l; )
	{
		if (!Q_strcasecmp(l->from, efname))
		{
			efname = l->to;

			if (recurselimit --> 0)
				l = partaliaslist;
			else
				return P_INVALID;
		}
		else
			l = l->next;
	}

	return pe->FindParticleType(efname);
}

void P_Shutdown(void)
{
	if (pe)
	{
		CL_ClearTEntParticleState();
		CL_ClearLerpEntsParticleState();
#ifdef Q2CLIENT
		CLQ2_ClearParticleState();
#endif
		pe->ShutdownParticles();
	}
	pe = NULL;

	R_Clutter_Purge();
}

//traces against renderable entities
//0 says hit nothing.
//1 says hit world
//>1 says hit some entity
entity_t *TraceLineR (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal, qboolean bsponly)
{
	trace_t		trace;
	float len, bestlen;
	int i, j;
	vec3_t delta, ts, te;
	entity_t *pe;
	entity_t *result=NULL;
//	vec3_t axis[3];
	vec3_t movemins, movemaxs;

	memset (&trace, 0, sizeof(trace));

	VectorSubtract(end, start, delta);
	bestlen = Length(delta);

	VectorCopy (end, impact);

	for (i = 0; i < 3; i++)
	{
		if (start[i] > end[i])
		{
			movemins[i] = end[i];
			movemaxs[i] = start[i];
		}
		else
		{
			movemins[i] = start[i];
			movemaxs[i] = end[i];
		}
	}

	for (i=-1 ; i<cl_numvisedicts ; i++)
	{
		if (i == -1)
			pe = &r_worldentity;
		else
			pe = &cl_visedicts[i];
		if (pe->rtype != RT_MODEL || !pe->model || (pe->shaderRGBAf[3] < 1&&(pe->flags&RF_TRANSLUCENT)) || (pe->flags & (RF_ADDITIVE|RF_NODEPTHTEST|RF_TRANSLUCENT|RF_EXTERNALMODEL)))
			continue;
		if (bsponly && pe->model->type != mod_brush)
			continue;
		if (pe->model->funcs.NativeTrace && pe->model->loadstate == MLS_LOADED)
		{
			//try to trivially reject the mesh.
			float ext = 0;
			float t;
			for (j = 0; j < 3; j++)
			{
				t = fabs(pe->model->maxs[j]);
				ext = max(ext, t);
				t = fabs(pe->model->mins[j]);
				ext = max(ext, t);
			}
			if (	movemins[0] > pe->origin[0]+ext
				||	movemins[1] > pe->origin[1]+ext
				||	movemins[2] > pe->origin[2]+ext
				||	movemaxs[0] < pe->origin[0]-ext
				||	movemaxs[1] < pe->origin[1]-ext
				||	movemaxs[2] < pe->origin[2]-ext )
				continue;

			VectorSubtract(start, pe->origin, ts);
			VectorSubtract(end, pe->origin, te);
			pe->model->funcs.NativeTrace(pe->model, 0, &pe->framestate, pe->axis, ts, te, vec3_origin, vec3_origin, false, MASK_WORLDSOLID, &trace);
			if (trace.fraction<1)
			{
				VectorSubtract(trace.endpos, ts, delta);
				len = DotProduct(delta,delta);
				if (len < bestlen)
				{
					bestlen = len;
					if (normal)
						VectorCopy (trace.plane.normal, normal);
					VectorAdd (pe->origin, trace.endpos, impact);
				}

				result = pe;
			}
			/*if (trace.startsolid)
			{
				VectorNormalize(delta);
				if (normal)
				{
					normal[0] = -delta[0];
					normal[1] = -delta[1];
					normal[2] = -delta[2];
				}
				VectorCopy (end, impact);
				return NULL;
			}*/
		}
	}

	return result;
}

#include "pr_common.h"
//traces against networked entities only.
float CL_TraceLine (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal, int *ent)
{
	trace_t		trace;
#ifdef CSQC_DAT
	extern world_t csqc_world;
	if (csqc_world.progs)
	{
		trace = World_Move(&csqc_world, start, vec3_origin, vec3_origin, end, MOVE_NOMONSTERS, csqc_world.edicts);
		VectorCopy(trace.endpos, impact);
		if (normal)
			VectorCopy(trace.plane.normal, normal);
		if (ent)
		{
			if (trace.entnum)	//ssqc numbers...
				*ent = trace.entnum; //aka: trace_networkentity
			else if (trace.ent)	//csqc numbers are negative...
				*ent = -((wedict_t*)trace.ent)->entnum;
			//else makestatic, though those are assumed non-solid so won't be returned anyway.
			else
				*ent = 0;
		}
		return trace.fraction;
	}
	else
#endif
	{
		float bestfrac=1;
		int i;
		vec3_t ts, te;
		physent_t *pe;
		model_t *mod;
		int result=0;
		vec3_t axis[3];

		memset (&trace, 0, sizeof(trace));

		VectorCopy (end, impact);
		if (normal)
			VectorClear(normal);

		for (i=0 ; i < pmove.numphysent ; i++)
		{
			pe = &pmove.physents[i];
			if (pe->nonsolid)
				continue;
			mod = pe->model;
			if (mod && mod->loadstate == MLS_LOADED && mod->funcs.NativeTrace)
			{
				VectorSubtract(start, pe->origin, ts);
				VectorSubtract(end, pe->origin, te);
				if (pe->angles[0] || pe->angles[1] || pe->angles[2])
				{
					AngleVectors(pe->angles, axis[0], axis[1], axis[2]);
					VectorNegate(axis[1], axis[1]);
					mod->funcs.NativeTrace(mod, 0, PE_FRAMESTATE, axis, ts, te, vec3_origin, vec3_origin, false, MASK_WORLDSOLID, &trace);
				}
				else
					mod->funcs.NativeTrace(mod, 0, PE_FRAMESTATE, NULL, ts, te, vec3_origin, vec3_origin, false, MASK_WORLDSOLID, &trace);
				if (trace.fraction<1)
				{
					if (bestfrac > trace.fraction)
					{
						bestfrac = trace.fraction;
						if (normal)
							VectorCopy (trace.plane.normal, normal);
						VectorAdd (pe->origin, trace.endpos, impact);
						result = pe->info;
					}
				}
				if (trace.startsolid)
				{
					if (normal)
					{
						VectorSubtract(start, end, normal);
						VectorNormalize(normal);
					}
					VectorCopy (end, impact);

					//hit nothing
					if (ent)
						*ent = 0;
					return 1;
				}

			}
		}

		if (ent)
			*ent = result;
		return bestfrac;
	}
}

//handy utility...
void P_EmitEffect (vec3_t pos, vec3_t orientation[3], unsigned int modeleflags, int type, trailkey_t *tk)
{
	float count;
	if (cl.paused)
		return;

	count = ((host_frametime>0.1)?0.1:host_frametime);
	if (orientation)
	{
		if (modeleflags & MDLF_EMITFORWARDS)
			pe->RunParticleEffectState(pos, orientation[0], count, type, tk);
		else
		{
			vec3_t down;
			VectorNegate(orientation[2], down);
			pe->RunParticleEffectState(pos, down, count, type, tk);
		}
	}
	else
		pe->RunParticleEffectState(pos, NULL, count, type, tk);
}











// P_SelectableTrail: given default/opposite effects, model pointer, and a user selection cvar
// changes model to the appropriate trail effect and default trail index
static void P_SelectableTrail(int *trailid, int *trailpalidx, cvar_t *selection, int mdleffect, int mdlcidx, int oppeffect, int oppcidx)
{
	int select = (int)(selection->value);

	switch (select)
	{
	case 0: // check for string, otherwise no trail
		if (selection->string[0] == '0')
		{
			*trailid = P_INVALID;
			*trailpalidx = -1;
			break;
		}
		else
		{
			int effect = P_FindParticleType(selection->string);

			if (effect >= 0)
			{
				*trailid = effect;
				*trailpalidx = mdlcidx;
				break;
			}
		}
		// fall through to default (so semicheat will work properly)
	case 1: // default model effect
	default:
		*trailid = mdleffect;
		*trailpalidx = mdlcidx;
		break;
	case 2: // opposite effect
		*trailid = oppeffect;
		*trailpalidx= oppcidx;
		break;
	case 3: // alt rocket effect
		*trailid = P_FindParticleType("TR_ALTROCKET");
		*trailpalidx = 107;
		break;
	case 4: // gib
		*trailid = P_FindParticleType("TR_BLOOD");
		*trailpalidx = 70;
		break;
	case 5: // zombie gib
		*trailid = P_FindParticleType("TR_SLIGHTBLOOD");
		*trailpalidx = 70;
		break;
	case 6: // Scrag tracer
		*trailid = P_FindParticleType("TR_WIZSPIKE");
		*trailpalidx = 60;
		break;
	case 7: // Knight tracer
		*trailid = P_FindParticleType("TR_KNIGHTSPIKE");
		*trailpalidx = 238;
		break;
	case 8: // Vore tracer
		*trailid = P_FindParticleType("TR_VORESPIKE");
		*trailpalidx = 154;
		break;
	case 9: // rail trail
		*trailid = P_FindParticleType("TE_RAILTRAIL");
		*trailpalidx = 15;
		break;
	}
}



//figure out which particle trail to use for the given model, filling in its values as required.
void P_DefaultTrail (unsigned int entityeffects, unsigned int modelflags, int *trailid, int *trailpalidx)
{
	// TODO: EF_BRIGHTFIELD should probably be handled in here somewhere
	// TODO: make trail default color into RGB values instead of indexes
	if (!pe)
		return;

	if (entityeffects & EF_BRIGHTFIELD)
	{
		*trailid = P_FindParticleType("EF_BRIGHTFIELD");
		*trailpalidx = 70;
	}
	else if (entityeffects & DPEF_FLAME)
	{
		*trailid = P_FindParticleType("EF_FLAME");
		*trailpalidx = 70;
	}
	else if (entityeffects & DPEF_STARDUST)
	{
		*trailid = P_FindParticleType("EF_STARDUST");
		*trailpalidx = 70;
	}
	else if (modelflags & MF_ROCKET)
		P_SelectableTrail(trailid, trailpalidx, &r_rockettrail, P_FindParticleType("TR_ROCKET"), 109, P_FindParticleType("TR_GRENADE"), 6);
	else if (modelflags & MF_GRENADE)
		P_SelectableTrail(trailid, trailpalidx, &r_grenadetrail, P_FindParticleType("TR_GRENADE"), 6, P_FindParticleType("TR_ROCKET"), 109);
	else if (modelflags & MF_GIB)
	{
		*trailid = P_FindParticleType("TR_BLOOD");
		*trailpalidx = 70;
	}
	else if (modelflags & MF_TRACER)
	{
		*trailid = P_FindParticleType("TR_WIZSPIKE");
		*trailpalidx = 60;
	}
	else if (modelflags & MF_ZOMGIB)
	{
		*trailid = P_FindParticleType("TR_SLIGHTBLOOD");
		*trailpalidx = 70;
	}
	else if (modelflags & MF_TRACER2)
	{
		*trailid = P_FindParticleType("TR_KNIGHTSPIKE");
		*trailpalidx = 238;
	}
	else if (modelflags & MF_TRACER3)
	{
		*trailid = P_FindParticleType("TR_VORESPIKE");
		*trailpalidx = 154;
	}
#ifdef HEXEN2
	else if (modelflags & MFH2_BLOODSHOT)	//these are the hexen2 ones.
	{
		*trailid = P_FindParticleType("tr_bloodshot");
		*trailpalidx = 136;
	}
	else if (modelflags & MFH2_FIREBALL)
	{
		*trailid = P_FindParticleType("tr_fireball");
		*trailpalidx = 424;
	}
	else if (modelflags & MFH2_ACIDBALL)
	{
		*trailid = P_FindParticleType("tr_acidball");
		*trailpalidx = 440;
	}
	else if (modelflags & MFH2_ICE)
	{
		*trailid = P_FindParticleType("tr_ice");
		*trailpalidx = 408;
	}
	else if (modelflags & MFH2_SPIT)
	{
		*trailid = P_FindParticleType("tr_spit");
		*trailpalidx = 260;
	}
	else if (modelflags & MFH2_SPELL)
	{
		*trailid = P_FindParticleType("tr_spell");
		*trailpalidx = 260;
	}
	else if (modelflags & MFH2_VORP_MISSILE)
	{
		*trailid = P_FindParticleType("tr_vorpmissile");
		*trailpalidx = 302;
	}
	else if (modelflags & MFH2_SET_STAFF)
	{
		*trailid = P_FindParticleType("tr_setstaff");
		*trailpalidx = 424;
	}
	else if (modelflags & MFH2_MAGICMISSILE)
	{
		*trailid = P_FindParticleType("tr_magicmissile");
		*trailpalidx = 149;
	}
	else if (modelflags & MFH2_BONESHARD)
	{
		*trailid = P_FindParticleType("tr_boneshard");
		*trailpalidx = 384;
	}
	else if (modelflags & MFH2_SCARAB)
	{
		*trailid = P_FindParticleType("tr_scarab");
		*trailpalidx = 254;
	}
	else if (modelflags & MFH2_SPIDERBLOOD)
	{
		//spiders
		*trailid = P_FindParticleType("TR_GREENBLOOD");
		*trailpalidx = 70;	//fixme
	}
#endif
	else
	{
		*trailid = P_INVALID;
		*trailpalidx = -1;
	}
}
