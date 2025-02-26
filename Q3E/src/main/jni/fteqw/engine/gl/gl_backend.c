#include "quakedef.h"

//#define FORCESTATE

void DumpGLState(void);

#ifdef GLQUAKE

#define r_refract_fboival (gl_config.ext_framebuffer_objects && r_refract_fbo.ival)

#include "glquake.h"
#include "shader.h"

#ifdef FORCESTATE
#pragma warningmsg("FORCESTATE is active")
#endif

#ifdef ANDROID
/*android appears to have a bug, and requires f and not i*/
#define qglTexEnvi qglTexEnvf
#endif

extern cvar_t gl_overbright;
extern cvar_t r_tessellation;
extern cvar_t r_wireframe;
extern cvar_t r_outline;
extern cvar_t r_outline_width;
extern cvar_t r_refract_fbo;

extern texid_t missing_texture;
extern texid_t missing_texture_gloss;
extern texid_t missing_texture_normal;
extern texid_t scenepp_postproc_cube;
extern texid_t r_whiteimage;

#ifdef GLQUAKE
static texid_t shadowmap[3];
static int shadow_fbo_id;
static int shadow_fbo_depth_num;
#endif

#ifndef GLSLONLY
static void GenerateTCMods(const shaderpass_t *pass, int passnum);
#endif

static const char LIGHTPASS_SHADER[] = "\
{\n\
	program rtlight%s\n\
\
	{\n\
		map $diffuse\n\
		nodepth\n\
		blendfunc add\n\
	}\n\
	{\n\
		map $normalmap\n\
	}\n\
	{\n\
		map $specular\n\
	}\n\
	{\n\
		map $lightcubemap\n\
	}\n\
	{\n\
		map $shadowmap\n\
	}\n\
	{\n\
		map $loweroverlay\n\
	}\n\
	{\n\
		map $upperoverlay\n\
	}\n\
}";

extern cvar_t r_glsl_offsetmapping, r_portalrecursion, r_portalonly;

static void BE_SendPassBlendDepthMask(unsigned int sbits);
void GLBE_SubmitBatch(batch_t *batch);
#ifdef RTLIGHTS
static qboolean GLBE_RegisterLightShader(int mode);
#endif

static struct {
	//internal state
	struct {
		int lastpasstmus;
//		int vbo_colour;
//		int vbo_texcoords[SHADER_PASS_MAX];
//		int vbo_deforms;	//holds verticies... in case you didn't realise.

		const shader_t *shader_light[LSHADER_MODES];
		qboolean inited_shader_light[LSHADER_MODES];

		const shader_t *crepskyshader;
		const shader_t *crepopaqueshader;
		const shader_t *depthonlyshader;
		const shader_t *wireframeshader;

		union programhandle_u	allblackshader;
		int allblack_mvp;

		program_t *programfixedemu[8];

		texid_t tex_gbuf[GBUFFER_COUNT];
		int fbo_current;	//the one currently being rendered to
		texid_t tex_sourcecol; /*this is used by $sourcecolour tgen*/
		texid_t tex_sourcedepth;
		texid_t tex_reflectcube;/*used where $reflectcube was invalid or failed*/
		fbostate_t fbo_2dfbo;
		fbostate_t fbo_reflectrefrac[R_MAX_RECURSE];
		fbostate_t fbo_lprepass;
		texid_t tex_reflection[R_MAX_RECURSE];	/*basically a portal rendered to texture*/
		texid_t tex_refraction[R_MAX_RECURSE];	/*the (culled) underwater view*/
		texid_t tex_refractiondepth[R_MAX_RECURSE];	/*the (culled) underwater view*/
		texid_t tex_ripplemap[R_MAX_RECURSE];	/*temp image for waves and things.*/

		int curpatchverts;
		qboolean force2d;
		int currenttmu;
		int blendmode[SHADER_TMU_MAX];
		int texenvmode[SHADER_TMU_MAX];
		int currenttextures[SHADER_TMU_MAX];
		GLenum curtexturetype[SHADER_TMU_MAX];

		polyoffset_t curpolyoffset;
		unsigned int curcull;
		texid_t curshadowmap;

		unsigned int shaderbits;
		unsigned int sha_attr;
		int currentprogram;
		int lastuniform; /*program which was last set, so using the same prog for multiple surfaces on the same ent (ie: world) does not require lots of extra uniform chnges*/

		batch_t dummybatch;
		vbo_t dummyvbo;
		int currentvbo;
		int currentebo;
		int currentvao;

		mesh_t **meshes;
		unsigned int meshcount;
		float modelmatrix[16];
		float modelmatrixinv[16];
		float modelviewmatrix[16];
		float projectionmatrix[16];

		int colourarraytype;
		vec4_t pendingcolourflat;
		int pendingcolourvbo;
		void *pendingcolourpointer;
		int curcolourvbo;
		void *curcolourpointer;

		int pendingvertexvbo;
		void *pendingvertexpointer;
		int curvertexvbo;
		void *curvertexpointer;

		int streamvbo[64];
		int streamebo[64];
		int streamvao[64];
		int streamid;

		int pendingtexcoordparts[SHADER_TMU_MAX];
		int pendingtexcoordvbo[SHADER_TMU_MAX];
		void *pendingtexcoordpointer[SHADER_TMU_MAX];

		float identitylighting;	//set to how bright world lighting should be (reduced by realtime_world_lightmaps)
		float identitylightmap;	//set to how bright lightmaps should be (reduced by overbrights+realtime_world_lightmaps)
		polyoffset_t polyoffset; //mode-specific polygon offsets...

		texid_t temptexture; //$current
		texid_t fogtexture;
		texid_t normalisationcubemap;
		float fogfar;
		int usingweaponviewmatrix;

		batch_t **mbatches;	//model batches (ie: not world)
	};

	//exterior state (paramters)
	struct {
		backendmode_t mode;
		unsigned int flags;
		int oldwidth, oldheight;

		vbo_t *sourcevbo;
		const material_t *curshader;
		const entity_t *curentity;
		const batch_t *curbatch;
		const texnums_t *curtexnums;
		const mfog_t *fog;
		const dlight_t *curdlight;

		float curtime;
		float updatetime;

		int lightmode;
		vec3_t lightorg;
		vec3_t lightdir;
		vec3_t lightcolours;
		vec3_t lightcolourscale;
		float lightradius;
		texid_t lighttexture;
		texid_t lightcubemap;
		float lightprojmatrix[16]; /*world space*/
		vec2_t lightshadowmapscale;
		vec4_t lightshadowmapinfo;
		vec4_t lightshadowmapproj;
	};

	int wbatch;
	int maxwbatches;
	batch_t *wbatches;
} shaderstate;

#ifdef _DEBUG
#define DRAWCALL(f) if (sh_config.showbatches) BE_PrintDrawCall(f)
#include "pr_common.h"
static void BE_PrintDrawCall(const char *msg)
{
	char shadername[512];
	char modelname[512];
	Q_snprintfz(shadername, sizeof(shadername), "^[%-16s\\tipimg\\%s\\tipimgtype\\%i\\tip\\%s^]",
			shaderstate.curshader->name,
			shaderstate.curshader->name,shaderstate.curshader->usageflags,
			shaderstate.curshader->name);

	if (shaderstate.curbatch && shaderstate.curbatch->ent)
	{
#ifdef HAVE_SERVER
		int num = shaderstate.curbatch->ent->keynum;
#endif
		if (shaderstate.curbatch->ent->model)
			Q_snprintfz(modelname, sizeof(modelname), " - ^[%s\\modelviewer\\%s^]",
				shaderstate.curbatch->ent->model->name, shaderstate.curbatch->ent->model->name);
		else
			*modelname = 0;
#ifdef HAVE_SERVER
		if (num >= 1 && num < sv.world.num_edicts)
			Con_Printf("%s shader %s ent %i%s - \"%s\"\n", msg, shadername, shaderstate.curbatch->ent->keynum, modelname, sv.world.progs->StringToNative(sv.world.progs, (WEDICT_NUM_PB(sv.world.progs, num))->v->classname));
		else
#endif
			Con_Printf("%s shader %s ent %i%s\n", msg, shadername, shaderstate.curbatch->ent->keynum, modelname);
	}
	else
		Con_Printf("%s shader %s\n", msg, shadername);
}
#else
#define DRAWCALL(f)
#endif

static void BE_PolyOffset(void)
{
	polyoffset_t po;
	po.factor = shaderstate.curshader->polyoffset.factor;
	po.unit = shaderstate.curshader->polyoffset.unit;

#ifdef BEF_PUSHDEPTH
	if (shaderstate.flags & BEF_PUSHDEPTH)
	{
		/*some quake doors etc are flush with the walls that they're meant to be hidden behind, or plats the same height as the floor, etc
		we move them back very slightly using polygonoffset to avoid really ugly z-fighting*/
		extern cvar_t r_polygonoffset_submodel_offset, r_polygonoffset_submodel_factor;
		po.factor += r_polygonoffset_submodel_factor.value;
		po.unit += r_polygonoffset_submodel_offset.value;
	}
#endif
	po.factor += shaderstate.polyoffset.factor;
	po.unit += shaderstate.polyoffset.unit;

#ifndef FORCESTATE
	if (shaderstate.curpolyoffset.factor != po.factor || shaderstate.curpolyoffset.unit != po.unit)
#endif
	{
		shaderstate.curpolyoffset = po;
		if (shaderstate.curpolyoffset.factor || shaderstate.curpolyoffset.unit)
		{
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglPolygonOffset(shaderstate.curpolyoffset.factor, shaderstate.curpolyoffset.unit);
		}
		else
			qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void GLBE_PolyOffsetStencilShadow
					#ifdef BEF_PUSHDEPTH
							(qboolean pushdepth)
					#else
							(void)
					#endif
{
	extern cvar_t r_polygonoffset_stencil_offset, r_polygonoffset_stencil_factor;
	polyoffset_t po;
	po.factor = r_polygonoffset_stencil_factor.value;
	po.unit = r_polygonoffset_stencil_offset.value;

#ifdef BEF_PUSHDEPTH
	if (pushdepth)
	{
		/*some quake doors etc are flush with the walls that they're meant to be hidden behind, or plats the same height as the floor, etc
		we move them back very slightly using polygonoffset to avoid really ugly z-fighting*/
		extern cvar_t r_polygonoffset_submodel_offset, r_polygonoffset_submodel_factor;
		po.factor += r_polygonoffset_submodel_factor.value;
		po.unit += r_polygonoffset_submodel_offset.value;
	}
#endif

#ifndef FORCESTATE
	if (shaderstate.curpolyoffset.factor != po.factor || shaderstate.curpolyoffset.unit != po.unit)
#endif
	{
		shaderstate.curpolyoffset = po;
		if (shaderstate.curpolyoffset.factor || shaderstate.curpolyoffset.unit)
		{
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglPolygonOffset(shaderstate.curpolyoffset.factor, shaderstate.curpolyoffset.unit);
		}
		else
			qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}
static void GLBE_PolyOffsetShadowMap(void)
{
	extern cvar_t r_polygonoffset_shadowmap_offset, r_polygonoffset_shadowmap_factor;
	polyoffset_t po;
#if 0//def BEF_PUSHDEPTH
	if (pushdepth)
	{
		/*some quake doors etc are flush with the walls that they're meant to be hidden behind, or plats the same height as the floor, etc
		we move them back very slightly using polygonoffset to avoid really ugly z-fighting*/
		extern cvar_t r_polygonoffset_submodel_offset, r_polygonoffset_submodel_factor;
		po.factor = r_polygonoffset_submodel_factor.value + r_polygonoffset_shadowmap_factor.value;
		po.unit = r_polygonoffset_submodel_offset.value + r_polygonoffset_shadowmap_offset.value;
	}
	else
#endif
	{
		po.factor = r_polygonoffset_shadowmap_factor.value;
		po.unit = r_polygonoffset_shadowmap_offset.value;
	}

#ifndef FORCESTATE
	if (shaderstate.curpolyoffset.factor != po.factor || shaderstate.curpolyoffset.unit != po.unit)
#endif
	{
		shaderstate.curpolyoffset = po;
		if (shaderstate.curpolyoffset.factor || shaderstate.curpolyoffset.unit)
		{
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglPolygonOffset(shaderstate.curpolyoffset.factor, shaderstate.curpolyoffset.unit);
		}
		else
			qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

#ifndef GLSLONLY
void GL_TexEnv(GLenum mode)
{
#ifndef FORCESTATE
	if (mode != shaderstate.texenvmode[shaderstate.currenttmu])
#endif
	{
		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		shaderstate.texenvmode[shaderstate.currenttmu] = mode;
	}
}

static void BE_SetPassBlendMode(int tmu, int pbm)
{
#ifndef FORCESTATE
	if (shaderstate.blendmode[tmu] != pbm)
#endif
	{
		shaderstate.blendmode[tmu] = pbm;
#ifndef FORCESTATE
		if (shaderstate.currenttmu != tmu)
#endif
			GL_SelectTexture(tmu);

		switch (pbm)
		{
		case PBM_DOTPRODUCT:
			GL_TexEnv(GL_COMBINE_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB_ARB);
			qglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
			break;
		case PBM_MODULATE_PREV_COLOUR:
			GL_TexEnv(GL_COMBINE_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			qglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
			break;
		case PBM_REPLACELIGHT:
		//	if (shaderstate.identitylighting != 1)
				goto forcemod;
			GL_TexEnv(GL_REPLACE);
			break;
		case PBM_REPLACE:
			GL_TexEnv(GL_REPLACE);
			break;
		case PBM_DECAL:
			if (tmu == 0)
				goto forcemod;
			GL_TexEnv(GL_DECAL);
			break;
		case PBM_ADD:
			if (tmu == 0)
				goto forcemod;
			GL_TexEnv(GL_ADD);
			break;
		case PBM_OVERBRIGHT:
			GL_TexEnv(GL_COMBINE_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			qglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			qglTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1<<gl_overbright.ival);
			break;
		default:
		case PBM_MODULATE:
		forcemod:
			GL_TexEnv(GL_MODULATE);
			break;
		}
	}
}
#endif

/*OpenGL requires glDepthMask(GL_TRUE) or glClear(GL_DEPTH_BUFFER_BIT) will fail*/
void GL_ForceDepthWritable(void)
{
#ifndef FORCESTATE
	if (!(shaderstate.shaderbits & SBITS_MISC_DEPTHWRITE))
#endif
	{
		shaderstate.shaderbits |= SBITS_MISC_DEPTHWRITE;
		qglDepthMask(GL_TRUE);
	}

	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	shaderstate.shaderbits |= SBITS_MASK_BITS;
}

void GL_SetShaderState2D(qboolean is2d)
{
	shaderstate.usingweaponviewmatrix = -1;	//force projection matrix info to get reset
	shaderstate.updatetime = realtime;
	shaderstate.force2d = is2d;
	if (is2d)
	{
		memcpy(shaderstate.modelviewmatrix, r_refdef.m_view, sizeof(shaderstate.modelviewmatrix));
		if (qglLoadMatrixf)
			qglLoadMatrixf(r_refdef.m_view);
	}
	BE_SelectMode(BEM_STANDARD);


//	if (cl.paused || cls.state < ca_active)
		shaderstate.updatetime = r_refdef.time;
//	else
//		shaderstate.updatetime = cl.servertime;
	BE_SelectEntity(&r_worldentity);
	shaderstate.curtime = shaderstate.updatetime - shaderstate.curentity->shaderTime;
}

void GL_SelectTexture(int target)
{
	shaderstate.currenttmu = target;
	if (qglActiveTextureARB)
		qglActiveTextureARB(target + mtexid0);
}

void GL_SelectVBO(int vbo)
{
#ifndef FORCESTATE
	if (shaderstate.currentvbo != vbo)
#endif
	{
		shaderstate.currentvbo = vbo;
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, shaderstate.currentvbo);
	}
}
void GL_DeselectVAO(void)
{
	if (shaderstate.currentvao)
	{
		qglBindVertexArray(0);
		shaderstate.currentvao = 0;
	}
}
void GL_SelectEBO(int vbo)
{
	//EBO is part of the current VAO, so keep things matching that
#ifndef FORCESTATE
	if (shaderstate.currentebo != vbo)
#endif
	{
		shaderstate.currentebo = vbo;
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, shaderstate.currentebo);
	}
}

void GL_MTBind(int tmu, int target, texid_t texnum)
{
	GL_SelectTexture(tmu);

#ifndef FORCESTATE
	if (shaderstate.currenttextures[tmu] == texnum->num)
		return;
#endif

	shaderstate.currenttextures[tmu] = texnum->num;
	if (target)
		qglBindTexture (target, texnum->num);

	if (
#ifndef FORCESTATE
	shaderstate.curtexturetype[tmu] != target &&
#endif
 !gl_config_nofixedfunc)
	{

		if (shaderstate.curtexturetype[tmu])
			qglDisable(shaderstate.curtexturetype[tmu]);
		shaderstate.curtexturetype[tmu] = target;
		if (target)
			qglEnable(target);
	}
}

#if 0//def GLSLONLY
static void GL_LazyBind(int tmu, texid_t texnum)
{
	int glnum = texnum?texnum->num:0;
	if (shaderstate.currenttextures[tmu] != glnum)
	{
		qglBindTextureUnit(tmu, glnum);
		shaderstate.currenttextures[tmu] = glnum;
	}
}
#else
static void GL_LazyBind(int tmu, texid_t texnum)
{
	int glnum;
	GLenum target;
	if (texnum)
	{
		static GLenum imgtab[] = {GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY_ARB};
		glnum = texnum->num, target = imgtab[(enum imgtype_e)((texnum->flags & IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT)];
	}
	else
		glnum = 0, target = 0;

#ifndef FORCESTATE
	if (shaderstate.currenttextures[tmu] != glnum)
#endif
	{
		GL_SelectTexture(tmu);

		shaderstate.currenttextures[shaderstate.currenttmu] = glnum;

#ifndef FORCESTATE
		if (shaderstate.curtexturetype[tmu] != target)
#endif
		{
			if (gl_config_nofixedfunc)
			{
				shaderstate.curtexturetype[tmu] = target;
			}
			else
			{
				if (shaderstate.curtexturetype[tmu])
				{
					qglBindTexture (shaderstate.curtexturetype[tmu], 0);
					qglDisable(shaderstate.curtexturetype[tmu]);
				}
				shaderstate.curtexturetype[tmu] = target;
				if (target)
					qglEnable(target);
			}
		}

		if (target)
			qglBindTexture (target, glnum);
	}
}
#endif

static void BE_ApplyAttributes(unsigned int bitstochange, unsigned int bitstoendisable)
{
	unsigned int i;

#ifndef GLSLONLY
	//legacy colour attribute (including flat shaded)
	if ((bitstochange) & (1u<<VATTR_LEG_COLOUR))
	{
		if (!shaderstate.pendingcolourpointer && !shaderstate.pendingcolourvbo)
		{
			if (shaderstate.curcolourpointer || shaderstate.curcolourvbo)
			{
				qglShadeModel(GL_FLAT);
				qglDisableClientState(GL_COLOR_ARRAY);
			}
			shaderstate.curcolourpointer = NULL;
			shaderstate.curcolourvbo = 0;
			qglColor4fv(shaderstate.pendingcolourflat);
		}
		else
		{
		#ifndef FORCESTATE
			if (shaderstate.curcolourpointer != shaderstate.pendingcolourpointer || shaderstate.pendingcolourvbo != shaderstate.curcolourvbo)
		#endif
			{
				if (!shaderstate.curcolourpointer && !shaderstate.curcolourvbo)
				{
					if (qglShadeModel)
						qglShadeModel(GL_SMOOTH);
					bitstoendisable |= (1u<<VATTR_LEG_COLOUR);
				}
				shaderstate.curcolourpointer = shaderstate.pendingcolourpointer;
				shaderstate.curcolourvbo = shaderstate.pendingcolourvbo;
				GL_SelectVBO(shaderstate.curcolourvbo);
				qglColorPointer(4, shaderstate.colourarraytype, 0, shaderstate.curcolourpointer);
			}

			if ((bitstoendisable) & (1u<<VATTR_LEG_COLOUR))
			{
				qglEnableClientState(GL_COLOR_ARRAY);
			}
		}
	}
	else
	{
		if ((bitstoendisable) & (1u<<VATTR_LEG_COLOUR))
		{
			qglDisableClientState(GL_COLOR_ARRAY);
		}
	}

	//legacy tmus
	if ((bitstoendisable|bitstochange) >= (1u<<VATTR_LEG_TMU0))
	{
		for (i = VATTR_LEG_TMU0; (bitstoendisable|bitstochange) >= (1u<<i); i++)
		{
			if ((bitstochange) & (1u<<i))
			{
				qglClientActiveTextureARB(i-VATTR_LEG_TMU0 + mtexid0);
				if (bitstoendisable & (1u<<i))
					qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
				GL_SelectVBO(shaderstate.pendingtexcoordvbo[i-VATTR_LEG_TMU0]);
				qglTexCoordPointer(shaderstate.pendingtexcoordparts[i-VATTR_LEG_TMU0], GL_FLOAT, 0, shaderstate.pendingtexcoordpointer[i-VATTR_LEG_TMU0]);
			}

#ifndef FORCESTATE
			else if (bitstoendisable & (1u<<i))
#endif
			{
				qglClientActiveTextureARB(i-VATTR_LEG_TMU0 + mtexid0);
				if (bitstochange & (1u<<i))
					qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
				else
					qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}
	}

	//legacy vertex coords
	if ((bitstochange) & (1u<<VATTR_LEG_VERTEX))
	{
	#ifndef FORCESTATE
		if (shaderstate.currentvao || shaderstate.curvertexpointer != shaderstate.pendingvertexpointer || shaderstate.pendingvertexvbo != shaderstate.curvertexvbo)
	#endif
		{
			shaderstate.curvertexpointer = shaderstate.pendingvertexpointer;
			shaderstate.curvertexvbo = shaderstate.pendingvertexvbo;
			GL_SelectVBO(shaderstate.curvertexvbo);
			qglVertexPointer(3, GL_FLOAT, VECV_STRIDE, shaderstate.curvertexpointer);
		}
		if ((bitstoendisable) & (1u<<VATTR_LEG_VERTEX))
		{
			qglEnableClientState(GL_VERTEX_ARRAY);
		}
	}
	else
	{
		if ((bitstoendisable) & (1u<<VATTR_LEG_VERTEX))
		{
			qglDisableClientState(GL_VERTEX_ARRAY);
		}
	}
#endif

	if (!((bitstochange|bitstoendisable) & ((1u<<VATTR_LEG_FIRST)-1)))
		return;

	for (i = 0; i < VATTR_LEG_FIRST; i++)
	{
		if ((bitstochange) & (1u<<i))
		{
			switch (i)
			{
			case VATTR_VERTEX1:
				/*we still do vertex transforms for billboards and shadows and such*/
				GL_SelectVBO(shaderstate.pendingvertexvbo);
				qglVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, VECV_STRIDE, shaderstate.pendingvertexpointer);
				break;
			case VATTR_VERTEX2:
				if (!shaderstate.sourcevbo->coord2.gl.vbo && !shaderstate.sourcevbo->coord2.gl.addr)
				{
					GL_SelectVBO(shaderstate.pendingvertexvbo);
					qglVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, VECV_STRIDE, shaderstate.pendingvertexpointer);
				}
				else
				{
					GL_SelectVBO(shaderstate.sourcevbo->coord2.gl.vbo);
					qglVertexAttribPointer(VATTR_VERTEX2, 3, GL_FLOAT, GL_FALSE, VECV_STRIDE, shaderstate.sourcevbo->coord2.gl.addr);
				}
				break;
			case VATTR_COLOUR:
				if (!shaderstate.pendingcolourvbo && !shaderstate.pendingcolourpointer)
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					qglVertexAttrib4f(VATTR_COLOUR, 1, 1, 1, 1);
					continue;
				}
				GL_SelectVBO(shaderstate.pendingcolourvbo);
				qglVertexAttribPointer(VATTR_COLOUR, 4, shaderstate.colourarraytype, ((shaderstate.colourarraytype==GL_FLOAT)?GL_FALSE:GL_TRUE), 0, shaderstate.pendingcolourpointer);
				break;
#if MAXRLIGHTMAPS > 1
			case VATTR_COLOUR2:
				GL_SelectVBO(shaderstate.sourcevbo->colours[1].gl.vbo);
				qglVertexAttribPointer(VATTR_COLOUR2, 4, shaderstate.colourarraytype, ((shaderstate.colourarraytype==GL_FLOAT)?GL_FALSE:GL_TRUE), 0, shaderstate.sourcevbo->colours[1].gl.addr);
				break;
			case VATTR_COLOUR3:
				GL_SelectVBO(shaderstate.sourcevbo->colours[2].gl.vbo);
				qglVertexAttribPointer(VATTR_COLOUR3, 4, shaderstate.colourarraytype, ((shaderstate.colourarraytype==GL_FLOAT)?GL_FALSE:GL_TRUE), 0, shaderstate.sourcevbo->colours[2].gl.addr);
				break;
			case VATTR_COLOUR4:
				GL_SelectVBO(shaderstate.sourcevbo->colours[3].gl.vbo);
				qglVertexAttribPointer(VATTR_COLOUR4, 4, shaderstate.colourarraytype, ((shaderstate.colourarraytype==GL_FLOAT)?GL_FALSE:GL_TRUE), 0, shaderstate.sourcevbo->colours[3].gl.addr);
				break;
#endif
			case VATTR_TEXCOORD:
				if (!shaderstate.pendingtexcoordvbo[0] && !shaderstate.pendingtexcoordpointer[0])
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					continue;
				}
				GL_SelectVBO(shaderstate.pendingtexcoordvbo[0]);
				qglVertexAttribPointer(VATTR_TEXCOORD, shaderstate.pendingtexcoordparts[0], GL_FLOAT, GL_FALSE, 0, shaderstate.pendingtexcoordpointer[0]);
				break;
			case VATTR_LMCOORD:
				if (!shaderstate.sourcevbo->lmcoord[0].gl.vbo && !shaderstate.sourcevbo->lmcoord[0].gl.addr)
				{
					GL_SelectVBO(shaderstate.sourcevbo->texcoord.gl.vbo);
					qglVertexAttribPointer(VATTR_LMCOORD, 2, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->texcoord.gl.addr);
				}
				else
				{
					GL_SelectVBO(shaderstate.sourcevbo->lmcoord[0].gl.vbo);
					qglVertexAttribPointer(VATTR_LMCOORD, 2, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->lmcoord[0].gl.addr);
				}
				break;
#if MAXRLIGHTMAPS > 1
			case VATTR_LMCOORD2:
				GL_SelectVBO(shaderstate.sourcevbo->lmcoord[1].gl.vbo);
				qglVertexAttribPointer(VATTR_LMCOORD2, 2, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->lmcoord[1].gl.addr);
				break;
			case VATTR_LMCOORD3:
				GL_SelectVBO(shaderstate.sourcevbo->lmcoord[2].gl.vbo);
				qglVertexAttribPointer(VATTR_LMCOORD3, 2, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->lmcoord[2].gl.addr);
				break;
			case VATTR_LMCOORD4:
				GL_SelectVBO(shaderstate.sourcevbo->lmcoord[3].gl.vbo);
				qglVertexAttribPointer(VATTR_LMCOORD4, 2, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->lmcoord[3].gl.addr);
				break;
#endif
			case VATTR_NORMALS:
				if (!shaderstate.sourcevbo->normals.gl.addr)
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					continue;
				}
				GL_SelectVBO(shaderstate.sourcevbo->normals.gl.vbo);
				qglVertexAttribPointer(VATTR_NORMALS, 3, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->normals.gl.addr);
				break;
			case VATTR_SNORMALS:
				if (!shaderstate.sourcevbo->svector.gl.addr)
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					continue;
				}
				GL_SelectVBO(shaderstate.sourcevbo->svector.gl.vbo);
				qglVertexAttribPointer(VATTR_SNORMALS, 3, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->svector.gl.addr);
				break;
			case VATTR_TNORMALS:
				if (!shaderstate.sourcevbo->tvector.gl.addr)
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					continue;
				}
				GL_SelectVBO(shaderstate.sourcevbo->tvector.gl.vbo);
				qglVertexAttribPointer(VATTR_TNORMALS, 3, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->tvector.gl.addr);
				break;
			case VATTR_BONENUMS:
				if (!shaderstate.sourcevbo->bonenums.gl.vbo && !shaderstate.sourcevbo->bonenums.gl.addr)
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					continue;
				}
				GL_SelectVBO(shaderstate.sourcevbo->bonenums.gl.vbo);
				qglVertexAttribPointer(VATTR_BONENUMS, 4, GL_BONE_INDEX_TYPE, GL_FALSE, 0, shaderstate.sourcevbo->bonenums.gl.addr);
				break;
			case VATTR_BONEWEIGHTS:
				if (!shaderstate.sourcevbo->boneweights.gl.vbo && !shaderstate.sourcevbo->boneweights.gl.addr)
				{
					shaderstate.sha_attr &= ~(1u<<i);
					qglDisableVertexAttribArray(i);
					continue;
				}
				GL_SelectVBO(shaderstate.sourcevbo->boneweights.gl.vbo);
				qglVertexAttribPointer(VATTR_BONEWEIGHTS, 4, GL_FLOAT, GL_FALSE, 0, shaderstate.sourcevbo->boneweights.gl.addr);
				break;
			}
			if ((bitstoendisable) & (1u<<i))
			{
				qglEnableVertexAttribArray(i);
			}
		}
		else
		{
			if ((bitstoendisable) & (1u<<i))
			{
				qglDisableVertexAttribArray(i);
			}
		}
	}
}

static void BE_EnableShaderAttributes(unsigned int progattrmask, int usevao)
{
	unsigned int bitstochange, bitstoendisable;

	if (shaderstate.currentvao != usevao)
	{
		shaderstate.currentvao = usevao;
		qglBindVertexArray(usevao); 
	}

	if (shaderstate.currentvao)
	{
		bitstochange = shaderstate.sourcevbo->vaodynamic&progattrmask;
#if 0
		bitstoendisable = 0;
#else
		bitstoendisable = shaderstate.sourcevbo->vaoenabled^progattrmask;
		if (bitstoendisable)
			bitstochange |= bitstoendisable;
		shaderstate.sourcevbo->vaoenabled = progattrmask;
#endif

		if (bitstochange & (1u<<VATTR_LEG_ELEMENTS))
		{
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, shaderstate.sourcevbo->indicies.gl.vbo);
		}
	}
	else
	{
		bitstochange = progattrmask;
		bitstoendisable = progattrmask^shaderstate.sha_attr;

		shaderstate.sha_attr = progattrmask;

/*
#ifndef FORCESTATE
		if (shaderstate.currentebo != shaderstate.sourcevbo->indicies.gl.vbo)
#endif
		{
			shaderstate.currentebo = shaderstate.sourcevbo->indicies.gl.vbo;
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, shaderstate.currentebo);
		}
*/
	}

	if (bitstochange || bitstoendisable)
		BE_ApplyAttributes(bitstochange, bitstoendisable);
}

