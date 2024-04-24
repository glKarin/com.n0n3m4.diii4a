
#ifndef R_TEXTURES_H
#define R_TEXTURES_H

#include "qtypes.h"
#include "qdefs.h"

// transparent
#define TEXF_ALPHA 0x00000001
// mipmapped
#define TEXF_MIPMAP 0x00000002
// multiply RGB by A channel before uploading
#define TEXF_RGBMULTIPLYBYALPHA 0x00000004
// indicates texture coordinates should be clamped rather than wrapping
#define TEXF_CLAMP 0x00000020
// indicates texture should be uploaded using GL_NEAREST or GL_NEAREST_MIPMAP_NEAREST mode
#define TEXF_FORCENEAREST 0x00000040
// indicates texture should be uploaded using GL_LINEAR or GL_LINEAR_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_LINEAR mode
#define TEXF_FORCELINEAR 0x00000080
// indicates texture should be affected by gl_picmip and gl_max_size cvar
#define TEXF_PICMIP 0x00000100
// indicates texture should be compressed if possible
#define TEXF_COMPRESS 0x00000200
// use this flag to block R_PurgeTexture from freeing a texture (only used by r_texture_white and similar which may be used in skinframe_t)
#define TEXF_PERSISTENT 0x00000400
// indicates texture should use GL_COMPARE_R_TO_TEXTURE mode
#define TEXF_COMPARE 0x00000800
// indicates texture should use lower precision where supported
#define TEXF_LOWPRECISION 0x00001000
// indicates texture should support R_UpdateTexture on small regions, actual uploads may be delayed until R_Mesh_TexBind if gl_nopartialtextureupdates is on
#define TEXF_ALLOWUPDATES 0x00002000
// indicates texture should be affected by gl_picmip_world and r_picmipworld (maybe others in the future) instead of gl_picmip_other
#define TEXF_ISWORLD 0x00004000
// indicates texture should be affected by gl_picmip_sprites and r_picmipsprites (maybe others in the future) instead of gl_picmip_other
#define TEXF_ISSPRITE 0x00008000
// indicates the texture will be used as a render target (D3D hint)
#define TEXF_RENDERTARGET 0x0010000
// used for checking if textures mismatch
#define TEXF_IMPORTANTBITS (TEXF_ALPHA | TEXF_MIPMAP | TEXF_RGBMULTIPLYBYALPHA | TEXF_CLAMP | TEXF_FORCENEAREST | TEXF_FORCELINEAR | TEXF_PICMIP | TEXF_COMPARE | TEXF_LOWPRECISION | TEXF_RENDERTARGET)
// set as a flag to force the texture to be reloaded
#define TEXF_FORCE_RELOAD 0x80000000

typedef enum textype_e
{
	// placeholder for unused textures in r_rendertarget_t
	TEXTYPE_UNUSED,

	// 8bit paletted
	TEXTYPE_PALETTE,
	// 32bit RGBA
	TEXTYPE_RGBA,
	// 32bit BGRA (preferred format due to faster uploads on most hardware)
	TEXTYPE_BGRA,
	// 8bit ALPHA (used for freetype fonts)
	TEXTYPE_ALPHA,
	// 4x4 block compressed 15bit color (4 bits per pixel)
	TEXTYPE_DXT1,
	// 4x4 block compressed 15bit color plus 1bit alpha (4 bits per pixel)
	TEXTYPE_DXT1A,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel)
	TEXTYPE_DXT3,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel)
	TEXTYPE_DXT5,

	// default compressed type for GLES2
	TEXTYPE_ETC1,

	// 8bit paletted in sRGB colorspace
	TEXTYPE_SRGB_PALETTE,
	// 32bit RGBA in sRGB colorspace
	TEXTYPE_SRGB_RGBA,
	// 32bit BGRA (preferred format due to faster uploads on most hardware) in sRGB colorspace
	TEXTYPE_SRGB_BGRA,
	// 4x4 block compressed 15bit color (4 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT1,
	// 4x4 block compressed 15bit color plus 1bit alpha (4 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT1A,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT3,
	// 4x4 block compressed 15bit color plus 8bit alpha (8 bits per pixel) in sRGB colorspace
	TEXTYPE_SRGB_DXT5,

	// this represents the same format as the framebuffer, for fast copies
	TEXTYPE_COLORBUFFER,
	// this represents an RGBA half_float texture (4 16bit floats)
	TEXTYPE_COLORBUFFER16F,
	// this represents an RGBA float texture (4 32bit floats)
	TEXTYPE_COLORBUFFER32F,
	// depth-stencil buffer (or texture)
	TEXTYPE_DEPTHBUFFER16,
	// depth-stencil buffer (or texture)
	TEXTYPE_DEPTHBUFFER24,
	// 32bit D24S8 buffer (24bit depth, 8bit stencil), not supported on OpenGL ES
	TEXTYPE_DEPTHBUFFER24STENCIL8,
	// shadowmap-friendly format with depth comparison (not supported on some hardware)
	TEXTYPE_SHADOWMAP16_COMP,
	// shadowmap-friendly format with raw reading (not supported on some hardware)
	TEXTYPE_SHADOWMAP16_RAW,
	// shadowmap-friendly format with depth comparison (not supported on some hardware)
	TEXTYPE_SHADOWMAP24_COMP,
	// shadowmap-friendly format with raw reading (not supported on some hardware)
	TEXTYPE_SHADOWMAP24_RAW,
}
textype_t;

