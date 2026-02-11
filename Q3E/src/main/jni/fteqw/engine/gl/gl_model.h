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

#ifndef __MODEL__
#define __MODEL__

#include "../client/modelgen.h"
#include "../client/spritegn.h"

struct hull_s;
struct trace_s;
struct wedict_s;
struct model_s;
struct world_s;
struct dlight_s;
typedef struct builddata_s builddata_t;
typedef struct bspx_header_s bspx_header_t;

typedef enum {
	SHADER_SORT_NONE,
	SHADER_SORT_RIPPLE,			//new
	SHADER_SORT_DEFERREDLIGHT,	//new
	SHADER_SORT_PORTAL,
	SHADER_SORT_SKY,			//aka environment
	SHADER_SORT_OPAQUE,
	//fixme: occlusion tests
	SHADER_SORT_DECAL,
	SHADER_SORT_SEETHROUGH,
	//then rtlights are drawn
	SHADER_SORT_UNLITDECAL,		//new
	SHADER_SORT_BANNER,
	//fog
	SHADER_SORT_UNDERWATER,
	SHADER_SORT_BLEND,
	SHADER_SORT_ADDITIVE,
	//blend2,3,6
	//stencilshadow
	//almostnearest
	SHADER_SORT_NEAREST,


	SHADER_SORT_COUNT
} shadersort_t;

#ifdef FTE_TARGET_WEB
#define MAX_BONES 256
#elif defined(IQMTOOL)
#define MAX_BONES 8192
#else
#define MAX_BONES 256	//Note: there's lots of bone data allocated on the stack, so don't bump recklessly.
#endif
#if MAX_BONES>65536
#define GL_BONE_INDEX_TYPE GL_UNSIGNED_INT
typedef unsigned int boneidx_t;
#elif MAX_BONES>256
#define GL_BONE_INDEX_TYPE GL_UNSIGNED_SHORT
typedef unsigned short boneidx_t;
#else
#define GL_BONE_INDEX_TYPE GL_UNSIGNED_BYTE
typedef unsigned char boneidx_t;
#endif
typedef boneidx_t bone_vec4_t[4];

struct doll_s;
void rag_uninstanciateall(void);
void rag_flushdolls(qboolean force);
void rag_freedoll(struct doll_s *doll);
struct doll_s *rag_createdollfromstring(struct model_s *mod, const char *fname, int numbones, const char *file);
struct world_s;
void rag_doallanimations(struct world_s *world);
void rag_removedeltaent(lerpents_t *le);
void rag_updatedeltaent(struct world_s *w, entity_t *ent, lerpents_t *le);
void rag_lerpdeltaent(lerpents_t *le, unsigned int bonecount, short *newstate, float frac, short *oldstate);
void skel_reset(struct world_s *world);

typedef struct mesh_s
{
	int				numvertexes;
	int				numindexes;

	/*position within its vbo*/
	unsigned int	vbofirstvert;
	unsigned int	vbofirstelement;

	/*
	FIXME: move most of this stuff out into a vbo struct
	*/

	float			xyz_blendw[2];

	/*arrays used for rendering*/
	vecV_t			*xyz_array;
	vecV_t			*xyz2_array;
	vec3_t			*normals_array;	/*required for lighting*/
	vec3_t			*snormals_array;/*required for rtlighting*/
	vec3_t			*tnormals_array;/*required for rtlighting*/
	vec2_t			*st_array;		/*texture coords*/
	vec2_t			*lmst_array[MAXRLIGHTMAPS];	/*second texturecoord set (merely dubbed lightmap, one for each potential lightstyle)*/
	avec4_t			*colors4f_array[MAXRLIGHTMAPS];/*floating point colours array*/
	byte_vec4_t		*colors4b_array;/*byte colours array*/

    index_t			*indexes;

	//required for shadow volumes
	int				*trneighbors;
	vec3_t			*trnormals;

	qboolean		istrifan;	/*if its a fan/poly/single quad  (permits optimisations)*/
	const float		*bones;
	int				numbones;
	bone_vec4_t		*bonenums;
	vec4_t			*boneweights;
} mesh_t;

