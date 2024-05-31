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

#ifndef MODEL_SHARED_H
#define MODEL_SHARED_H

#include <stddef.h>
#include "qdefs.h"
#include "bspfile.h"
#include "r_qshader.h"
#include "matrixlib.h"
struct rtexture_s;
struct mempool_s;
struct skeleton_s;
struct skinframe_s;

typedef enum synctype_e {ST_SYNC=0, ST_RAND } synctype_t;

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

typedef enum modtype_e {mod_invalid, mod_brushq1, mod_sprite, mod_alias, mod_brushq2, mod_brushq3, mod_brushhl2, mod_obj, mod_null} modtype_t;

typedef struct animscene_s
{
	char name[32]; // for viewthing support
	int firstframe;
	int framecount;
	int loop; // true or false
	float framerate;
}
animscene_t;

struct md3vertex_s;
struct trivertx_s;
typedef struct texvecvertex_s
{
	signed char svec[3];
	signed char tvec[3];
}
texvecvertex_t;

typedef struct blendweights_s
{
	unsigned char index[4];
	unsigned char influence[4];
}
blendweights_t;

typedef struct r_meshbuffer_s
{
	int bufferobject; // OpenGL
	void *devicebuffer; // Direct3D
	size_t size;
	qbool isindexbuffer;
	qbool isuniformbuffer;
	qbool isdynamic;
	qbool isindex16;
	char name[MAX_QPATH];
}
r_meshbuffer_t;

// used for mesh lists in q1bsp/q3bsp map models
// (the surfaces reference portions of these meshes)
typedef struct surfmesh_s
{
	// triangle data in system memory
	int num_triangles; // number of triangles in the mesh
	int *data_element3i; // int[tris*3] triangles of the mesh, 3 indices into vertex arrays for each

	// vertex data in system memory
	int num_vertices; // number of vertices in the mesh
	float *data_vertex3f; // float[verts*3] vertex locations
	float *data_svector3f; // float[verts*3] direction of 'S' (right) texture axis for each vertex
	float *data_tvector3f; // float[verts*3] direction of 'T' (down) texture axis for each vertex
	float *data_normal3f; // float[verts*3] direction of 'R' (out) texture axis for each vertex
	float *data_texcoordtexture2f; // float[verts*2] texcoords for surface texture
	float *data_texcoordlightmap2f; // float[verts*2] texcoords for lightmap texture
	float *data_lightmapcolor4f;
	unsigned char *data_skeletalindex4ub;
	unsigned char *data_skeletalweight4ub;
	int *data_lightmapoffsets; // index into surface's lightmap samples for vertex lighting
	// index buffer - only one of these will be non-NULL
	r_meshbuffer_t *data_element3i_indexbuffer;
	int data_element3i_bufferoffset;
	unsigned short *data_element3s; // unsigned short[tris*3] triangles of the mesh in unsigned short format (NULL if num_vertices > 65536)
	r_meshbuffer_t *data_element3s_indexbuffer;
	int data_element3s_bufferoffset;
	// vertex buffers
	r_meshbuffer_t *data_vertex3f_vertexbuffer;
	int data_vertex3f_bufferoffset;
	r_meshbuffer_t *data_svector3f_vertexbuffer;
	int data_svector3f_bufferoffset;
	r_meshbuffer_t *data_tvector3f_vertexbuffer;
	int data_tvector3f_bufferoffset;
	r_meshbuffer_t *data_normal3f_vertexbuffer;
	int data_normal3f_bufferoffset;
	r_meshbuffer_t *data_texcoordtexture2f_vertexbuffer;
	int data_texcoordtexture2f_bufferoffset;
	r_meshbuffer_t *data_texcoordlightmap2f_vertexbuffer;
	int data_texcoordlightmap2f_bufferoffset;
	r_meshbuffer_t *data_lightmapcolor4f_vertexbuffer;
	int data_lightmapcolor4f_bufferoffset;
	r_meshbuffer_t *data_skeletalindex4ub_vertexbuffer;
	int data_skeletalindex4ub_bufferoffset;
	r_meshbuffer_t *data_skeletalweight4ub_vertexbuffer;
	int data_skeletalweight4ub_bufferoffset;
	// morph blending, these are zero if model is skeletal or static
	int num_morphframes;
	struct md3vertex_s *data_morphmd3vertex;
	struct trivertx_s *data_morphmdlvertex;
	struct texvecvertex_s *data_morphtexvecvertex;
	float *data_morphmd2framesize6f;
	float num_morphmdlframescale[3];
	float num_morphmdlframetranslate[3];
	// skeletal blending, these are NULL if model is morph or static
	struct blendweights_s *data_blendweights;
	int num_blends;
	unsigned short *blends;
	// set if there is some kind of animation on this model
	qbool isanimated;

	// dynamic mesh building support (Mod_Mesh_*)
	int num_vertexhashsize; // always pow2 for simple masking
	int *data_vertexhash; // hash table - wrapping buffer for storing index of similar vertex with -1 as terminator
	int max_vertices; // preallocated size of data_vertex3f and friends (always >= num_vertices)
	int max_triangles; // preallocated size of data_element3i
}
surfmesh_t;

