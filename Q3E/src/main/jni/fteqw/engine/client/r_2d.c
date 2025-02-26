#include "quakedef.h"
#ifndef SERVERONLY
#include "shader.h"
#include "gl_draw.h"

#define WIN32_BLOATED
#include "glquake.h"

qboolean r2d_canhwgamma;	//says the video code has successfully activated hardware gamma
texid_t missing_texture;
texid_t missing_texture_gloss;
texid_t missing_texture_normal;

texid_t translate_texture;
shader_t *translate_shader;

texid_t ch_int_texture;
vec3_t ch_color;
shader_t *shader_crosshair;

static mpic_t *conback;
static mpic_t *draw_backtile;
shader_t *shader_draw_fill, *shader_draw_fill_trans;
mpic_t		*draw_disc;

shader_t *shader_contrastup;
shader_t *shader_contrastdown;
shader_t *shader_brightness;
shader_t *shader_gammacb;
shader_t *shader_polyblend;
shader_t *shader_menutint;

#define DRAW_QUADS 128
static int		draw_active_flags;
static shader_t *draw_active_shader;
static avec4_t	draw_active_colour;
static mesh_t	draw_mesh;
static vecV_t	draw_mesh_xyz[DRAW_QUADS];
vec2_t			draw_mesh_st[DRAW_QUADS];
static avec4_t	draw_mesh_colors[DRAW_QUADS];
index_t			r_quad_indexes[DRAW_QUADS*6];
unsigned int	r2d_be_flags;

struct
{
	lmalloc_t allocation;
	qboolean dirty;
	uploadfmt_t fmt;
	int lastid;
	unsigned int *data;
	shader_t *shader;
	texid_t tex;
	apic_t *pics;
} atlas;

extern cvar_t scr_conalpha;
extern cvar_t gl_conback;
extern cvar_t gl_font, con_textfont;
extern cvar_t r_font_postprocess_outline, r_font_postprocess_mono;
extern cvar_t gl_screenangle;
extern cvar_t vid_minsize;
extern cvar_t vid_conautoscale;
extern cvar_t vid_baseheight;
extern cvar_t vid_conheight;
extern cvar_t vid_conwidth;
extern cvar_t con_textsize;
static void QDECL R2D_Font_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL R2D_Conautoscale_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL R2D_ScreenAngle_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL R2D_Conheight_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL R2D_Conwidth_Callback(struct cvar_s *var, char *oldvalue);

extern cvar_t crosshair;
extern cvar_t crosshaircolor;
extern cvar_t crosshairsize;
extern cvar_t crosshairimage;
extern cvar_t crosshairalpha;
static void QDECL R2D_Crosshair_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL R2D_CrosshairImage_Callback(struct cvar_s *var, char *oldvalue);
static void QDECL R2D_CrosshairColor_Callback(struct cvar_s *var, char *oldvalue);

void (*R2D_Flush)(void);

//We need this for minor things though, so we'll just use the slow accurate method.
//this is unlikly to be called too often.
qbyte GetPaletteIndex(int red, int green, int blue)
{
	//slow, horrible method.
	{
		int i, best=15;
		int bestdif=256*256*256, curdif;
		extern qbyte *host_basepal;
		qbyte *pa;

	#define _abs(x) ((x)*(x))

		pa = host_basepal;
		for (i = 0; i < 256; i++, pa+=3)
		{
			curdif = _abs(red - pa[0]) + _abs(green - pa[1]) + _abs(blue - pa[2]);
			if (curdif < bestdif)
			{
				if (curdif<1)
					return i;
				bestdif = curdif;
				best = i;
			}
		}
		return best;
	}
}
qbyte GetPaletteIndexNoFB(int red, int green, int blue)
{
	//slow, horrible method.
	{
		int i, best=15;
		int bestdif=256*256*256, curdif;
		extern qbyte *host_basepal;
		qbyte *pa;

	#define _abs(x) ((x)*(x))

		pa = host_basepal;
		for (i = 0; i < 256 - vid.fullbright; i++, pa+=3)
		{
			curdif = _abs(red - pa[0]) + _abs(green - pa[1]) + _abs(blue - pa[2]);
			if (curdif < bestdif)
			{
				if (curdif<1)
					return i;
				bestdif = curdif;
				best = i;
			}
		}
		return best;
	}
}
qbyte GetPaletteIndexRange(int first, int stop, int red, int green, int blue)
{
	//slow, horrible method.
	{
		int i, best=15;
		int bestdif=256*256*256, curdif;
		extern qbyte *host_basepal;
		qbyte *pa;

	#define _abs(x) ((x)*(x))

		pa = host_basepal;
		for (i = first; i < stop; i++, pa+=3)
		{
			curdif = _abs(red - pa[0]) + _abs(green - pa[1]) + _abs(blue - pa[2]);
			if (curdif < bestdif)
			{
				if (curdif<1)
					return i;
				bestdif = curdif;
				best = i;
			}
		}
		return best;
	}
}

void R2D_Shutdown(void)
{
	Cvar_Unhook(&con_textfont);
	Cvar_Unhook(&gl_font);
	Cvar_Unhook(&r_font_postprocess_outline);
	Cvar_Unhook(&r_font_postprocess_mono);
	Cvar_Unhook(&vid_conautoscale);
	Cvar_Unhook(&gl_screenangle);
	Cvar_Unhook(&vid_conheight);
	Cvar_Unhook(&vid_conwidth);
	Cvar_Unhook(&vid_baseheight);
	Cvar_Unhook(&vid_minsize);

	Cvar_Unhook(&crosshair);
	Cvar_Unhook(&crosshairimage);
	Cvar_Unhook(&crosshaircolor);

	BZ_Free(cl_stris);
	cl_stris = NULL;
	BZ_Free(cl_strisvertv);
	cl_strisvertv = NULL;
	BZ_Free(cl_strisvertc);
	cl_strisvertc = NULL;
	BZ_Free(cl_strisvertt);
	cl_strisvertt = NULL;
	BZ_Free(cl_strisidx);
	cl_strisidx = NULL;
	cl_numstrisidx = 0;
	cl_maxstrisidx = 0;
	cl_numstrisvert = 0;
	cl_maxstrisvert = 0;
	cl_numstris = 0;
	cl_maxstris = 0;

	Con_FlushBackgrounds();

	if (font_console == font_default)
		font_console = NULL;

	if (font_console)
		Font_Free(font_console);
	font_console = NULL; 
	if (font_default)
		Font_Free(font_default);
	font_default = NULL; 
	if (font_tiny)
		Font_Free(font_tiny);
	font_tiny = NULL; 

#ifndef NOBUILTINMENUS
	M_ReloadMenus();
#endif

#if defined(MENU_DAT) || defined(CSQC_DAT)
	PR_ReloadFonts(false);
#endif

	while(atlas.pics)
	{
		apic_t *a = atlas.pics;
		atlas.pics = a->next;
		Z_Free(a);
	}

	Z_Free(atlas.data);
	memset(&atlas, 0, sizeof(atlas));
}