/*
batches are generated for each shader/ent as required.
once a batch is known to the backend for that frame, its shader, vbo, ent, lightmap, textures may not be changed until the frame has finished rendering. This is to potentially permit caching.
*/
typedef struct batch_s
{
	mesh_t **mesh; /*list must be long enough for all surfaces that will form part of this batch times two, for mirrors/portals*/
	struct batch_s *next;
	unsigned int meshes;
	unsigned int firstmesh;

	shader_t *shader;
	struct vbo_s *vbo;
	entity_t *ent;	/*used for shader properties*/
	struct mfog_s *fog;
	image_t		*envmap;

	short lightmap[MAXRLIGHTMAPS];	/*used for shader lightmap textures*/
	lightstyleindex_t lmlightstyle[MAXRLIGHTMAPS];
	unsigned char vtlightstyle[MAXRLIGHTMAPS];

	unsigned int maxmeshes;	/*not used by backend*/
	unsigned int flags;	/*backend flags (force transparency etc)*/
	struct texture_s *texture; /*is this used by the backend?*/
	struct texnums_s *skin;

	void (*buildmeshes)(struct batch_s *b);
#if R_MAX_RECURSE > 2
	unsigned int recursefirst[R_MAX_RECURSE-2];	//fixme: should thih, firstmesh, and meshes be made ushorts?
#endif
	/*caller-use, not interpreted by backend*/
	union
	{
		struct
		{
			unsigned int ebobatch;		//temporal scene cache stuff, basically just a simple index so we don't have to deal with shader sort values when generating new index lists.
			unsigned int shadowbatch;	//a unique index to accelerate shadowmesh generation (dlights, yay!)
//		} bmodel;
//		struct
//		{
			vec4_t plane;	/*used only at load (for portal surfaces, so multiple planes are not part of the same batch)*/
		} bmodel;	//bmodel surfaces.
		struct
		{
			unsigned int lightidx;
			unsigned int lightmode;
		} dlight;	//deferred light batches
		struct
		{
			unsigned short surfrefs[sizeof(mesh_t)/sizeof(unsigned short)];	//for hlmdl batching...
		} alias;
		struct
		{
			unsigned int surface;
		} poly;
	/*	struct
		{
			unsigned int first;
			unsigned int count;
		} surf;*/
		struct
		{
			unsigned int ebobatch;		//temporal scene cache stuff, basically just a simple index so we don't have to deal with shader sort values when generating new index lists.
			mesh_t meshbuf;
			mesh_t *meshptr;
		};
	} user;
} batch_t;
/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define	EF_BRIGHTFIELD			(1<<0)
#define	EF_MUZZLEFLASH 			(1<<1)
#define	EF_BRIGHTLIGHT 			(1<<2)
#define	EF_DIMLIGHT 			(1<<3)
#define	QWEF_FLAG1	 			(1<<4)	//only applies to qw player entities
#define	NQEF_NODRAW				(1<<4)	//so packet entities are free to get this instead
#define REEF_QUADLIGHT			(1<<4)
#define TENEBRAEEF_FULLDYNAMIC	(1<<4)
#define	QWEF_FLAG2	 			(1<<5)	//only applies to qw player entities
#define	NQEF_ADDITIVE			(1<<5)	//so packet entities are free to get this instead
#define REEF_PENTLIGHT			(1<<5)
#define TENEBRAEEF_GREEN		(1<<5)
#define	EF_BLUE					(1<<6)
#define REEF_CANDLELIGHT		(1<<6)
#define	EF_RED					(1<<7)
#define	H2EF_NODRAW				(1<<7)	//this is going to get complicated... emulated server side.
#define	DPEF_NOGUNBOB			(1<<8)	//viewmodel attachment does not bob. only applies to viewmodelforclient/RF_WEAPONMODEL
#define	EF_FULLBRIGHT			(1<<9)	//abslight=1
#define	DPEF_FLAME				(1<<10)	//'onfire'
#define	DPEF_STARDUST			(1<<11)	//'showering sparks'
#define	EF_NOSHADOW		 		(1<<12)	//doesn't cast a shadow
#define	EF_NODEPTHTEST			(1<<13)	//shows through walls.
#define		DPEF_SELECTABLE_		(1<<14)	//highlights when prydoncursored
#define		DPEF_DOUBLESIDED_		(1<<15)	//disables culling
#define		DPEF_NOSELFSHADOW_		(1<<16)	//doesn't cast shadows on any noselfshadow entities.
#define		DPEF_DYNAMICMODELLIGHT_	(1<<17)	//forces dynamic lights... I have no idea what this is actually needed for.
#define	EF_GREEN				(1<<18)
#define	EF_UNUSED19				(1<<19)
#define	EF_RESTARTANIM_BIT		(1<<20)	//restarts the anim when toggled between states
#define	EF_TELEPORT_BIT			(1<<21)	//disable lerping when toggled between states
#define DPEF_LOWPRECISION		(1<<22) //part of the protocol/server, not the client itself.
#define EF_NOMODELFLAGS			(1<<23)
#define EF_MF_ROCKET			(1<<24)
#define EF_MF_GRENADE			(1<<25)
#define EF_MF_GIB				(1<<26)
#define EF_MF_ROTATE			(1<<27)
#define EF_MF_TRACER			(1<<28)
#define EF_MF_ZOMGIB			(1u<<29)
#define EF_MF_TRACER2			(1u<<30)
#define EF_MF_TRACER3			(1u<<31)

#define EF_HASPARTICLETRAIL		(0xff800000 | EF_BRIGHTFIELD|DPEF_FLAME|DPEF_STARDUST)

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

struct mnode_s;

typedef struct
{
	qbyte *buffer;	//reallocated if needed.
	size_t buffersize;
} pvsbuffer_t;
typedef enum
{
	PVM_FAST,
	PVM_MERGE,	//merge the pvs bits into the provided buffer
	PVM_REPLACE,//return value is guarenteed to be the provided buffer.
} pvsmerge_t;

