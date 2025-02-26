#ifdef TERRAIN

//#define STRICTEDGES	//strict (ugly) grid
#define TERRAINTHICKNESS 16
#define TERRAINACTIVESECTIONS 3000

/*
a note on networking:
By default terrain is NOT networked. This means content is loaded without networking delays.
If you wish to edit the terrain collaboratively, you can enable the mod_terrain_networked cvar.
When set, changes on the server will notify clients that a section has changed, and the client will reload it as needed.
Changes on the client WILL NOT notify the server, and will get clobbered if the change is also made on the server.
This means for editing purposes, you MUST funnel it via ssqc with your own permission checks.
It also means for explosions and local stuff, the server will merely restate changes from impacts if you do them early. BUT DO NOT CALL THE EDIT FUNCTION IF THE SERVER HAS ALREADY APPLIED THE CHANGE.
*/
/*
terminology:
tile:
	a single grid tile of 2*2 height samples.
	iterrated for collisions but otherwise unused.
section:
	16*16 tiles, with a single texture spread over them.
	samples have an overlap with the neighbouring section (so 17*17 height samples). texture samples do not quite match height frequency (63*63 vs 16*16).
	smallest unit for culling.
block:
	16*16 sections. forms a single disk file. used only to avoid 16777216 files in a single directory, instead getting 65536 files for a single fully populated map... much smaller...
	each block file is about 4mb each. larger can be detrimental to automatic downloads.
cluster:
	64*64 sections
	internal concept to avoid a single pointer array of 16 million entries per terrain.
*/

int Surf_NewLightmaps(int count, int width, int height, uploadfmt_t fmt, qboolean deluxe);

#define MAXCLUSTERS 64
#define MAXSECTIONS 64	//this many sections within each cluster in each direction
#define SECTHEIGHTSIZE 17 //this many height samples per section (one for overlap)
#define SECTTEXSIZE 64	//this many texture samples per section (one for overlap, yes, this is a little awkward)
#define SECTIONSPERBLOCK 16

//each section is this many sections higher in world space, to keep the middle centered at '0 0'
#define CHUNKBIAS	(MAXCLUSTERS*MAXSECTIONS/2)
#define CHUNKLIMIT	(MAXCLUSTERS*MAXSECTIONS)

#define LMCHUNKS 8//(LMBLOCK_WIDTH/SECTTEXSIZE)	//FIXME: make dynamic.
#define HMLMSTRIDE (LMCHUNKS*SECTTEXSIZE)

#define SECTION_MAGIC (*(int*)"HMMS")
#define SECTION_VER_DEFAULT	1
/*simple version history:
ver=0
	SECTHEIGHTSIZE=16
ver=1
	SECTHEIGHTSIZE=17 (oops, makes holes more usable)
	(holes in this format are no longer supported)
ver=2
	uses deltas instead of absolute values
	variable length image names
*/

#define TGS_NOLOAD			0
#define TGS_LAZYLOAD		1	//see if its available, if not, queue it. don't create too much work at once. peace man
#define TGS_TRYLOAD			2	//try and get it, but don't stress if its not available yet
#define TGS_WAITLOAD		4	//load it, wait for it if needed.
#define TGS_ANYSTATE		8	//returns the section regardless of its current state, even if its loading.
#define TGS_NODOWNLOAD		16	//don't queue it for download
#define TGS_NORENDER		32	//don't upload any textures or whatever
#define TGS_DEFAULTONFAIL	64	//if it failed to load, generate a default anyway

enum
{
	//these flags can be found on disk
	TSF_HASWATER_V0	= 1u<<0,	//no longer flagged.
	TSF_HASCOLOURS	= 1u<<1,
	TSF_HASHEIGHTS	= 1u<<2,
	TSF_HASSHADOW	= 1u<<3,

	//these flags are found only on disk
	TSF_D_UNUSED1	= 1u<<28,
	TSF_D_UNUSED2	= 1u<<29,
	TSF_D_UNUSED3	= 1u<<30,
	TSF_D_UNUSED4	= 1u<<31,