// contents of this structure are mostly private to gl_textures.c
typedef struct rtexture_s
{
	// this is exposed (rather than private) for speed reasons only
	int texnum; // GL texture slot number
	int renderbuffernum; // GL renderbuffer slot number
	qbool dirty; // indicates that R_RealGetTexture should be called
	qbool glisdepthstencil; // indicates that FBO attachment has to be GL_DEPTH_STENCIL_ATTACHMENT
	int gltexturetypeenum; // used by R_Mesh_TexBind
}
rtexture_t;

// contents of this structure are private to gl_textures.c
typedef struct rtexturepool_s
{
	int useless;
}
rtexturepool_t;

typedef struct skinframe_s
{
	struct rtexture_s *stain; // inverse modulate with background (used for decals and such)
	struct rtexture_s *merged; // original texture without glow
	struct rtexture_s *base; // original texture without pants/shirt/glow
	struct rtexture_s *pants; // pants only (in greyscale)
	struct rtexture_s *shirt; // shirt only (in greyscale)
	struct rtexture_s *nmap; // normalmap (bumpmap for dot3)
	struct rtexture_s *gloss; // glossmap (for dot3)
	struct rtexture_s *glow; // glow only (fullbrights)
	struct rtexture_s *fog; // alpha of the base texture (if not opaque)
	struct rtexture_s *reflect; // colored mask for cubemap reflections
	// accounting data for hash searches:
	// the compare variables are used to identify internal skins from certain
	// model formats
	// (so that two q1bsp maps with the same texture name for different
	//  textures do not have any conflicts)
	struct skinframe_s *next; // next on hash chain
	char basename[MAX_QPATH]; // name of this
	int textureflags; // texture flags to use
	int comparewidth;
	int compareheight;
	int comparecrc;
	// mark and sweep garbage collection, this value is updated to a new value
	// on each level change for the used skinframes, if some are not used they
	// are freed
	unsigned int loadsequence;
	// indicates whether this texture has transparent pixels
	qbool hasalpha;
	// average texture color, if applicable
	float avgcolor[4];
	// for mdl skins, we actually only upload on first use (many are never used, and they are almost never used in both base+pants+shirt and merged modes)
	unsigned char *qpixels;
	int qwidth;
	int qheight;
	qbool qhascolormapping;
	qbool qgeneratebase;
	qbool qgeneratemerged;
	qbool qgeneratenmap;
	qbool qgenerateglow;
}
skinframe_t;

typedef void (*updatecallback_t)(rtexture_t *rt, void *data);

// allocate a texture pool, to be used with R_LoadTexture
rtexturepool_t *R_AllocTexturePool(void);
// free a texture pool (textures can not be freed individually)
void R_FreeTexturePool(rtexturepool_t **rtexturepool);