void GLBE_SetupVAO(vbo_t *vbo, unsigned int vaodynamic, unsigned int vaostatic)
{
	if (qglGenVertexArrays)
	{
		qglGenVertexArrays(1, &vbo->vao);

		if ((vaostatic & VATTR_VERTEX1) && !gl_config_nofixedfunc)
			vaostatic = (vaostatic & ~VATTR_VERTEX1) | VATTR_LEG_VERTEX;
		if ((vaodynamic & VATTR_VERTEX1) && !gl_config_nofixedfunc)
			vaodynamic = (vaodynamic & ~VATTR_VERTEX1) | VATTR_LEG_VERTEX;

		shaderstate.curvertexpointer = NULL;
		shaderstate.curvertexvbo = 0;

		shaderstate.sourcevbo = vbo;
		shaderstate.pendingvertexvbo = shaderstate.sourcevbo->coord.gl.vbo;
		shaderstate.pendingvertexpointer = shaderstate.sourcevbo->coord.gl.addr;
		shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
		shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
		shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
		shaderstate.pendingtexcoordparts[0] = 2;

		shaderstate.currentvao = vbo->vao;
		qglBindVertexArray(vbo->vao);
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, shaderstate.sourcevbo->indicies.gl.vbo);
		BE_ApplyAttributes(vaostatic, vaodynamic|vaostatic);
		GL_SelectVBO(shaderstate.sourcevbo->coord.gl.vbo);
		vbo->vaoenabled = vaodynamic|vaostatic;
		vbo->vaodynamic = vaodynamic;

		shaderstate.curvertexpointer = NULL;
		shaderstate.curvertexvbo = 0;
	}
	else
	{
		GL_DeselectVAO();

		/*always select the coord vbo and indicies ebo, for easy bufferdata*/
		GL_SelectEBO(vbo->indicies.gl.vbo);
		GL_SelectVBO(vbo->coord.gl.vbo);
	}
}

void GL_SelectProgram(GLuint program)
{
	if (shaderstate.currentprogram != program)
	{
		qglUseProgramObjectARB(program);
		shaderstate.currentprogram = program;
	}
}
static void GL_DeSelectProgram(void)
{
	if (shaderstate.currentprogram != 0)
	{
		qglUseProgramObjectARB(0);
		shaderstate.currentprogram = 0;
	}
}


void GLBE_RenderShadowBuffer(unsigned int numverts, int vbo, vecV_t *verts, unsigned numindicies, int ibo, index_t *indicies)
{
	shaderstate.pendingvertexvbo = vbo;
	shaderstate.pendingvertexpointer = verts;

	shaderstate.sourcevbo = &shaderstate.dummyvbo;
	shaderstate.dummyvbo.indicies.gl.vbo = ibo;

	if (shaderstate.mode != BEM_STENCIL)
		GLBE_PolyOffsetShadowMap();

	if (shaderstate.allblackshader.glsl.handle)
	{
		GL_SelectProgram(shaderstate.allblackshader.glsl.handle);

		BE_EnableShaderAttributes(gl_config_nofixedfunc?(1u<<VATTR_VERTEX1):(1u<<VATTR_LEG_VERTEX), 0);

		if (shaderstate.allblackshader.glsl.handle != shaderstate.lastuniform && shaderstate.allblack_mvp != -1)
		{
			float m16[16];
			Matrix4_Multiply(shaderstate.projectionmatrix, shaderstate.modelviewmatrix, m16);
			qglUniformMatrix4fvARB(shaderstate.allblack_mvp, 1, false, m16);
		}
		shaderstate.lastuniform = shaderstate.allblackshader.glsl.handle;

		GL_SelectEBO(ibo);
		qglDrawRangeElements(GL_TRIANGLES, 0, numverts - 1, numindicies, GL_INDEX_TYPE, indicies);
	}
	else
	{
		GL_DeSelectProgram();
		BE_EnableShaderAttributes((1u<<VATTR_LEG_VERTEX), 0);

		//draw cached world shadow mesh
		GL_SelectEBO(ibo);
		qglDrawRangeElements(GL_TRIANGLES, 0, numverts - 1, numindicies, GL_INDEX_TYPE, indicies);
	}
	RQuantAdd(RQUANT_DRAWS, 1);
	DRAWCALL("GLBE_RenderShadowBuffer");
	RQuantAdd(RQUANT_SHADOWINDICIES, numindicies);
	shaderstate.dummyvbo.indicies.gl.vbo = 0;
	shaderstate.sourcevbo = NULL;
}

void GL_CullFace(unsigned int sflags)
{
	if (shaderstate.flags & BEF_FORCETWOSIDED)
		sflags = 0;
	else if (sflags)
		sflags ^= r_refdef.flipcull;

#ifndef FORCESTATE
	if (shaderstate.curcull == sflags)
		return;
#endif
	shaderstate.curcull = sflags;

	if (shaderstate.curcull & SHADER_CULL_FRONT)
	{
		qglEnable(GL_CULL_FACE);
		qglCullFace(GL_FRONT);
	}
	else if (shaderstate.curcull & SHADER_CULL_BACK)
	{
		qglEnable(GL_CULL_FACE);
		qglCullFace(GL_BACK);
	}
	else
	{
		qglDisable(GL_CULL_FACE);
	}
}

static void R_FetchPlayerColour(unsigned int cv, vec3_t rgb)
{
	int i;

	if (cv >= 16)
	{
		rgb[0] = (((cv&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[15]+0)) / (256.0*256);
		rgb[1] = (((cv&0x00ff00)>>8)**((unsigned char*)&d_8to24rgbtable[15]+1)) / (256.0*256);
		rgb[2] = (((cv&0x0000ff)>>0)**((unsigned char*)&d_8to24rgbtable[15]+2)) / (256.0*256);
		return;
	}
	i = cv;
	if (i >= 8)
	{
		i<<=4;
	}
	else
	{
		i<<=4;
		i+=15;
	}
	i*=3;
	rgb[0] = host_basepal[i+0] / 255.0;
	rgb[1] = host_basepal[i+1] / 255.0;
	rgb[2] = host_basepal[i+2] / 255.0;
/*	if (!gammaworks)
	{
		*retred = gammatable[*retred];
		*retgreen = gammatable[*retgreen];
		*retblue = gammatable[*retblue];
	}*/
}

#ifdef RTLIGHTS
void GLBE_SetupForShadowMap(dlight_t *dl, int texwidth, int texheight, float shadowscale)
{
	extern cvar_t r_shadow_shadowmapping_bias;
	extern cvar_t r_shadow_shadowmapping_nearclip;
	float n = dl->nearclip?dl->nearclip:r_shadow_shadowmapping_nearclip.value;
	float f = dl->radius;
	float b = r_shadow_shadowmapping_bias.value;

	shaderstate.lightshadowmapproj[0] = shadowscale * (1.0-(1.0/texwidth)) * 0.5/3.0;
	shaderstate.lightshadowmapproj[1] = shadowscale * (1.0-(1.0/texheight)) * 0.5/2.0;
	shaderstate.lightshadowmapproj[2] = 0.5*(f+n)/(n-f);
	shaderstate.lightshadowmapproj[3] = (f*n)/(n-f) - b*n*(1024/texheight);

	shaderstate.lightshadowmapscale[0] = 1.0/texwidth;
	shaderstate.lightshadowmapscale[1] = 1.0/texheight;
}

int GLBE_BeginRenderBuffer_DepthOnly(texid_t depthtexture);
qboolean GLBE_BeginShadowMap(int id, int w, int h, uploadfmt_t encoding, int *restorefbo)
{
	if (!gl_config.ext_framebuffer_objects)
		return false;

	if (!TEXVALID(shadowmap[id]) || shadowmap[id]->width != w || shadowmap[id]->height != h || shadowmap[id]->format != encoding || shadowmap[id]->status != TEX_LOADED)
	{
		texid_t tex;
		if (shadowmap[id])
			Image_DestroyTexture(shadowmap[id]);
		tex = shadowmap[id] = Image_CreateTexture(va("***shadowmap2d%i***", id), NULL, IF_NOPURGE|IF_CLAMP|IF_NOMIPMAP|IF_RENDERTARGET);
		tex->width = w;
		tex->height = h;
		tex->format = encoding;
		qglGenTextures(1, &tex->num);
		GL_MTBind(0, GL_TEXTURE_2D, tex);
#ifdef SHADOWDBG_COLOURNOTDEPTH
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#else
		if (qglTexStorage2D)
			qglTexStorage2D(GL_TEXTURE_2D, 1, gl_config.formatinfo[encoding].sizedformat, w, h);
		else if (gl_config.formatinfo[encoding].type)
			qglTexImage2D				(GL_TEXTURE_2D, 0, gl_config.formatinfo[encoding].sizedformat, w, h, 0, gl_config.formatinfo[encoding].format, gl_config.formatinfo[encoding].type,	NULL);
		else
			qglCompressedTexImage2D		(GL_TEXTURE_2D, 0, gl_config.formatinfo[encoding].sizedformat, w, h, 0,	0, NULL);
#endif

		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#if 0//def SHADOWDBG_COLOURNOTDEPTH
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#else
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
		//in case we're using shadow samplers
		if (gl_config.arb_shadow)
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			//qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		}
		tex->status = TEX_LOADED;
	}
	shaderstate.curshadowmap = shadowmap[id];

	/*set framebuffer*/
	*restorefbo = GLBE_BeginRenderBuffer_DepthOnly(shaderstate.curshadowmap);

	shaderstate.usingweaponviewmatrix = -1;		//make sure the projection matrix is updated.

	while(shaderstate.lastpasstmus>0)
	{
		GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
	}

	shaderstate.shaderbits &= ~SBITS_MISC_DEPTHWRITE;

	BE_SelectMode(BEM_DEPTHONLY);

	BE_Scissor(NULL);
	qglViewport(0, 0, w, h);
	GL_ForceDepthWritable();
	qglClear (GL_DEPTH_BUFFER_BIT);
#ifdef SHADOWDBG_COLOURNOTDEPTH
	qglColorMask(TRUE,TRUE,TRUE,TRUE);
	qglClearColor(1,1,1,1);
	qglClear (GL_COLOR_BUFFER_BIT);
#endif

	return true;
}
void GLBE_EndShadowMap(int restorefbo)
{
	GLBE_FBO_Pop(restorefbo);
	shaderstate.usingweaponviewmatrix = -1;		//make sure the projection matrix is updated.
}
#endif

static void T_Gen_CurrentRender(int tmu)
{
	int vwidth, vheight;
	int pwidth = vid.fbpwidth;
	int pheight = vid.fbpheight;
	GLenum fmt;
	if (r_refdef.recurse)
		return;

	if (sh_config.texture_non_power_of_two_pic)
	{
		vwidth = pwidth;
		vheight = pheight;
	}
	else
	{
		vwidth = 1;
		vheight = 1;
		while (vwidth < pwidth)
		{
			vwidth *= 2;
		}
		while (vheight < pheight)
		{
			vheight *= 2;
		}
	}

	if (vid.flags&VID_FP16)
		fmt = GL_RGBA16F;
	else if (vid.flags&VID_SRGB_FB)
		fmt = GL_SRGB8_EXT;
	else
		fmt = GL_RGB;

	// copy the scene to texture
	if (!TEXVALID(shaderstate.temptexture))
	{
		TEXASSIGN(shaderstate.temptexture, Image_CreateTexture("***$currentrender***", NULL, 0));
		qglGenTextures(1, &shaderstate.temptexture->num);
	}
	GL_MTBind(tmu, GL_TEXTURE_2D, shaderstate.temptexture);
	qglCopyTexImage2D(GL_TEXTURE_2D, 0, fmt, 0, 0, vwidth, vheight, 0);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void Shader_BindTextureForPass(int tmu, const shaderpass_t *pass)
{
	extern cvar_t gl_specular_fallback;

	texid_t t;
	switch(pass->texgen)
	{
	default:
	case T_GEN_SINGLEMAP:
		t = pass->anim_frames[0];
		break;
	case T_GEN_ANIMMAP:
		t = pass->anim_frames[(int)(pass->anim_fps * shaderstate.curtime) % pass->anim_numframes];
		break;
	case T_GEN_LIGHTMAP:
		if ((unsigned short)shaderstate.curbatch->lightmap[0] >= numlightmaps)
			t = r_whiteimage;
		else
			t = lightmap[shaderstate.curbatch->lightmap[0]]->lightmap_texture;
		break;
	case T_GEN_DELUXMAP:
		{
			unsigned int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi >= numlightmaps || !lightmap[lmi]->hasdeluxe)
				t = missing_texture_normal;
			else
				t = lightmap[lmi+1]->lightmap_texture;
		}
		break;
	case T_GEN_DIFFUSE:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->base))
			t = shaderstate.curtexnums->base;
		else
			t = missing_texture;
		break;
	case T_GEN_PALETTED:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->paletted))
			t = shaderstate.curtexnums->paletted;
		else
			t = r_whiteimage;
		break;
	case T_GEN_NORMALMAP:
		t = (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->bump))?shaderstate.curtexnums->bump:missing_texture_normal;
		break;
	case T_GEN_SPECULAR:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->specular))
			t = shaderstate.curtexnums->specular;
		else if (gl_specular_fallback.value<0 && shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->base))
			t = shaderstate.curtexnums->base;
		else
			t = missing_texture_gloss;
		break;
	case T_GEN_UPPEROVERLAY:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->upperoverlay))
			t = shaderstate.curtexnums->upperoverlay;
		else
			t = r_nulltex;
		break;
	case T_GEN_LOWEROVERLAY:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->loweroverlay))
			t = shaderstate.curtexnums->loweroverlay;
		else
			t = r_nulltex;
		break;
	case T_GEN_FULLBRIGHT:
		t = shaderstate.curtexnums->fullbright;
		break;
	case T_GEN_REFLECTCUBE:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->reflectcube))
			t = shaderstate.curtexnums->reflectcube;
		else if (shaderstate.curbatch->envmap)
			t = shaderstate.curbatch->envmap;
		else
			t = shaderstate.tex_reflectcube;
		break;
	case T_GEN_REFLECTMASK:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->reflectmask))
			t = shaderstate.curtexnums->reflectmask;
		else if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->base))
			t = shaderstate.curtexnums->base;
		else
			t = r_whiteimage;
		break;
	case T_GEN_DISPLACEMENT:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->displacement))
			t = shaderstate.curtexnums->displacement;
		else
			t = r_whiteimage;
		break;
	case T_GEN_OCCLUSION:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->occlusion))
			t = shaderstate.curtexnums->occlusion;
		else
			t = r_whiteimage;
		break;
	case T_GEN_TRANSMISSION:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->transmission))
			t = shaderstate.curtexnums->transmission;
		else
			t = r_whiteimage;
		break;
	case T_GEN_THICKNESS:
		if (shaderstate.curtexnums && TEXLOADED(shaderstate.curtexnums->thickness))
			t = shaderstate.curtexnums->thickness;
		else
			t = r_whiteimage;
		break;
	case T_GEN_SHADOWMAP:
		t = shaderstate.curshadowmap;
		break;

	case T_GEN_LIGHTCUBEMAP:
		t = shaderstate.lightcubemap;
		break;
	case T_GEN_SOURCECUBE:
		t = scenepp_postproc_cube;
		break;

#ifdef HAVE_MEDIA_DECODER
	case T_GEN_VIDEOMAP:
		t = Media_UpdateForShader(pass->cin);
		if (!TEXLOADED(t))
			t = shaderstate.curtexnums?shaderstate.curtexnums->base:r_nulltex;
		break;
#endif

	case T_GEN_CURRENTRENDER:
		T_Gen_CurrentRender(tmu);
		return;

	case T_GEN_SOURCECOLOUR:
		t = shaderstate.tex_sourcecol;
		break;
	case T_GEN_SOURCEDEPTH:
		t = shaderstate.tex_sourcedepth;
		break;
	case T_GEN_REFLECTION:
		t = shaderstate.tex_reflection[r_refdef.recurse];
		break;
	case T_GEN_REFRACTION:
		if (!r_refract_fboival)
		{
			T_Gen_CurrentRender(tmu);
			return;
		}
		t = shaderstate.tex_refraction[r_refdef.recurse];
		break;
	case T_GEN_REFRACTIONDEPTH:
		t = shaderstate.tex_refractiondepth[r_refdef.recurse];
		break;
	case T_GEN_RIPPLEMAP:
		t = shaderstate.tex_ripplemap[r_refdef.recurse];
		break;
	case T_GEN_GBUFFERCASE:
		t = shaderstate.tex_gbuf[pass->texgen-T_GEN_GBUFFER0];
		break;
	}
	GL_LazyBind(tmu, t);
}

/*========================================== matrix functions =====================================*/

typedef vec3_t mat3_t[3];
static mat3_t axisDefault={{1, 0, 0},
					{0, 1, 0},
					{0, 0, 1}};

static void Matrix3_Transpose (mat3_t in, mat3_t out)
{
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}
static void Matrix3_Multiply_Vec3 (const mat3_t a, const vec3_t b, vec3_t product)
{
	product[0] = a[0][0]*b[0] + a[0][1]*b[1] + a[0][2]*b[2];
	product[1] = a[1][0]*b[0] + a[1][1]*b[1] + a[1][2]*b[2];
	product[2] = a[2][0]*b[0] + a[2][1]*b[1] + a[2][2]*b[2];
}

static int Matrix3_Compare(const mat3_t in, const mat3_t out)
{
	return memcmp(in, out, sizeof(mat3_t));
}

//end matrix functions
/*========================================== tables for deforms =====================================*/
#define frand() (rand()*(1.0/RAND_MAX))
#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand()*((x)-floor(x)))

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

//#define R_FastSin(x) sin((x)*(2*M_PI))
#define R_FastSin(x) r_sintable[FTABLE_CLAMP(x)]

static float *FTableForFunc ( unsigned int func )
{
	switch (func)
	{
		default:
		case SHADER_FUNC_SIN:
			return r_sintable;

		case SHADER_FUNC_TRIANGLE:
			return r_triangletable;

		case SHADER_FUNC_SQUARE:
			return r_squaretable;

		case SHADER_FUNC_SAWTOOTH:
			return r_sawtoothtable;

		case SHADER_FUNC_INVERSESAWTOOTH:
			return r_inversesawtoothtable;

		case SHADER_FUNC_NOISE:
			return NULL;
	}
}

void Shader_LightPass(struct shaderparsestate_s *ps, const char *shortname, const void *args)
{
	char shadertext[8192*2];
	extern cvar_t r_drawflat;
	sprintf(shadertext, LIGHTPASS_SHADER, (r_lightmap.ival||r_drawflat.ival)?"#FLAT=1.0":"");
	Shader_DefaultScript(ps, shortname, shadertext);
}

extern cvar_t r_fog_linear;
extern cvar_t r_fog_exp2;
void GenerateFogTexture(texid_t *tex, float density, float zscale)
{
#define FOGS 256
#define FOGT 32
	byte_vec4_t fogdata[FOGS*FOGT];
	int s, t;
	float f, z;
	static float fogdensity, fogzscale;
	if (TEXVALID(*tex) && density == fogdensity && zscale == fogzscale)
		return;
	fogdensity = density;
	fogzscale = zscale;

	for(s = 0; s < FOGS; s++)
		for(t = 0; t < FOGT; t++)
		{
			z = (float)s / (FOGS-1);
			z *= zscale;

			if (r_fog_linear.ival) {
				f = 1.0 - ((density - z) / (density/* - r_refdef.globalfog.depthbias*/)); //pow(z, 0.5);
			} else {
				if (!r_fog_exp2.ival)//GL_EXP
					f = 1-exp(-density * z);
				else //GL_EXP2
					f = 1-exp(-(density*density) * z);
			}

			if (f < 0)
				f = 0;
			if (f > 1)
				f = 1;
			f *= (float)t / (FOGT-1);

			fogdata[t*FOGS + s][0] = 255;
			fogdata[t*FOGS + s][1] = 255;
			fogdata[t*FOGS + s][2] = 255;
			fogdata[t*FOGS + s][3] = 255*f;
		}

	if (!TEXVALID(*tex))
		*tex = Image_CreateTexture("***fog***", NULL, IF_CLAMP|IF_NOMIPMAP);
	Image_Upload(*tex, TF_RGBA32, fogdata, NULL, FOGS, FOGT, 1, IF_CLAMP|IF_NOMIPMAP);
}

void GLBE_DestroyFBOs(void)
{
	size_t i;
	GLBE_FBO_Destroy(&shaderstate.fbo_lprepass);

	for (i = 0; i < R_MAX_RECURSE; i++)
	{
		GLBE_FBO_Destroy(&shaderstate.fbo_reflectrefrac[i]);
		if (shaderstate.tex_reflection[i])
		{
			Image_DestroyTexture(shaderstate.tex_reflection[i]);
			shaderstate.tex_reflection[i] = r_nulltex;
		}
		if (shaderstate.tex_refraction[i])
		{
			Image_DestroyTexture(shaderstate.tex_refraction[i]);
			shaderstate.tex_refraction[i] = r_nulltex;
		}
		if (shaderstate.tex_refractiondepth[i])
		{
			Image_DestroyTexture(shaderstate.tex_refractiondepth[i]);
			shaderstate.tex_refractiondepth[i] = r_nulltex;
		}
	}
	if (shaderstate.temptexture)
	{
		Image_DestroyTexture(shaderstate.temptexture);
		shaderstate.temptexture = r_nulltex;
	}

	//shadowmapping stuff
	if (shadow_fbo_id)
	{
		qglDeleteFramebuffersEXT(1, &shadow_fbo_id);
		shadow_fbo_id = 0;
		shadow_fbo_depth_num = 0;
	}
	for (i = 0; i < countof(shadowmap); i++)
	{
		if (shadowmap[i])
		{
			Image_DestroyTexture(shadowmap[i]);
			shadowmap[i] = r_nulltex;
		}
	}

	//nuke deferred rendering stuff
	for (i = 0; i < countof(shaderstate.tex_gbuf); i++)
	{
		if (shaderstate.tex_gbuf[i])
		{
			Image_DestroyTexture(shaderstate.tex_gbuf[i]);
			shaderstate.tex_gbuf[i] = r_nulltex;
		}
	}
}

void GLBE_Shutdown(void)
{
	size_t u;
	GLBE_FBO_Destroy(&shaderstate.fbo_2dfbo);
	GLBE_DestroyFBOs();

	for (u = 0; u < countof(shaderstate.programfixedemu); u++)
	{
		Shader_ReleaseGeneric(shaderstate.programfixedemu[u]);
		shaderstate.programfixedemu[u] = NULL;
	}

	BZ_Free(shaderstate.wbatches);
	shaderstate.wbatches = NULL;
	shaderstate.maxwbatches = 0;

	//on vid_reload, the gl drivers might have various things bound that have since been destroyed/etc
	//so reset that state to avoid any issues with state
	BE_EnableShaderAttributes(0, 0);
	GL_SelectVBO(0);
	GL_SelectEBO(0);

	for (u = 0; u < countof(shaderstate.currenttextures); u++)
		GL_LazyBind(u, r_nulltex);
	GL_SelectTexture(0);
}

void GLBE_Init(void)
{
	int i;
	double t;

	GLBE_Shutdown();

	memset(&shaderstate, 0, sizeof(shaderstate));

	shaderstate.curentity = &r_worldentity;
	be_maxpasses = gl_config_nofixedfunc?1:gl_mtexarbable;
	be_maxpasses = min(SHADER_TMU_MAX, min(be_maxpasses, 32-VATTR_LEG_TMU0));
	sh_config.stencilbits = 0;
#ifndef GLESONLY
	if (!gl_config_gles && gl_config.glversion >= 3.0 && gl_config_nofixedfunc)
	{
		//docs say this line should be okay in gl3+. nvidia do not seem to agree. GL_STENCIL_BITS is depricated however. so for now, just assume.
		qglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER_EXT, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &sh_config.stencilbits);
		if (qglGetError())
			sh_config.stencilbits = 8;
	}
	else
#endif
		qglGetIntegerv(GL_STENCIL_BITS, &sh_config.stencilbits);
	for (i = 0; i < FTABLE_SIZE; i++)
	{
		t = (double)i / (double)FTABLE_SIZE;

		r_sintable[i] = sin(t * 2*M_PI);

		if (t < 0.25)
			r_triangletable[i] = t * 4.0;
		else if (t < 0.75)
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = (t - 0.75) * 4.0 - 1.0;

		if (t < 0.5)
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}

	shaderstate.identitylighting = 1;
	shaderstate.identitylightmap = 1;
	for (i = 0; i < MAXRLIGHTMAPS; i++)
	{
		shaderstate.dummybatch.lightmap[i] = -1;
		shaderstate.dummybatch.lmlightstyle[i] = INVALID_LIGHTSTYLE;
		shaderstate.dummybatch.vtlightstyle[i] = ~0;
	}

#ifdef RTLIGHTS
	Sh_CheckSettings();
	if ((r_shadow_realtime_dlight.ival && r_shadow_shadowmapping.ival) ||
		(r_shadow_realtime_world.ival  && r_shadow_shadowmapping.ival) )
	{	//we expect some lights that will need shadowmapped shadows.
		GLBE_RegisterLightShader(LSHADER_SMAP);
		GLBE_RegisterLightShader(LSHADER_SMAP|LSHADER_CUBE);
		GLBE_RegisterLightShader(LSHADER_SMAP|LSHADER_SPOT);
	}
	if ((r_shadow_realtime_dlight.ival && (!r_shadow_shadowmapping.ival || !r_shadow_realtime_dlight_shadows.ival)) ||
		(r_shadow_realtime_world.ival  && (!r_shadow_shadowmapping.ival || !r_shadow_realtime_world_shadows.ival )) )
	{	//these are also used when there's no shadow.
		//FIXME: should also happen if there's static world lights without shadows. Move elsewhere?
		GLBE_RegisterLightShader(LSHADER_STANDARD);
		GLBE_RegisterLightShader(LSHADER_STANDARD|LSHADER_CUBE);
		GLBE_RegisterLightShader(LSHADER_STANDARD|LSHADER_SPOT);
	}
#endif

	gl_overbright.modified = true; /*in case the d3d renderer does the same*/
	/*lock the cvar down if the backend can't actually do it*/
	if (
#if 1//defined(QUAKETC)
		//TCs are expected to be using glsl and weird overbright things etc, don't take the risk.
		(!sh_config.progs_supported)
#else
		//Q3 can get away with tex_env_combine for everything, if only because the content allows everything to be flattened to a single pass if needed...
		//some shaders might screw up from our approach though...
		(!gl_config.tex_env_combine && !gl_config_nofixedfunc)
#endif
		&& gl_overbright.ival)
		Cvar_ApplyLatchFlag(&gl_overbright, "0", CVAR_RENDERERLATCH, 0);
	shaderstate.shaderbits = ~SBITS_ATEST_BITS;
	BE_SendPassBlendDepthMask(0);
	currententity = &r_worldentity;

	shaderstate.fogtexture = r_nulltex;

	shaderstate.depthonlyshader = R_RegisterShader("depthonly", SUF_NONE,
				"{\n"
					"program depthonly\n"
					"{\n"
						"depthwrite\n"
						"maskcolor\n"
					"}\n"
				"}\n"
			);
	//if the shader doesn't work, give up with that.
	if (!shaderstate.depthonlyshader || !shaderstate.depthonlyshader->prog)
		shaderstate.depthonlyshader = NULL;

	//make sure the world draws correctly
	r_worldentity.shaderRGBAf[0] = 1;
	r_worldentity.shaderRGBAf[1] = 1;
	r_worldentity.shaderRGBAf[2] = 1;
	r_worldentity.shaderRGBAf[3] = 1;
	r_worldentity.axis[0][0] = 1;
	r_worldentity.axis[1][1] = 1;
	r_worldentity.axis[2][2] = 1;
	r_worldentity.light_avg[0] = 1;
	r_worldentity.light_avg[1] = 1;
	r_worldentity.light_avg[2] = 1;

	R_InitFlashblends();

	memset(&shaderstate.streamvbo, 0, sizeof(shaderstate.streamvbo));
	memset(&shaderstate.streamebo, 0, sizeof(shaderstate.streamebo));
	memset(&shaderstate.streamvao, 0, sizeof(shaderstate.streamvao));
	//only do this where we have to.
	if (qglBufferDataARB && gl_config_nofixedfunc
#ifndef FTE_TARGET_WEB
		&& !gl_config_gles
#endif
		)
	{
		qglGenBuffersARB(sizeof(shaderstate.streamvbo)/sizeof(shaderstate.streamvbo[0]), shaderstate.streamvbo);
		qglGenBuffersARB(sizeof(shaderstate.streamebo)/sizeof(shaderstate.streamebo[0]), shaderstate.streamebo);
		if (qglGenVertexArrays)
			qglGenVertexArrays(sizeof(shaderstate.streamvao)/sizeof(shaderstate.streamvao[0]), shaderstate.streamvao);
	}
}

//end tables

#define MAX_ARRAY_VERTS 65536
static vecV_t		vertexarray[MAX_ARRAY_VERTS];
#if 1//ndef GLSLONLY
static avec4_t		coloursarray[MAX_ARRAY_VERTS];
#ifdef FTE_TARGET_WEB
static float		texcoordarray[1][MAX_ARRAY_VERTS*2];
#else
static float		texcoordarray[SHADER_PASS_MAX][MAX_ARRAY_VERTS*2];
#endif

/*========================================== texture coord generation =====================================*/