	//these flags should not be found on disk
	TSF_NOTIFY		= 1u<<28,	//modified on server, waiting for clients to be told about the change.
	TSF_RELIGHT		= 1u<<29,	//height edited, needs relighting.
	TSF_DIRTY		= 1u<<30,	//its heightmap has changed, the mesh needs rebuilding
	TSF_EDITED		= 1u<<31	//says it needs to be written if saved

#define TSF_INTERNAL	(TSF_RELIGHT|TSF_DIRTY|TSF_EDITED|TSF_NOTIFY)
};
enum
{
	TMF_SCALE	= 1u<<0,
	//what else do we want? alpha? colormod perhaps?
};

typedef struct
{
	int size;
	vec3_t axisorg[4];
	float scale;
	int reserved3;
	int reserved2;
	int reserved1;
	//char modelname[1+];
} dsmesh_v1_t;

typedef struct
{
	unsigned int flags;
	char texname[4][32];
	unsigned int texmap[SECTTEXSIZE][SECTTEXSIZE];
	float heights[SECTHEIGHTSIZE*SECTHEIGHTSIZE];
	unsigned short holes;
	unsigned short reserved0;
	float waterheight;
	float minh;
	float maxh;
	int ents_num;
	int reserved1;
	int reserved4;
	int reserved3;
	int reserved2;
} dsection_v1_t;

//file header for a single section
typedef struct
{
	int magic;
	int ver;
} dsection_t;

//file header for a block of sections.
//(because 16777216 files in a single directory is a bad plan. windows really doesn't like it.)
typedef struct
{
	//a block is a X*Y group of sections
	//if offset==0, the section isn't present.
	//the data length of the section preceeds the actual data.
	int magic;
	int ver;
	unsigned int offset[SECTIONSPERBLOCK*SECTIONSPERBLOCK];
} dblock_t;

typedef struct hmpolyset_s
{
	struct hmpolyset_s *next;
	shader_t *shader;
	mesh_t mesh;
	mesh_t *amesh;
	vbo_t vbo;
} hmpolyset_t;
struct hmwater_s
{
	struct hmwater_s *next;
	unsigned int contentmask;
	qboolean simple;	//no holes, one height
	float minheight;
	float maxheight;
	char shadername[MAX_QPATH];
	shader_t *shader;
	qbyte holes[8];
	float heights[9*9];

/*
	qboolean facesdown;
	unsigned int contentmask;
	float		heights[SECTHEIGHTSIZE*SECTHEIGHTSIZE];
#ifndef SERVERONLY
	byte_vec4_t	colours[SECTHEIGHTSIZE*SECTHEIGHTSIZE];
	char texname[4][MAX_QPATH];
	int lightmap;
	int lmx, lmy;

	texnums_t textures;
	vbo_t vbo;
	mesh_t mesh;
	mesh_t *amesh;
#endif
*/
};
enum
{
	TSLS_NOTLOADED,
	TSLS_LOADING0,	//posted to a worker, but not picked up yet. this allows a single worker to generate multiple sections without fighting.
	TSLS_LOADING1,	//section is queued to the worker (and may be loaded as part of another section)
	TSLS_LOADING2,	//waiting for main thread to finish, worker will ignore
	TSLS_LOADED,
	TSLS_FAILED
};
typedef struct
{
	link_t recycle;
	int sx, sy;

	int loadstate;
	float timestamp;	//don't recycle it if its still fairly recent.

	float heights[SECTHEIGHTSIZE*SECTHEIGHTSIZE];
	unsigned char holes[8];
	unsigned int flags;
	float maxh_cull;	//includes water+mesh heights
	float minh, maxh;
	struct heightmap_s *hmmod;

	//FIXME: make layers, each with their own holes+heights+contents+textures+shader+mixes. water will presumably have specific values set for each part.
	struct hmwater_s *water;

	size_t traceseq;

#ifndef SERVERONLY
	pvscache_t pvscache;
	vec4_t colours[SECTHEIGHTSIZE*SECTHEIGHTSIZE];	//FIXME: make bytes
	char texname[4][MAX_QPATH];	//fixme: make path length dynamic.
	int lightmap;
	int lmx, lmy;

	texnums_t textures;
	vbo_t vbo;
	mesh_t mesh;
	mesh_t *amesh;

	hmpolyset_t *polys;
#endif
	int numents;
	int maxents;
	struct hmentity_s **ents;
} hmsection_t;
typedef struct
{
	hmsection_t *section[MAXSECTIONS*MAXSECTIONS];
} hmcluster_t;

