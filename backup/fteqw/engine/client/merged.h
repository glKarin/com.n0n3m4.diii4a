#ifndef MERGED_H
#define MERGED_H

#ifdef VKQUAKE
//we need some types available elsewhere, but don't really want to have to include the entire vulkan api everywhere.
//unfortunately, vulkan's handle types are not well defined.
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VulkanAPIRandomness void*
#elif defined(_MSC_VER) && _MSC_VER < 1300
#define VulkanAPIRandomness __int64
#else
#define VulkanAPIRandomness long long
#endif
#define qVkDescriptorSet VulkanAPIRandomness
#define qVkShaderModule VulkanAPIRandomness
#define qVkPipelineLayout VulkanAPIRandomness
#define qVkDescriptorSetLayout VulkanAPIRandomness
#define qVkBuffer VulkanAPIRandomness
#define qVkDeviceMemory VulkanAPIRandomness
#endif

//These are defined later in the source tree. This file should probably be moved to a later spot.
struct pubprogfuncs_s;
struct globalvars_s;
struct texture_s;
struct texnums_s;
struct vbo_s;
struct mesh_s;
struct batch_s;
struct entity_s;
struct dlight_s;
struct galiasbone_s;
struct dlight_s;
struct font_s;

typedef enum
{
	SKEL_RELATIVE,	//relative to parent.
	SKEL_ABSOLUTE,	//relative to model. doesn't blend very well.
	SKEL_INVERSE_RELATIVE,	//pre-inverted. faster than regular relative but has weirdness with skeletal objects. blends okay.
	SKEL_INVERSE_ABSOLUTE,	//final renderable type.
	SKEL_IDENTITY,	//PANIC
	SKEL_QUATS		//quat+org, 7 floats rather than 12. better lerping.
} skeltype_t;

#ifdef HALFLIFEMODELS
	#define MAX_BONE_CONTROLLERS 5
#endif

#ifdef HAVE_LEGACY
#define FRAME_BLENDS 4	//for compat with DP (for mods that want 4-way blending yet refuse to use framegroups properly). real mods should be using skeletal objects allowing for N-way blending.
#else
#define FRAME_BLENDS 2
#endif

#define FST_BASE 0	//base frames
#define FS_REG 1	//regular frames
#define FS_COUNT 2	//regular frames
typedef struct framestate_s {
	struct framestateregion_s {
		int frame[FRAME_BLENDS];
		float frametime[FRAME_BLENDS];
		float lerpweight[FRAME_BLENDS];

#ifdef HALFLIFEMODELS
		float subblendfrac;		//hl models are weird
		float subblend2frac;	//very weird.
#endif

		int endbone;
	} g[FS_COUNT];

#ifdef SKELETALOBJECTS
	float *bonestate;
	int bonecount;
	skeltype_t skeltype;
#endif

#ifdef HALFLIFEMODELS
	float bonecontrols[MAX_BONE_CONTROLLERS];	//hl special bone controllers
#endif
} framestate_t;
#define NULLFRAMESTATE (framestate_t*)NULL




//function prototypes

#if defined(SERVERONLY)
#define qrenderer QR_NONE
#define FNC(n) (n)			//FNC is defined as 'pointer if client build, direct if dedicated server'

#else
#define FNC(n) (*n)
extern r_qrenderer_t qrenderer;
extern char *q_renderername;

apic_t *R2D_LoadAtlasedPic(const char *name);
void R2D_ImageAtlas(float x, float y, float w, float h, float s1, float t1, float s2, float t2, apic_t *pic);
#define R2D_ScalePicAtlas(x,y,w,h,p) R2D_ImageAtlas(x,y,w,h,0,0,1,1,p)

mpic_t *R2D_SafeCachePic (const char *path);
mpic_t *R2D_SafePicFromWad (const char *name);
void R2D_DrawCrosshair (void);
void R2D_ScalePic (float x, float y, float width, float height, mpic_t *pic);
void R2D_SubPic(float x, float y, float width, float height, mpic_t *pic, float srcx, float srcy, float srcwidth, float srcheight);
void R2D_TransPicTranslate (float x, float y, int width, int height, qbyte *pic, unsigned int *palette);
void R2D_TileClear (float x, float y, float w, float h);
void R2D_FadeScreen (void);