static void tcgen_environment(float *st, unsigned int numverts, float *xyz, float *normal)
{
	int			i;
	vec3_t		viewer, reflected;
	float		d;

	vec3_t		rorg;


	RotateLightVector(shaderstate.curentity->axis, shaderstate.curentity->origin, r_origin, rorg);

	for (i = 0 ; i < numverts ; i++, xyz += sizeof(vecV_t)/sizeof(vec_t), normal += 3, st += 2 )
	{
		VectorSubtract (rorg, xyz, viewer);
		VectorNormalizeFast (viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

#ifndef GLSLONLY
static void tcgen_fog(float *st, unsigned int numverts, float *xyz, mfog_t *fog)
{
	int			i;

	float z;
	float eye, point;
	vec4_t zmat;

	//generate a simple matrix to calc only the projected z coord
	zmat[0] = -shaderstate.modelviewmatrix[2];
	zmat[1] = -shaderstate.modelviewmatrix[6];
	zmat[2] = -shaderstate.modelviewmatrix[10];
	zmat[3] = -shaderstate.modelviewmatrix[14];

	Vector4Scale(zmat, shaderstate.fogfar, zmat);

	if (fog && fog->visibleplane)
	{
		eye = (DotProduct(r_refdef.vieworg, fog->visibleplane->normal) - fog->visibleplane->dist);
		if (eye < 1)
			eye = 1;	//avoids a nan

		for (i = 0 ; i < numverts ; i++, xyz += sizeof(vecV_t)/sizeof(vec_t), st += 2 )
		{
			z = DotProduct(xyz, zmat) + zmat[3];
			st[0] = z;

			if (fog->visibleplane)
				point = (DotProduct(xyz, fog->visibleplane->normal) - fog->visibleplane->dist);
			else
				point = 1;
			st[1] = point / (point - eye);
			if (st[1] > 31/32.0)
				st[1] = 31/32.0;
		}
	}
	else
	{
		for (i = 0 ; i < numverts ; i++, xyz += sizeof(vecV_t)/sizeof(vec_t), st += 2 )
		{
			z = DotProduct(xyz, zmat) + zmat[3];
			st[0] = z;
			st[1] = 31/32.0;
		}
	}
}
static void GenerateTCFog(int passnum, mfog_t *fog)
{
	int m;
	mesh_t *mesh;
	for (m = 0; m < shaderstate.meshcount; m++)
	{
		mesh = shaderstate.meshes[m];
		if (!mesh->xyz_array)
			memset(texcoordarray[passnum]+mesh->vbofirstvert*2, 0, mesh->numvertexes*sizeof(float)*2);
		else
			tcgen_fog(texcoordarray[passnum]+mesh->vbofirstvert*2, mesh->numvertexes, (float*)mesh->xyz_array, fog);
	}

	shaderstate.pendingtexcoordparts[passnum] = 2;
	shaderstate.pendingtexcoordvbo[passnum] = 0;
	shaderstate.pendingtexcoordpointer[passnum] = texcoordarray[passnum];
}
#endif

static float *tcgen3(const shaderpass_t *pass, int cnt, float *dst, const mesh_t *mesh)
{
	int i;
	vecV_t *src;
	switch (pass->tcgen)
	{
	default:
	case TC_GEN_SKYBOX:
		src = mesh->xyz_array;
		for (i = 0; i < cnt; i++, dst += 3)
		{
			dst[0] = src[i][0] - r_refdef.vieworg[0];
			dst[1] = src[i][1] - r_refdef.vieworg[1];
			dst[2] = src[i][2] - r_refdef.vieworg[2];
		}
		return dst-cnt*3;

//	case TC_GEN_WOBBLESKY:
//	case TC_GEN_REFLECT:
//		break;
	}
}
static float *tcgen(const shaderpass_t *pass, int cnt, float *dst, const mesh_t *mesh)
{
	int i;
	vecV_t *src;
	switch (pass->tcgen)
	{
	default:
	case TC_GEN_BASE:
		return (float*)mesh->st_array;
	case TC_GEN_LIGHTMAP:
		if (!mesh->lmst_array[0])
			return (float*)mesh->st_array;
		else
			return (float*)mesh->lmst_array[0];
	case TC_GEN_NORMAL:
		return (float*)mesh->normals_array;
	case TC_GEN_SVECTOR:
		return (float*)mesh->snormals_array;
	case TC_GEN_TVECTOR:
		return (float*)mesh->tnormals_array;
	case TC_GEN_ENVIRONMENT:
		if (!mesh->normals_array)
			return (float*)mesh->st_array;
		tcgen_environment(dst, cnt, (float*)mesh->xyz_array, (float*)mesh->normals_array);
		return dst;

//	case TC_GEN_DOTPRODUCT:
//		return mesh->st_array[0];
	case TC_GEN_VECTOR:
		src = mesh->xyz_array;
		for (i = 0; i < cnt; i++, dst += 2)
		{
			dst[0] = DotProduct(pass->tcgenvec[0], src[i]);
			dst[1] = DotProduct(pass->tcgenvec[1], src[i]);
		}
		return dst-cnt*2;

//	case TC_GEN_SKYBOX:
//	case TC_GEN_WOBBLESKY:
//	case TC_GEN_REFLECT:
//		break;
	}
}

/*src and dst can be the same address when tcmods are chained*/
static void tcmod(const tcmod_t *tcmod, int cnt, const float *src, float *dst, const mesh_t *mesh)
{
	float *table;
	float t1, t2;
	float cost, sint;
	int j;
	switch (tcmod->type)
	{
		case SHADER_TCMOD_ROTATE:
			cost = tcmod->args[0] * shaderstate.curtime;
			sint = R_FastSin(cost);
			cost = R_FastSin(cost + 0.25);

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				t1 = cost * (src[0] - 0.5f) - sint * (src[1] - 0.5f) + 0.5f;
				t2 = cost * (src[1] - 0.5f) + sint * (src[0] - 0.5f) + 0.5f;
				dst[0] = t1;
				dst[1] = t2;
			}
			break;

		case SHADER_TCMOD_SCALE:
			t1 = tcmod->args[0];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1;
				dst[1] = src[1] * t2;
			}
			break;

		case SHADER_TCMOD_TURB:
			t1 = tcmod->args[2] + shaderstate.curtime * tcmod->args[3];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] + R_FastSin (src[0]*t2+t1) * t2;
				dst[1] = src[1] + R_FastSin (src[1]*t2+t1) * t2;
			}
			break;

		case SHADER_TCMOD_STRETCH:
			table = FTableForFunc(tcmod->args[0]);
			t2 = tcmod->args[3] + shaderstate.curtime * tcmod->args[4];
			t1 = FTABLE_EVALUATE(table, t2) * tcmod->args[2] + tcmod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1 + t2;
				dst[1] = src[1] * t1 + t2;
			}
			break;

		case SHADER_TCMOD_SCROLL:
			t1 = tcmod->args[0] * shaderstate.curtime;
			t2 = tcmod->args[1] * shaderstate.curtime;

			for (j = 0; j < cnt; j++, dst += 2, src+=2)
			{
				dst[0] = src[0] + t1;
				dst[1] = src[1] + t2;
			}
			break;

		case SHADER_TCMOD_TRANSFORM:
			for (j = 0; j < cnt; j++, dst+=2, src+=2)
			{
				t1 = src[0];
				t2 = src[1];
				dst[0] = t1 * tcmod->args[0] + t2 * tcmod->args[2] + tcmod->args[4];
				dst[1] = t1 * tcmod->args[1] + t2 * tcmod->args[3] + tcmod->args[5];
			}
			break;

		case SHADER_TCMOD_PAGE:
			{	//simple atlas bias with no scaling. use a separate tcmod for that.
				int w = tcmod->args[0];
				int h = tcmod->args[1];
				float f = shaderstate.curtime / (tcmod->args[2]*w*h);
				int idx = (f - floor(f))*w*h;
				t1 = (idx%w)/tcmod->args[0];
				t2 = (idx/w)/tcmod->args[1];

				for (j = 0; j < cnt; j++, dst += 2, src+=2)
				{
					dst[0] = src[0] + t1;
					dst[1] = src[1] + t2;
				}
			}
			break;

		default:
			break;
	}
}

static void GenerateTCMods3(const shaderpass_t *pass, int passnum)
{
#if 1
	int m;
	float *src;
	mesh_t *mesh;
	for (m = 0; m < shaderstate.meshcount; m++)
	{
		mesh = shaderstate.meshes[m];

		src = tcgen3(pass, mesh->numvertexes, texcoordarray[passnum]+mesh->vbofirstvert*3, mesh);

		if (src != texcoordarray[passnum]+mesh->vbofirstvert*3)
		{
			//this shouldn't actually ever be true
			memcpy(texcoordarray[passnum]+mesh->vbofirstvert*3, src, sizeof(vec3_t)*mesh->numvertexes);
		}
	}
	shaderstate.pendingtexcoordparts[passnum] = 3;
	shaderstate.pendingtexcoordvbo[passnum] = 0;
	shaderstate.pendingtexcoordpointer[passnum] = texcoordarray[passnum];
#else
	GL_DeselectVAO();
	if (!shaderstate.vbo_texcoords[passnum])
	{
		shaderstate.vbo_texcoords[passnum] = 0;
		qglGenBuffersARB(1, &shaderstate.vbo_texcoords[passnum]);
	}
	GL_SelectVBO(shaderstate.vbo_texcoords[passnum]);

	{
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, MAX_ARRAY_VERTS*sizeof(float)*3, NULL, GL_STREAM_DRAW_ARB);
		for (; meshlist; meshlist = meshlist->next)
		{
			int i;
			float *src;
			src = tcge3n(pass, meshlist->numvertexes, texcoordarray[passnum], meshlist);
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, meshlist->vbofirstvert*8, meshlist->numvertexes*8, src);
		}
	}

	shaderstate.pendingtexcoordparts[passnum] = 2;
	shaderstate.pendingtexcoordvbo[passnum] = shaderstate.vbo_texcoords[passnum];
	shaderstate.pendingtexcoordpointer[passnum] = NULL;
#endif
}

static void GenerateTCMods(const shaderpass_t *pass, int passnum)
{
#if 1
	int i, m;
	float *src;
	mesh_t *mesh;
	for (m = 0; m < shaderstate.meshcount; m++)
	{
		mesh = shaderstate.meshes[m];

		src = tcgen(pass, mesh->numvertexes, texcoordarray[passnum]+mesh->vbofirstvert*2, mesh);
		//tcgen might return unmodified info
		if (!src)
		{	//don't crash... not much else we can do.
			shaderstate.pendingtexcoordparts[passnum] = 2;
			shaderstate.pendingtexcoordvbo[passnum] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[passnum] = shaderstate.sourcevbo->texcoord.gl.addr;
		}
		else if (pass->numtcmods)
		{
			tcmod(&pass->tcmods[0], mesh->numvertexes, src, texcoordarray[passnum]+mesh->vbofirstvert*2, mesh);
			for (i = 1; i < pass->numtcmods; i++)
			{
				tcmod(&pass->tcmods[i], mesh->numvertexes, texcoordarray[passnum]+mesh->vbofirstvert*2, texcoordarray[passnum]+mesh->vbofirstvert*2, mesh);
			}
			src = texcoordarray[passnum]+mesh->vbofirstvert*2;
		}
		else if (src != texcoordarray[passnum]+mesh->vbofirstvert*2)
		{
			//this shouldn't actually ever be true
			memcpy(texcoordarray[passnum]+mesh->vbofirstvert*2, src, 8*mesh->numvertexes);
		}
	}
	shaderstate.pendingtexcoordparts[passnum] = 2;
	shaderstate.pendingtexcoordvbo[passnum] = 0;
	shaderstate.pendingtexcoordpointer[passnum] = texcoordarray[passnum];
#else
	GL_DeselectVAO();
	if (!shaderstate.vbo_texcoords[passnum])
	{
		shaderstate.vbo_texcoords[passnum] = 0;
		qglGenBuffersARB(1, &shaderstate.vbo_texcoords[passnum]);
	}
	GL_SelectVBO(shaderstate.vbo_texcoords[passnum]);

	{
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, MAX_ARRAY_VERTS*sizeof(float)*2, NULL, GL_STREAM_DRAW_ARB);
		for (; meshlist; meshlist = meshlist->next)
		{
			int i;
			float *src;
			src = tcgen(pass, meshlist->numvertexes, texcoordarray[passnum], meshlist);
			//tcgen might return unmodified info
			if (pass->numtcmods)
			{
				tcmod(&pass->tcmods[0], meshlist->numvertexes, src, texcoordarray[passnum], meshlist);
				for (i = 1; i < pass->numtcmods; i++)
				{
					tcmod(&pass->tcmods[i], meshlist->numvertexes, texcoordarray[passnum], texcoordarray[passnum], meshlist);
				}
				src = texcoordarray[passnum];
			}
			qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, meshlist->vbofirstvert*8, meshlist->numvertexes*8, src);
		}
	}

	shaderstate.pendingtexcoordparts[passnum] = 2;
	shaderstate.pendingtexcoordvbo[passnum] = shaderstate.vbo_texcoords[passnum];
	shaderstate.pendingtexcoordpointer[passnum] = NULL;
#endif
}

//end texture coords
/*========================================== colour generation =====================================*/

//source is always packed
//dest is packed too
static void colourgen(const shaderpass_t *pass, int cnt, vec4_t *src, vec4_t *dst, const mesh_t *mesh)
{
	switch (pass->rgbgen)
	{
	case RGB_GEN_ENTITY:
		while((cnt)--)
		{
			dst[cnt][0] = shaderstate.curentity->shaderRGBAf[0];
			dst[cnt][1] = shaderstate.curentity->shaderRGBAf[1];
			dst[cnt][2] = shaderstate.curentity->shaderRGBAf[2];
		}
		break;
	case RGB_GEN_ONE_MINUS_ENTITY:
		while((cnt)--)
		{
			dst[cnt][0] = 1-shaderstate.curentity->shaderRGBAf[0];
			dst[cnt][1] = 1-shaderstate.curentity->shaderRGBAf[1];
			dst[cnt][2] = 1-shaderstate.curentity->shaderRGBAf[2];
		}
		break;
	case RGB_GEN_VERTEX_LIGHTING:
#if MAXRLIGHTMAPS > 1
		if (mesh->colors4f_array[1])
		{
			float lm[MAXRLIGHTMAPS];
			lm[0] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]]/256.0f*shaderstate.identitylighting;
			lm[1] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[1]]/256.0f*shaderstate.identitylighting;
			lm[2] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[2]]/256.0f*shaderstate.identitylighting;
			lm[3] = d_lightstylevalue[shaderstate.curbatch->vtlightstyle[3]]/256.0f*shaderstate.identitylighting;
			while((cnt)--)
			{
				VectorScale(		mesh->colors4f_array[0][cnt], lm[0], dst[cnt]);
				VectorMA(dst[cnt],	lm[1], mesh->colors4f_array[1][cnt], dst[cnt]);
				VectorMA(dst[cnt],	lm[2], mesh->colors4f_array[2][cnt], dst[cnt]);
				VectorMA(dst[cnt],	lm[3], mesh->colors4f_array[3][cnt], dst[cnt]);
			}
			break;
		}
#endif

		if (shaderstate.identitylighting != 1)
		{
			if (!src)
			{
				while((cnt)--)
				{
					dst[cnt][0] = shaderstate.identitylighting;
					dst[cnt][1] = shaderstate.identitylighting;
					dst[cnt][2] = shaderstate.identitylighting;
				}
				break;
			}
			while((cnt)--)
			{
				dst[cnt][0] = src[cnt][0]*shaderstate.identitylighting;
				dst[cnt][1] = src[cnt][1]*shaderstate.identitylighting;
				dst[cnt][2] = src[cnt][2]*shaderstate.identitylighting;
			}
			break;
		}
	case RGB_GEN_VERTEX_EXACT:
		if (!src)
		{
			while((cnt)--)
			{
				dst[cnt][0] = 1;
				dst[cnt][1] = 1;
				dst[cnt][2] = 1;
			}
			break;
		}

		while((cnt)--)
		{
			dst[cnt][0] = src[cnt][0];
			dst[cnt][1] = src[cnt][1];
			dst[cnt][2] = src[cnt][2];
		}
		break;
	case RGB_GEN_ONE_MINUS_VERTEX:
		while((cnt)--)
		{
			dst[cnt][0] = 1-src[cnt][0];
			dst[cnt][1] = 1-src[cnt][1];
			dst[cnt][2] = 1-src[cnt][2];
		}
		break;
	case RGB_GEN_IDENTITY_LIGHTING:
		if (shaderstate.curbatch->vtlightstyle[0] != 255 && d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]] != 256)
		{
			//FIXME: 
			while((cnt)--)
			{
				dst[cnt][0] = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]]/256.0f;
				dst[cnt][1] = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]]/256.0f;
				dst[cnt][2] = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->vtlightstyle[0]]/256.0f;
			}
		}
		else
		{
			//compensate for overbrights
			while((cnt)--)
			{
				dst[cnt][0] = shaderstate.identitylighting;
				dst[cnt][1] = shaderstate.identitylighting;
				dst[cnt][2] = shaderstate.identitylighting;
			}
		}
		break;
	case RGB_GEN_IDENTITY_OVERBRIGHT:
		while((cnt)--)
		{
			dst[cnt][0] = shaderstate.identitylightmap;
			dst[cnt][1] = shaderstate.identitylightmap;
			dst[cnt][2] = shaderstate.identitylightmap;
		}
		break;
	default:
	case RGB_GEN_IDENTITY:
		while((cnt)--)
		{
			dst[cnt][0] = shaderstate.identitylighting;
			dst[cnt][1] = shaderstate.identitylighting;
			dst[cnt][2] = shaderstate.identitylighting;
		}
		break;
	case RGB_GEN_CONST:
		while((cnt)--)
		{
			dst[cnt][0] = pass->rgbgen_func.args[0];
			dst[cnt][1] = pass->rgbgen_func.args[1];
			dst[cnt][2] = pass->rgbgen_func.args[2];
		}
		break;
	case RGB_GEN_ENTITY_LIGHTING_DIFFUSE:
		R_LightArrays(shaderstate.curentity, mesh->xyz_array, dst, cnt, mesh->normals_array, shaderstate.identitylighting, true);
		break;
	case RGB_GEN_LIGHTING_DIFFUSE:
		R_LightArrays(shaderstate.curentity, mesh->xyz_array, dst, cnt, mesh->normals_array, shaderstate.identitylighting, false);
		break;
	case RGB_GEN_WAVE:
		{
			float *table;
			float c;

			table = FTableForFunc(pass->rgbgen_func.type);
			c = pass->rgbgen_func.args[2] + shaderstate.curtime * pass->rgbgen_func.args[3];
			c = FTABLE_EVALUATE(table, c) * pass->rgbgen_func.args[1] + pass->rgbgen_func.args[0];
			c = bound(0.0f, c, 1.0f);

			while((cnt)--)
			{
				dst[cnt][0] = c;
				dst[cnt][1] = c;
				dst[cnt][2] = c;
			}
		}
		break;

	case RGB_GEN_TOPCOLOR:
		if (cnt)
		{
			R_FetchPlayerColour(shaderstate.curentity->topcolour, dst[0]);
			while((cnt)--)
			{
				dst[cnt][0] = dst[0][0];
				dst[cnt][1] = dst[0][1];
				dst[cnt][2] = dst[0][2];
			}
		}
		break;
	case RGB_GEN_BOTTOMCOLOR:
		if (cnt)
		{
			R_FetchPlayerColour(shaderstate.curentity->bottomcolour, dst[0]);
			while((cnt)--)
			{
				dst[cnt][0] = dst[0][0];
				dst[cnt][1] = dst[0][1];
				dst[cnt][2] = dst[0][2];
			}
		}
		break;
	}
}
#endif

static void BE_GenTempMeshVBO(vbo_t **vbo, mesh_t *m);
static void DeformGen_Text(int stringid, int cnt, vecV_t *src, vecV_t *dst, const mesh_t *mesh)
{
#define maxlen 32
	vecV_t *textverts = vertexarray;
	static vec2_t texttc[maxlen*4];
	extern index_t	r_quad_indexes[];
	static mesh_t textmesh, *meshptr = &textmesh;
	int i;
	vec3_t org;
	vec3_t right;
	vec3_t down;
	float s, t, d;
	char cvarname[64];
	const char *text;
	Q_snprintfz(cvarname, sizeof(cvarname), "r_shadertext_%i", stringid);
	text = Cvar_Get(cvarname, "", 0, "Shader System")->string;
	VectorCopy(mesh->snormals_array[0], right);
	VectorNegate(mesh->tnormals_array[0], down);
	CrossProduct(right, down, org);
	if (DotProduct(mesh->normals_array[0], org) > 0)
		VectorNegate(right, right);
	VectorClear(org);
	for (i = 0; i < cnt; i++)
		VectorAdd(org, src[i], org);
	VectorScale(org, 1.0/i, org);
	for (i = 0, s = 0; i < 4; i++)
	{
		d = DotProduct(right, src[i]) - DotProduct(right, org);
		if (s < d)
			s = d;
	}
	i = strlen(text);
	VectorScale(right, 2*s/i, right);
	VectorScale(down, 2*s/i, down);
	VectorMA(org, -i*0.5, right, org);

	memset(&textmesh, 0, sizeof(textmesh));
	textmesh.indexes = r_quad_indexes;
	textmesh.xyz_array = textverts;
	textmesh.st_array = texttc;

	org[1] += 0;

	for (i = 0; i < maxlen; )
	{
		qbyte c = *text++;
		if (!c)
			break;
		if (c != ' ')
		{
			const float sz = 1 / 16.0f;
			s = (c&15)*sz;
			t = (c>>4)*sz;
			VectorCopy(org, textverts[i*4+0]);
			Vector2Set(texttc[i*4+0], s, t);

			VectorAdd(textverts[i*4+0], right, textverts[i*4+1]);
			Vector2Set(texttc[i*4+1], s+sz, t);

			VectorAdd(textverts[i*4+1], down, textverts[i*4+2]);
			Vector2Set(texttc[i*4+2], s+sz, t+sz);

			VectorAdd(textverts[i*4+0], down, textverts[i*4+3]);
			Vector2Set(texttc[i*4+3], s, t+sz);
			i++;
		}
		VectorAdd(org, right, org);
	}
	textmesh.numindexes = i*6;
	textmesh.numvertexes = i*4;

	BE_GenTempMeshVBO(&shaderstate.sourcevbo, &textmesh);

	shaderstate.meshcount = 1;
	shaderstate.meshes = &meshptr;
#undef maxlen
}
static void deformgen(const deformv_t *deformv, int cnt, vecV_t *src, vecV_t *dst, const mesh_t *mesh)
{
	float *table;
	int j, k;
	float args[4];
	float deflect;
	switch (deformv->type)
	{
	default:
	case DEFORMV_NONE:
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
		break;

	case DEFORMV_WAVE:
		if (!mesh->normals_array)
		{
			if (src != dst)
				memcpy(dst, src, sizeof(*src)*cnt);
			return;
		}
		args[0] = deformv->func.args[0];
		args[1] = deformv->func.args[1];
		args[3] = deformv->func.args[2] + deformv->func.args[3] * shaderstate.curtime;
		table = FTableForFunc(deformv->func.type);

		for ( j = 0; j < cnt; j++ )
		{
			deflect = deformv->args[0] * (src[j][0]+src[j][1]+src[j][2]) + args[3];
			deflect = FTABLE_EVALUATE(table, deflect) * args[1] + args[0];

			// Deflect vertex along its normal by wave amount
			VectorMA(src[j], deflect, mesh->normals_array[j], dst[j]);
		}
		break;

	case DEFORMV_NORMAL:
		//normal does not actually move the verts, but it does change the normals array
		//we don't currently support that.
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
/*
		args[0] = deformv->args[1] * shaderstate.curtime;

		for ( j = 0; j < cnt; j++ )
		{
			args[1] = normalsArray[j][2] * args[0];

			deflect = deformv->args[0] * R_FastSin(args[1]);
			normalsArray[j][0] *= deflect;
			deflect = deformv->args[0] * R_FastSin(args[1] + 0.25);
			normalsArray[j][1] *= deflect;
			VectorNormalizeFast(normalsArray[j]);
		}
*/		break;

	case DEFORMV_MOVE:
		table = FTableForFunc(deformv->func.type);
		deflect = deformv->func.args[2] + shaderstate.curtime * deformv->func.args[3];
		deflect = FTABLE_EVALUATE(table, deflect) * deformv->func.args[1] + deformv->func.args[0];

		for ( j = 0; j < cnt; j++ )
			VectorMA(src[j], deflect, deformv->args, dst[j]);
		break;

	case DEFORMV_BULGE:
		if (!mesh->normals_array)
		{
			if (src != dst)
				memcpy(dst, src, sizeof(*src)*cnt);
			break;
		}
		args[0] = deformv->args[0]/(2*M_PI);
		args[1] = deformv->args[1];
		args[2] = shaderstate.curtime * deformv->args[2]/(2*M_PI);

		for (j = 0; j < cnt; j++)
		{
			deflect = R_FastSin(mesh->st_array[j][0]*args[0] + args[2])*args[1];
			dst[j][0] = src[j][0]+deflect*mesh->normals_array[j][0];
			dst[j][1] = src[j][1]+deflect*mesh->normals_array[j][1];
			dst[j][2] = src[j][2]+deflect*mesh->normals_array[j][2];
		}
		break;

	case DEFORMV_AUTOSPRITE:
		if (mesh->numindexes < 6)
			break;

		for (j = 0; j+3 < cnt; j+=4, src+=4, dst+=4)
		{
			vec3_t mid, d;
			vec2_t mid2;
			float radius, s,t;
			vec2_t *fte_restrict st = &mesh->st_array[j];
			mid[0] = 0.25*(src[0][0] + src[1][0] + src[2][0] + src[3][0]);
			mid[1] = 0.25*(src[0][1] + src[1][1] + src[2][1] + src[3][1]);
			mid[2] = 0.25*(src[0][2] + src[1][2] + src[2][2] + src[3][2]);
			VectorSubtract(src[0], mid, d);
			radius = VectorLength(d) * 0.707;

			mid2[0] = 0.25*(st[0][0] + st[1][0] + st[2][0] + st[3][0]);
			mid2[1] = 0.25*(st[0][1] + st[1][1] + st[2][1] + st[3][1]);

			for (k = 0; k < 4; k++)
			{
				//q3 fully regenerates verts. we don't because that destroys ST coords.
				//even so, if the ST coords are non-centered for some reason then we still need to get the right values as if they were.
				//hence the mid2.
				s = (st[k][0] > mid2[0])?1:-1;
				t = (st[k][1] > mid2[1])?1:-1;
				dst[k][0] = mid[0] + radius*(s*shaderstate.modelviewmatrix[0+0]-t*shaderstate.modelviewmatrix[0+1]);
				dst[k][1] = mid[1] + radius*(s*shaderstate.modelviewmatrix[4+0]-t*shaderstate.modelviewmatrix[4+1]);
				dst[k][2] = mid[2] + radius*(s*shaderstate.modelviewmatrix[8+0]-t*shaderstate.modelviewmatrix[8+1]);
			}
		}
		break;

	case DEFORMV_AUTOSPRITE2:
		if (mesh->numindexes < 6)
			break;

		for (k = 0; k < mesh->numindexes; k += 6)
		{
			int long_axis, short_axis;
			vec3_t axis;
			float len[3];
			mat3_t m0, m1, m2, result;
			float *quad[4];
			vec3_t rot_centre, tv, tv2;

			quad[0] = (float *)(src + mesh->indexes[k+0]);
			quad[1] = (float *)(src + mesh->indexes[k+1]);
			quad[2] = (float *)(src + mesh->indexes[k+2]);

			for (j = 2; j >= 0; j--)
			{
				quad[3] = (float *)(src + mesh->indexes[k+3+j]);
				if (!VectorEquals (quad[3], quad[0]) &&
					!VectorEquals (quad[3], quad[1]) &&
					!VectorEquals (quad[3], quad[2]))
				{
					break;
				}
			}

			// build a matrix were the longest axis of the billboard is the Y-Axis
			VectorSubtract(quad[1], quad[0], m0[0]);
			VectorSubtract(quad[2], quad[0], m0[1]);
			VectorSubtract(quad[2], quad[1], m0[2]);
			len[0] = DotProduct(m0[0], m0[0]);
			len[1] = DotProduct(m0[1], m0[1]);
			len[2] = DotProduct(m0[2], m0[2]);

			if ((len[2] > len[1]) && (len[2] > len[0]))
			{
				if (len[1] > len[0])
				{
					long_axis = 1;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 1;
				}
			}
			else if ((len[1] > len[2]) && (len[1] > len[0]))
			{
				if (len[2] > len[0])
				{
					long_axis = 2;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 2;
				}
			}
			else //if ( (len[0] > len[1]) && (len[0] > len[2]) )
			{
				if (len[2] > len[1])
				{
					long_axis = 2;
					short_axis = 1;
				}
				else
				{
					long_axis = 1;
					short_axis = 2;
				}
			}

			if (DotProduct(m0[long_axis], m0[short_axis]))
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorCopy(axis, m0[1]);

				if (axis[0] || axis[1])
				{
					VectorVectors(m0[1], m0[2], m0[0]);
				}
				else
				{
					VectorVectors(m0[1], m0[0], m0[2]);
				}
			}
			else
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorNormalize2(m0[short_axis], m0[0]);
				VectorCopy(axis, m0[1]);
				CrossProduct(m0[0], m0[1], m0[2]);
			}

			for (j = 0; j < 3; j++)
				rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

			if (shaderstate.curentity)
			{
				VectorAdd(shaderstate.curentity->origin, rot_centre, tv);
			}
			else
			{
				VectorCopy(rot_centre, tv);
			}
			VectorSubtract(r_origin, tv, tv);

			// filter any longest-axis-parts off the camera-direction
			deflect = -DotProduct(tv, axis);

			VectorMA(tv, deflect, axis, m1[2]);
			VectorNormalizeFast(m1[2]);
			VectorCopy(axis, m1[1]);
			CrossProduct(m1[1], m1[2], m1[0]);

			Matrix3_Transpose(m1, m2);
			Matrix3_Multiply(m2, m0, result);

			for (j = 0; j < 4; j++)
			{
				int v = ((vecV_t*)quad[j]-src);
				VectorSubtract(quad[j], rot_centre, tv);
				Matrix3_Multiply_Vec3((const vec3_t*)result, tv, tv2);
				VectorAdd(rot_centre, tv2, dst[v]);
			}
		}
		break;

//	case DEFORMV_PROJECTION_SHADOW:
//		break;

	case DEFORMV_TEXT:
		DeformGen_Text(deformv->args[0], cnt, src, dst, mesh);
		break;
	}
}

static void GenerateVertexBlends(const shader_t *shader)
{
	int i, m;
	mesh_t *meshlist;
	vecV_t *ov, *iv1, *iv2;
	float w1, w2;
	for (m = 0; m < shaderstate.meshcount; m++)
	{
		meshlist = shaderstate.meshes[m];

		ov = vertexarray+meshlist->vbofirstvert;
		iv1 = meshlist->xyz_array;
		iv2 = meshlist->xyz2_array;
		w1 = meshlist->xyz_blendw[0];
		w2 = meshlist->xyz_blendw[1];
		for (i = 0; i < meshlist->numvertexes; i++)
		{
			ov[i][0] = iv1[i][0]*w1 + iv2[i][0]*w2;
			ov[i][1] = iv1[i][1]*w1 + iv2[i][1]*w2;
			ov[i][2] = iv1[i][2]*w1 + iv2[i][2]*w2;
		}
		for (i = 0; i < shader->numdeforms; i++)
		{
			deformgen(&shader->deforms[i], meshlist->numvertexes, vertexarray+meshlist->vbofirstvert, vertexarray+meshlist->vbofirstvert, meshlist);
		}
	}

	shaderstate.pendingvertexpointer = vertexarray;
	shaderstate.pendingvertexvbo = 0;
}
static void GenerateVertexDeforms(const shader_t *shader)
{
	int i, m;
	mesh_t *meshlist;
	for (m = 0; m < shaderstate.meshcount; m++)
	{
		meshlist = shaderstate.meshes[m];

		deformgen(&shader->deforms[0], meshlist->numvertexes, meshlist->xyz_array, vertexarray+meshlist->vbofirstvert, meshlist);
		for (i = 1; i < shader->numdeforms; i++)
		{
			deformgen(&shader->deforms[i], meshlist->numvertexes, vertexarray+meshlist->vbofirstvert, vertexarray+meshlist->vbofirstvert, meshlist);
		}
	}

	shaderstate.pendingvertexpointer = vertexarray;
	shaderstate.pendingvertexvbo = 0;
}

