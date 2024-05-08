/*
Copyright (C) 2000-2020 DarkPlaces contributors

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


#include "quakedef.h"
#include "image.h"
#include "jpeg.h"
#include "image_png.h"

cvar_t gl_max_size = {CF_CLIENT | CF_ARCHIVE, "gl_max_size", "2048", "maximum allowed texture size, can be used to reduce video memory usage, limited by hardware capabilities (typically 2048, 4096, or 8192)"};
cvar_t gl_max_lightmapsize = {CF_CLIENT | CF_ARCHIVE, "gl_max_lightmapsize", "512", "maximum allowed texture size for lightmap textures, use larger values to improve rendering speed, as long as there is enough video memory available (setting it too high for the hardware will cause very bad performance)"};
cvar_t gl_picmip = {CF_CLIENT | CF_ARCHIVE, "gl_picmip", "0", "reduces resolution of textures by powers of 2, for example 1 will halve width/height, reducing texture memory usage by 75%"};
cvar_t gl_picmip_world = {CF_CLIENT | CF_ARCHIVE, "gl_picmip_world", "0", "extra picmip level for world textures (may be negative, which will then reduce gl_picmip for these)"};
cvar_t r_picmipworld = {CF_CLIENT | CF_ARCHIVE, "r_picmipworld", "1", "whether gl_picmip shall apply to world textures too (setting this to 0 is a shorthand for gl_picmip_world -9999999)"};
cvar_t gl_picmip_sprites = {CF_CLIENT | CF_ARCHIVE, "gl_picmip_sprites", "0", "extra picmip level for sprite textures (may be negative, which will then reduce gl_picmip for these)"};
cvar_t r_picmipsprites = {CF_CLIENT | CF_ARCHIVE, "r_picmipsprites", "1", "make gl_picmip affect sprites too (saves some graphics memory in sprite heavy games) (setting this to 0 is a shorthand for gl_picmip_sprites -9999999)"};
cvar_t gl_picmip_other = {CF_CLIENT | CF_ARCHIVE, "gl_picmip_other", "0", "extra picmip level for other textures (may be negative, which will then reduce gl_picmip for these)"};
cvar_t r_lerpimages = {CF_CLIENT | CF_ARCHIVE, "r_lerpimages", "1", "bilinear filters images when scaling them up to power of 2 size (mode 1), looks better than glquake (mode 0)"};
cvar_t gl_texture_anisotropy = {CF_CLIENT | CF_ARCHIVE, "gl_texture_anisotropy", "1", "anisotropic filtering quality (if supported by hardware), 1 sample (no anisotropy) and 8 sample (8 tap anisotropy) are recommended values"};
cvar_t gl_texturecompression = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression", "0", "whether to compress textures, a value of 0 disables compression (even if the individual cvars are 1), 1 enables fast (low quality) compression at startup, 2 enables slow (high quality) compression at startup"};
cvar_t gl_texturecompression_color = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_color", "1", "whether to compress colormap (diffuse) textures"};
cvar_t gl_texturecompression_normal = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_normal", "0", "whether to compress normalmap (normalmap) textures"};
cvar_t gl_texturecompression_gloss = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_gloss", "1", "whether to compress glossmap (specular) textures"};
cvar_t gl_texturecompression_glow = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_glow", "1", "whether to compress glowmap (luma) textures"};
cvar_t gl_texturecompression_2d = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_2d", "0", "whether to compress 2d (hud/menu) textures other than the font"};
cvar_t gl_texturecompression_q3bsplightmaps = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_q3bsplightmaps", "0", "whether to compress lightmaps in q3bsp format levels"};
cvar_t gl_texturecompression_q3bspdeluxemaps = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_q3bspdeluxemaps", "0", "whether to compress deluxemaps in q3bsp format levels (only levels compiled with q3map2 -deluxe have these)"};
cvar_t gl_texturecompression_sky = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_sky", "0", "whether to compress sky textures"};
cvar_t gl_texturecompression_lightcubemaps = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_lightcubemaps", "1", "whether to compress light cubemaps (spotlights and other light projection images)"};
cvar_t gl_texturecompression_reflectmask = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_reflectmask", "1", "whether to compress reflection cubemap masks (mask of which areas of the texture should reflect the generic shiny cubemap)"};
cvar_t gl_texturecompression_sprites = {CF_CLIENT | CF_ARCHIVE, "gl_texturecompression_sprites", "1", "whether to compress sprites"};
cvar_t r_texture_dds_load_alphamode = {CF_CLIENT, "r_texture_dds_load_alphamode", "1", "0: trust DDPF_ALPHAPIXELS flag, 1: texture format and brute force search if ambiguous, 2: texture format only"};
cvar_t r_texture_dds_load_logfailure = {CF_CLIENT, "r_texture_dds_load_logfailure", "0", "log missing DDS textures to ddstexturefailures.log, 0: done log, 1: log with no optional textures (_norm, glow etc.). 2: log all"};
cvar_t r_texture_dds_swdecode = {CF_CLIENT, "r_texture_dds_swdecode", "0", "0: don't software decode DDS, 1: software decode DDS if unsupported, 2: always software decode DDS"};

qbool	gl_filter_force = false;
int		gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int		gl_filter_mag = GL_LINEAR;


static mempool_t *texturemempool;
static memexpandablearray_t texturearray;

// note: this must not conflict with TEXF_ flags in r_textures.h
// bitmask for mismatch checking
#define GLTEXF_IMPORTANTBITS (0)
// dynamic texture (treat texnum == 0 differently)
#define GLTEXF_DYNAMIC		0x00080000

typedef struct textypeinfo_s
{
	const char *name;
	textype_t textype;
	int inputbytesperpixel;
	int internalbytesperpixel;
	float glinternalbytesperpixel;
	int glinternalformat;
	int glformat;
	int gltype;
}
textypeinfo_t;

#ifdef USE_GLES2

// we use these internally even if we never deliver such data to the driver
#define GL_BGR					0x80E0
#define GL_BGRA					0x80E1

// framebuffer texture formats
// GLES2 devices rarely support depth textures, so we actually use a renderbuffer there
static textypeinfo_t textype_shadowmap16_comp            = {"shadowmap16_comp",         TEXTYPE_SHADOWMAP16_COMP     ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_shadowmap16_raw             = {"shadowmap16_raw",          TEXTYPE_SHADOWMAP16_RAW      ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_shadowmap24_comp            = {"shadowmap24_comp",         TEXTYPE_SHADOWMAP24_COMP     ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_shadowmap24_raw             = {"shadowmap24_raw",          TEXTYPE_SHADOWMAP24_RAW      ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_depth16                     = {"depth16",                  TEXTYPE_DEPTHBUFFER16        ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_depth24                     = {"depth24",                  TEXTYPE_DEPTHBUFFER24        ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_depth24stencil8             = {"depth24stencil8",          TEXTYPE_DEPTHBUFFER24STENCIL8,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
#ifdef _DIII4A //karin: using RGBA
static textypeinfo_t textype_colorbuffer                 = {"colorbuffer",              TEXTYPE_COLORBUFFER          ,  2,  2,  2.0f, GL_RGBA                         , GL_RGBA           , GL_UNSIGNED_BYTE};
#else
static textypeinfo_t textype_colorbuffer                 = {"colorbuffer",              TEXTYPE_COLORBUFFER          ,  2,  2,  2.0f, GL_RGB565                         , GL_RGBA           , GL_UNSIGNED_SHORT_5_6_5};
#endif
static textypeinfo_t textype_colorbuffer16f              = {"colorbuffer16f",           TEXTYPE_COLORBUFFER16F       ,  2,  2,  2.0f, GL_RGBA16F                        , GL_RGBA           , GL_HALF_FLOAT};
static textypeinfo_t textype_colorbuffer32f              = {"colorbuffer32f",           TEXTYPE_COLORBUFFER32F       ,  2,  2,  2.0f, GL_RGBA32F                        , GL_RGBA           , GL_FLOAT};

// image formats:
static textypeinfo_t textype_alpha                       = {"alpha",                    TEXTYPE_ALPHA         ,  1,  4,  4.0f, GL_ALPHA                              , GL_ALPHA          , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_palette                     = {"palette",                  TEXTYPE_PALETTE       ,  1,  4,  4.0f, GL_RGBA                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_palette_alpha               = {"palette_alpha",            TEXTYPE_PALETTE       ,  1,  4,  4.0f, GL_RGBA                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_rgba                        = {"rgba",                     TEXTYPE_RGBA          ,  4,  4,  4.0f, GL_RGBA                               , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_rgba_alpha                  = {"rgba_alpha",               TEXTYPE_RGBA          ,  4,  4,  4.0f, GL_RGBA                               , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_bgra                        = {"bgra",                     TEXTYPE_BGRA          ,  4,  4,  4.0f, GL_RGBA                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_bgra_alpha                  = {"bgra_alpha",               TEXTYPE_BGRA          ,  4,  4,  4.0f, GL_RGBA                               , GL_BGRA           , GL_UNSIGNED_BYTE };
#ifdef __ANDROID__
static textypeinfo_t textype_etc1                        = {"etc1",                     TEXTYPE_ETC1          ,  1,  3,  0.5f, GL_ETC1_RGB8_OES                         , 0                 , 0                };
#endif
#else
// framebuffer texture formats
static textypeinfo_t textype_shadowmap16_comp            = {"shadowmap16_comp",         TEXTYPE_SHADOWMAP16_COMP     ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_shadowmap16_raw             = {"shadowmap16_raw",          TEXTYPE_SHADOWMAP16_RAW      ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_shadowmap24_comp            = {"shadowmap24_comp",         TEXTYPE_SHADOWMAP24_COMP     ,  4,  4,  4.0f, GL_DEPTH_COMPONENT24              , GL_DEPTH_COMPONENT, GL_UNSIGNED_INT  };
static textypeinfo_t textype_shadowmap24_raw             = {"shadowmap24_raw",          TEXTYPE_SHADOWMAP24_RAW      ,  4,  4,  4.0f, GL_DEPTH_COMPONENT24              , GL_DEPTH_COMPONENT, GL_UNSIGNED_INT  };
static textypeinfo_t textype_depth16                     = {"depth16",                  TEXTYPE_DEPTHBUFFER16        ,  2,  2,  2.0f, GL_DEPTH_COMPONENT16              , GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
static textypeinfo_t textype_depth24                     = {"depth24",                  TEXTYPE_DEPTHBUFFER24        ,  4,  4,  4.0f, GL_DEPTH_COMPONENT24              , GL_DEPTH_COMPONENT, GL_UNSIGNED_INT  };
static textypeinfo_t textype_depth24stencil8             = {"depth24stencil8",          TEXTYPE_DEPTHBUFFER24STENCIL8,  4,  4,  4.0f, GL_DEPTH24_STENCIL8               , GL_DEPTH_STENCIL  , GL_UNSIGNED_INT_24_8};
static textypeinfo_t textype_colorbuffer                 = {"colorbuffer",              TEXTYPE_COLORBUFFER          ,  4,  4,  4.0f, GL_RGBA                           , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_colorbuffer16f              = {"colorbuffer16f",           TEXTYPE_COLORBUFFER16F       ,  8,  8,  8.0f, GL_RGBA16F                        , GL_RGBA           , GL_HALF_FLOAT    };
static textypeinfo_t textype_colorbuffer32f              = {"colorbuffer32f",           TEXTYPE_COLORBUFFER32F       , 16, 16, 16.0f, GL_RGBA32F                        , GL_RGBA           , GL_FLOAT         };

// image formats:
static textypeinfo_t textype_alpha                       = {"alpha",                    TEXTYPE_ALPHA         ,  1,  4,  4.0f, GL_ALPHA                              , GL_ALPHA          , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_palette                     = {"palette",                  TEXTYPE_PALETTE       ,  1,  4,  4.0f, GL_RGB                                , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_palette_alpha               = {"palette_alpha",            TEXTYPE_PALETTE       ,  1,  4,  4.0f, GL_RGBA                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_rgba                        = {"rgba",                     TEXTYPE_RGBA          ,  4,  4,  4.0f, GL_RGB                                , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_rgba_alpha                  = {"rgba_alpha",               TEXTYPE_RGBA          ,  4,  4,  4.0f, GL_RGBA                               , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_rgba_compress               = {"rgba_compress",            TEXTYPE_RGBA          ,  4,  4,  0.5f, GL_COMPRESSED_RGB_S3TC_DXT1_EXT       , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_rgba_alpha_compress         = {"rgba_alpha_compress",      TEXTYPE_RGBA          ,  4,  4,  1.0f, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT      , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_bgra                        = {"bgra",                     TEXTYPE_BGRA          ,  4,  4,  4.0f, GL_RGB                                , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_bgra_alpha                  = {"bgra_alpha",               TEXTYPE_BGRA          ,  4,  4,  4.0f, GL_RGBA                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_bgra_compress               = {"bgra_compress",            TEXTYPE_BGRA          ,  4,  4,  0.5f, GL_COMPRESSED_RGB_S3TC_DXT1_EXT       , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_bgra_alpha_compress         = {"bgra_alpha_compress",      TEXTYPE_BGRA          ,  4,  4,  1.0f, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT      , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_dxt1                        = {"dxt1",                     TEXTYPE_DXT1          ,  4,  0,  0.5f, GL_COMPRESSED_RGB_S3TC_DXT1_EXT       , 0                 , 0                };
static textypeinfo_t textype_dxt1a                       = {"dxt1a",                    TEXTYPE_DXT1A         ,  4,  0,  0.5f, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT      , 0                 , 0                };
static textypeinfo_t textype_dxt3                        = {"dxt3",                     TEXTYPE_DXT3          ,  4,  0,  1.0f, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT      , 0                 , 0                };
static textypeinfo_t textype_dxt5                        = {"dxt5",                     TEXTYPE_DXT5          ,  4,  0,  1.0f, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT      , 0                 , 0                };
static textypeinfo_t textype_sRGB_palette                = {"sRGB_palette",             TEXTYPE_PALETTE       ,  1,  4,  4.0f, GL_SRGB                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_palette_alpha          = {"sRGB_palette_alpha",       TEXTYPE_PALETTE       ,  1,  4,  4.0f, GL_SRGB_ALPHA                         , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_rgba                   = {"sRGB_rgba",                TEXTYPE_RGBA          ,  4,  4,  4.0f, GL_SRGB                               , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_rgba_alpha             = {"sRGB_rgba_alpha",          TEXTYPE_RGBA          ,  4,  4,  4.0f, GL_SRGB_ALPHA                         , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_rgba_compress          = {"sRGB_rgba_compress",       TEXTYPE_RGBA          ,  4,  4,  0.5f, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT      , GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_rgba_alpha_compress    = {"sRGB_rgba_alpha_compress", TEXTYPE_RGBA          ,  4,  4,  1.0f, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_bgra                   = {"sRGB_bgra",                TEXTYPE_BGRA          ,  4,  4,  4.0f, GL_SRGB                               , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_bgra_alpha             = {"sRGB_bgra_alpha",          TEXTYPE_BGRA          ,  4,  4,  4.0f, GL_SRGB_ALPHA                         , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_bgra_compress          = {"sRGB_bgra_compress",       TEXTYPE_BGRA          ,  4,  4,  0.5f, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT      , GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_bgra_alpha_compress    = {"sRGB_bgra_alpha_compress", TEXTYPE_BGRA          ,  4,  4,  1.0f, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_BGRA           , GL_UNSIGNED_BYTE };
static textypeinfo_t textype_sRGB_dxt1                   = {"sRGB_dxt1",                TEXTYPE_DXT1          ,  4,  0,  0.5f, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT      , 0                 , 0                };
static textypeinfo_t textype_sRGB_dxt1a                  = {"sRGB_dxt1a",               TEXTYPE_DXT1A         ,  4,  0,  0.5f, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0                 , 0                };
static textypeinfo_t textype_sRGB_dxt3                   = {"sRGB_dxt3",                TEXTYPE_DXT3          ,  4,  0,  1.0f, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0                 , 0                };
static textypeinfo_t textype_sRGB_dxt5                   = {"sRGB_dxt5",                TEXTYPE_DXT5          ,  4,  0,  1.0f, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0                 , 0                };
#endif

typedef enum gltexturetype_e
{
	GLTEXTURETYPE_2D,
	GLTEXTURETYPE_3D,
	GLTEXTURETYPE_CUBEMAP,
	GLTEXTURETYPE_TOTAL
}
gltexturetype_t;

static int gltexturetypeenums[GLTEXTURETYPE_TOTAL] = {GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP};
#ifdef GL_TEXTURE_WRAP_R
static int gltexturetypedimensions[GLTEXTURETYPE_TOTAL] = {2, 3, 2};
#endif
static int cubemapside[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

typedef struct gltexture_s
{
	// this portion of the struct is exposed to the R_GetTexture macro for
	// speed reasons, must be identical in rtexture_t!
	int texnum; // GL texture slot number
	int renderbuffernum; // GL renderbuffer slot number
	qbool dirty; // indicates that R_RealGetTexture should be called
	qbool glisdepthstencil; // indicates that FBO attachment has to be GL_DEPTH_STENCIL_ATTACHMENT
	int gltexturetypeenum; // used by R_Mesh_TexBind

	// dynamic texture stuff [11/22/2007 Black]
	updatecallback_t updatecallback;
	void *updatecallback_data;
	// --- [11/22/2007 Black]

	// stores backup copy of texture for deferred texture updates (R_UpdateTexture when combine = true)
	unsigned char *bufferpixels;
	int modified_mins[3], modified_maxs[3];
	qbool buffermodified;

	// pointer to texturepool (check this to see if the texture is allocated)
	struct gltexturepool_s *pool;
	// pointer to next texture in texturepool chain
	struct gltexture_s *chain;
	// name of the texture (this might be removed someday), no duplicates
	char identifier[MAX_QPATH + 32];
	// original data size in *inputtexels
	int inputwidth, inputheight, inputdepth;
	// copy of the original texture(s) supplied to the upload function, for
	// delayed uploads (non-precached)
	unsigned char *inputtexels;
	// original data size in *inputtexels
	int inputdatasize;
	// flags supplied to the LoadTexture function
	// (might be altered to remove TEXF_ALPHA), and GLTEXF_ private flags
	int flags;
	// picmip level
	int miplevel;
	// pointer to one of the textype_ structs
	textypeinfo_t *textype;
	// one of the GLTEXTURETYPE_ values
	int texturetype;
	// palette if the texture is TEXTYPE_PALETTE
	const unsigned int *palette;
	// actual stored texture size after gl_picmip and gl_max_size are applied
	int tilewidth, tileheight, tiledepth;
	// 1 or 6 depending on texturetype
	int sides;
	// how many mipmap levels in this texture
	int miplevels;
	// bytes per pixel
	int bytesperpixel;
	// GL_RGB or GL_RGBA or GL_DEPTH_COMPONENT
	int glformat;
	// 3 or 4
	int glinternalformat;
	// GL_UNSIGNED_BYTE or GL_UNSIGNED_INT or GL_UNSIGNED_SHORT or GL_FLOAT
	int gltype;
}
gltexture_t;

#define TEXTUREPOOL_SENTINEL 0xC0DEDBAD

typedef struct gltexturepool_s
{
	unsigned int sentinel;
	struct gltexture_s *gltchain;
	struct gltexturepool_s *next;
}
gltexturepool_t;

static gltexturepool_t *gltexturepoolchain = NULL;

static unsigned char *resizebuffer = NULL, *colorconvertbuffer;
static int resizebuffersize = 0;
static const unsigned char *texturebuffer;

static textypeinfo_t *R_GetTexTypeInfo(textype_t textype, int flags)
{
	switch(textype)
	{
#ifdef USE_GLES2
	case TEXTYPE_PALETTE: return (flags & TEXF_ALPHA) ? &textype_palette_alpha : &textype_palette;
	case TEXTYPE_RGBA: return ((flags & TEXF_ALPHA) ? &textype_rgba_alpha : &textype_rgba);
	case TEXTYPE_BGRA: return ((flags & TEXF_ALPHA) ? &textype_bgra_alpha : &textype_bgra);
#ifdef __ANDROID__
	case TEXTYPE_ETC1: return &textype_etc1;
#endif
	case TEXTYPE_ALPHA: return &textype_alpha;
	case TEXTYPE_COLORBUFFER: return &textype_colorbuffer;
	case TEXTYPE_COLORBUFFER16F: return &textype_colorbuffer16f;
	case TEXTYPE_COLORBUFFER32F: return &textype_colorbuffer32f;
	case TEXTYPE_DEPTHBUFFER16: return &textype_depth16;
	case TEXTYPE_DEPTHBUFFER24: return &textype_depth24;
	case TEXTYPE_DEPTHBUFFER24STENCIL8: return &textype_depth24stencil8;
	case TEXTYPE_SHADOWMAP16_COMP: return &textype_shadowmap16_comp;
	case TEXTYPE_SHADOWMAP16_RAW: return &textype_shadowmap16_raw;
	case TEXTYPE_SHADOWMAP24_COMP: return &textype_shadowmap24_comp;
	case TEXTYPE_SHADOWMAP24_RAW: return &textype_shadowmap24_raw;
#else
	case TEXTYPE_DXT1: return &textype_dxt1;
	case TEXTYPE_DXT1A: return &textype_dxt1a;
	case TEXTYPE_DXT3: return &textype_dxt3;
	case TEXTYPE_DXT5: return &textype_dxt5;
	case TEXTYPE_PALETTE: return (flags & TEXF_ALPHA) ? &textype_palette_alpha : &textype_palette;
	case TEXTYPE_RGBA: return ((flags & TEXF_COMPRESS) && vid.support.ext_texture_compression_s3tc) ? ((flags & TEXF_ALPHA) ? &textype_rgba_alpha_compress : &textype_rgba_compress) : ((flags & TEXF_ALPHA) ? &textype_rgba_alpha : &textype_rgba);
	case TEXTYPE_BGRA: return ((flags & TEXF_COMPRESS) && vid.support.ext_texture_compression_s3tc) ? ((flags & TEXF_ALPHA) ? &textype_bgra_alpha_compress : &textype_bgra_compress) : ((flags & TEXF_ALPHA) ? &textype_bgra_alpha : &textype_bgra);
	case TEXTYPE_ALPHA: return &textype_alpha;
	case TEXTYPE_COLORBUFFER: return &textype_colorbuffer;
	case TEXTYPE_COLORBUFFER16F: return &textype_colorbuffer16f;
	case TEXTYPE_COLORBUFFER32F: return &textype_colorbuffer32f;
	case TEXTYPE_DEPTHBUFFER16: return &textype_depth16;
	case TEXTYPE_DEPTHBUFFER24: return &textype_depth24;
	case TEXTYPE_DEPTHBUFFER24STENCIL8: return &textype_depth24stencil8;
	case TEXTYPE_SHADOWMAP16_COMP: return &textype_shadowmap16_comp;
	case TEXTYPE_SHADOWMAP16_RAW: return &textype_shadowmap16_raw;
	case TEXTYPE_SHADOWMAP24_COMP: return &textype_shadowmap24_comp;
	case TEXTYPE_SHADOWMAP24_RAW: return &textype_shadowmap24_raw;
	case TEXTYPE_SRGB_DXT1: return &textype_sRGB_dxt1;
	case TEXTYPE_SRGB_DXT1A: return &textype_sRGB_dxt1a;
	case TEXTYPE_SRGB_DXT3: return &textype_sRGB_dxt3;
	case TEXTYPE_SRGB_DXT5: return &textype_sRGB_dxt5;
	case TEXTYPE_SRGB_PALETTE: return (flags & TEXF_ALPHA) ? &textype_sRGB_palette_alpha : &textype_sRGB_palette;
	case TEXTYPE_SRGB_RGBA: return ((flags & TEXF_COMPRESS) && vid.support.ext_texture_compression_s3tc) ? ((flags & TEXF_ALPHA) ? &textype_sRGB_rgba_alpha_compress : &textype_sRGB_rgba_compress) : ((flags & TEXF_ALPHA) ? &textype_sRGB_rgba_alpha : &textype_sRGB_rgba);
	case TEXTYPE_SRGB_BGRA: return ((flags & TEXF_COMPRESS) && vid.support.ext_texture_compression_s3tc) ? ((flags & TEXF_ALPHA) ? &textype_sRGB_bgra_alpha_compress : &textype_sRGB_bgra_compress) : ((flags & TEXF_ALPHA) ? &textype_sRGB_bgra_alpha : &textype_sRGB_bgra);
#endif
	default:
		Host_Error("R_GetTexTypeInfo: unknown texture format %i with flags %x", (int)textype, flags);
		break;
	}
	return NULL;
}

// dynamic texture code [11/22/2007 Black]
void R_MarkDirtyTexture(rtexture_t *rt) {
	gltexture_t *glt = (gltexture_t*) rt;
	if( !glt ) {
		return;
	}

	// dont do anything if the texture is already dirty (and make sure this *is* a dynamic texture after all!)
	if (glt->flags & GLTEXF_DYNAMIC)
	{
		// mark it as dirty, so R_RealGetTexture gets called
		glt->dirty = true;
	}
}

void R_MakeTextureDynamic(rtexture_t *rt, updatecallback_t updatecallback, void *data) {
	gltexture_t *glt = (gltexture_t*) rt;
	if( !glt ) {
		return;
	}

	glt->flags |= GLTEXF_DYNAMIC;
	glt->updatecallback = updatecallback;
	glt->updatecallback_data = data;
}

static void R_UpdateDynamicTexture(gltexture_t *glt) {
	glt->dirty = false;
	if( glt->updatecallback ) {
		glt->updatecallback( (rtexture_t*) glt, glt->updatecallback_data );
	}
}

void R_PurgeTexture(rtexture_t *rt)
{
	if(rt && !(((gltexture_t*) rt)->flags & TEXF_PERSISTENT)) {
		R_FreeTexture(rt);
	}
}

void R_FreeTexture(rtexture_t *rt)
{
	gltexture_t *glt, **gltpointer;

	glt = (gltexture_t *)rt;
	if (glt == NULL)
		Host_Error("R_FreeTexture: texture == NULL");

	for (gltpointer = &glt->pool->gltchain;*gltpointer && *gltpointer != glt;gltpointer = &(*gltpointer)->chain);
	if (*gltpointer == glt)
		*gltpointer = glt->chain;
	else
		Host_Error("R_FreeTexture: texture \"%s\" not linked in pool", glt->identifier);

	R_Mesh_ClearBindingsForTexture(glt->texnum);

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (glt->texnum)
		{
			CHECKGLERROR
			qglDeleteTextures(1, (GLuint *)&glt->texnum);CHECKGLERROR
		}
		if (glt->renderbuffernum)
		{
			CHECKGLERROR
			qglDeleteRenderbuffers(1, (GLuint *)&glt->renderbuffernum);CHECKGLERROR
		}
		break;
	}

	if (glt->inputtexels)
		Mem_Free(glt->inputtexels);
	Mem_ExpandableArray_FreeRecord(&texturearray, glt);
}

rtexturepool_t *R_AllocTexturePool(void)
{
	gltexturepool_t *pool;
	if (texturemempool == NULL)
		return NULL;
	pool = (gltexturepool_t *)Mem_Alloc(texturemempool, sizeof(gltexturepool_t));
	if (pool == NULL)
		return NULL;
	pool->next = gltexturepoolchain;
	gltexturepoolchain = pool;
	pool->sentinel = TEXTUREPOOL_SENTINEL;
	return (rtexturepool_t *)pool;
}

void R_FreeTexturePool(rtexturepool_t **rtexturepool)
{
	gltexturepool_t *pool, **poolpointer;
	if (rtexturepool == NULL)
		return;
	if (*rtexturepool == NULL)
		return;
	pool = (gltexturepool_t *)(*rtexturepool);
	*rtexturepool = NULL;
	if (pool->sentinel != TEXTUREPOOL_SENTINEL)
		Host_Error("R_FreeTexturePool: pool already freed");
	for (poolpointer = &gltexturepoolchain;*poolpointer && *poolpointer != pool;poolpointer = &(*poolpointer)->next);
	if (*poolpointer == pool)
		*poolpointer = pool->next;
	else
		Host_Error("R_FreeTexturePool: pool not linked");
	while (pool->gltchain)
		R_FreeTexture((rtexture_t *)pool->gltchain);
	Mem_Free(pool);
}


typedef struct glmode_s
{
	const char *name;
	int minification, magnification;
}
glmode_t;

static glmode_t modes[6] =
{
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

static void GL_TextureMode_f(cmd_state_t *cmd)
{
	int i;
	GLint oldbindtexnum;
	gltexture_t *glt;
	gltexturepool_t *pool;

	if (Cmd_Argc(cmd) == 1)
	{
		Con_Printf("Texture mode is %sforced\n", gl_filter_force ? "" : "not ");
		for (i = 0;i < 6;i++)
		{
			if (gl_filter_min == modes[i].minification)
			{
				Con_Printf("%s\n", modes[i].name);
				return;
			}
		}
		Con_Print("current filter is unknown???\n");
		return;
	}

	for (i = 0;i < (int)(sizeof(modes)/sizeof(*modes));i++)
		if (!strcasecmp (modes[i].name, Cmd_Argv(cmd, 1) ) )
			break;
	if (i == 6)
	{
		Con_Print("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minification;
	gl_filter_mag = modes[i].magnification;
	gl_filter_force = ((Cmd_Argc(cmd) > 2) && !strcasecmp(Cmd_Argv(cmd, 2), "force"));

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		// change all the existing mipmap texture objects
		// FIXME: force renderer(/client/something?) restart instead?
		CHECKGLERROR
		GL_ActiveTexture(0);
		for (pool = gltexturepoolchain;pool;pool = pool->next)
		{
			for (glt = pool->gltchain;glt;glt = glt->chain)
			{
				// only update already uploaded images
				if (glt->texnum && (gl_filter_force || !(glt->flags & (TEXF_FORCENEAREST | TEXF_FORCELINEAR))))
				{
					oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);
					qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR
					if (glt->flags & TEXF_MIPMAP)
					{
						qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MIN_FILTER, gl_filter_min);CHECKGLERROR
					}
					else
					{
						qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MIN_FILTER, gl_filter_mag);CHECKGLERROR
					}
					qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MAG_FILTER, gl_filter_mag);CHECKGLERROR
					qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
				}
			}
		}
		break;
	}
}

static void GL_Texture_CalcImageSize(int texturetype, int flags, int miplevel, int inwidth, int inheight, int indepth, int *outwidth, int *outheight, int *outdepth, int *outmiplevels)
{
	int picmip = 0, maxsize = 0, width2 = 1, height2 = 1, depth2 = 1, miplevels = 1;

	switch (texturetype)
	{
	default:
	case GLTEXTURETYPE_2D:
		maxsize = vid.maxtexturesize_2d;
		if (flags & TEXF_PICMIP)
		{
			maxsize = bound(1, gl_max_size.integer, maxsize);
			picmip = miplevel;
		}
		break;
	case GLTEXTURETYPE_3D:
		maxsize = vid.maxtexturesize_3d;
		break;
	case GLTEXTURETYPE_CUBEMAP:
		maxsize = vid.maxtexturesize_cubemap;
		break;
	}

	width2 = min(inwidth >> picmip, maxsize);
	height2 = min(inheight >> picmip, maxsize);
	depth2 = min(indepth >> picmip, maxsize);

	miplevels = 1;
	if (flags & TEXF_MIPMAP)
	{
		int extent = max(width2, max(height2, depth2));
		while(extent >>= 1)
			miplevels++;
	}

	if (outwidth)
		*outwidth = max(1, width2);
	if (outheight)
		*outheight = max(1, height2);
	if (outdepth)
		*outdepth = max(1, depth2);
	if (outmiplevels)
		*outmiplevels = miplevels;
}


static int R_CalcTexelDataSize (gltexture_t *glt)
{
	int width2, height2, depth2, size;

	GL_Texture_CalcImageSize(glt->texturetype, glt->flags, glt->miplevel, glt->inputwidth, glt->inputheight, glt->inputdepth, &width2, &height2, &depth2, NULL);

	size = width2 * height2 * depth2;

	if (glt->flags & TEXF_MIPMAP)
	{
		while (width2 > 1 || height2 > 1 || depth2 > 1)
		{
			if (width2 > 1)
				width2 >>= 1;
			if (height2 > 1)
				height2 >>= 1;
			if (depth2 > 1)
				depth2 >>= 1;
			size += width2 * height2 * depth2;
		}
	}

	return (int)(size * glt->textype->glinternalbytesperpixel) * glt->sides;
}

void R_TextureStats_Print(qbool printeach, qbool printpool, qbool printtotal)
{
	int glsize;
	int isloaded;
	int pooltotal = 0, pooltotalt = 0, pooltotalp = 0, poolloaded = 0, poolloadedt = 0, poolloadedp = 0;
	int sumtotal = 0, sumtotalt = 0, sumtotalp = 0, sumloaded = 0, sumloadedt = 0, sumloadedp = 0;
	gltexture_t *glt;
	gltexturepool_t *pool;
	if (printeach)
		Con_Print("glsize input loaded mip alpha name\n");
	for (pool = gltexturepoolchain;pool;pool = pool->next)
	{
		pooltotal = 0;
		pooltotalt = 0;
		pooltotalp = 0;
		poolloaded = 0;
		poolloadedt = 0;
		poolloadedp = 0;
		for (glt = pool->gltchain;glt;glt = glt->chain)
		{
			glsize = R_CalcTexelDataSize(glt);
			isloaded = glt->texnum != 0 || glt->renderbuffernum != 0;
			pooltotal++;
			pooltotalt += glsize;
			pooltotalp += glt->inputdatasize;
			if (isloaded)
			{
				poolloaded++;
				poolloadedt += glsize;
				poolloadedp += glt->inputdatasize;
			}
			if (printeach)
				Con_Printf("%c%4i%c%c%4i%c %-24s %s %s %s %s\n", isloaded ? '[' : ' ', (glsize + 1023) / 1024, isloaded ? ']' : ' ', glt->inputtexels ? '[' : ' ', (glt->inputdatasize + 1023) / 1024, glt->inputtexels ? ']' : ' ', glt->textype->name, isloaded ? "loaded" : "      ", (glt->flags & TEXF_MIPMAP) ? "mip" : "   ", (glt->flags & TEXF_ALPHA) ? "alpha" : "     ", glt->identifier);
		}
		if (printpool)
			Con_Printf("texturepool %10p total: %i (%.3fMB, %.3fMB original), uploaded %i (%.3fMB, %.3fMB original), upload on demand %i (%.3fMB, %.3fMB original)\n", (void *)pool, pooltotal, pooltotalt / 1048576.0, pooltotalp / 1048576.0, poolloaded, poolloadedt / 1048576.0, poolloadedp / 1048576.0, pooltotal - poolloaded, (pooltotalt - poolloadedt) / 1048576.0, (pooltotalp - poolloadedp) / 1048576.0);
		sumtotal += pooltotal;
		sumtotalt += pooltotalt;
		sumtotalp += pooltotalp;
		sumloaded += poolloaded;
		sumloadedt += poolloadedt;
		sumloadedp += poolloadedp;
	}
	if (printtotal)
		Con_Printf("textures total: %i (%.3fMB, %.3fMB original), uploaded %i (%.3fMB, %.3fMB original), upload on demand %i (%.3fMB, %.3fMB original)\n", sumtotal, sumtotalt / 1048576.0, sumtotalp / 1048576.0, sumloaded, sumloadedt / 1048576.0, sumloadedp / 1048576.0, sumtotal - sumloaded, (sumtotalt - sumloadedt) / 1048576.0, (sumtotalp - sumloadedp) / 1048576.0);
}

static void R_TextureStats_f(cmd_state_t *cmd)
{
	R_TextureStats_Print(true, true, true);
}

static void r_textures_start(void)
{
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		// LadyHavoc: allow any alignment
		CHECKGLERROR
		qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);CHECKGLERROR
		qglPixelStorei(GL_PACK_ALIGNMENT, 1);CHECKGLERROR
		break;
	}

	texturemempool = Mem_AllocPool("texture management", 0, NULL);
	Mem_ExpandableArray_NewArray(&texturearray, texturemempool, sizeof(gltexture_t), 512);

	// Disable JPEG screenshots if the DLL isn't loaded
	if (! JPEG_OpenLibrary ())
		Cvar_SetValueQuick (&scr_screenshot_jpeg, 0);
	if (! PNG_OpenLibrary ())
		Cvar_SetValueQuick (&scr_screenshot_png, 0);
}

static void r_textures_shutdown(void)
{
	rtexturepool_t *temp;

	JPEG_CloseLibrary ();

	while(gltexturepoolchain)
	{
		temp = (rtexturepool_t *) gltexturepoolchain;
		R_FreeTexturePool(&temp);
	}

	resizebuffersize = 0;
	resizebuffer = NULL;
	colorconvertbuffer = NULL;
	texturebuffer = NULL;
	Mem_ExpandableArray_FreeArray(&texturearray);
	Mem_FreePool(&texturemempool);
}

static void r_textures_newmap(void)
{
}

static void r_textures_devicelost(void)
{
	int i, endindex;
	gltexture_t *glt;
	endindex = (int)Mem_ExpandableArray_IndexRange(&texturearray);
	for (i = 0;i < endindex;i++)
	{
		glt = (gltexture_t *) Mem_ExpandableArray_RecordAtIndex(&texturearray, i);
		if (!glt || !(glt->flags & TEXF_RENDERTARGET))
			continue;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			break;
		}
	}
}

static void r_textures_devicerestored(void)
{
	int i, endindex;
	gltexture_t *glt;
	endindex = (int)Mem_ExpandableArray_IndexRange(&texturearray);
	for (i = 0;i < endindex;i++)
	{
		glt = (gltexture_t *) Mem_ExpandableArray_RecordAtIndex(&texturearray, i);
		if (!glt || !(glt->flags & TEXF_RENDERTARGET))
			continue;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			break;
		}
	}
}


void R_Textures_Init (void)
{
	Cmd_AddCommand(CF_CLIENT, "gl_texturemode", &GL_TextureMode_f, "set texture filtering mode (GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, etc); an additional argument 'force' forces the texture mode even in cases where it may not be appropriate");
	Cmd_AddCommand(CF_CLIENT, "r_texturestats", R_TextureStats_f, "print information about all loaded textures and some statistics");
	Cvar_RegisterVariable (&gl_max_size);
	Cvar_RegisterVariable (&gl_picmip);
	Cvar_RegisterVariable (&gl_picmip_world);
	Cvar_RegisterVariable (&r_picmipworld);
	Cvar_RegisterVariable (&gl_picmip_sprites);
	Cvar_RegisterVariable (&r_picmipsprites);
	Cvar_RegisterVariable (&gl_picmip_other);
	Cvar_RegisterVariable (&gl_max_lightmapsize);
	Cvar_RegisterVariable (&r_lerpimages);
	Cvar_RegisterVariable (&gl_texture_anisotropy);
	Cvar_RegisterVariable (&gl_texturecompression);
	Cvar_RegisterVariable (&gl_texturecompression_color);
	Cvar_RegisterVariable (&gl_texturecompression_normal);
	Cvar_RegisterVariable (&gl_texturecompression_gloss);
	Cvar_RegisterVariable (&gl_texturecompression_glow);
	Cvar_RegisterVariable (&gl_texturecompression_2d);
	Cvar_RegisterVariable (&gl_texturecompression_q3bsplightmaps);
	Cvar_RegisterVariable (&gl_texturecompression_q3bspdeluxemaps);
	Cvar_RegisterVariable (&gl_texturecompression_sky);
	Cvar_RegisterVariable (&gl_texturecompression_lightcubemaps);
	Cvar_RegisterVariable (&gl_texturecompression_reflectmask);
	Cvar_RegisterVariable (&gl_texturecompression_sprites);
	Cvar_RegisterVariable (&r_texture_dds_load_alphamode);
	Cvar_RegisterVariable (&r_texture_dds_load_logfailure);
	Cvar_RegisterVariable (&r_texture_dds_swdecode);

	R_RegisterModule("R_Textures", r_textures_start, r_textures_shutdown, r_textures_newmap, r_textures_devicelost, r_textures_devicerestored);
}

void R_Textures_Frame (void)
{
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
	static int old_aniso = 0;
	static qbool first_time_aniso = true;
#endif

	// could do procedural texture animation here, if we keep track of which
	// textures were accessed this frame...

	// free the resize buffers
	resizebuffersize = 0;
	if (resizebuffer)
	{
		Mem_Free(resizebuffer);
		resizebuffer = NULL;
	}
	if (colorconvertbuffer)
	{
		Mem_Free(colorconvertbuffer);
		colorconvertbuffer = NULL;
	}

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
	if (old_aniso != gl_texture_anisotropy.integer)
	{
		gltexture_t *glt;
		gltexturepool_t *pool;
		GLint oldbindtexnum;

		old_aniso = bound(1, gl_texture_anisotropy.integer, (int)vid.max_anisotropy);

		Cvar_SetValueQuick(&gl_texture_anisotropy, old_aniso);

		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			// ignore the first difference, any textures loaded by now probably had the same aniso value
			if (first_time_aniso)
			{
				first_time_aniso = false;
				break;
			}
			CHECKGLERROR
			GL_ActiveTexture(0);
			for (pool = gltexturepoolchain;pool;pool = pool->next)
			{
				for (glt = pool->gltchain;glt;glt = glt->chain)
				{
					// only update already uploaded images
					if (glt->texnum && (glt->flags & TEXF_MIPMAP) == TEXF_MIPMAP)
					{
						oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);

						qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR
						qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MAX_ANISOTROPY_EXT, old_aniso);CHECKGLERROR

						qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
					}
				}
			}
			break;
		}
	}
#endif
}

static void R_MakeResizeBufferBigger(int size)
{
	if (resizebuffersize < size)
	{
		resizebuffersize = size;
		if (resizebuffer)
			Mem_Free(resizebuffer);
		if (colorconvertbuffer)
			Mem_Free(colorconvertbuffer);
		resizebuffer = (unsigned char *)Mem_Alloc(texturemempool, resizebuffersize);
		colorconvertbuffer = (unsigned char *)Mem_Alloc(texturemempool, resizebuffersize);
		if (!resizebuffer || !colorconvertbuffer)
			Host_Error("R_Upload: out of memory");
	}
}

static void GL_SetupTextureParameters(int flags, textype_t textype, int texturetype)
{
	int textureenum = gltexturetypeenums[texturetype];
	int wrapmode = (flags & TEXF_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT;

	CHECKGLERROR

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
	if (vid.support.ext_texture_filter_anisotropic && (flags & TEXF_MIPMAP))
	{
		int aniso = bound(1, gl_texture_anisotropy.integer, (int)vid.max_anisotropy);
		if (gl_texture_anisotropy.integer != aniso)
			Cvar_SetValueQuick(&gl_texture_anisotropy, aniso);
		qglTexParameteri(textureenum, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);CHECKGLERROR
	}
#endif
	qglTexParameteri(textureenum, GL_TEXTURE_WRAP_S, wrapmode);CHECKGLERROR
	qglTexParameteri(textureenum, GL_TEXTURE_WRAP_T, wrapmode);CHECKGLERROR
#ifdef GL_TEXTURE_WRAP_R
	if (gltexturetypedimensions[texturetype] >= 3)
	{
		qglTexParameteri(textureenum, GL_TEXTURE_WRAP_R, wrapmode);CHECKGLERROR
	}
#endif

	CHECKGLERROR
	if (!gl_filter_force && flags & TEXF_FORCENEAREST)
	{
		if (flags & TEXF_MIPMAP)
		{
			qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);CHECKGLERROR
		}
		else
		{
			qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, GL_NEAREST);CHECKGLERROR
		}
		qglTexParameteri(textureenum, GL_TEXTURE_MAG_FILTER, GL_NEAREST);CHECKGLERROR
	}
	else if (!gl_filter_force && flags & TEXF_FORCELINEAR)
	{
		if (flags & TEXF_MIPMAP)
		{
			if (gl_filter_min == GL_NEAREST_MIPMAP_LINEAR || gl_filter_min == GL_LINEAR_MIPMAP_LINEAR)
			{
				qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);CHECKGLERROR
			}
			else
			{
				qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);CHECKGLERROR
			}
		}
		else
		{
			qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, GL_LINEAR);CHECKGLERROR
		}
		qglTexParameteri(textureenum, GL_TEXTURE_MAG_FILTER, GL_LINEAR);CHECKGLERROR
	}
	else
	{
		if (flags & TEXF_MIPMAP)
		{
			qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, gl_filter_min);CHECKGLERROR
		}
		else
		{
			qglTexParameteri(textureenum, GL_TEXTURE_MIN_FILTER, gl_filter_mag);CHECKGLERROR
		}
		qglTexParameteri(textureenum, GL_TEXTURE_MAG_FILTER, gl_filter_mag);CHECKGLERROR
	}

#ifndef USE_GLES2
	switch(textype)
	{
	case TEXTYPE_SHADOWMAP16_COMP:
	case TEXTYPE_SHADOWMAP24_COMP:
		qglTexParameteri(textureenum, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);CHECKGLERROR
		qglTexParameteri(textureenum, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);CHECKGLERROR
		break;
	case TEXTYPE_SHADOWMAP16_RAW:
	case TEXTYPE_SHADOWMAP24_RAW:
		qglTexParameteri(textureenum, GL_TEXTURE_COMPARE_MODE, GL_NONE);CHECKGLERROR
		qglTexParameteri(textureenum, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);CHECKGLERROR
		break;
	default:
		break;
	}
#endif

	CHECKGLERROR
}

static void R_UploadPartialTexture(gltexture_t *glt, const unsigned char *data, int fragx, int fragy, int fragz, int fragwidth, int fragheight, int fragdepth)
{
	if (data == NULL)
		Sys_Error("R_UploadPartialTexture \"%s\": partial update with NULL pixels", glt->identifier);

	if (glt->texturetype != GLTEXTURETYPE_2D)
		Sys_Error("R_UploadPartialTexture \"%s\": partial update of type other than 2D", glt->identifier);

	if (glt->textype->textype == TEXTYPE_PALETTE)
		Sys_Error("R_UploadPartialTexture \"%s\": partial update of paletted texture", glt->identifier);

	if (glt->flags & (TEXF_MIPMAP | TEXF_PICMIP))
		Sys_Error("R_UploadPartialTexture \"%s\": partial update not supported with MIPMAP or PICMIP flags", glt->identifier);

	if (glt->inputwidth != glt->tilewidth || glt->inputheight != glt->tileheight || glt->tiledepth != 1)
		Sys_Error("R_UploadPartialTexture \"%s\": partial update not supported with stretched or special textures", glt->identifier);

	// update a portion of the image

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		{
			int oldbindtexnum;
			CHECKGLERROR
			// we need to restore the texture binding after finishing the upload
			GL_ActiveTexture(0);
			oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);
			qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR
			qglTexSubImage2D(GL_TEXTURE_2D, 0, fragx, fragy, fragwidth, fragheight, glt->glformat, glt->gltype, data);CHECKGLERROR
			qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
		}
		break;
	}
}

static void R_UploadFullTexture(gltexture_t *glt, const unsigned char *data)
{
	int i, mip = 0, width, height, depth;
	GLint oldbindtexnum = 0;
	const unsigned char *prevbuffer;
	prevbuffer = data;

	// error out if a stretch is needed on special texture types
	if (glt->texturetype != GLTEXTURETYPE_2D && (glt->tilewidth != glt->inputwidth || glt->tileheight != glt->inputheight || glt->tiledepth != glt->inputdepth))
		Sys_Error("R_UploadFullTexture \"%s\": stretch uploads allowed only on 2D textures\n", glt->identifier);

	// when picmip or maxsize is applied, we scale up to a power of 2 multiple
	// of the target size and then use the mipmap reduction function to get
	// high quality supersampled results
	for (width  = glt->tilewidth;width  < glt->inputwidth ;width  <<= 1);
	for (height = glt->tileheight;height < glt->inputheight;height <<= 1);
	for (depth  = glt->tiledepth;depth  < glt->inputdepth ;depth  <<= 1);

	if (prevbuffer == NULL)
	{
		width = glt->tilewidth;
		height = glt->tileheight;
		depth = glt->tiledepth;
//		R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
//		memset(resizebuffer, 0, width * height * depth * glt->sides * glt->bytesperpixel);
//		prevbuffer = resizebuffer;
	}
	else
	{
		if (glt->textype->textype == TEXTYPE_PALETTE)
		{
			// promote paletted to BGRA, so we only have to worry about BGRA in the rest of this code
			R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
			Image_Copy8bitBGRA(prevbuffer, colorconvertbuffer, glt->inputwidth * glt->inputheight * glt->inputdepth * glt->sides, glt->palette);
			prevbuffer = colorconvertbuffer;
		}
		if (glt->flags & TEXF_RGBMULTIPLYBYALPHA)
		{
			// multiply RGB channels by A channel before uploading
			int alpha;
			R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
			for (i = 0;i < glt->inputwidth*glt->inputheight*glt->inputdepth*4;i += 4)
			{
				alpha = prevbuffer[i+3];
				colorconvertbuffer[i] = (prevbuffer[i] * alpha) >> 8;
				colorconvertbuffer[i+1] = (prevbuffer[i+1] * alpha) >> 8;
				colorconvertbuffer[i+2] = (prevbuffer[i+2] * alpha) >> 8;
				colorconvertbuffer[i+3] = alpha;
			}
			prevbuffer = colorconvertbuffer;
		}
		// scale up to a power of 2 size (if appropriate)
		if (glt->inputwidth != width || glt->inputheight != height || glt->inputdepth != depth)
		{
			R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
			Image_Resample32(prevbuffer, glt->inputwidth, glt->inputheight, glt->inputdepth, resizebuffer, width, height, depth, r_lerpimages.integer);
			prevbuffer = resizebuffer;
		}
		// apply mipmap reduction algorithm to get down to picmip/max_size
		while (width > glt->tilewidth || height > glt->tileheight || depth > glt->tiledepth)
		{
			R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
			Image_MipReduce32(prevbuffer, resizebuffer, &width, &height, &depth, glt->tilewidth, glt->tileheight, glt->tiledepth);
			prevbuffer = resizebuffer;
		}
	}

	// do the appropriate upload type...
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		if (glt->texnum) // not renderbuffers
		{
			CHECKGLERROR

			// we need to restore the texture binding after finishing the upload
			GL_ActiveTexture(0);
			oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);
			qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR

#ifndef USE_GLES2
			if (gl_texturecompression.integer >= 2)
				qglHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
			else
				qglHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
			CHECKGLERROR
#endif
			switch(glt->texturetype)
			{
			case GLTEXTURETYPE_2D:
				qglTexImage2D(GL_TEXTURE_2D, mip++, glt->glinternalformat, width, height, 0, glt->glformat, glt->gltype, prevbuffer);CHECKGLERROR
				if (glt->flags & TEXF_MIPMAP)
				{
					while (width > 1 || height > 1 || depth > 1)
					{
						R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
						Image_MipReduce32(prevbuffer, resizebuffer, &width, &height, &depth, 1, 1, 1);
						prevbuffer = resizebuffer;
						qglTexImage2D(GL_TEXTURE_2D, mip++, glt->glinternalformat, width, height, 0, glt->glformat, glt->gltype, prevbuffer);CHECKGLERROR
					}
				}
				break;
			case GLTEXTURETYPE_3D:
#ifndef USE_GLES2
				qglTexImage3D(GL_TEXTURE_3D, mip++, glt->glinternalformat, width, height, depth, 0, glt->glformat, glt->gltype, prevbuffer);CHECKGLERROR
				if (glt->flags & TEXF_MIPMAP)
				{
					while (width > 1 || height > 1 || depth > 1)
					{
						R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
						Image_MipReduce32(prevbuffer, resizebuffer, &width, &height, &depth, 1, 1, 1);
						prevbuffer = resizebuffer;
						qglTexImage3D(GL_TEXTURE_3D, mip++, glt->glinternalformat, width, height, depth, 0, glt->glformat, glt->gltype, prevbuffer);CHECKGLERROR
					}
				}
#endif
				break;
			case GLTEXTURETYPE_CUBEMAP:
				// convert and upload each side in turn,
				// from a continuous block of input texels
				texturebuffer = (unsigned char *)prevbuffer;
				for (i = 0;i < 6;i++)
				{
					prevbuffer = texturebuffer;
					texturebuffer += glt->inputwidth * glt->inputheight * glt->inputdepth * glt->textype->inputbytesperpixel;
					if (glt->inputwidth != width || glt->inputheight != height || glt->inputdepth != depth)
					{
						R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
						Image_Resample32(prevbuffer, glt->inputwidth, glt->inputheight, glt->inputdepth, resizebuffer, width, height, depth, r_lerpimages.integer);
						prevbuffer = resizebuffer;
					}
					// picmip/max_size
					while (width > glt->tilewidth || height > glt->tileheight || depth > glt->tiledepth)
					{
						R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
						Image_MipReduce32(prevbuffer, resizebuffer, &width, &height, &depth, glt->tilewidth, glt->tileheight, glt->tiledepth);
						prevbuffer = resizebuffer;
					}
					mip = 0;
					qglTexImage2D(cubemapside[i], mip++, glt->glinternalformat, width, height, 0, glt->glformat, glt->gltype, prevbuffer);CHECKGLERROR
					if (glt->flags & TEXF_MIPMAP)
					{
						while (width > 1 || height > 1 || depth > 1)
						{
							R_MakeResizeBufferBigger(width * height * depth * glt->sides * glt->bytesperpixel);
							Image_MipReduce32(prevbuffer, resizebuffer, &width, &height, &depth, 1, 1, 1);
							prevbuffer = resizebuffer;
							qglTexImage2D(cubemapside[i], mip++, glt->glinternalformat, width, height, 0, glt->glformat, glt->gltype, prevbuffer);CHECKGLERROR
						}
					}
				}
				break;
			}
			GL_SetupTextureParameters(glt->flags, glt->textype->textype, glt->texturetype);
			qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
		}
		break;
	}
}

static rtexture_t *R_SetupTexture(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, int depth, int sides, int flags, int miplevel, textype_t textype, int texturetype, const unsigned char *data, const unsigned int *palette)
{
	int i, size;
	gltexture_t *glt;
	gltexturepool_t *pool = (gltexturepool_t *)rtexturepool;
	textypeinfo_t *texinfo, *texinfo2;
	unsigned char *temppixels = NULL;
	qbool swaprb;

	if (cls.state == ca_dedicated)
		return NULL;

	// see if we need to swap red and blue (BGRA <-> RGBA conversion)
	if (textype == TEXTYPE_PALETTE && vid.forcetextype == TEXTYPE_RGBA)
	{
		int numpixels = width * height * depth * sides;
		size = numpixels * 4;
		temppixels = (unsigned char *)Mem_Alloc(tempmempool, size);
		if (data)
		{
			const unsigned char *p;
			unsigned char *o = temppixels;
			for (i = 0;i < numpixels;i++, o += 4)
			{
				p = (const unsigned char *)palette + 4*data[i];
				o[0] = p[2];
				o[1] = p[1];
				o[2] = p[0];
				o[3] = p[3];
			}
		}
		data = temppixels;
		textype = TEXTYPE_RGBA;
	}
	swaprb = false;
	switch(textype)
	{
	case TEXTYPE_RGBA: if (vid.forcetextype == TEXTYPE_BGRA) {swaprb = true;textype = TEXTYPE_BGRA;} break;
	case TEXTYPE_BGRA: if (vid.forcetextype == TEXTYPE_RGBA) {swaprb = true;textype = TEXTYPE_RGBA;} break;
	case TEXTYPE_SRGB_RGBA: if (vid.forcetextype == TEXTYPE_BGRA) {swaprb = true;textype = TEXTYPE_SRGB_BGRA;} break;
	case TEXTYPE_SRGB_BGRA: if (vid.forcetextype == TEXTYPE_RGBA) {swaprb = true;textype = TEXTYPE_SRGB_RGBA;} break;
	default: break;
	}
	if (swaprb)
	{
		// swap bytes
		static int rgbaswapindices[4] = {2, 1, 0, 3};
		size = width * height * depth * sides * 4;
		temppixels = (unsigned char *)Mem_Alloc(tempmempool, size);
		if (data)
			Image_CopyMux(temppixels, data, width, height*depth*sides, false, false, false, 4, 4, rgbaswapindices);
		data = temppixels;
	}

	// if sRGB texture formats are not supported, convert input to linear and upload as normal types
	if (!vid.support.ext_texture_srgb)
	{
		qbool convertsRGB = false;
		switch(textype)
		{
		case TEXTYPE_SRGB_DXT1:    textype = TEXTYPE_DXT1   ;convertsRGB = true;break;
		case TEXTYPE_SRGB_DXT1A:   textype = TEXTYPE_DXT1A  ;convertsRGB = true;break;
		case TEXTYPE_SRGB_DXT3:    textype = TEXTYPE_DXT3   ;convertsRGB = true;break;
		case TEXTYPE_SRGB_DXT5:    textype = TEXTYPE_DXT5   ;convertsRGB = true;break;
		case TEXTYPE_SRGB_PALETTE: textype = TEXTYPE_PALETTE;/*convertsRGB = true;*/break;
		case TEXTYPE_SRGB_RGBA:    textype = TEXTYPE_RGBA   ;convertsRGB = true;break;
		case TEXTYPE_SRGB_BGRA:    textype = TEXTYPE_BGRA   ;convertsRGB = true;break;
		default:
			break;
		}
		if (convertsRGB && data)
		{
			size = width * height * depth * sides * 4;
			if (!temppixels)
			{
				temppixels = (unsigned char *)Mem_Alloc(tempmempool, size);
				memcpy(temppixels, data, size);
				data = temppixels;
			}
			Image_MakeLinearColorsFromsRGB(temppixels, temppixels, width*height*depth*sides);
		}
	}

	texinfo = R_GetTexTypeInfo(textype, flags);
	size = width * height * depth * sides * texinfo->inputbytesperpixel;
	if (size < 1)
	{
		Con_Printf ("R_LoadTexture: bogus texture size (%dx%dx%dx%dbppx%dsides = %d bytes)\n", width, height, depth, texinfo->inputbytesperpixel * 8, sides, size);
		return NULL;
	}

	// clear the alpha flag if the texture has no transparent pixels
	switch(textype)
	{
	case TEXTYPE_PALETTE:
	case TEXTYPE_SRGB_PALETTE:
		if (flags & TEXF_ALPHA)
		{
			flags &= ~TEXF_ALPHA;
			if (data)
			{
				for (i = 0;i < size;i++)
				{
					if (((unsigned char *)&palette[data[i]])[3] < 255)
					{
						flags |= TEXF_ALPHA;
						break;
					}
				}
			}
		}
		break;
	case TEXTYPE_RGBA:
	case TEXTYPE_BGRA:
	case TEXTYPE_SRGB_RGBA:
	case TEXTYPE_SRGB_BGRA:
		if (flags & TEXF_ALPHA)
		{
			flags &= ~TEXF_ALPHA;
			if (data)
			{
				for (i = 3;i < size;i += 4)
				{
					if (data[i] < 255)
					{
						flags |= TEXF_ALPHA;
						break;
					}
				}
			}
		}
		break;
	case TEXTYPE_SHADOWMAP16_COMP:
	case TEXTYPE_SHADOWMAP16_RAW:
	case TEXTYPE_SHADOWMAP24_COMP:
	case TEXTYPE_SHADOWMAP24_RAW:
		break;
	case TEXTYPE_DXT1:
	case TEXTYPE_SRGB_DXT1:
		break;
	case TEXTYPE_DXT1A:
	case TEXTYPE_SRGB_DXT1A:
	case TEXTYPE_DXT3:
	case TEXTYPE_SRGB_DXT3:
	case TEXTYPE_DXT5:
	case TEXTYPE_SRGB_DXT5:
		flags |= TEXF_ALPHA;
		break;
	case TEXTYPE_ALPHA:
		flags |= TEXF_ALPHA;
		break;
	case TEXTYPE_COLORBUFFER:
	case TEXTYPE_COLORBUFFER16F:
	case TEXTYPE_COLORBUFFER32F:
		flags |= TEXF_ALPHA;
		break;
	default:
		Sys_Error("R_LoadTexture: unknown texture type");
	}

	texinfo2 = R_GetTexTypeInfo(textype, flags);
	if(size == width * height * depth * sides * texinfo->inputbytesperpixel)
		texinfo = texinfo2;
	else
		Con_Printf ("R_LoadTexture: input size changed after alpha fallback\n");

	glt = (gltexture_t *)Mem_ExpandableArray_AllocRecord(&texturearray);
	if (identifier)
		dp_strlcpy (glt->identifier, identifier, sizeof(glt->identifier));
	glt->pool = pool;
	glt->chain = pool->gltchain;
	pool->gltchain = glt;
	glt->inputwidth = width;
	glt->inputheight = height;
	glt->inputdepth = depth;
	glt->flags = flags;
	glt->miplevel = (miplevel < 0) ? R_PicmipForFlags(flags) : miplevel; // note: if miplevel is -1, we know the texture is in original size and we can picmip it normally
	glt->textype = texinfo;
	glt->texturetype = texturetype;
	glt->inputdatasize = size;
	glt->palette = palette;
	glt->glinternalformat = texinfo->glinternalformat;
	glt->glformat = texinfo->glformat;
	glt->gltype = texinfo->gltype;
	glt->bytesperpixel = texinfo->internalbytesperpixel;
	glt->sides = glt->texturetype == GLTEXTURETYPE_CUBEMAP ? 6 : 1;
	glt->texnum = 0;
	glt->dirty = false;
	glt->glisdepthstencil = false;
	glt->gltexturetypeenum = gltexturetypeenums[glt->texturetype];
	// init the dynamic texture attributes, too [11/22/2007 Black]
	glt->updatecallback = NULL;
	glt->updatecallback_data = NULL;

	GL_Texture_CalcImageSize(glt->texturetype, glt->flags, glt->miplevel, glt->inputwidth, glt->inputheight, glt->inputdepth, &glt->tilewidth, &glt->tileheight, &glt->tiledepth, &glt->miplevels);

	// upload the texture
	// data may be NULL (blank texture for dynamic rendering)
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		qglGenTextures(1, (GLuint *)&glt->texnum);CHECKGLERROR
		break;
	}

	R_UploadFullTexture(glt, data);
	if (glt->flags & TEXF_ALLOWUPDATES)
		glt->bufferpixels = (unsigned char *)Mem_Alloc(texturemempool, glt->tilewidth*glt->tileheight*glt->tiledepth*glt->sides*glt->bytesperpixel);

	glt->buffermodified = false;
	VectorClear(glt->modified_mins);
	VectorClear(glt->modified_maxs);

	// free any temporary processing buffer we allocated...
	if (temppixels)
		Mem_Free(temppixels);

	// texture converting and uploading can take a while, so make sure we're sending keepalives
	// FIXME: this causes rendering during R_Shadow_DrawLights
