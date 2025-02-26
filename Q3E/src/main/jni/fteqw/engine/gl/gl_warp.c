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
// gl_warp.c -- sky and water polygons

#include "quakedef.h"
#ifdef HAVE_CLIENT
#include "glquake.h"
#include "shader.h"
#include <ctype.h>

static void R_CalcSkyChainBounds (batch_t *s);
static void GL_DrawSkySphere (batch_t *fa, shader_t *shader);
static void GL_SkyForceDepth(batch_t *fa);
static void GL_DrawSkyBox (texid_t *texnums, batch_t *s);

static void GL_DrawSkyGrid (texnums_t *tex);

static void QDECL R_SkyBox_Changed (struct cvar_s *var, char *oldvalue);

cvar_t r_fastsky							= CVARF ("r_fastsky", "0",	CVAR_ARCHIVE);
static cvar_t r_fastskycolour						= CVARF ("r_fastskycolour", "0",	CVAR_RENDERERCALLBACK|CVAR_SHADERSYSTEM);
static cvar_t gl_skyboxdist						= CVARD  ("gl_skyboxdist", "0", "The distance of the skybox. If 0, the engine will determine it based upon the far clip plane distance.");	//0 = guess.
static cvar_t r_skycloudalpha						= CVARFD ("r_skycloudalpha", "1", CVAR_RENDERERLATCH, "Controls how opaque the front layer of legacy scrolling skies should be.");
cvar_t r_skyboxname							= CVARFC ("r_skybox", "", CVAR_RENDERERCALLBACK | CVAR_SHADERSYSTEM, R_SkyBox_Changed);
cvar_t r_skybox_orientation					= CVARFD ("r_glsl_skybox_orientation", "0 0 0 0", CVAR_SHADERSYSTEM, "Defines the axis around which skyboxes will rotate (the first three values). The fourth value defines the speed the skybox rotates at, in degrees per second.");
cvar_t r_skybox_autorotate					= CVARFD ("r_glsl_skybox_autorotate", "1", CVAR_SHADERSYSTEM, "Defines the axis around which skyboxes will rotate (the first three values). The fourth value defines the speed the skybox rotates at, in degrees per second.");
cvar_t r_skyfog								= CVARD  ("r_skyfog", "0.5", "This controls an alpha-blend value for fog on skyboxes, cumulative with regular fog alpha.");

static shader_t *forcedsky;
static shader_t *skyboxface;
static shader_t *skygridface;


//=========================================================

//called on video shutdown to reset internal state
void R_SkyShutdown(void)
{
	skyboxface = NULL;
	skygridface = NULL;
	forcedsky = NULL;
}

//lets the backend know which fallback envmap it can use.
texid_t R_GetDefaultEnvmap(void)
{
	if (*r_refdef.nearenvmap.texname)
		return Image_GetTexture(r_refdef.nearenvmap.texname, NULL, IF_TEXTYPE_CUBE, NULL, NULL, 0, 0, TF_INVALID);

	if (forcedsky && TEXLOADED(forcedsky->defaulttextures->reflectcube))
		return forcedsky->defaulttextures->reflectcube;

	return r_nulltex;
}

void R_SetSky(const char *sky)
{
	int i;
	const char *shadername;
	extern cvar_t r_skyboxname;
	if (sky)
		Q_strncpyz(cl.skyname, sky, sizeof(cl.skyname));
	else
		sky = cl.skyname;
	if (qrenderer <= QR_NONE)
		return;	//not ready yet...
	if (*r_skyboxname.string)	//override it with the user's preference
		sky = r_skyboxname.string;

	shadername = va("skybox_%s", sky);
	if (!forcedsky || strcmp(shadername, forcedsky->name))
	{
		texnums_t tex;
		forcedsky = NULL;	//fall back to regular skies if forcing fails.

		if (!*sky)
			return; //no need to do anything

		memset(&tex, 0, sizeof(tex));

		tex.base = R_LoadHiResTexture(sky, "env:gfx/env", IF_LOADNOW|IF_NOMIPMAP);
		if (tex.base && tex.base->status == TEX_LOADING)
			COM_WorkerPartialSync(tex.base, &tex.base->status, TEX_LOADING);
		if (tex.base->width && TEXLOADED(tex.base))
		{
			forcedsky = R_RegisterShader(shadername, 0,
				"{\n"
					"sort sky\n"
					"program defaultsky#EQUI\n"
					"{\n"
						"if !$unmaskedsky\n"	/* Q2/HL require the skybox to not draw over geometry, shouldn't we force it? --eukara */
							"depthwrite\n"
						"endif\n"
						"map \"$diffuse\"\n"
						"tcgen skybox\n"
					"}\n"
					"surfaceparm nodlight\n"
				"surfaceparm sky\n"
				"}");
			R_BuildDefaultTexnums(&tex, forcedsky, IF_WORLDTEX);
			return;
		}

		//if we have cubemaps then we can just go and use a cubemap for our skybox
		if (sh_config.havecubemaps)
		{
			memset(&tex, 0, sizeof(tex));
			tex.reflectcube = R_LoadHiResTexture(sky, "env:gfx/env", IF_LOADNOW|IF_TEXTYPE_CUBE|IF_NOMIPMAP|IF_CLAMP);
			if (tex.reflectcube && tex.reflectcube->status == TEX_LOADING)
				COM_WorkerPartialSync(tex.reflectcube, &tex.reflectcube->status, TEX_LOADING);
			if (tex.reflectcube->width && TEXLOADED(tex.reflectcube))
			{
				forcedsky = R_RegisterShader(shadername, 0,
					"{\n"
						"sort sky\n"
						"program defaultskybox\n"
						"{\n"
							"if !$unmaskedsky\n"	/* Q2/HL require the skybox to not draw over geometry, shouldn't we force it? --eukara */
								"depthwrite\n"
							"endif\n"
							"map \"$cube:$reflectcube\"\n"
							"tcgen skybox\n"
						"}\n"
						"surfaceparm nodlight\n"
					"surfaceparm sky\n"
					"}");
				R_BuildDefaultTexnums(&tex, forcedsky, IF_WORLDTEX);
				return;
			}
		}

		//crappy old path that I still need to fix up a bit
		//unlike cubemaps, this works on gl1.1/gles1, and also works with the different faces as different sizes.
		forcedsky = R_RegisterShader(shadername, 0, va("{\nsort sky\nskyparms \"%s\" 512 -\nsurfaceparm nodlight\n}", sky));
		//check that we actually got some textures.
		//we accept the skybox if even 1 face is valid.
		//we ignore the replacement only request if all are invalid.
		for (i = 0; i < 6; i++)
		{
			extern texid_t missing_texture;
			if (forcedsky->skydome && forcedsky->skydome->farbox_textures[i] != missing_texture)
				break;
		}
		if (i == 6)	//couldn't find ANY sky textures.
			forcedsky = NULL;
	}
}