#define SHADOWMESHVERTEXHASH 1024
typedef struct shadowmeshvertexhash_s
{
	struct shadowmeshvertexhash_s *next;
}
shadowmeshvertexhash_t;

typedef struct shadowmesh_s
{
	struct mempool_s *mempool;

	int numverts;
	int maxverts;
	float *vertex3f;
	r_meshbuffer_t *vbo_vertexbuffer;
	int vbooffset_vertex3f;

	int numtriangles;
	int maxtriangles;
	int *element3i;
	r_meshbuffer_t *element3i_indexbuffer;
	int element3i_bufferoffset;
	unsigned short *element3s;
	r_meshbuffer_t *element3s_indexbuffer;
	int element3s_bufferoffset;

	// used for shadow mapping cubemap side partitioning
	int sideoffsets[6], sidetotals[6];

	// these are NULL after Mod_ShadowMesh_Finish is performed, only used
	// while building meshes
	shadowmeshvertexhash_t **vertexhashtable, *vertexhashentries;
}
shadowmesh_t;

typedef struct texture_s
{
	// name
	char name[64];

	// q1bsp
	// size
	unsigned int width, height;
	// SURF_ flags
	//unsigned int flags;

	// base material flags
	int basematerialflags;
	// current material flags (updated each bmodel render)
	int currentmaterialflags;
	// base material alpha (used for Q2 materials)
	float basealpha;

	// PolygonOffset values for rendering this material
	// (these are added to the r_refdef values and submodel values)
	float biaspolygonfactor;
	float biaspolygonoffset;

	// textures to use when rendering this material (derived from materialshaderpass)
	struct skinframe_s *currentskinframe;
	// textures to use for terrain texture blending (derived from backgroundshaderpass)
	struct skinframe_s *backgroundcurrentskinframe;

	// total frames in sequence and alternate sequence
	int anim_total[2];
	// direct pointers to each of the frames in the sequences
	// (indexed as [alternate][frame])
	struct texture_s *anim_frames[2][10];
	// 1 = q1bsp animation with anim_total[0] >= 2 (animated) or anim_total[1] >= 1 (alternate frame set)
	// 2 = q2bsp animation with anim_total[0] >= 2 (uses self.frame)
	int animated;

	// renderer checks if this texture needs updating...
	int update_lastrenderframe;
	void *update_lastrenderentity;
	// the current alpha of this texture (may be affected by r_wateralpha, also basealpha, and ent->alpha)
	float currentalpha;
	// current value of blendfunc - one of:
	// {GL_SRC_ALPHA, GL_ONE}
	// {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA}
	// {customblendfunc[0], customblendfunc[1]}
	// {GL_ONE, GL_ZERO}
	int currentblendfunc[2];
	// the current texture frame in animation
	struct texture_s *currentframe;
	// current texture transform matrix (used for water scrolling)
	matrix4x4_t currenttexmatrix;
	matrix4x4_t currentbackgroundtexmatrix;

	// various q3 shader features
	q3shaderinfo_deform_t deforms[Q3MAXDEFORMS];
	texture_shaderpass_t *shaderpasses[Q3SHADER_MAXLAYERS]; // all shader passes in one array
	texture_shaderpass_t *materialshaderpass; // equal to one of shaderpasses[] or NULL
	texture_shaderpass_t *backgroundshaderpass; // equal to one of shaderpasses[] or NULL
	unsigned char startpreshaderpass; // range within shaderpasses[]
	unsigned char endpreshaderpass; // number of preshaderpasses
	unsigned char startpostshaderpass; // range within shaderpasses[]
	unsigned char endpostshaderpass; // number of postshaderpasses

	qbool colormapping;
	struct rtexture_s *basetexture; // original texture without pants/shirt/glow
	struct rtexture_s *pantstexture; // pants only (in greyscale)
	struct rtexture_s *shirttexture; // shirt only (in greyscale)
	struct rtexture_s *nmaptexture; // normalmap (bumpmap for dot3)
	struct rtexture_s *glosstexture; // glossmap (for dot3)
	struct rtexture_s *glowtexture; // glow only (fullbrights)
	struct rtexture_s *fogtexture; // alpha of the base texture (if not opaque)
	struct rtexture_s *reflectmasktexture; // mask for fake reflections
	struct rtexture_s *reflectcubetexture; // fake reflections cubemap
	struct rtexture_s *backgroundbasetexture; // original texture without pants/shirt/glow
	struct rtexture_s *backgroundnmaptexture; // normalmap (bumpmap for dot3)
	struct rtexture_s *backgroundglosstexture; // glossmap (for dot3)
	struct rtexture_s *backgroundglowtexture; // glow only (fullbrights)
	float specularpower;

	// rendering parameters - updated by R_GetCurrentTexture using rsurface.render_* fields
	// (almost) all map textures are lightmap (no MATERIALFLAG_MODELLIGHT set),
	// (almost) all model textures are MATERIALFLAG_MODELLIGHT,
	// MATERIALFLAG_FULLBRIGHT is rendered as a forced MATERIALFLAG_MODELLIGHT with rtlights disabled
	float render_glowmod[3];
	// MATERIALFLAG_MODELLIGHT uses these parameters
	float render_modellight_ambient[3];
	float render_modellight_diffuse[3];
	float render_modellight_lightdir_world[3];
	float render_modellight_lightdir_local[3];
	float render_modellight_specular[3];
	// lightmap rendering (not MATERIALFLAG_MODELLIGHT)
	float render_lightmap_ambient[3];
	float render_lightmap_diffuse[3];
	float render_lightmap_specular[3];
	// rtlights use these colors for the materials on this entity
	float render_rtlight_diffuse[3];
	float render_rtlight_specular[3];
	// tint applied on top of render_*_diffuse for pants layer
	float render_colormap_pants[3];
	// tint applied on top of render_*_diffuse for shirt layer
	float render_colormap_shirt[3];

	// from q3 shaders
	int customblendfunc[2];

	// q3bsp
	int surfaceflags;
	int supercontents;

	// q2bsp
	// we have to load the texture multiple times when Q2SURF_ flags differ,
	// though it still shares the skinframe
	int q2flags;
	int q2value;
	int q2contents;

	// q1qsp
	/// this points to a variant of the sky texture that has MATERIALFLAG_NOSHADOW, for the e1m5 logo shadow trick.
	struct texture_s *skynoshadowtexture;

	// reflection
	float reflectmin; // when refraction is used, minimum amount of reflection (when looking straight down)
	float reflectmax; // when refraction is used, maximum amount of reflection (when looking parallel to water)
	float refractfactor; // amount of refraction distort (1.0 = like the cvar specifies)
	vec4_t refractcolor4f; // color tint of refraction (including alpha factor)
	float reflectfactor; // amount of reflection distort (1.0 = like the cvar specifies)
	vec4_t reflectcolor4f; // color tint of reflection (including alpha factor)
	float r_water_wateralpha; // additional wateralpha to apply when r_water is active
	float r_water_waterscroll[2]; // scale and speed
	float refractive_index; // used by r_shadow_bouncegrid for bending photons for refracted light
	int camera_entity; // entity number for use by cameras

	// offsetmapping
	dpoffsetmapping_technique_t offsetmapping;
	float offsetscale;
	float offsetbias;

	// transparent sort category
	dptransparentsortcategory_t transparentsort;

	// gloss
	float specularscalemod;
	float specularpowermod;

	// diffuse and ambient
	float rtlightambient;

	// used by Mod_Mesh_GetTexture for drawflag and materialflag overrides, to disambiguate the same texture with different hints
	int mesh_drawflag;
	int mesh_defaulttexflags;
	int mesh_defaultmaterialflags;
}
 texture_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			textureindex;
	int			q1flags;
	int			q2flags;			// miptex flags + overrides
	int			q2value;			// light emission, etc
	char		q2texture[32];	// texture name (textures/*.wal)
	int			q2nexttexinfo;	// for animations, -1 = end of chain
}
mtexinfo_t;