//	CL_KeepaliveMessage(false);

	return (rtexture_t *)glt;
}

rtexture_t *R_LoadTexture2D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette)
{
	return R_SetupTexture(rtexturepool, identifier, width, height, 1, 1, flags, miplevel, textype, GLTEXTURETYPE_2D, data, palette);
}

rtexture_t *R_LoadTexture3D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, int depth, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette)
{
	return R_SetupTexture(rtexturepool, identifier, width, height, depth, 1, flags, miplevel, textype, GLTEXTURETYPE_3D, data, palette);
}

rtexture_t *R_LoadTextureCubeMap(rtexturepool_t *rtexturepool, const char *identifier, int width, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette)
{
	return R_SetupTexture(rtexturepool, identifier, width, width, 1, 6, flags, miplevel, textype, GLTEXTURETYPE_CUBEMAP, data, palette);
}

rtexture_t *R_LoadTextureShadowMap2D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, textype_t textype, qbool filter)
{
	return R_SetupTexture(rtexturepool, identifier, width, height, 1, 1, TEXF_RENDERTARGET | TEXF_CLAMP | (filter ? TEXF_FORCELINEAR : TEXF_FORCENEAREST), -1, textype, GLTEXTURETYPE_2D, NULL, NULL);
}

rtexture_t *R_LoadTextureRenderBuffer(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, textype_t textype)
{
	gltexture_t *glt;
	gltexturepool_t *pool = (gltexturepool_t *)rtexturepool;
	textypeinfo_t *texinfo;

	if (cls.state == ca_dedicated)
		return NULL;

	texinfo = R_GetTexTypeInfo(textype, TEXF_RENDERTARGET | TEXF_CLAMP);

	glt = (gltexture_t *)Mem_ExpandableArray_AllocRecord(&texturearray);
	if (identifier)
		dp_strlcpy (glt->identifier, identifier, sizeof(glt->identifier));
	glt->pool = pool;
	glt->chain = pool->gltchain;
	pool->gltchain = glt;
	glt->inputwidth = width;
	glt->inputheight = height;
	glt->inputdepth = 1;
	glt->flags = TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_FORCENEAREST;
	glt->miplevel = 0;
	glt->textype = texinfo;
	glt->texturetype = textype;
	glt->inputdatasize = width*height*texinfo->internalbytesperpixel;
	glt->palette = NULL;
	glt->glinternalformat = texinfo->glinternalformat;
	glt->glformat = texinfo->glformat;
	glt->gltype = texinfo->gltype;
	glt->bytesperpixel = texinfo->internalbytesperpixel;
	glt->sides = glt->texturetype == GLTEXTURETYPE_CUBEMAP ? 6 : 1;
	glt->texnum = 0;
	glt->dirty = false;
	glt->glisdepthstencil = textype == TEXTYPE_DEPTHBUFFER24STENCIL8;
	glt->gltexturetypeenum = GL_TEXTURE_2D;
	// init the dynamic texture attributes, too [11/22/2007 Black]
	glt->updatecallback = NULL;
	glt->updatecallback_data = NULL;

	GL_Texture_CalcImageSize(glt->texturetype, glt->flags, glt->miplevel, glt->inputwidth, glt->inputheight, glt->inputdepth, &glt->tilewidth, &glt->tileheight, &glt->tiledepth, &glt->miplevels);

	// upload the texture
	// data may be NULL (blank texture for dynamic rendering)
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		qglGenRenderbuffers(1, (GLuint *)&glt->renderbuffernum);CHECKGLERROR
		qglBindRenderbuffer(GL_RENDERBUFFER, glt->renderbuffernum);CHECKGLERROR
		qglRenderbufferStorage(GL_RENDERBUFFER, glt->glinternalformat, glt->tilewidth, glt->tileheight);CHECKGLERROR
		// note we can query the renderbuffer for info with glGetRenderbufferParameteriv for GL_WIDTH, GL_HEIGHt, GL_RED_SIZE, GL_GREEN_SIZE, GL_BLUE_SIZE, GL_GL_ALPHA_SIZE, GL_DEPTH_SIZE, GL_STENCIL_SIZE, GL_INTERNAL_FORMAT
		qglBindRenderbuffer(GL_RENDERBUFFER, 0);CHECKGLERROR
		break;
	}

	return (rtexture_t *)glt;
}