/*
Iniitalise the 2d rendering functions (including font).
Image loading code must be ready for use at this point.
*/
void R2D_Init(void)
{
	unsigned int nonorm[4*4];
	unsigned int nogloss[4*4];
	int i, j;
	unsigned int glossval;
	unsigned int normval;
	extern cvar_t gl_specular_fallback, gl_specular_fallbackexp, gl_texturemode;
	conback = NULL;

	Cvar_ForceCallback(&gl_texturemode);

	draw_mesh.istrifan = true;
	draw_mesh.numvertexes = 4;
	draw_mesh.numindexes = 6;
	draw_mesh.xyz_array = draw_mesh_xyz;
	draw_mesh.st_array = draw_mesh_st;
	draw_mesh.colors4f_array[0] = draw_mesh_colors;
	draw_mesh.indexes = r_quad_indexes;

	for (i = 0, j = 0; i < countof(r_quad_indexes); i += 6, j += 4)
	{
		r_quad_indexes[i+0] = j+0;
		r_quad_indexes[i+1] = j+1;
		r_quad_indexes[i+2] = j+2;
		r_quad_indexes[i+3] = j+2;
		r_quad_indexes[i+4] = j+3;
		r_quad_indexes[i+5] = j+0;
	}


	if (strchr(gl_specular_fallback.string, ' '))
	{
		glossval = bound(0, (int)(gl_specular_fallback.vec4[0]*255), 255)<<0;
		glossval |= bound(0, (int)(gl_specular_fallback.vec4[1]*255), 255)<<8;
		glossval |= bound(0, (int)(gl_specular_fallback.vec4[2]*255), 255)<<16;
	}
	else
	{
		glossval = min(gl_specular_fallback.value*255, 255);
		glossval *= 0x10101;
	}
	glossval |= 0x01000000 * bound(0, (int)(gl_specular_fallbackexp.value*255), 255);
	glossval = LittleLong(glossval);
	normval = 0xffff8080;
	normval = LittleLong(normval);
	for (i = 0; i < 4*4; i++)
	{
		nogloss[i] = glossval;
		nonorm[i] = normval;
	}
	missing_texture = R_LoadTexture8("no_texture", 16, 16, (unsigned char*)(r_notexture_mip+1), IF_NOALPHA|IF_NOGAMMA|IF_NOPURGE, 0);
	missing_texture_gloss = R_LoadTexture("no_texture_gloss", 4, 4, TF_RGBA32, (unsigned char*)nogloss, IF_NOGAMMA|IF_NOPURGE);
	missing_texture_normal = R_LoadTexture("no_texture_normal", 4, 4, TF_RGBA32, (unsigned char*)nonorm, IF_NOGAMMA|IF_NOPURGE);
	translate_texture = r_nulltex;
	ch_int_texture = r_nulltex;

	Shader_Init();
	BE_Init();
	Font_Init();

	draw_backtile = R_RegisterShader("gfx/backtile.lmp", SUF_NONE,
		"{\n"
			"if $nofixed\n"
				"program default2d\n"
			"endif\n"
			"affine\n"
			"nomipmaps\n"
			"{\n"
				"map $diffuse\n"
			"}\n"
		"}\n");
	TEXDOWAIT(draw_backtile->defaulttextures->base);
	if (!TEXLOADED(draw_backtile->defaulttextures->base))
		draw_backtile->defaulttextures->base = R_LoadHiResTexture("gfx/backtile", NULL, IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_NOWORKER);
	if (!TEXLOADED(draw_backtile->defaulttextures->base))
		draw_backtile->defaulttextures->base = R_LoadHiResTexture("gfx/menu/backtile", NULL, IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_NOWORKER);
	if (!TEXLOADED(draw_backtile->defaulttextures->base))
		draw_backtile->defaulttextures->base = R_LoadHiResTexture("pics/backtile", NULL, IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_NOWORKER);

	shader_draw_fill = R_RegisterShader("fill_opaque", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"map $whiteimage\n"
				"rgbgen exactvertex\n"
				"alphagen vertex\n"
			"}\n"
		"}\n");
	shader_draw_fill_trans = R_RegisterShader("fill_trans", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"map $whiteimage\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"blendfunc blend\n"
				"maskalpha\n"
			"}\n"
		"}\n");
	shader_contrastup = R_RegisterShader("contrastupshader", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"nodepthtest\n"
				"map $whiteimage\n"
				"blendfunc gl_dst_color gl_one\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"maskalpha\n"
			"}\n"
		"}\n"
	);
	shader_contrastdown = R_RegisterShader("contrastdownshader", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"nodepthtest\n"
				"map $whiteimage\n"
				"blendfunc gl_dst_color gl_zero\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"maskalpha\n"
			"}\n"
		"}\n"
	);
	shader_brightness = R_RegisterShader("brightnessshader", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"nodepthtest\n"
				"map $whiteimage\n"
				"blendfunc gl_one gl_one\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"maskalpha\n"
			"}\n"
		"}\n"
	);
	shader_gammacb = R_RegisterShader("gammacbshader", SUF_NONE,
		"{\n"
			"program defaultgammacb\n"
			"affine\n"
			"{\n"
				"map $currentrender\n"
				"nodepthtest\n"
				"maskalpha\n"
			"}\n"
		"}\n"
	);
	shader_polyblend = R_RegisterShader("polyblendshader", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"map $whiteimage\n"
				"blendfunc gl_src_alpha gl_one_minus_src_alpha\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"maskalpha\n"
			"}\n"
		"}\n"
	);
	shader_menutint = R_RegisterShader("menutint", SUF_NONE,
		"{\n"
			"affine\n"
#ifdef FTE_TARGET_WEB
				//currentrender is problematic here, so avoid using it.
				"program default2d\n"
				"{\n"
					"map $whiteimage\n"
					"blendfunc gl_dst_color gl_zero\n"
					"rgbgen srgb $r_menutint\n"
				"}\n"
#else
			"if gl_menutint_shader != 0\n"
				"program menutint\n"
			"endif\n"
			"if $haveprogram\n"
				"{\n"
					"map $currentrender\n"
				"}\n"
			"else\n"
				"{\n"
					"map $whiteimage\n"
					"blendfunc gl_dst_color gl_zero\n"
					"rgbgen srgb $r_menutint\n"
				"}\n"
			"endif\n"
#endif
		"}\n"
	);
	shader_crosshair = R_RegisterShader("crosshairshader", SUF_NONE,
		"{\n"
			"if $nofixed\n"
				"program default2d\n"
			"endif\n"
			"affine\n"
			"nomipmaps\n"
			"{\n"
				"map $diffuse\n"
				"blendfunc blend\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}\n"
		);


	Cvar_Hook(&con_textfont, R2D_Font_Callback);
	Cvar_Hook(&gl_font, R2D_Font_Callback);
	Cvar_Hook(&r_font_postprocess_outline, R2D_Font_Callback);
	Cvar_Hook(&r_font_postprocess_mono, R2D_Font_Callback);
	Cvar_Hook(&vid_conautoscale, R2D_Conautoscale_Callback);
	Cvar_Hook(&gl_screenangle, R2D_ScreenAngle_Callback);
	Cvar_Hook(&vid_conheight, R2D_Conheight_Callback);
	Cvar_Hook(&vid_conwidth, R2D_Conwidth_Callback);
	Cvar_Hook(&vid_baseheight, R2D_Conautoscale_Callback);
	Cvar_Hook(&vid_minsize, R2D_Conautoscale_Callback);

	Cvar_Hook(&crosshair, R2D_Crosshair_Callback);
	Cvar_Hook(&crosshairimage, R2D_CrosshairImage_Callback);
	Cvar_Hook(&crosshaircolor, R2D_CrosshairColor_Callback);

	Cvar_ForceCallback(&gl_conback);
	Cvar_ForceCallback(&vid_conautoscale);
	Cvar_ForceCallback(&gl_font);

	Cvar_ForceCallback(&crosshair);
	Cvar_ForceCallback(&crosshaircolor);

	R2D_Font_Changed();

	R_NetgraphInit();

	atlas.lastid = -1;
	atlas.shader = NULL;
	atlas.data = NULL;
	atlas.dirty = false;
	Mod_LightmapAllocInit(&atlas.allocation, false, min(512, sh_config.texture2d_maxsize), min(512, sh_config.texture2d_maxsize), 0);
}

mpic_t	*R2D_SafeCachePic (const char *path)
{
	shader_t *s;
	if (!qrenderer)
		return NULL;
	s = R_RegisterPic(path, NULL);
	return s;
}


mpic_t *R2D_SafePicFromWad (const char *name)
{
	shader_t *s;
	if (!qrenderer || strchr(name, ':'))
		return NULL;
	s = R_RegisterCustom (NULL, va("gfx/%s", name), SUF_2D, Shader_Default2D, "wad");
	return s;
}

