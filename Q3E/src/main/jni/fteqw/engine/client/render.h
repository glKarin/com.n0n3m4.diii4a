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

// refresh.h -- public interface to refresh functions

// default soldier colors
#define TOP_DEFAULT		1
#define BOTTOM_DEFAULT	6

#define	TOP_RANGE		(TOP_DEFAULT<<4)
#define	BOTTOM_RANGE	(BOTTOM_DEFAULT<<4)

struct msurface_s;
struct batch_s;
struct model_s;
struct texnums_s;
struct texture_s;

static const texid_t r_nulltex = NULL;

//GLES2 requires GL_UNSIGNED_SHORT (gles3 or GL_OES_element_index_uint relax this requirement)
//geforce4 only does shorts. gffx can do ints, but with a performance hit (like most things on that gpu)
//ati is generally more capable, but generally also has a smaller market share
//desktop-gl will generally cope with ints, but expect a performance hit from that with old gpus (so we don't bother)
//vulkan+dx10 can cope with ints, but might be 24bit
//either way, all renderers in the same build need to use the same thing.
#ifndef sizeof_index_t
	#ifdef VERTEXINDEXBYTES	//maybe set in config_*.h
		#define sizeof_index_t VERTEXINDEXBYTES
	#elif (defined(GLQUAKE) && defined(HAVE_LEGACY)) || defined(MINIMAL) || defined(D3D8QUAKE) || defined(D3D9QUAKE) || defined(ANDROID) || defined(FTE_TARGET_WEB)
		#define sizeof_index_t 2
	#endif
#endif
#if sizeof_index_t == 2
	#define GL_INDEX_TYPE GL_UNSIGNED_SHORT
	#define D3DFMT_QINDEX D3DFMT_INDEX16
	#define DXGI_FORMAT_INDEX_UINT DXGI_FORMAT_R16_UINT
	#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT16
	typedef unsigned short index_t;
	#define MAX_INDICIES 0xffffu
#else
	#undef sizeof_index_t
	#define sizeof_index_t 4
	#define GL_INDEX_TYPE GL_UNSIGNED_INT
	#define D3DFMT_QINDEX D3DFMT_INDEX32
	#define DXGI_FORMAT_INDEX_UINT DXGI_FORMAT_R32_UINT
	#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT32
	typedef unsigned int index_t;
	#define MAX_INDICIES 0x00ffffffu
#endif

//=============================================================================

//the eye doesn't see different colours in the same proportion.
//must add to slightly less than 1
#define NTSC_RED 0.299
#define NTSC_GREEN 0.587
#define NTSC_BLUE 0.114
#define NTSC_SUM (NTSC_RED + NTSC_GREEN + NTSC_BLUE)

typedef enum {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals
	//q3 ones stop here.

	//fte ones start here
	RT_PORTALCAMERA,		// an alternative to RT_PORTALSURFACE.

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef unsigned int skinid_t;	//skin 0 is 'unused'

struct dlight_s;
typedef struct entity_s
{
	//FIXME: instancing somehow. separate visentity+visinstance. only viable with full glsl though.
	//will need to generate a vbo somehow for the instances.

	int						keynum;			// for matching entities in different frames
	vec3_t					origin;
	vec3_t					angles;			// fixme: should be redundant.
	vec3_t					axis[3];

	vec4_t					shaderRGBAf; /*colormod+alpha, available for shaders to mix*/
	float					shaderTime;  /*timestamp, for syncing shader times to spawns*/
	vec3_t					glowmod;     /*meant to be a multiplier for the fullbrights*/

	int						light_known; /*bsp lighting has been calced*/
	vec3_t					light_avg;   /*midpoint level*/
	vec3_t					light_range; /*avg + this = max, avg - this = min*/
	vec3_t					light_dir;

	vec3_t					oldorigin;	/*for q2/q3 beams*/

	struct model_s			*model;			// NULL = no model
	int						skinnum;		// for Alias models
	skinid_t				customskin;		// quake3 style skins

	int						playerindex;	//for qw skins
	int						topcolour;		//colourmapping
	int						bottomcolour;	//colourmapping
#ifdef HEXEN2
	int						h2playerclass;	//hexen2's quirky colourmapping
#endif

//	struct efrag_s			*efrag;			// linked list of efrags (FIXME)
//	int						visframe;		// last frame this entity was
											// found in an active leaf
											// only used for static objects
											
//	int						dlightframe;	// dynamic lighting
//	dlightbitmask_t			dlightbits;
	
// FIXME: could turn these into a union
//	int						trivial_accept;
//	struct mnode_s			*topnode;		// for bmodels, first world node
											//  that splits bmodel, or NULL if
											//  not split

	framestate_t			framestate;

	int flags;

	refEntityType_t rtype;
	float rotation;

	struct shader_s *forcedshader;

	pvscache_t pvscache; //for culling of csqc ents.

#ifdef PEXT_SCALE
	float scale;
#endif
#ifdef PEXT_FATNESS
	float fatness;
#endif
#ifdef HEXEN2
	int drawflags;
	int abslight;
#endif
} entity_t;

#define MAX_GEOMSETS 32u
#define Q1UNSPECIFIED 0x00ffffff	//0xffRRGGBB or 0x0000000V are both valid values. so this is an otherwise-illegal value to say its not been set.
typedef struct
{
	int refcount;
	char skinname[MAX_QPATH];
	int nummappings;
	int maxmappings;
	qbyte geomset[MAX_GEOMSETS];	//allows selecting a single set of geometry from alternatives. this might be a can of worms.
#ifdef QWSKINS
	char qwskinname[MAX_QPATH];
	struct qwskin_s *qwskin;
	unsigned int q1upper;	//Q1UNSPECIFIED
	unsigned int q1lower;	//Q1UNSPECIFIED
	unsigned int h2class;	//Q1UNSPECIFIED. urgh.
#endif
	struct
	{
		char surface[MAX_QPATH];
		shader_t *shader;
		texnums_t texnums;
		int needsfree;	//which textures need to be freed.
	} mappings[1];
} skinfile_t;

// plane_t structure
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	qbyte	type;			// for texture axis selection and fast side tests
	qbyte	signbits;		// signx + signy<<1 + signz<<1
	qbyte	pad[2];
} mplane_t;
#define MAXFRUSTUMPLANES 7	//4 side, 1 near, 1 far (fog), 1 water plane.