#if 1//ndef GLSLONLY

/*======================================alpha ===============================*/

static void alphagen(const shaderpass_t *pass, int cnt, avec4_t *const src, avec4_t *dst, const mesh_t *mesh)
{
	float *table;
	float t;
	float f;
	vec3_t v1, v2;
	int i;

	switch (pass->alphagen)
	{
	default:
	case ALPHA_GEN_IDENTITY:
		if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			while(cnt--)
				dst[cnt][3] = shaderstate.curentity->shaderRGBAf[3];
		}
		else
		{
			while(cnt--)
				dst[cnt][3] = 1;
		}
		break;

	case ALPHA_GEN_CONST:
		t = pass->alphagen_func.args[0];
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_WAVE:
		table = FTableForFunc(pass->alphagen_func.type);
		f = pass->alphagen_func.args[2] + shaderstate.curtime * pass->alphagen_func.args[3];
		f = FTABLE_EVALUATE(table, f) * pass->alphagen_func.args[1] + pass->alphagen_func.args[0];
		t = bound(0.0f, f, 1.0f);
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_PORTAL:
		//FIXME: should this be per-vert?
		if (r_refdef.recurse || !mesh->xyz_array)
			f = 1;
		else
		{
			VectorAdd(mesh->xyz_array[0], shaderstate.curentity->origin, v1);
			VectorSubtract(r_origin, v1, v2);
			f = VectorLength(v2) * (1.0 / shaderstate.curshader->portaldist);
			f = bound(0.0f, f, 1.0f);
		}

		while(cnt--)
			dst[cnt][3] = f;
		break;

	case ALPHA_GEN_VERTEX:
		if (!src)
		{
			while(cnt--)
			{
				dst[cnt][3] = 1;
			}
			break;
		}

		while(cnt--)
		{
			dst[cnt][3] = src[cnt][3];
		}
		break;

	case ALPHA_GEN_ENTITY:
		f = bound(0, shaderstate.curentity->shaderRGBAf[3], 1);
		while(cnt--)
		{
			dst[cnt][3] = f;
		}
		break;


	case ALPHA_GEN_SPECULAR:
		if (!mesh->normals_array)
		{
			while(cnt--)
				dst[cnt][3] = 0.2;
		}
		else
		{
			VectorSubtract(r_origin, shaderstate.curentity->origin, v1);

			if (!Matrix3_Compare((const vec3_t*)shaderstate.curentity->axis, (const vec3_t*)axisDefault))
			{
				Matrix3_Multiply_Vec3(shaderstate.curentity->axis, v1, v2);
			}
			else
			{
				VectorCopy(v1, v2);
			}

			for (i = 0; i < cnt; i++)
			{
				VectorSubtract(v2, mesh->xyz_array[i], v1);
				f = DotProduct(v1, mesh->normals_array[i] ) * Q_rsqrt(DotProduct(v1,v1));
				f = f * f * f * f * f;
				dst[i][3] = bound (0.0f, f, 1.0f);
			}
		}
		break;
	}
}

static void GenerateColourMods(const shaderpass_t *pass)
{
	unsigned int m;
	mesh_t *meshlist;
	meshlist = shaderstate.meshes[0];

	if (pass->flags & SHADER_PASS_NOCOLORARRAY)
	{
		colourgen(pass, 1, meshlist->colors4f_array[0], &shaderstate.pendingcolourflat, meshlist);
		alphagen(pass, 1, meshlist->colors4f_array[0], &shaderstate.pendingcolourflat, meshlist);
		shaderstate.colourarraytype = 0;
		shaderstate.pendingcolourvbo = 0;
		shaderstate.pendingcolourpointer = NULL;
	}
	else
	{
		extern cvar_t r_nolightdir;
		if (pass->rgbgen == RGB_GEN_LIGHTING_DIFFUSE || pass->rgbgen == RGB_GEN_ENTITY_LIGHTING_DIFFUSE)
		{
			if (shaderstate.mode == BEM_DEPTHDARK || shaderstate.mode == BEM_DEPTHONLY)
			{
				shaderstate.pendingcolourflat[0] = shaderstate.pendingcolourflat[1] = shaderstate.pendingcolourflat[2] = 0;
				alphagen(pass, 1, meshlist->colors4f_array[0], &shaderstate.pendingcolourflat, meshlist);
				shaderstate.colourarraytype = 0;
				shaderstate.pendingcolourvbo = 0;
				shaderstate.pendingcolourpointer = NULL;
				return;
			}
			if (shaderstate.mode == BEM_LIGHT)
			{
				shaderstate.pendingcolourflat[0] = shaderstate.pendingcolourflat[1] = shaderstate.pendingcolourflat[2] = 1;
				alphagen(pass, 1, meshlist->colors4f_array[0], &shaderstate.pendingcolourflat, meshlist);
				shaderstate.colourarraytype = 0;
				shaderstate.pendingcolourvbo = 0;
				shaderstate.pendingcolourpointer = NULL;
				return;
			}
			if (r_nolightdir.ival || (!shaderstate.curentity->light_range[0] && !shaderstate.curentity->light_range[1] && !shaderstate.curentity->light_range[2]))
			{
				VectorCopy(shaderstate.curentity->light_avg, shaderstate.pendingcolourflat);
				alphagen(pass, 1, meshlist->colors4f_array[0], &shaderstate.pendingcolourflat, meshlist);
				shaderstate.colourarraytype = 0;
				shaderstate.pendingcolourvbo = 0;
				shaderstate.pendingcolourpointer = NULL;
				return;
			}
		}

		//if its vetex lighting, just use the vbo
		if (((pass->rgbgen == RGB_GEN_VERTEX_LIGHTING && shaderstate.identitylighting == 1) || pass->rgbgen == RGB_GEN_VERTEX_EXACT) && pass->alphagen == ALPHA_GEN_VERTEX)
		{
			shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
			shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
			shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
			return;
		}

		for (m = 0; m < shaderstate.meshcount; m++)
		{
			meshlist = shaderstate.meshes[m];

			colourgen(pass, meshlist->numvertexes, meshlist->colors4f_array[0], coloursarray + meshlist->vbofirstvert, meshlist);
			alphagen(pass, meshlist->numvertexes, meshlist->colors4f_array[0], coloursarray + meshlist->vbofirstvert, meshlist);
		}

		shaderstate.colourarraytype = GL_FLOAT;
		shaderstate.pendingcolourvbo = 0;
		shaderstate.pendingcolourpointer = coloursarray;
	}
}

static void BE_GeneratePassTC(const shaderpass_t *pass, int tmu)
{
	if (!pass->numtcmods)
	{
		//if there are no tcmods, pass through here as fast as possible
		switch(pass->tcgen)
		{
		case TC_GEN_BASE:
			shaderstate.pendingtexcoordparts[tmu] = 2;
			shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->texcoord.gl.addr;
			break;
		case TC_GEN_LIGHTMAP:
			if (!shaderstate.sourcevbo->lmcoord[0].gl.addr)
			{
				shaderstate.pendingtexcoordparts[tmu] = 2;
				shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->texcoord.gl.vbo;
				shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->texcoord.gl.addr;
			}
			else
			{
				shaderstate.pendingtexcoordparts[tmu] = 2;
				shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->lmcoord[0].gl.vbo;
				shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->lmcoord[0].gl.addr;
			}
			break;
		case TC_GEN_NORMAL:
			shaderstate.pendingtexcoordparts[tmu] = 3;
			shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->normals.gl.vbo;
			shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->normals.gl.addr;
			break;
		case TC_GEN_SVECTOR:
			shaderstate.pendingtexcoordparts[tmu] = 3;
			shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->svector.gl.vbo;
			shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->svector.gl.addr;
			break;
		case TC_GEN_TVECTOR:
			shaderstate.pendingtexcoordparts[tmu] = 3;
			shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->tvector.gl.vbo;
			shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->tvector.gl.addr;
			break;
		case TC_GEN_SKYBOX:
			GenerateTCMods3(pass, tmu);
			break;
			//position - viewpos
//	case TC_GEN_WOBBLESKY:
//	case TC_GEN_REFLECT:
		default:
			//specular highlights and reflections have no fixed data, and must be generated.
			GenerateTCMods(pass, tmu);
			break;
		}
	}
	else
	{
		GenerateTCMods(pass, tmu);
	}
}
#endif

static void BE_SendPassBlendDepthMask(unsigned int sbits)
{
	unsigned int delta;

	/*2d mode doesn't depth test or depth write*/
	if (shaderstate.force2d)
	{
#ifdef warningmsg
#pragma warningmsg("fixme: q3 doesn't seem to have this, why do we need it?")
#endif
		sbits &= ~(SBITS_MISC_DEPTHWRITE|SBITS_DEPTHFUNC_BITS);
		sbits |= SBITS_MISC_NODEPTHTEST;
	}
	if (shaderstate.flags & (BEF_FORCEADDITIVE|BEF_FORCETRANSPARENT|BEF_FORCENODEPTH|BEF_FORCEDEPTHTEST|BEF_FORCEDEPTHWRITE))
	{
		if (shaderstate.flags & BEF_FORCEADDITIVE)
			sbits = (sbits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
						| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE);
		else if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			if ((sbits & SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ONE| SBITS_DSTBLEND_ZERO) || !(sbits & SBITS_BLEND_BITS) || (sbits & SBITS_ATEST_GE128)) 	/*if transparency is forced, clear alpha test bits*/
				sbits = (sbits & ~(SBITS_BLEND_BITS|SBITS_ATEST_BITS))
							| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA | SBITS_ATEST_GT0);
		}

		if (shaderstate.flags & BEF_FORCENODEPTH) 	/*EF_NODEPTHTEST dp extension*/
			sbits |= SBITS_MISC_NODEPTHTEST;
		else
		{
			if (shaderstate.flags & BEF_FORCEDEPTHTEST)
				sbits &= ~SBITS_MISC_NODEPTHTEST;
			if (shaderstate.flags & BEF_FORCEDEPTHWRITE)
				sbits |= SBITS_MISC_DEPTHWRITE;
		}
	}
	sbits |= r_refdef.colourmask;


	delta = sbits^shaderstate.shaderbits;

#ifdef FORCESTATE
	delta |= ~0;
#endif
	if (!delta)
		return;
	shaderstate.shaderbits = sbits;
	if (delta & SBITS_BLEND_BITS)
	{
		if (sbits & SBITS_BLEND_BITS)
		{
			int src, dst;
			/*unpack the src and dst factors*/
			switch(sbits & SBITS_SRCBLEND_BITS)
			{
			case SBITS_SRCBLEND_ZERO:					src = GL_ZERO;	break;
			default:
			case SBITS_SRCBLEND_ONE:					src = GL_ONE;	break;
			case SBITS_SRCBLEND_DST_COLOR:				src = GL_DST_COLOR;	break;
			case SBITS_SRCBLEND_ONE_MINUS_DST_COLOR:	src = GL_ONE_MINUS_DST_COLOR;	break;
			case SBITS_SRCBLEND_SRC_ALPHA:				src = GL_SRC_ALPHA;	break;
			case SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	src = GL_ONE_MINUS_SRC_ALPHA;	break;
			case SBITS_SRCBLEND_DST_ALPHA:				src = GL_DST_ALPHA;	break;
			case SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA:	src = GL_ONE_MINUS_DST_ALPHA;	break;
			case SBITS_SRCBLEND_ALPHA_SATURATE:			src = GL_SRC_ALPHA_SATURATE;	break;
			}
			switch((sbits & SBITS_DSTBLEND_BITS)>>4)
			{
			case SBITS_DSTBLEND_ZERO>>4:				dst = GL_ZERO;	break;
			default:
			case SBITS_DSTBLEND_ONE>>4:					dst = GL_ONE;	break;
			case SBITS_DSTBLEND_SRC_COLOR>>4:			dst = GL_SRC_COLOR;	break;
			case SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR>>4:	dst = GL_ONE_MINUS_SRC_COLOR;	break;
			case SBITS_DSTBLEND_SRC_ALPHA>>4:			dst = GL_SRC_ALPHA;	break;
			case SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA>>4:	dst = GL_ONE_MINUS_SRC_ALPHA;	break;
			case SBITS_DSTBLEND_DST_ALPHA>>4:			dst = GL_DST_ALPHA;	break;
			case SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA>>4:	dst = GL_ONE_MINUS_DST_ALPHA;	break;
			}
			qglEnable(GL_BLEND);
			qglBlendFunc(src, dst);
		}
		else
			qglDisable(GL_BLEND);
	}

#ifdef GL_ALPHA_TEST	//alpha test doesn't exist in gles2
	if ((delta & SBITS_ATEST_BITS) && !gl_config_nofixedfunc)
	{
		switch (sbits & SBITS_ATEST_BITS)
		{
		default:
			qglDisable(GL_ALPHA_TEST);
			break;
		case SBITS_ATEST_GT0:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GREATER, 0);
			break;
		case SBITS_ATEST_LT128:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_LESS, 0.5f);
			break;
		case SBITS_ATEST_GE128:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GEQUAL, 0.5f);
			break;
		}
	}
#endif

	if (delta & SBITS_MISC_NODEPTHTEST)
	{
		if (sbits & SBITS_MISC_NODEPTHTEST)
			qglDisable(GL_DEPTH_TEST);
		else
			qglEnable(GL_DEPTH_TEST);
	}
	if (delta & SBITS_MISC_DEPTHWRITE)
	{
		if (sbits & SBITS_MISC_DEPTHWRITE)
			qglDepthMask(GL_TRUE);
		else
			qglDepthMask(GL_FALSE);
	}
	if (delta & (SBITS_DEPTHFUNC_BITS))
	{
		extern int gldepthfunc;
		switch (sbits & SBITS_DEPTHFUNC_BITS)
		{
		case SBITS_DEPTHFUNC_EQUAL:
			qglDepthFunc(GL_EQUAL);
			break;
		case SBITS_DEPTHFUNC_FURTHER:
			if (gldepthfunc == GL_LEQUAL)
				qglDepthFunc(GL_GREATER);
			else
				qglDepthFunc(GL_LESS);
			break;
		case SBITS_DEPTHFUNC_CLOSER:
			if (gldepthfunc == GL_LEQUAL)
				qglDepthFunc(GL_LESS);
			else
				qglDepthFunc(GL_GREATER);
			break;
		default:
		case SBITS_DEPTHFUNC_CLOSEREQUAL:
			qglDepthFunc(gldepthfunc);
			break;
		}
	}
	if (delta & (SBITS_MASK_BITS))
	{
		qglColorMask(
				(sbits&SBITS_MASK_RED)?GL_FALSE:GL_TRUE,
				(sbits&SBITS_MASK_GREEN)?GL_FALSE:GL_TRUE,
				(sbits&SBITS_MASK_BLUE)?GL_FALSE:GL_TRUE,
				(sbits&SBITS_MASK_ALPHA)?GL_FALSE:GL_TRUE
				);
	}
	if ((delta & SBITS_TESSELLATION) && qglPNTrianglesiATI)
	{
		if ((sbits & SBITS_TESSELLATION) && r_tessellation.ival)
			qglEnable(GL_PN_TRIANGLES_ATI);
		else
			qglDisable(GL_PN_TRIANGLES_ATI);
	}

#ifdef GL_PERSPECTIVE_CORRECTION_HINT
	if ((delta & SBITS_AFFINE) && qglHint)
	{
		if (!qglHint || (gl_config_gles && gl_config.glversion >= 2) || (!gl_config_gles && gl_config_nofixedfunc))
			;	//doesn't exist in gles2 nor gl3+core contexts
		else if (sbits & SBITS_AFFINE)
			qglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		else
			qglHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	}
#endif
}

static void BE_SubmitMeshChain(qboolean usetesselation)
{
	int startv, starti, endv, endi;
	int m;
	mesh_t *mesh;
	int batchtype;

	if (usetesselation)
	{
		m = (shaderstate.flags & BEF_LINES)?2:3;
		if (shaderstate.curpatchverts != m)
		{
			shaderstate.curpatchverts = m;
			qglPatchParameteriARB(GL_PATCH_VERTICES_ARB, m);
		}
		batchtype = GL_PATCHES_ARB;
	}
	else
		batchtype = (shaderstate.flags & BEF_LINES)?GL_LINES:GL_TRIANGLES;

	if (!shaderstate.streamvbo[0])	//only if we're not forcing vbos elsewhere.
	{
		//q3map2 sucks. it splits static meshes randomly rather than with any pvs consistancy, and then splays them out over 1000 different surfaces.
		//this means we end up needing a boatload of draw calls whenever the batch got split.
		//so skip all that and splurge out a usable index list on demand.
		if (shaderstate.meshcount == 1)
		{
			GL_SelectEBO(shaderstate.sourcevbo->indicies.gl.vbo);
			mesh = shaderstate.meshes[0];
			qglDrawRangeElements(batchtype, mesh->vbofirstvert, mesh->vbofirstvert+mesh->numvertexes - 1, mesh->numindexes, GL_INDEX_TYPE, (index_t*)shaderstate.sourcevbo->indicies.gl.addr + mesh->vbofirstelement);
			RQuantAdd(RQUANT_DRAWS, 1);
			DRAWCALL("BE_SubmitMeshChain 1");
			RQuantAdd(RQUANT_PRIMITIVEINDICIES, mesh->numindexes);
			return;
		}
		else
		{
			index_t *fte_restrict ilst; //FIXME: this should be cached for multiple-pass shaders.
			GL_SelectEBO(0);

			//FIXME: use a coherant persistently mapped buffer.

			mesh = shaderstate.meshes[0];
			startv = mesh->vbofirstvert;
			endv = startv + mesh->numvertexes;
			endi = mesh->numindexes;
			for (m = 1; m < shaderstate.meshcount; m++)
			{
				mesh = shaderstate.meshes[m];
				endi += mesh->numindexes;

				if (startv > mesh->vbofirstvert)
					startv = mesh->vbofirstvert;
				if (endv < mesh->vbofirstvert+mesh->numvertexes)
					endv = mesh->vbofirstvert+mesh->numvertexes;
			}

			ilst = alloca(endi*sizeof(index_t));
			endi = 0;
			for (m = 0; m < shaderstate.meshcount; m++)
			{
				mesh = shaderstate.meshes[m];
				for (starti = 0; starti < mesh->numindexes; )
					ilst[endi++] = mesh->vbofirstvert + mesh->indexes[starti++];
			}
			qglDrawRangeElements(batchtype, startv, endv - 1, endi, GL_INDEX_TYPE, ilst);
			RQuantAdd(RQUANT_DRAWS, 1);
			DRAWCALL("BE_SubmitMeshChain N");
			RQuantAdd(RQUANT_PRIMITIVEINDICIES, endi);
		}
		return;
	}
	else if (qglMultiDrawElements)
	{	//if we're drawing via a VBO then we don't really need DrawRangeElements any more.
		//and avoiding so many calls into the driver also gives the driver a chance to optimise the draws instead of constantly checking if anything changed.
		static GLsizei counts[1024];
		static const GLvoid *indicies[countof(counts)];
		GLsizei drawcount = 0;
		GL_SelectEBO(shaderstate.sourcevbo->indicies.gl.vbo);

		for (m = 0, mesh = shaderstate.meshes[0]; m < shaderstate.meshcount; )
		{
			startv = mesh->vbofirstvert;
			starti = mesh->vbofirstelement;

			endv = startv+mesh->numvertexes;
			endi = starti+mesh->numindexes;

			//find consecutive surfaces
			for (++m; m < shaderstate.meshcount; m++)
			{
				mesh = shaderstate.meshes[m];
				if (endi == mesh->vbofirstelement)
				{
					endv = mesh->vbofirstvert+mesh->numvertexes;
					endi = mesh->vbofirstelement+mesh->numindexes;
				}
				else
				{
					break;
				}
			}
			if (drawcount == countof(counts))
			{
				qglMultiDrawElements(batchtype, counts, GL_INDEX_TYPE, indicies, drawcount);
				RQuantAdd(RQUANT_DRAWS, drawcount);
				DRAWCALL("BE_SubmitMeshChain MultiDraw");
				drawcount = 0;
			}
			counts[drawcount] = endi-starti;
			indicies[drawcount] = (index_t*)shaderstate.sourcevbo->indicies.gl.addr + starti;
			drawcount++;
			RQuantAdd(RQUANT_PRIMITIVEINDICIES, endi-starti);
		}
		qglMultiDrawElements(batchtype, counts, GL_INDEX_TYPE, indicies, drawcount);
		RQuantAdd(RQUANT_DRAWS, drawcount);
		DRAWCALL("BE_SubmitMeshChain MultiDraw");
	}
#if 0 //def FTE_TARGET_WEB
	else if (shaderstate.meshcount > 1)
	{	//FIXME: not really needed if index lists are consecutive
		index_t *tmp;
		int ebo;

		for (endi = 0, m = 0; m < shaderstate.meshcount; m++)
		{
			mesh = shaderstate.meshes[m];
			endi += mesh->numindexes;
		}
		tmp = alloca(endi * sizeof(*tmp));
		for (endi = 0, m = 0; m < shaderstate.meshcount; m++)
		{
			mesh = shaderstate.meshes[m];
			for (starti = 0; starti < mesh->numindexes; starti++)
				tmp[endi++] = mesh->vbofirstvert + mesh->indexes[starti];
		}

		shaderstate.streamid = (shaderstate.streamid + 1) & (sizeof(shaderstate.streamvbo)/sizeof(shaderstate.streamvbo[0]) - 1);
		ebo = shaderstate.streamebo[shaderstate.streamid];
		GL_SelectEBO(ebo);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(*tmp) * endi, tmp, GL_STREAM_DRAW_ARB);
		qglDrawElements(batchtype, endi, GL_INDEX_TYPE, NULL);
		RQuantAdd(RQUANT_DRAWS, 1);
		RQuantAdd(RQUANT_PRIMITIVEINDICIES, endi);
	}
#endif
	else
	{
		GL_SelectEBO(shaderstate.sourcevbo->indicies.gl.vbo);

/*
		if (qglLockArraysEXT)
		{
			endv = 0;
			startv = 0x7fffffff;
			for (m = 0; m < shaderstate.meshcount; m++)
			{
				mesh = shaderstate.meshes[m];
				starti = mesh->vbofirstvert;
				if (starti < startv)
					startv = starti;
				endi = mesh->vbofirstvert+mesh->numvertexes;
				if (endi > endv)
					endv = endi;
			}
			qglLockArraysEXT(startv, endv);
		}
*/

		for (m = 0, mesh = shaderstate.meshes[0]; m < shaderstate.meshcount; )
		{
			startv = mesh->vbofirstvert;
			starti = mesh->vbofirstelement;

			endv = startv+mesh->numvertexes;
			endi = starti+mesh->numindexes;

			//find consecutive surfaces
			for (++m; m < shaderstate.meshcount; m++)
			{
				mesh = shaderstate.meshes[m];
				if (endi == mesh->vbofirstelement)
				{
					endv = mesh->vbofirstvert+mesh->numvertexes;
					endi = mesh->vbofirstelement+mesh->numindexes;
				}
				else
				{
					break;
				}
			}
			qglDrawRangeElements(batchtype, startv, endv - 1, endi-starti, GL_INDEX_TYPE, (index_t*)shaderstate.sourcevbo->indicies.gl.addr + starti);
			RQuantAdd(RQUANT_DRAWS, 1);
			DRAWCALL("BE_SubmitMeshChain Fallback");
			RQuantAdd(RQUANT_PRIMITIVEINDICIES, endi-starti);
 		}
/*
		if (qglUnlockArraysEXT)
			qglUnlockArraysEXT();
*/
	}
}

#ifndef GLSLONLY
static void DrawPass(const shaderpass_t *pass)
{
	int i;
#if MAXRLIGHTMAPS > 1
	int j, k;
#endif
	int tmu;
	int lastpass = pass->numMergedPasses;
	unsigned int attr = (1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR);

	for (i = 0; i < lastpass; i++)
	{
		if (pass[i].texgen == T_GEN_UPPEROVERLAY && !TEXLOADED(shaderstate.curtexnums->upperoverlay))
			continue;
		if (pass[i].texgen == T_GEN_LOWEROVERLAY && !TEXLOADED(shaderstate.curtexnums->loweroverlay))
			continue;
		if (pass[i].texgen == T_GEN_FULLBRIGHT && !TEXLOADED(shaderstate.curtexnums->fullbright))
			continue;
		break;
	}
	if (i == lastpass)
		return;
	BE_SendPassBlendDepthMask(pass[i].shaderbits);
	GenerateColourMods(pass+i);
	tmu = 0;
	for (; i < lastpass; i++)
	{
		if (pass[i].texgen == T_GEN_UPPEROVERLAY && !TEXLOADED(shaderstate.curtexnums->upperoverlay))
			continue;
		if (pass[i].texgen == T_GEN_LOWEROVERLAY && !TEXLOADED(shaderstate.curtexnums->loweroverlay))
			continue;
		if (pass[i].texgen == T_GEN_FULLBRIGHT && !TEXLOADED(shaderstate.curtexnums->fullbright))
			continue;
		Shader_BindTextureForPass(tmu, pass+i);
		attr |= (1u<<(VATTR_LEG_TMU0+tmu));

		BE_GeneratePassTC(pass+i, tmu);

		BE_SetPassBlendMode(tmu, pass[i].blendmode);

		tmu++;

		//add in 
		if (pass[i].texgen == T_GEN_LIGHTMAP)
		{
			//first pass should have been REPLACE
			//second pass should be an ADD
			//this depends upon rgbgens for light levels, so each pass *must* be pushed to hardware individually

#if MAXRLIGHTMAPS > 1
			for (j = 1; j < MAXRLIGHTMAPS && shaderstate.curbatch->lightmap[j] >= 0; j++)
			{
				if (j == 1)
					BE_SetPassBlendMode(tmu, PBM_REPLACE);

				/*make sure no textures linger*/
				for (k = tmu; k < shaderstate.lastpasstmus; k++)
				{
					GL_LazyBind(k, r_nulltex);
				}
				shaderstate.lastpasstmus = tmu;

				/*push it*/
				BE_EnableShaderAttributes(attr, 0);
				BE_SubmitMeshChain(false);
				tmu = 0;

				/*bind the light texture*/
				GL_LazyBind(tmu, lightmap[shaderstate.curbatch->lightmap[j]]->lightmap_texture);

				/*set up the colourmod for this style's lighting*/
				shaderstate.pendingcolourvbo = 0;
				shaderstate.pendingcolourpointer = NULL;

				shaderstate.pendingcolourflat[0] = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->lmlightstyle[j]]/256.0f;
				shaderstate.pendingcolourflat[1] = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->lmlightstyle[j]]/256.0f;
				shaderstate.pendingcolourflat[2] = shaderstate.identitylighting * d_lightstylevalue[shaderstate.curbatch->lmlightstyle[j]]/256.0f;
				shaderstate.pendingcolourflat[3] = 1;

				/*pick the correct st coords for this lightmap pass*/
				shaderstate.pendingtexcoordparts[tmu] = 2;
				shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->lmcoord[j].gl.vbo;
				shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->lmcoord[j].gl.addr;

				BE_SetPassBlendMode(tmu, PBM_ADD);
				BE_SendPassBlendDepthMask((pass[0].shaderbits & ~SBITS_BLEND_BITS) | SBITS_SRCBLEND_ONE | SBITS_DSTBLEND_ONE);

				attr = (1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR);
				attr |= (1u<<(VATTR_LEG_TMU0+tmu));

				tmu++;
			}

			//might need to break the pass here
			if (j > 1 && i != lastpass)
			{
				for (k = tmu; k < shaderstate.lastpasstmus; k++)
				{
					GL_LazyBind(k, r_nulltex);
				}
				shaderstate.lastpasstmus = tmu;
				BE_EnableShaderAttributes(attr, 0);

				BE_SubmitMeshChain(false);
				tmu = 0;

				BE_SendPassBlendDepthMask(pass[i+1].shaderbits);
				GenerateColourMods(&pass[i+1]);
			}
#endif
		}
	}

	if (!tmu)
		return;

	for (i = tmu; i < shaderstate.lastpasstmus; i++)
	{
		GL_LazyBind(i, r_nulltex);
	}
	shaderstate.lastpasstmus = tmu;
	BE_EnableShaderAttributes(attr, 0);

	BE_SubmitMeshChain(false);
}
#endif