void R2D_Font_Changed(void);
void R2D_ConsoleBackground (int firstline, int lastline, qboolean forceopaque);
void R2D_EditorBackground (void);
qboolean R2D_DrawLevelshot(void);

void R2D_Image(float x, float y, float w, float h, float s1, float t1, float s2, float t2, mpic_t *pic);
void R2D_Image2dQuad(vec2_t const*points, vec2_t const*texcoords, vec4_t const*rgba, mpic_t *pic);

void R2D_ImageColours(float r, float g, float b, float a);
void R2D_ImagePaletteColour(unsigned int i, float a);
void R2D_FillBlock(float x, float y, float w, float h);
void R2D_Line(float x1, float y1, float x2, float y2, mpic_t *pic);
extern void (*R2D_Flush)(void);

extern void	(*Draw_Init)							(void);

extern void	(*R_Init)								(void);
extern void	(*R_DeInit)								(void);
extern void	(*R_RenderView)							(void);		// must set r_refdef first

extern qboolean	(*VID_Init)							(rendererstate_t *info, unsigned char *palette);
extern void	(*VID_DeInit)							(void);
extern char *(*VID_GetRGBInfo)						(int *stride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt); //if stride is negative, then the return value points to the last line intead of the first. this allows it to be freed normally.
extern void	(*VID_SetWindowCaption)					(const char *msg);

extern void *SCR_ScreenShot_Capture					(int fbwidth, int fbheight, int *stride, enum uploadfmt *fmt, qboolean no2d, qboolean hdr);
extern void SCR_Init								(void);
extern void SCR_DeInit								(void);
extern qboolean (*SCR_UpdateScreen)					(void);
extern void SCR_BeginLoadingPlaque					(void);
extern void SCR_EndLoadingPlaque					(void);
extern void SCR_DrawConsole							(qboolean noback);
extern void SCR_SetUpToDrawConsole					(void);
extern void SCR_CenterPrint							(int pnum, const char *str, qboolean skipgamecode);

int R_DrawTextField(int x, int y, int w, int h, const char *text, unsigned int defaultmask, unsigned int fieldflags, struct font_s *font, vec2_t fontscale);
#define CPRINT_LALIGN		(1<<0)	//L
#define CPRINT_TALIGN		(1<<1)	//T
#define CPRINT_RALIGN		(1<<2)	//R
#define CPRINT_BALIGN		(1<<3)	//B
#define CPRINT_BACKGROUND	(1<<4)	//P
#define CPRINT_NOWRAP		(1<<5)

#define CPRINT_OBITUARTY	(1<<16)	//O (show at 2/3rds from top)
#define CPRINT_PERSIST		(1<<17)	//P (doesn't time out)
#define CPRINT_TYPEWRITER	(1<<18)	//  (char at a time)
#define CPRINT_CURSOR		(1<<19)	//C (use a mouse cursor, also enabled by the presence of a (auto) link)

#endif

//mod_purge flags
enum mod_purge_e
{
	MP_MAPCHANGED,	//new map. old stuff no longer needed, can skip stuff if it'll be expensive.
	MP_FLUSH,		//user flush command. anything flushable goes.
	MP_RESET		//*everything* is destroyed. renderer is going down, or at least nothing depends upon it.
};
enum mlverbosity_e
{
	MLV_SILENT,
	MLV_SILENTSYNC,
	MLV_WARN,
	MLV_WARNSYNC,
	MLV_ERROR
};