typedef struct {
	//model is being purged from memory.
	void (*PurgeModel) (struct model_s *mod);

	unsigned int (*PointContents)	(struct model_s *model, const vec3_t axis[3], const vec3_t p);
//	unsigned int (*BoxContents)		(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs);

	//deals with whatever is native for the bsp (gamecode is expected to distinguish this).
	qboolean (*NativeTrace)		(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p1, const vec3_t p2, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int against, struct trace_s *trace);
	unsigned int (*NativeContents)(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs);

	unsigned int (*FatPVS)		(struct model_s *model, const vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge);
	qboolean (*EdictInFatPVS)	(struct model_s *model, const struct pvscache_s *edict, const qbyte *pvs, const int *areas);	//areas[0] is the count of accepted areas, if valid.
	void (*FindTouchedLeafs)	(struct model_s *model, struct pvscache_s *ent, const vec3_t cullmins, const vec3_t cullmaxs);	//edict system as opposed to q2 game dll system.

	void (*LightPointValues)	(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir);
	void (*StainNode)			(struct model_s *model, float *parms);
	void (*MarkLights)			(struct dlight_s *light, dlightbitmask_t bit, struct mnode_s *node);

	int	(*ClusterForPoint)		(struct model_s *model, const vec3_t point, int *areaout);	//pvs index (leaf-1 for q1bsp). may be negative (ie: no pvs).
	qbyte *(*ClusterPVS)		(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge);
	qbyte *(*ClusterPHS)		(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer);
	qbyte *(*ClustersInSphere)	(struct model_s *model, const vec3_t point, float radius, pvsbuffer_t *pvsbuffer, const qbyte *fte_restrict unionwith);

	size_t (*WriteAreaBits)		(struct model_s *model, qbyte *buffer, size_t maxbytes, int area, qboolean merge);	//writes a set of bits valid for a specific viewpoint's area.
	qboolean (*AreasConnected)	(struct model_s *model, unsigned int area1, unsigned int area2);	//fails if there's no open doors
	void (*SetAreaPortalState)	(struct model_s *model, unsigned int portal, unsigned int area1, unsigned int area2, qboolean open);	//a door moved...
	size_t (*SaveAreaPortalBlob)(struct model_s *model, void **ptr);				//for vid_reload to not break portals. dupe the ptrbefore freeing the model.
	size_t (*LoadAreaPortalBlob)(struct model_s *model, void *ptr, size_t size);	//for vid_reload to not break portals (has refcount info etc).

	void (*PrepareFrame)		(struct model_s *model, refdef_t *refdef, int area, int clusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out);
	void (*GenerateShadowMesh)	(struct model_s *model, dlight_t *dl, const qbyte *lvis, qbyte *truevis, void(*callback)(struct msurface_s*));
	void (*InfoForPoint)		(struct model_s *model, vec3_t pos, int *area, int *cluster, unsigned int *contentbits);
} modelfuncs_t;




//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

typedef struct vbo_s
{
	unsigned int numvisible;
	struct msurface_s **vislist;

	unsigned int indexcount;
	unsigned int vertcount;
	unsigned int meshcount;
	struct msurface_s **meshlist;

	vboarray_t		indicies;
	void *vertdata; /*internal use*/

	int vao;
	unsigned int vaodynamic;	/*mask of the attributes that are dynamic*/
	unsigned int vaoenabled;	/*mask of the attributes *currently* enabled. renderer may change this */
	vboarray_t coord;
	vboarray_t coord2;
	vboarray_t texcoord;
	vboarray_t lmcoord[MAXRLIGHTMAPS];

	vboarray_t normals;
	vboarray_t svector;
	vboarray_t tvector;

	qboolean colours_bytes;
	vboarray_t colours[MAXRLIGHTMAPS];

	vboarray_t bonenums;

	vboarray_t boneweights;

	void *vbomem;
	void *ebomem;

	unsigned int vbobones;
	const float *bones;
	unsigned  int numbones;

	struct vbo_s *next;
} vbo_t;
void GL_SelectVBO(int vbo);
void GL_SelectEBO(int vbo);
void GL_DeselectVAO(void);

typedef struct texture_s
{
	char		name[128];
	unsigned	vwidth, vheight;	//used for lightmap coord generation

	struct shader_s	*shader;
	char		*partname;				//parsed from the worldspawn entity

	unsigned int	anim_total;				// total tenths in sequence ( 0 = no)
	unsigned int	anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these

	uploadfmt_t srcfmt;
	unsigned int srcwidth, srcheight;	//actual size (updated miptex format)
	qbyte		*srcdata;		//the different mipmap levels.
	qbyte		*palette;	//host_basepal or halflife per-texture palette (or null)
} texture_t;
/*
typedef struct
{
	float coord[3];
	float texcoord[2];
	float lmcoord[2];

	float normals[3];
	float svector[3];
	float tvector[3];
} vbovertex_t;
*/
#define SURF_DRAWSKYBOX		0x00001
#define	SURF_PLANEBACK		0x00002
#define	SURF_DRAWSKY		0x00004
#define SURF_DRAWSPRITE		0x00008
#define SURF_DRAWTURB		0x00010	//water warp effect
#define SURF_DRAWTILED		0x00020	//no need for a sw surface cache... (read: no lightmap)
#define SURF_DRAWBACKGROUND	0x00040
#define SURF_UNDERWATER		0x00080
#define SURF_DONTWARP		0x00100
//#define SURF_BULLETEN		0x00200
#define SURF_NOFLAT			0x08000
#define SURF_DRAWALPHA		0x10000
#define SURF_NODRAW			0x20000	//set on non-vertical halflife water submodel surfaces

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned int	v[2];
} medge_t;