struct skylist_s
{
	const char *prefix;
	const char *partial;
	size_t partiallen;
	struct xcommandargcompletioncb_s *ctx;
};
static int QDECL R_ForceSky_Enumerated (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct skylist_s *ctx = parm;
	char base[MAX_QPATH];
	const char *ext;
	size_t l = strlen(ctx->prefix), pl;
	size_t p;

	static char *skypost[] =
	{
		"rt","lf","bk","ft","up","dn",
		"px","nx","py","ny","pz","nz",
		"posx","negx","posy","negy","posz","negz",
	};

	if (!strncmp(name, ctx->prefix, l))
	{
		ext = COM_GetFileExtension(name+l, NULL);
		COM_StripExtension(name+l, base, sizeof(base));
		l = strlen(base);
		for (p = 0; p < countof(skypost); p++)
		{
			pl = strlen(skypost[p]);
			if (pl > l)
				continue;
			if (!strcmp(base+l-pl, skypost[p]))
			{
				if (p%6)
					return true;
				base[l-pl] = 0;	//strip the postfix too.
				break;
			}
		}
		if (p == countof(skypost) && strcmp(ext, ".tga") && strcmp(ext, ".png"))	//give it its extension back if its not a regular skybox.
			Q_strncatz(base, ext, sizeof(base));
		if (!Q_strncasecmp(base, ctx->partial, ctx->partiallen))
		{
			//non-matches are dds/ktx cubemaps, or equirectangular or something
			if (ctx->ctx)
				ctx->ctx->cb(base, NULL, NULL, ctx->ctx);
			else
				Con_Printf("\t^[%s\\type\\%s %s^]\n", base, r_skyboxname.name, base);
		}
	}
	return true;
}
static void R_ForceSky_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	static char *skypath[] =
	{
		"env/",
		"gfx/env/",
		"textures/env/",
		"textures/gfx/env/",
	};
	struct skylist_s l;
	size_t pre;
	l.ctx = ctx;
	l.partial = partial;
	l.partiallen = strlen(partial);
	for (pre = 0; pre < countof(skypath); pre++)
	{
		l.prefix = skypath[pre];
		COM_EnumerateFiles(va("%s*.*", l.prefix), R_ForceSky_Enumerated, &l);
	}

	//no skybox is also an option.
	if (ctx && !*partial)
		ctx->cb("", NULL, NULL, ctx);
}
static void R_ListSkyBoxes_f(void)
{
	Con_Printf("Skybox Options:\n");
	R_ForceSky_c(0, "", NULL);
}
static void R_ForceSky_f(void)
{
	if (Cmd_Argc() < 2)
	{
		if (*r_skyboxname.string)
			Con_Printf("Current user skybox is %s\n", r_skyboxname.string);
		else if (*cl.skyname)
			Con_Printf("Current per-map skybox is %s\n", cl.skyname);
		else
			Con_Printf("no skybox forced.\n");
	}
	else
	{
		R_SetSky(Cmd_Argv(1));
	}
}
void QDECL R_SkyBox_Changed (struct cvar_s *var, char *oldvalue)
{
	R_SetSky(NULL);
//	Shader_NeedReload(false);
}

void R_DrawFastSky(batch_t *batch)
{
	batch_t b = *batch;
	b.shader = R_RegisterShader("fastsky", 0, "{\n"
					"sort sky\n"
					"{\n"
						"map $whiteimage\n"
						"rgbgen const $r_fastskycolour\n"
					"}\n"
					"surfaceparm nodlight\n"
				"}\n");
	b.skin = NULL;
	b.texture = NULL;
	BE_SubmitBatch(&b);
}

//annoyingly sky does not always write depth values.
//this can allow entities to appear beyond it.
//this can include (massive) entities outside of the skyroom.
//so, we can only draw entities in skyrooms if:
//1) either the ents have robust pvs info and we can draw ONLY the ones actually inside the skyroom
//2) or if r_ignoreentpvs==1 we must mask depth and waste a whole load of batches drawing invisible ents in the sky
extern cvar_t r_ignoreentpvs;