typedef struct
{
	//note: uniforms expect specific padding/ordering. be really careful with reordering this
	vec3_t colour;		//w_fog[0].xyz
	float alpha;		//w_fog[0].w scales clamped fog value
	float density;		//w_fog[1].x egads, everyone has a different opinion.
	float depthbias;	//w_fog[1].y distance until the fog actually starts
	float glslpad1;		//w_fog[1].z
	float glslpad2;		//w_fog[1].w

//	float start;
//	float end;
//	float height;
//	float fadedepth;

	float time;	//timestamp for when its current.
} fogstate_t;
void CL_BlendFog(fogstate_t *result, fogstate_t *oldf, float time, fogstate_t *newf);
void CL_ResetFog(int fogtype);

typedef enum {
	STEREO_OFF,
	STEREO_QUAD,
	STEREO_RED_CYAN,
	STEREO_RED_BLUE,
	STEREO_RED_GREEN,
	STEREO_CROSSEYED,

	//these are internal methods and do not form part of any public API
	STEREO_LEFTONLY,
	STEREO_RIGHTONLY
} stereomethod_t;

typedef enum
{
	PROJ_STANDARD			= 0,
	PROJ_STEREOGRAPHIC		= 1,
	PROJ_FISHEYE			= 2,	//standard fisheye
	PROJ_PANORAMA			= 3,	//for nice panoramas
	PROJ_LAEA				= 4,	//lambert azimuthal equal-area 
	PROJ_EQUIRECTANGULAR	= 5,	//projects a sphere into 2d. used by vr screenshots.
	PROJ_PANINI       = 6		//like stereographic, but vertical lines stay straight.
} qprojection_t;

typedef struct {
	char texname[MAX_QPATH];
} rtname_t;
#define R_MAX_RENDERTARGETS 8