int R_SaveTextureDDSFile(rtexture_t *rt, const char *filename, qbool skipuncompressed, qbool hasalpha)
{
#ifdef USE_GLES2
	return -1; // unsupported on this platform
#else
	gltexture_t *glt = (gltexture_t *)rt;
	unsigned char *dds;
	int oldbindtexnum;
	int bytesperpixel = 0;
	int bytesperblock = 0;
	int dds_flags;
	int dds_format_flags;
	int dds_caps1;
	int dds_caps2;
	int ret;
	int mip;
	int mipmaps;
	int mipinfo[16][4];
	int ddssize = 128;
	GLint internalformat;
	const char *ddsfourcc;
	if (!rt)
		return -1; // NULL pointer
	if (!strcmp(gl_version, "2.0.5885 WinXP Release"))
		return -2; // broken driver - crashes on reading internal format
	if (!qglGetTexLevelParameteriv)
		return -2;
	GL_ActiveTexture(0);
	oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);
	qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR
	qglGetTexLevelParameteriv(gltexturetypeenums[glt->texturetype], 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
	switch(internalformat)
	{
	default: ddsfourcc = NULL;bytesperpixel = 4;break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: ddsfourcc = "DXT1";bytesperblock = 8;break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: ddsfourcc = "DXT3";bytesperblock = 16;break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: ddsfourcc = "DXT5";bytesperblock = 16;break;
	}
	// if premultiplied alpha, say so in the DDS file
	if(glt->flags & TEXF_RGBMULTIPLYBYALPHA)
	{
		switch(internalformat)
		{
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: ddsfourcc = "DXT2";break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: ddsfourcc = "DXT4";break;
		}
	}
	if (!bytesperblock && skipuncompressed)
		return -3; // skipped
	memset(mipinfo, 0, sizeof(mipinfo));
	mipinfo[0][0] = glt->tilewidth;
	mipinfo[0][1] = glt->tileheight;
	mipmaps = 1;
	if ((glt->flags & TEXF_MIPMAP) && !(glt->tilewidth == 1 && glt->tileheight == 1))
	{
		for (mip = 1;mip < 16;mip++)
		{
			mipinfo[mip][0] = mipinfo[mip-1][0] > 1 ? mipinfo[mip-1][0] >> 1 : 1;
			mipinfo[mip][1] = mipinfo[mip-1][1] > 1 ? mipinfo[mip-1][1] >> 1 : 1;
			if (mipinfo[mip][0] == 1 && mipinfo[mip][1] == 1)
			{
				mip++;
				break;
			}
		}
		mipmaps = mip;
	}
	for (mip = 0;mip < mipmaps;mip++)
	{
		mipinfo[mip][2] = bytesperblock ? ((mipinfo[mip][0]+3)/4)*((mipinfo[mip][1]+3)/4)*bytesperblock : mipinfo[mip][0]*mipinfo[mip][1]*bytesperpixel;
		mipinfo[mip][3] = ddssize;
		ddssize += mipinfo[mip][2];
	}
	dds = (unsigned char *)Mem_Alloc(tempmempool, ddssize);
	if (!dds)
		return -4;
	dds_caps1 = 0x1000; // DDSCAPS_TEXTURE
	dds_caps2 = 0;
	if (bytesperblock)
	{
		dds_flags = 0x81007; // DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_LINEARSIZE
		dds_format_flags = 0x4; // DDPF_FOURCC
	}
	else
	{
		dds_flags = 0x100F; // DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH
		dds_format_flags = 0x40; // DDPF_RGB
	}
	if (mipmaps)
	{
		dds_flags |= 0x20000; // DDSD_MIPMAPCOUNT
		dds_caps1 |= 0x400008; // DDSCAPS_MIPMAP | DDSCAPS_COMPLEX
	}
	if(hasalpha)
		dds_format_flags |= 0x1; // DDPF_ALPHAPIXELS
	memcpy(dds, "DDS ", 4);
	StoreLittleLong(dds+4, 124); // http://msdn.microsoft.com/en-us/library/bb943982%28v=vs.85%29.aspx says so
	StoreLittleLong(dds+8, dds_flags);
	StoreLittleLong(dds+12, mipinfo[0][1]); // height
	StoreLittleLong(dds+16, mipinfo[0][0]); // width
	StoreLittleLong(dds+24, 0); // depth
	StoreLittleLong(dds+28, mipmaps); // mipmaps
	StoreLittleLong(dds+76, 32); // format size
	StoreLittleLong(dds+80, dds_format_flags);
	StoreLittleLong(dds+108, dds_caps1);
	StoreLittleLong(dds+112, dds_caps2);
	if (bytesperblock)
	{
		StoreLittleLong(dds+20, mipinfo[0][2]); // linear size
		memcpy(dds+84, ddsfourcc, 4);
		for (mip = 0;mip < mipmaps;mip++)
		{
			qglGetCompressedTexImage(gltexturetypeenums[glt->texturetype], mip, dds + mipinfo[mip][3]);CHECKGLERROR
		}
	}
	else
	{
		StoreLittleLong(dds+20, mipinfo[0][0]*bytesperpixel); // pitch
		StoreLittleLong(dds+88, bytesperpixel*8); // bits per pixel
		dds[94] = dds[97] = dds[100] = dds[107] = 255; // bgra byte order masks
		for (mip = 0;mip < mipmaps;mip++)
		{
			qglGetTexImage(gltexturetypeenums[glt->texturetype], mip, GL_BGRA, GL_UNSIGNED_BYTE, dds + mipinfo[mip][3]);CHECKGLERROR
		}
	}
	qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
	ret = FS_WriteFile(filename, dds, ddssize);
	Mem_Free(dds);
	return ret ? ddssize : -5;
#endif
}