qboolean R_DrawSkyroom(shader_t *skyshader)
{
	float vmat[16];
	refdef_t oldrefdef;
	int oldarea = r_viewarea, oldcluster[2] = {r_viewcluster,r_viewcluster2};
//	extern cvar_t r_ignoreentpvs; //legacy value is 1...

	if (r_viewcluster == -1)
		return false;	//don't draw the skyroom if the camera is outside.

	if (r_fastsky.value)
		return false;	//skyrooms can be expensive.
	if (!r_refdef.skyroom_enabled || r_refdef.recurse >= R_MAX_RECURSE-1)
		return false;

	oldrefdef = r_refdef;
	r_refdef.recurse+=1;

//	if (r_ignoreentpvs.ival)	//if we're ignoring ent pvs then we're probably drawing lots of ents in the skybox that shouldn't be there
//		r_refdef.firstvisedict = cl_numvisedicts;
	r_refdef.externalview = true;	//an out-of-body experience...
	r_refdef.skyroom_enabled = false;
	r_refdef.forcevis = false;
	r_refdef.flags |= RDF_DISABLEPARTICLES;
	r_refdef.flags &= ~RDF_SKIPSKY;
	r_refdef.forcedvis = NULL;
	r_refdef.areabitsknown = false;	//recalculate areas clientside.
	r_refdef.sceneareas = NULL;

	if (cl.fog[FOGTYPE_SKYROOM].density)
	{
		CL_BlendFog(&r_refdef.globalfog, &cl.oldfog[FOGTYPE_SKYROOM], realtime, &cl.fog[FOGTYPE_SKYROOM]);
		r_refdef.globalfog.density/=64;
	}

	/*work out where the camera should be (use the same angles)*/
	VectorCopy(r_refdef.skyroom_pos, r_refdef.vieworg);
	VectorCopy(r_refdef.skyroom_pos, r_refdef.pvsorigin);

	if (developer.ival)
		if (r_worldentity.model->funcs.PointContents(r_worldentity.model, NULL, r_refdef.skyroom_pos) & FTECONTENTS_SOLID)
			Con_DPrintf("Skyroom position %.1f %.1f %.1f in solid\n", r_refdef.skyroom_pos[0], r_refdef.skyroom_pos[1], r_refdef.skyroom_pos[2]);

	if (r_refdef.skyroom_spin[3])
	{
		vec3_t axis[3];
		float ang = r_refdef.skyroom_spin[3];
		if (!r_refdef.skyroom_spin[0]&&!r_refdef.skyroom_spin[1]&&!r_refdef.skyroom_spin[2])
			VectorSet(r_refdef.skyroom_spin, 0,0,1);
		VectorNormalize(r_refdef.skyroom_spin);
		RotatePointAroundVector(axis[0], r_refdef.skyroom_spin, vpn, ang);
		RotatePointAroundVector(axis[1], r_refdef.skyroom_spin, vright, ang);
		RotatePointAroundVector(axis[2], r_refdef.skyroom_spin, vup, ang);
		Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, axis[0], axis[1], axis[2], r_refdef.vieworg);
	}
	else
		Matrix4x4_CM_ModelViewMatrixFromAxis(vmat, vpn, vright, vup, r_refdef.vieworg);
	R_SetFrustum (r_refdef.m_projection_std, vmat);

	//now determine the stuff the backend will use.
	memcpy(r_refdef.m_view, vmat, sizeof(float)*16);
	VectorAngles(vpn, vup, r_refdef.viewangles, false);
	VectorCopy(r_refdef.vieworg, r_origin);

	Surf_SetupFrame();
	Surf_DrawWorld ();

	r_viewarea = oldarea;
	r_viewcluster = oldcluster[0];
	r_viewcluster2 = oldcluster[1];
	r_refdef = oldrefdef;

	/*broken stuff*/
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

	return true;
}

//q3 mustn't mask sky (breaks q3map2's invisible skyportals), whereas q1 must (or its a cheat). halflife doesn't normally expect masking.
//we also MUST mask any sky inside skyrooms, or you'll see all the entities outside of the skyroom through the room's own sky (q3map2 skyportals are hopefully irrelevant in this case).
#define SKYMUSTBEMASKED (r_worldentity.model->fromgame != fg_quake3 || ((r_refdef.flags & RDF_DISABLEPARTICLES) && r_ignoreentpvs.ival) || !cls.allow_unmaskedskyboxes)