#ifndef R_MAX_RECURSE
#define R_MAX_RECURSE	6
#endif
#define RDFD_FOV 1
typedef struct refdef_s
{
	vrect_t		grect;				// game rectangle. fullscreen except for csqc/splitscreen/hud.
	vrect_t		vrect;				// subwindow in grect for 3d view. equal to grect if no hud.

	vec3_t		pvsorigin;			/*render the view using this point for pvs (useful for mirror views)*/
	vec3_t		vieworg;			/*logical view center*/
	vec3_t		viewangles;
	vec3_t		viewaxis[3];		/*forward, left, up (NOT RIGHT)*/
	vec3_t		eyeoffset;			/*world space, for vr screenies*/
	vec2_t		projectionoffset;	/*for off-centre rendering*/

	qboolean	base_known;			/*otherwise we do some fallback behaviour (ie: viewangles.0y0 and forcing input_angles)*/
	vec3_t		base_angles, base_origin; /*for vr output, overrides per-eye viewangles according to that eye's matrix.*/

	vec3_t		weaponmatrix[4];		/*forward/left/up/origin*/
	vec3_t		weaponmatrix_bob[4];	/*forward/left/up/origin*/

	float		fov_x, fov_y, afov;
	float		fovv_x, fovv_y;	//viewmodel fovs
	float		mindist, maxdist;	//maxdist may be 0, for 'infinite', in which case mindist probably isn't valid either.

	qboolean	drawsbar;
	qboolean	drawcrosshair;
	int			flags;	//(Q2)RDF_ flags
	int			dirty;

	playerview_t *playerview;
//	int			currentplayernum;

	float		time;
//	float		waterheight;	//updated by the renderer. stuff sitting at this height generate ripple effects

	float		m_projection_std[16];	//projection matrix for normal stuff
	float		m_projection_view[16];	//projection matrix for the viewmodel. because people are weird.
	float		m_view[16];
	qbyte		*scenevis;			/*this is the vis that's currently being draw*/
	int			*sceneareas;		/*this is the area info for the camera (should normally be count+one area, but could be two areas near an opaque water plane)*/

	mplane_t	frustum[MAXFRUSTUMPLANES];
	int			frustum_numworldplanes;	//all but far, which isn't culled because this wouldn't cover the entire screen.
	int			frustum_numplanes;	//includes far plane (which is reduced with fog).

	fogstate_t	globalfog;
	float		hdr_value;

	vec3_t		skyroom_pos;		/*the camera position for sky rooms*/
	vec4_t		skyroom_spin;		/*the camera spin for sky rooms*/
	qboolean	skyroom_enabled;	/*whether a skyroom position is defined*/
	int			firstvisedict;		/*so we can skip visedicts in skies*/

	pxrect_t	pxrect;				/*vrect, but in pixels rather than virtual coords*/
	qboolean	externalview;		/*draw external models and not viewmodels*/
	int			recurse;			/*in a mirror/portal/half way through drawing something else*/
	qboolean	forcevis;			/*if true, vis comes from the forcedvis field instead of recalculated*/
	unsigned int	flipcull;		/*reflected/flipped view, requires inverted culling (should be set to SHADER_CULL_FLIPPED or 0 - its implemented as a xor)*/
	unsigned int	colourmask;		/*shaderbits mask. anything not here will be forced to 0. this is for red/green type stereo*/
	qboolean	useperspective;		/*not orthographic*/

	stereomethod_t stereomethod;
	rtname_t	rt_destcolour[R_MAX_RENDERTARGETS];	/*used for 2d. written by 3d*/
	rtname_t	rt_sourcecolour;	/*read by 2d. not used for 3d. */
	rtname_t	rt_depth;			/*read by 2d. used by 3d (renderbuffer used if not set)*/
	rtname_t	rt_ripplemap;		/*read by 2d. used by 3d (internal ripplemap buffer used if not set)*/
	rtname_t	nearenvmap;			/*provides a fallback endmap cubemap to render with*/

	qbyte		*forcedvis;			/*set if forcevis is set*/
	qboolean	areabitsknown;
	qbyte		areabits[MAX_MAP_AREA_BYTES];

	vec4_t		userdata[16];		/*for custom glsl*/

	qboolean	warndraw;			/*buggy gamecode likes drawing outside of te drawing logic*/

	qboolean	fixedview;			/*if true, use a fixed camera setup for out-of-body screenshots (usually to build cubemaps)*/
	vec3_t		fixedvieworg;
	vec3_t		fixedviewangles;
} refdef_t;

extern	refdef_t	r_refdef;
extern vec3_t	r_origin, vpn, vright, vup;

extern	struct texture_s	*r_notexture_mip;

extern	entity_t	r_worldentity;

void BE_GenModelBatches(struct batch_s **batches, const struct dlight_s *dl, unsigned int bemode, const qbyte *worldpvs, const int *worldareas);	//if dl, filters based upon the dlight.

//gl_alias.c
void R_GAliasFlushSkinCache(qboolean final);
void R_GAlias_DrawBatch(struct batch_s *batch);
void R_GAlias_GenerateBatches(entity_t *e, struct batch_s **batches);
void R_LightArraysByte_BGR(const entity_t *entity, vecV_t *coords, byte_vec4_t *colours, int vertcount, vec3_t *normals, qboolean colormod);
void R_LightArrays(const entity_t *entity, vecV_t *coords, avec4_t *colours, int vertcount, vec3_t *normals, float scale, qboolean colormod);

qboolean R_DrawSkyChain (struct batch_s *batch); /*called from the backend, and calls back into it*/
void R_InitSky (shader_t *shader, const char *skyname, uploadfmt_t fmt, qbyte *src, unsigned int width, unsigned int height);	/*generate q1 sky texnums*/

void R_Clutter_Emit(struct batch_s **batches);
void R_Clutter_Purge(void);