#if !defined(_NOKTX)
#ifdef __ANDROID__
// ELUAN: FIXME: separate this code
#include "ktx10/include/ktx.h"
#endif

rtexture_t *R_LoadTextureDDSFile(rtexturepool_t *rtexturepool, const char *filename, qbool srgb, int flags, qbool *hasalphaflag, float *avgcolor, int miplevel, qbool optionaltexture) // DDS textures are opaque, so miplevel isn't a pointer but just seen as a hint
{
	int i, size, dds_format_flags, dds_miplevels, dds_width, dds_height;
	//int dds_flags;
	textype_t textype;
	int bytesperblock, bytesperpixel;
	int mipcomplete;
	gltexture_t *glt;
	gltexturepool_t *pool = (gltexturepool_t *)rtexturepool;
	textypeinfo_t *texinfo;
	int mip, mipwidth, mipheight, mipsize, mipsize_total;
	unsigned int c, r, g, b;
	GLint oldbindtexnum = 0;
	unsigned char *mippixels;
	unsigned char *mippixels_start;
	unsigned char *ddspixels;
	unsigned char *dds;
	fs_offset_t ddsfilesize;
	unsigned int ddssize;
	qbool force_swdecode;
#ifdef __ANDROID__
	// ELUAN: FIXME: separate this code
	char vabuf[1024];
	char vabuf2[1024];
	int strsize;
	KTX_dimensions sizes;
    #endif

	if (cls.state == ca_dedicated)
		return NULL;

#ifdef __ANDROID__
	// ELUAN: FIXME: separate this code
	if (vid.renderpath != RENDERPATH_GLES2)
	{
		Con_DPrintf("KTX texture format is only supported on the GLES2 renderpath\n");
		return NULL;
	}

	// some textures are specified with extensions, so it becomes .tga.dds
	FS_StripExtension (filename, vabuf2, sizeof(vabuf2));
	FS_StripExtension (vabuf2, vabuf, sizeof(vabuf));
	FS_DefaultExtension (vabuf, ".ktx", sizeof(vabuf));
	strsize = strlen(vabuf);
	if (strsize > 5)
	for (i = 0; i <= strsize - 4; i++) // copy null termination
		vabuf[i] = vabuf[i + 4];

	Con_DPrintf("Loading %s...\n", vabuf);
	dds = FS_LoadFile(vabuf, tempmempool, true, &ddsfilesize);
	ddssize = ddsfilesize;

	if (!dds)
	{
		Con_DPrintf("Not found!\n");
		return NULL; // not found
	}
	Con_DPrintf("Found!\n");

	if (flags & TEXF_ALPHA)
	{
		Con_DPrintf("KTX texture with alpha not supported yet, disabling\n");
		flags &= ~TEXF_ALPHA;
	}

	{
		GLenum target;
		GLenum glerror;
		GLboolean isMipmapped;
		KTX_error_code ktxerror;

		glt = (gltexture_t *)Mem_ExpandableArray_AllocRecord(&texturearray);

		// texture uploading can take a while, so make sure we're sending keepalives
		CL_KeepaliveMessage(false);

		// create the texture object
		CHECKGLERROR
		GL_ActiveTexture(0);
		oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[GLTEXTURETYPE_2D]);
		qglGenTextures(1, (GLuint *)&glt->texnum);CHECKGLERROR
		qglBindTexture(gltexturetypeenums[GLTEXTURETYPE_2D], glt->texnum);CHECKGLERROR

		// upload the texture
		// we need to restore the texture binding after finishing the upload

		// NOTE: some drivers fail with ETC1 NPOT (only PowerVR?). This may make the driver crash later.
		ktxerror = ktxLoadTextureM(dds, ddssize, &glt->texnum, &target, &sizes, &isMipmapped, &glerror,
								0, NULL);// can't CHECKGLERROR, the lib catches it

		// FIXME: delete texture if we fail here
		if (target != GL_TEXTURE_2D)
		{
			qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
			Mem_Free(dds);
			Con_DPrintf("%s target != GL_TEXTURE_2D, target == %x\n", vabuf, target);
			return NULL; // FIXME: delete the texture from memory
		}

		if (KTX_SUCCESS == ktxerror)
		{
			textype = TEXTYPE_ETC1;
			flags &= ~TEXF_COMPRESS; // don't let the textype be wrong

			// return whether this texture is transparent
			if (hasalphaflag)
				*hasalphaflag = (flags & TEXF_ALPHA) != 0;

			// TODO: apply gl_picmip
			// TODO: avgcolor
			// TODO: srgb
			// TODO: only load mipmaps if requested

			if (isMipmapped)
				flags |= TEXF_MIPMAP;
			else
				flags &= ~TEXF_MIPMAP;

			texinfo = R_GetTexTypeInfo(textype, flags);

			dp_strlcpy(glt->identifier, vabuf, sizeof(glt->identifier));
			glt->pool = pool;
			glt->chain = pool->gltchain;
			pool->gltchain = glt;
			glt->inputwidth = sizes.width;
			glt->inputheight = sizes.height;
			glt->inputdepth = 1;
			glt->flags = flags;
			glt->textype = texinfo;
			glt->texturetype = GLTEXTURETYPE_2D;
			glt->inputdatasize = ddssize;
			glt->glinternalformat = texinfo->glinternalformat;
			glt->glformat = texinfo->glformat;
			glt->gltype = texinfo->gltype;
			glt->bytesperpixel = texinfo->internalbytesperpixel;
			glt->sides = 1;
			glt->gltexturetypeenum = gltexturetypeenums[glt->texturetype];
			glt->tilewidth = sizes.width;
			glt->tileheight = sizes.height;
			glt->tiledepth = 1;
			glt->miplevels = isMipmapped ? 1 : 0; // FIXME

				// after upload we have to set some parameters...
#ifdef GL_TEXTURE_MAX_LEVEL
			/* FIXME
				if (dds_miplevels >= 1 && !mipcomplete)
				{
					// need to set GL_TEXTURE_MAX_LEVEL
					qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MAX_LEVEL, dds_miplevels - 1);CHECKGLERROR
				}
			*/
#endif
				GL_SetupTextureParameters(glt->flags, glt->textype->textype, glt->texturetype);

				qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
				Mem_Free(dds);
				return (rtexture_t *)glt;
		}
		else
		{
			qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
			Mem_Free(dds);
			Con_DPrintf("KTX texture %s failed to load: %x\n", vabuf, ktxerror);
			return NULL;
		}
	}