typedef struct mtexinfo_s
{
	vec4_t		vecs[2];
	float		vecscale[2];
	texture_t	*texture;
	int			flags;

	//it's a q2 thing.
	int			numframes;
	struct mtexinfo_s	*next;
} mtexinfo_t;

#define SPECULAR
#ifdef SPECULAR
#define	VERTEXSIZE	10
#else
#define	VERTEXSIZE	7
#endif

typedef struct mfog_s
{
	char			shadername[MAX_QPATH];
	struct shader_s		*shader;

	mplane_t		*visibleplane;

	int				numplanes;
	mplane_t		**planes;
} mfog_t;

typedef struct
{
	vec3_t origin;
	int cubesize;	//pixels
} denvmap_t;
typedef struct
{
	vec3_t origin;
	int cubesize;	//pixels

	texid_t image;
} menvmap_t;

#define LMSHIFT_DEFAULT 4
typedef struct msurface_s
{
	mplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	unsigned short	numedges;	// are backwards edges

	unsigned short		lmshift;	//texels>>lmshift = lightmap samples.
	int			texturemins[2];
	short		extents[2];

	unsigned short	light_s[MAXRLIGHTMAPS], light_t[MAXRLIGHTMAPS];	// gl lightmap coordinates

	image_t		*envmap;
	mfog_t		*fog;
	mesh_t		*mesh;

	batch_t		*sbatch;
	mtexinfo_t	*texinfo;
	int			visframe;		// should be drawn when node is crossed
#ifdef RTLIGHTS
	int			shadowframe;
#endif
//	int			clipcount;

// legacy lighting info
	dlightbitmask_t	dlightbits;
	int			dlightframe;
	qboolean	cached_dlight;				// true if dynamic light in cache

//static lighting
	int			lightmaptexturenums[MAXRLIGHTMAPS];	//rbsp+fbsp formats have multiple lightmaps
	lightstyleindex_t	styles[MAXCPULIGHTMAPS];
	qbyte		vlstyles[MAXRLIGHTMAPS];
	int			cached_light[MAXCPULIGHTMAPS];	// values currently used in lightmap
	int			cached_colour[MAXCPULIGHTMAPS];	// values currently used in lightmap
#ifndef NOSTAINS
	qboolean stained;
#endif
	qbyte		*samples;		// [numstyles*surfsize]
} msurface_t;

/*typedef struct mbrush_s
{
	struct mbrush_s *next;
	unsigned int contents;
	int numplanes;
	struct mbrushplane_s
	{
		vec3_t normal;
		float dist;
	} planes[1];
} mbrush_t;*/

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	int			shadowframe;

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;
//	struct mbrush_s	*brushes;

// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];
#if defined(Q2BSPS) || defined(Q3BSPS) || defined(MAP_PROC)
	int childnum[2];
#endif

	unsigned int		firstsurface;
	unsigned int		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current
	int			shadowframe;

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	qbyte		*compressed_vis;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	qbyte		ambient_sound_level[NUM_AMBIENTS];

#if defined(Q2BSPS) || defined(Q3BSPS)
	int			cluster;
	int			area;
	unsigned int	firstleafbrush;
	unsigned int	numleafbrushes;
#endif
#ifdef Q3BSPS
	unsigned int	firstleafcmesh;
	unsigned int	numleafcmeshes;
	unsigned int	firstleafpatch;
	unsigned int	numleafpatches;
#endif
} mleaf_t;


typedef struct
{
	float				mins[3], maxs[3];
	float				origin[3];
	unsigned int		headnode[MAX_MAP_HULLSM];
	unsigned int		visleafs;		// not including the solid leaf 0
	unsigned int		firstface, numfaces;
	qboolean			hullavailable[MAX_MAP_HULLSM];

	struct q2cbrush_s	*brushes;
	unsigned int		numbrushes;
} mmodel_t;


// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct hull_s
{
	mclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
	int			available;
} hull_t;

void Q1BSP_CheckHullNodes(hull_t *hull);
void Q1BSP_SetModelFuncs(struct model_s *mod);
void Q1BSP_LoadBrushes(struct model_s *model, bspx_header_t *bspx, void *mod_base);
void Q1BSP_Init(void);
void Q1BSP_GenerateShadowMesh(struct model_s *model, struct dlight_s *dl, const qbyte *lightvis, qbyte *litvis, void (*callback)(msurface_t *surf));