/*
=================
GL_DrawSkyChain
=================
*/
qboolean R_DrawSkyChain (batch_t *batch)
{
	shader_t *skyshader;
	texid_t *skyboxtex;

	if (r_fastsky.value)
	{
		R_DrawFastSky(batch);
		return true;	//depth will always be drawn with this pathway.
	}

	if (forcedsky)
	{
		skyshader = forcedsky;

		if (r_refdef.flags & RDF_SKIPSKY)
		{
			if (r_worldentity.model->fromgame != fg_quake3)
				GL_SkyForceDepth(batch);
			return true;
		}

		if (forcedsky->numpasses && !forcedsky->skydome && batch->mesh[0]->xyz_array)
		{	//cubemap skies!
			//this is just a simple pass. we use glsl/texgen for any actual work
			batch_t b = *batch;
			b.shader = forcedsky;
			b.skin = NULL;
			b.texture = NULL;
			BE_SubmitBatch(&b);
			return true;
		}
	}
	else
	{
		skyshader = batch->shader;
		if (skyshader->prog)	//glsl is expected to do the whole skybox/warpsky thing itself, with no assistance from this legacy code.
		{
			if (r_refdef.flags & RDF_SKIPSKY)
			{
				if (SKYMUSTBEMASKED)
					GL_SkyForceDepth(batch);
				return true;
			}
			//if the first pass is transparent in some form, then be prepared to give it a skyroom behind.
			return false;	//draw as normal...
		}
	}

	if (skyshader->skydome)
		skyboxtex = skyshader->skydome->farbox_textures;
	else
		skyboxtex = NULL;

	if (r_refdef.flags & RDF_SKIPSKY)
	{	//don't obscure the skyroom if the sky shader is opaque.
		qboolean opaque = false;
		if (skyshader->numpasses)
		{
			shaderpass_t *pass = skyshader->passes;
			if (pass->shaderbits & SBITS_ATEST_BITS)	//alphatests
				;
			else if (pass->shaderbits & SBITS_MASK_BITS)	//colormasks
				;
			else if ((pass->shaderbits & SBITS_BLEND_BITS) != 0 && (pass->shaderbits & SBITS_BLEND_BITS) != (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO))	//blendfunc
				;
			else
				opaque = true;	//that shader looks like its opaque.
		}
		if (!opaque)
			GL_DrawSkySphere(batch, skyshader);
	}
	else if (skyboxtex && TEXVALID(*skyboxtex))
	{	//draw a skybox if we were given the textures
		R_CalcSkyChainBounds(batch);
		GL_DrawSkyBox (skyboxtex, batch);

		if (skyshader->numpasses)
			GL_DrawSkySphere(batch, skyshader);
	}
	else if (skyshader->numpasses)
	{	//if we have passes, then they're normally projected.
		if (*r_fastsky.string && skyshader->numpasses == 2 && TEXVALID(batch->shader->defaulttextures->base) && TEXVALID(batch->shader->defaulttextures->fullbright))
		{	//we have a small perf trick to accelerate q1 skies, also helps avoids distortions, but doesn't work too well for any other type of sky.
			R_CalcSkyChainBounds(batch);
			GL_DrawSkyGrid(skyshader->defaulttextures);
		}
		else
			GL_DrawSkySphere(batch, skyshader);
	}
	else if (batch->meshes)
	{	//skys are weird.
		//they're the one type of surface with implicit nodraw when there's no passes.
		if ((skyboxtex&&*skyboxtex) || batch->shader->numpasses)
			R_DrawFastSky(batch);
		return true;	//depth will always be drawn with this pathway... or we were not drawing anything anyway...
	}

	//neither skydomes nor skyboxes nor skygrids will have been drawn with the correct depth values for the sky.
	//this can result in rooms behind the sky surfaces being visible.
	//so make sure they're correct where they're expected to be.
	//don't do it on q3 bsp, because q3map2 can't do skyrooms without being weird about it. or something. anyway, we get different (buggy) behaviour from q3 if we don't skip this.
	//See: The Edge Of Forever (motef, by sock) for an example of where this needs to be skipped.
	//See dm3 for an example of where the depth needs to be correct (OMG THERE'S PLAYERS IN MY SKYBOX! WALLHAXX!).
	//you can't please them all.
	if (SKYMUSTBEMASKED)
		GL_SkyForceDepth(batch);

	return true;
}

/*
=================================================================

  Quake 2 environment sky

=================================================================
*/

static vec3_t	skyclip[6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};

// 1 = s, 2 = t, 3 = 2048
static int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
static int	vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

static float	skymins[2][6], skymaxs[2][6];