static void BE_Program_Set_Attributes(const program_t *prog, struct programpermu_s *perm, qboolean entunchanged)
{
	vec4_t param4;
	int r, g;//, b;
	int i;
	unsigned int ph;
	const shaderprogparm_t *p;

	if (perm->factorsuniform != -1)
		qglUniform4fvARB(perm->factorsuniform, countof(shaderstate.curshader->factors), shaderstate.curshader->factors[0]);

	/*don't bother setting it if the ent properties are unchanged (but do if the mesh changed)*/
	if (entunchanged)
		return;

	p = perm->parm;
	for (i = perm->numparms; i > 0; i--, p++)
	{
		ph = p->handle;
		switch(p->type)
		{
/*
		case SP_UBO_ENTITYINFO:
			struct
			{
				vec2_t		blendweights;
					vec2_t	pad1;
				vec3_t		glowmod;
					vec_t	pad2;
				vec3_t		origin;
					vec_t	pad3;
				vec4_t		colormod;
				vec3_t		glowmod;
					vec_t	pad4;
				vec3_t		uppercolour;
					vec_t	pad5;
				vec3_t		lowercolour;
					vec_t	pad6;
				vec3_t		fogcolours;
				vec_t		fogalpha;
				vec3_t		vlightdir;
				vec_t		fogdensity;
				vec3_t		vlightmul;
				vec_t		fogdepthbias;
				vec3_t		vlightadd;
				vec_t		time;
			} u_entityinfo;

			Vector2Copy(shaderstate.meshes[0]->xyz_blendw, u_entityinfo.blendweights);
			VectorCopy(shaderstate.curentity->glowmod, u_entityinfo.glowmod);
			VectorCopy(shaderstate.curentity->origin, u_entityinfo.origin);
			Vector4Copy(shaderstate.curentity->shaderRGBAf, u_entityinfo.colormod);
			R_FetchPlayerColour(shaderstate.curentity->topcolour, u_entityinfo.uppercolour);
			R_FetchPlayerColour(shaderstate.curentity->bottomcolour, u_entityinfo.lowercolour);
			Vector3Copy(r_refdef.globalfog.colour, u_entityinfo.fogcolours);
			u_entityinfo.fogalpha = r_refdef.globalfog.alpha;
			u_entityinfo.fogdensity = r_refdef.globalfog.density;
			u_entityinfo.fogdepthbias = r_refdef.globalfog.depthbias;
			u_entityinfo.time = shaderstate.curtime;
			Vector3Copy(shaderstate.curentity->light_dir, u_entityinfo.vlightdir);
			Vector3Copy(shaderstate.curentity->light_range, u_entityinfo.vlightmul);
			Vector3Copy(shaderstate.curentity->light_avg, u_entityinfo.vlightadd);
			break;
*/

/*
		case SP_UBO_LIGHTINFO:
			struct
			{
				vec3_t		toscreen;
				vec_t		lightradius;
				vec3_t		lightcolours;
					vec_t	pad1;
				vec3_t		lightcolourscale;
					vec_t	pad2;
				vec3_t		lightorigin_modelspace;
					vec_t	pad3;
				matrix4x4_t	lightcubematrix;
				vec4_t		lightshadowmapproj;
				vec2_t		lightshadowmapscale;
					vec2_t	pad4;
			} u_lightinfo;
			{
				float v[4], tempv[4];

				v[0] = shaderstate.lightorg[0];
				v[1] = shaderstate.lightorg[1];
				v[2] = shaderstate.lightorg[2];
				v[3] = 1;

				Matrix4x4_CM_Transform4(shaderstate.modelviewmatrix, v, tempv); 
				Matrix4x4_CM_Transform4(r_refdef.m_projection, tempv, v);

				v[3] *= 2;
				u_lightinfo.toscreen[0] = (v[0]/v[3]) + 0.5;
				u_lightinfo.toscreen[1] = (v[1]/v[3]) + 0.5;
				u_lightinfo.toscreen[2] = (v[2]/v[3]) + 0.5;
			}
			u_lightinfo.lightradius = shaderstate.lightradius;
			Vector3Copy(shaderstate.lightcolours, u_lightinfo.lightcolours);

			Matrix4x4_CM_Transform3(shaderstate.modelmatrixinv, shaderstate.lightorg, u_lightinfo.lightorigin_modelspace);
			Vector3Copy(shaderstate.lightcolourscale, u_lightinfo.lightcolourscale);
			Matrix4_Multiply(shaderstate.lightprojmatrix, shaderstate.modelmatrix, u_lightinfo.lightcubematrix);
			Vector4Copy(shaderstate.lightshadowmapproj, u_lightinfo.lightshadowmapproj);
			Vector2Copy(shaderstate.lightshadowmapscale, u_lightinfo.lightshadowmapscale);
			break;
*/
		case SP_M_VIEW:
			qglUniformMatrix4fvARB(ph, 1, false, r_refdef.m_view);
			break;
		case SP_M_PROJECTION:
			qglUniformMatrix4fvARB(ph, 1, false, shaderstate.projectionmatrix);
			break;
		case SP_M_MODELVIEW:
			qglUniformMatrix4fvARB(ph, 1, false, shaderstate.modelviewmatrix);
			break;
		case SP_M_MODELVIEWPROJECTION:
			{
				float m16[16];
				Matrix4_Multiply(shaderstate.projectionmatrix, shaderstate.modelviewmatrix, m16);
				qglUniformMatrix4fvARB(ph, 1, false, m16);
			}
			break;
		case SP_M_INVMODELVIEWPROJECTION:
			{
				float m16[16], inv[16];
				Matrix4_Multiply(shaderstate.projectionmatrix, shaderstate.modelviewmatrix, m16);
				Matrix4_Invert(m16, inv);
				qglUniformMatrix4fvARB(ph, 1, false, inv);
			}
			break;
		case SP_M_INVMODELVIEW:
			{
				float inv[9];
				Matrix3x4_InvertTo3x3(shaderstate.modelviewmatrix, inv);
				qglUniformMatrix3fvARB(ph, 1, false, inv);
			}
			break;
		case SP_M_MODEL:
			qglUniformMatrix4fvARB(ph, 1, false, shaderstate.modelmatrix);
			break;
		case SP_M_ENTBONES_PACKED:
			qglUniform4fvARB(ph, shaderstate.sourcevbo->numbones*3, shaderstate.sourcevbo->bones);
			break;
		case SP_M_ENTBONES_MAT3X4:
			qglUniformMatrix3x4fv(ph, shaderstate.sourcevbo->numbones, false, shaderstate.sourcevbo->bones);
			break;
		case SP_M_INVVIEWPROJECTION:
			{
				float m16[16], inv[16];
				Matrix4_Multiply(shaderstate.projectionmatrix, r_refdef.m_view, m16);
				Matrix4_Invert(m16, inv);
				qglUniformMatrix4fvARB(ph, 1, false, inv);
			}
			break;

		case SP_E_VBLEND:
			qglUniform2fvARB(ph, 1, shaderstate.meshes[0]->xyz_blendw);
			break;

		case SP_E_VLSCALE:
#if MAXRLIGHTMAPS > 1
			if (perm->permutation & PERMUTATION_LIGHTSTYLES)
			{
				vec4_t colscale[MAXRLIGHTMAPS];
				int j, s;
				for (j = 0; j < MAXRLIGHTMAPS ; j++)
				{
					s = shaderstate.curbatch->vtlightstyle[j];
					if (s == 255)
					{
						for (; j < MAXRLIGHTMAPS ; j++)
						{
							colscale[j][0] = 0;
							colscale[j][1] = 0;
							colscale[j][2] = 0;
							colscale[j][3] = 1;
						}
						break;
					}
					if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
					{
						float sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
						VectorSet(colscale[j], sc, sc, sc);
					}
					else
					{
						VectorSet(colscale[j], shaderstate.identitylighting, shaderstate.identitylighting, shaderstate.identitylighting);
					}
					colscale[j][3] = 1;

					VectorScale(colscale[j], d_lightstylevalue[s]/256.0f, colscale[j]);
				}

				qglUniform4fvARB(ph, j, (GLfloat*)colscale);
				shaderstate.lastuniform = 0;
			}
			else
#endif
			{
				vec4_t param4;
				if (shaderstate.curbatch->flags & BEF_NODLIGHT)
				{
					Vector4Set(param4, 1, 1, 1, 1);
				}
				else
				{
					Vector4Set(param4, shaderstate.identitylighting, shaderstate.identitylighting, shaderstate.identitylighting, 1);
				}
				qglUniform4fvARB(ph, 1, (GLfloat*)param4);
			}
			break;
		case SP_E_LMSCALE:
#if MAXRLIGHTMAPS > 1
			if (perm->permutation & PERMUTATION_LIGHTSTYLES)
			{
				vec4_t colscale[MAXRLIGHTMAPS];
				int j, s;
				for (j = 0; j < MAXRLIGHTMAPS ; j++)
				{
					s = shaderstate.curbatch->lmlightstyle[j];
					if (s == INVALID_LIGHTSTYLE)
					{
						for (; j < MAXRLIGHTMAPS ; j++)
						{
							colscale[j][0] = 0;
							colscale[j][1] = 0;
							colscale[j][2] = 0;
							colscale[j][3] = 1;
						}
						break;
					}
					if (shaderstate.curentity->model && (shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT) && !shaderstate.force2d)
					{
						float sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
						VectorSet(colscale[j], sc, sc, sc);
					}
					else
					{
						VectorSet(colscale[j], shaderstate.identitylighting, shaderstate.identitylighting, shaderstate.identitylighting);
					}
					colscale[j][3] = 1;

					VectorScale(colscale[j], d_lightstylevalue[s]/256.0f, colscale[j]);
				}

				qglUniform4fvARB(ph, j, (GLfloat*)colscale);
				shaderstate.lastuniform = 0;
			}
			else
#endif
			{
				unsigned short s;
				if (shaderstate.curentity->model && (shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT) && !shaderstate.force2d)
				{
					float sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
					Vector4Set(param4, sc, sc, sc, 1);
				}
				else
				{
					Vector4Set(param4, shaderstate.identitylighting, shaderstate.identitylighting, shaderstate.identitylighting, 1);
				}

				s = shaderstate.curbatch->lmlightstyle[0];	//only one style.
				if (s != INVALID_LIGHTSTYLE)
					VectorScale(param4, d_lightstylevalue[s]/256.0f, param4);

				qglUniform4fvARB(ph, 1, (GLfloat*)param4);
			}
			break;

		case SP_E_GLOWMOD:
			qglUniform3fvARB(ph, 1, (GLfloat*)shaderstate.curentity->glowmod);
			break;
		case SP_E_ORIGIN:
			qglUniform3fvARB(ph, 1, (GLfloat*)shaderstate.curentity->origin);
			break;
		case SP_E_COLOURS:
			qglUniform4fvARB(ph, 1, (GLfloat*)shaderstate.curentity->shaderRGBAf);
			break;
		case SP_S_COLOUR:
			if (shaderstate.colourarraytype)
				qglUniform4fARB(ph, 1, 1, 1, 1);	//invalid use
			else
				qglUniform4fvARB(ph, 1, (GLfloat*)shaderstate.pendingcolourflat);
			break;
		case SP_E_COLOURSIDENT:
			if (shaderstate.flags & BEF_FORCECOLOURMOD)
				qglUniform4fvARB(ph, 1, (GLfloat*)shaderstate.curentity->shaderRGBAf);
			else
				qglUniform4fARB(ph, 1, 1, 1, shaderstate.curentity->shaderRGBAf[3]);
			break;
		case SP_E_TOPCOLOURS:
			R_FetchPlayerColour(shaderstate.curentity->topcolour, param4);
			qglUniform3fvARB(ph, 1, param4);
			break;
		case SP_E_BOTTOMCOLOURS:
			R_FetchPlayerColour(shaderstate.curentity->bottomcolour, param4);
			qglUniform3fvARB(ph, 1, param4);
			break;

		case SP_SOURCESIZE:
			if (shaderstate.tex_sourcecol)
			{
				param4[0] = shaderstate.tex_sourcecol->width;
				param4[1] = shaderstate.tex_sourcecol->height;
			}
			else if (shaderstate.tex_sourcedepth)
			{
				param4[0] = shaderstate.tex_sourcedepth->width;
				param4[1] = shaderstate.tex_sourcedepth->height;
			}
			else
			{
				param4[0] = 1;
				param4[1] = 1;
			}
			qglUniform2fvARB(ph, 1, param4);
			break;
		case SP_RENDERTEXTURESCALE:
			if (sh_config.texture_non_power_of_two_pic)
			{
				param4[0] = 1;
				param4[1] = 1;
			}
			else
			{
				r = 1;
				g = 1;
				while (r < vid.pixelwidth)
					param4[0] *= 2;
				while (g < vid.pixelheight)
					param4[1] *= 2;
				param4[0] = vid.pixelwidth/param4[0];
				param4[1] = vid.pixelheight/param4[1];
			}
			param4[2] = 0;
			param4[3] = 0;
			qglUniform4fvARB(ph, 1, param4);
			break;

		case SP_LIGHTSCREEN:
			{
				float v[4], tempv[4];

				v[0] = shaderstate.lightorg[0];
				v[1] = shaderstate.lightorg[1];
				v[2] = shaderstate.lightorg[2];
				v[3] = 1;

				Matrix4x4_CM_Transform4(shaderstate.modelviewmatrix, v, tempv); 
				Matrix4x4_CM_Transform4(shaderstate.projectionmatrix, tempv, v);

				v[3] *= 2;
				v[0] = (v[0]/v[3]) + 0.5;
				v[1] = (v[1]/v[3]) + 0.5;
				v[2] = (v[2]/v[3]) + 0.5;

				qglUniform3fvARB(ph, 1, v);
			}
			break;
		case SP_LIGHTRADIUS:
			qglUniform1fARB(ph, shaderstate.lightradius);
			break;
		case SP_LIGHTCOLOUR:
			qglUniform3fvARB(ph, 1, shaderstate.lightcolours);
			break;
		case SP_W_FOG:
			qglUniform4fvARB(ph, 2, r_refdef.globalfog.colour);	//and density
			break;
		case SP_W_USER:
			qglUniform4fvARB(ph, countof(r_refdef.userdata), r_refdef.userdata[0]);	//and density
			break;
		case SP_V_EYEPOS:
			qglUniform3fvARB(ph, 1, r_origin);
			break;
		case SP_E_EYEPOS:
			{
				/*eye position in model space*/
				vec3_t t2;
				Matrix4x4_CM_Transform3(shaderstate.modelmatrixinv, r_origin, t2);
				qglUniform3fvARB(ph, 1, t2);
			}
			break;
		case SP_LIGHTPOSITION:
			{
				/*light position in model space*/
				vec3_t t2;
				Matrix4x4_CM_Transform3(shaderstate.modelmatrixinv, shaderstate.lightorg, t2);
				qglUniform3fvARB(ph, 1, t2);
			}
			break;
		case SP_LIGHTDIRECTION:
			{
				/*light position in model space*/
				vec3_t t2;
				Matrix4x4_CM_Transform3x3(shaderstate.modelmatrixinv, shaderstate.lightdir, t2);
				qglUniform3fvARB(ph, 1, t2);
			}
			break;
		case SP_LIGHTCOLOURSCALE:
			qglUniform3fvARB(ph, 1, shaderstate.lightcolourscale);
			break;
		case SP_LIGHTCUBEMATRIX:
			/*light's texture projection matrix*/
			{
				float t[16];
				Matrix4_Multiply(shaderstate.lightprojmatrix, shaderstate.modelmatrix, t);
				qglUniformMatrix4fvARB(ph, 1, false, t);
			}
			break;
		case SP_LIGHTSHADOWMAPPROJ:
			qglUniform4fvARB(ph, 1, shaderstate.lightshadowmapproj);
			break;
		case SP_LIGHTSHADOWMAPSCALE:
			qglUniform2fvARB(ph, 1, shaderstate.lightshadowmapscale);
			break;

		/*static lighting info*/
		case SP_E_L_DIR:
			qglUniform3fvARB(ph, 1, (float*)shaderstate.curentity->light_dir);
			break;
		case SP_E_L_MUL:
			if (shaderstate.mode == BEM_DEPTHDARK)
				qglUniform3fvARB(ph, 1, vec3_origin);
			else
				qglUniform3fvARB(ph, 1, (float*)shaderstate.curentity->light_range);
			break;
		case SP_E_L_AMBIENT:
			if (shaderstate.mode == BEM_DEPTHDARK)
				qglUniform3fvARB(ph, 1, vec3_origin);
			else
				qglUniform3fvARB(ph, 1, (float*)shaderstate.curentity->light_avg);
			break;

		case SP_E_TIME:
			qglUniform1fARB(ph, shaderstate.curtime);
			break;
		case SP_CONST1I:
		case SP_TEXTURE:
			qglUniform1iARB(ph, p->ival[0]);
			break;
		case SP_CONST1F:
			qglUniform1fARB(ph, p->fval[0]);
			break;
		case SP_CONST2F:
			qglUniform3fARB(ph, p->fval[0], p->fval[1], 0);
			break;
		case SP_CONST3F:
			qglUniform3fARB(ph, p->fval[0], p->fval[1], p->fval[2]);
			break;
		case SP_CONST4F:
			qglUniform4fARB(ph, p->fval[0], p->fval[1], p->fval[2], p->fval[3]);
			break;
		case SP_CVARI:
			qglUniform1iARB(ph, ((cvar_t*)p->pval)->ival);
			break;
		case SP_CVARF:
			qglUniform1fARB(ph, ((cvar_t*)p->pval)->value);
			break;
		case SP_CVAR3F:
			qglUniform3fvARB(ph, 1, ((cvar_t*)p->pval)->vec4);
			break;
		case SP_CVAR4F:
			qglUniform3fvARB(ph, 1, ((cvar_t*)p->pval)->vec4);
			break;

		default:
			Host_EndGame("Bad shader program parameter type (%i)", p->type);
			break;
		}
	}
}

static void BE_RenderMeshProgram(const shader_t *shader, const shaderpass_t *pass, program_t *p)
{
	int	i;

	int perm;

	struct programpermu_s *permu;

	perm = 0;
	if (shaderstate.sourcevbo->numbones)
		perm |= PERMUTATION_SKELETAL;
#ifdef NONSKELETALMODELS
	if (shaderstate.sourcevbo->coord2.gl.addr)
		perm |= PERMUTATION_FRAMEBLEND;
#endif
	if (TEXLOADED(shaderstate.curtexnums->bump))
		perm |= PERMUTATION_BUMPMAP;
	if (TEXLOADED(shaderstate.curtexnums->fullbright))
		perm |= PERMUTATION_FULLBRIGHT;
	if ((TEXLOADED(shaderstate.curtexnums->loweroverlay) || TEXLOADED(shaderstate.curtexnums->upperoverlay)))
		perm |= PERMUTATION_UPPERLOWER;
	if (r_refdef.globalfog.density)
		perm |= PERMUTATION_FOG;
//	if (TEXLOADED(shaderstate.curtexnums->bump) && shaderstate.curbatch->lightmap[0] >= 0 && lightmap[shaderstate.curbatch->lightmap[0]]->hasdeluxe)
//		perm |= PERMUTATION_DELUXE;
	if ((TEXLOADED(shaderstate.curtexnums->reflectcube) || TEXLOADED(shaderstate.curtexnums->reflectmask)))
		perm |= PERMUTATION_REFLECTCUBEMASK;
#if MAXRLIGHTMAPS > 1
	if (shaderstate.curbatch->lightmap[1] >= 0)
		perm |= PERMUTATION_LIGHTSTYLES;
#endif

	perm &= p->supportedpermutations;
	permu = p->permu[perm];
	if (!permu)
	{
		p->permu[perm] = permu = Shader_LoadPermutation(p, perm);
		if (!permu)
		{	//failed? copy from 0 so we don't keep re-failing
			permu = p->permu[perm] = p->permu[0];
		}
	}

	GL_SelectProgram(permu->h.glsl.handle);
#ifndef FORCESTATE
	if (shaderstate.lastuniform == shaderstate.currentprogram)
		i = true;
	else
#endif
	{
		i = false;
		shaderstate.lastuniform = shaderstate.currentprogram;
	}
	BE_Program_Set_Attributes(p, permu, i);

	BE_SendPassBlendDepthMask(pass->shaderbits);

#ifndef GLSLONLY
	if (p->calcgens)
	{
		GenerateColourMods(pass);
		for (i = 0; i < pass->numMergedPasses; i++)
		{
			Shader_BindTextureForPass(i, pass+i);
			BE_GeneratePassTC(pass+i, i);
		}
	}
	else
#endif
	{
		for (i = 0; i < pass->numMergedPasses; i++)
		{
			Shader_BindTextureForPass(i, pass+i);
		}
	}
#if MAXRLIGHTMAPS > 1
	if (perm & PERMUTATION_LIGHTSTYLES)
	{
		GL_LazyBind(i++, shaderstate.curbatch->lightmap[1]>=0?lightmap[shaderstate.curbatch->lightmap[1]]->lightmap_texture:r_nulltex);
		GL_LazyBind(i++, shaderstate.curbatch->lightmap[2]>=0?lightmap[shaderstate.curbatch->lightmap[2]]->lightmap_texture:r_nulltex);
		GL_LazyBind(i++, shaderstate.curbatch->lightmap[3]>=0?lightmap[shaderstate.curbatch->lightmap[3]]->lightmap_texture:r_nulltex);
		GL_LazyBind(i++, (shaderstate.curbatch->lightmap[1]>=0&&lightmap[shaderstate.curbatch->lightmap[1]]->hasdeluxe)?lightmap[shaderstate.curbatch->lightmap[1]+1]->lightmap_texture:missing_texture_normal);
		GL_LazyBind(i++, (shaderstate.curbatch->lightmap[2]>=0&&lightmap[shaderstate.curbatch->lightmap[2]]->hasdeluxe)?lightmap[shaderstate.curbatch->lightmap[2]+1]->lightmap_texture:missing_texture_normal);
		GL_LazyBind(i++, (shaderstate.curbatch->lightmap[3]>=0&&lightmap[shaderstate.curbatch->lightmap[3]]->hasdeluxe)?lightmap[shaderstate.curbatch->lightmap[3]+1]->lightmap_texture:missing_texture_normal);
	}
#endif
	while (shaderstate.lastpasstmus > i)
		GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
	shaderstate.lastpasstmus = i;

	BE_EnableShaderAttributes(permu->attrmask, shaderstate.sourcevbo->vao);
	BE_SubmitMeshChain(permu->h.glsl.usetesselation);
}

qboolean GLBE_LightCullModel(vec3_t org, model_t *model)
{
#ifdef RTLIGHTS
	if ((shaderstate.mode == BEM_LIGHT || shaderstate.mode == BEM_STENCIL || shaderstate.mode == BEM_DEPTHONLY))
	{
		float dist;
		vec3_t disp;
		if (model->type == mod_alias)
		{
			VectorSubtract(org, shaderstate.lightorg, disp);
			dist = DotProduct(disp, disp);
			if (dist > model->radius*model->radius + shaderstate.lightradius*shaderstate.lightradius)
				return true;
		}
		else
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				if (shaderstate.lightorg[i]-shaderstate.lightradius > org[i] + model->maxs[i])
					return true;
				if (shaderstate.lightorg[i]+shaderstate.lightradius < org[i] + model->mins[i])
					return true;
			}
		}
	}
#endif
	return false;
}

//Note: Be cautious about using BEM_LIGHT here, as it won't select the light.
void GLBE_SelectMode(backendmode_t mode)
{
	extern cvar_t r_polygonoffset_shadowmap_offset, r_polygonoffset_shadowmap_factor;
	extern int gldepthfunc;

//	shaderstate.lastuniform = 0;
#ifndef FORCESTATE
	if (mode != shaderstate.mode)
#endif
	{
		shaderstate.mode = mode;
		shaderstate.flags = 0;
		shaderstate.polyoffset.factor = 0;
		shaderstate.polyoffset.unit = 0;
		switch (mode)
		{
		default:
			break;
		case BEM_WIREFRAME:
			if (!shaderstate.wireframeshader && gl_config.arb_shader_objects)
				shaderstate.wireframeshader = R_RegisterShader("wireframe", SUF_NONE,
					"{\n"
						"program wireframe\n"
						"{\n"
							"nodepthtest\n"
						"}\n"
					"}\n"
				);
			break;
		case BEM_DEPTHONLY:
			shaderstate.polyoffset.factor = r_polygonoffset_shadowmap_factor.value;
			shaderstate.polyoffset.unit = r_polygonoffset_shadowmap_offset.value;
#ifndef GLSLONLY
			if (!gl_config_nofixedfunc)
			{
				BE_SetPassBlendMode(0, PBM_REPLACE);
				GL_DeSelectProgram();
			}
			else
#endif
				if (!shaderstate.allblackshader.glsl.handle)
			{
				const char *defs[] = {NULL};
				shaderstate.allblackshader = GLSlang_CreateProgram(NULL, "allblackprogram", sh_config.minver, defs, "#include \"sys/skeletal.h\"\nvoid main(){gl_Position = skeletaltransform();}", NULL, NULL, NULL, "void main(){gl_FragColor=vec4(0.0,0.0,0.0,1.0);}", false, NULL);
				shaderstate.allblack_mvp = qglGetUniformLocationARB(shaderstate.allblackshader.glsl.handle, "m_modelviewprojection");
			}
			/*BEM_DEPTHONLY does support mesh writing, but its not the only way its used... FIXME!*/
			while(shaderstate.lastpasstmus>0)
			{
				GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
			}

			//we don't write or blend anything (maybe alpha test... but mneh)
			BE_SendPassBlendDepthMask(SBITS_MISC_DEPTHWRITE | SBITS_MASK_BITS);

//			BE_PolyOffset(false);

			GL_CullFace(SHADER_CULL_FRONT);
			break;

#ifdef RTLIGHTS
		case BEM_STENCIL:
			/*BEM_STENCIL doesn't support mesh writing*/
#ifdef BEF_PUSHDEPTH
			GLBE_PolyOffsetStencilShadow(false);
#else
			GLBE_PolyOffsetStencilShadow();
#endif

			if (gl_config_nofixedfunc && !shaderstate.allblackshader.glsl.handle)
			{
				const char *defs[] = {NULL};
				shaderstate.allblackshader = GLSlang_CreateProgram(NULL, "allblackprogram", sh_config.minver, defs, "#include \"sys/skeletal.h\"\nvoid main(){gl_Position = skeletaltransform();}", NULL, NULL, NULL, "void main(){gl_FragColor=vec4(0.0,0.0,0.0,1.0);}", false, NULL);
				shaderstate.allblack_mvp = qglGetUniformLocationARB(shaderstate.allblackshader.glsl.handle, "m_modelviewprojection");
			}

			//disable all tmus
			while(shaderstate.lastpasstmus>0)
			{
				GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
			}
#ifndef GLSLONLY
			if (!gl_config_nofixedfunc)
			{
				GL_DeSelectProgram();

			//replace mode please
				BE_SetPassBlendMode(0, PBM_REPLACE);
			}
#endif

			//we don't write or blend anything (maybe alpha test... but mneh)
			BE_SendPassBlendDepthMask(SBITS_DEPTHFUNC_CLOSER | SBITS_MASK_BITS);
			GL_CullFace(0);

			//don't change cull stuff, and
			//don't actually change stencil stuff - caller needs to be
			//aware of how many times stuff is drawn, so they can do that themselves.
			break;
		case BEM_CREPUSCULAR:
			if (!shaderstate.crepopaqueshader)
			{
				shaderstate.crepopaqueshader = R_RegisterShader("crepuscular_opaque", SUF_NONE,
					"{\n"
						"program crepuscular_opaque\n"
					"}\n"
					);
			}
			if (!shaderstate.crepskyshader)
			{
				shaderstate.crepskyshader = R_RegisterShader("crepuscular_sky", SUF_NONE,
					"{\n"
						"program crepuscular_sky\n"
						"{\n"
							"map $diffuse\n"
						"}\n"
						"{\n"
							"map $fullbright\n"
						"}\n"
					"}\n"
					);
			}
			break;
#endif
		case BEM_FOG:
			while(shaderstate.lastpasstmus>0)
			{
				GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
			}
			GL_LazyBind(0, shaderstate.fogtexture);
			shaderstate.lastpasstmus = 1;

			Vector4Set(shaderstate.pendingcolourflat, 1, 1, 1, 1);
			shaderstate.pendingcolourvbo = 0;
			shaderstate.pendingcolourpointer = NULL;
#ifndef GLSLONLY
			if (!gl_config_nofixedfunc)
				BE_SetPassBlendMode(0, PBM_MODULATE);
#endif
			BE_SendPassBlendDepthMask(SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA | SBITS_DEPTHFUNC_EQUAL);
			break;
		}
	}
}

void GLBE_SelectEntity(entity_t *ent)
{
	unsigned int fl;
	shaderstate.curentity = ent;
	currententity = ent;
	R_RotateForEntity(shaderstate.modelmatrix, shaderstate.modelviewmatrix, shaderstate.curentity, shaderstate.curentity->model);
	Matrix4_Invert(shaderstate.modelmatrix, shaderstate.modelmatrixinv);
	if (qglLoadMatrixf)
		qglLoadMatrixf(shaderstate.modelviewmatrix);


	fl = shaderstate.curentity->flags&(RF_DEPTHHACK|RF_XFLIP);
	if (shaderstate.usingweaponviewmatrix != fl)
	{
		if ((shaderstate.usingweaponviewmatrix & RF_XFLIP) && shaderstate.usingweaponviewmatrix != -1)
			r_refdef.flipcull ^= SHADER_CULL_FLIP;
		shaderstate.usingweaponviewmatrix = fl;

		if (fl)
			memcpy(shaderstate.projectionmatrix, r_refdef.m_projection_view, sizeof(shaderstate.projectionmatrix));
		else
			memcpy(shaderstate.projectionmatrix, r_refdef.m_projection_std, sizeof(shaderstate.projectionmatrix));

		if (fl&RF_XFLIP)
		{
			shaderstate.projectionmatrix[0] *= -1;
			shaderstate.projectionmatrix[4] *= -1;
			shaderstate.projectionmatrix[8] *= -1;
			shaderstate.projectionmatrix[12] *= -1;
			r_refdef.flipcull ^= SHADER_CULL_FLIP;
		}

		if (qglLoadMatrixf)
		{
			qglMatrixMode(GL_PROJECTION);
			qglLoadMatrixf(shaderstate.projectionmatrix);
			qglMatrixMode(GL_MODELVIEW);
		}
	}

	shaderstate.lastuniform = 0;
}

#ifndef GLSLONLY
static void BE_SelectFog(vec3_t colour, float alpha, float density)
{
	float zscale;

	GL_DeSelectProgram();

	zscale = 2048;	/*this value is meant to be the distance at which fog the value becomes as good as fully fogged, just hack it to 2048...*/
	GenerateFogTexture(&shaderstate.fogtexture, density, zscale);
	shaderstate.fogfar = 1/zscale; /*scaler for z coords*/

	VectorCopy(colour, shaderstate.pendingcolourflat);
	shaderstate.pendingcolourflat[3] = alpha;
	shaderstate.pendingcolourvbo = 0;
	shaderstate.pendingcolourpointer = NULL;
}
#endif
#ifdef RTLIGHTS
static qboolean GLBE_RegisterLightShader(int mode)
{
	if (!shaderstate.inited_shader_light[mode])
	{
		char *name = va("rtlight%s%s%s%s%s", 
			(mode & LSHADER_SMAP)?"#PCF":"",
			(mode & LSHADER_SPOT)?"#SPOT":"",
			(mode & LSHADER_CUBE)?"#CUBE":"",
			(mode & LSHADER_ORTHO)?"#ORTHO":"",
			(gl_config.arb_shadow && (mode & (LSHADER_SMAP|LSHADER_SPOT|LSHADER_CUBE|LSHADER_ORTHO)))?"#USE_ARB_SHADOW":""
			);

		shaderstate.inited_shader_light[mode] = true;
		shaderstate.shader_light[mode] = R_RegisterCustom(NULL, name, SUF_NONE, Shader_LightPass, NULL);
	}

	if (shaderstate.shader_light[mode])
	{
		//make sure it has a program and forget it if it doesn't, to save a compare.
		if (!shaderstate.shader_light[mode]->prog)
		{
			shaderstate.shader_light[mode] = NULL;
			return false;
		}
		return true;
	}
	return false;
}
#endif