apic_t *R2D_LoadAtlasedPic(const char *name)
{
	apic_t *apic = Z_Malloc(sizeof(*apic));
	qpic_t *qp;
	int x,y;
	qbyte *indata = NULL;
	int atlasid;

	if (!gl_load24bit.ival)
	{
		size_t lumpsize;
		qbyte lumptype;
		qp = W_GetLumpName(name, &lumpsize, &lumptype);
		if (qp && lumptype == TYP_QPIC && lumpsize == 8+qp->width*qp->height)
		{
			apic->width = qp->width;
			apic->height = qp->height;
			indata = qp->data;
		}
	}
	
	if (!indata || apic->width > atlas.allocation.width || apic->height > atlas.allocation.height)
		atlasid = -1;	//can happen on voodoo cards
	else
		Mod_LightmapAllocBlock(&atlas.allocation, apic->width+2, apic->height+2, &apic->x, &apic->y, &atlasid);

	if (atlasid >= 0)
	{
		unsigned int *out;
		apic->x += 1;
		apic->y += 1;

		//FIXME: extend the atlas height instead, and keep going with a single atlas?
		if (atlasid != atlas.lastid)
		{
			atlas.lastid = atlasid;
			if (atlas.dirty)
				Image_Upload(atlas.tex, atlas.fmt, atlas.data, NULL, atlas.allocation.width, atlas.allocation.height, 1, IF_NOMIPMAP);
			atlas.tex = r_nulltex;
			atlas.fmt = sh_config.texfmt[PTI_BGRA8]?PTI_BGRA8:PTI_RGBA8;
			atlas.shader = NULL;
			atlas.dirty = false;
			if (atlas.data)	//clear atlas data instead of reallocating it.
				memset(atlas.data, 0, sizeof(*atlas.data) * atlas.allocation.width * atlas.allocation.height);
		}

		if (!atlas.tex)
			atlas.tex = Image_CreateTexture(va("fte_atlas%i", atlasid), NULL, IF_NOMIPMAP|IF_NOMIPMAP);
		if (!atlas.shader)
		{
			atlas.shader = R_RegisterShader(va("fte_atlas%i", atlasid), SUF_NONE,
				"{\n"
					"affine\n"
					"program default2d\n"
					"{\n"
						"map $diffuse\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"blendfunc gl_one gl_one_minus_src_alpha\n"
					"}\n"
				"}\n"
			);
			atlas.shader->defaulttextures->base = atlas.tex;
		}

		if (!atlas.data)
			atlas.data = Z_Malloc(sizeof(*atlas.data) * atlas.allocation.width * atlas.allocation.height);

		out = atlas.data;
		out += apic->x;
		out += apic->y * atlas.allocation.width;
		apic->atlas = atlas.shader;

		if (atlas.fmt == PTI_BGRA8)
		{
			//pad above. extra casts because 64bit msvc is RETARDED.
			out[-1 - (qintptr_t)atlas.allocation.width] = (indata[0] == 255)?0:d_8to24bgrtable[indata[0]];	//pad left
			for (x = 0; x < apic->width; x++)
				out[x-(qintptr_t)atlas.allocation.width] = (indata[x] == 255)?0:d_8to24bgrtable[indata[x]];
			out[x - (qintptr_t)atlas.allocation.width] = (indata[x-1] == 255)?0:d_8to24bgrtable[indata[x-1]];	//pad right
			for (y = 0; y < apic->height; y++)
			{
				out[-1] = (indata[0] == 255)?0:d_8to24bgrtable[indata[0]];	//pad left
				for (x = 0; x < apic->width; x++)
					out[x] = (indata[x] == 255)?0:d_8to24bgrtable[indata[x]];
				out[x] = (indata[x-1] == 255)?0:d_8to24bgrtable[indata[x-1]];	//pad right
				indata += x;
				out += atlas.allocation.width;
			}
			//pad below
			out[-1] = (indata[0] == 255)?0:d_8to24bgrtable[indata[0]];	//pad left
			for (x = 0; x < apic->width; x++)
				out[x] = (indata[x] == 255)?0:d_8to24bgrtable[indata[x]];
			out[x] = (indata[x-1] == 255)?0:d_8to24bgrtable[indata[x-1]];	//pad right
		}
		else
		{
			//pad above. extra casts because 64bit msvc is RETARDED.
			out[-1 - (qintptr_t)atlas.allocation.width] = (indata[0] == 255)?0:d_8to24rgbtable[indata[0]];	//pad left
			for (x = 0; x < apic->width; x++)
				out[x-(qintptr_t)atlas.allocation.width] = (indata[x] == 255)?0:d_8to24rgbtable[indata[x]];
			out[x - (qintptr_t)atlas.allocation.width] = (indata[x-1] == 255)?0:d_8to24rgbtable[indata[x-1]];	//pad right
			for (y = 0; y < apic->height; y++)
			{
				out[-1] = (indata[0] == 255)?0:d_8to24rgbtable[indata[0]];	//pad left
				for (x = 0; x < apic->width; x++)
					out[x] = (indata[x] == 255)?0:d_8to24rgbtable[indata[x]];
				out[x] = (indata[x-1] == 255)?0:d_8to24rgbtable[indata[x-1]];	//pad right
				indata += x;
				out += atlas.allocation.width;
			}
			//pad below
			out[-1] = (indata[0] == 255)?0:d_8to24rgbtable[indata[0]];	//pad left
			for (x = 0; x < apic->width; x++)
				out[x] = (indata[x] == 255)?0:d_8to24rgbtable[indata[x]];
			out[x] = (indata[x-1] == 255)?0:d_8to24rgbtable[indata[x-1]];	//pad right
		}

		//pinch inwards, for linear sampling
		apic->sl = (apic->x+0.5)/(float)atlas.allocation.width;
		apic->sh = (apic->width-1.0)/(float)atlas.allocation.width;

		apic->tl = (apic->y+0.5)/(float)atlas.allocation.height;
		apic->th = (apic->height-1.0)/(float)atlas.allocation.height;

		atlas.dirty = true;

		apic->next = atlas.pics;
		atlas.pics = apic;
	}
	else if (1)
	{
		apic->atlas = R_RegisterPic(va("gfx/%s", name), "wad");
		apic->sl = 0;
		apic->sh = 1;
		apic->tl = 0;
		apic->th = 1;

		apic->next = atlas.pics;
		atlas.pics = apic;
	}
	else
	{
		Z_Free(apic);
		return NULL;
	}
	return apic;
}

void R2D_ImageColours(float r, float g, float b, float a)
{
	draw_active_colour[0] = r;
	draw_active_colour[1] = g;
	draw_active_colour[2] = b;
	draw_active_colour[3] = a;

	Font_InvalidateColour(draw_active_colour);
}
void R2D_ImagePaletteColour(unsigned int i, float a)
{
	draw_active_colour[0] = SRGBf(host_basepal[i*3+0]/255.0);
	draw_active_colour[1] = SRGBf(host_basepal[i*3+1]/255.0);
	draw_active_colour[2] = SRGBf(host_basepal[i*3+2]/255.0);
	draw_active_colour[3] = a;

	Font_InvalidateColour(draw_active_colour);
}

//awkward and weird to use
void R2D_ImageAtlas(float x, float y, float w, float h, float s1, float t1, float s2, float t2, apic_t *pic)
{
	float newsl, newsh, newtl, newth;
	if (!pic)
		return;
	if (atlas.dirty)
	{
		Image_Upload(atlas.tex, atlas.fmt, atlas.data, NULL, atlas.allocation.width, atlas.allocation.height, 1, IF_NOMIPMAP);
		atlas.dirty = false;
	}

	newsl = pic->sl + s1 * pic->sh;
	newsh = newsl + (s2-s1) * pic->sh;

	newtl = pic->tl + t1 * pic->th;
	newth = newtl + (t2-t1) * pic->th;

	R2D_Image(x, y, w, h, newsl, newtl, newsh, newth, pic->atlas);
}

void R2D_ImageFlush(void)
{
	BE_DrawMesh_Single(draw_active_shader, &draw_mesh, NULL, draw_active_flags);

	R2D_Flush = NULL;
	draw_active_shader = NULL;
}
//awkward and weird to use
void R2D_Image(float x, float y, float w, float h, float s1, float t1, float s2, float t2, mpic_t *pic)
{
	int i;
	if (!pic)
		return;

	//don't draw pics if they have an image which is still loading.
	for (i = 0; i < pic->numpasses; i++)
	{
		if (pic->passes[i].texgen == T_GEN_SINGLEMAP && pic->passes[i].anim_frames[0] && pic->passes[i].anim_frames[0]->status == TEX_LOADING)
			return;
		if (pic->passes[i].texgen == T_GEN_DIFFUSE && pic->defaulttextures->base && pic->defaulttextures->base->status == TEX_LOADING)
			return;
	}

	if (draw_active_shader != pic || draw_active_flags != r2d_be_flags || R2D_Flush != R2D_ImageFlush || draw_mesh.numvertexes+4 > DRAW_QUADS)
	{
		if (R2D_Flush)
			R2D_Flush();

		draw_active_shader = pic;
		draw_active_flags = r2d_be_flags;
		R2D_Flush = R2D_ImageFlush;

		draw_mesh.numindexes = 0;
		draw_mesh.numvertexes = 0;
	}

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+0],	x, y);
	Vector2Set(draw_mesh_st[draw_mesh.numvertexes+0],	s1, t1);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+0]);

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+1],	x+w, y);
	Vector2Set(draw_mesh_st[draw_mesh.numvertexes+1],	s2, t1);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+1]);

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+2],	x+w, y+h);
	Vector2Set(draw_mesh_st[draw_mesh.numvertexes+2],	s2, t2);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+2]);

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+3],	x, y+h);
	Vector2Set(draw_mesh_st[draw_mesh.numvertexes+3],	s1, t2);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+3]);

	draw_mesh.numvertexes += 4;
	draw_mesh.numindexes += 6;
}
void R2D_Image2dQuad(vec2_t const*points, vec2_t const*texcoords, vec4_t const*rgba, mpic_t *pic)
{
	int i;
	if (!pic)
		return;

	//don't draw pics if they have an image which is still loading.
	for (i = 0; i < pic->numpasses; i++)
	{
		if (pic->passes[i].texgen == T_GEN_SINGLEMAP && pic->passes[i].anim_frames[0] && pic->passes[i].anim_frames[0]->status == TEX_LOADING)
			return;
		if (pic->passes[i].texgen == T_GEN_DIFFUSE && pic->defaulttextures->base && pic->defaulttextures->base->status == TEX_LOADING)
			return;
	}

	if (draw_active_shader != pic || draw_active_flags != r2d_be_flags || R2D_Flush != R2D_ImageFlush || draw_mesh.numvertexes+4 > DRAW_QUADS)
	{
		if (R2D_Flush)
			R2D_Flush();

		draw_active_shader = pic;
		draw_active_flags = r2d_be_flags;
		R2D_Flush = R2D_ImageFlush;

		draw_mesh.numindexes = 0;
		draw_mesh.numvertexes = 0;
	}

	for (i = 0; i < 4; i++)
	{
		Vector2Copy(points[i], draw_mesh_xyz[draw_mesh.numvertexes+i]);
		Vector2Copy(texcoords[i], draw_mesh_st[draw_mesh.numvertexes+i]);
		if (rgba)
			Vector4Copy(rgba[i], draw_mesh_colors[draw_mesh.numvertexes+i]);
		else
			Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+i]);
	}

	draw_mesh.numvertexes += 4;
	draw_mesh.numindexes += 6;
}