//r_surf.c
void Surf_NewMap (struct model_s *worldmodel);
void Surf_PreNewMap(void);
void Surf_SetupFrame(void);	//determine pvs+viewcontents
void Surf_DrawWorld(void);
void Surf_GenBrushBatches(struct batch_s **batches, entity_t *ent);
void Surf_StainSurf(struct model_s *mod, struct msurface_s *surf, float *parms);
void Surf_AddStain(vec3_t org, float red, float green, float blue, float radius);
void Surf_LessenStains(void);
void Surf_WipeStains(void);
void Surf_DeInit(void);
void Surf_Clear(struct model_s *mod);
void Surf_BuildLightmaps(void);				//enables Surf_BuildModelLightmaps, calls it for each bsp.
void Surf_ClearSceneCache(void);			//stops Surf_BuildModelLightmaps from working.
void Surf_BuildModelLightmaps (struct model_s *m);	//rebuild lightmaps for a single bsp. beware of submodels.
void Surf_RenderDynamicLightmaps (struct msurface_s *fa);
void Surf_RenderAmbientLightmaps (struct msurface_s *fa, int ambient);
int Surf_LightmapShift (struct model_s *model);
#define LMBLOCK_SIZE_MAX 2048	//single axis
typedef struct glRect_s {
	unsigned short l,t,r,b;
} glRect_t;
typedef unsigned char stmap;
struct mesh_s;
typedef struct {
	texid_t lightmap_texture;
	qboolean	modified;	//data was changed. consult rectchange to see the bounds.
	qboolean	external;	//the data was loaded from a file (q3bsp feature where we shouldn't be blending lightmaps at all)
	qboolean	hasdeluxe;	//says that the next lightmap index contains deluxemap info
	uploadfmt_t	fmt;		//texture format that we're using
	qbyte		pixbytes;	//yes, this means no compressed formats.
	int			width;
	int			height;
	glRect_t	rectchange;
	qbyte		*lightmaps;	//[pixbytes*LMBLOCK_WIDTH*LMBLOCK_HEIGHT];
	stmap		*stainmaps;	//[3*LMBLOCK_WIDTH*LMBLOCK_HEIGHT];	//rgb no a. added to lightmap for added (hopefully) speed.
#ifdef GLQUAKE
	int			pbo_handle;	//when set, lightmaps is a persistently mapped write-only pbo for us to scribble data into, ready to be copied to the actual texture without waiting for glTexSubImage to complete.
#endif
} lightmapinfo_t;
extern lightmapinfo_t **lightmap;
extern int numlightmaps;

void QDECL Surf_RebuildLightmap_Callback (struct cvar_s *var, char *oldvalue);


void R_Sky_Register(void);
void R_SkyShutdown(void);
void R_SetSky(const char *skyname);
texid_t R_GetDefaultEnvmap(void);

#if defined(GLQUAKE)
void GLR_Init (void);
void GLR_InitTextures (void);
void GLR_InitEfrags (void);
void GLR_RenderView (void);		// must set r_refdef first
								// called whenever r_refdef or vid change
void GLR_DrawPortal(struct batch_s *batch, struct batch_s **blist, struct batch_s *depthmasklist[2], int portaltype);

void GLR_PushDlights (void);
void GLR_DrawWaterSurfaces (void);

void GLVID_DeInit (void);
void GLR_DeInit (void);
void GLSCR_DeInit (void);
void GLVID_Console_Resize(void);
#endif
int R_LightPoint (vec3_t p);
void R_RenderDlights (void);

typedef struct
{
	int allocated[LMBLOCK_SIZE_MAX];
	int firstlm;
	int lmnum;
	unsigned int width;
	unsigned int height;
	qboolean deluxe;
} lmalloc_t;
void Mod_LightmapAllocInit(lmalloc_t *lmallocator, qboolean hasdeluxe, unsigned int width, unsigned int height, int firstlm);	//firstlm is for debugging stray lightmap indexes
//void Mod_LightmapAllocDone(lmalloc_t *lmallocator, model_t *mod);
void Mod_LightmapAllocBlock(lmalloc_t *lmallocator, int w, int h, unsigned short *x, unsigned short *y, int *tnum);

enum imageflags
{
	/*warning: many of these flags only apply the first time it is requested*/
	IF_CLAMP			= 1<<0,		//disable texture coord wrapping.
	IF_NOMIPMAP			= 1<<1,		//disable mipmaps.
	IF_NEAREST			= 1<<2,		//force nearest
	IF_LINEAR			= 1<<3,		//force linear
	IF_UIPIC			= 1<<4,		//subject to texturemode2d
	//IF_DEPTHCMD		= 1<<5,		//Reserved for d3d11
	IF_SRGB				= 1<<6,		//texture data is srgb (read-as-linear)
	/*WARNING: If the above are changed, be sure to change shader pass flags*/

	IF_NOPICMIP			= 1<<7,
	IF_NOALPHA			= 1<<8,		/*hint rather than requirement*/
	IF_NOGAMMA			= 1<<9,		/*do not apply texture-based gamma*/
	IF_TEXTYPEMASK			= (1<<10) | (1<<11) | (1<<12), /*0=2d, 1=3d, 2=cubeface, 3=2d array texture*/
#define IF_TEXTYPESHIFT		10
#define IF_TEXTYPE_2D (PTI_2D<<IF_TEXTYPESHIFT)
#define IF_TEXTYPE_3D (PTI_3D<<IF_TEXTYPESHIFT)
#define IF_TEXTYPE_CUBE (PTI_CUBE<<IF_TEXTYPESHIFT)
#define IF_TEXTYPE_2D_ARRAY (PTI_2D_ARRAY<<IF_TEXTYPESHIFT)
#define IF_TEXTYPE_CUBE_ARRAY (PTI_CUBE_ARRAY<<IF_TEXTYPESHIFT)
#define IF_TEXTYPE_ANY (PTI_ANY<<IF_TEXTYPESHIFT)
	IF_MIPCAP			= 1<<13,	//allow the use of d_mipcap
	IF_PREMULTIPLYALPHA	= 1<<14,	//rgb *= alpha