qboolean GLBE_SelectDLight(dlight_t *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode)
{
#ifdef RTLIGHTS
	extern cvar_t gl_specular;
#endif

	shaderstate.lastuniform = 0;
	shaderstate.curdlight = dl;
	shaderstate.lightmode =	lmode;

	/*simple info*/
	shaderstate.lightradius = dl->radius;
	VectorCopy(dl->origin, shaderstate.lightorg);
	VectorCopy(axis[0], shaderstate.lightdir);
	VectorCopy(colour, shaderstate.lightcolours);
#ifdef RTLIGHTS
	VectorCopy(dl->lightcolourscales, shaderstate.lightcolourscale);
	shaderstate.lightcolourscale[2] *= gl_specular.value;
	if (lmode & (LSHADER_SPOT|LSHADER_ORTHO))
		shaderstate.lightcubemap = r_nulltex;
	else
		shaderstate.lightcubemap = dl->cubetexture;

	if (TEXLOADED(shaderstate.lightcubemap) && GLBE_RegisterLightShader(shaderstate.lightmode | LSHADER_CUBE))
		shaderstate.lightmode |= LSHADER_CUBE;
	if (!GLBE_RegisterLightShader(shaderstate.lightmode))
		return false;

	/*generate light projection information*/
	if (shaderstate.lightmode & LSHADER_ORTHO)
	{
		float view[16];
		float proj[16];
		float xmin = -dl->radius;
		float ymin = -dl->radius;
		float znear = -dl->radius;
		float xmax = dl->radius;
		float ymax = dl->radius;
		float zfar = dl->radius;
		Matrix4x4_CM_Orthographic(proj, xmin, xmax, ymax, ymin, znear, zfar);
		Matrix4x4_CM_ModelViewMatrixFromAxis(view, axis[0], axis[2], axis[1], dl->origin);
		Matrix4_Multiply(proj, view, shaderstate.lightprojmatrix);
//		Matrix4x4_CM_LightMatrixFromAxis(shaderstate.lightprojmatrix, axis[0], axis[1], axis[2], dl->origin);
	}
	else if (shaderstate.lightmode & LSHADER_SPOT)
	{
		float view[16];
		float proj[16];
		extern cvar_t r_shadow_shadowmapping_nearclip;
		Matrix4x4_CM_Projection_Far(proj, dl->fov, dl->fov, dl->nearclip?dl->nearclip:r_shadow_shadowmapping_nearclip.value, dl->radius, false);
		Matrix4x4_CM_ModelViewMatrixFromAxis(view, axis[0], axis[1], axis[2], dl->origin);
		Matrix4_Multiply(proj, view, shaderstate.lightprojmatrix);
	}
	else if (shaderstate.lightmode & (LSHADER_SMAP|LSHADER_CUBE))
	{
		Matrix4x4_CM_LightMatrixFromAxis(shaderstate.lightprojmatrix, axis[0], axis[1], axis[2], dl->origin);
		/*
		vec3_t down;
		vec3_t back;
		vec3_t right;
		VectorScale(axis[2], -1, down);
		VectorScale(axis[1], 1, right);
		VectorScale(axis[0], 1, back);
		Matrix4x4_CM_ModelViewMatrixFromAxis(shaderstate.lightprojmatrix, down, back, right, dl->origin);
		*/
	}
#endif

	return true;
}

void GLBE_Scissor(srect_t *rect)
{
	if (rect)
	{
		qglScissor(
			floor(r_refdef.pxrect.x + rect->x*r_refdef.pxrect.width),
//			floor(r_refdef.pxrect.y + rect->y*r_refdef.pxrect.height),// - r_refdef.pxrect.maxheight),
			floor(rect->y*r_refdef.pxrect.height + (r_refdef.pxrect.maxheight-r_refdef.pxrect.y)-r_refdef.pxrect.height),
			ceil(rect->width * r_refdef.pxrect.width),
			ceil(rect->height * r_refdef.pxrect.height));
		qglEnable(GL_SCISSOR_TEST);

		if (qglDepthBoundsEXT)
		{
			qglDepthBoundsEXT(rect->dmin, rect->dmax);
			qglEnable(GL_DEPTH_BOUNDS_TEST_EXT);
		}
	}
	else
	{
/*		qglScissor(
			r_refdef.pxrect.x,
			r_refdef.pxrect.y - r_refdef.pxrect.height,
			r_refdef.pxrect.width,
			r_refdef.pxrect.height);
*/		qglDisable(GL_SCISSOR_TEST);
		if (qglDepthBoundsEXT)
			qglDisable(GL_DEPTH_BOUNDS_TEST_EXT);

//		if (qglDepthBoundsEXT)
//			qglDepthBoundsEXT(0, 1);
	}
}

#if defined(RTLIGHTS) && !defined(GLSLONLY)
texid_t GenerateNormalisationCubeMap(void);
static void BE_LegacyLighting(void)
{
	//bigfoot wants rtlight support without glsl, so here goes madness...
	//register combiners for bumpmapping using 4 tmus...
	float *col;
	float *ldir;
	vec3_t lightdir, rellight;
	float scale;
	int i, m;
	mesh_t *mesh;
	unsigned int attr = (1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR);
	int tmu = 0;

	BE_SendPassBlendDepthMask(SBITS_SRCBLEND_ONE | SBITS_DSTBLEND_ONE);

	//rotate this into modelspace
	Matrix4x4_CM_Transform3(shaderstate.modelmatrixinv, shaderstate.lightorg, rellight);

	for (m = 0; m < shaderstate.meshcount; m++)
	{
		mesh = shaderstate.meshes[m];

		//vbo-only mesh.
		if (!mesh->xyz_array)
			return;
		if (!mesh->normals_array)
			return;

		col = coloursarray[0] + mesh->vbofirstvert*4;
		ldir = texcoordarray[0] + mesh->vbofirstvert*3;
		for (i = 0; i < mesh->numvertexes; i++, col+=4, ldir+=3)
		{
			VectorSubtract(rellight, mesh->xyz_array[i], lightdir);
			scale = VectorNormalize(lightdir);
			scale = 1 - (scale/shaderstate.lightradius);
			VectorScale(shaderstate.lightcolours, scale, col);
			col[3] = 1;
			ldir[0] = -DotProduct(lightdir, mesh->snormals_array[i]);
			ldir[1] = DotProduct(lightdir, mesh->tnormals_array[i]);
			ldir[2] = DotProduct(lightdir, mesh->normals_array[i]);
		}
	}

	if (TEXLOADED(shaderstate.curtexnums->bump) && sh_config.havecubemaps && gl_config.arb_texture_env_dot3 && gl_config.arb_texture_env_combine && be_maxpasses >= 4)
	{	//we could get this down to 2 tmus by arranging for the dot3 result to be written the alpha buffer. But then we'd need to have an alpha buffer too.

		if (!shaderstate.normalisationcubemap)
			shaderstate.normalisationcubemap = GenerateNormalisationCubeMap();

		//tmu0: normalmap+replace+regular tex coords
		GL_LazyBind(tmu, shaderstate.curtexnums->bump);
		BE_SetPassBlendMode(tmu, PBM_REPLACE);
		shaderstate.pendingtexcoordparts[tmu] = 2;
		shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->texcoord.gl.vbo;
		shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->texcoord.gl.addr;
		attr |= (1u<<(VATTR_LEG_TMU0+tmu));
		tmu++;

		//tmu1: normalizationcubemap+dot3+lightdir
		GL_LazyBind(tmu, shaderstate.normalisationcubemap);
		BE_SetPassBlendMode(tmu, PBM_DOTPRODUCT);
		shaderstate.pendingtexcoordparts[tmu] = 3;
		shaderstate.pendingtexcoordvbo[tmu] = 0;
		shaderstate.pendingtexcoordpointer[tmu] = texcoordarray[0];
		attr |= (1u<<(VATTR_LEG_TMU0+tmu));
		tmu++;

		//tmu2: $diffuse+multiply+regular tex coords
		GL_LazyBind(tmu, shaderstate.curtexnums->base);	//texture not used, its just to make sure the code leaves it enabled.
		BE_SetPassBlendMode(tmu, PBM_MODULATE);
		shaderstate.pendingtexcoordparts[tmu] = 2;
		shaderstate.pendingtexcoordvbo[tmu] = shaderstate.sourcevbo->texcoord.gl.vbo;
		shaderstate.pendingtexcoordpointer[tmu] = shaderstate.sourcevbo->texcoord.gl.addr;
		attr |= (1u<<(VATTR_LEG_TMU0+tmu));
		tmu++;
	
		//tmu3: $any+multiply-by-colour+notc
		GL_LazyBind(tmu, shaderstate.curtexnums->base);	//texture not used, its just to make sure the code leaves it enabled.
		BE_SetPassBlendMode(tmu, PBM_MODULATE_PREV_COLOUR);
		shaderstate.pendingtexcoordparts[tmu] = 0;
		shaderstate.pendingtexcoordvbo[tmu] = 0;
		shaderstate.pendingtexcoordpointer[tmu] = NULL;
		tmu++;

		//note we need 4 combiners in the first because we can't use the colour argument in the first without breaking the normals.

		for (i = tmu; i < shaderstate.lastpasstmus; i++)
		{
			GL_LazyBind(i, r_nulltex);
		}
		shaderstate.lastpasstmus = tmu;
	}
	else
	{
		attr |= (1u<<(VATTR_LEG_TMU0));

		//tmu0: $diffuse+multiply+regular tex coords
		//multiplies by vertex colours
		GL_LazyBind(0, shaderstate.curtexnums->base);	//texture not used, its just to make sure the code leaves it enabled.
		BE_SetPassBlendMode(0, PBM_MODULATE);
		shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
		shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;

		for (i = 1; i < shaderstate.lastpasstmus; i++)
		{
			GL_LazyBind(i, r_nulltex);
		}
		shaderstate.lastpasstmus = 1;
	}

	shaderstate.colourarraytype = GL_FLOAT;
	shaderstate.pendingcolourvbo = 0;
	shaderstate.pendingcolourpointer = coloursarray;

	GL_DeSelectProgram();
	BE_EnableShaderAttributes(attr, 0);

	BE_SubmitMeshChain(false);

//	GL_LazyBind(1, 0, r_nulltex);
//	GL_LazyBind(2, 0, r_nulltex);
//	GL_LazyBind(3, 0, r_nulltex);
}
#endif

static void DrawMeshes(void)
{
	const shader_t *altshader;
	const shaderpass_t *p;
	int passno;
	int flags;
	passno = 0;

	if (shaderstate.force2d)
	{
		RQuantAdd(RQUANT_2DBATCHES, 1);
	}
	else if (shaderstate.curentity == &r_worldentity)
	{
		RQuantAdd(RQUANT_WORLDBATCHES, 1);
	}
	else
	{
		RQuantAdd(RQUANT_ENTBATCHES, 1);
	}

	flags = shaderstate.curshader->flags;
	GL_CullFace(flags & (SHADER_CULL_FRONT|SHADER_CULL_BACK));

	if (shaderstate.sourcevbo->coord2.gl.addr && (shaderstate.curshader->numdeforms || !shaderstate.curshader->prog))
		GenerateVertexBlends(shaderstate.curshader);
	else if (shaderstate.curshader->numdeforms)
		GenerateVertexDeforms(shaderstate.curshader);
	else
	{
		shaderstate.pendingvertexpointer = shaderstate.sourcevbo->coord.gl.addr;
		shaderstate.pendingvertexvbo = shaderstate.sourcevbo->coord.gl.vbo;
	}

#ifdef _DEBUG
	if (!shaderstate.pendingvertexpointer && !shaderstate.pendingvertexvbo)
	{
		Con_Printf(CON_ERROR "pendingvertexpointer+vbo are both null! shader is %s\n", shaderstate.curshader->name);
		return;
	}
#endif

#ifdef FTE_TARGET_WEB
	if (!shaderstate.pendingvertexvbo)
	{
		int len = 0, m;
		mesh_t *meshlist;
		for (m = 0; m < shaderstate.meshcount; m++)
		{
			meshlist = shaderstate.meshes[m];
			if (len < meshlist->vbofirstvert + meshlist->numvertexes)
				len = meshlist->vbofirstvert + meshlist->numvertexes;
		}
		len *= sizeof(vecV_t);

		shaderstate.streamid = (shaderstate.streamid + 1) & (sizeof(shaderstate.streamvbo)/sizeof(shaderstate.streamvbo[0]) - 1);
		GL_SelectVBO(shaderstate.pendingvertexvbo = shaderstate.streamvbo[shaderstate.streamid]);
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, len, shaderstate.pendingvertexpointer, GL_STREAM_DRAW_ARB);
		shaderstate.pendingvertexpointer = NULL;
	}
	if (!shaderstate.sourcevbo->indicies.gl.vbo)
		return;
#endif

	BE_PolyOffset();
	switch(shaderstate.mode)
	{
	case BEM_STENCIL:
		Host_Error("Shader system is not meant to accept stencil meshes\n");
		break;
#ifdef RTLIGHTS
	case BEM_LIGHT:
		altshader = shaderstate.curshader->bemoverrides[shaderstate.lightmode];
		if (!altshader)
			altshader = shaderstate.shader_light[shaderstate.lightmode];
		if (altshader && altshader->prog)
		{
			shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
			shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
			shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
			shaderstate.pendingtexcoordparts[0] = 2;
			shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
			BE_RenderMeshProgram(altshader, altshader->passes, altshader->prog);
		}
#ifndef GLSLONLY
		else
			BE_LegacyLighting();
#endif
		break;
	case BEM_GBUFFER:
		altshader = shaderstate.curshader->bemoverrides[bemoverride_gbuffer];
		if (altshader)
		{
			if (altshader->prog)
			{
				shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
				shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
				shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
				shaderstate.pendingtexcoordparts[0] = 2;
				shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
				shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
				BE_RenderMeshProgram(altshader, altshader->passes, altshader->prog);
			}
			else if (altshader->numpasses && altshader->passes[0].prog)
			{
				shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
				shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
				shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
				shaderstate.pendingtexcoordparts[0] = 2;
				shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
				shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
				BE_RenderMeshProgram(altshader, altshader->passes, altshader->passes[0].prog);
			}
		}

		break;
#endif
	case BEM_CREPUSCULAR:
		altshader = shaderstate.curshader->bemoverrides[bemoverride_crepuscular];
		if (!altshader && (shaderstate.curshader->flags & SHADER_SKY))
			altshader = shaderstate.crepskyshader;
		if (!altshader)
			altshader = shaderstate.crepopaqueshader;

		if (altshader && altshader->prog)
		{
			shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
			shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
			shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
			shaderstate.pendingtexcoordparts[0] = 2;
			shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
			BE_RenderMeshProgram(altshader, altshader->passes, altshader->prog);
		}
		break;
	case BEM_DEPTHONLY:
		altshader = shaderstate.curshader->bemoverrides[bemoverride_depthonly];
		if (!altshader)
			altshader = shaderstate.depthonlyshader;

		if (altshader && altshader->prog)
		{
			shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
			shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
			shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
			shaderstate.pendingtexcoordparts[0] = 2;
			shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
			BE_RenderMeshProgram(altshader, altshader->passes, altshader->prog);
		}
		else
		{
			GL_DeSelectProgram();
#ifdef warningmsg
#pragma warningmsg("fixme: support alpha test")
#endif
			BE_EnableShaderAttributes((1u<<VATTR_LEG_VERTEX), 0);
			BE_SubmitMeshChain(false);	//fixme: dangerous
		}
		break;

	case BEM_FOG:
#ifndef GLSLONLY
		GenerateTCFog(0, NULL);
		BE_EnableShaderAttributes((1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR) | (1u<<VATTR_LEG_TMU0), 0);
		BE_SubmitMeshChain(false);
#endif
		break;

	case BEM_WIREFRAME:
		if (shaderstate.wireframeshader && shaderstate.wireframeshader->prog)
		{
			shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
			shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
			shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;
			shaderstate.pendingtexcoordparts[0] = 2;
			shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;
			BE_RenderMeshProgram(shaderstate.wireframeshader, shaderstate.wireframeshader->passes, shaderstate.wireframeshader->prog);
		}
#ifndef GLSLONLY
		else if (!gl_config_nofixedfunc)
		{
			BE_SetPassBlendMode(0, PBM_REPLACE);
			GL_DeSelectProgram();

			shaderstate.pendingcolourvbo = 0;
			shaderstate.pendingcolourpointer = NULL;
			Vector4Set(shaderstate.pendingcolourflat, 1, 1, 1, 1);
			while(shaderstate.lastpasstmus>0)
			{
				GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
			}
			BE_SendPassBlendDepthMask((shaderstate.curshader->passes[0].shaderbits & ~SBITS_BLEND_BITS) | SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA | ((r_wireframe.ival == 1)?SBITS_MISC_NODEPTHTEST:0));

			BE_EnableShaderAttributes((1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR), 0);
			BE_SubmitMeshChain(false);
		}
#endif
		break;
	case BEM_DEPTHDARK:
		if ((shaderstate.curshader->flags & (SHADER_HASLIGHTMAP|SHADER_NODLIGHT))==SHADER_HASLIGHTMAP && !TEXVALID(shaderstate.curtexnums->fullbright))
		{
			if (gl_config.arb_shader_objects)
			{
				if (!shaderstate.allblackshader.glsl.handle)
				{
					const char *defs[] = {NULL};
					shaderstate.allblackshader = GLSlang_CreateProgram(NULL, "allblackprogram", sh_config.minver, defs, "#include \"sys/skeletal.h\"\nvoid main(){gl_Position = skeletaltransform();}", NULL, NULL, NULL, "void main(){gl_FragColor=vec4(0.0,0.0,0.0,1.0);}", false, NULL);
					shaderstate.allblack_mvp = qglGetUniformLocationARB(shaderstate.allblackshader.glsl.handle, "m_modelviewprojection");
				}

				GL_SelectProgram(shaderstate.allblackshader.glsl.handle);
				BE_SendPassBlendDepthMask(shaderstate.curshader->passes[0].shaderbits);
				BE_EnableShaderAttributes(gl_config_nofixedfunc?(1u<<VATTR_VERTEX1):(1u<<VATTR_LEG_VERTEX), 0);
				if (shaderstate.allblackshader.glsl.handle != shaderstate.lastuniform && shaderstate.allblack_mvp != -1)
				{
					float m16[16];
					Matrix4_Multiply(shaderstate.projectionmatrix, shaderstate.modelviewmatrix, m16);
					qglUniformMatrix4fvARB(shaderstate.allblack_mvp, 1, false, m16);
				}
				BE_SubmitMeshChain(shaderstate.allblackshader.glsl.usetesselation);

				shaderstate.lastuniform = shaderstate.allblackshader.glsl.handle;
				break;
			}
#ifndef GLSLONLY
			else if (!gl_config_nofixedfunc)
			{
				GL_DeSelectProgram();
				shaderstate.pendingcolourvbo = 0;
				shaderstate.pendingcolourpointer = NULL;
				Vector4Set(shaderstate.pendingcolourflat, 0, 0, 0, 1);
				while(shaderstate.lastpasstmus>0)
				{
					GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
				}

				BE_SetPassBlendMode(0, PBM_REPLACE);
				BE_SendPassBlendDepthMask(shaderstate.curshader->passes[0].shaderbits);

				BE_EnableShaderAttributes((1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR), 0);
				BE_SubmitMeshChain(false);
				break;
			}
#endif
		}
		//fallthrough
	case BEM_STANDARD:
	default:
		if (shaderstate.curshader->prog)
		{
			shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
			shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
			shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;

			shaderstate.pendingtexcoordparts[0] = 2;
			shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
			shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;

			BE_RenderMeshProgram(shaderstate.curshader, shaderstate.curshader->passes, shaderstate.curshader->prog);
		}
		else if (gl_config_nofixedfunc)
		{
#ifdef FTE_TARGET_WEB
			int maxverts = 0, m;
			mesh_t *meshlist;
			for (m = 0; m < shaderstate.meshcount; m++)
			{
				meshlist = shaderstate.meshes[m];
				if (maxverts < meshlist->vbofirstvert + meshlist->numvertexes)
					maxverts = meshlist->vbofirstvert + meshlist->numvertexes;
			}
#endif

			while (passno < shaderstate.curshader->numpasses)
			{
				int emumode;
				p = &shaderstate.curshader->passes[passno];
				passno += p->numMergedPasses;

				if (p->prog)
				{
					shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
					shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
					shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;

					shaderstate.pendingtexcoordparts[0] = 2;
					shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
					shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;

					BE_RenderMeshProgram(shaderstate.curshader, p, p->prog);
					continue;
				}

				emumode = 0;
				emumode = (p->shaderbits & SBITS_ATEST_BITS) >> SBITS_ATEST_SHIFT;

				GenerateColourMods(p);
				if (!shaderstate.colourarraytype)
				{
					emumode |= 4;
					shaderstate.lastuniform = 0;	//FIXME: s_colour uniform might be wrong.
				}
#ifdef FTE_TARGET_WEB
				else if (!shaderstate.pendingcolourvbo && shaderstate.pendingcolourpointer)
				{
					shaderstate.streamid = (shaderstate.streamid + 1) & (sizeof(shaderstate.streamvbo)/sizeof(shaderstate.streamvbo[0]) - 1);
					GL_SelectVBO(shaderstate.pendingcolourvbo = shaderstate.streamvbo[shaderstate.streamid]);
					switch(shaderstate.colourarraytype)
					{
					case GL_FLOAT:
						qglBufferDataARB(GL_ARRAY_BUFFER_ARB, maxverts * sizeof(vec4_t), shaderstate.pendingcolourpointer, GL_STREAM_DRAW_ARB);
						break;
					case GL_UNSIGNED_BYTE:
						qglBufferDataARB(GL_ARRAY_BUFFER_ARB, maxverts * sizeof(byte_vec4_t), shaderstate.pendingcolourpointer, GL_STREAM_DRAW_ARB);
						break;
					}
					shaderstate.pendingcolourpointer = NULL;
				}
#endif

				BE_GeneratePassTC(p, 0);
#ifdef FTE_TARGET_WEB
				if (!shaderstate.pendingtexcoordvbo[0] && shaderstate.pendingtexcoordpointer[0])
				{
					shaderstate.streamid = (shaderstate.streamid + 1) & (sizeof(shaderstate.streamvbo)/sizeof(shaderstate.streamvbo[0]) - 1);
					GL_SelectVBO(shaderstate.pendingtexcoordvbo[0] = shaderstate.streamvbo[shaderstate.streamid]);
					qglBufferDataARB(GL_ARRAY_BUFFER_ARB, maxverts * sizeof(float) * shaderstate.pendingtexcoordparts[0], shaderstate.pendingtexcoordpointer[0], GL_STREAM_DRAW_ARB);
					shaderstate.pendingtexcoordpointer[0] = NULL;
				}
#endif

				if (!shaderstate.programfixedemu[emumode])
				{
					char *modes[] = {
						"","#ALPHATEST=>0.0","#ALPHATEST=<0.5","#ALPHATEST=>=0.5",
						"#UC","#ALPHATEST=>0.0#UC","#ALPHATEST=<0.5#UC","#ALPHATEST=>=0.5#UC"
					};
					shaderstate.programfixedemu[emumode] = Shader_FindGeneric(va("fixedemu%s", modes[emumode]), QR_OPENGL);
					if (!shaderstate.programfixedemu[emumode])
						break;
				}

				BE_RenderMeshProgram(shaderstate.curshader, p, shaderstate.programfixedemu[emumode]);
			}
			break;
		}
#ifndef GLSLONLY
		else
		{
			while (passno < shaderstate.curshader->numpasses)
			{
				p = &shaderstate.curshader->passes[passno];
				passno += p->numMergedPasses;
		//		if (p->flags & SHADER_PASS_DETAIL)
		//			continue;

				if (p->prog)
				{
					shaderstate.pendingcolourvbo = shaderstate.sourcevbo->colours[0].gl.vbo;
					shaderstate.pendingcolourpointer = shaderstate.sourcevbo->colours[0].gl.addr;
					shaderstate.colourarraytype = shaderstate.sourcevbo->colours_bytes?GL_UNSIGNED_BYTE:GL_FLOAT;

					shaderstate.pendingtexcoordparts[0] = 2;
					shaderstate.pendingtexcoordvbo[0] = shaderstate.sourcevbo->texcoord.gl.vbo;
					shaderstate.pendingtexcoordpointer[0] = shaderstate.sourcevbo->texcoord.gl.addr;

					BE_RenderMeshProgram(shaderstate.curshader, p, p->prog);
				}
				else
				{
					GL_DeSelectProgram();
					DrawPass(p);
				}
			}
		}
		if (shaderstate.curbatch->fog && shaderstate.curbatch->fog->shader)
		{
			//FIXME: if glsl, do this fog volume crap properly!

			GL_DeSelectProgram();

			GenerateFogTexture(&shaderstate.fogtexture, shaderstate.curbatch->fog->shader->fog_dist, 2048);
			shaderstate.fogfar = 1.0f/2048; /*scaler for z coords*/

			while(shaderstate.lastpasstmus>1)
			{
				GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
			}
			GL_LazyBind(0, shaderstate.fogtexture);
			shaderstate.lastpasstmus = 1;

			Vector4Scale(shaderstate.curbatch->fog->shader->fog_color, (1/255.0), shaderstate.pendingcolourflat);
			shaderstate.pendingcolourvbo = 0;
			shaderstate.pendingcolourpointer = NULL;
			BE_SetPassBlendMode(0, PBM_MODULATE);
			BE_SendPassBlendDepthMask(SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA | (shaderstate.curshader->numpasses?SBITS_DEPTHFUNC_EQUAL:0));

			GenerateTCFog(0, shaderstate.curbatch->fog);
			BE_EnableShaderAttributes((1u<<VATTR_LEG_VERTEX) | (1u<<VATTR_LEG_COLOUR) | (1u<<VATTR_LEG_TMU0), 0);
			BE_SubmitMeshChain(false);
		}
		break;
#endif
	}
}

static void BE_GenTempMeshVBO(vbo_t **vbo, mesh_t *m)
{
	*vbo = &shaderstate.dummyvbo;

	//this code is shit shit shit.
	if (shaderstate.streamvbo[0])
	{
		//use a local. can't use a static, that crashes the compiler due to memory use, so lets eat the malloc or stack or whatever because we really don't have a choice.
		static char *buffer;
		size_t len = 0;
		if (!buffer)
			buffer = malloc(65536 * 33 * sizeof(float));

		//we're not doing vao... but just in case someone added that as an extension...
		GL_DeselectVAO();

		//cycle the vbo. this reduces the likelyhood that we'll have to wait for the vbo to no longer be in use.
		//remember, we can't do nice things like orphan in webgl. we have to create a new buffer every single time.
		//we can't stream with BufferSubData, because browsers are pure shite, and probably mmap and read back huge blocks of code as they translate the calls to direct3d nonsense.
		//we can't use client memory and let the driver do the right thing, because lets face it, our driver is technically a browser. its safer to run everything in a scripted language than to fight the nonsense.
		//how many buffers do we need? no idea. all this memory allocation is going to be shit for performance.
		//but we really do not have a choice. This is the only way to 'stream' without dropping down to <1fps.
		//although arguably we should build our entire hud+2d stuff into a single vbo each frame... meh. At least this keeps the memory use in the driver's 64bit memory space instead of the browser's 32bit one...
		shaderstate.streamid = (shaderstate.streamid + 1) & (sizeof(shaderstate.streamvbo)/sizeof(shaderstate.streamvbo[0]) - 1);
		shaderstate.dummyvbo.vao = shaderstate.streamvao[shaderstate.streamid];
		shaderstate.dummyvbo.vaodynamic = ~0;
		shaderstate.dummyvbo.vaoenabled = 0;
		if (shaderstate.dummyvbo.vao)
		{
			qglBindVertexArray(shaderstate.dummyvbo.vao);
			shaderstate.currentvao = shaderstate.dummyvbo.vao;
		}
		GL_SelectVBO(shaderstate.streamvbo[shaderstate.streamid]);
		GL_SelectEBO(shaderstate.streamebo[shaderstate.streamid]);

		memcpy(buffer+len, m->xyz_array, sizeof(*m->xyz_array) * m->numvertexes);
		shaderstate.dummyvbo.coord.gl.addr = (void*)len;
		shaderstate.dummyvbo.coord.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
		len += sizeof(*m->xyz_array) * m->numvertexes;

		if (m->xyz2_array)
		{
			memcpy(buffer+len, m->xyz2_array, sizeof(*m->xyz2_array) * m->numvertexes);
			shaderstate.dummyvbo.coord2.gl.addr = (void*)len;
			shaderstate.dummyvbo.coord2.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->xyz2_array) * m->numvertexes;
		}
		else
		{
			shaderstate.dummyvbo.coord2.gl.addr = NULL;
			shaderstate.dummyvbo.coord2.gl.vbo = 0;
		}

		memcpy(buffer+len, m->st_array, sizeof(*m->st_array) * m->numvertexes);
		shaderstate.dummyvbo.texcoord.gl.addr = (void*)len;
		shaderstate.dummyvbo.texcoord.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
		len += sizeof(*m->st_array) * m->numvertexes;

		//FIXME: lightmaps

		if (m->colors4f_array[0])
		{
			memcpy(buffer+len, m->colors4f_array[0], sizeof(*m->colors4f_array[0]) * m->numvertexes);
			shaderstate.dummyvbo.colours[0].gl.addr = (void*)len;
			shaderstate.dummyvbo.colours[0].gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->colors4f_array[0]) * m->numvertexes;
			shaderstate.dummyvbo.colours_bytes = false;
		}
		else if (m->colors4b_array)
		{
			memcpy(buffer+len, m->colors4b_array, sizeof(*m->colors4b_array) * m->numvertexes);
			shaderstate.dummyvbo.colours[0].gl.addr = (void*)len;
			shaderstate.dummyvbo.colours[0].gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->colors4b_array) * m->numvertexes;
			shaderstate.dummyvbo.colours_bytes = true;
		}
		else
		{
			shaderstate.dummyvbo.colours[0].gl.addr = NULL;
			shaderstate.dummyvbo.colours[0].gl.vbo = 0;
			shaderstate.dummyvbo.colours_bytes = false;
		}

		if (m->normals_array)
		{
			memcpy(buffer+len, m->normals_array, sizeof(*m->normals_array) * m->numvertexes);
			shaderstate.dummyvbo.normals.gl.addr = (void*)len;
			shaderstate.dummyvbo.normals.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->normals_array) * m->numvertexes;
		}
		else
		{
			shaderstate.dummyvbo.normals.gl.addr = NULL;
			shaderstate.dummyvbo.normals.gl.vbo = 0;
		}

		if (m->snormals_array)
		{
			memcpy(buffer+len, m->snormals_array, sizeof(*m->snormals_array) * m->numvertexes);
			shaderstate.dummyvbo.svector.gl.addr = (void*)len;
			shaderstate.dummyvbo.svector.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->snormals_array) * m->numvertexes;
		}
		else
		{
			shaderstate.dummyvbo.svector.gl.addr = NULL;
			shaderstate.dummyvbo.svector.gl.vbo = 0;
		}

		if (m->tnormals_array)
		{
			memcpy(buffer+len, m->tnormals_array, sizeof(*m->tnormals_array) * m->numvertexes);
			shaderstate.dummyvbo.tvector.gl.addr = (void*)len;
			shaderstate.dummyvbo.tvector.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->tnormals_array) * m->numvertexes;
		}
		else
		{
			shaderstate.dummyvbo.tvector.gl.addr = NULL;
			shaderstate.dummyvbo.tvector.gl.vbo = 0;
		}

		if (m->bonenums)
		{
			memcpy(buffer+len, m->bonenums, sizeof(*m->bonenums) * m->numvertexes);
			shaderstate.dummyvbo.bonenums.gl.addr = (void*)len;
			shaderstate.dummyvbo.bonenums.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->bonenums) * m->numvertexes;
		}
		else
		{
			shaderstate.dummyvbo.bonenums.gl.addr = NULL;
			shaderstate.dummyvbo.bonenums.gl.vbo = 0;
		}

		if (m->boneweights)
		{
			memcpy(buffer+len, m->boneweights, sizeof(*m->boneweights) * m->numvertexes);
			shaderstate.dummyvbo.boneweights.gl.addr = (void*)len;
			shaderstate.dummyvbo.boneweights.gl.vbo = shaderstate.streamvbo[shaderstate.streamid];
			len += sizeof(*m->boneweights) * m->numvertexes;
		}
		else
		{
			shaderstate.dummyvbo.boneweights.gl.addr = NULL;
			shaderstate.dummyvbo.boneweights.gl.vbo = 0;
		}

		//FIXME: normals, svector, tvector, bone nums, bone weights

		//now we've got a single buffer in a single place, update the buffer
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, len, buffer, GL_STREAM_DRAW_ARB);



		//and finally the elements array, which is a much simpler affair
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(*m->indexes) * m->numindexes, m->indexes, GL_STREAM_DRAW_ARB);
		shaderstate.dummyvbo.indicies.gl.addr = (void*)NULL;
		shaderstate.dummyvbo.indicies.gl.vbo = shaderstate.streamebo[shaderstate.streamid];
	}
	else
	{
		//client memory. may be slower. will probably be faster.
		shaderstate.dummyvbo.coord.gl.addr = m->xyz_array;
		shaderstate.dummyvbo.coord2.gl.addr = m->xyz2_array;
		shaderstate.dummyvbo.texcoord.gl.addr = m->st_array;
		shaderstate.dummyvbo.indicies.gl.addr = m->indexes;
		shaderstate.dummyvbo.normals.gl.addr = m->normals_array;
		shaderstate.dummyvbo.svector.gl.addr = m->snormals_array;
		shaderstate.dummyvbo.tvector.gl.addr = m->tnormals_array;
		if (m->colors4f_array[0])
		{
			shaderstate.dummyvbo.colours_bytes = false;
			shaderstate.dummyvbo.colours[0].gl.addr = m->colors4f_array[0];
		}
		else
		{
			shaderstate.dummyvbo.colours_bytes = true;
			shaderstate.dummyvbo.colours[0].gl.addr = m->colors4b_array;
		}
		shaderstate.dummyvbo.bonenums.gl.addr = m->bonenums;
		shaderstate.dummyvbo.boneweights.gl.addr = m->boneweights;
	}
	shaderstate.dummyvbo.bones = m->bones;
	shaderstate.dummyvbo.numbones = m->numbones;
}