/*draws a block of the current colour on the screen*/
void R2D_FillBlock(float x, float y, float w, float h)
{
	mpic_t *pic;
	if (draw_active_colour[3] != 1)
		pic = shader_draw_fill_trans;
	else
		pic = shader_draw_fill;
	if (draw_active_shader != pic || draw_active_flags != r2d_be_flags || R2D_Flush != R2D_ImageFlush || draw_mesh.numvertexes+4 > DRAW_QUADS)
	{
		if (R2D_Flush)
			R2D_Flush();

		draw_active_shader = pic;
		draw_active_flags = r2d_be_flags;
		R2D_Flush = R2D_ImageFlush;

		draw_mesh.numindexes = 0;
		draw_mesh.numvertexes = 0;
	}

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+0], x, y);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+0]);

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+1], x+w, y);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+1]);

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+2], x+w, y+h);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+2]);

	Vector2Set(draw_mesh_xyz[draw_mesh.numvertexes+3], x, y+h);
	Vector4Copy(draw_active_colour, draw_mesh_colors[draw_mesh.numvertexes+3]);

	draw_mesh.numvertexes += 4;
	draw_mesh.numindexes += 6;
}

void R2D_Line(float x1, float y1, float x2, float y2, shader_t *shader)
{
	if (R2D_Flush)
	{
		R2D_Flush();
		R2D_Flush = NULL;
	}

	VectorSet(draw_mesh_xyz[0], x1, y1, 0);
	Vector2Set(draw_mesh_st[0], 0, 0);
	Vector4Copy(draw_active_colour, draw_mesh_colors[0]);

	VectorSet(draw_mesh_xyz[1], x2, y2, 0);
	Vector2Set(draw_mesh_st[1], 1, 0);
	Vector4Copy(draw_active_colour, draw_mesh_colors[1]);

	if (!shader)
	{
		if (draw_active_colour[3] != 1)
			shader = shader_draw_fill_trans;
		else
			shader = shader_draw_fill;
	}

	draw_mesh.numvertexes = 2;
	draw_mesh.numindexes = 2;
	BE_DrawMesh_Single(shader, &draw_mesh, NULL, BEF_LINES);
}

void R2D_ScalePic (float x, float y, float width, float height, mpic_t *pic)
{
	R2D_Image(x, y, width, height, 0, 0, 1, 1, pic);
}

void R2D_SubPic(float x, float y, float width, float height, mpic_t *pic, float srcx, float srcy, float srcwidth, float srcheight)
{
	float newsl, newtl, newsh, newth;

	newsl = (srcx)/(float)srcwidth;
	newsh = newsl + (width)/(float)srcwidth;

	newtl = (srcy)/(float)srcheight;
	newth = newtl + (height)/(float)srcheight;

	R2D_Image(x, y, width, height, newsl, newtl, newsh, newth, pic);
}

void R2D_Letterbox(float sx, float sy, float sw, float sh, mpic_t *pic, float pw, float ph)
{
	float ratiox = (float)pw / sw;
	float ratioy = (float)ph / sh;

	if (pw<=0 || ph<=0)
	{	//no size info...
		R2D_ImageColours(0, 0, 0, 1);
		R2D_FillBlock(sx, sy, sw, sh);
		R2D_ScalePic(sx, sy, 0, 0, pic);	//in case its a videoshader with audio
	}
	else if (ratiox > ratioy)
	{	//x ratio is greatest
		float h = (sw * ph) / pw;
		float p = sh - h;

		//letterbox
		R2D_ImageColours(0, 0, 0, 1);
		R2D_FillBlock(sx, sy, sw, p/2);
		R2D_FillBlock(sx, sy + h + (p/2), sw, p/2);

		R2D_ImageColours(1, 1, 1, 1);
		R2D_ScalePic(sx, sy + p/2, sw, h, pic);
	}
	else
	{	//y ratio is greatest
		float w = (sh * pw) / ph;
		float p = sw - w;

		//sidethingies
		R2D_ImageColours(0, 0, 0, 1);
		R2D_FillBlock(sx, sy, (p/2), sh);
		R2D_FillBlock(sx + w + (p/2), sy, p/2, sh);

		R2D_ImageColours(1, 1, 1, 1);
		R2D_ScalePic(sx + p/2, sy, w, sh, pic);
	}
}

/* this is an ugly special case drawing func that's only used for the player color selection menu */
void R2D_TransPicTranslate (float x, float y, int width, int height, qbyte *pic, unsigned int *palette)
{
	int				v, u;
	unsigned		trans[64*64], *dest;
	qbyte			*src;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &pic[ ((v*height)>>6) *width];
		for (u=0 ; u<64 ; u++)
			dest[u] = palette[src[(u*width)>>6]];
	}

	if (!TEXVALID(translate_texture))
	{
		translate_texture = Image_CreateTexture("***translatedpic***", NULL, IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA);
		translate_shader = R_RegisterShader("translatedpic", SUF_2D,
			"{\n"
				"if $nofixed\n"
					"program default2d\n"
				"endif\n"
				"nomipmaps\n"
				"{\n"
					"map $diffuse\n"
					"blendfunc blend\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
				"}\n"
			"}\n");
		translate_shader->defaulttextures->base = translate_texture;
	}
	/* could avoid reuploading already translated textures but this func really isn't used enough anyway */
	Image_Upload(translate_texture, TF_RGBA32, trans, NULL, 64, 64, 1, IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA);
	R2D_ScalePic(x, y, width, height, translate_shader);
}

/*
================
Draw_ConsoleBackground

================
*/
void R2D_ConsoleBackground (int firstline, int lastline, qboolean forceopaque)
{
	float a;
	int w, h;

	w = vid.width;
	h = vid.height;

	if (forceopaque)
	{
		a = 1; // console background is necessary
	}
	else
	{
		if (!scr_conalpha.value)
			return;

		a = scr_conalpha.value;
	}

	if (R_GetShaderSizes(conback, NULL, NULL, false) <= 0)
	{
		R2D_ImageColours(0, 0, 0, a);
		R2D_FillBlock(0, lastline-(int)vid.height, w, h);
		R2D_ImageColours(1, 1, 1, 1);
	}
	else if (a >= 1)
	{
		R2D_ImageColours(1, 1, 1, 1);
		R2D_ScalePic(0, lastline-(int)vid.height, w, h, conback);
	}
	else
	{
		R2D_ImageColours(1, 1, 1, a);
		R2D_ScalePic (0, lastline - (int)vid.height, w, h, conback);
		R2D_ImageColours(1, 1, 1, 1);
	}
}