extern struct model_s *mod_known; //for evil people that want to do evil indexing.
const char *Mod_GetEntitiesString(struct model_s *mod);
void Mod_SetEntitiesStringLen(struct model_s *mod, const char *str, size_t strsize);
void Mod_SetEntitiesString(struct model_s *mod, const char *str, qboolean docopy);
qboolean Mod_LoadEntitiesBlob(struct model_s *mod, const char *entdata, size_t entdatasize);	//initial read, with .ent file replacement etc
void Mod_ParseEntities(struct model_s *mod);
void Mod_LoadMapArchive(struct model_s *mod, void *archivedata, size_t archivesize);
extern void	Mod_ClearAll						(void);
extern void Mod_Purge							(enum mod_purge_e type);
extern void Mod_SetModifier						(const char *modifier);
extern char mod_modifier[];
extern qboolean Mod_PurgeModel					(struct model_s	*mod, enum mod_purge_e ptype);
extern struct model_s *Mod_FindName				(const char *name);	//find without loading. needload should be set.
extern struct model_s *Mod_ForName				(const char *name, enum mlverbosity_e verbosity);	//finds+loads
extern struct model_s *Mod_LoadModel			(struct model_s *mod, enum mlverbosity_e verbose);	//makes sure a model is loaded
extern void Mod_FileWritten						(const char *filename);
extern void	*Mod_Extradata						(struct model_s *mod);	// handles caching
extern void	Mod_TouchModel						(const char *name);
extern const char *Mod_FixName					(const char *modname, const char *worldname);	//remaps the name appropriately
const char *Mod_ParseWorldspawnKey				(struct model_s *mod, const char *key, char *buffer, size_t sizeofbuffer);

extern qboolean Mod_GetModelEvent				(struct model_s *model, int animation, int eventidx, float *timestamp, int *eventcode, char **eventdata);
extern int Mod_SkinNumForName					(struct model_s *model, int surfaceidx, const char *name);
extern int Mod_FrameNumForName					(struct model_s *model, int surfaceidx, const char *name);
extern float Mod_GetFrameDuration				(struct model_s *model, int surfaceidx, int framenum);
extern int Mod_GetFrameCount					(struct model_s *model);

#undef FNC

extern qboolean	Mod_GetTag						(struct model_s *model, int tagnum, framestate_t *framestate, float *transforms);
extern int Mod_TagNumForName					(struct model_s *model, const char *name, int firsttag);

void Mod_AddSingleSurface(struct entity_s *ent, int surfaceidx, shader_t *shader, int mode);
int Mod_GetNumBones(struct model_s *model, qboolean allowtags);
int Mod_GetBoneRelations(struct model_s *model, int firstbone, int lastbone, const struct galiasbone_s *boneinfo, const framestate_t *fstate, float *result);
int Mod_GetBoneParent(struct model_s *model, int bonenum);
struct galiasbone_s *Mod_GetBoneInfo(struct model_s *model, int *numbones);
const char *Mod_GetBoneName(struct model_s *model, int bonenum);

void Draw_FunString(float x, float y, const void *str);
void Draw_FunStringU8(unsigned int flags, float x, float y, const void *str);
void Draw_AltFunString(float x, float y, const void *str);
void Draw_FunStringWidthFont(struct font_s *font, float x, float y, const void *str, int width, int rightalign, qboolean highlight);
#define Draw_FunStringWidth(x,y,str,width,rightalign,highlight) Draw_FunStringWidthFont(font_default,x,y,str,width,rightalign,highlight)

extern int r_regsequence;

enum
{
	TEX_NOTLOADED,
	TEX_LOADING,
	TEX_LOADED,
	TEX_FAILED
};
typedef struct image_s
{
#ifdef _DEBUG
	char dbgident[32];
#endif
	char *ident;	//allocated on end
	char *subpath;	//allocated on end
	int regsequence;
	int width;	//this is the logical size. the physical size is not considered important (except for render targets, which should not be loaded from disk).
	int height;
	int depth;
	uploadfmt_t format;
	int status;	//TEX_
	unsigned int flags;
	struct image_s *next;
//	struct image_s *prev;
	struct image_s *aliasof;
	union
	{
		int nosyntaxerror;
#if defined(D3DQUAKE) || defined(SWQUAKE)
		struct
		{
			void *ptr;	//texture
			void *ptr2;	//view
		};
#endif
#ifdef GLQUAKE
		int num;
#endif
#ifdef VKQUAKE
		struct
		{
			qVkDescriptorSet vkdescriptor;
			struct vk_image_s *vkimage;
		};
#endif
	};

	void *fallbackdata;
	int fallbackwidth;
	int fallbackheight;
	uploadfmt_t fallbackfmt;
} image_t;