#ifndef SERVERONLY
typedef struct brushbatch_s
{
	vbo_t vbo;
	mesh_t mesh;
	mesh_t *pmesh;
	int lightmap;
	struct brushbatch_s *next;
	avec4_t align;	//meh, cos we can.
} brushbatch_t;
#endif
typedef struct brushtex_s
{
	char shadername[MAX_QPATH];
#ifndef SERVERONLY
	shader_t *shader;

	//for rebuild performance
	int firstlm;
	int lmcount;

	struct brushbatch_s *batches;
#endif
	qboolean rebuild;
	struct brushtex_s *next;
} brushtex_t;

typedef struct patchtessvert_s
{
	vec3_t v;
	vec4_t rgba;
	vec2_t tc;
//	vec3_t norm;
//	vec3_t sdir;
//	vec3_t tdir;
} patchtessvert_t;
typedef struct qcpatchvert_s
{
	vec3_t v;
	vec4_t rgba;
	vec2_t tc;
} qcpatchvert_t;
patchtessvert_t *PatchInfo_Evaluate(const qcpatchvert_t *cp, const unsigned short patch_cp[2], const short subdiv[2], unsigned short *size);
unsigned int PatchInfo_EvaluateIndexes(const unsigned short *size, index_t *out_indexes);

typedef struct
{
	unsigned int	contents;	//bitmask
	unsigned int	id;			//networked/gamecode id.
	unsigned int	axialplanes;	//+x,+y,+z,-x,-y,-z. used for bevel stuff.
	unsigned int	numplanes;
	unsigned char	selected:1;	//different shader stuff
	unsigned char	ispatch:1;	//just for parsing really
	vec4_t			*planes;
	vec3_t			mins, maxs;	//for optimisation and stuff
	struct patchdata_s
	{	//unlit, always...
		brushtex_t *tex;
		unsigned short numcp[2];
		short subdiv[2];	//<0=regular q3 patch, 0=cp-only, >0=fixed-tessellation.

		unsigned short tesssize[2];
		patchtessvert_t *tessvert; //x+(y*tesssize[0])

		//control points
		qcpatchvert_t cp[1]; //x+(y*numcp[0]) extends past end of patchdata_s
	} *patch;	//if this is NULL, then its a regular brush. otherwise its a patch.
	struct brushface_s
	{
		brushtex_t *tex;
		vec4_t stdir[2];
		vec3_t *points;
		unsigned short numpoints;
		unsigned short lmscale;
		int	lightmap;
		unsigned short lmbase[2];	//min st coord of the lightmap atlas, in texels.
		unsigned short relight:1;
		unsigned short relit:1;
		int lmbias[2];
		unsigned short lmextents[2];
		unsigned int surfaceflags;	//used by q2
		unsigned int surfacevalue;	//used by q2 (generally light levels)
		qbyte *lightdata;
	} *faces;
} brushes_t;

typedef struct
{
	string_t	shadername;
	vec3_t		planenormal;
	float		planedist;
	vec3_t		sdir;
	float		sbias;
	vec3_t		tdir;
	float		tbias;
} qcbrushface_t;
typedef struct
{
	string_t	shadername;
	unsigned int contents;
	unsigned int cp_width;
	unsigned int cp_height;
	unsigned int subdiv_x;
	unsigned int subdiv_y;
	vec3_t		texinfo;
} qcpatchinfo_t;