typedef struct msurface_lightmapinfo_s
{
	// texture mapping properties used by this surface
	mtexinfo_t *texinfo; // q1bsp
	// index into r_refdef.scene.lightstylevalue array, 255 means not used (black)
	unsigned char styles[MAXLIGHTMAPS]; // q1bsp
	// RGB lighting data [numstyles][height][width][3]
	unsigned char *samples; // q1bsp
	// RGB normalmap data [numstyles][height][width][3]
	unsigned char *nmapsamples; // q1bsp
	// stain to apply on lightmap (soot/dirt/blood/whatever)
	unsigned char *stainsamples; // q1bsp
	int texturemins[2]; // q1bsp
	int extents[2]; // q1bsp
	int lightmaporigin[2]; // q1bsp
}
msurface_lightmapinfo_t;

struct q3deffect_s;

/// <summary>
///  describes the textures to use on a range of triangles in the model, and mins/maxs (AABB) for culling.
/// </summary>
typedef struct msurface_s
{
	/// range of triangles and vertices in model->surfmesh
	int num_triangles; // triangles
	int num_firsttriangle; // first element is this *3
	int num_vertices; // length of the range referenced by elements
	int num_firstvertex; // min vertex referenced by elements

	/// the texture to use on the surface
	texture_t *texture;
	/// the lightmap texture fragment to use on the rendering mesh
	struct rtexture_s *lightmaptexture;
	/// the lighting direction texture fragment to use on the rendering mesh
	struct rtexture_s *deluxemaptexture;

	// the following fields are used situationally and are not part of rendering in typical usage

	/// bounding box for onscreen checks
	vec3_t mins;
	vec3_t maxs;

	/// lightmaptexture rebuild information not used in q3bsp
	msurface_lightmapinfo_t* lightmapinfo; // q1bsp
	/// fog volume info in q3bsp
	struct q3deffect_s* effect; // q3bsp

	/// mesh information for collisions (only used by q3bsp curves)
	int num_firstcollisiontriangle; // q3bsp only
	int num_collisiontriangles; // number of triangles (if surface has collisions enabled)
	int num_collisionvertices; // number of vertices referenced by collision triangles (if surface has collisions enabled)

	// used by Mod_Mesh_Finalize when building sortedmodelsurfaces
	qbool included;
}
msurface_t;