void R2D_EditorBackground (void)
{
	R2D_ImageColours(0, 0, 0, 1);
	R2D_FillBlock(0, 0, vid.width, vid.height);
//	R2D_ScalePic(0, 0, vid.width, vid.height, conback);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void R2D_TileClear (float x, float y, float w, float h)
{
	float newsl, newsh, newtl, newth;
	newsl = (x)/(float)64;
	newsh = newsl + (w)/(float)64;

	newtl = (y)/(float)64;
	newth = newtl + (h)/(float)64;

	R2D_ImageColours(1,1,1,1);

	R2D_Image(x, y, w, h, newsl, newtl, newsh, newth, draw_backtile);
}

void QDECL R2D_Conback_Callback(struct cvar_s *var, char *oldvalue)
{
	if (qrenderer == QR_NONE || !strcmp(var->string, "none"))
	{
		conback = NULL;
		return;
	}

	if (*var->string)
		conback = R_RegisterPic(var->string, NULL);
#ifdef HAVE_LEGACY
	else if (Cvar_FindVar("scr_conalphafactor"))
	{	//dp bullshit
		conback = R_RegisterShader("gfx/conback", SUF_2D,
			"{\n"
				"nomipmaps\n"
				"{\n"
					"map gfx/conback\n"
					"rgbgen const $scr_conbrightness\n"
					"alphagen const $scr_conalphafactor\n"
					"tcmod scroll $scr_conscroll_x $scr_conscroll_y\n"
					"blendfunc blend\n"
				"}\n"
				"{\n"
					"map gfx/conback2\n"
					"rgbgen const $scr_conbrightness\n"
					"alphagen const $scr_conalpha2factor\n"
					"tcmod scroll $scr_conscroll2_x $scr_conscroll2_y\n"
					"blendfunc blend\n"
				"}\n"
				"{\n"
					"map gfx/conback3\n"
					"rgbgen const $scr_conbrightness\n"
					"alphagen const $scr_conalpha3factor\n"
					"tcmod scroll $scr_conscroll3_x $scr_conscroll3_y\n"
					"blendfunc blend\n"
				"}\n"
			"}\n");
	}
#endif

	if (!R_GetShaderSizes(conback, NULL, NULL, true))
	{
		conback = R_RegisterCustom(NULL, "console", SUF_2D, NULL, NULL);	//quake3
		if (!R_GetShaderSizes(conback, NULL, NULL, true))
		{
#ifdef HEXEN2
			if (M_GameType() == MGT_HEXEN2)
				conback = R_RegisterPic("gfx/menu/conback.lmp", NULL);
			else
#endif
				if (M_GameType() == MGT_QUAKE2)
				conback = R_RegisterPic("pics/conback.pcx", NULL);
			else
				conback = R_RegisterPic("gfx/conback.lmp", NULL);
		}
	}
}

#ifdef AVAIL_FREETYPE
#if defined(LIBFONTCONFIG_STATIC)
#include <fontconfig/fontconfig.h>
static int QDECL SortCompareFonts(const void *av, const void *bv)
{	//qsort compare
	const FcPattern *af = *(FcPattern *const*const)av, *bf = *(FcPattern *const*const)bv;
	FcChar8 *as, *bs;
	int r = 0;
	if (FcPatternGetString(af, FC_FAMILY, 0, &as) == FcResultMatch && FcPatternGetString(bf, FC_FAMILY, 0, &bs) == FcResultMatch)
	{
		r = strcmp(as, bs);
		if (!r && FcPatternGetString(af, FC_STYLE, 0, &as) == FcResultMatch && FcPatternGetString(bf, FC_STYLE, 0, &bs) == FcResultMatch)
			r = strcmp(as, bs);
	}
	return r;
}
#elif defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT) && !defined(_XBOX)
#include <windows.h>
qboolean R2D_Font_WasAdded(char *buffer, char *fontfilename)
{
	char *match;
	if (!fontfilename)
		return true;
	match = strstr(buffer, fontfilename);
	if (!match)
		return false;
	if (!(match == buffer || match[-1] == ','))
		return false;
	match += strlen(fontfilename);
	if (*match && *match != ',')
		return false;
	return true;
}
extern qboolean	WinNT;
void R2D_Font_AddFontLink(char *buffer, int buffersize, char *fontname)
{
	char link[1024];
	char *res, *comma, *othercomma, *nl;
	if (fontname)
	if (MyRegGetStringValueMultiSz(HKEY_LOCAL_MACHINE, WinNT?"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink":"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\FontLink\\SystemLink", fontname, link, sizeof(link)))
	{
		res = nl = link;
		while (*nl)
		{
			nl += strlen(nl);
			nl++;
			comma = strchr(res, ',');
			if (comma)
			{
				*comma++ = 0;
				othercomma = strchr(comma, ',');
				if (othercomma)
					*othercomma = 0;
			}
			else
				comma = "";
			if (!R2D_Font_WasAdded(buffer, res))
			{
				Q_strncatz(buffer, ",", buffersize);
				Q_strncatz(buffer, res, buffersize);
				R2D_Font_AddFontLink(buffer, buffersize, comma);
			}
			res = nl;
		}
	}
}
#else
int R2D_Font_ListSystemFonts(const char *fname, qofs_t fsize, time_t modtime, void *ctx, searchpathfuncs_t *spath)
{
	char tmp[MAX_QPATH];
	COM_StripExtension(fname, tmp, sizeof(tmp));
	Con_Printf("^[/gl_font %s^]\n", tmp);
	return true;
}
#endif
#endif
void R2D_Font_Changed(void)
{
	float tsize;
	const char *con_font_name = con_textfont.string;
	unsigned int flags;
	if (!con_textsize.modified)
		return;
	if (!*con_font_name)
		con_font_name = gl_font.string;
	con_textsize.modified = false;

	if (con_textsize.value < 0)
		tsize = (-con_textsize.value * vid.height) / vid.pixelheight;	//size defined in physical pixels
	else
		tsize = con_textsize.value;	//size defined in virtual pixels.
	if (!tsize)
		tsize = 8;

	if (font_console == font_default)
		font_console = NULL;
	if (font_console)
		Font_Free(font_console);
	font_console = NULL;
	if (font_default)
		Font_Free(font_default);
	font_default = NULL;

	if (font_tiny)
		Font_Free(font_tiny);
	font_tiny = NULL; 

#if defined(MENU_DAT) || defined(CSQC_DAT)
	PR_ReloadFonts(true);
#endif

	if (qrenderer == QR_NONE)
		return;

	flags = 0;
	if (r_font_postprocess_mono.ival)
		flags |= FONT_MONO;

	if (!strcmp(gl_font.string, "?"))
	{
#ifndef AVAIL_FREETYPE
		Cvar_Set(&gl_font, "");
#elif defined(LIBFONTCONFIG_STATIC)
		Cvar_Set(&gl_font, "");
		{
			FcConfig *config = FcInitLoadConfigAndFonts();
			FcPattern *pat = FcPatternCreate();
			FcObjectSet *os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, (char *) 0);
			FcFontSet *fs = FcFontList(config, pat, os);

			if (fs)
			{
				int i;
				FcChar8 *oldfam = NULL;
				FcPattern **fonts = BZ_Malloc(sizeof(*fonts)*fs->nfont);
				memcpy(fonts, fs->fonts, sizeof(*fonts)*fs->nfont);
				qsort(fonts, fs->nfont, sizeof(*fonts), SortCompareFonts);
				for (i=0; fs && i < fs->nfont; i++)
				{
					FcPattern *font = fonts[i];
					FcChar8 *style, *family;
					if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch && FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
					{
						if (!oldfam || strcmp(oldfam, family))
						{
							if (oldfam)
								Con_Printf("\n");
							oldfam = family;
							Con_Printf("^["S_COLOR_WHITE"%s\\type\\/gl_font %s^]: ", family, family);
						}
						Con_Printf(" \t^[%s\\type\\/gl_font %s?style=%s^]", style, family, style);
					}
				}
				if (oldfam)
					Con_Printf("\n");
				BZ_Free(fonts);
				FcFontSetDestroy(fs);
			}
			FcObjectSetDestroy(os);
			FcPatternDestroy(pat);
		}
#elif defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT) && !defined(_XBOX)
		BOOL (APIENTRY *pChooseFontW)(LPCHOOSEFONTW) = NULL;
		dllfunction_t funcs[] =
		{
			{(void*)&pChooseFontW, "ChooseFontW"},
			{NULL}
		};
		LOGFONTW lf = {0};
		CHOOSEFONTW cf = {sizeof(cf)};
		extern HWND	mainwindow;
		font_default = Font_LoadFont("", 8, 1, r_font_postprocess_outline.ival, flags);
		if (tsize != 8)
			font_console = Font_LoadFont("", tsize, 1, r_font_postprocess_outline.ival, flags);
		if (!font_console)
			font_console = font_default;

		cf.hwndOwner = mainwindow;
		cf.iPointSize = (8 * vid.rotpixelheight)/vid.height;
		cf.Flags = CF_FORCEFONTEXIST | CF_TTONLY;
		cf.lpLogFont = &lf;

		Sys_LoadLibrary("comdlg32.dll", funcs);

		if (pChooseFontW && pChooseFontW(&cf))
		{
			char fname[MAX_OSPATH*8];
			char lfFaceName[MAX_OSPATH];
			char *keyname;
			narrowen(lfFaceName, sizeof(lfFaceName), lf.lfFaceName);
			*fname = 0;
			//FIXME: should enumerate and split & and ignore sizes and () crap.
			if (!*fname)
			{
				keyname = va("%s%s%s (TrueType)", lfFaceName, lf.lfWeight>=FW_BOLD?" Bold":"", lf.lfItalic?" Italic":"");
				if (!MyRegGetStringValue(HKEY_LOCAL_MACHINE, WinNT?"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts":"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts", keyname, fname, sizeof(fname)))
					*fname = 0;
			}
			if (!*fname)
			{
				keyname = va("%s (OpenType)", lfFaceName);
				if (!MyRegGetStringValue(HKEY_LOCAL_MACHINE, WinNT?"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts":"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts", keyname, fname, sizeof(fname)))
					*fname = 0;
			}

			R2D_Font_AddFontLink(fname, sizeof(fname), lfFaceName);
			Cvar_Set(&gl_font, fname);
		}
		return;
#else
		Sys_EnumerateFiles("/usr/share/fonts/truetype/", "*/*.ttf", R2D_Font_ListSystemFonts, NULL, NULL);
		COM_EnumerateFiles("*.ttf", R2D_Font_ListSystemFonts, NULL);
		Cvar_Set(&gl_font, "");
#endif
	}

	if (COM_FDepthFile("fonts/qfont.kfont", true) <= COM_FDepthFile("gfx/mainmenu.lmp", true))
		font_menu = Font_LoadFont("qfont", 20, 1, r_font_postprocess_outline.ival, flags);
	else
		font_menu = NULL;

	font_default = Font_LoadFont(gl_font.string, 8, 1, r_font_postprocess_outline.ival, flags);
	if (!font_default && *gl_font.string)
		font_default = Font_LoadFont("", 8, 1, r_font_postprocess_outline.ival, flags);

	if (tsize != 8 || strcmp(gl_font.string, con_font_name))
	{
		font_console = Font_LoadFont(con_font_name, tsize, 1, r_font_postprocess_outline.ival, flags);
		if (!font_console)
			font_console = Font_LoadFont("", tsize, 1, r_font_postprocess_outline.ival, flags);
	}
	if (!font_console)
		font_console = font_default;

	//these are here instead of R2D_Console_Resize because this is guarenteed to happen in a sane place, while R2D_Console_Resize can happen during waits.
#ifdef MENU_NATIVECODE
	if (mn_entry)
		mn_entry->Init(MI_RESOLUTION, vid.width, vid.height, vid.rotpixelwidth, vid.rotpixelheight);
#endif
#ifdef PLUGINS
	Plug_ResChanged(false);
#endif
}