#if 1
typedef image_t *texid_t;
#define texid_tf texid_t
#define TEXASSIGN(d,s) d=s
#define TEXASSIGNF(d,s) d=s
#define TEXVALID(t) ((t))
#define TEXLOADED(tex) ((tex) && (tex)->status == TEX_LOADED)
#define TEXDOWAIT(tex)	do{if ((tex) && (tex)->status == TEX_LOADING)	COM_WorkerPartialSync((tex), &(tex)->status, TEX_LOADING);}while(0)
#else
typedef struct texid_s texid_t[1];
typedef struct texid_s texid_tf;
#define TEXASSIGN(d,s) memcpy(&d,&s,sizeof(d))
#define TEXASSIGNF(d,s) memcpy(&d,&s,sizeof(d))
#define TEXVALID(t) 1
#endif

#define Image_LinearFloatFromsRGBFloat(c) (((c) <= 0.04045f) ? (c) * (1.0f / 12.92f) : (float)pow(((c) + 0.055f)*(1.0f/1.055f), 2.4f))
#define Image_sRGBFloatFromLinearFloat(c) (((c) < 0.0031308f) ? (c) * 12.92f : 1.055f * (float)pow((c), 1.0f/2.4f) - 0.055f)

struct pendingtextureinfo
{
	enum imgtype_e
	{
		//formats are all w*h*d (where depth has limitations according to type)
		PTI_2D,			//w*h*1 - depth MUST be 1
		PTI_3D,			//w*h*d - we can't generate 3d mips
		PTI_CUBE,		//w*h*6 - depth MUST be 6 (faces must be tightly packed)
		PTI_2D_ARRAY,	//w*h*layers - depth is =layers
		PTI_CUBE_ARRAY,	//w*h*(layers*6) - depth is =(layers*6).

		PTI_ANY			//says we don't care.
	} type;

	uploadfmt_t encoding;	//PTI_* formats
	void *extrafree;		//avoids some memcpys
	unsigned int mipcount;
	struct
	{
		void *data;
		size_t datasize; //ceil(width/blockwidth)*ceil(height/blockheight)*ceil(depth/blockdepth)*blocksize - except that blockdepth is always considered 1 for now.
		unsigned int width;
		unsigned int height;
		int depth;
		qboolean needfree;
	} mip[72];	//enough for a 4096 cubemap. or a really smegging big 2d texture...
	//mips are ordered as in arrayindex THEN mip order, allowing easy truncation of mip levels.
	//cubemaps are just arrayindex*6
};

//small context for easy vbo creation.
typedef struct
{
	size_t maxsize;
	size_t pos;
	int vboid[2];
	void *vboptr[2];
	void *fallback;
} vbobctx_t;

typedef union vboarray_s
{
	void *sysptr;
#ifdef GLQUAKE
	struct
	{
		int vbo;
		void *addr;
	} gl;
#endif
#if defined(D3D8QUAKE) || defined(D3D9QUAKE) || defined(D3D11QUAKE)
	struct
	{
		void *buff;
		unsigned int offs;
	} d3d;
#endif
#ifdef VKQUAKE
	struct
	{
		qVkBuffer buff;
		unsigned int offs;
	} vk;
#endif
	struct
	{	//matches the biggest version. currently vulkan. this ensures that plugins can allocate model data without caring about renderers.
		qint64_t buff;
		unsigned int offs;
	} pad;
} vboarray_t;

//scissor rects
typedef struct
{
	float x;
	float y;
	float width;
	float height;
	double dmin;
	double dmax;
} srect_t;