void GLBE_DrawMesh_List(shader_t *shader, int nummeshes, mesh_t **meshlist, vbo_t *vbo, texnums_t *texnums, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.curshader = shader->remapto;

	if (!vbo)
	{
		mesh_t *m;
		shaderstate.flags = beflags;
		TRACE(("GLBE_DrawMesh_List: shader %s\n", shader->name));
		if (shaderstate.curentity != &r_worldentity)
			GLBE_SelectEntity(&r_worldentity);
		shaderstate.curtime = shaderstate.updatetime - (shaderstate.curentity->shaderTime + shader->remaptime);
		if (texnums)
			shaderstate.curtexnums = texnums;
		else if (shader->numdefaulttextures)
			shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
		else
			shaderstate.curtexnums = shader->defaulttextures;

		while (nummeshes--)
		{
			m = *meshlist++;

			BE_GenTempMeshVBO(&shaderstate.sourcevbo, m);

			shaderstate.meshcount = 1;
			shaderstate.meshes = &m;
			DrawMeshes();
		}
	}
	else
	{
		shaderstate.sourcevbo = vbo;
		shaderstate.flags = beflags;
		if (shaderstate.curentity != &r_worldentity)
			GLBE_SelectEntity(&r_worldentity);
		shaderstate.curtime = shaderstate.updatetime - (shaderstate.curentity->shaderTime + shader->remaptime);
		if (texnums)
			shaderstate.curtexnums = texnums;
		else if (shader->numdefaulttextures)
			shaderstate.curtexnums = shader->defaulttextures + ((int)(shader->defaulttextures_fps * shaderstate.curtime) % shader->numdefaulttextures);
		else
			shaderstate.curtexnums = shader->defaulttextures;

		shaderstate.meshcount = nummeshes;
		shaderstate.meshes = meshlist;
		DrawMeshes();
	}
}
void GLBE_DrawMesh_Single(shader_t *shader, mesh_t *mesh, vbo_t *vbo, unsigned int beflags)
{
	shader->next = NULL;
	GLBE_DrawMesh_List(shader, 1, &mesh, NULL, NULL, beflags);
}

void GLBE_SubmitBatch(batch_t *batch)
{
	shader_t *sh;
	shaderstate.curbatch = batch;
	if (batch->vbo)
	{
		shaderstate.sourcevbo = batch->vbo;

		if (!batch->vbo->vao)
			batch->vbo->vao = shaderstate.streamvao[0];
		batch->vbo->vaodynamic = ~0;
		batch->vbo->vaoenabled = 0;
	}
	else
	{
		//we're only allowed one mesh per batch if there's no vbo info.
		BE_GenTempMeshVBO(&shaderstate.sourcevbo, batch->mesh[0]);
	}

	sh = batch->shader;
	shaderstate.curshader = sh->remapto;
	shaderstate.flags = batch->flags;
	if (shaderstate.curentity != batch->ent)
		GLBE_SelectEntity(batch->ent);
	shaderstate.curtime = shaderstate.updatetime - (shaderstate.curentity->shaderTime + sh->remaptime);
	if (batch->skin)
		shaderstate.curtexnums = batch->skin;
	else if (sh->numdefaulttextures)
		shaderstate.curtexnums = sh->defaulttextures + ((int)(sh->defaulttextures_fps * shaderstate.curtime) % sh->numdefaulttextures);
	else
		shaderstate.curtexnums = sh->defaulttextures;

	if (0)
	{
		int i;
		for (i = batch->firstmesh; i < batch->meshes; i++)
		{
			shaderstate.meshcount = 1;
			shaderstate.meshes = &batch->mesh[i];
			DrawMeshes();
		}
	}
	else
	{
		shaderstate.meshcount = batch->meshes - batch->firstmesh;
		shaderstate.meshes = batch->mesh+batch->firstmesh;
		DrawMeshes();
	}
}

static void GLBE_SubmitMeshesPortals(batch_t **worldlist, batch_t *dynamiclist)
{
	batch_t *batch, *masklists[2];
	int i;
	float il;

	if (!dynamiclist && !worldlist[SHADER_SORT_PORTAL])
		return;	//no portals to draw

	/*attempt to draw portal shaders*/
	if (shaderstate.mode == BEM_STANDARD)
	{
		for (i = 0; i < 2; i++)
		{
			for (batch = i?dynamiclist:worldlist[SHADER_SORT_PORTAL]; batch; batch = batch->next)
			{
				if (batch->meshes == batch->firstmesh)
					continue;

				if (batch->buildmeshes)
					batch->buildmeshes(batch);

				il = shaderstate.identitylighting;
				masklists[0] = worldlist[SHADER_SORT_PORTAL];
				masklists[1] = dynamiclist;
				GLR_DrawPortal(batch, worldlist, masklists, 0);
				shaderstate.identitylighting = il;

				/*clear depth again*/
				GL_ForceDepthWritable();
				qglClear(GL_DEPTH_BUFFER_BIT);
			}
		}
		//make sure the current scene doesn't draw over the portal where its not meant to. clamp depth so the near clip plane doesn't cause problems.
		if (gl_config.arb_depth_clamp)
			qglEnable(GL_DEPTH_CLAMP_ARB);
		/*draw depth only, to mask it off*/
		GLBE_SelectMode(BEM_DEPTHONLY);
		for (i = 0; i < 2; i++)
		{
			for (batch = i?dynamiclist:worldlist[SHADER_SORT_PORTAL]; batch; batch = batch->next)
			{
//				if (batch->meshes == batch->firstmesh)
//					continue;

				GLBE_SubmitBatch(batch);
			}
		}
		GLBE_SelectMode(BEM_STANDARD);
		if (gl_config.arb_depth_clamp)
			qglDisable(GL_DEPTH_CLAMP_ARB);
	}
}

static qboolean GLBE_GenerateBatchTextures(batch_t *batch, shader_t *bs)
{
	int oldfbo;
	float oldil;
	int oldbem;
	if (r_refdef.recurse >= r_portalrecursion.ival || r_refdef.recurse == R_MAX_RECURSE)
		return false;
	//these flags require rendering some view as an fbo
	//(BEM_DEPTHDARK is used when lightmap scale is 0, but still shows any emissive stuff)
	if (shaderstate.mode != BEM_STANDARD && shaderstate.mode != BEM_DEPTHDARK)
		return false;
	oldbem = shaderstate.mode;
	oldil = shaderstate.identitylighting;

	if ((bs->flags & SHADER_HASREFLECT) && gl_config.ext_framebuffer_objects)
	{
		float renderscale = bs->portalfboscale;
		vrect_t orect = r_refdef.vrect;
		pxrect_t oprect = r_refdef.pxrect;
		if (!shaderstate.tex_reflection[r_refdef.recurse])
		{
			shaderstate.tex_reflection[r_refdef.recurse] = Image_CreateTexture("***tex_reflection***", NULL, 0);
			if (!shaderstate.tex_reflection[r_refdef.recurse]->num)
				qglGenTextures(1, &shaderstate.tex_reflection[r_refdef.recurse]->num);
		}

		r_refdef.vrect.x = 0;
		r_refdef.vrect.y = 0;
		r_refdef.vrect.width = max(1, vid.fbvwidth * renderscale);
		r_refdef.vrect.height = max(1, vid.fbvheight * renderscale);
		r_refdef.pxrect.x = 0;
		r_refdef.pxrect.y = 0;
		r_refdef.pxrect.width = max(1, vid.fbpwidth * renderscale);
		r_refdef.pxrect.height = max(1, vid.fbpheight * renderscale);
		if (shaderstate.tex_reflection[r_refdef.recurse]->width!=r_refdef.pxrect.width || shaderstate.tex_reflection[r_refdef.recurse]->height!=r_refdef.pxrect.height)
		{
			shaderstate.tex_reflection[r_refdef.recurse]->width = r_refdef.pxrect.width;
			shaderstate.tex_reflection[r_refdef.recurse]->height = r_refdef.pxrect.height;
			GL_MTBind(0, GL_TEXTURE_2D, shaderstate.tex_reflection[r_refdef.recurse]);

			if ((vid.flags&VID_FP16) && sh_config.texfmt[PTI_RGBA16F])
				qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, shaderstate.tex_reflection[r_refdef.recurse]->width, shaderstate.tex_reflection[r_refdef.recurse]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			else if ((vid.flags&(VID_SRGBAWARE|VID_FP16)) && sh_config.texfmt[PTI_RGBA8_SRGB])
				qglTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8_EXT, shaderstate.tex_reflection[r_refdef.recurse]->width, shaderstate.tex_reflection[r_refdef.recurse]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			else
				qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shaderstate.tex_reflection[r_refdef.recurse]->width, shaderstate.tex_reflection[r_refdef.recurse]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		oldfbo = GLBE_FBO_Update(&shaderstate.fbo_reflectrefrac[r_refdef.recurse], FBO_RB_DEPTH, &shaderstate.tex_reflection[r_refdef.recurse], 1, r_nulltex, shaderstate.tex_reflection[r_refdef.recurse]->width, shaderstate.tex_reflection[r_refdef.recurse]->height, 0);
		r_refdef.pxrect.maxheight = shaderstate.fbo_reflectrefrac[r_refdef.recurse].rb_size[1];
		GL_ViewportUpdate();
		GL_ForceDepthWritable();
		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLR_DrawPortal(batch, cl.worldmodel->batches, NULL, 1);
		GLBE_FBO_Pop(oldfbo);
		r_refdef.vrect = orect;
		r_refdef.pxrect = oprect;
		GL_ViewportUpdate();
	}
	if (bs->flags & (SHADER_HASREFRACT|SHADER_HASREFRACTDEPTH))
	{
		if (r_refract_fboival || (bs->flags&SHADER_HASPORTAL))
		{
			float renderscale = min(1, bs->portalfboscale);
			vrect_t ovrect = r_refdef.vrect;
			pxrect_t oprect = r_refdef.pxrect;
			r_refdef.vrect.x = 0;
			r_refdef.vrect.y = 0;
			r_refdef.vrect.width = max(1, vid.fbvwidth * renderscale);
			r_refdef.vrect.height = max(1, vid.fbvheight * renderscale);
			r_refdef.pxrect.x = 0;
			r_refdef.pxrect.y = 0;
			r_refdef.pxrect.width = max(1, vid.fbpwidth * renderscale);
			r_refdef.pxrect.height = max(1, vid.fbpheight * renderscale);

			if (!shaderstate.tex_refraction[r_refdef.recurse])
			{
				shaderstate.tex_refraction[r_refdef.recurse] = Image_CreateTexture("***tex_refraction***", NULL, 0);
				if (!shaderstate.tex_refraction[r_refdef.recurse]->num)
					qglGenTextures(1, &shaderstate.tex_refraction[r_refdef.recurse]->num);
			}
			if (shaderstate.tex_refraction[r_refdef.recurse]->width != r_refdef.pxrect.width || shaderstate.tex_refraction[r_refdef.recurse]->height != r_refdef.pxrect.height)
			{
				shaderstate.tex_refraction[r_refdef.recurse]->width = r_refdef.pxrect.width;
				shaderstate.tex_refraction[r_refdef.recurse]->height = r_refdef.pxrect.height;
				GL_MTBind(0, GL_TEXTURE_2D, shaderstate.tex_refraction[r_refdef.recurse]);
				if ((vid.flags&VID_FP16) && sh_config.texfmt[PTI_RGBA16F])
					qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, r_refdef.pxrect.width, r_refdef.pxrect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				else if ((vid.flags&(VID_SRGBAWARE|VID_FP16)) && sh_config.texfmt[PTI_RGBA16F])
					qglTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8_EXT, r_refdef.pxrect.width, r_refdef.pxrect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				else
					qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, r_refdef.pxrect.width, r_refdef.pxrect.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			if (bs->flags & SHADER_HASREFRACTDEPTH)
			{
				if (!shaderstate.tex_refractiondepth[r_refdef.recurse])
				{
					shaderstate.tex_refractiondepth[r_refdef.recurse] = Image_CreateTexture("***tex_refractiondepth***", NULL, 0);
					if (!shaderstate.tex_refractiondepth[r_refdef.recurse]->num)
						qglGenTextures(1, &shaderstate.tex_refractiondepth[r_refdef.recurse]->num);
				}
				if (shaderstate.tex_refractiondepth[r_refdef.recurse]->width != r_refdef.pxrect.width || shaderstate.tex_refractiondepth[r_refdef.recurse]->height != r_refdef.pxrect.height)
				{
					shaderstate.tex_refractiondepth[r_refdef.recurse]->width = r_refdef.pxrect.width;
					shaderstate.tex_refractiondepth[r_refdef.recurse]->height = r_refdef.pxrect.height;
					GL_MTBind(0, GL_TEXTURE_2D, shaderstate.tex_refractiondepth[r_refdef.recurse]);
					qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, r_refdef.pxrect.width, r_refdef.pxrect.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				}
				oldfbo = GLBE_FBO_Update(&shaderstate.fbo_reflectrefrac[r_refdef.recurse], FBO_TEX_DEPTH, &shaderstate.tex_refraction[r_refdef.recurse], 1, shaderstate.tex_refractiondepth[r_refdef.recurse], r_refdef.pxrect.width, r_refdef.pxrect.height, 0);
			}
			else
			{
				oldfbo = GLBE_FBO_Update(&shaderstate.fbo_reflectrefrac[r_refdef.recurse], FBO_RB_DEPTH, &shaderstate.tex_refraction[r_refdef.recurse], 1, r_nulltex, r_refdef.pxrect.width, r_refdef.pxrect.height, 0);
			}
			r_refdef.pxrect.maxheight = shaderstate.fbo_reflectrefrac[r_refdef.recurse].rb_size[1];
			GL_ViewportUpdate();

			GL_ForceDepthWritable();
			qglClearColor(0, 0, 0, 1);
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (bs->flags&SHADER_HASPORTAL)
				GLR_DrawPortal(batch, cl.worldmodel->batches, NULL, 0);
			else
				GLR_DrawPortal(batch, cl.worldmodel->batches, NULL, ((bs->flags & SHADER_HASREFRACTDEPTH)?3:2));	//fixme
			GLBE_FBO_Pop(oldfbo);

			r_refdef.vrect = ovrect;
			r_refdef.pxrect = oprect;
			GL_ViewportUpdate();
		}
		else
			GLR_DrawPortal(batch, cl.worldmodel->batches, NULL, 3);
	}
	if ((bs->flags & SHADER_HASRIPPLEMAP) && gl_config.ext_framebuffer_objects)
	{
		float renderscale = bs->portalfboscale;
		vrect_t orect = r_refdef.vrect;
		pxrect_t oprect = r_refdef.pxrect;
		r_refdef.vrect.x = 0;
		r_refdef.vrect.y = 0;
		r_refdef.vrect.width = max(1, vid.fbvwidth * renderscale);
		r_refdef.vrect.height = max(1, vid.fbvheight * renderscale);
		r_refdef.pxrect.x = 0;
		r_refdef.pxrect.y = 0;
		r_refdef.pxrect.width = max(1, vid.fbpwidth * renderscale);
		r_refdef.pxrect.height = max(1, vid.fbpheight * renderscale);

		if (!shaderstate.tex_ripplemap[r_refdef.recurse])
		{
			//FIXME: can we use RGB8 instead?
			shaderstate.tex_ripplemap[r_refdef.recurse] = Image_CreateTexture("***tex_ripplemap***", NULL, 0);
			if (!shaderstate.tex_ripplemap[r_refdef.recurse]->num)
				qglGenTextures(1, &shaderstate.tex_ripplemap[r_refdef.recurse]->num);
		}
		if (shaderstate.tex_ripplemap[r_refdef.recurse]->width != r_refdef.pxrect.width || shaderstate.tex_ripplemap[r_refdef.recurse]->height != r_refdef.pxrect.height)
		{
			shaderstate.tex_ripplemap[r_refdef.recurse]->width = r_refdef.pxrect.width;
			shaderstate.tex_ripplemap[r_refdef.recurse]->height = r_refdef.pxrect.height;
			GL_MTBind(0, GL_TEXTURE_2D, shaderstate.tex_ripplemap[r_refdef.recurse]);
			qglTexImage2D(GL_TEXTURE_2D, 0, /*(gl_config.glversion>3.1)?GL_RGBA8_SNORM:*/GL_RGBA16F, r_refdef.pxrect.width, r_refdef.pxrect.height, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		oldfbo = GLBE_FBO_Update(&shaderstate.fbo_reflectrefrac[r_refdef.recurse], 0, &shaderstate.tex_ripplemap[r_refdef.recurse], 1, r_nulltex, r_refdef.pxrect.width, r_refdef.pxrect.height, 0);
		r_refdef.pxrect.maxheight = shaderstate.fbo_reflectrefrac[r_refdef.recurse].rb_size[1];
		GL_ViewportUpdate();

		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT);

//		r_refdef.waterheight = DotProduct(batch->mesh[0]->xyz_array[0], batch->mesh[0]->normals_array[0]);

		r_refdef.recurse+=1; //paranoid, should stop potential infinite loops
		GLBE_SubmitMeshes(cl.worldmodel->batches, SHADER_SORT_RIPPLE, SHADER_SORT_RIPPLE);
		r_refdef.recurse-=1;
		GLBE_FBO_Pop(oldfbo);

		r_refdef.vrect = orect;
		r_refdef.pxrect = oprect;
		GL_ViewportUpdate();
	}
	BE_SelectMode(oldbem);
	shaderstate.identitylighting = oldil;
	return true;
}
static void GLBE_SubmitMeshesSortList(batch_t *sortlist)
{
	batch_t *batch;
	shader_t *bs;
	for (batch = sortlist; batch; batch = batch->next)
	{
		if (batch->meshes == batch->firstmesh)
			continue;

		if (batch->flags & BEF_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT)
				continue;
		if (batch->flags & BEF_NOSHADOWS)
			if (shaderstate.mode == BEM_STENCIL || shaderstate.mode == BEM_DEPTHONLY)	//fixme: depthonly is not just shadows.
				continue;

		//buildmeshes updates shaders and generates pose information for sufaces that need it.
		//the shader flags checked *after* this call may be a performance issue if it generated lots of new mesh data.
		//FIXME: should we assume that the batch's shader will have the same flags?
		if (batch->buildmeshes)
		{
			TRACE(("GLBE_SubmitMeshesSortList: build\n"));
			batch->buildmeshes(batch);
		}

		bs = batch->shader;

		TRACE(("GLBE_SubmitMeshesSortList: shader %s\n", bs->name));

		//FIXME:!!
		if (!bs)
		{
			Con_Printf("Shader not set...\n");
			if (batch->texture)
				bs = R_TextureAnimation(0, batch->texture)->shader;
			else
				continue;
		}

		if ((bs->flags & SHADER_NODRAW) || !batch->meshes)
			continue;
		if (bs->flags & SHADER_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT)
				continue;
		if (bs->flags & SHADER_NOSHADOWS)
			if (shaderstate.mode == BEM_STENCIL || shaderstate.mode == BEM_DEPTHONLY)	//fixme: depthonly is not just shadows.
				continue;
		if (bs->flags & SHADER_SKY)
		{
			if (shaderstate.mode == BEM_STANDARD || shaderstate.mode == BEM_DEPTHDARK)// || shaderstate.mode == BEM_WIREFRAME)
			{
				float il = shaderstate.identitylighting;	//this stuff sucks!
				if (R_DrawSkyChain(batch))
				{
					shaderstate.identitylighting = il;
					continue;
				}
				shaderstate.identitylighting = il;
			}
			else if (/*shaderstate.mode != BEM_FOG &&*/ shaderstate.mode != BEM_CREPUSCULAR && shaderstate.mode != BEM_WIREFRAME)
				continue;
		}

		if ((bs->flags & (SHADER_HASREFLECT | SHADER_HASREFRACT | SHADER_HASRIPPLEMAP)) && shaderstate.mode != BEM_WIREFRAME)
		{
			if (!GLBE_GenerateBatchTextures(batch, bs))
				continue;
			if ((bs->flags&SHADER_HASPORTAL) && shaderstate.mode != BEM_DEPTHONLY && gl_config.arb_depth_clamp)
			{	//this little bit of code is meant to prevent issues when the near clip plane intersects the portal surface, allowing us to be that little bit closer to the portal.
				qglEnable(GL_DEPTH_CLAMP_ARB);
				GLBE_SubmitBatch(batch);
				qglDisable(GL_DEPTH_CLAMP_ARB);
				continue;
			}
		}

		GLBE_SubmitBatch(batch);
	}
}

void GLBE_SubmitMeshes (batch_t **worldbatches, int start, int stop)
{
	int i;
	int portaldepth = r_portalrecursion.ival;

	for (i = start; i <= stop; i++)
	{
		if (worldbatches)
		{
			if (i == SHADER_SORT_PORTAL && r_refdef.recurse < portaldepth)
			{
				GLBE_SubmitMeshesPortals(worldbatches, shaderstate.mbatches[i]);

				if (!r_refdef.recurse && r_portalonly.ival)
					return;
			}

			GLBE_SubmitMeshesSortList(worldbatches[i]);
		}
		GLBE_SubmitMeshesSortList(shaderstate.mbatches[i]);
	}
}

#if (defined(GLQUAKE) || defined(VKQUAKE)) && defined(MULTITHREAD)
#define THREADEDWORLD
#endif

extern double r_loaderstalltime;
void GLBE_UpdateLightmaps(void)
{
	double starttime;
	lightmapinfo_t *lm;
	int lmidx;

#ifdef THREADEDWORLD
	extern int webo_blocklightmapupdates;
	if (webo_blocklightmapupdates == 3)
		return;	//we've not had a new scene to render yet. don't bother uploading while something's still painting, its going to be redundant.
	webo_blocklightmapupdates |= 2;	//FIXME: round-robin it? one lightmap per frame?
#endif

	starttime = Sys_DoubleTime();
	for (lmidx = 0; lmidx < numlightmaps; lmidx++)
	{
		lm = lightmap[lmidx];
		if (!lm)
			continue;
		if (lm->modified)
		{
			int t = lm->rectchange.t;	//pull them out now, in the hopes that it'll be more robust with respect to r_dynamic -1
			int b = lm->rectchange.b;
#ifdef _DEBUG
			if (t >= b)
				Con_Printf("Dodgy lightmaps\n");
			else
#endif
			if (!TEXVALID(lm->lightmap_texture))
			{
				extern cvar_t r_lightmap_nearest;
				TEXASSIGN(lm->lightmap_texture, Image_CreateTexture(va("***lightmap %i***", lmidx), NULL, (r_lightmap_nearest.ival?IF_NEAREST:IF_LINEAR)|IF_NOMIPMAP));
				qglGenTextures(1, &lm->lightmap_texture->num);
				GL_MTBind(0, GL_TEXTURE_2D, lm->lightmap_texture);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				if (qglTexStorage2D && gl_config.formatinfo[lm->fmt].sizedformat)
				{
					qglTexStorage2D(GL_TEXTURE_2D, 1, gl_config.formatinfo[lm->fmt].sizedformat, lm->width, lm->height);
					if (lm->pbo_handle)
					{
						qglBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, lm->pbo_handle);
						qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, t, lm->width, b-t, gl_config.formatinfo[lm->fmt].format, gl_config.formatinfo[lm->fmt].type, (char*)NULL + t*lm->width*lm->pixbytes);
						qglBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
					}
					else
						qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, t, lm->width, b-t, gl_config.formatinfo[lm->fmt].format, gl_config.formatinfo[lm->fmt].type, lm->lightmaps+t*lm->width*lm->pixbytes);
				}
				else
					qglTexImage2D(GL_TEXTURE_2D, 0, gl_config.formatinfo[lm->fmt].internalformat,	lm->width, lm->height, 0, gl_config.formatinfo[lm->fmt].format, gl_config.formatinfo[lm->fmt].type,	lm->lightmaps);

#ifndef FTE_TARGET_WEB
				if (gl_config.glversion >= (gl_config.gles?3.0:3.3))
				{
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, gl_config.formatinfo[lm->fmt].swizzle_r);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, gl_config.formatinfo[lm->fmt].swizzle_g);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, gl_config.formatinfo[lm->fmt].swizzle_b);
					qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, gl_config.formatinfo[lm->fmt].swizzle_a);
				}
#endif

				//for completeness.
				lm->lightmap_texture->format = lm->fmt;
				lm->lightmap_texture->width = lm->width;
				lm->lightmap_texture->height = lm->height;
				lm->lightmap_texture->depth = 1;
				lm->lightmap_texture->status = TEX_LOADED;
			}
			else
			{
				GL_MTBind(0, GL_TEXTURE_2D, lm->lightmap_texture);
				if (lm->pbo_handle)
				{
					qglBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, lm->pbo_handle);
					qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, t, lm->width, b-t, gl_config.formatinfo[lm->fmt].format, gl_config.formatinfo[lm->fmt].type, (char*)NULL + t*lm->width*lm->pixbytes);
					qglBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
				}
				else
					qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, t, lm->width, b-t, gl_config.formatinfo[lm->fmt].format, gl_config.formatinfo[lm->fmt].type, lm->lightmaps+t*lm->width*lm->pixbytes);
			}
			lm->modified = false;
			lm->rectchange.l = lm->width;
			lm->rectchange.t = lm->height;
			lm->rectchange.r = 0;
			lm->rectchange.b = 0;
		}
	}
	r_loaderstalltime += Sys_DoubleTime()-starttime;
}

batch_t *GLBE_GetTempBatch(void)
{
	batch_t *b;
	if (shaderstate.wbatch >= shaderstate.maxwbatches)
	{
		shaderstate.wbatch++;
		return NULL;
	}
	b = &shaderstate.wbatches[shaderstate.wbatch++];
	b->fog = NULL;
	return b;
}

/*called from shadowmapping code*/
#ifdef RTLIGHTS
void GLBE_BaseEntTextures(const qbyte *worldpvs, const int *worldareas)
{
	batch_t *batches[SHADER_SORT_COUNT];
	batch_t **ob = shaderstate.mbatches;
	shaderstate.mbatches = batches;
	BE_GenModelBatches(batches, shaderstate.curdlight, shaderstate.mode, worldpvs, worldareas);
	GLBE_SubmitMeshes(NULL, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
	GLBE_SelectEntity(&r_worldentity);
	shaderstate.mbatches = ob;
}
#endif

void GLBE_RenderToTextureUpdate2d(qboolean destchanged)
{
	unsigned int width = 0, height = 0;
	if (destchanged)
	{
		if (*r_refdef.rt_destcolour[0].texname)
		{
			texid_t tex = R2D_RT_GetTexture(r_refdef.rt_destcolour[0].texname, &width, &height);
			GLBE_FBO_Update(&shaderstate.fbo_2dfbo, 0, &tex, 1, r_nulltex, width, height, 0);
		}
		else
			GLBE_FBO_Push(NULL);

		GL_Set2D(false);
	}
	else
	{
		shaderstate.tex_sourcecol = R2D_RT_GetTexture(r_refdef.rt_sourcecolour.texname, &width, &height);
		shaderstate.tex_sourcedepth = R2D_RT_GetTexture(r_refdef.rt_depth.texname, &width, &height);

		shaderstate.tex_reflectcube = R_GetDefaultEnvmap();
	}
}
void GLBE_FBO_Sources(texid_t sourcecolour, texid_t sourcedepth)
{
	shaderstate.tex_sourcecol = sourcecolour;
	shaderstate.tex_sourcedepth = sourcedepth;
}
int GLBE_FBO_Push(fbostate_t *state)
{
	int newfbo;
	int oldfbo = shaderstate.fbo_current;
	if (state)
		newfbo = state->fbo;
	else
		newfbo = 0;
	if (shaderstate.fbo_current == newfbo)	//don't bother if its not changed (also avoids crashes when fbos are not supported)
		return oldfbo;
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaderstate.fbo_current=newfbo);
	return oldfbo;
}
void GLBE_FBO_Pop(int oldfbo)
{
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, oldfbo);
	shaderstate.fbo_current = oldfbo;
}

void GLBE_FBO_Destroy(fbostate_t *state)
{
	if (state->fbo == shaderstate.fbo_current)
		GLBE_FBO_Push(NULL);

	//wasn't initialised anyway.
	if (!state->fbo)
		return;

	qglDeleteFramebuffersEXT(1, &state->fbo);
	state->fbo = 0;

	if (state->rb_depth)
		qglDeleteRenderbuffersEXT(1, &state->rb_depth);
	state->rb_depth = 0;
	if (state->rb_stencil)
		qglDeleteRenderbuffersEXT(1, &state->rb_stencil);
	state->rb_stencil = 0;
	if (state->rb_depthstencil)
		qglDeleteRenderbuffersEXT(1, &state->rb_depthstencil);
	state->rb_depthstencil = 0;

	state->enables = 0;
}