#include "bih.h"

#include "model_brush.h"
#include "model_q1bsp.h"
#include "model_q2bsp.h"
#include "model_q3bsp.h"
#include "model_vbsp.h"
#include "model_sprite.h"
#include "model_alias.h"

struct trace_s;

struct frameblend_s;
struct skeleton_s;

typedef struct model_s
{
	// name and path of model, for example "progs/player.mdl"
	char			name[MAX_QPATH];
	// model needs to be loaded if this is false
	qbool		loaded;
	// set if the model is used in current map, models which are not, are purged
	qbool		used;
	// CRC of the file this model was loaded from, to reload if changed
	unsigned int	crc;
	// mod_brush, mod_alias, mod_sprite
	modtype_t		type;
	// memory pool for allocations
	struct mempool_s		*mempool;
	// all models use textures...
	rtexturepool_t	*texturepool;
	// EF_* flags (translated from the model file's different flags layout)
	unsigned				effects;
	// number of QC accessible frame(group)s in the model
	int				numframes;
	// number of QC accessible skin(group)s in the model
	int				numskins;
	// whether to randomize animated framegroups
	synctype_t		synctype;
	// bounding box at angles '0 0 0'
	vec3_t			normalmins, normalmaxs;
	// bounding box if yaw angle is not 0, but pitch and roll are
	vec3_t			yawmins, yawmaxs;
	// bounding box if pitch or roll are used
	vec3_t			rotatedmins, rotatedmaxs;
	// sphere radius, usable at any angles
	float			radius;
	// squared sphere radius for easier comparisons
	float			radius2;
	// skin animation info
	animscene_t		*skinscenes; // [numskins]
	// skin animation info
	animscene_t		*animscenes; // [numframes]
	// range of surface numbers in this model
	int				submodelsurfaces_start;
	int				submodelsurfaces_end;
	/// surface indices of model in an optimal draw order (submodelindex -> texture -> lightmap -> index)
	int				*modelsurfaces_sorted; // same size as num_surfaces
	// range of collision brush numbers in this (sub)model
	int				firstmodelbrush;
	int				nummodelbrushes;
	// BIH (Bounding Interval Hierarchy) for this (sub)model
	bih_t			collision_bih;
	bih_t			render_bih; // if not set, use collision_bih instead for rendering purposes too
	// for md3 models
	int				num_tags;
	int				num_tagframes;
	aliastag_t		*data_tags;
	// for skeletal models
	int				num_bones;
	aliasbone_t		*data_bones;
	float			num_posescale; // scaling factor from origin in poses7s format (includes divide by 32767)
	float			num_poseinvscale; // scaling factor to origin in poses7s format (includes multiply by 32767)
	int				num_poses;
	short			*data_poses7s; // origin xyz, quat xyzw, unit length, values normalized to +/-32767 range
	float			*data_baseboneposeinverse;
	// textures of this model
	int				num_textures;
	int				max_textures; // preallocated for expansion (Mod_Mesh_*)
	int				num_texturesperskin;
	texture_t		*data_textures;
	qbool		wantnormals;
	qbool		wanttangents;
	// surfaces of this model
	int				num_surfaces;
	int				max_surfaces; // preallocated for expansion (Mod_Mesh_*)
	msurface_t		*data_surfaces;
	// optional lightmapinfo data for surface lightmap updates
	msurface_lightmapinfo_t *data_surfaces_lightmapinfo;
	// all surfaces belong to this mesh
	surfmesh_t		surfmesh;
	// data type of model
	const char		*modeldatatypestring;
	// generates vertex data for a given frameblend
	void(*AnimateVertices)(const struct model_s * RESTRICT model, const struct frameblend_s * RESTRICT frameblend, const struct skeleton_s *skeleton, float * RESTRICT vertex3f, float * RESTRICT normal3f, float * RESTRICT svector3f, float * RESTRICT tvector3f);
	// draw the model's sky polygons
	void(*DrawSky)(struct entity_render_s *ent);
	// draw refraction/reflection textures for the model's water polygons
	void(*DrawAddWaterPlanes)(struct entity_render_s *ent);
	// draw the model using lightmap/dlight shading
	void(*Draw)(struct entity_render_s *ent);
	// draw the model to the depth buffer (no color rendering at all)
	void(*DrawDepth)(struct entity_render_s *ent);
	// draw any enabled debugging effects on this model (such as showing triangles, normals, collision brushes...)
	void(*DrawDebug)(struct entity_render_s *ent);
	// draw geometry textures for deferred rendering
	void(*DrawPrepass)(struct entity_render_s *ent);
    // compile an optimized shadowmap mesh for the model based on light source
	void(*CompileShadowMap)(struct entity_render_s *ent, vec3_t relativelightorigin, vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist);
	// draw depth into a shadowmap
	void(*DrawShadowMap)(int side, struct entity_render_s *ent, const vec3_t relativelightorigin, const vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist, const unsigned char *surfacesides, const vec3_t lightmins, const vec3_t lightmaxs);
	// gathers info on which clusters and surfaces are lit by light, as well as calculating a bounding box
	void(*GetLightInfo)(struct entity_render_s *ent, vec3_t relativelightorigin, float lightradius, vec3_t outmins, vec3_t outmaxs, int *outleaflist, unsigned char *outleafpvs, int *outnumleafspointer, int *outsurfacelist, unsigned char *outsurfacepvs, int *outnumsurfacespointer, unsigned char *outshadowtrispvs, unsigned char *outlighttrispvs, unsigned char *visitingleafpvs, int numfrustumplanes, const mplane_t *frustumplanes, qbool noocclusion);
	// draw the lighting on a model (through stencil)
	void(*DrawLight)(struct entity_render_s *ent, int numsurfaces, const int *surfacelist, const unsigned char *trispvs);
	// trace a box against this model
	void (*TraceBox)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t boxmins, const vec3_t boxmaxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
	void (*TraceBrush)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, struct colbrushf_s *start, struct colbrushf_s *end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
	// trace a box against this model
	void (*TraceLine)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
	// trace a point against this model (like PointSuperContents)
	void (*TracePoint)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
	// find the supercontents value at a point in this model
	int (*PointSuperContents)(struct model_s *model, int frame, const vec3_t point);
	// trace a line against geometry in this model and report correct texture (used by r_shadow_bouncegrid)
	void (*TraceLineAgainstSurfaces)(struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
	// fields belonging to some types of model
	model_sprite_t	sprite;
	model_brush_t	brush;
	model_brushq1_t	brushq1;
	model_brushq3_t	brushq3;
	// flags this model for offseting sounds to the model center (used by brush models)
	int soundfromcenter;

	// if set, the model contains light information (lightmap, or vertexlight)
	qbool lit;
	float lightmapscale;

	qbool nolerp;
}
model_t;

//============================================================================

// model loading
extern model_t *loadmodel;
extern unsigned char *mod_base;

typedef struct modloader_s
{
	const char *extension;
	const char *header;
	size_t headersize; // The header might not be NULL terminated
	void (*Load)(model_t *, void *, void *);
} modloader_t;

// sky/water subdivision
//extern struct cvar_s gl_subdivide_size;
// texture fullbrights
extern struct cvar_s r_fullbrights;

extern struct cvar_s mod_noshader_default_offsetmapping;
extern struct cvar_s mod_q3shader_default_offsetmapping;
extern struct cvar_s mod_q3shader_default_offsetmapping_scale;
extern struct cvar_s mod_q3shader_default_offsetmapping_bias;
extern struct cvar_s mod_q3shader_default_polygonoffset;
extern struct cvar_s mod_q3shader_default_polygonfactor;
extern struct cvar_s mod_q3shader_default_refractive_index;
extern struct cvar_s mod_q3shader_force_addalpha;
extern struct cvar_s mod_q3shader_force_terrain_alphaflag;
extern struct cvar_s mod_q3bsp_lightgrid_texture;
extern struct cvar_s mod_q3bsp_lightgrid_world_surfaces;
extern struct cvar_s mod_q3bsp_lightgrid_bsp_surfaces;

void Mod_Init (void);
void Mod_Reload (void);
model_t *Mod_LoadModel(model_t *mod, qbool crash, qbool checkdisk);
model_t *Mod_FindName (const char *name, const char *parentname);
model_t *Mod_ForName (const char *name, qbool crash, qbool checkdisk, const char *parentname);
void Mod_UnloadModel (model_t *mod);

void Mod_ClearUsed(void);
void Mod_PurgeUnused(void);
void Mod_RemoveStaleWorldModels(model_t *skip); // only used during loading!

extern model_t *loadmodel;
extern char loadname[32];	// for hunk tags

int Mod_BuildVertexRemapTableFromElements(int numelements, const int *elements, int numvertices, int *remapvertices);
void Mod_BuildNormals(int firstvertex, int numvertices, int numtriangles, const float *vertex3f, const int *elements, float *normal3f, qbool areaweighting);
void Mod_BuildTextureVectorsFromNormals(int firstvertex, int numvertices, int numtriangles, const float *vertex3f, const float *texcoord2f, const float *normal3f, const int *elements, float *svector3f, float *tvector3f, qbool areaweighting);

qbool Mod_ValidateElements(int *element3i, unsigned short *element3s, int numtriangles, int firstvertex, int numvertices, const char *filename, int fileline);
void Mod_AllocSurfMesh(struct mempool_s *mempool, int numvertices, int numtriangles, qbool lightmapoffsets, qbool vertexcolors);
void Mod_MakeSortedSurfaces(model_t *mod);

// called specially by brush model loaders before generating submodels
// automatically called after model loader returns
void Mod_BuildVBOs(void);

/// Sets the mod->DrawSky and mod->DrawAddWaterPlanes pointers conditionally based on whether surfaces in this submodel use these features
/// called specifically by brush model loaders when generating submodels
/// automatically called after model loader returns
void Mod_SetDrawSkyAndWater(model_t* mod);

shadowmesh_t *Mod_ShadowMesh_Alloc(struct mempool_s *mempool, int maxverts, int maxtriangles);
int Mod_ShadowMesh_AddVertex(shadowmesh_t *mesh, const float *vertex3f);
void Mod_ShadowMesh_AddMesh(shadowmesh_t *mesh, const float *vertex3f, int numtris, const int *element3i);
shadowmesh_t *Mod_ShadowMesh_Begin(struct mempool_s *mempool, int maxverts, int maxtriangles);
shadowmesh_t *Mod_ShadowMesh_Finish(shadowmesh_t *firstmesh, qbool createvbo);
void Mod_ShadowMesh_CalcBBox(shadowmesh_t *firstmesh, vec3_t mins, vec3_t maxs, vec3_t center, float *radius);
void Mod_ShadowMesh_Free(shadowmesh_t *mesh);

void Mod_CreateCollisionMesh(model_t *mod);

void Mod_FreeQ3Shaders(void);
void Mod_LoadQ3Shaders(void);
shader_t *Mod_LookupQ3Shader(const char *name);
qbool Mod_LoadTextureFromQ3Shader(struct mempool_s *mempool, const char *modelname, texture_t *texture, const char *name, qbool warnmissing, qbool fallback, int defaulttexflags, int defaultmaterialflags);
texture_shaderpass_t *Mod_CreateShaderPass(struct mempool_s *mempool, struct skinframe_s *skinframe);
texture_shaderpass_t *Mod_CreateShaderPassFromQ3ShaderLayer(struct mempool_s *mempool, const char *modelname, q3shaderinfo_layer_t *layer, int layerindex, int texflags, const char *texturename);
/// Sets up a material to render the provided skinframe.  See also R_SkinFrame_LoadInternalBGRA.
void Mod_LoadCustomMaterial(struct mempool_s *mempool, texture_t *texture, const char *name, int supercontents, int materialflags, struct skinframe_s *skinframe);
/// Removes all shaderpasses from material, and optionally deletes the textures in the skinframes.
void Mod_UnloadCustomMaterial(texture_t *texture, qbool purgeskins);

extern struct cvar_s r_mipskins;
extern struct cvar_s r_mipnormalmaps;

typedef struct skinfileitem_s
{
	struct skinfileitem_s *next;
	char name[MAX_QPATH];
	char replacement[MAX_QPATH];
}
skinfileitem_t;

typedef struct skinfile_s
{
	struct skinfile_s *next;
	skinfileitem_t *items;
}
skinfile_t;

skinfile_t *Mod_LoadSkinFiles(void);
void Mod_FreeSkinFiles(skinfile_t *skinfile);
int Mod_CountSkinFiles(skinfile_t *skinfile);
void Mod_BuildAliasSkinsFromSkinFiles(texture_t *skin, skinfile_t *skinfile, const char *meshname, const char *shadername);

void Mod_SnapVertices(int numcomponents, int numvertices, float *vertices, float snap);
int Mod_RemoveDegenerateTriangles(int numtriangles, const int *inelement3i, int *outelement3i, const float *vertex3f);
void Mod_VertexRangeFromElements(int numelements, const int *elements, int *firstvertexpointer, int *lastvertexpointer);

typedef struct mod_alloclightmap_row_s
{
	int rowY;
	int currentX;
}
mod_alloclightmap_row_t;

typedef struct mod_alloclightmap_state_s
{
	int width;
	int height;
	int currentY;
	mod_alloclightmap_row_t *rows;
}
mod_alloclightmap_state_t;

void Mod_AllocLightmap_Init(mod_alloclightmap_state_t *state, struct mempool_s *mempool, int width, int height);
void Mod_AllocLightmap_Free(mod_alloclightmap_state_t *state);
void Mod_AllocLightmap_Reset(mod_alloclightmap_state_t *state);
qbool Mod_AllocLightmap_Block(mod_alloclightmap_state_t *state, int blockwidth, int blockheight, int *outx, int *outy);

// bsp models
void Mod_BrushInit(void);
// used for talking to the QuakeC mainly
int Mod_Q1BSP_NativeContentsFromSuperContents(int supercontents);
int Mod_Q1BSP_SuperContentsFromNativeContents(int nativecontents);
// used for loading wal files in Mod_LoadTextureFromQ3Shader
int Mod_Q2BSP_SuperContentsFromNativeContents(int nativecontents);
int Mod_Q2BSP_NativeContentsFromSuperContents(int supercontents);

// a lot of model formats use the Q1BSP code, so here are the prototypes...
struct entity_render_s;
void R_Mod_DrawAddWaterPlanes(struct entity_render_s *ent);
void R_Mod_DrawSky(struct entity_render_s *ent);
void R_Mod_Draw(struct entity_render_s *ent);
void R_Mod_DrawDepth(struct entity_render_s *ent);
void R_Mod_DrawDebug(struct entity_render_s *ent);
void R_Mod_DrawPrepass(struct entity_render_s *ent);
void R_Mod_GetLightInfo(struct entity_render_s *ent, vec3_t relativelightorigin, float lightradius, vec3_t outmins, vec3_t outmaxs, int *outleaflist, unsigned char *outleafpvs, int *outnumleafspointer, int *outsurfacelist, unsigned char *outsurfacepvs, int *outnumsurfacespointer, unsigned char *outshadowtrispvs, unsigned char *outlighttrispvs, unsigned char *visitingleafpvs, int numfrustumplanes, const mplane_t *frustumplanes, qbool noocclusion);
void R_Mod_CompileShadowMap(struct entity_render_s *ent, vec3_t relativelightorigin, vec3_t relativelightdirection, float lightradius, int numsurfaces, const int *surfacelist);
void R_Mod_DrawShadowMap(int side, struct entity_render_s *ent, const vec3_t relativelightorigin, const vec3_t relativelightdirection, float lightradius, int modelnumsurfaces, const int *modelsurfacelist, const unsigned char *surfacesides, const vec3_t lightmins, const vec3_t lightmaxs);
void R_Mod_DrawLight(struct entity_render_s *ent, int numsurfaces, const int *surfacelist, const unsigned char *trispvs);

// dynamic mesh building (every frame) for debugging and other uses
void Mod_Mesh_Create(model_t *mod, const char *name);
void Mod_Mesh_Destroy(model_t *mod);
void Mod_Mesh_Reset(model_t *mod);
texture_t *Mod_Mesh_GetTexture(model_t *mod, const char *name, int defaultdrawflags, int defaulttexflags, int defaultmaterialflags);
msurface_t *Mod_Mesh_AddSurface(model_t *mod, texture_t *tex, qbool batchwithprevioussurface);
int Mod_Mesh_IndexForVertex(model_t *mod, msurface_t *surf, float x, float y, float z, float nx, float ny, float nz, float s, float t, float u, float v, float r, float g, float b, float a);
void Mod_Mesh_AddTriangle(model_t *mod, msurface_t *surf, int e0, int e1, int e2);
void Mod_Mesh_Validate(model_t *mod);
void Mod_Mesh_Finalize(model_t *mod);

// Collision optimization using Bounding Interval Hierarchy
void Mod_CollisionBIH_TracePoint(model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
void Mod_CollisionBIH_TraceLine(model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
void Mod_CollisionBIH_TraceBox(model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, const vec3_t boxmins, const vec3_t boxmaxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
void Mod_CollisionBIH_TraceBrush(model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, struct colbrushf_s *start, struct colbrushf_s *end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
void Mod_CollisionBIH_TracePoint_Mesh(model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, struct trace_s *trace, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
qbool Mod_CollisionBIH_TraceLineOfSight(struct model_s *model, const vec3_t start, const vec3_t end, const vec3_t acceptmins, const vec3_t acceptmaxs);
int Mod_CollisionBIH_PointSuperContents(struct model_s *model, int frame, const vec3_t point);
int Mod_CollisionBIH_PointSuperContents_Mesh(struct model_s *model, int frame, const vec3_t point);
bih_t *Mod_MakeCollisionBIH(model_t *model, qbool userendersurfaces, bih_t *out);

// alias models
struct frameblend_s;
struct skeleton_s;
void Mod_AliasInit(void);
int Mod_Alias_GetTagMatrix(const model_t *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, int tagindex, matrix4x4_t *outmatrix);
int Mod_Alias_GetTagIndexForName(const model_t *model, unsigned int skin, const char *tagname);
int Mod_Alias_GetExtendedTagInfoForIndex(const model_t *model, unsigned int skin, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, int tagindex, int *parentindex, const char **tagname, matrix4x4_t *tag_localmatrix);

void Mod_Skeletal_FreeBuffers(void);

// sprite models
void Mod_SpriteInit(void);

// loaders
void Mod_2PSB_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_BSP2_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_HLBSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_Q1BSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IBSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_VBSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_MAP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_OBJ_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDP0_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDP2_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDP3_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_ZYMOTICMODEL_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_DARKPLACESMODEL_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_PSKMODEL_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDSP_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_IDS2_Load(model_t *mod, void *buffer, void *bufferend);
void Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer, void *bufferend);

#endif	// MODEL_SHARED_H