// the color/normal/etc cvars should be checked by callers of R_LoadTexture* functions to decide whether to add TEXF_COMPRESS to the flags
extern struct cvar_s gl_texturecompression;
extern struct cvar_s gl_texturecompression_color;
extern struct cvar_s gl_texturecompression_normal;
extern struct cvar_s gl_texturecompression_gloss;
extern struct cvar_s gl_texturecompression_glow;
extern struct cvar_s gl_texturecompression_2d;
extern struct cvar_s gl_texturecompression_q3bsplightmaps;
extern struct cvar_s gl_texturecompression_q3bspdeluxemaps;
extern struct cvar_s gl_texturecompression_sky;
extern struct cvar_s gl_texturecompression_lightcubemaps;
extern struct cvar_s gl_texturecompression_reflectmask;
extern struct cvar_s r_texture_dds_load;
extern struct cvar_s r_texture_dds_save;

// add a texture to a pool and optionally precache (upload) it
// (note: data == NULL is perfectly acceptable)
// (note: palette must not be NULL if using TEXTYPE_PALETTE)
rtexture_t *R_LoadTexture2D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette);
rtexture_t *R_LoadTexture3D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, int depth, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette);
rtexture_t *R_LoadTextureCubeMap(rtexturepool_t *rtexturepool, const char *identifier, int width, const unsigned char *data, textype_t textype, int flags, int miplevel, const unsigned int *palette);
rtexture_t *R_LoadTextureShadowMap2D(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, textype_t textype, qbool filter);
rtexture_t *R_LoadTextureRenderBuffer(rtexturepool_t *rtexturepool, const char *identifier, int width, int height, textype_t textype);
rtexture_t *R_LoadTextureDDSFile(rtexturepool_t *rtexturepool, const char *filename, qbool srgb, int flags, qbool *hasalphaflag, float *avgcolor, int miplevel, qbool optionaltexture);

// saves a texture to a DDS file
int R_SaveTextureDDSFile(rtexture_t *rt, const char *filename, qbool skipuncompressed, qbool hasalpha);

// free a texture
void R_FreeTexture(rtexture_t *rt);

// update a portion of the image data of a texture, used by lightmap updates
// and procedural textures such as video playback, actual uploads may be
// delayed by gl_nopartialtextureupdates cvar until R_Mesh_TexBind uses it
// combine has 3 values: 0 = immediately upload (glTexSubImage2D), 1 = combine with other updates (glTexSubImage2D on next draw), 2 = combine with other updates and never upload partial images (glTexImage2D on next draw)
void R_UpdateTexture(rtexture_t *rt, const unsigned char *data, int x, int y, int z, int width, int height, int depth, int combine);

// returns the renderer dependent texture slot number (call this before each
// use, as a texture might not have been precached)
#define R_GetTexture(rt) ((rt) ? ((rt)->dirty ? R_RealGetTexture(rt) : (rt)->texnum) : r_texture_white->texnum)
int R_RealGetTexture (rtexture_t *rt);

// returns width of texture, as was specified when it was uploaded
int R_TextureWidth(rtexture_t *rt);

// returns height of texture, as was specified when it was uploaded
int R_TextureHeight(rtexture_t *rt);

// returns flags of texture, as was specified when it was uploaded
int R_TextureFlags(rtexture_t *rt);

// only frees the texture if TEXF_PERSISTENT is not set
// also resets the variable
void R_PurgeTexture(rtexture_t *prt);

// frees processing buffers each frame, and may someday animate procedural textures
void R_Textures_Frame(void);

// maybe rename this - sounds awful? [11/21/2007 Black]
void R_MarkDirtyTexture(rtexture_t *rt);
void R_MakeTextureDynamic(rtexture_t *rt, updatecallback_t updatecallback, void *data);

// Clear the texture's contents
void R_ClearTexture (rtexture_t *rt);

// returns the desired picmip level for given TEXF_ flags
int R_PicmipForFlags(int flags);

void R_TextureStats_Print(qbool printeach, qbool printpool, qbool printtotal);

#endif