	IF_UNUSED15			= 1<<15,	//
	IF_UNUSED16			= 1<<16,	//
	IF_INEXACT			= 1<<17,	//subdir info isn't to be used for matching

	IF_WORLDTEX			= 1<<18,	//gl_picmip_world
	IF_SPRITETEX		= 1<<19,	//gl_picmip_sprites
	IF_NOSRGB			= 1<<20,	//ignore srgb when loading. this is guarenteed to be linear, for normalmaps etc.

	IF_PALETTIZE		= 1<<21,	//convert+load it as an RTI_P8 texture for the current palette/colourmap
	IF_NOPURGE			= 1<<22,	//texture is not flushed when no more shaders refer to it (for C code that holds a permanant reference to it - still purged on vid_reloads though)
	IF_HIGHPRIORITY		= 1<<23,	//pushed to start of worker queue instead of end...
	IF_LOWPRIORITY		= 1<<24,	//
	IF_LOADNOW			= 1<<25,	/*hit the disk now, and delay the gl load until its actually needed. this is used only so that the width+height are known in advance. valid on worker threads.*/
	IF_NOPCX			= 1<<26,	/*block pcx format. meaning qw skins can use team colours and cropping*/
	IF_TRYBUMP			= 1<<27,	/*attempt to load _bump if the specified _norm texture wasn't found*/
	IF_RENDERTARGET		= 1<<28,	/*never loaded from disk, loading can't fail*/
	IF_EXACTEXTENSION	= 1<<29,	/*don't mangle extensions, use what is specified and ONLY that*/
	IF_NOREPLACE		= 1<<30,	/*don't load a replacement, for some reason*/
	IF_NOWORKER			= 1u<<31	/*don't pass the work to a loader thread. this gives fully synchronous loading. only valid from the main thread.*/
};

#define R_LoadTexture8(id,w,h,d,f,t)		Image_GetTexture(id, NULL, f, d, NULL, w, h, t?TF_TRANS8:TF_SOLID8)
#define R_LoadTexture32(id,w,h,d,f)			Image_GetTexture(id, NULL, f, d, NULL, w, h, TF_RGBA32)
#define R_LoadTextureFB(id,w,h,d,f)			Image_GetTexture(id, NULL, f, d, NULL, w, h, TF_TRANS8_FULLBRIGHT)
#define R_LoadTexture(id,w,h,fmt,d,fl)		Image_GetTexture(id, NULL, fl, d, NULL, w, h, fmt)

image_t *Image_TextureIsValid(qintptr_t address);
image_t *Image_FindTexture	(const char *identifier, const char *subpath, unsigned int flags);
image_t *Image_CreateTexture(const char *identifier, const char *subpath, unsigned int flags);
image_t *QDECL Image_GetTexture	(const char *identifier, const char *subpath, unsigned int flags, void *fallbackdata, void *fallbackpalette, int fallbackwidth, int fallbackheight, uploadfmt_t fallbackfmt);
qboolean Image_UnloadTexture(image_t *tex);	//true if it did something.
void Image_DestroyTexture	(image_t *tex);
qboolean Image_LoadTextureFromMemory(texid_t tex, int flags, const char *iname, const char *fname, qbyte *filedata, int filesize);	//intended really for worker threads, but should be fine from the main thread too
qboolean Image_LocateHighResTexture(image_t *tex, flocation_t *bestloc, char *bestname, size_t bestnamesize, unsigned int *bestflags);
void Image_Upload			(texid_t tex, uploadfmt_t fmt, void *data, void *palette, int width, int height, int depth, unsigned int flags);
void Image_Purge(void);	//purge any textures which are not needed any more (releases memory, but doesn't give null pointers).
void Image_Init(void);
void Image_Shutdown(void);
void Image_PrintInputFormatVersions(void); //for version info
qboolean Image_WriteKTXFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips);
qboolean Image_WriteDDSFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips);
void Image_BlockSizeForEncoding(uploadfmt_t encoding, unsigned int *blockbytes, unsigned int *blockwidth, unsigned int *blockheight, unsigned int *blockdepth);
const char *Image_FormatName(uploadfmt_t encoding);
qboolean Image_FormatHasAlpha(uploadfmt_t encoding);
image_t *Image_LoadTexture	(const char *identifier, int width, int height, uploadfmt_t fmt, void *data, unsigned int flags);
struct pendingtextureinfo *Image_LoadMipsFromMemory(int flags, const char *iname, const char *fname, qbyte *filedata, int filesize);
void Image_ChangeFormat(struct pendingtextureinfo *mips, qboolean *allowedformats, uploadfmt_t origfmt, const char *imagename);
void Image_Premultiply(struct pendingtextureinfo *mips);
void *Image_FlipImage(const void *inbuffer, void *outbuffer, int *inoutwidth, int *inoutheight, int pixelbytes, qboolean flipx, qboolean flipy, qboolean flipd);