typedef struct texnums_s {
	char	mapname[MAX_QPATH];	//the 'official' name of the diffusemap. used to generate filenames for other textures.
	texid_t base;			//2d,	regular diffuse texture. may have alpha if surface is transparent.
	texid_t bump;			//2d,	normalmap. height values packed in alpha.
	texid_t specular;		//2d,	specular lighting values. alpha contains exponent multiplier
	texid_t upperoverlay;	//2d,	diffuse texture for the upper body(shirt colour). no alpha channel. added to base.rgb. ideally an l8 texture
	texid_t loweroverlay;	//2d,	diffuse texture for the lower body(trouser colour). no alpha channel. added to base.rgb. ideally an l8 texture
	texid_t paletted;		//2d,	8bit paletted data, just because. only red is used.
	texid_t fullbright;		//2d,	emissive texture. alpha should be 1.
	texid_t reflectcube;	//cube,	for fake reflections
	texid_t reflectmask;	//2d,	defines how reflective it is (for cubemap reflections)
	texid_t displacement;	//2d,	alternative to bump.a, eg R16[F] for offsetmapping or tessellation
	texid_t occlusion;		//2d,	occlusion map... only red is used.
	texid_t transmission;	//2d,	multiplier for transmissionfactor... only red is used.
	texid_t thickness;		//2d,	multiplier for thicknessfactor... only green is used.

	//the material's pushconstants. vulkan guarentees only 128 bytes. so 8 vec4s. note that lmscales should want 4 of them...
	/*struct
	{
		vec4_t basefactors;
		vec4_t specfactors;
		vec4_t fullbrightfactors;

		//FIXME: envmap index, lightmap index, etc.
	} factors;*/
} texnums_t;

//not all modes accept meshes - STENCIL(intentional) and DEPTHONLY(not implemented)
typedef enum backendmode_e
{
	BEM_STANDARD,			//regular mode to draw surfaces akin to q3 (aka: legacy mode). lightmaps+delux+ambient
	BEM_DEPTHONLY,			//just a quick depth pass. textures used only for alpha test (shadowmaps).
	BEM_WIREFRAME,			//for debugging or something
	BEM_STENCIL,			//used for drawing shadow volumes to the stencil buffer.
	BEM_DEPTHDARK,			//a quick depth pass. textures used only for alpha test. additive textures still shown as normal.
	BEM_CREPUSCULAR,		//sky is special, everything else completely black
	BEM_GBUFFER,			//
	BEM_FOG,				//drawing a fog volume
	BEM_LIGHT,				//we have a valid light
} backendmode_t;

typedef struct rendererinfo_s {
	char *description;
	char *name[5];
	r_qrenderer_t rtype;
	//FIXME: all but the vid stuff really should be filled in by the video code, simplifying system-specific stuff.

//FIXME: remove these...
	void	(*Draw_Init)				(void);
	void	(*Draw_Shutdown)			(void);

	void	 (*IMG_UpdateFiltering)		(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis);
	qboolean (*IMG_LoadTextureMips)		(texid_t tex, const struct pendingtextureinfo *mips);
	void	 (*IMG_DestroyTexture)		(texid_t tex);

	void	 (*R_Init)					(void); //FIXME - merge implementations
	void	 (*R_DeInit)					(void);	//FIXME - merge implementations
	void	 (*R_RenderView)				(void);	// must set r_refdef first

//FIXME: keep these...
	qboolean (*VID_Init)				(rendererstate_t *info, unsigned char *palette);
	void	 (*VID_DeInit)				(void);
	void	 (*VID_SwapBuffers)			(void);	//force a buffer swap, regardless of what's displayed.
	qboolean (*VID_ApplyGammaRamps)		(unsigned int size, unsigned short *ramps);

	void	*(*VID_CreateCursor)			(const qbyte *imagedata, int width, int height, uploadfmt_t format, float hotx, float hoty, float scale);	//may be null, stub returns null
	qboolean (*VID_SetCursor)			(void *cursor);	//may be null
	void	 (*VID_DestroyCursor)			(void *cursor);	//may be null

	void	 (*VID_SetWindowCaption)		(const char *msg);

//FIXME: add clipboard stuff...

//FIXME: remove these...
	char	*(*VID_GetRGBInfo)			(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt);

	qboolean (*SCR_UpdateScreen)			(void);

	
	//Select the current render mode and modifier flags
	void	(*BE_SelectMode)(backendmode_t mode);
	/*Draws an entire mesh list from a VBO. vbo can be null, in which case the chain may be drawn without batching.
	  Rules for using a list: Every mesh must be part of the same VBO, shader, lightmap, and must have the same pointers set*/
	void	(*BE_DrawMesh_List)(shader_t *shader, int nummeshes, struct mesh_s **mesh, struct vbo_s *vbo, struct texnums_s *texnums, unsigned int be_flags);
	void	(*BE_DrawMesh_Single)(shader_t *shader, struct mesh_s *meshchain, struct vbo_s *vbo, unsigned int be_flags);
	void	(*BE_SubmitBatch)(struct batch_s *batch);
	struct batch_s *(*BE_GetTempBatch)(void);
	//Asks the backend to invoke DrawMeshChain for each surface, and to upload lightmaps as required
	void	(*BE_DrawWorld) (struct batch_s **worldbatches);
	//called at init, force the display to the right defaults etc
	void	(*BE_Init)(void);
	//Generates an optimised VBO, one for each texture on the map
	void	(*BE_GenBrushModelVBO)(struct model_s *mod);
	//Destroys the given vbo
	void	(*BE_ClearVBO)(struct vbo_s *vbo, qboolean dataonly);
	//Uploads all modified lightmaps
	void	(*BE_UploadAllLightmaps)(void);
	void	(*BE_SelectEntity)(struct entity_s *ent);
	qboolean (*BE_SelectDLight)(struct dlight_s *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode);
	void	(*BE_Scissor)(srect_t *rect);
	/*check to see if an ent should be drawn for the selected light*/
	qboolean (*BE_LightCullModel)(vec3_t org, struct model_s *model);
	void	(*BE_VBO_Begin)(vbobctx_t *ctx, size_t maxsize);
	void	(*BE_VBO_Data)(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray);
	void	(*BE_VBO_Finish)(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray, void **vbomem, void **ebomem);
	void	(*BE_VBO_Destroy)(vboarray_t *vearray, void *mem);
	void	(*BE_RenderToTextureUpdate2d)(qboolean destchanged);

	char *alignment;	//just to make sure that added functions cause compile warnings.

	int		(*VID_GetPriority)	(void);	//so that eg x11 or wayland can be prioritised depending on environment settings. assumed to be 1.
	void	(*VID_EnumerateVideoModes) (const char *driver, const char *output, void (*cb) (int w, int h));
	qboolean(*VID_EnumerateDevices) (void *usercontext, void(*callback)(void *context, const char *devicename, const char *outputname, const char *desc));
	qboolean(*VID_MayRefresh)	(void);

//FIXME: add getdestopres
//FIXME: add clipboard handling
} rendererinfo_t;