void BSPX_LightGridLoad(struct model_s *model, bspx_header_t *bspx, qbyte *mod_base);	//for q1 or q2 models.
void BSPX_LoadEnvmaps(struct model_s *mod, bspx_header_t *bspx, void *mod_base);
void *BSPX_FindLump(bspx_header_t *bspxheader, void *mod_base, char *lumpname, size_t *lumpsize);
bspx_header_t *BSPX_Setup(struct model_s *mod, char *filebase, size_t filelen, lump_t *lumps, size_t numlumps);

typedef struct fragmentdecal_s fragmentdecal_t;
void Fragment_ClipPoly(fragmentdecal_t *dec, int numverts, float *inverts, shader_t *surfshader);
size_t Fragment_ClipPlaneToBrush(vecV_t *points, size_t maxpoints, void *planes, size_t planestride, size_t numplanes, vec4_t face);
void Mod_ClipDecal(struct model_s *mod, vec3_t center, vec3_t normal, vec3_t tangent1, vec3_t tangent2, float size, unsigned int surfflagmask, unsigned int surflagmatch, void (*callback)(void *ctx, vec3_t *fte_restrict points, size_t numpoints, shader_t *shader), void *ctx);

void Q1BSP_MarkLights (dlight_t *light, dlightbitmask_t bit, mnode_t *node);
void GLQ1BSP_LightPointValues(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir);
qboolean Q1BSP_RecursiveHullCheck (hull_t *hull, int num, const vec3_t p1, const vec3_t p2, unsigned int hitcontents, struct trace_s *trace);

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s
{
	float	up, down, left, right;
	qboolean xmirror;
	qboolean lit;
	shader_t *shader;
	image_t *image;
} mspriteframe_t;

mspriteframe_t *R_GetSpriteFrame (entity_t *currententity);


typedef struct
{
	int				numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t	type;
	mspriteframe_t		*frameptr;
} mspriteframedesc_t;