#endif // __ANDROID__

	dds = FS_LoadFile(filename, tempmempool, true, &ddsfilesize);
	ddssize = ddsfilesize;

	if (!dds)
	{
		if (r_texture_dds_load_logfailure.integer && (r_texture_dds_load_logfailure.integer >= 2 || !optionaltexture))
			Log_Printf("ddstexturefailures.log", "%s\n", filename);
		return NULL; // not found
	}

	if (ddsfilesize <= 128 || memcmp(dds, "DDS ", 4) || ddssize < (unsigned int)BuffLittleLong(dds+4) || BuffLittleLong(dds+76) != 32)
	{
		Mem_Free(dds);
		Con_Printf("^1%s: not a DDS image\n", filename);
		return NULL;
	}

	//dds_flags = BuffLittleLong(dds+8);
	dds_format_flags = BuffLittleLong(dds+80);
	dds_miplevels = (BuffLittleLong(dds+108) & 0x400000) ? BuffLittleLong(dds+28) : 1;
	dds_width = BuffLittleLong(dds+16);
	dds_height = BuffLittleLong(dds+12);
	ddspixels = dds + 128;

	if(r_texture_dds_load_alphamode.integer == 0)
		if(!(dds_format_flags & 0x1)) // DDPF_ALPHAPIXELS
			flags &= ~TEXF_ALPHA;

	//flags &= ~TEXF_ALPHA; // disabled, as we DISABLE TEXF_ALPHA in the alpha detection, not enable it!
	if ((dds_format_flags & 0x40) && BuffLittleLong(dds+88) == 32)
	{
		// very sloppy BGRA 32bit identification
		textype = TEXTYPE_BGRA;
		flags &= ~TEXF_COMPRESS; // don't let the textype be wrong
		bytesperblock = 0;
		bytesperpixel = 4;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(dds_width, dds_height), bytesperpixel);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid BGRA DDS image\n", filename);
			return NULL;
		}
		if((r_texture_dds_load_alphamode.integer == 1) && (flags & TEXF_ALPHA))
		{
			// check alpha
			for (i = 3;i < size;i += 4)
				if (ddspixels[i] < 255)
					break;
			if (i >= size)
				flags &= ~TEXF_ALPHA;
		}
	}
	else if (!memcmp(dds+84, "DXT1", 4))
	{
		// we need to find out if this is DXT1 (opaque) or DXT1A (transparent)
		// LadyHavoc: it is my belief that this does not infringe on the
		// patent because it is not decoding pixels...
		textype = TEXTYPE_DXT1;
		bytesperblock = 8;
		bytesperpixel = 0;
		//size = ((dds_width+3)/4)*((dds_height+3)/4)*bytesperblock;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_width, 3), 4), INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_height, 3), 4)), bytesperblock);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid DXT1 DDS image\n", filename);
			return NULL;
		}
		if (flags & TEXF_ALPHA)
		{
			if (r_texture_dds_load_alphamode.integer == 1)
			{
				// check alpha
				for (i = 0;i < size;i += bytesperblock)
					if (ddspixels[i+0] + ddspixels[i+1] * 256 <= ddspixels[i+2] + ddspixels[i+3] * 256)
					{
						// NOTE: this assumes sizeof(unsigned int) == 4
						unsigned int data = * (unsigned int *) &(ddspixels[i+4]);
						// check if data, in base 4, contains a digit 3 (DXT1: transparent pixel)
						if(data & (data<<1) & 0xAAAAAAAA)//rgh
							break;
					}
				if (i < size)
					textype = TEXTYPE_DXT1A;
				else
					flags &= ~TEXF_ALPHA;
			}
			else if (r_texture_dds_load_alphamode.integer == 0)
				textype = TEXTYPE_DXT1A;
			else
			{
				flags &= ~TEXF_ALPHA;
			}
		}
	}
	else if (!memcmp(dds+84, "DXT3", 4) || !memcmp(dds+84, "DXT2", 4))
	{
		if(!memcmp(dds+84, "DXT2", 4))
		{
			if(!(flags & TEXF_RGBMULTIPLYBYALPHA))
			{
				Con_Printf("^1%s: expecting DXT3 image without premultiplied alpha, got DXT2 image with premultiplied alpha\n", filename);
			}
		}
		else
		{
			if(flags & TEXF_RGBMULTIPLYBYALPHA)
			{
				Con_Printf("^1%s: expecting DXT2 image without premultiplied alpha, got DXT3 image without premultiplied alpha\n", filename);
			}
		}
		textype = TEXTYPE_DXT3;
		bytesperblock = 16;
		bytesperpixel = 0;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_width, 3), 4), INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_height, 3), 4)), bytesperblock);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid DXT3 DDS image\n", filename);
			return NULL;
		}
		// we currently always assume alpha
	}
	else if (!memcmp(dds+84, "DXT5", 4) || !memcmp(dds+84, "DXT4", 4))
	{
		if(!memcmp(dds+84, "DXT4", 4))
		{
			if(!(flags & TEXF_RGBMULTIPLYBYALPHA))
			{
				Con_Printf("^1%s: expecting DXT5 image without premultiplied alpha, got DXT4 image with premultiplied alpha\n", filename);
			}
		}
		else
		{
			if(flags & TEXF_RGBMULTIPLYBYALPHA)
			{
				Con_Printf("^1%s: expecting DXT4 image without premultiplied alpha, got DXT5 image without premultiplied alpha\n", filename);
			}
		}
		textype = TEXTYPE_DXT5;
		bytesperblock = 16;
		bytesperpixel = 0;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_width, 3), 4), INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_height, 3), 4)), bytesperblock);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid DXT5 DDS image\n", filename);
			return NULL;
		}
		// we currently always assume alpha
	}
	else
	{
		Mem_Free(dds);
		Con_Printf("^1%s: unrecognized/unsupported DDS format\n", filename);
		return NULL;
	}

	// when requesting a non-alpha texture and we have DXT3/5, convert to DXT1
	if(!(flags & TEXF_ALPHA) && (textype == TEXTYPE_DXT3 || textype == TEXTYPE_DXT5))
	{
		textype = TEXTYPE_DXT1;
		bytesperblock = 8;
		ddssize -= 128;
		ddssize /= 2;
		for (i = 0;i < (int)ddssize;i += bytesperblock)
			memcpy(&ddspixels[i], &ddspixels[(i<<1)+8], 8);
		ddssize += 128;
	}

	force_swdecode = false;
	if(bytesperblock)
	{
		if(vid.support.ext_texture_compression_s3tc)
		{
			if(r_texture_dds_swdecode.integer > 1)
				force_swdecode = true;
		}
		else
		{
			if(r_texture_dds_swdecode.integer < 1)
			{
				// unsupported
				Mem_Free(dds);
				return NULL;
			}
			force_swdecode = true;
		}
	}

	// return whether this texture is transparent
	if (hasalphaflag)
		*hasalphaflag = (flags & TEXF_ALPHA) != 0;

	// if we SW decode, choose 2 sizes bigger
	if(force_swdecode)
	{
		// this is quarter res, so do not scale down more than we have to
		miplevel -= 2;

		if(miplevel < 0)
			Con_DPrintf("WARNING: fake software decoding of compressed texture %s degraded quality\n", filename);
	}

	// this is where we apply gl_picmip
	mippixels_start = ddspixels;
	mipwidth = dds_width;
	mipheight = dds_height;
	while(miplevel >= 1 && dds_miplevels >= 1)
	{
		if (mipwidth <= 1 && mipheight <= 1)
			break;
		mipsize = bytesperblock ? ((mipwidth+3)/4)*((mipheight+3)/4)*bytesperblock : mipwidth*mipheight*bytesperpixel;
		mippixels_start += mipsize; // just skip
		--dds_miplevels;
		--miplevel;
		if (mipwidth > 1)
			mipwidth >>= 1;
		if (mipheight > 1)
			mipheight >>= 1;
	}
	mipsize_total = ddssize - 128 - (mippixels_start - ddspixels);
	mipsize = bytesperblock ? ((mipwidth+3)/4)*((mipheight+3)/4)*bytesperblock : mipwidth*mipheight*bytesperpixel;

	// from here on, we do not need the ddspixels and ddssize any more (apart from the statistics entry in glt)

	// fake decode S3TC if needed
	if(force_swdecode)
	{
		int mipsize_new = mipsize_total / bytesperblock * 4;
		unsigned char *mipnewpixels = (unsigned char *) Mem_Alloc(tempmempool, mipsize_new);
		unsigned char *p = mipnewpixels;
		for (i = bytesperblock == 16 ? 8 : 0;i < (int)mipsize_total;i += bytesperblock, p += 4)
		{
			// UBSan: unsigned literals because promotion to int causes signed overflow when mippixels_start >= 128
			c = mippixels_start[i] + 256u*mippixels_start[i+1] + 65536u*mippixels_start[i+2] + 16777216u*mippixels_start[i+3];
			p[2] = (((c >> 11) & 0x1F) + ((c >> 27) & 0x1F)) * (0.5f / 31.0f * 255.0f);
			p[1] = (((c >>  5) & 0x3F) + ((c >> 21) & 0x3F)) * (0.5f / 63.0f * 255.0f);
			p[0] = (((c      ) & 0x1F) + ((c >> 16) & 0x1F)) * (0.5f / 31.0f * 255.0f);
			if(textype == TEXTYPE_DXT5)
				p[3] = (0.5 * mippixels_start[i-8] + 0.5 * mippixels_start[i-7]);
			else if(textype == TEXTYPE_DXT3)
				p[3] = (
					  (mippixels_start[i-8] & 0x0F)
					+ (mippixels_start[i-8] >> 4)
					+ (mippixels_start[i-7] & 0x0F)
					+ (mippixels_start[i-7] >> 4)
					+ (mippixels_start[i-6] & 0x0F)
					+ (mippixels_start[i-6] >> 4)
					+ (mippixels_start[i-5] & 0x0F)
					+ (mippixels_start[i-5] >> 4)
				       ) * (0.125f / 15.0f * 255.0f);
			else
				p[3] = 255;
		}

		textype = TEXTYPE_BGRA;
		bytesperblock = 0;
		bytesperpixel = 4;

		// as each block becomes a pixel, we must use pixel count for this
		mipwidth = (mipwidth + 3) / 4;
		mipheight = (mipheight + 3) / 4;
		mipsize = bytesperpixel * mipwidth * mipheight;
		mippixels_start = mipnewpixels;
		mipsize_total = mipsize_new;
	}

	// start mip counting
	mippixels = mippixels_start;

	// calculate average color if requested
	if (avgcolor)
	{
		float f;
		Vector4Clear(avgcolor);
		if (bytesperblock)
		{
			for (i = bytesperblock == 16 ? 8 : 0;i < mipsize;i += bytesperblock)
			{
				// UBSan: unsigned literals because promotion to int causes signed overflow when mippixels >= 128
				c = mippixels[i] + 256u*mippixels[i+1] + 65536u*mippixels[i+2] + 16777216u*mippixels[i+3];
				avgcolor[0] += ((c >> 11) & 0x1F) + ((c >> 27) & 0x1F);
				avgcolor[1] += ((c >>  5) & 0x3F) + ((c >> 21) & 0x3F);
				avgcolor[2] += ((c      ) & 0x1F) + ((c >> 16) & 0x1F);
				if(textype == TEXTYPE_DXT5)
					avgcolor[3] += (mippixels[i-8] + (int) mippixels[i-7]) * (0.5f / 255.0f);
				else if(textype == TEXTYPE_DXT3)
					avgcolor[3] += (
						  (mippixels_start[i-8] & 0x0F)
						+ (mippixels_start[i-8] >> 4)
						+ (mippixels_start[i-7] & 0x0F)
						+ (mippixels_start[i-7] >> 4)
						+ (mippixels_start[i-6] & 0x0F)
						+ (mippixels_start[i-6] >> 4)
						+ (mippixels_start[i-5] & 0x0F)
						+ (mippixels_start[i-5] >> 4)
					       ) * (0.125f / 15.0f);
				else
					avgcolor[3] += 1.0f;
			}
			f = (float)bytesperblock / mipsize;
			avgcolor[0] *= (0.5f / 31.0f) * f;
			avgcolor[1] *= (0.5f / 63.0f) * f;
			avgcolor[2] *= (0.5f / 31.0f) * f;
			avgcolor[3] *= f;
		}
		else
		{
			for (i = 0;i < mipsize;i += 4)
			{
				avgcolor[0] += mippixels[i+2];
				avgcolor[1] += mippixels[i+1];
				avgcolor[2] += mippixels[i];
				avgcolor[3] += mippixels[i+3];
			}
			f = (1.0f / 255.0f) * bytesperpixel / mipsize;
			avgcolor[0] *= f;
			avgcolor[1] *= f;
			avgcolor[2] *= f;
			avgcolor[3] *= f;
		}
	}

	// if we want sRGB, convert now
	if(srgb)
	{
		if (vid.support.ext_texture_srgb)
		{
			switch(textype)
			{
			case TEXTYPE_DXT1:    textype = TEXTYPE_SRGB_DXT1   ;break;
			case TEXTYPE_DXT1A:   textype = TEXTYPE_SRGB_DXT1A  ;break;
			case TEXTYPE_DXT3:    textype = TEXTYPE_SRGB_DXT3   ;break;
			case TEXTYPE_DXT5:    textype = TEXTYPE_SRGB_DXT5   ;break;
			case TEXTYPE_RGBA:    textype = TEXTYPE_SRGB_RGBA   ;break;
			default:
				break;
			}
		}
		else
		{
			switch(textype)
			{
			case TEXTYPE_DXT1:
			case TEXTYPE_DXT1A:
			case TEXTYPE_DXT3:
			case TEXTYPE_DXT5:
				{
					for (i = bytesperblock == 16 ? 8 : 0;i < mipsize_total;i += bytesperblock)
					{
						int c0, c1, c0new, c1new;
						c0 = mippixels_start[i] + 256*mippixels_start[i+1];
						r = ((c0 >> 11) & 0x1F);
						g = ((c0 >>  5) & 0x3F);
						b = ((c0      ) & 0x1F);
						r = floor(Image_LinearFloatFromsRGB(r * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						g = floor(Image_LinearFloatFromsRGB(g * (255.0f / 63.0f)) * 63.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						b = floor(Image_LinearFloatFromsRGB(b * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						c0new = (r << 11) | (g << 5) | b;
						c1 = mippixels_start[i+2] + 256*mippixels_start[i+3];
						r = ((c1 >> 11) & 0x1F);
						g = ((c1 >>  5) & 0x3F);
						b = ((c1      ) & 0x1F);
						r = floor(Image_LinearFloatFromsRGB(r * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						g = floor(Image_LinearFloatFromsRGB(g * (255.0f / 63.0f)) * 63.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						b = floor(Image_LinearFloatFromsRGB(b * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						c1new = (r << 11) | (g << 5) | b;
						// swap the colors if needed to fix order
						if(c0 > c1) // thirds
						{
							if(c0new < c1new)
							{
								c = c0new;
								c0new = c1new;
								c1new = c;
								if(c0new == c1new)
								mippixels_start[i+4] ^= 0x55;
								mippixels_start[i+5] ^= 0x55;
								mippixels_start[i+6] ^= 0x55;
								mippixels_start[i+7] ^= 0x55;
							}
							else if(c0new == c1new)
							{
								mippixels_start[i+4] = 0x00;
								mippixels_start[i+5] = 0x00;
								mippixels_start[i+6] = 0x00;
								mippixels_start[i+7] = 0x00;
							}
						}
						else // half + transparent
						{
							if(c0new > c1new)
							{
								c = c0new;
								c0new = c1new;
								c1new = c;
								mippixels_start[i+4] ^= (~mippixels_start[i+4] >> 1) & 0x55;
								mippixels_start[i+5] ^= (~mippixels_start[i+5] >> 1) & 0x55;
								mippixels_start[i+6] ^= (~mippixels_start[i+6] >> 1) & 0x55;
								mippixels_start[i+7] ^= (~mippixels_start[i+7] >> 1) & 0x55;
							}
						}
						mippixels_start[i] = c0new & 255;
						mippixels_start[i+1] = c0new >> 8;
						mippixels_start[i+2] = c1new & 255;
						mippixels_start[i+3] = c1new >> 8;
					}
				}
				break;
			case TEXTYPE_RGBA:
				Image_MakeLinearColorsFromsRGB(mippixels, mippixels, mipsize_total / bytesperblock);
				break;
			default:
				break;
			}
		}
	}

	// when not requesting mipmaps, do not load them
	if(!(flags & TEXF_MIPMAP))
		dds_miplevels = 0;

	if (dds_miplevels >= 1)
		flags |= TEXF_MIPMAP;
	else
		flags &= ~TEXF_MIPMAP;

	texinfo = R_GetTexTypeInfo(textype, flags);

	glt = (gltexture_t *)Mem_ExpandableArray_AllocRecord(&texturearray);
	dp_strlcpy (glt->identifier, filename, sizeof(glt->identifier));
	glt->pool = pool;
	glt->chain = pool->gltchain;
	pool->gltchain = glt;
	glt->inputwidth = mipwidth;
	glt->inputheight = mipheight;
	glt->inputdepth = 1;
	glt->flags = flags;
	glt->textype = texinfo;
	glt->texturetype = GLTEXTURETYPE_2D;
	glt->inputdatasize = ddssize;
	glt->glinternalformat = texinfo->glinternalformat;
	glt->glformat = texinfo->glformat;
	glt->gltype = texinfo->gltype;
	glt->bytesperpixel = texinfo->internalbytesperpixel;
	glt->sides = 1;
	glt->gltexturetypeenum = gltexturetypeenums[glt->texturetype];
	glt->tilewidth = mipwidth;
	glt->tileheight = mipheight;
	glt->tiledepth = 1;
	glt->miplevels = dds_miplevels;

	// texture uploading can take a while, so make sure we're sending keepalives
	CL_KeepaliveMessage(false);

	// create the texture object
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		CHECKGLERROR
		GL_ActiveTexture(0);
		oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);
		qglGenTextures(1, (GLuint *)&glt->texnum);CHECKGLERROR
		qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR
		break;
	}

	// upload the texture
	// we need to restore the texture binding after finishing the upload
	mipcomplete = false;

	for (mip = 0;mip <= dds_miplevels;mip++) // <= to include the not-counted "largest" miplevel
	{
		unsigned char *upload_mippixels = mippixels;
		int upload_mipwidth = mipwidth;
		int upload_mipheight = mipheight;
		mipsize = bytesperblock ? ((mipwidth+3)/4)*((mipheight+3)/4)*bytesperblock : mipwidth*mipheight*bytesperpixel;
		if (mippixels + mipsize > mippixels_start + mipsize_total)
			break;
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			if (bytesperblock)
			{
				qglCompressedTexImage2D(GL_TEXTURE_2D, mip, glt->glinternalformat, upload_mipwidth, upload_mipheight, 0, mipsize, upload_mippixels);CHECKGLERROR
			}
			else
			{
				qglTexImage2D(GL_TEXTURE_2D, mip, glt->glinternalformat, upload_mipwidth, upload_mipheight, 0, glt->glformat, glt->gltype, upload_mippixels);CHECKGLERROR
			}
			break;
		}
		if(upload_mippixels != mippixels)
			Mem_Free(upload_mippixels);
		mippixels += mipsize;
		if (mipwidth <= 1 && mipheight <= 1)
		{
			mipcomplete = true;
			break;
		}
		if (mipwidth > 1)
			mipwidth >>= 1;
		if (mipheight > 1)
			mipheight >>= 1;
	}

	// after upload we have to set some parameters...
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
#ifdef GL_TEXTURE_MAX_LEVEL
		if (dds_miplevels >= 1 && !mipcomplete)
		{
			// need to set GL_TEXTURE_MAX_LEVEL
			qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MAX_LEVEL, dds_miplevels - 1);CHECKGLERROR
		}
#endif
		GL_SetupTextureParameters(glt->flags, glt->textype->textype, glt->texturetype);
		qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
		break;
	}

	Mem_Free(dds);
	if(force_swdecode)
		Mem_Free((unsigned char *) mippixels_start);
	return (rtexture_t *)glt;
}
#else
rtexture_t *R_LoadTextureDDSFile(rtexturepool_t *rtexturepool, const char *filename, qbool srgb, int flags, qbool *hasalphaflag, float *avgcolor, int miplevel, qbool optionaltexture) // DDS textures are opaque, so miplevel isn't a pointer but just seen as a hint
{
	int i, size, dds_format_flags, dds_miplevels, dds_width, dds_height;
	//int dds_flags;
	textype_t textype;
	int bytesperblock, bytesperpixel;
	int mipcomplete;
	gltexture_t *glt;
	gltexturepool_t *pool = (gltexturepool_t *)rtexturepool;
	textypeinfo_t *texinfo;
	int mip, mipwidth, mipheight, mipsize, mipsize_total;
	unsigned int c, r, g, b;
	GLint oldbindtexnum = 0;
	unsigned char *mippixels;
	unsigned char *mippixels_start;
	unsigned char *ddspixels;
	unsigned char *dds;
	fs_offset_t ddsfilesize;
	unsigned int ddssize;
	qbool force_swdecode, npothack;

	if (cls.state == ca_dedicated)
		return NULL;

	dds = FS_LoadFile(filename, tempmempool, true, &ddsfilesize);
	ddssize = ddsfilesize;

	if (!dds)
	{
		if (r_texture_dds_load_logfailure.integer && (r_texture_dds_load_logfailure.integer >= 2 || !optionaltexture))
			Log_Printf("ddstexturefailures.log", "%s\n", filename);
		return NULL; // not found
	}

	if (ddsfilesize <= 128 || memcmp(dds, "DDS ", 4) || ddssize < (unsigned int)BuffLittleLong(dds+4) || BuffLittleLong(dds+76) != 32)
	{
		Mem_Free(dds);
		Con_Printf("^1%s: not a DDS image\n", filename);
		return NULL;
	}

	//dds_flags = BuffLittleLong(dds+8);
	dds_format_flags = BuffLittleLong(dds+80);
	dds_miplevels = (BuffLittleLong(dds+108) & 0x400000) ? BuffLittleLong(dds+28) : 1;
	dds_width = BuffLittleLong(dds+16);
	dds_height = BuffLittleLong(dds+12);
	ddspixels = dds + 128;

	if(r_texture_dds_load_alphamode.integer == 0)
		if(!(dds_format_flags & 0x1)) // DDPF_ALPHAPIXELS
			flags &= ~TEXF_ALPHA;

	//flags &= ~TEXF_ALPHA; // disabled, as we DISABLE TEXF_ALPHA in the alpha detection, not enable it!
	if ((dds_format_flags & 0x40) && BuffLittleLong(dds+88) == 32)
	{
		// very sloppy BGRA 32bit identification
		textype = TEXTYPE_BGRA;
		flags &= ~TEXF_COMPRESS; // don't let the textype be wrong
		bytesperblock = 0;
		bytesperpixel = 4;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(dds_width, dds_height), bytesperpixel);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid BGRA DDS image\n", filename);
			return NULL;
		}
		if((r_texture_dds_load_alphamode.integer == 1) && (flags & TEXF_ALPHA))
		{
			// check alpha
			for (i = 3;i < size;i += 4)
				if (ddspixels[i] < 255)
					break;
			if (i >= size)
				flags &= ~TEXF_ALPHA;
		}
	}
	else if (!memcmp(dds+84, "DXT1", 4))
	{
		// we need to find out if this is DXT1 (opaque) or DXT1A (transparent)
		// LordHavoc: it is my belief that this does not infringe on the
		// patent because it is not decoding pixels...
		textype = TEXTYPE_DXT1;
		bytesperblock = 8;
		bytesperpixel = 0;
		//size = ((dds_width+3)/4)*((dds_height+3)/4)*bytesperblock;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_width, 3), 4), INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_height, 3), 4)), bytesperblock);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid DXT1 DDS image\n", filename);
			return NULL;
		}
		if (flags & TEXF_ALPHA)
		{
			if (r_texture_dds_load_alphamode.integer == 1)
			{
				// check alpha
				for (i = 0;i < size;i += bytesperblock)
					if (ddspixels[i+0] + ddspixels[i+1] * 256 <= ddspixels[i+2] + ddspixels[i+3] * 256)
					{
						// NOTE: this assumes sizeof(unsigned int) == 4
						unsigned int data = * (unsigned int *) &(ddspixels[i+4]);
						// check if data, in base 4, contains a digit 3 (DXT1: transparent pixel)
						if(data & (data<<1) & 0xAAAAAAAA)//rgh
							break;
					}
				if (i < size)
					textype = TEXTYPE_DXT1A;
				else
					flags &= ~TEXF_ALPHA;
			}
			else if (r_texture_dds_load_alphamode.integer == 0)
				textype = TEXTYPE_DXT1A;
			else
			{
				flags &= ~TEXF_ALPHA;
			}
		}
	}
	else if (!memcmp(dds+84, "DXT3", 4) || !memcmp(dds+84, "DXT2", 4))
	{
		if(!memcmp(dds+84, "DXT2", 4))
		{
			if(!(flags & TEXF_RGBMULTIPLYBYALPHA))
			{
				Con_Printf("^1%s: expecting DXT3 image without premultiplied alpha, got DXT2 image with premultiplied alpha\n", filename);
			}
		}
		else
		{
			if(flags & TEXF_RGBMULTIPLYBYALPHA)
			{
				Con_Printf("^1%s: expecting DXT2 image without premultiplied alpha, got DXT3 image without premultiplied alpha\n", filename);
			}
		}
		textype = TEXTYPE_DXT3;
		bytesperblock = 16;
		bytesperpixel = 0;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_width, 3), 4), INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_height, 3), 4)), bytesperblock);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid DXT3 DDS image\n", filename);
			return NULL;
		}
		// we currently always assume alpha
	}
	else if (!memcmp(dds+84, "DXT5", 4) || !memcmp(dds+84, "DXT4", 4))
	{
		if(!memcmp(dds+84, "DXT4", 4))
		{
			if(!(flags & TEXF_RGBMULTIPLYBYALPHA))
			{
				Con_Printf("^1%s: expecting DXT5 image without premultiplied alpha, got DXT4 image with premultiplied alpha\n", filename);
			}
		}
		else
		{
			if(flags & TEXF_RGBMULTIPLYBYALPHA)
			{
				Con_Printf("^1%s: expecting DXT4 image without premultiplied alpha, got DXT5 image without premultiplied alpha\n", filename);
			}
		}
		textype = TEXTYPE_DXT5;
		bytesperblock = 16;
		bytesperpixel = 0;
		size = INTOVERFLOW_MUL(INTOVERFLOW_MUL(INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_width, 3), 4), INTOVERFLOW_DIV(INTOVERFLOW_ADD(dds_height, 3), 4)), bytesperblock);
		if(INTOVERFLOW_ADD(128, size) > INTOVERFLOW_NORMALIZE(ddsfilesize))
		{
			Mem_Free(dds);
			Con_Printf("^1%s: invalid DXT5 DDS image\n", filename);
			return NULL;
		}
		// we currently always assume alpha
	}
	else
	{
		Mem_Free(dds);
		Con_Printf("^1%s: unrecognized/unsupported DDS format\n", filename);
		return NULL;
	}

	// when requesting a non-alpha texture and we have DXT3/5, convert to DXT1
	if(!(flags & TEXF_ALPHA) && (textype == TEXTYPE_DXT3 || textype == TEXTYPE_DXT5))
	{
		textype = TEXTYPE_DXT1;
		bytesperblock = 8;
		ddssize -= 128;
		ddssize /= 2;
		for (i = 0;i < (int)ddssize;i += bytesperblock)
			memcpy(&ddspixels[i], &ddspixels[(i<<1)+8], 8);
		ddssize += 128;
	}

	force_swdecode = false;
	npothack = 
		(!true/*vid.support.arb_texture_non_power_of_two*/ &&
			(
				(dds_width & (dds_width - 1))
				||
				(dds_height & (dds_height - 1))
			)
		);
	if(bytesperblock)
	{
		if(true/*vid.support.arb_texture_compression*/ && vid.support.ext_texture_compression_s3tc && !npothack)
		{
			if(r_texture_dds_swdecode.integer > 1)
				force_swdecode = true;
		}
		else
		{
			if(r_texture_dds_swdecode.integer < 1)
			{
				// unsupported
				Mem_Free(dds);
				return NULL;
			}
			force_swdecode = true;
		}
	}

	// return whether this texture is transparent
	if (hasalphaflag)
		*hasalphaflag = (flags & TEXF_ALPHA) != 0;

	// if we SW decode, choose 2 sizes bigger
	if(force_swdecode)
	{
		// this is quarter res, so do not scale down more than we have to
		miplevel -= 2;

		if(miplevel < 0)
			Con_DPrintf("WARNING: fake software decoding of compressed texture %s degraded quality\n", filename);
	}

	// this is where we apply gl_picmip
	mippixels_start = ddspixels;
	mipwidth = dds_width;
	mipheight = dds_height;
	while(miplevel >= 1 && dds_miplevels >= 1)
	{
		if (mipwidth <= 1 && mipheight <= 1)
			break;
		mipsize = bytesperblock ? ((mipwidth+3)/4)*((mipheight+3)/4)*bytesperblock : mipwidth*mipheight*bytesperpixel;
		mippixels_start += mipsize; // just skip
		--dds_miplevels;
		--miplevel;
		if (mipwidth > 1)
			mipwidth >>= 1;
		if (mipheight > 1)
			mipheight >>= 1;
	}
	mipsize_total = ddssize - 128 - (mippixels_start - ddspixels);
	mipsize = bytesperblock ? ((mipwidth+3)/4)*((mipheight+3)/4)*bytesperblock : mipwidth*mipheight*bytesperpixel;

	// from here on, we do not need the ddspixels and ddssize any more (apart from the statistics entry in glt)

	// fake decode S3TC if needed
	if(force_swdecode)
	{
		int mipsize_new = mipsize_total / bytesperblock * 4;
		unsigned char *mipnewpixels = (unsigned char *) Mem_Alloc(tempmempool, mipsize_new);
		unsigned char *p = mipnewpixels;
		for (i = bytesperblock == 16 ? 8 : 0;i < (int)mipsize_total;i += bytesperblock, p += 4)
		{
			c = mippixels_start[i] + 256*mippixels_start[i+1] + 65536*mippixels_start[i+2] + 16777216*mippixels_start[i+3];
			p[2] = (((c >> 11) & 0x1F) + ((c >> 27) & 0x1F)) * (0.5f / 31.0f * 255.0f);
			p[1] = (((c >>  5) & 0x3F) + ((c >> 21) & 0x3F)) * (0.5f / 63.0f * 255.0f);
			p[0] = (((c      ) & 0x1F) + ((c >> 16) & 0x1F)) * (0.5f / 31.0f * 255.0f);
			if(textype == TEXTYPE_DXT5)
				p[3] = (0.5 * mippixels_start[i-8] + 0.5 * mippixels_start[i-7]);
			else if(textype == TEXTYPE_DXT3)
				p[3] = (
					  (mippixels_start[i-8] & 0x0F)
					+ (mippixels_start[i-8] >> 4)
					+ (mippixels_start[i-7] & 0x0F)
					+ (mippixels_start[i-7] >> 4)
					+ (mippixels_start[i-6] & 0x0F)
					+ (mippixels_start[i-6] >> 4)
					+ (mippixels_start[i-5] & 0x0F)
					+ (mippixels_start[i-5] >> 4)
				       ) * (0.125f / 15.0f * 255.0f);
			else
				p[3] = 255;
		}

		textype = TEXTYPE_BGRA;
		bytesperblock = 0;
		bytesperpixel = 4;

		// as each block becomes a pixel, we must use pixel count for this
		mipwidth = (mipwidth + 3) / 4;
		mipheight = (mipheight + 3) / 4;
		mipsize = bytesperpixel * mipwidth * mipheight;
		mippixels_start = mipnewpixels;
		mipsize_total = mipsize_new;
	}

	// start mip counting
	mippixels = mippixels_start;

	// calculate average color if requested
	if (avgcolor)
	{
		float f;
		Vector4Clear(avgcolor);
		if (bytesperblock)
		{
			for (i = bytesperblock == 16 ? 8 : 0;i < mipsize;i += bytesperblock)
			{
				c = mippixels[i] + 256*mippixels[i+1] + 65536*mippixels[i+2] + 16777216*mippixels[i+3];
				avgcolor[0] += ((c >> 11) & 0x1F) + ((c >> 27) & 0x1F);
				avgcolor[1] += ((c >>  5) & 0x3F) + ((c >> 21) & 0x3F);
				avgcolor[2] += ((c      ) & 0x1F) + ((c >> 16) & 0x1F);
				if(textype == TEXTYPE_DXT5)
					avgcolor[3] += (mippixels[i-8] + (int) mippixels[i-7]) * (0.5f / 255.0f);
				else if(textype == TEXTYPE_DXT3)
					avgcolor[3] += (
						  (mippixels_start[i-8] & 0x0F)
						+ (mippixels_start[i-8] >> 4)
						+ (mippixels_start[i-7] & 0x0F)
						+ (mippixels_start[i-7] >> 4)
						+ (mippixels_start[i-6] & 0x0F)
						+ (mippixels_start[i-6] >> 4)
						+ (mippixels_start[i-5] & 0x0F)
						+ (mippixels_start[i-5] >> 4)
					       ) * (0.125f / 15.0f);
				else
					avgcolor[3] += 1.0f;
			}
			f = (float)bytesperblock / mipsize;
			avgcolor[0] *= (0.5f / 31.0f) * f;
			avgcolor[1] *= (0.5f / 63.0f) * f;
			avgcolor[2] *= (0.5f / 31.0f) * f;
			avgcolor[3] *= f;
		}
		else
		{
			for (i = 0;i < mipsize;i += 4)
			{
				avgcolor[0] += mippixels[i+2];
				avgcolor[1] += mippixels[i+1];
				avgcolor[2] += mippixels[i];
				avgcolor[3] += mippixels[i+3];
			}
			f = (1.0f / 255.0f) * bytesperpixel / mipsize;
			avgcolor[0] *= f;
			avgcolor[1] *= f;
			avgcolor[2] *= f;
			avgcolor[3] *= f;
		}
	}

	// if we want sRGB, convert now
	if(srgb)
	{
		if (vid.support.ext_texture_srgb)
		{
			switch(textype)
			{
			case TEXTYPE_DXT1:    textype = TEXTYPE_SRGB_DXT1   ;break;
			case TEXTYPE_DXT1A:   textype = TEXTYPE_SRGB_DXT1A  ;break;
			case TEXTYPE_DXT3:    textype = TEXTYPE_SRGB_DXT3   ;break;
			case TEXTYPE_DXT5:    textype = TEXTYPE_SRGB_DXT5   ;break;
			case TEXTYPE_RGBA:    textype = TEXTYPE_SRGB_RGBA   ;break;
			default:
				break;
			}
		}
		else
		{
			switch(textype)
			{
			case TEXTYPE_DXT1:
			case TEXTYPE_DXT1A:
			case TEXTYPE_DXT3:
			case TEXTYPE_DXT5:
				{
					for (i = bytesperblock == 16 ? 8 : 0;i < mipsize_total;i += bytesperblock)
					{
						int c0, c1, c0new, c1new;
						c0 = mippixels_start[i] + 256*mippixels_start[i+1];
						r = ((c0 >> 11) & 0x1F);
						g = ((c0 >>  5) & 0x3F);
						b = ((c0      ) & 0x1F);
						r = floor(Image_LinearFloatFromsRGB(r * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						g = floor(Image_LinearFloatFromsRGB(g * (255.0f / 63.0f)) * 63.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						b = floor(Image_LinearFloatFromsRGB(b * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						c0new = (r << 11) | (g << 5) | b;
						c1 = mippixels_start[i+2] + 256*mippixels_start[i+3];
						r = ((c1 >> 11) & 0x1F);
						g = ((c1 >>  5) & 0x3F);
						b = ((c1      ) & 0x1F);
						r = floor(Image_LinearFloatFromsRGB(r * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						g = floor(Image_LinearFloatFromsRGB(g * (255.0f / 63.0f)) * 63.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						b = floor(Image_LinearFloatFromsRGB(b * (255.0f / 31.0f)) * 31.0f + 0.5f); // these multiplications here get combined with multiplications in Image_LinearFloatFromsRGB
						c1new = (r << 11) | (g << 5) | b;
						// swap the colors if needed to fix order
						if(c0 > c1) // thirds
						{
							if(c0new < c1new)
							{
								c = c0new;
								c0new = c1new;
								c1new = c;
								if(c0new == c1new)
								mippixels_start[i+4] ^= 0x55;
								mippixels_start[i+5] ^= 0x55;
								mippixels_start[i+6] ^= 0x55;
								mippixels_start[i+7] ^= 0x55;
							}
							else if(c0new == c1new)
							{
								mippixels_start[i+4] = 0x00;
								mippixels_start[i+5] = 0x00;
								mippixels_start[i+6] = 0x00;
								mippixels_start[i+7] = 0x00;
							}
						}
						else // half + transparent
						{
							if(c0new > c1new)
							{
								c = c0new;
								c0new = c1new;
								c1new = c;
								mippixels_start[i+4] ^= (~mippixels_start[i+4] >> 1) & 0x55;
								mippixels_start[i+5] ^= (~mippixels_start[i+5] >> 1) & 0x55;
								mippixels_start[i+6] ^= (~mippixels_start[i+6] >> 1) & 0x55;
								mippixels_start[i+7] ^= (~mippixels_start[i+7] >> 1) & 0x55;
							}
						}
						mippixels_start[i] = c0new & 255;
						mippixels_start[i+1] = c0new >> 8;
						mippixels_start[i+2] = c1new & 255;
						mippixels_start[i+3] = c1new >> 8;
					}
				}
				break;
			case TEXTYPE_RGBA:
				Image_MakeLinearColorsFromsRGB(mippixels, mippixels, mipsize_total / bytesperblock);
				break;
			default:
				break;
			}
		}
	}

	// when not requesting mipmaps, do not load them
	if(!(flags & TEXF_MIPMAP))
		dds_miplevels = 0;

	if (dds_miplevels >= 1)
		flags |= TEXF_MIPMAP;
	else
		flags &= ~TEXF_MIPMAP;

	texinfo = R_GetTexTypeInfo(textype, flags);

	glt = (gltexture_t *)Mem_ExpandableArray_AllocRecord(&texturearray);
	dp_strlcpy (glt->identifier, filename, sizeof(glt->identifier));
	glt->pool = pool;
	glt->chain = pool->gltchain;
	pool->gltchain = glt;
	glt->inputwidth = mipwidth;
	glt->inputheight = mipheight;
	glt->inputdepth = 1;
	glt->flags = flags;
	glt->textype = texinfo;
	glt->texturetype = GLTEXTURETYPE_2D;
	glt->inputdatasize = ddssize;
	glt->glinternalformat = texinfo->glinternalformat;
	glt->glformat = texinfo->glformat;
	glt->gltype = texinfo->gltype;
	glt->bytesperpixel = texinfo->internalbytesperpixel;
	glt->sides = 1;
	glt->gltexturetypeenum = gltexturetypeenums[glt->texturetype];
	glt->tilewidth = mipwidth;
	glt->tileheight = mipheight;
	glt->tiledepth = 1;
	glt->miplevels = dds_miplevels;

	if(npothack)
	{
		for (glt->tilewidth = 1;glt->tilewidth < mipwidth;glt->tilewidth <<= 1);
		for (glt->tileheight = 1;glt->tileheight < mipheight;glt->tileheight <<= 1);
	}

	// texture uploading can take a while, so make sure we're sending keepalives
	CL_KeepaliveMessage(false);

	// create the texture object
	switch(vid.renderpath)
	{
	case RENDERPATH_GLES2:
		CHECKGLERROR
		GL_ActiveTexture(0);
		oldbindtexnum = R_Mesh_TexBound(0, gltexturetypeenums[glt->texturetype]);
		qglGenTextures(1, (GLuint *)&glt->texnum);CHECKGLERROR
		qglBindTexture(gltexturetypeenums[glt->texturetype], glt->texnum);CHECKGLERROR
		break;
	}

	// upload the texture
	// we need to restore the texture binding after finishing the upload
	mipcomplete = false;

	for (mip = 0;mip <= dds_miplevels;mip++) // <= to include the not-counted "largest" miplevel
	{
		unsigned char *upload_mippixels = mippixels;
		int upload_mipwidth = mipwidth;
		int upload_mipheight = mipheight;
		mipsize = bytesperblock ? ((mipwidth+3)/4)*((mipheight+3)/4)*bytesperblock : mipwidth*mipheight*bytesperpixel;
		if (mippixels + mipsize > mippixels_start + mipsize_total)
			break;
		if(npothack)
		{
			upload_mipwidth = (glt->tilewidth >> mip);
			upload_mipheight = (glt->tileheight >> mip);
			if(upload_mipwidth != mipwidth || upload_mipheight != mipheight)
			// I _think_ they always mismatch, but I was too lazy
			// to properly check, and this test here is really
			// harmless
			{
				upload_mippixels = (unsigned char *) Mem_Alloc(tempmempool, 4 * upload_mipwidth * upload_mipheight);
				Image_Resample32(mippixels, mipwidth, mipheight, 1, upload_mippixels, upload_mipwidth, upload_mipheight, 1, r_lerpimages.integer);
			}
		}
		switch(vid.renderpath)
		{
		case RENDERPATH_GLES2:
			if (bytesperblock)
			{
				qglCompressedTexImage2D(GL_TEXTURE_2D, mip, glt->glinternalformat, upload_mipwidth, upload_mipheight, 0, mipsize, upload_mippixels);CHECKGLERROR
			}
			else
			{
				qglTexImage2D(GL_TEXTURE_2D, mip, glt->glinternalformat, upload_mipwidth, upload_mipheight, 0, glt->glformat, glt->gltype, upload_mippixels);CHECKGLERROR
			}
			break;
		}
		if(upload_mippixels != mippixels)
			Mem_Free(upload_mippixels);
		mippixels += mipsize;
		if (mipwidth <= 1 && mipheight <= 1)
		{
			mipcomplete = true;
			break;
		}
		if (mipwidth > 1)
			mipwidth >>= 1;
		if (mipheight > 1)
			mipheight >>= 1;
	}

	// after upload we have to set some parameters...
	switch(vid.renderpath)
	{
	case RENDERPATH_GLES2:
#ifdef GL_TEXTURE_MAX_LEVEL
		if (dds_miplevels >= 1 && !mipcomplete)
		{
			// need to set GL_TEXTURE_MAX_LEVEL
			qglTexParameteri(gltexturetypeenums[glt->texturetype], GL_TEXTURE_MAX_LEVEL, dds_miplevels - 1);CHECKGLERROR
		}
#endif
		GL_SetupTextureParameters(glt->flags, glt->textype->textype, glt->texturetype);
		qglBindTexture(gltexturetypeenums[glt->texturetype], oldbindtexnum);CHECKGLERROR
		break;
	}

	Mem_Free(dds);
	if(force_swdecode)
		Mem_Free((unsigned char *) mippixels_start);
	return (rtexture_t *)glt;
}

#endif

int R_TextureWidth(rtexture_t *rt)
{
	return rt ? ((gltexture_t *)rt)->inputwidth : 0;
}

int R_TextureHeight(rtexture_t *rt)
{
	return rt ? ((gltexture_t *)rt)->inputheight : 0;
}

int R_TextureFlags(rtexture_t *rt)
{
	return rt ? ((gltexture_t *)rt)->flags : 0;
}

void R_UpdateTexture(rtexture_t *rt, const unsigned char *data, int x, int y, int z, int width, int height, int depth, int combine)
{
	gltexture_t *glt = (gltexture_t *)rt;
	if (data == NULL)
		Host_Error("R_UpdateTexture: no data supplied");
	if (glt == NULL)
		Host_Error("R_UpdateTexture: no texture supplied");
	if (!glt->texnum)
	{
		Con_DPrintf("R_UpdateTexture: texture %p \"%s\" in pool %p has not been uploaded yet\n", (void *)glt, glt->identifier, (void *)glt->pool);
		return;
	}
	// update part of the texture
	if (glt->bufferpixels)
	{
		size_t j, bpp = glt->bytesperpixel;

		// depth and sides are not fully implemented here - can still do full updates but not partial.
		if (glt->inputdepth != 1 || glt->sides != 1)
			Host_Error("R_UpdateTexture on buffered texture that is not 2D\n");
		if (x < 0 || y < 0 || z < 0 || glt->tilewidth < x + width || glt->tileheight < y + height || glt->tiledepth < z + depth)
			Host_Error("R_UpdateTexture on buffered texture with out of bounds coordinates (%i %i %i to %i %i %i is not within 0 0 0 to %i %i %i)", x, y, z, x + width, y + height, z + depth, glt->tilewidth, glt->tileheight, glt->tiledepth);

		for (j = 0; j < (size_t)height; j++)
			memcpy(glt->bufferpixels + ((y + j) * glt->tilewidth + x) * bpp, data + j * width * bpp, width * bpp);

		switch(combine)
		{
		case 0:
			// immediately update the part of the texture, no combining
			R_UploadPartialTexture(glt, data, x, y, z, width, height, depth);
			break;
		case 1:
			// keep track of the region that is modified, decide later how big the partial update area is
			if (glt->buffermodified)
			{
				glt->modified_mins[0] = min(glt->modified_mins[0], x);
				glt->modified_mins[1] = min(glt->modified_mins[1], y);
				glt->modified_mins[2] = min(glt->modified_mins[2], z);
				glt->modified_maxs[0] = max(glt->modified_maxs[0], x + width);
				glt->modified_maxs[1] = max(glt->modified_maxs[1], y + height);
				glt->modified_maxs[2] = max(glt->modified_maxs[2], z + depth);
			}
			else
			{
				glt->buffermodified = true;
				glt->modified_mins[0] = x;
				glt->modified_mins[1] = y;
				glt->modified_mins[2] = z;
				glt->modified_maxs[0] = x + width;
				glt->modified_maxs[1] = y + height;
				glt->modified_maxs[2] = z + depth;
			}
			glt->dirty = true;
			break;
		default:
		case 2:
			// mark the entire texture as dirty, it will be uploaded later
			glt->buffermodified = true;
			glt->modified_mins[0] = 0;
			glt->modified_mins[1] = 0;
			glt->modified_mins[2] = 0;
			glt->modified_maxs[0] = glt->tilewidth;
			glt->modified_maxs[1] = glt->tileheight;
			glt->modified_maxs[2] = glt->tiledepth;
			glt->dirty = true;
			break;
		}
	}
	else
		R_UploadFullTexture(glt, data);
}

int R_RealGetTexture(rtexture_t *rt)
{
	if (rt)
	{
		gltexture_t *glt;
		glt = (gltexture_t *)rt;
		if (glt->flags & GLTEXF_DYNAMIC)
			R_UpdateDynamicTexture(glt);
		if (glt->buffermodified && glt->bufferpixels)
		{
			glt->buffermodified = false;
			// Because we currently don't set the relevant upload stride parameters, just make it full width.
			glt->modified_mins[0] = 0;
			glt->modified_maxs[0] = glt->tilewidth;
			// Check also if it's updating at least half the height of the texture.
			if (glt->modified_maxs[1] - glt->modified_mins[1] > glt->tileheight / 2)
				R_UploadFullTexture(glt, glt->bufferpixels);
			else
				R_UploadPartialTexture(glt, glt->bufferpixels + (size_t)glt->modified_mins[1] * glt->tilewidth * glt->bytesperpixel, glt->modified_mins[0], glt->modified_mins[1], glt->modified_mins[2], glt->modified_maxs[0] - glt->modified_mins[0], glt->modified_maxs[1] - glt->modified_mins[1], glt->modified_maxs[2] - glt->modified_mins[2]);
		}
		VectorClear(glt->modified_mins);
		VectorClear(glt->modified_maxs);
		glt->dirty = false;
		return glt->texnum;
	}
	else
		return r_texture_white->texnum;
}

void R_ClearTexture (rtexture_t *rt)
{
	gltexture_t *glt = (gltexture_t *)rt;

	R_UploadFullTexture(glt, NULL);
}

int R_PicmipForFlags(int flags)
{
	int miplevel = 0;
	if(flags & TEXF_PICMIP)
	{
		miplevel += gl_picmip.integer;
		if (flags & TEXF_ISWORLD)
		{
			if (r_picmipworld.integer)
				miplevel += gl_picmip_world.integer;
			else
				miplevel = 0;
		}
		else if (flags & TEXF_ISSPRITE)
		{
			if (r_picmipsprites.integer)
				miplevel += gl_picmip_sprites.integer;
			else
				miplevel = 0;
		}
		else
			miplevel += gl_picmip_other.integer;
	}
	return max(0, miplevel);
}