typedef struct heightmap_s
{
	char path[MAX_QPATH];
	char skyname[MAX_QPATH];			//name of the skybox
	char groundshadername[MAX_QPATH];	//this is the shader we're using to draw the terrain itself. you could use other shaders here, for eg debugging or stylised weirdness.
	unsigned int culldistance;			//entities will be culled if they're this far away (squared distance
	float	maxdrawdist;				//maximum view distance. extends view if larger than fog implies.

	unsigned char *seed;				//used by whatever terrain generator.
	qboolean forcedefault;				//sections that cannot be loaded/generated will receive default values for stuff.
	char defaultgroundtexture[MAX_QPATH];//texture used for defaulted sections
	char defaultwatershader[MAX_QPATH];	//shader used for defaulted sections that have heights beneath defaultwaterheight.
	float defaultwaterheight;			//water height. if you want your islands to be surrounded by water.
	float defaultgroundheight;			//defaulted sections will have a z plane this high

	int firstsegx, firstsegy; //min bounds of the terrain, in sections
	int maxsegx, maxsegy; //max bounds of the terrain, in sections
	float sectionsize;	//each section is this big, in world coords
	hmcluster_t *cluster[MAXCLUSTERS*MAXCLUSTERS];
	shader_t *skyshader;
	shader_t *shader;
	mesh_t skymesh;
	mesh_t *askymesh;
	qboolean	legacyterrain;	//forced exterior=SOLID
	unsigned int exteriorcontents;	//contents type outside of the terrain sections area (.map should be empty, while terrain will usually block).
	unsigned int loadingsections;	//number of sections currently being loaded. avoid loading extras while non-zero.
	size_t traceseq;
	size_t drawnframe;
	enum
	{
		DGT_SOLID,	//invalid/new areas should be completely solid until painted.
		DGT_HOLES,	//invalid/new sections should be non-solid+invisible
		DGT_FLAT	//invalid/new sections should be filled with ground by default
	} defaultgroundtype;
	enum
	{
		HMM_TERRAIN,
		HMM_BLOCKS
	} mode;
	int tilecount[2];
	int tilepixcount[2];

	int activesections;
	link_t recycle;		//section list in lru order
//	link_t collected;	//memory that may be reused, to avoid excess reallocs.

	struct hmentity_s
	{
		size_t drawnframe;	//don't add it to the scene multiple times.
		size_t traceseq;	//don't trace through this entity multiple times if its in different sections.
		int refs;		//entity is free/reusable when its no longer referenced by any sections
		entity_t ent;	//note: only model+modelmatrix info is relevant. fixme: implement instancing.

		struct hmentity_s *next;	//used for freeing/allocating an entity
	} *entities;
	void *entitylock;	//lock this if you're going to read/write entities of any kind.

#ifndef SERVERONLY
	unsigned int numusedlmsects;	//to track leaks and stats
	unsigned int numunusedlmsects;
	struct lmsect_s
	{
		struct lmsect_s *next;
		int lm, x, y;
	} *unusedlmsects;
#endif

#ifndef SERVERONLY
	//I'm putting this here because we might have some quite expensive lighting routines going on
	//and that'll make editing the terrain jerky as fook, so relighting it a few texels at a time will help maintain a framerate while editing
	hmsection_t *relight;
	unsigned int relightidx;
	vec2_t relightmin;
#endif


	
	char *texmask;	//for editing - visually masks off the areas which CANNOT accept this texture
	qboolean entsdirty;	//ents were edited, so we need to reload all lighting...
	struct relight_ctx_s *relightcontext;
	struct llightinfo_s *lightthreadmem;
	qboolean inheritedlightthreadmem;
	qboolean recalculatebrushlighting;
	lmalloc_t brushlmalloc;
	float brushlmscale;
	unsigned int *brushlmremaps;
	unsigned int brushmaxlms;
	brushtex_t *brushtextures;
	brushes_t *wbrushes;
	unsigned int numbrushes;
	unsigned int brushidseq;
	qboolean brushesedited;
} heightmap_t;

typedef struct plugterrainfuncs_s
{
	void			*(QDECL *GenerateWater)			(hmsection_t *s, float maxheight);
	qboolean		 (QDECL *InitLightmap)			(hmsection_t *s, qboolean initialise);
	unsigned char	*(QDECL *GetLightmap)			(hmsection_t *s, int idx, qboolean edit);
	void			 (QDECL *AddMesh)				(heightmap_t *hm, int loadflags, model_t *mod, const char *modelname, vec3_t epos, vec3_t axis[3], float scale);
	hmsection_t		*(QDECL *GetSection)			(heightmap_t *hm, int x, int y, unsigned int flags);
	int				 (QDECL *GenerateSections)		(heightmap_t *hm, int sx, int sy, int count, hmsection_t **sects);
	void			 (QDECL *FinishedSection)		(hmsection_t *s, qboolean success);

	qboolean		 (QDECL *AutogenerateSection)(heightmap_t *hm, int sx, int sy, unsigned int tgsflags);	//replace this if you want to make a terrain generator.
#define plugterrainfuncs_name "Terrain"
} plugterrainfuncs_t;
#endif