static void QDECL R2D_Font_Callback(struct cvar_s *var, char *oldvalue)
{
	con_textsize.modified = true;
}

// console size manipulation callbacks
void R2D_Console_Resize(void)
{
	extern cvar_t gl_font;
	extern cvar_t vid_conwidth, vid_conheight;
	int cwidth, cheight;
	float xratio;
	float yratio=0;
	float ang, rad;
	extern cvar_t gl_screenangle;

	int fbwidth = vid.pixelwidth;
	int fbheight = vid.pixelheight;

	if (vid.framebuffer)
	{
		fbwidth = vid.framebuffer->width;
		fbheight = vid.framebuffer->height;
	}

	ang = (gl_screenangle.value>0?(gl_screenangle.value+45):(gl_screenangle.value-45))/90;
	ang = (int)ang * 90;
	if (ang)
	{
		rad = (ang * M_PI) / 180;
		vid.rotpixelwidth = fabs(cos(rad)) * (fbwidth) + fabs(sin(rad)) * (fbheight);
		vid.rotpixelheight = fabs(sin(rad)) * (fbwidth) + fabs(cos(rad)) * (fbheight);
	}
	else
	{
		vid.rotpixelwidth = fbwidth;
		vid.rotpixelheight = fbheight;
	}

	cwidth = vid_conwidth.value;
	cheight = vid_conheight.value;

	xratio = vid_conautoscale.value;
	if (xratio > 0)
	{
		char *s = strchr(vid_conautoscale.string, ' ');
		if (s)
			yratio = atof(s + 1);

		if (yratio <= 0)
			yratio = xratio;

		xratio = 1 / xratio;
		yratio = 1 / yratio;

		//autoscale overrides conwidth/height (without actually changing them)
		cwidth = vid.rotpixelwidth;
		cheight = vid.rotpixelheight;
	}
	else
	{
		xratio = 1;
		yratio = 1;
	}

	if (!cwidth && !cheight)
	{
		int i = 0, nh;
		int biggest = 0;
		//find the biggest artwork size that won't get minified
		for (i = 0; i < countof(vid_baseheight.vec4); i++)
			if (biggest < vid_baseheight.vec4[i] && vid_baseheight.vec4[i] < vid.rotpixelheight && vid_baseheight.vec4[i] >= vid_minsize.vec4[1])
				biggest = vid_baseheight.vec4[i];
		if (biggest)
		{
			nh = vid.rotpixelheight;
			while ((nh>>1) >= biggest)
				nh >>= 1;
			cheight = nh;
		}
		else
		{
			if (vid.dpi_y)
				cheight = (480 * 96) / vid.dpi_y;
			else
				cheight = 480;
		}
	}
	if (cheight && !cwidth && vid.rotpixelheight)
		cwidth = (cheight*vid.rotpixelwidth)/vid.rotpixelheight;
	if (cwidth && !cheight && vid.rotpixelwidth)
		cheight = (cwidth*vid.rotpixelheight)/vid.rotpixelwidth;

	if (!cwidth)
		cwidth = vid.rotpixelwidth;
	if (!cheight)
		cheight = vid.rotpixelheight;

	cwidth*=xratio;
	cheight*=yratio;

	//never go lower than the mod's minimum, UI elements would end up off-screen.
	if (cwidth < vid_minsize.vec4[0])
		cwidth = vid_minsize.vec4[0];
	if (cheight < vid_minsize.vec4[1])
		cheight = vid_minsize.vec4[1];
	//the engine has its own requirements too, sorry, though its unlikely for mods go lower.
	if (cwidth < 320)
		cwidth = 320;
	if (cheight < 200)
		cheight = 200;

	vid.width = cwidth;
	vid.height = cheight;

	Cvar_ForceCallback(&gl_font);
}

static void QDECL R2D_Conheight_Callback(struct cvar_s *var, char *oldvalue)
{
	if (var->value > 1536)	//anything higher is unreadable.
	{
		Cvar_ForceSet(var, "1536");
		return;
	}
	if (var->value < 200 && var->value)	//lower would be wrong
	{
		Cvar_ForceSet(var, "200");
		return;
	}

	R2D_Console_Resize();
}

static void QDECL R2D_Conwidth_Callback(struct cvar_s *var, char *oldvalue)
{
	//let let the user be too crazy
	if (var->value > 2048)	//anything higher is unreadable.
	{
		Cvar_ForceSet(var, "2048");
		return;
	}
	if (var->value < 320 && var->value)	//lower would be wrong
	{
		Cvar_ForceSet(var, "320");
		return;
	}

	R2D_Console_Resize();
}

static void QDECL R2D_Conautoscale_Callback(struct cvar_s *var, char *oldvalue)
{
	R2D_Console_Resize();
}

static void QDECL R2D_ScreenAngle_Callback(struct cvar_s *var, char *oldvalue)
{
	R2D_Console_Resize();
}