typedef struct
{
	const char *loadername;
	size_t pendingtextureinfosize;
	qboolean canloadcubemaps;
	struct pendingtextureinfo *(*ReadImageFile)(unsigned int imgflags, const char *fname, qbyte *filedata, size_t filesize);
#define plugimageloaderfuncs_name "ImageLoader"
} plugimageloaderfuncs_t;
qboolean Image_RegisterLoader(void *module, plugimageloaderfuncs_t *loader);

#ifdef D3D8QUAKE
void		D3D8_Set2D (void);
void		D3D8_UpdateFiltering	(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float anis);
qboolean	D3D8_LoadTextureMips	(texid_t tex, const struct pendingtextureinfo *mips);
void		D3D8_DestroyTexture		(texid_t tex);
#endif
#ifdef D3D9QUAKE
void		D3D9_Set2D (void);
void		D3D9_UpdateFiltering	(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis);
qboolean	D3D9_LoadTextureMips	(texid_t tex, const struct pendingtextureinfo *mips);
void		D3D9_DestroyTexture		(texid_t tex);
#endif
#ifdef D3D11QUAKE
void		D3D11_UpdateFiltering	(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis);
qboolean	D3D11_LoadTextureMips	(texid_t tex, const struct pendingtextureinfo *mips);
void		D3D11_DestroyTexture	(texid_t tex);
#endif

//extern int image_width, image_height;
texid_t R_LoadReplacementTexture(const char *name, const char *subpath, unsigned int flags, void *lowres, int lowreswidth, int lowresheight, uploadfmt_t fmt);
texid_tf R_LoadHiResTexture(const char *name, const char *subpath, unsigned int flags);
texid_tf R_LoadBumpmapTexture(const char *name, const char *subpath);
void R_LoadNumberedLightTexture(struct dlight_s *dl, int cubetexnum);

extern	texid_t	particletexture;
extern	texid_t particlecqtexture;
extern	texid_t explosiontexture;
extern	texid_t balltexture;
extern	texid_t beamtexture;
extern	texid_t ptritexture;

skinid_t Mod_RegisterSkinFile(const char *skinname);
skinid_t Mod_ReadSkinFile(const char *skinname, const char *skintext);
void Mod_WipeSkin(skinid_t id, qboolean force);
skinfile_t *Mod_LookupSkin(skinid_t id);

void	Mod_Init (qboolean initial);
void Mod_Shutdown (qboolean final);
int Mod_TagNumForName(struct model_s *model, const char *name, int firsttag);
int Mod_SkinNumForName(struct model_s *model, int surfaceidx, const char *name);
int Mod_FrameNumForName(struct model_s *model, int surfaceidx, const char *name);
int Mod_FrameNumForAction(struct model_s *model, int surfaceidx, int actionid);
float Mod_GetFrameDuration(struct model_s *model, int surfaceidx, int frameno);

void Mod_ResortShaders(void);
void	Mod_ClearAll (void);
struct model_s *Mod_FindName (const char *name);
void	*Mod_Extradata (struct model_s *mod);	// handles caching
void	Mod_TouchModel (const char *name);
void Mod_RebuildLightmaps (void);

void Mod_NowLoadExternal(struct model_s *loadmodel);
void GLR_LoadSkys (void);
void R_BloomRegister(void);

int Mod_RegisterModelFormatText(void *module, const char *formatname, char *magictext, qboolean (QDECL *load) (struct model_s *mod, void *buffer, size_t fsize));
int Mod_RegisterModelFormatMagic(void *module, const char *formatname, qbyte *magic, size_t magicsize, qboolean (QDECL *load) (struct model_s *mod, void *buffer, size_t fsize));
void Mod_UnRegisterModelFormat(void *module, int idx);
void Mod_UnRegisterAllModelFormats(void *module);
void Mod_ModelLoaded(void *ctx, void *data, size_t a, size_t b);
void Mod_SubmodelLoaded(struct model_s *mod, int state);

#ifdef RUNTIMELIGHTING
struct relight_ctx_s;
struct llightinfo_s;
void LightPlane (struct relight_ctx_s *ctx, struct llightinfo_s *threadctx, lightstyleindex_t surf_styles[MAXCPULIGHTMAPS], unsigned int *surf_expsamples, qbyte *surf_rgbsamples, qbyte *surf_deluxesamples, vec4_t surf_plane, vec4_t surf_texplanes[2], vec2_t exactmins, vec2_t exactmaxs, int texmins[2], int texsize[2], float lmscale);	//special version that doesn't know what a face is or anything.
struct relight_ctx_s *LightStartup(struct relight_ctx_s *ctx, struct model_s *model, qboolean shadows, qboolean skiplit);
void LightReloadEntities(struct relight_ctx_s *ctx, const char *entstring, qboolean ignorestyles);
void LightShutdown(struct relight_ctx_s *ctx, struct model_s *model);
extern const size_t lightthreadctxsize;

qboolean RelightSetup (struct model_s *model, size_t lightsamples, qboolean generatelit);
void RelightThink (void);
const char *RelightGetProgress(float *progress);	//reports filename and progress
void RelightTerminate(struct model_s *mod);	//NULL acts as a wildcard
#endif