#ifdef RTLIGHTS
#ifdef SHADOWDBG_COLOURNOTDEPTH
void GLBE_BeginRenderBuffer_DepthOnly(texid_t depthtexture)
{
	if (gl_config.ext_framebuffer_objects)
	{
		if (!shadow_fbo_id)
		{
			int drb;
			qglGenFramebuffersEXT(1, &shadow_fbo_id);
			qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shadow_fbo_id);

			//create an unnamed depth buffer
//			qglGenRenderbuffersEXT(1, &drb);
//			qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, drb);
//			qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24_ARB, SHADOWMAP_SIZE*3, SHADOWMAP_SIZE*2);
//			qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, drb);

			if (qglDrawBuffer)
				qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			if (qglReadBuffer)
				qglReadBuffer(GL_NONE);
		}
		else
			qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shadow_fbo_id);

		if (TEXVALID(depthtexture))
			qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, depthtexture->num, 0);
	}
}
#else
int GLBE_BeginRenderBuffer_DepthOnly(texid_t depthtexture)
{
	int old = shaderstate.fbo_current;
	if (gl_config.ext_framebuffer_objects)
	{
		if (!shadow_fbo_id)
		{
			qglGenFramebuffersEXT(1, &shadow_fbo_id);
			qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shadow_fbo_id);
			if (qglDrawBuffers)
				qglDrawBuffers(0, NULL);
			else if (qglDrawBuffer)
				qglDrawBuffer(GL_NONE);
			if (qglReadBuffer)
				qglReadBuffer(GL_NONE);
		}
		else
			qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shadow_fbo_id);
		shaderstate.fbo_current = shadow_fbo_id;

		if (shadow_fbo_depth_num != depthtexture->num)
		{
			shadow_fbo_depth_num = depthtexture->num;
			if (TEXVALID(depthtexture))
				qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthtexture->num, 0);
		}
	}
	return old;
}
#endif
#endif

//state->colour is created if usedepth is set and it doesn't previously exist
int GLBE_FBO_Update(fbostate_t *state, unsigned int enables, texid_t *destcol, int mrt, texid_t destdepth, int width, int height, int layer)
{
	static GLenum allcolourattachments[] ={GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT1_EXT,GL_COLOR_ATTACHMENT2_EXT,GL_COLOR_ATTACHMENT3_EXT,
									GL_COLOR_ATTACHMENT4_EXT,GL_COLOR_ATTACHMENT5_EXT,GL_COLOR_ATTACHMENT6_EXT,GL_COLOR_ATTACHMENT7_EXT};
	int i;
	int old;

	if (TEXVALID(destdepth))
	{
		enables |= FBO_TEX_DEPTH;
		enables &= ~FBO_RB_DEPTH;
	}

	enables |= (mrt<<16);

	if ((state->enables ^ enables) & ~FBO_RESET)
	{
		GLBE_FBO_Destroy(state);
		state->enables = enables & ~FBO_RESET;
		enables |= FBO_RESET;
	}

	if (!state->fbo)
	{
		qglGenFramebuffersEXT(1, &state->fbo);
		old = GLBE_FBO_Push(state);
		enables |= FBO_RESET;
	}
	else
		old = GLBE_FBO_Push(state);
	if (state->rb_size[0] != width || state->rb_size[1] != height || (enables & FBO_RESET))
	{
		if (state->rb_depth && !(enables & FBO_RB_DEPTH))
		{
			qglDeleteRenderbuffersEXT(1, &state->rb_depth);
			state->rb_depth = 0;
		}
		if (state->rb_stencil && !(enables & FBO_RB_STENCIL))
		{
			qglDeleteRenderbuffersEXT(1, &state->rb_stencil);
			state->rb_stencil = 0;
		}
		state->rb_size[0] = width;
		state->rb_size[1] = height;

		enables |= FBO_RESET;
		if (mrt)
		{	//be careful here, gles2 doesn't support glDrawBuffer. hopefully it'll make things up, but this is worrying.
			if (qglDrawBuffers)
				qglDrawBuffers(mrt, allcolourattachments);
			else if (qglDrawBuffer)
				qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			if (qglReadBuffer)
				qglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		}
		else
		{
			if (qglDrawBuffers)
				qglDrawBuffers(0, NULL);
			else if (qglDrawBuffer)
				qglDrawBuffer(GL_NONE);
			if (qglReadBuffer)
				qglReadBuffer(GL_NONE);
		}
	}
	if (enables & FBO_TEX_DEPTH)
	{
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, destdepth->num, 0);
		//fixme: no stencil
	}
	else if (enables & FBO_RB_DEPTH)
	{
		if (!state->rb_depth)
		{
			//create an unnamed depth buffer
			qglGenRenderbuffersEXT(1, &state->rb_depth);
//			if (!gl_config.ext_packed_depth_stencil)
//				qglGenRenderbuffersEXT(1, &state->rb_stencil);
			enables |= FBO_RESET;	//make sure it gets instanciated
		}

		if (enables & FBO_RESET)
		{
			qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, state->rb_depth);
			if (gl_config.ext_packed_depth_stencil)
				qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, state->rb_size[0], state->rb_size[1]);
			else
			{
				qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16_ARB, state->rb_size[0], state->rb_size[1]);
//				qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, state->rb_stencil);
//				qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, state->rb_size[0], state->rb_size[1]);
			}
			qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, state->rb_depth);
			if (gl_config.ext_packed_depth_stencil)
				qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, state->rb_depth);
			else
				qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, state->rb_stencil);
		}
	}

	for (i = 0; i < mrt; i++)
	{
		if ((destcol[i]->flags & IF_TEXTYPEMASK) == IF_TEXTYPE_CUBE)
		{
			//fixme: we should probably support whole-cubemap rendering for shadowmaps or something.
			qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + layer, destcol[i]->num, 0);
		}
		else
		{	//layer does not make sense here
			qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_TEXTURE_2D, destcol[i]->num, 0);
		}
	}

	i = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (GL_FRAMEBUFFER_COMPLETE_EXT != i)
	{
		switch(i)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_INCOMPLETE_FORMATS\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			Con_Printf("glCheckFramebufferStatus reported GL_FRAMEBUFFER_UNSUPPORTED\n");
			break;
		default:
			Con_Printf("glCheckFramebufferStatus returned %#x\n", i);
			break;
		}

		Con_Printf("FBO Enables: %x %i*%i\n", enables, width, height);
		for (i = 0; i < mrt; i++)
		{
			if (destcol[i])
			{
				Con_Printf("^[\\imgptr\\%#"PRIxSIZE"^]", (quintptr_t)destcol[i]);
				Con_Printf("Colour%i: \"%s\" %x %i*%i*%i%s %s\n", i, destcol[i]->ident, destcol[i]->flags, destcol[i]->width, destcol[i]->height, destcol[i]->depth, (destcol[i]->status != TEX_LOADED)?" NOT LOADED":"",  Image_FormatName(destcol[i]->format));
			}
			else
				Con_Printf("Colour%i: <NO IMAGE>\n", i);
		}
		if (enables & FBO_TEX_DEPTH && destdepth)
		{
			Con_Printf("^[\\imgptr\\%#"PRIxSIZE"^]", (quintptr_t)destdepth);
			Con_Printf("Depth: \"%s\" %x %i*%i*%i%s %s\n", destdepth->ident, destdepth->flags, destdepth->width, destdepth->height, destdepth->depth, (destdepth->status != TEX_LOADED)?" NOT LOADED":"",  Image_FormatName(destdepth->format));
		}
		else if (enables & FBO_TEX_DEPTH)
			Con_Printf("Depth: <NO IMAGE>\n");
		else if (enables & FBO_RB_DEPTH)
			Con_Printf("Depth: <RenderBuffer>\n");
	}
	return old;
}
/*
void GLBE_RenderToTexture(texid_t sourcecol, texid_t sourcedepth, texid_t destcol, texid_t destdepth, qboolean usedepth)
{
	shaderstate.tex_sourcecol = sourcecol;
	shaderstate.tex_sourcedepth = sourcedepth;
	if (!destcol.num)
	{
		shaderstate.fbo_current = 0;
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaderstate.fbo_current);
	}
	else
	{
		if (usedepth)
		{
			if (!shaderstate.fbo_diffuse)
			{
				qglGenFramebuffersEXT(1, &shaderstate.fbo_diffuse);
				qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaderstate.fbo_diffuse);

				//create an unnamed depth buffer
				qglGenRenderbuffersEXT(1, &shaderstate.rb_depth);
				qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, shaderstate.rb_depth);
				if (gl_config.ext_packed_depth_stencil)
				{
					qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_depth);
				}
				else
				{
					qglGenRenderbuffersEXT(1, &shaderstate.rb_stencil);
					qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, shaderstate.rb_stencil);
					qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_stencil);
				}
				qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_depth);

				qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
				qglReadBuffer(GL_NONE);
			}
			else
				qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaderstate.fbo_diffuse);
			shaderstate.fbo_current = shaderstate.fbo_diffuse;

			if (destdepth.num)
			{
				qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, destdepth.num, 0);
				qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
			}
			else
			{
				//resize the depth renderbuffer if its the wrong size now
				if (shaderstate.rb_depth_size[0] != r_refdef.fbo_width || shaderstate.rb_depth_size[1] != r_refdef.fbo_height)
				{
					shaderstate.rb_depth_size[0] = r_refdef.fbo_width;
					shaderstate.rb_depth_size[1] = r_refdef.fbo_height;

					qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, shaderstate.rb_depth);
					if (gl_config.ext_packed_depth_stencil)
					{
						qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, r_refdef.fbo_width, r_refdef.fbo_height);
						qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_depth);
					}
					else
					{
						qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24_ARB, r_refdef.fbo_width, r_refdef.fbo_height);

						qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, shaderstate.rb_stencil);
						qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, r_refdef.fbo_width, r_refdef.fbo_height);
					}
				}
				qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_depth);
				if (gl_config.ext_packed_depth_stencil)
					qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_depth);
				else
					qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, shaderstate.rb_stencil);
			}
		}
		else
		{
			if (!shaderstate.fbo_depthless)
			{
				qglGenFramebuffersEXT(1, &shaderstate.fbo_depthless);
				qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaderstate.fbo_depthless);

				qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
				qglReadBuffer(GL_NONE);
			}
			else
				qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaderstate.fbo_depthless);

			shaderstate.fbo_current = shaderstate.fbo_depthless;
		}
		
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, destcol.num, 0);
	}
}
*/

void GLBE_DrawLightPrePass(void)
{
	cvar_t *var;
	unsigned int i;
	qboolean redefine = false;
	texid_t depth, targets[countof(shaderstate.tex_gbuf)];
	const char *s;
	int w = r_refdef.pxrect.width, h = r_refdef.pxrect.height;
	/*
	walls(bumps) -> normalbuffer
	lights+normalbuffer -> lightlevelbuffer
	walls(diffuse)+lightlevelbuffer -> screen

	normalbuffer contains depth in the alpha channel. an actual depthbuffer is also generated at this time, which is used for depth test stuff but not as a shader input.
	*/
	int oldfbo;

	if (r_refdef.recurse)
		return;	//fixme: messy stuff...

	/*do portals*/
	BE_SelectMode(BEM_STANDARD);
	GLBE_SubmitMeshes(cl.worldmodel->batches, SHADER_SORT_PORTAL, SHADER_SORT_PORTAL);

	BE_SelectMode(BEM_GBUFFER);
	for (i = 0; i < countof(shaderstate.tex_gbuf); i++)
	{
		if (!TEXVALID(shaderstate.tex_gbuf[i]) || w != shaderstate.tex_gbuf[i]->width || h != shaderstate.tex_gbuf[i]->height)
		{
			if (!shaderstate.tex_gbuf[i])
			{
				shaderstate.tex_gbuf[i] = Image_CreateTexture(va("***gbuffer %u***", i), NULL, IF_CLAMP|IF_NEAREST|IF_NOMIPMAP|IF_RENDERTARGET);
				qglGenTextures(1, &shaderstate.tex_gbuf[i]->num);
			}
			shaderstate.tex_gbuf[i]->width = w;
			shaderstate.tex_gbuf[i]->height = h;
			redefine = true;
		}
	}

	//something changed, redefine the textures.
	if (redefine)
	{
		static const char *defualtfmts[countof(shaderstate.tex_gbuf)] =
			//depth,	normals,	difflight,	speclight
			{"depth",	"rgba16f",	"rgba16f",	"rgba8",	"", "", "", ""};

		checkglerror();

		for (i = 0; i < countof(shaderstate.tex_gbuf); i++)
		{
			GLint ifmt = 0;
			GLenum dfmt = GL_RGBA;
			GLenum dtype = GL_UNSIGNED_BYTE;
			var = Cvar_Get(va("gl_deferred_gbuffmt_%i", i), defualtfmts[i]?defualtfmts[i]:"", 0, "Deferred Rendering");
			if (!var)
				continue;
			if (!strcmp(var->string, "rgba32f"))
			{
				if (gl_config_gles)
				{	//gles3
					ifmt = GL_RGBA32F;
					dfmt = GL_RGBA;
					dtype = GL_FLOAT;
				}
				else
					ifmt = GL_RGBA32F;
			}
			else if (!strcmp(var->string, "rgba16f"))
			{
				if (gl_config_gles)
				{	//gles3
					ifmt = GL_RGBA16F;
					dfmt = GL_RGBA;
					dtype = GL_HALF_FLOAT;
				}
				else
					ifmt = GL_RGBA16F;
			}
//			else if (!strcmp(var->string, "rgba8s"))
//				ifmt = GL_RGBA8_SNORM;
			else if (!strcmp(var->string, "depth"))
			{
				dtype = GL_UNSIGNED_INT;
				ifmt = GL_DEPTH_COMPONENT;
				dfmt = GL_DEPTH_COMPONENT;
			}
			else if (!strcmp(var->string, "depth16"))
			{
				if (gl_config_gles)
				{
					dtype = GL_UNSIGNED_SHORT;
					ifmt = GL_DEPTH_COMPONENT;
				}
				else
					ifmt = GL_DEPTH_COMPONENT16_ARB;
				dfmt = GL_DEPTH_COMPONENT;
			}
			else if (!strcmp(var->string, "depth24"))
			{
				if (gl_config_gles)
				{
					dtype = GL_UNSIGNED_INT;
					ifmt = GL_DEPTH_COMPONENT;
				}
				else
					ifmt = GL_DEPTH_COMPONENT24_ARB;
				dfmt = GL_DEPTH_COMPONENT;
			}
			else if (!strcmp(var->string, "depth32"))
			{
				if (gl_config_gles)
				{
					dtype = GL_FLOAT;
					ifmt = GL_DEPTH_COMPONENT;
				}
				else
					ifmt = GL_DEPTH_COMPONENT32_ARB;
				dfmt = GL_DEPTH_COMPONENT;
			}
			else if (!strcmp(var->string, "rgb565"))
			{
				dtype = GL_UNSIGNED_SHORT_5_6_5;
				ifmt = GL_RGB;
				dfmt = GL_RGB;
			}
			else if (!strcmp(var->string, "rgba4"))
			{
				dtype = GL_UNSIGNED_SHORT_4_4_4_4;
				ifmt = GL_RGBA;
				dfmt = GL_RGBA;
			}
			else if (!strcmp(var->string, "rgba5551"))
			{
				dtype = GL_UNSIGNED_SHORT_5_5_5_1;
				ifmt = GL_RGBA;
				dfmt = GL_RGBA;
			}
			else if (!strcmp(var->string, "rgba8") || *var->string)
			{
#ifndef GLESONLY
				if (!gl_config_gles)
					ifmt = GL_RGBA8;
				else
#endif
					ifmt = GL_RGBA;
				dfmt = GL_RGBA;
			}
			else
				continue;

			shaderstate.tex_gbuf[i]->status = TEX_LOADED;
			GL_MTBind(0, GL_TEXTURE_2D, shaderstate.tex_gbuf[i]);
			qglTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, dfmt, dtype, NULL);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			if (qglGetError())
				Con_Printf("unable to configure gbuffer image as '%s'\n", var->string);
		}
	}

	/*set the FB up to draw surface info*/
	var = Cvar_Get2("gl_deferred_pre_depth", "0", 0, "gbuffer index used for depth. negative means to use an annonamous renderbuffer", "Deferred Rendering");
	if (var->ival < 0 || var->ival >= countof(shaderstate.tex_gbuf))
		depth = r_nulltex;
	else
		depth = shaderstate.tex_gbuf[var->ival];
	var = Cvar_Get2("gl_deferred_pre_targets", "1", 0, "space-separated list of gbuffer indexes to use for deferred surface information", "Deferred Rendering");
	for (i = 0, s = var->string; *s && i < countof(targets); )
	{
		char token[32];
		int b;
		s = COM_ParseOut(s, token, sizeof(token));
		if (!*token)
			continue;
		b = atoi(token);
		if (b >= 0 && b < countof(shaderstate.tex_gbuf))
			targets[i++] = shaderstate.tex_gbuf[b];
	}

	oldfbo = GLBE_FBO_Update(&shaderstate.fbo_lprepass, depth?FBO_TEX_DEPTH:FBO_RB_DEPTH, targets, i, depth, w, h, 0);
	if (GL_FRAMEBUFFER_COMPLETE_EXT != qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT))
	{
		Con_Printf("Bad framebuffer\n");
		return;
	}
	GL_ForceDepthWritable();
	//FIXME: should probably clear colour buffer too.
	qglClear(GL_DEPTH_BUFFER_BIT);

	/*draw surfaces that can be drawn this way*/
	GLBE_SubmitMeshes(cl.worldmodel->batches, SHADER_SORT_OPAQUE, SHADER_SORT_OPAQUE);

	/*reconfigure - now drawing diffuse light info using the previous fb image as a source image*/
	var = Cvar_Get2("gl_deferred_light_targets", "2 3", 0, "space-separated list of gbuffer indexes for lighting to write to", "Deferred Rendering");
	for (i = 0, s = var->string; *s && i < countof(targets); )
	{
		char token[32];
		int b;
		s = COM_ParseOut(s, token, sizeof(token));
		if (!*token)
			continue;
		b = atoi(token);
		if (b >= 0 && b < countof(shaderstate.tex_gbuf))
			targets[i++] = shaderstate.tex_gbuf[b];
	}
	GLBE_FBO_Update(&shaderstate.fbo_lprepass, depth?FBO_TEX_DEPTH:FBO_RB_DEPTH, targets, i, depth, w, h, 0);

	BE_SelectMode(BEM_STANDARD);
	qglClearColor (0,0,0,0);
	qglClear(GL_COLOR_BUFFER_BIT);

	GLBE_SelectEntity(&r_worldentity);
	/*now draw the prelights*/
	GLBE_SubmitMeshes(cl.worldmodel->batches, SHADER_SORT_DEFERREDLIGHT, SHADER_SORT_DEFERREDLIGHT);

	/*final reconfigure - now drawing final surface data onto true framebuffer*/
	GLBE_FBO_Pop(oldfbo);
	if (!oldfbo && qglDrawBuffer)
		qglDrawBuffer(GL_BACK);

	/*now draw the postlight passes (this includes blended stuff which will NOT be lit)*/
	GLBE_SelectEntity(&r_worldentity);
	GLBE_SubmitMeshes(cl.worldmodel->batches, SHADER_SORT_SKY, SHADER_SORT_NEAREST);

#ifdef RTLIGHTS
	/*regular lighting now*/
	GLBE_SelectEntity(&r_worldentity);
	Sh_DrawLights(r_refdef.scenevis);
#endif

	qglClearColor (1,0,0,1);
}

qboolean R_DrawSkyroom(shader_t *skyshader);

void GLBE_DrawWorld (batch_t **worldbatches)
{
#ifdef RTLIGHTS
	extern cvar_t r_shadow_realtime_world, r_shadow_realtime_world_lightmaps;
#endif
	batch_t *batches[SHADER_SORT_COUNT];
	batch_t **ob = shaderstate.mbatches;
	RSpeedLocals();
	shaderstate.mbatches = batches;

	shaderstate.usingweaponviewmatrix = -1;

	TRACE(("GLBE_DrawWorld: %p\n", worldbatches));

	//reset batches if we needed more mem, to avoid allocations mid-frame.
	if (!r_refdef.recurse)
	{
		if (shaderstate.wbatch + 50 > shaderstate.maxwbatches)
		{
			int newm = shaderstate.wbatch + 100;
			shaderstate.wbatches = BZ_Realloc(shaderstate.wbatches, newm * sizeof(*shaderstate.wbatches));
			memset(shaderstate.wbatches + shaderstate.maxwbatches, 0, (newm - shaderstate.maxwbatches) * sizeof(*shaderstate.wbatches));
			shaderstate.maxwbatches = newm;
		}

		shaderstate.wbatch = 0;
	}
	//if the video mode changed, update any fbos (hopefully this won't happen on mirrors)
	if (shaderstate.oldwidth != vid.pixelwidth || shaderstate.oldheight != vid.pixelheight)
	{
		GLBE_DestroyFBOs();	//will be recreated on demand
		shaderstate.oldwidth = vid.pixelwidth;
		shaderstate.oldheight = vid.pixelheight;

		while(shaderstate.lastpasstmus>0)
		{
			GL_LazyBind(--shaderstate.lastpasstmus, r_nulltex);
		}
#ifdef RTLIGHTS
		Sh_Reset();
#endif
	}

	//memset(batches, 0, sizeof(batches));
	BE_GenModelBatches(batches, shaderstate.curdlight, BEM_STANDARD, r_refdef.scenevis, r_refdef.sceneareas);
	R_GenDlightBatches(batches);
	shaderstate.curentity = &r_worldentity;
//	if (cl.paused || cls.state < ca_active)
		shaderstate.updatetime = r_refdef.time;
//	else
//		shaderstate.updatetime = cl.servertime;

	GLBE_UpdateLightmaps();
	if (worldbatches)
	{
		if (worldbatches[SHADER_SORT_SKY] && r_refdef.skyroom_enabled)
		{
			batch_t *b;
			for (b = worldbatches[SHADER_SORT_SKY]; b; b = b->next)
				if (R_DrawSkyroom(b->shader))
				{
					GL_CullFace(0);//make sure flipcull reversion takes effect
					currententity = NULL;
					GLBE_SelectEntity(&r_worldentity);
					GL_ForceDepthWritable();
					qglClear(GL_DEPTH_BUFFER_BIT);
					r_refdef.flags |= RDF_SKIPSKY;
					break;
				}
		}
		if (gl_overbright.modified)
		{
			int i;
			gl_overbright.modified = false;
			if (gl_overbright.ival > 2)
				gl_overbright.ival = 2;

			for (i = 0; i < SHADER_TMU_MAX; i++)
				shaderstate.blendmode[i] = -1;
		}

#ifdef RTLIGHTS
		if (worldbatches && r_shadow_realtime_world.ival)
			shaderstate.identitylighting = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			shaderstate.identitylighting = r_lightmap_scale.value;
		shaderstate.identitylighting *= r_refdef.hdr_value;
		shaderstate.identitylightmap = shaderstate.identitylighting;
//		shaderstate.identitylightmap *= 1<<gl_overbright.ival;

//		if (cl.worldmodel && cl.worldmodel->fromgame == fg_quake3)
//			shaderstate.identitylighting *= 2;

#ifdef RTLIGHTS
		if (r_lightprepass)
		{
			GLBE_SelectEntity(&r_worldentity);
			GLBE_DrawLightPrePass();
		}
		else
#endif
		{
#ifdef RTLIGHTS
			if (r_fakeshadows)
				Sh_GenerateFakeShadows();
#endif
			GLBE_SelectEntity(&r_worldentity);

			if (shaderstate.identitylighting == 0)
				BE_SelectMode(BEM_DEPTHDARK);
			else
				BE_SelectMode(BEM_STANDARD);

			RSpeedRemark();
			GLBE_SubmitMeshes(worldbatches, SHADER_SORT_PORTAL, SHADER_SORT_SEETHROUGH+1);
			RSpeedEnd(RSPEED_OPAQUE);

#ifdef RTLIGHTS
			if (worldbatches)
			{
				RSpeedRemark();
				TRACE(("GLBE_DrawWorld: drawing lights\n"));
				GLBE_SelectEntity(&r_worldentity);
				Sh_DrawLights(r_refdef.scenevis);
				RSpeedEnd(RSPEED_RTLIGHTS);
				TRACE(("GLBE_DrawWorld: lights drawn\n"));
			}
#endif
		}

#ifndef GLSLONLY
		if (r_outline.ival && !r_wireframe.ival && qglPolygonMode && qglLineWidth)
		{
			int oc = r_refdef.flipcull;
			shaderstate.identitylighting = 0;
			shaderstate.identitylightmap = 0;
			r_refdef.flipcull ^= SHADER_CULL_FLIP;
			GLBE_SelectMode(BEM_DEPTHDARK);
			shaderstate.polyoffset.unit = 1;
			shaderstate.polyoffset.factor = 1;

			qglEnable(GL_POLYGON_OFFSET_LINE);
			qglLineWidth (bound(0.1, r_outline_width.value, 3.0));
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			GLBE_SubmitMeshes(NULL, SHADER_SORT_PORTAL, SHADER_SORT_OPAQUE+1);
			r_refdef.flipcull = oc;
			GLBE_SelectMode(BEM_STANDARD);
			qglDisable(GL_POLYGON_OFFSET_LINE);
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			qglLineWidth (1);
		}
#endif

		shaderstate.identitylighting = 1;

		RSpeedRemark();
		GLBE_SubmitMeshes(worldbatches, SHADER_SORT_SEETHROUGH+1, SHADER_SORT_NEAREST);
		RSpeedEnd(RSPEED_TRANSPARENTS);

#ifndef GLSLONLY
		if (r_refdef.globalfog.density && (!gl_config.arb_shader_objects || !r_fog_permutation.ival))
		{	//fixed function-only. with global fog. that means we need to hack something in.
			//FIXME: should really be doing this on a per-shader basis, for custom shaders that don't use glsl
			BE_SelectMode(BEM_FOG);

			BE_SelectFog(r_refdef.globalfog.colour, r_refdef.globalfog.alpha, r_refdef.globalfog.density);
			GLBE_SubmitMeshes(worldbatches, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);
		}
#endif

#ifdef GL_LINE	//no gles
		if (r_wireframe.ival && qglPolygonMode)
		{
			BE_SelectMode(BEM_WIREFRAME);
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			GLBE_SubmitMeshes(worldbatches, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);
			BE_SelectMode(BEM_STANDARD);
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
#endif
		shaderstate.curdlight = NULL;
	}
	else
	{
		GLBE_SelectEntity(&r_worldentity);
		GLBE_SubmitMeshes(NULL, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);

#ifdef GL_LINE	//no gles
		if (r_wireframe.ival && qglPolygonMode)
		{
			BE_SelectMode(BEM_WIREFRAME);
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			GLBE_SubmitMeshes(NULL, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);
			BE_SelectMode(BEM_STANDARD);
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
#endif
	}

	GLBE_SelectEntity(&r_worldentity);
//	shaderstate.curtime = shaderstate.updatetime = realtime;
	shaderstate.usingweaponviewmatrix = -1;

	shaderstate.identitylighting = 1;

	shaderstate.mbatches = ob;

	TRACE(("GLBE_DrawWorld: drawn everything\n"));
}

void GLBE_VBO_Begin(vbobctx_t *ctx, size_t maxsize)
{
	COM_AssertMainThread("GLBE_VBO_Begin");	//actually, we should probably just build this in memory and throw it to the main thread as needed, but we still need some buffers indexes. I guess we could build a list of varrays. the other option is to just create a large persistant-mapped buffer, and then just append+reuse from any thread, but that makes destruction messy.

	ctx->maxsize = maxsize;
	ctx->pos = 0;
	ctx->fallback = NULL;
	if (qglBufferStorage)
	{
		ctx->vboid[0] = ctx->vboid[1] = 0;
		qglGenBuffersARB(2, ctx->vboid);
		ctx->fallback = BZ_Malloc(maxsize);
	}
	else if (qglBufferDataARB)
	{
		ctx->vboid[0] = ctx->vboid[1] = 0;
		qglGenBuffersARB(2, ctx->vboid);
		GL_SelectVBO(ctx->vboid[0]);
		//WARNING: in emscripten/webgl, we should probably not pass null.
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, ctx->maxsize, NULL, GL_STATIC_DRAW_ARB);
	}
	else
		ctx->fallback = BZ_Malloc(maxsize);
}
void GLBE_VBO_Data(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray)
{
	if (qglBufferStorage)
	{
		memcpy((char*)ctx->fallback + ctx->pos, data, size);
		varray->gl.vbo = ctx->vboid[0];
		varray->gl.addr = (void*)ctx->pos;
	}
	else if (ctx->fallback)
	{
		memcpy((char*)ctx->fallback + ctx->pos, data, size);
		varray->gl.vbo = 0;
		varray->gl.addr = (char*)ctx->fallback + ctx->pos;
	}
	else
	{
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, ctx->pos, size, data);
		varray->gl.vbo = ctx->vboid[0];
		varray->gl.addr = (void*)ctx->pos;
	}
	ctx->pos += size;
}

void GLBE_VBO_Finish(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem)
{
	if (ctx->pos > ctx->maxsize)
		Sys_Error("BE_VBO_Finish: too much data given\n");
	if (qglBufferStorage)
	{
		GL_SelectVBO(ctx->vboid[0]);
		qglBufferStorage(GL_ARRAY_BUFFER_ARB, ctx->pos, ctx->fallback, 0);
		BZ_Free(ctx->fallback);
		ctx->fallback = NULL;
		GL_SelectEBO(ctx->vboid[1]);
		qglBufferStorage(GL_ELEMENT_ARRAY_BUFFER_ARB, esize, edata, 0);
		earray->gl.vbo = ctx->vboid[1];
		earray->gl.addr = NULL;
	}
	else if (ctx->fallback)
	{
		void *d = BZ_Malloc(esize);
		memcpy(d, edata, esize);
		earray->gl.vbo = 0;
		earray->gl.addr = d;
	}
	else
	{
		GL_SelectEBO(ctx->vboid[1]);
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, esize, edata, GL_STATIC_DRAW_ARB);
		earray->gl.vbo = ctx->vboid[1];
		earray->gl.addr = NULL;
	}
}
void GLBE_VBO_Destroy(vboarray_t *vearray, void *vbomem)
{
	if (vearray->gl.vbo)
	{
		qglDeleteBuffersARB(1, &vearray->gl.vbo);
		vearray->gl.vbo = 0;
	}
	else
		BZ_Free(vearray->gl.addr);
	vearray->gl.addr = NULL;
}
#endif