/*
============
R_PolyBlend
============
*/
//bright flashes and stuff, game only, doesn't touch sbar
void R2D_PolyBlend (void)
{
	float bordersize = gl_cshiftborder.value;

	if (r_refdef.flags & RDF_NOWORLDMODEL)
		return;


	if (r_refdef.playerview->bordertint[3])
	{
		vec2_t points[4];
		vec2_t tcoords[4];
		vec4_t colours[4];
		int i;
		if (!bordersize)
			bordersize = 64;
		bordersize = min(bordersize, min(r_refdef.vrect.width, r_refdef.vrect.height)/2);

		for (i = 0; i < 4; i++)
		{
			Vector4Copy(r_refdef.playerview->bordertint, colours[0]);
			Vector4Copy(r_refdef.playerview->bordertint, colours[1]);
			Vector4Copy(r_refdef.playerview->bordertint, colours[2]);
			Vector4Copy(r_refdef.playerview->bordertint, colours[3]);
			switch(i)
			{
			case 0:	//top
				Vector2Set(points[0], r_refdef.vrect.x,									r_refdef.vrect.y);
				Vector2Set(points[1], r_refdef.vrect.x+r_refdef.vrect.width,			r_refdef.vrect.y);
				Vector2Set(points[2], r_refdef.vrect.x+r_refdef.vrect.width-bordersize,	r_refdef.vrect.y+bordersize);
				Vector2Set(points[3], r_refdef.vrect.x+bordersize,						r_refdef.vrect.y+bordersize);

				colours[2][3] = colours[3][3] = 0;
				break;
			case 1:	//bottom
				Vector2Set(points[0], r_refdef.vrect.x+bordersize,						r_refdef.vrect.y+r_refdef.vrect.height-bordersize);
				Vector2Set(points[1], r_refdef.vrect.x+r_refdef.vrect.width-bordersize,	r_refdef.vrect.y+r_refdef.vrect.height-bordersize);
				Vector2Set(points[2], r_refdef.vrect.x+r_refdef.vrect.width,			r_refdef.vrect.y+r_refdef.vrect.height);
				Vector2Set(points[3], r_refdef.vrect.x,									r_refdef.vrect.y+r_refdef.vrect.height);

				colours[0][3] = colours[1][3] = 0;
				break;
			case 2:	//left
				Vector2Set(points[0], r_refdef.vrect.x,									r_refdef.vrect.y);
				Vector2Set(points[1], r_refdef.vrect.x+bordersize,						r_refdef.vrect.y+bordersize);
				Vector2Set(points[2], r_refdef.vrect.x+bordersize,						r_refdef.vrect.y+r_refdef.vrect.height-bordersize);
				Vector2Set(points[3], r_refdef.vrect.x,									r_refdef.vrect.y+r_refdef.vrect.height);

				colours[1][3] = colours[2][3] = 0;
				break;
			case 3:	//right
				Vector2Set(points[0], r_refdef.vrect.x+r_refdef.vrect.width-bordersize,	r_refdef.vrect.y+bordersize);
				Vector2Set(points[1], r_refdef.vrect.x+r_refdef.vrect.width,			r_refdef.vrect.y);
				Vector2Set(points[2], r_refdef.vrect.x+r_refdef.vrect.width,			r_refdef.vrect.y+r_refdef.vrect.height);
				Vector2Set(points[3], r_refdef.vrect.x+r_refdef.vrect.width-bordersize,	r_refdef.vrect.y+r_refdef.vrect.height-bordersize);
				colours[0][3] = colours[3][3] = 0;
				break;
			}

			Vector2Set(tcoords[0], points[0][0]/64, points[0][1]/64);
			Vector2Set(tcoords[1], points[1][0]/64, points[1][1]/64);
			Vector2Set(tcoords[2], points[2][0]/64, points[2][1]/64);
			Vector2Set(tcoords[3], points[3][0]/64, points[3][1]/64);

			R2D_Image2dQuad((const vec2_t*)points, (const vec2_t*)tcoords, (const vec4_t*)colours, shader_polyblend);
		}
	}

	if (r_refdef.playerview->screentint[3])
	{
		R2D_ImageColours (SRGBA(r_refdef.playerview->screentint[0], r_refdef.playerview->screentint[1], r_refdef.playerview->screentint[2], r_refdef.playerview->screentint[3]));
		R2D_ScalePic(r_refdef.vrect.x, r_refdef.vrect.y, r_refdef.vrect.width, r_refdef.vrect.height, shader_polyblend);
		R2D_ImageColours (1, 1, 1, 1);
	}
}

//for lack of hardware gamma
void R2D_BrightenScreen (void)
{
	float f;

	RSpeedMark();

	if (fabs(v_contrast.value - 1.0) < 0.05 && fabs(v_contrastboost.value - 1.0) < 0.05 && fabs(v_brightness.value - 0) < 0.05 && fabs(v_gamma.value - 1) < 0.05)
		return;

	//don't go crazy with brightness. that makes it unusable and is thus unsafe - and worse, lots of people assume its based around 1 (like gamma and contrast are). cap to 0.5
	if (v_brightness.value > 0.5)
		v_brightness.value = 0.5;
	if (v_contrast.value < 0.5)
		v_contrast.value = 0.5;

	if (r2d_canhwgamma || vid_hardwaregamma.ival == 4)
		return;

	TRACE(("R2D_BrightenScreen: brightening\n"));
	if ((v_gamma.value != 1 || v_contrast.value > 3 || v_contrastboost.value != 1) && shader_gammacb->prog)
	{
		//this should really be done properly, with render-to-texture
		R2D_ImageColours (v_gammainverted.ival?v_gamma.value:(1/v_gamma.value), v_contrast.value, v_brightness.value, v_contrastboost.value);
		R2D_Image(0, 0, vid.width, vid.height, 0, 0, 1, 1, shader_gammacb);
	}
	else
	{
		f = v_contrast.value;
		f = min (f, 3);

		while (f > 1)
		{
			if (f >= 2)
			{
				R2D_ImageColours (1, 1, 1, 1);
				f *= 0.5;
			}
			else
			{
				R2D_ImageColours (f - 1, f - 1, f - 1, 1);
				f = 1;
			}
			R2D_ScalePic(0, 0, vid.width, vid.height, shader_contrastup);
		}
		if (f < 1)
		{
			R2D_ImageColours (f, f, f, 1);
			R2D_ScalePic(0, 0, vid.width, vid.height, shader_contrastdown);
		}

		if (v_brightness.value)
		{
			R2D_ImageColours (v_brightness.value, v_brightness.value, v_brightness.value, 1);
			R2D_ScalePic(0, 0, vid.width, vid.height, shader_brightness);
		}
	}
	R2D_ImageColours (1, 1, 1, 1);

	/*make sure the hud is redrawn after if needed*/
	Sbar_Changed();

	RSpeedEnd(RSPEED_PALETTEFLASHES);
}

//for menus
void R2D_FadeScreen (void)
{
	R2D_ScalePic(0, 0, vid.width, vid.height, shader_menutint);

	Sbar_Changed();
}

qboolean R2D_DrawLevelshot(void)
{
	extern char levelshotname[];
	extern cvar_t scr_loadingscreen_aspect;
	if (*levelshotname)
	{
		shader_t *pic = R2D_SafeCachePic (levelshotname);
		int w,h;
		if (!R_GetShaderSizes(pic, &w, &h, true))
		{
#ifdef Q3CLIENT
			pic = R2D_SafeCachePic("menu/art/unkownmap");
			if (!R_GetShaderSizes(pic, &w, &h, true))
#endif
				w = h = 1;
		}
		switch(scr_loadingscreen_aspect.ival)
		{
		case 0:	//use the source image's aspect
			break;
		case 1:	//q3 assumed 4:3 resolutions, with power-of-two images. lame, but lets retain the aspect
			w = 640;
			h = 480;
			break;
		case 2:	//just ignore aspect entirely and stretch the thing in hideous ways
			w = vid.width;
			h = vid.height;
			break;
		}
		R2D_Letterbox(0, 0, vid.width, vid.height, pic, w, h);

		//q3's noise.
		pic = R2D_SafeCachePic("levelShotDetail");
		if (R_GetShaderSizes(pic, &w, &h, true))
			R2D_Image(0, 0, vid.width, vid.height, 0, 0, vid.width/256, vid.height/256, pic);
		return true;
	}
	return false;
}