struct builddata_s
{
	void (*buildfunc)(struct model_s *mod, struct msurface_s *surf, struct builddata_s *bd);
	qboolean paintlightmaps;
	void *facedata;
};
void Mod_Batches_Build(struct model_s *mod, struct builddata_s *bd);
shader_t *Mod_RegisterBasicShader(struct model_s *mod, const char *texname, unsigned int usageflags, const char *shadertext, uploadfmt_t pixelfmt, unsigned int width, unsigned int height, void *pixeldata, void *palettedata);

extern struct model_s		*currentmodel;

void Media_CaptureDemoEnd(void);
void Media_RecordFrame (void);
qboolean Media_PausedDemo (qboolean fortiming);
int Media_Capturing (void);
double Media_TweekCaptureFrameTime(double oldtime, double time);
void Media_WriteCurrentTrack(sizebuf_t *buf);
void Media_VideoRestarting(void);
void Media_VideoRestarted(void);

void MYgluPerspective(double fovx, double fovy, double zNear, double zFar);

void	R_PushDlights				(void);
void R_SetFrustum (float projmat[16], float viewmat[16]);
void R_SetRenderer(rendererinfo_t *ri);
qboolean R_RegisterRenderer(void *module, rendererinfo_t *ri);
struct plugvrfuncs_s;
qboolean R_RegisterVRDriver(void *module, struct plugvrfuncs_s *vrfuncs);
qboolean R_UnRegisterModule(void *module);
void R_AnimateLight (void);
void R_UpdateHDR(vec3_t org);
void R_UpdateLightStyle(unsigned int style, const char *stylestring, float r, float g, float b);
void R_BumpLightstyles(unsigned int maxstyle);	//bumps the cl_max_lightstyles array size, if needed.
qboolean R_CalcModelLighting(entity_t *e, struct model_s *clmodel);
struct texture_s *R_TextureAnimation (int frame, struct texture_s *base);	//mostly deprecated, only lingers for rtlights so world only.
struct texture_s *R_TextureAnimation_Q2 (struct texture_s *base);	//mostly deprecated, only lingers for rtlights so world only.
void RQ_Init(void);
void RQ_Shutdown(void);

qboolean WritePCXfile (const char *filename, enum fs_relative fsroot, qbyte *data, int width, int height, int rowbytes, qbyte *palette, qboolean upload); //data is 8bit.
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height);
void *ReadTargaFile(qbyte *buf, int length, int *width, int *height, uploadfmt_t *format, qboolean greyonly, uploadfmt_t forceformat);
qbyte *ReadPNGFile(const char *fname, qbyte *buf, int length, int *width, int *height, uploadfmt_t *format, qboolean force_rgb32);
qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out);

qbyte *ReadRawImageFile(qbyte *buf, int len, int *width, int *height, uploadfmt_t *format, qboolean force_rgba8, const char *fname);
void *Image_ResampleTexture (uploadfmt_t format, const void *in, int inwidth, int inheight, void *out,  int outwidth, int outheight);
void Image_ReadExternalAlpha(qbyte *rgbadata, size_t imgwidth, size_t imgheight, const char *fname, uploadfmt_t *format);
void BoostGamma(qbyte *rgba, int width, int height, uploadfmt_t fmt);
void SaturateR8G8B8(qbyte *data, int size, float sat);
void AddOcranaLEDsIndexed (qbyte *image, int h, int w);

void Renderer_Init(void);
void Renderer_Start(void);
qboolean Renderer_Started(void);
void R_ShutdownRenderer(qboolean videotoo);
void R_RestartRenderer_f (void);//this goes here so we can save some stack when first initing the sw renderer.

//used to live in glquake.h
qbyte GetPaletteIndexRange(int first, int stop, int red, int green, int blue);
qbyte GetPaletteIndex(int red, int green, int blue);
extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_drawviewmodelinvis;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_glsl_offsetmapping;
extern	cvar_t	r_skyfog;	//additional fog alpha on sky
extern	cvar_t	r_shadow_playershadows;
extern	cvar_t	r_shadow_realtime_dlight, r_shadow_realtime_dlight_shadows;
extern	cvar_t	r_shadow_realtime_dlight_ambient;
extern	cvar_t	r_shadow_realtime_dlight_diffuse;
extern	cvar_t	r_shadow_realtime_dlight_specular;
extern	cvar_t	r_shadow_realtime_world, r_shadow_realtime_world_shadows, r_shadow_realtime_world_lightmaps, r_shadow_realtime_world_importlightentitiesfrommap;
extern	float r_shadow_realtime_world_lightmaps_force;
extern	cvar_t	r_shadow_raytrace;
extern	cvar_t	r_shadow_shadowmapping;
extern	cvar_t	r_halfrate;
extern	cvar_t	r_mirroralpha;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_lavaalpha;
extern	cvar_t	r_slimealpha;
extern	cvar_t	r_telealpha;
extern	cvar_t	r_waterstyle;
extern	cvar_t	r_lavastyle;
extern	cvar_t	r_slimestyle;
extern	cvar_t	r_telestyle;
extern	cvar_t	r_dynamic;
extern qboolean r_dlightlightmaps;
extern	cvar_t	r_temporalscenecache;
extern	cvar_t	r_novis;
extern	cvar_t	r_netgraph;
extern	cvar_t	r_deluxemapping_cvar;
extern	qboolean r_deluxemapping;
#ifdef RTLIGHTS
extern	qboolean r_fakeshadows; //enables the use of ortho model-only shadows
#endif
extern	float	r_blobshadows;
extern	cvar_t r_softwarebanding_cvar;
extern	qboolean r_softwarebanding;
extern	cvar_t r_lightprepass_cvar;
extern	int r_lightprepass;	//0=off,1=16bit,2=32bit