typedef struct
{
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/
#if 0
typedef struct {
	int		s;
	int		t;
} mstvert_t;

typedef struct
{
	int					firstpose;
	int					numposes;
	float				interval;
	dtrivertx_t			bboxmin;
	dtrivertx_t			bboxmax;

	vec3_t		scale;
	vec3_t		scale_origin;

	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct
{
	dtrivertx_t			bboxmin;
	dtrivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct
{
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s {
	int					xyz_index[3];
	int					st_index[3];

	int	pad[2];
} mtriangle_t;


#define	MAX_SKINS	32
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;
	int					numposes;
	int					poseverts;
	int					posedata;	// numposes*poseverts trivert_t

	int					baseposedata; //original verts for triangles to reference
	int					triangles; //we need tri data for shadow volumes

	int					commands;	// gl command list with embedded s/t
	int					gl_texturenum[MAX_SKINS][4];
	int					texels[MAX_SKINS];
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

#define	MAXALIASVERTS	2048
#define ALIAS_Z_CLIP_PLANE	5
#define	MAXALIASFRAMES	256
#define	MAXALIASTRIS	2048
extern	aliashdr_t	*pheader;
extern	mstvert_t	stverts[MAXALIASVERTS*2];
extern	mtriangle_t	triangles[MAXALIASTRIS];
extern	dtrivertx_t	*poseverts[MAXALIASFRAMES];

#endif


/*
========================================================================

.MD2 triangle model file format

========================================================================
*/

// LordHavoc: grabbed this from the Q2 utility source,
// renamed a things to avoid conflicts

#define MD2IDALIASHEADER	"IDP2",4
#define MD2ALIAS_VERSION	8
#define	MD2MAX_SKINNAME		64	//part of the format

/*
#define	MD2MAX_TRIANGLES	4096
#define MD2MAX_FRAMES		512
#define MD2MAX_VERTS		2048
#define MD2MAX_SKINS		32
// sanity checking size
#define MD2MAX_SIZE	(1024*4200)
*/
typedef struct
{
	short	s;
	short	t;
} md2stvert_t;

typedef struct
{
	short	index_xyz[3];
	short	index_st[3];
} md2triangle_t;

typedef struct
{
	qbyte	v[3];			// scaled qbyte to fit in frame mins/maxs
	qbyte	lightnormalindex;
} md2trivertx_t;

/*
#define MD2TRIVERTX_V0   0
#define MD2TRIVERTX_V1   1
#define MD2TRIVERTX_V2   2
#define MD2TRIVERTX_LNI  3
#define MD2TRIVERTX_SIZE 4
*/

typedef struct
{
	float		scale[3];	// multiply qbyte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	md2trivertx_t	verts[1];	// variable sized
} md2frame_t;


// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.


typedef struct
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// qbyte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MAX_SKINNAME string
	int			ofs_st;			// qbyte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;
	int			ofs_end;		// end of file
} md2_t;

//#define ALIASTYPE_MDL 1
//#define ALIASTYPE_MD2 2





//===================================================================


typedef struct
{
	qbyte			ambient[3];
	qbyte			diffuse[3];
	qbyte			direction[2];
} dq3gridlight_t;

typedef struct
{
	unsigned char	ambient[4][3];
	unsigned char	diffuse[4][3];
	unsigned char	styles[4];
	unsigned char	direction[2];
} rbspgridlight_t;

//q3 based
typedef struct {
	int gridBounds[4];	//3 = 0*1
	vec3_t gridMins;
	vec3_t gridSize;
	int numlightgridelems;
//rbsp specific
	rbspgridlight_t *rbspelements;
	unsigned short *rbspindexes;
//non-rbsp specific
	dq3gridlight_t *lightgrid;

	//the reason rbsp is seperate from the non-rbsp is because it allows better memory compression.
	//I chose not to expand at loadtime because q3 would suffer from greater cache misses.
} q3lightgridinfo_t;


//
// Whole model
//

typedef enum {mod_brush, mod_sprite, mod_alias, mod_dummy, mod_halflife, mod_heightmap} modtype_t;
typedef enum {fg_quake, fg_quake2, fg_quake3, fg_halflife, fg_new, fg_doom} fromgame_t;	//useful when we have very similar model types. (eg quake/halflife bsps)
typedef enum {sb_none, sb_quake64, sb_long1, sb_long2} subbsp_t; // used to denote bsp specifics for load processing only (no runtime changes)

#define	MF_ROCKET				(1u<<0)			// leave a trail
#define	MF_GRENADE				(1u<<1)			// leave a trail
#define	MF_GIB					(1u<<2)			// leave a trail
#define	MF_ROTATE				(1u<<3)			// rotate (bonus items)
#define	MF_TRACER				(1u<<4)			// green split trail
#define	MF_ZOMGIB				(1u<<5)			// small blood trail
#define	MF_TRACER2				(1u<<6)			// orange split trail + rotate
#define	MF_TRACER3				(1u<<7)			// purple trail

//hexen2 support.
#define  MFH2_FIREBALL			(1u<<8)			// Yellow transparent trail in all directions
#define  MFH2_ICE				(1u<<9)			// Blue-white transparent trail, with gravity
#define  MFH2_MIP_MAP			(1u<<10)		// This model has mip-maps
#define  MFH2_SPIT				(1u<<11)		// Black transparent trail with negative light
#define  MFH2_TRANSPARENT		(1u<<12)		// Transparent sprite
#define  MFH2_SPELL				(1u<<13)		// Vertical spray of particles
#define  MFH2_HOLEY				(1u<<14)		// Solid model with color 0 cut out
#define  MFH2_SPECIAL_TRANS		(1u<<15)		// Translucency through the particle table
#define  MFH2_FACE_VIEW			(1u<<16)		// Poly Model always faces you
#define  MFH2_VORP_MISSILE		(1u<<17)		// leave a trail at top and bottom of model
#define  MFH2_SET_STAFF			(1u<<18)		// slowly move up and left/right
#define  MFH2_MAGICMISSILE		(1u<<19)		// a trickle of blue/white particles with gravity
#define  MFH2_BONESHARD			(1u<<20)		// a trickle of brown particles with gravity
#define  MFH2_SCARAB			(1u<<21)		// white transparent particles with little gravity
#define  MFH2_ACIDBALL			(1u<<22)		// Green drippy acid shit
#define  MFH2_BLOODSHOT			(1u<<23)		// Blood rain shot trail
#define  MFH2_SPIDERBLOOD		(1u<<31)		// spider blood (remapped from MF_ROCKET, to avoid dlight issues)

typedef union {
	struct {
		int numlinedefs;
		int numsidedefs;
		int numsectors;
	} doom;
} specificmodeltype_t;

typedef struct
{
	int walkno;
	int area[2];
	vec3_t plane;
	float dist;
	vec3_t min;
	vec3_t max;
	int numpoints;
	vec4_t *points;
} portal_t;

struct decoupled_lm_info_s
{
	quint16_t lmsize[2];	//made explicit. beware MAX_
    quint32_t lmoffset;		//replacement offset for vanilla compat.
    vec4_t lmvecs[2]; //lmcoord[] = dotproduct3(vertexcoord, lmvecs[])+lmvecs[][3]
};
struct facelmvecs_s
{
	vec4_t lmvecs[2];		//lmcoord[] = dotproduct3(vertexcoord, lmvecs[])+lmvecs[][3]
	float lmvecscale[2];	//just 1/Length(lmvecs). dlights work in luxels, but need to be able to work back to qu.
};

struct surfedgenormals_s {
	quint32_t n;
	quint32_t s;
	quint32_t t;
};

enum
{
	MLS_NOTLOADED,
	MLS_LOADING,
	MLS_LOADED,
	MLS_FAILED
};
typedef struct model_s
{
	char		name[MAX_QPATH];	//actual name on disk
	char		publicname[MAX_QPATH];	//name that the gamecode etc sees
	int			datasequence;	//if it gets old enough, we can purge it.
	int			engineflags;	//
	int			loadstate;		//MLS_
	qboolean	tainted;		// differs from the server's version. this model will be invisible as a result, to avoid spiked models.
	qboolean	pushdepth;		// bsp submodels have this flag set so you don't get z fighting on co-planar surfaces.
	time_t		mtime;			// modification time. so we can flush models if they're changed on disk. or at least worldmodels.

	struct model_s *submodelof;	// shares memory allocations with this model.

	modtype_t	type;
	fromgame_t	fromgame;

	int			numframes;
	synctype_t	synctype;

	int			flags;
	int			particleeffect;
	int			particletrail;
	int			traildefaultindex;

//
// volume occupied by the model graphics
//
	vec3_t		mins, maxs;
	float		radius;
	float		clampscale;
	float		maxlod;

//
// solid volume for clipping
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

	void		*cnodes;	//BIH tree
//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	mplane_t	*planes;

	size_t		pvsbytes;	//total bytes for the per-leaf pvs/phs data. rounded up to sizeof(int).
	int			numclusters;	//number of bits in the pvs data.
	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;
	vec3_t		*normals;
	struct surfedgenormals_s	*surfedgenormals; //for per-vertex normals
	struct facelmvecs_s			*facelmvecs;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	mnode_t		*nodes;
	mnode_t		*rootnode;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	mclipnode_t	*clipnodes;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	hull_t		hulls[MAX_MAP_HULLSM];

	int			numtextures;
	texture_t	**textures;

	qbyte		*pvs, *phs;			// fully expanded and decompressed
	qbyte		*visdata;
	void	*vis;
	qbyte		*lightdata;
	qbyte		*deluxdata;
	unsigned	lightdatasize;
	q3lightgridinfo_t *lightgrid;
	mfog_t		*fogs;
	int			numfogs;
	menvmap_t	*envmaps;
	unsigned	numenvmaps;

	struct skytris_s		*skytris;	//for surface emittance
	float					skytime;	//for surface emittance
	struct skytriblock_s	*skytrimem;

	struct {unsigned int id; char *keyvals;} *entityinfo;
	size_t		numentityinfo;
	const char	*entities_raw;
	int			entitiescrc;

	struct doll_s		*dollinfo;	//ragdoll info
	int camerabone;	//the 1-based bone index that the camera should be attached to (for gltf rather than anything else)

	shader_t	*simpleskin[4];		//simpleitems cache

	struct {
		texture_t *tex;
		vbo_t *vbo;
	} *shadowbatches;
	int numshadowbatches;
	vbo_t *vbos;
	void *terrain;
	batch_t *batches[SHADER_SORT_COUNT];
	unsigned int numbatches;
	struct
	{
		int first;				//once built...
		int count;				//num lightmaps
		int	mergew;				//merge this many source lightmaps together. woo.
		int	mergeh;				//merge this many source lightmaps together. woo.
		int width;				//x size of lightmaps
		int height;				//y size of lightmaps
		int surfstyles;			//numbers of style per surface.
		int maxstyle;			//highest (valid) style used (cl_max_lightstyles must be 1+ higher).
		enum {
			//vanilla used byte values, with 255 being a value of about 2.
			//float/hdr formats use 1 to mean 1, however.
			//internally, we still use integers for lighting, with .7 bits of extra precision.
			LM_L8,
			LM_RGB8,
			LM_E5BGR9,
		} fmt;
		enum uploadfmt prebaked;
		qboolean deluxemapping;	//lightmaps are interleaved with deluxemap data (lightmap indicies should only be even values)
		qboolean deluxemapping_modelspace; //deluxemaps are in modelspace - we need different glsl.
	} lightmaps;

	unsigned	checksum;	//the legacy checksum (excludes ent lump)
	unsigned	checksum2;	//the watervis checksum (excludes ent lump and vis stuff). this is the one that's sent over the network.

	portal_t *portal;
	unsigned int numportals;

	modelfuncs_t	funcs;
//
// additional model data
//
	void *meshinfo;	//data allocated within the memgroup allocations, will be nulled out when the model is flushed
	searchpathfuncs_t *archive;	//some bsp formats have an embedded zip...
	zonegroup_t memgroup;
} model_t;

#define MDLF_EMITREPLACE     0x0001 // particle effect engulphs model (don't draw)
#define MDLF_EMITFORWARDS    0x0002
#define MDLF_NODEFAULTTRAIL  0x0004
//#define MDLF_RGBLIGHTING     0x0008
#define MDLF_PLAYER          0x0010 // players have specific lighting values
#define MDLF_FLAME           0x0020 // can be excluded with r_drawflame, fullbright render hack
#define MDLF_DOCRC           0x0040 // model needs CRC built
#define MDLF_NEEDOVERBRIGHT  0x0080 // only overbright these models with gl_overbright_all set
#define MDLF_NOSHADOWS       0x0100 // doesn't produce shadows for one reason or another
#define	MDLF_NOTREPLACEMENTS 0x0200 // can be considered a cheat, disable texture replacements
#define MDLF_EZQUAKEFBCHEAT  0x0400 // this is a blatent cheat, one that can disadvantage us fairly significantly if we don't support it.
#define MDLF_NOLERP		     0x0800 // doesn't lerp, ever. for dodgy models that don't scale to nothingness before jumping.
#define MDLF_RECALCULATERAIN 0x1000 // particles changed, recalculate any sky polys

//============================================================================
#endif	// __MODEL__



typedef struct
{
	unsigned int *offsets;
	unsigned short *extents;
	unsigned char *styles8;
	unsigned short *styles16;
	unsigned int stylesperface;
	unsigned char *shifts;
	unsigned char defaultshift;
} lightmapoverrides_t;
void Mod_LoadLighting (struct model_s *loadmodel, bspx_header_t *bspx, qbyte *mod_base, lump_t *l, qboolean interleaveddeluxe, lightmapoverrides_t *overrides, subbsp_t subbsp);

float RadiusFromBounds (const vec3_t mins, const vec3_t maxs);


//
// gl_heightmap.c
//
#ifdef TERRAIN
void Terr_Init(void);
struct plugterrainfuncs_s;
struct plugterrainfuncs_s *Terr_GetTerrainFuncs(size_t structsize);
void Terr_DrawTerrainModel (batch_t **batch, entity_t *e);
void Terr_FreeModel(model_t *mod);
void Terr_FinishTerrain(model_t *model);
void Terr_PurgeTerrainModel(model_t *hm, qboolean lightmapsonly, qboolean lightmapreusable);
void *Mod_LoadTerrainInfo(model_t *mod, char *loadname, qboolean force);	//call this after loading a bsp
qboolean Terrain_LocateSection(const char *name, flocation_t *loc);	//used on servers to generate sections for download.
qboolean Heightmap_Trace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contentmask, struct trace_s *trace);
unsigned int Heightmap_PointContents(model_t *model, const vec3_t axis[3], const vec3_t org);
struct fragmentdecal_s;
void Terrain_ClipDecal(struct fragmentdecal_s *dec, float *center, float radius, model_t *model);
qboolean Terr_DownloadedSection(char *fname);
shader_t *Terr_GetShader(struct model_s *mod, struct trace_s *trace);

void CL_Parse_BrushEdit(void);
qboolean SV_Parse_BrushEdit(void);
qboolean SV_Prespawn_Brushes(sizebuf_t *msg, unsigned int *modelindex, unsigned int *lastid);
#endif





qboolean Heightmap_Edit(model_t *mod, int action, float *pos, float radius, float quant);


#if defined(Q2BSPS) || defined(Q3BSPS)
void CM_Init(void);
struct model_s *CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
#endif
#if 0

#ifdef __cplusplus
//#pragma warningmsg ("                  c++ stinks")
#else

qboolean	CM_SetAreaPortalState (struct model_s *mod, int portalnum, qboolean open);
qboolean	CM_HeadnodeVisible (struct model_s *mod, int nodenum, const qbyte *visbits);
qboolean	VARGS CM_AreasConnected (struct model_s *mod, unsigned int area1, unsigned int area2);
int		CM_ClusterBytes (struct model_s *mod);
int		CM_LeafContents (struct model_s *mod, int leafnum);
int		CM_LeafCluster (struct model_s *mod, int leafnum);
int		CM_LeafArea (struct model_s *mod, int leafnum);
int		CM_WriteAreaBits (struct model_s *mod, qbyte *buffer, int area, qboolean merge);
int		CM_PointLeafnum (struct model_s *mod, const vec3_t p);
qbyte	*CM_ClusterPVS (struct model_s *mod, int cluster, pvsbuffer_t *buffer, pvsmerge_t merge);
qbyte	*CM_ClusterPHS (struct model_s *mod, int cluster, pvsbuffer_t *buffer);
int		CM_BoxLeafnums (struct model_s *mod, const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *topnode);
int		CM_PointContents (struct model_s *mod, const vec3_t p);
int		CM_TransformedPointContents (struct model_s *mod, const vec3_t p, int headnode, const vec3_t origin, const vec3_t angles);
int		CM_HeadnodeForBox (struct model_s *mod, const vec3_t mins, const vec3_t maxs);
//struct trace_s	CM_TransformedBoxTrace (struct model_s *mod, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int brushmask, vec3_t origin, vec3_t angles);

//for gamecode to control portals/areas
void	CMQ2_SetAreaPortalState (model_t *mod, unsigned int portalnum, qboolean open);
void	CMQ3_SetAreaPortalState (model_t *mod, unsigned int area1, unsigned int area2, qboolean open);

//for saved games to write the raw state.
size_t	CM_WritePortalState (model_t *mod, void **data);
qofs_t	CM_ReadPortalState (model_t *mod, qbyte *ptr, qofs_t ptrsize);

#endif



#endif	//Q2BSPS

void CategorizePlane ( mplane_t *plane );
void CalcSurfaceExtents (model_t *mod, msurface_t *s);