//crosshairs
#define CS_HEIGHT 8
#define CS_WIDTH 8
unsigned char crosshair_pixels[] =
{
	// 2 + (spaced)
	0x08,
	0x00,
	0x08,
	0x55,
	0x08,
	0x00,
	0x08,
	0x00,
	// 3 +
	0x00,
	0x08,
	0x08,
	0x36,
	0x08,
	0x08,
	0x00,
	0x00,
	// 4 X
	0x00,
	0x22,
	0x14,
	0x00,
	0x14,
	0x22,
	0x00,
	0x00,
	// 5 X (spaced)
	0x41,
	0x00,
	0x14,
	0x00,
	0x14,
	0x00,
	0x41,
	0x00,
	// 6 diamond (unconnected)
	0x00,
	0x14,
	0x22,
	0x00,
	0x22,
	0x14,
	0x00,
	0x00,
	// 7 diamond
	0x00,
	0x08,
	0x14,
	0x22,
	0x14,
	0x08,
	0x00,
	0x00,
	// 8 four points
	0x00,
	0x08,
	0x00,
	0x22,
	0x00,
	0x08,
	0x00,
	0x00,
	// 9 three points
	0x00,
	0x00,
	0x08,
	0x00,
	0x22,
	0x00,
	0x00,
	0x00,
	// 10
	0x08,
	0x2a,
	0x00,
	0x63,
	0x00,
	0x2a,
	0x08,
	0x00,
	// 11
	0x49,
	0x2a,
	0x00,
	0x63,
	0x00,
	0x2a,
	0x49,
	0x00,
	// 12 horizontal line
	0x00,
	0x00,
	0x00,
	0x77,
	0x00,
	0x00,
	0x00,
	0x00,
	// 13 vertical line
	0x08,
	0x08,
	0x08,
	0x00,
	0x08,
	0x08,
	0x08,
	0x00,
	// 14 larger +
	0x08,
	0x08,
	0x08,
	0x77,
	0x08,
	0x08,
	0x08,
	0x00,
	// 15 angle
	0x00,
	0x00,
	0x00,
	0x70,
	0x08,
	0x08,
	0x08,
	0x00,
	// 16 dot
	0x00,
	0x00,
	0x00,
	0x08,
	0x00,
	0x00,
	0x00,
	0x00,
	// 17 weird angle thing
	0x00,
	0x00,
	0x00,
	0x38,
	0x48,
	0x08,
	0x10,
	0x00,
	// 18 circle w/ dot
	0x00,
	0x00,
	0x00,
	0x6b,
	0x41,
	0x63,
	0x3e,
	0x08,
	// 19 tripoint
	0x08,
	0x08,
	0x08,
	0x00,
	0x14,
	0x22,
	0x41,
	0x00,
};

void R2D_Crosshair_Update(void)
{
	int crossdata[CS_WIDTH*CS_HEIGHT];
	int c;
	int w, h;
	unsigned char *x;

	c = crosshair.ival;
	if (!crosshairimage.string)
		return;
	else if (crosshairimage.string[0] && c == 1)
	{
		shader_crosshair->defaulttextures->base = R_LoadHiResTexture (crosshairimage.string, "crosshairs", IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA);
		if (TEXVALID(shader_crosshair->defaulttextures->base))
			return;
	}
	else if (c <= 1)
		return;

	c -= 2;
	c = c % (sizeof(crosshair_pixels) / (CS_HEIGHT*sizeof(*crosshair_pixels)));

	if (!TEXVALID(ch_int_texture))
		ch_int_texture = Image_CreateTexture("***crosshair***", NULL, IF_UIPIC|IF_NOMIPMAP);
	shader_crosshair->defaulttextures->base = ch_int_texture;

	Q_memset(crossdata, 0, sizeof(crossdata));

	x = crosshair_pixels + (CS_HEIGHT * c);
	for (h = 0; h < CS_HEIGHT; h++)
	{
		int pix = x[h];
		for (w = 0; w < CS_WIDTH; w++)
		{
			if (pix & 0x1)
				crossdata[CS_WIDTH * h + w] = 0xffffffff;
			pix >>= 1;
		}
	}

	Image_Upload(ch_int_texture, TF_RGBA32, crossdata, NULL, CS_WIDTH, CS_HEIGHT, 1, IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA);

}

static void QDECL R2D_CrosshairImage_Callback(struct cvar_s *var, char *oldvalue)
{
	R2D_Crosshair_Update();
}

static void QDECL R2D_Crosshair_Callback(struct cvar_s *var, char *oldvalue)
{
	R2D_Crosshair_Update();
}

static void QDECL R2D_CrosshairColor_Callback(struct cvar_s *var, char *oldvalue)
{
	SCR_StringToRGB(var->string, ch_color, 255);

	ch_color[0] = bound(0, ch_color[0], 1);
	ch_color[1] = bound(0, ch_color[1], 1);
	ch_color[2] = bound(0, ch_color[2], 1);
}

void R2D_DrawCrosshair(void)
{
	int x, y;
	int sc;
	float sx, sy, sizex, sizey;

	float size;

	if (crosshair.ival < 1)
		return;

	// old style
	if (crosshair.ival == 1 && !crosshairimage.string[0])
	{
		// adjust console crosshair scale to match default
		size = crosshairsize.value;
		if (size == 0)
			size = 8;
		else if (size < 0)
			size = -size;
		for (sc = 0; sc < cl.splitclients; sc++)
		{
			SCR_CrosshairPosition(&cl.playerview[sc], &sx, &sy);
			Font_BeginScaledString(font_default, sx, sy, size, size, &sx, &sy);
			sx -= Font_CharScaleWidth(CON_WHITEMASK, '+' | 0xe000)/2;
			sy -= Font_CharScaleHeight()/2;
			R2D_ImageColours(ch_color[0], ch_color[1], ch_color[2], crosshairalpha.value);
			Font_DrawScaleChar(sx, sy, CON_WHITEMASK, '+' | 0xe000);
			Font_EndString(font_default);
		}
		R2D_ImageColours(1,1,1,1);
		return;
	}

	size = crosshairsize.value;

	if (size < 0)
	{
		size = -size;
		sizex = size;
		sizey = size;
	}
	else
	{
		sizex = (size*vid.rotpixelwidth) / (float)vid.width;
		sizey = (size*vid.rotpixelheight) / (float)vid.height;
	}

	sizex = (int)sizex;
	sizex = ((sizex)*(int)vid.width) / (float)vid.rotpixelwidth;

	sizey = (int)sizey;
	sizey = ((sizey)*(int)vid.height) / (float)vid.rotpixelheight;

	R2D_ImageColours(ch_color[0], ch_color[1], ch_color[2], crosshairalpha.value);
	for (sc = 0; sc < cl.splitclients; sc++)
	{
		SCR_CrosshairPosition(&cl.playerview[sc], &sx, &sy);

		//translate to pixel coord, for rounding
		x = ((sx-sizex+(sizex/CS_WIDTH))*vid.rotpixelwidth) / (float)vid.width;
		y = ((sy-sizey+(sizey/CS_HEIGHT))*vid.rotpixelheight) / (float)vid.height;

		//translate to screen coords
		sx = ((x)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((y)*(int)vid.height) / (float)vid.rotpixelheight;

		R2D_Image(sx, sy, sizex*2, sizey*2, 0, 0, 1, 1, shader_crosshair);
	}
	R2D_ImageColours(1, 1, 1, 1);
}

static texid_t internalrt;
//resize a texture for a render target and specify the format of it.
//pass TF_INVALID and sizes=0 to get without configuring (shaders that hardcode an $rt1 etc).
texid_t R2D_RT_Configure(const char *id, int width, int height, uploadfmt_t rtfmt, unsigned int imageflags)
{
	texid_t tid;
	if (!strcmp(id, "-"))
	{
		internalrt = tid = Image_CreateTexture("", NULL, imageflags);
	}
	else
	{
		tid = Image_FindTexture(id, NULL, imageflags);
		if (!TEXVALID(tid))
			tid = Image_CreateTexture(id, NULL, imageflags);
	}

	if (rtfmt)
	{
		if (tid->flags != ((tid->flags & ~(IF_NEAREST|IF_LINEAR)) | (imageflags & (IF_NEAREST|IF_LINEAR))))
		{
			tid->flags = ((tid->flags & ~(IF_NEAREST|IF_LINEAR)) | (imageflags & (IF_NEAREST|IF_LINEAR)));
			tid->width = -1;
		}
		Image_Upload(tid, rtfmt, NULL, NULL, width, height, 1, imageflags);
		tid->width = width;
		tid->height = height;
	}
	return tid;
}
texid_t R2D_RT_GetTexture(const char *id, unsigned int *width, unsigned int *height)
{
	texid_t tid;
	if (!strcmp(id, "-"))
	{
		tid = internalrt;
//		internalrt = r_nulltex;
	}
	else
		tid = Image_FindTexture(id, NULL, RT_IMAGEFLAGS);

	if (tid)
	{
		*width = tid->width;
		*height = tid->height;
	}
	else
	{
		*width = 0;
		*height = 0;
	}
	return tid;
}

#endif