extern cvar_t	r_xflip;

extern cvar_t gl_mindist, gl_maxdist;
extern	cvar_t	r_clear;
extern	cvar_t	r_clearcolour;
extern	cvar_t	gl_poly;
extern	cvar_t	gl_affinemodels;
extern	cvar_t r_renderscale;
extern	cvar_t	gl_nohwblend;
extern	cvar_t	r_coronas, r_coronas_intensity, r_coronas_occlusion, r_coronas_mindist, r_coronas_fadedist, r_flashblend, r_flashblendscale;
extern	cvar_t	r_lightstylesmooth;
extern	cvar_t	r_lightstylesmooth_limit;
extern	cvar_t	r_lightstylespeed;
extern	cvar_t	r_lightstylescale;
extern	cvar_t	r_lightmap_scale;
#ifdef QWSKINS
extern	cvar_t	gl_nocolors;
#endif
extern	cvar_t	gl_load24bit;
extern	cvar_t	gl_finish;

extern	cvar_t	gl_max_size;
extern	cvar_t	gl_playermip;

extern  cvar_t	r_lightmap_saturation;

#ifdef FTEPLUGIN	//evil hack... boo hiss.
extern cvar_t *cvar_r_meshpitch;
extern cvar_t *cvar_r_meshroll;
#define r_meshpitch (*cvar_r_meshpitch)
#define r_meshroll (*cvar_r_meshroll)
#else
extern cvar_t r_meshpitch;
extern cvar_t r_meshroll;	//gah!
#endif
extern cvar_t vid_hardwaregamma;

enum {
	RSPEED_TOTALREFRESH,
	RSPEED_CSQCPHYSICS,
	RSPEED_CSQCREDRAW,
	RSPEED_LINKENTITIES,
	RSPEED_WORLDNODE,
	RSPEED_DYNAMIC,
	RSPEED_OPAQUE,
	RSPEED_RTLIGHTS,
	RSPEED_TRANSPARENTS,
	RSPEED_PROTOCOL,
	RSPEED_PARTICLES,
	RSPEED_PARTICLESDRAW,
	RSPEED_PALETTEFLASHES,
	RSPEED_2D,
	RSPEED_SERVER,
	RSPEED_AUDIO,
	RSPEED_SETUP,
	RSPEED_SUBMIT,
	RSPEED_PRESENT,
	RSPEED_ACQUIRE,

	RSPEED_MAX
};
extern int rspeeds[RSPEED_MAX];

enum {
	RQUANT_MSECS,	//old r_speeds
	RQUANT_PRIMITIVEINDICIES,
	RQUANT_DRAWS,
	RQUANT_ENTBATCHES,
	RQUANT_WORLDBATCHES,
	RQUANT_2DBATCHES,

	RQUANT_SHADOWINDICIES,
	RQUANT_SHADOWEDGES,
	RQUANT_SHADOWSIDES,
	RQUANT_LITFACES,

	RQUANT_RTLIGHT_DRAWN,
	RQUANT_RTLIGHT_CULL_FRUSTUM,
	RQUANT_RTLIGHT_CULL_PVS,
	RQUANT_RTLIGHT_CULL_SCISSOR,

	RQUANT_MAX
};
extern int rquant[RQUANT_MAX];

#define RQuantAdd(type,quant) rquant[type] += quant

#if 0//defined(NDEBUG) || !defined(_WIN32)
#define RSpeedLocals()
#define RSpeedMark()
#define RSpeedRemark()
#define RSpeedEnd(spt)
#else
#define RSpeedLocals() double rsp
#define RSpeedMark() double rsp = (r_speeds.ival>1)?Sys_DoubleTime()*1000000:0
#define RSpeedRemark() rsp = (r_speeds.ival>1)?Sys_DoubleTime()*1000000:0

#if defined(_WIN32) && defined(GLQUAKE)
extern void (_stdcall *qglFinish) (void);
#define RSpeedEnd(spt) do {if(r_speeds.ival > 1){if(r_speeds.ival > 2 && qglFinish)qglFinish(); rspeeds[spt] += (double)(Sys_DoubleTime()*1000000) - rsp;}}while (0)
#else
#define RSpeedEnd(spt) rspeeds[spt] += (r_speeds.ival>1)?Sys_DoubleTime()*1000000 - rsp:0
#endif
#endif