#define rf currentrendererstate.renderer

#define VID_SwapBuffers		rf->VID_SwapBuffers
#define VID_MayRefresh		rf->VID_MayRefresh

#define BE_Init					rf->BE_Init
#define BE_SelectMode			rf->BE_SelectMode
#define BE_GenBrushModelVBO		rf->BE_GenBrushModelVBO
#define BE_ClearVBO				rf->BE_ClearVBO
#define BE_UploadAllLightmaps	rf->BE_UploadAllLightmaps
#define BE_LightCullModel		rf->BE_LightCullModel
#define BE_SelectEntity			rf->BE_SelectEntity
#define BE_SelectDLight			rf->BE_SelectDLight
#define BE_GetTempBatch			rf->BE_GetTempBatch
#define BE_SubmitBatch			rf->BE_SubmitBatch
#define BE_DrawMesh_List		rf->BE_DrawMesh_List
#define BE_DrawMesh_Single		rf->BE_DrawMesh_Single
#define BE_SubmitMeshes			rf->BE_SubmitMeshes
#define BE_DrawWorld			rf->BE_DrawWorld
#define BE_VBO_Begin 			rf->BE_VBO_Begin
#define BE_VBO_Data				rf->BE_VBO_Data
#define BE_VBO_Finish			rf->BE_VBO_Finish
#define BE_VBO_Destroy			rf->BE_VBO_Destroy
#define BE_Scissor				rf->BE_Scissor

#define BE_RenderToTextureUpdate2d rf->BE_RenderToTextureUpdate2d

#define RT_IMAGEFLAGS (IF_NOMIPMAP|IF_CLAMP|IF_LINEAR|IF_RENDERTARGET)
texid_t R2D_RT_Configure(const char *id, int width, int height, uploadfmt_t rtfmt, unsigned int imageflags);
texid_t R2D_RT_GetTexture(const char *id, unsigned int *width, unsigned int *height);

#endif //MERGED_H