static void DrawSkyPolygon (int nump, vec3_t vecs)
{
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;

	// decide which face it maps to
	VectorClear (v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
	{
		VectorAdd (vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];

		if (dv < 0.001)
			continue;	// don't divide by zero

		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (skymins[0][axis] > s)
			skymins[0][axis] = s;
		if (skymins[1][axis] > t)
			skymins[1][axis] = t;
		if (skymaxs[0][axis] < s)
			skymaxs[0][axis] = s;
		if (skymaxs[1][axis] < t)
			skymaxs[1][axis] = t;
	}
}

#define	MAX_CLIP_VERTS	64
static void ClipSkyPolygon (int nump, vec3_t vecs, int stage)
{
	float	*norm;
	float	*v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		Sys_Error ("ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

/*
=================
R_DrawSkyBoxChain
=================
*/
static void R_CalcSkyChainBounds (batch_t *batch)
{
	mesh_t *mesh;

	int		i, m;
	vec3_t	verts[MAX_CLIP_VERTS];

	if (batch->meshes == 1 && !batch->mesh[batch->firstmesh]->numindexes)
	{	//deal with geometryless skies, like terrain/raw maps
		for (i=0 ; i<6 ; i++)
		{
			skymins[0][i] = skymins[1][i] = -1;
			skymaxs[0][i] = skymaxs[1][i] = 1;
		}
		return;
	}
	for (i=0 ; i<6 ; i++)
	{
		skymins[0][i] = skymins[1][i] = 1;//9999;
		skymaxs[0][i] = skymaxs[1][i] = -1;//-9999;
	}

	// calculate vertex values for sky box
	for (m = batch->firstmesh; m < batch->meshes; m++)
	{
		mesh = batch->mesh[m];
		if (!mesh->xyz_array || !mesh->indexes)
			continue;
		//triangulate
		for (i = 0; i < mesh->numindexes; i+=3)
		{
			VectorSubtract (mesh->xyz_array[mesh->indexes[i+0]], r_origin, verts[0]);
			VectorSubtract (mesh->xyz_array[mesh->indexes[i+1]], r_origin, verts[1]);
			VectorSubtract (mesh->xyz_array[mesh->indexes[i+2]], r_origin, verts[2]);
			ClipSkyPolygon (3, verts[0], 0);
		}
	}
}

#define skygridx 16
#define skygridx1 (skygridx + 1)
#define skygridxrecip (1.0f / (skygridx))
#define skygridy 16
#define skygridy1 (skygridy + 1)
#define skygridyrecip (1.0f / (skygridy))
#define skysphere_numverts (skygridx1 * skygridy1)
#define skysphere_numtriangles (skygridx * skygridy * 2)

static int skymade;
static index_t skysphere_element3i[skysphere_numtriangles * 3];
static float skysphere_texcoord2f[skysphere_numverts * 2];

static vecV_t skysphere_vertex3f[skysphere_numverts];
static mesh_t skymesh;


static void gl_skyspherecalc(int skytype)
{	//yes, this is basically stolen from DarkPlaces
	int i, j;
	index_t *e;
	float a, b, x, ax, ay, v[3], length, *texcoord2f;
	vecV_t* vertex;
	float dx, dy, dz;

	float texscale;

	if (skymade == skytype)
		return;

	skymade = skytype;

	if (skymade == 2)
		texscale = 1/16.0f;
	else
		texscale = 1/1.5f;

	texscale*=3;

	skymesh.indexes = skysphere_element3i;
	skymesh.st_array = (void*)skysphere_texcoord2f;
	skymesh.lmst_array[0] = (void*)skysphere_texcoord2f;
	skymesh.xyz_array = (void*)skysphere_vertex3f;

	skymesh.numindexes = skysphere_numtriangles * 3;
	skymesh.numvertexes = skysphere_numverts;

	dx = 1;
	dy = 1;
	dz = 1 / 3.0;
	vertex = skysphere_vertex3f;
	texcoord2f = skysphere_texcoord2f;
	for (j = 0;j <= skygridy;j++)
	{
		a = j * skygridyrecip;
		ax = cos(a * M_PI * 2);
		ay = -sin(a * M_PI * 2);
		for (i = 0;i <= skygridx;i++)
		{
			b = i * skygridxrecip;
			x = cos((b + 0.5) * M_PI);
			v[0] = ax*x * dx;
			v[1] = ay*x * dy;
			v[2] = -sin((b + 0.5) * M_PI) * dz;
			length = texscale / sqrt(v[0]*v[0]+v[1]*v[1]+(v[2]*v[2]*9));
			*texcoord2f++ = v[0] * length;
			*texcoord2f++ = v[1] * length;
			(*vertex)[0] = v[0];
			(*vertex)[1] = v[1];
			(*vertex)[2] = v[2];
			vertex++;
		}
	}
	e = skysphere_element3i;
	for (j = 0;j < skygridy;j++)
	{
		for (i = 0;i < skygridx;i++)
		{
			*e++ =  j      * skygridx1 + i;
			*e++ =  j      * skygridx1 + i + 1;
			*e++ = (j + 1) * skygridx1 + i;

			*e++ =  j      * skygridx1 + i + 1;
			*e++ = (j + 1) * skygridx1 + i + 1;
			*e++ = (j + 1) * skygridx1 + i;
		}
	}
}

static void GL_SkyForceDepth(batch_t *batch)
{
	if (!cls.allow_unmaskedskyboxes && batch->texture)	//allow a little extra fps.
	{
		BE_SelectMode(BEM_DEPTHONLY);
		BE_DrawMesh_List(batch->shader, batch->meshes-batch->firstmesh, batch->mesh+batch->firstmesh, batch->vbo, NULL, batch->flags);
		BE_SelectMode(BEM_STANDARD);	/*skys only render in standard mode anyway, so this is safe*/
	}
}

static void R_DrawSkyMesh(batch_t *batch, mesh_t *m, shader_t *shader)
{
	static entity_t skyent;
	batch_t b;

	float skydist = gl_skyboxdist.value;
	if (skydist<1)
		skydist=r_refdef.maxdist * 0.577;
	if (skydist<1)
		skydist = 10000000;

	VectorCopy(r_refdef.vieworg, skyent.origin);
	skyent.axis[0][0] = skydist;
	skyent.axis[0][1] = 0;
	skyent.axis[0][2] = 0;
	skyent.axis[1][0] = 0;
	skyent.axis[1][1] = skydist;
	skyent.axis[1][2] = 0;
	skyent.axis[2][0] = 0;
	skyent.axis[2][1] = 0;
	skyent.axis[2][2] = skydist;
	skyent.scale = 1;

//FIXME: We should use the skybox clipping code and split the sphere into 6 sides.
	b = *batch;
	b.meshes = 1;
	b.firstmesh = 0;
	b.mesh = &m;
	b.ent = &skyent;
	b.shader = shader;
	b.skin = NULL;
	b.texture = NULL;
	b.vbo = NULL;
	Vector4Set(skyent.shaderRGBAf, 1, 1, 1, 1);
	BE_SubmitBatch(&b);
}

static void GL_DrawSkySphere (batch_t *batch, shader_t *shader)
{
	//FIXME: We should use the skybox clipping code and split the sphere into 6 sides.
	gl_skyspherecalc(2);
	R_DrawSkyMesh(batch, &skymesh, shader);
}

static void GL_MakeSkyVec (float s, float t, int axis, float *vc, float *tc)
{
	vec3_t		b;
	int			j, k;

	b[0] = s;
	b[1] = t;
	b[2] = 1;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			vc[j] = -b[-k - 1];
		else
			vc[j] = b[k - 1];
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	if (s < 1.0/512)
		s = 1.0/512;
	else if (s > 511.0/512)
		s = 511.0/512;
	if (t < 1.0/512)
		t = 1.0/512;
	else if (t > 511.0/512)
		t = 511.0/512;

	tc[0] = s;
	tc[1] = 1.0 - t;
}


static float	speedscale1;	// for top sky
static float	speedscale2;	// for bottom sky
static void EmitSkyGridVert (vec3_t v, vec2_t tc1, vec2_t tc2)
{
	vec3_t dir;
	float	length;

	VectorSubtract (v, r_origin, dir);
	dir[2] *= 3;	// flatten the sphere

	length = VectorLength (dir);
	length = 6*63/length;

	dir[0] *= length;
	dir[1] *= length;

	tc1[0] = (speedscale1 + dir[0]) * (1.0/128);
	tc1[1] = (speedscale1 + dir[1]) * (1.0/128);

	tc2[0] = (speedscale2 + dir[0]) * (1.0/128);
	tc2[1] = (speedscale2 + dir[1]) * (1.0/128);
}

// s and t range from -1 to 1
static void MakeSkyGridVec2 (float s, float t, int axis, vec3_t v, vec2_t tc1, vec2_t tc2)
{
	vec3_t		b;
	int			j, k;

	float skydist = gl_skyboxdist.value;
	if (skydist<1)
		skydist=r_refdef.maxdist * 0.577;
	if (skydist<1)
		skydist = 10000000;

	b[0] = s*skydist;
	b[1] = t*skydist;
	b[2] = skydist;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
		v[j] += r_origin[j];
	}

	EmitSkyGridVert(v, tc1, tc2);
}

#define SUBDIVISIONS	10

static void GL_DrawSkyGridFace (int axis, mesh_t *fte_restrict mesh)
{
	int i, j;
	float s, t;

	float fstep = 2.0 / SUBDIVISIONS;

	for (i = 0; i < SUBDIVISIONS; i++)
	{
		s = (float)(i*2 - SUBDIVISIONS) / SUBDIVISIONS;

		if (s + fstep < skymins[0][axis] || s > skymaxs[0][axis])
			continue;

		for (j = 0; j < SUBDIVISIONS; j++)
		{
			t = (float)(j*2 - SUBDIVISIONS) / SUBDIVISIONS;

			if (t + fstep < skymins[1][axis] || t > skymaxs[1][axis])
				continue;

			mesh->indexes[mesh->numindexes++] = mesh->numvertexes+0;
			mesh->indexes[mesh->numindexes++] = mesh->numvertexes+1;
			mesh->indexes[mesh->numindexes++] = mesh->numvertexes+2;
			mesh->indexes[mesh->numindexes++] = mesh->numvertexes+0;
			mesh->indexes[mesh->numindexes++] = mesh->numvertexes+2;
			mesh->indexes[mesh->numindexes++] = mesh->numvertexes+3;

			MakeSkyGridVec2 (s,			t,			axis, mesh->xyz_array[mesh->numvertexes], mesh->st_array[mesh->numvertexes], mesh->lmst_array[0][mesh->numvertexes]); mesh->numvertexes++;
			MakeSkyGridVec2 (s,			t + fstep,	axis, mesh->xyz_array[mesh->numvertexes], mesh->st_array[mesh->numvertexes], mesh->lmst_array[0][mesh->numvertexes]); mesh->numvertexes++;
			MakeSkyGridVec2 (s + fstep, t + fstep,	axis, mesh->xyz_array[mesh->numvertexes], mesh->st_array[mesh->numvertexes], mesh->lmst_array[0][mesh->numvertexes]); mesh->numvertexes++;
			MakeSkyGridVec2 (s + fstep, t,			axis, mesh->xyz_array[mesh->numvertexes], mesh->st_array[mesh->numvertexes], mesh->lmst_array[0][mesh->numvertexes]); mesh->numvertexes++;
		}
	}
}

static void GL_DrawSkyGrid (texnums_t *tex)
{
	static entity_t skyent;
	static batch_t b;
	static mesh_t skymesh, *meshptr=&skymesh;

	vecV_t coords[SUBDIVISIONS*SUBDIVISIONS*4*6];
	vec2_t texcoords1[SUBDIVISIONS*SUBDIVISIONS*4*6];
	vec2_t texcoords2[SUBDIVISIONS*SUBDIVISIONS*4*6];
	index_t indexes[SUBDIVISIONS*SUBDIVISIONS*6*6];

	int i;
	float time = cl.gametime+realtime-cl.gametimemark;

	speedscale1 = time*8;
	speedscale1 -= (int)speedscale1 & ~127;
	speedscale2 = time*16;
	speedscale2 -= (int)speedscale2 & ~127;

	skymesh.indexes = indexes;
	skymesh.st_array = texcoords1;
	skymesh.lmst_array[0] = texcoords2;
	skymesh.xyz_array = coords;
	skymesh.numindexes = 0;
	skymesh.numvertexes = 0;

	for (i = 0; i < 6; i++)
	{
		if ((skymins[0][i] >= skymaxs[0][i]	|| skymins[1][i] >= skymaxs[1][i]))
			continue;
		GL_DrawSkyGridFace (i, &skymesh);
	}

	VectorCopy(r_refdef.vieworg, skyent.origin);
	skyent.axis[0][0] = 1;
	skyent.axis[0][1] = 0;
	skyent.axis[0][2] = 0;
	skyent.axis[1][0] = 0;
	skyent.axis[1][1] = 1;
	skyent.axis[1][2] = 0;
	skyent.axis[2][0] = 0;
	skyent.axis[2][1] = 0;
	skyent.axis[2][2] = 1;
	skyent.scale = 1;

	if (!skygridface)
		skygridface = R_RegisterShader("skygridface", SUF_NONE,
				"{\n"
					"program default2d\n"
					"{\n"
						"map $diffuse\n"
						"nodepth\n"	//don't write depth. this stuff is meant to be an infiniteish distance away.
					"}\n"
					"{\n"
						"map $fullbright\n"
						"blendfunc blend\n"
						"nodepth\n"	//don't write depth. this stuff is meant to be an infiniteish distance away.
					"}\n"
				"}\n"
			);

//FIXME: We should use the skybox clipping code and split the sphere into 6 sides.
	b.meshes = 1;
	b.firstmesh = 0;
	b.mesh = &meshptr;
	b.ent = &skyent;
	b.shader = skygridface;
	b.skin = tex;
	b.texture = NULL;
	b.vbo = NULL;
	BE_SubmitBatch(&b);
}

/*
==============
R_DrawSkyBox
==============
*/
static int	skytexorder[6] = {0,2,1,3,4,5};
static void GL_DrawSkyBox (texid_t *texnums, batch_t *s)
{
	int i;

	vecV_t skyface_vertex[4];
	vec2_t skyface_texcoord[4];
	index_t skyface_index[6] = {0, 1, 2, 0, 2, 3};
	vec4_t skyface_colours[4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
	mesh_t skyfacemesh = {0};

	if (cl.skyrotate)
	{
		for (i=0 ; i<6 ; i++)
		{
			if (skymins[0][i] < skymaxs[0][i]
				&& skymins[1][i] < skymaxs[1][i])
					break;

			skymins[0][i] = -1;	//fully visible
			skymins[1][i] = -1;
			skymaxs[0][i] = 1;
			skymaxs[1][i] = 1;
		}
		if (i == 6)
			return;	//can't see anything
		for ( ; i<6 ; i++)
		{
			skymins[0][i] = -1;
			skymins[1][i] = -1;
			skymaxs[0][i] = 1;
			skymaxs[1][i] = 1;
		}
	}

	skyfacemesh.indexes = skyface_index;
	skyfacemesh.st_array = skyface_texcoord;
	skyfacemesh.xyz_array = skyface_vertex;
	skyfacemesh.colors4f_array[0] = skyface_colours;
	skyfacemesh.numindexes = 6;
	skyfacemesh.numvertexes = 4;

	if (!skyboxface)
		skyboxface = R_RegisterShader("skyboxface", SUF_NONE,
				"{\n"
					"program default2d\n"
					"{\n"
						"map $diffuse\n"
						"nodepth\n"	//don't write depth. this stuff is meant to be an infiniteish distance away.
					"}\n"
				"}\n"
			);

	for (i=0 ; i<6 ; i++)
	{
		if (skymins[0][i] >= skymaxs[0][i]
		|| skymins[1][i] >= skymaxs[1][i])
			continue;

		GL_MakeSkyVec (skymins[0][i], skymins[1][i], i, skyface_vertex[0], skyface_texcoord[0]);
		GL_MakeSkyVec (skymins[0][i], skymaxs[1][i], i, skyface_vertex[1], skyface_texcoord[1]);
		GL_MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i, skyface_vertex[2], skyface_texcoord[2]);
		GL_MakeSkyVec (skymaxs[0][i], skymins[1][i], i, skyface_vertex[3], skyface_texcoord[3]);

		skyboxface->defaulttextures->base = texnums[skytexorder[i]];
		R_DrawSkyMesh(s, &skyfacemesh, skyboxface);
	}
}

//===============================================================

/*
=============
R_InitSky

A sky image is 256*128 and comprises two logical textures.
the left is the transparent/blended part. the right is the opaque/background part.
==============
*/
void R_InitSky (shader_t *shader, const char *skyname, uploadfmt_t fmt, qbyte *src, unsigned int width, unsigned int height)
{
	int			i, j, p;
	unsigned	*temp;
	unsigned	transpix, alphamask;
	int			r, g, b;
	unsigned	*rgba;
	char name[MAX_QPATH*2];

	unsigned int stride = width;
	width /= 2;

	if (width < 1 || height < 1 || stride != width*2 || !src)
		return;

	//try to load dual-layer-single-image skies.
	//this is always going to be lame special case crap
	if (gl_load24bit.ival)
	{
		size_t filesize = 0;
		qbyte *filedata = NULL;
		if (!filedata)
		{
			Q_snprintfz(name, sizeof(name), "textures/%s.tga", skyname);
			filedata = FS_LoadMallocFile(name, &filesize);
		}
		if (!filedata)
		{
			Q_snprintfz(name, sizeof(name), "textures/%s.png", skyname);
			filedata = FS_LoadMallocFile(name, &filesize);
		}

		if (filedata)
		{
			int imagewidth, imageheight;
			uploadfmt_t format;	//fixme, if this has no alpha, is it worth all this code?
			unsigned int *imagedata = (unsigned int*)ReadRawImageFile(filedata, filesize, &imagewidth, &imageheight, &format, true, name);
			Z_Free(filedata);

			if (imagedata && !(imagewidth&1))
			{
				imagewidth>>=1;

				temp = BZF_Malloc(imagewidth*imageheight*sizeof(*temp));
				if (temp)
				{
					for (i=0 ; i<imageheight ; i++)
						for (j=0 ; j<imagewidth ; j++)
						{
							temp[i*imagewidth+j] = imagedata[i*(imagewidth<<1)+j+imagewidth];
						}
					Q_snprintfz(name, sizeof(name), "%s_solid", skyname);
					Q_strlwr(name);
					shader->defaulttextures->base = R_LoadReplacementTexture(name, NULL, IF_NOALPHA, temp, imagewidth, imageheight, TF_RGBX32);

					for (i=0 ; i<imageheight ; i++)
						for (j=0 ; j<imagewidth ; j++)
						{
							temp[i*imagewidth+j] = imagedata[i*(imagewidth<<1)+j];
						}
					BZ_Free(imagedata);
					Q_snprintfz(name, sizeof(name), "%s_alpha:%s_trans", skyname, skyname);
					Q_strlwr(name);
					shader->defaulttextures->fullbright = R_LoadReplacementTexture(name, NULL, 0, temp, imagewidth, imageheight, TF_RGBA32);
					BZ_Free(temp);
					return;
				}
			}
			BZ_Free(imagedata);
		}
	}

	if (fmt & PTI_FULLMIPCHAIN)
	{	//input is expected to make sense...
		qbyte *front, *back;
		unsigned int bb, bw, bh, bd;
		unsigned int w, h, y;
		fmt = fmt&~PTI_FULLMIPCHAIN;
		Image_BlockSizeForEncoding(fmt, &bb, &bw, &bh, &bd);

		w = (width+bw-1)/bw;
		h = (height+bh-1)/bh;
		//d = (depth+bd-1)/bd;

		back = BZ_Malloc(bb*w*2*h);
		front = back + bb*w*h;
		for (y = 0; y < h; y++)
		{
			memcpy(back + bb*y*w, src + bb*(y*w*2+w), w*bb);
			memcpy(front + bb*y*w, src + bb*(y*w*2), w*bb);
		}
		if (!shader->defaulttextures->base)
		{
			Q_snprintfz(name, sizeof(name), "%s_solid", skyname);
			Q_strlwr(name);
			shader->defaulttextures->base = R_LoadReplacementTexture(name, NULL, IF_NOALPHA, back, width, height, fmt);
		}
		if (!shader->defaulttextures->fullbright)
		{	//FIXME: support _trans
			Q_snprintfz(name, sizeof(name), "%s_alpha:%s_trans", skyname, skyname);
			Q_strlwr(name);
			shader->defaulttextures->fullbright = R_LoadReplacementTexture(name, NULL, 0, front, width, height, fmt);
		}
		BZ_Free(back);
	}
	else
	{
		temp = BZ_Malloc(width*height*sizeof(*temp));

		// make an average value for the back to avoid
		// a fringe on the top level

		r = g = b = 0;
		for (i=0 ; i<height ; i++)
			for (j=0 ; j<width ; j++)
			{
				p = src[i*stride + j + width];
				rgba = &d_8to24rgbtable[p];
				temp[(i*width) + j] = *rgba;
				r += ((qbyte *)rgba)[0];
				g += ((qbyte *)rgba)[1];
				b += ((qbyte *)rgba)[2];
			}

		if (!shader->defaulttextures->base)
		{
			Q_snprintfz(name, sizeof(name), "%s_solid", skyname);
			Q_strlwr(name);
			shader->defaulttextures->base = R_LoadReplacementTexture(name, NULL, IF_NOALPHA, temp, width, height, TF_RGBX32);
		}

		if (!shader->defaulttextures->fullbright)
		{
			//fixme: use premultiplied alpha here.
			((qbyte *)&transpix)[0] = r/(width*height);
			((qbyte *)&transpix)[1] = g/(width*height);
			((qbyte *)&transpix)[2] = b/(width*height);
			((qbyte *)&transpix)[3] = 0;
			alphamask = r_skycloudalpha.value*255;
			alphamask = ((bound(0, alphamask, 0xff)<<24) | 0x00ffffff);
			alphamask = LittleLong(alphamask);
			for (i=0 ; i<height ; i++)
				for (j=0 ; j<width ; j++)
				{
					p = src[i*stride + j];
					if (p == 0)
						temp[(i*width) + j] = transpix;
					else
						temp[(i*width) + j] = d_8to24rgbtable[p] & alphamask;
				}

			//FIXME: support _trans
			Q_snprintfz(name, sizeof(name), "%s_alpha:%s_trans", skyname, skyname);
			Q_strlwr(name);
			shader->defaulttextures->fullbright = R_LoadReplacementTexture(name, NULL, 0, temp, width, height, TF_RGBA32);
		}
		BZ_Free(temp);
	}
}

void R_Sky_Register(void)
{
	const char *groupname = "Skies";
	Cvar_Register (&r_skycloudalpha,		groupname);
	Cvar_Register (&r_fastsky,				groupname);
	Cvar_Register (&r_fastskycolour,		groupname);
	Cvar_Register (&r_skyfog,				groupname);
	Cvar_Register (&r_skyboxname,			groupname);
	Cvar_Register (&r_skybox_orientation,	groupname);
	Cvar_Register (&r_skybox_autorotate,	groupname);
	Cvar_Register (&gl_skyboxdist,			groupname);

	Cmd_AddCommandAD("sky", R_ForceSky_f, R_ForceSky_c, "For compat with Quakespasm, please use r_skybox.");	//QS compat
	Cmd_AddCommandAD("loadsky", R_ForceSky_f, R_ForceSky_c, "For compat with DarkPlaces, please use r_skybox.");
	Cmd_AddCommandD ("listskyboxes", R_ListSkyBoxes_f, "Displays a list of available custom skyboxes that can be set with r_skybox, type or click an entry.");
}
#endif
