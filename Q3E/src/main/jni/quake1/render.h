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

#ifndef RENDER_H
#define RENDER_H

#include "svbsp.h"
#include "r_stats.h"

typedef enum glsl_attrib_e
{
	GLSLATTRIB_POSITION = 0,
	GLSLATTRIB_COLOR = 1,
	GLSLATTRIB_TEXCOORD0 = 2,
	GLSLATTRIB_TEXCOORD1 = 3,
	GLSLATTRIB_TEXCOORD2 = 4,
	GLSLATTRIB_TEXCOORD3 = 5,
	GLSLATTRIB_TEXCOORD4 = 6,
	GLSLATTRIB_TEXCOORD5 = 7,
	GLSLATTRIB_TEXCOORD6 = 8,
	GLSLATTRIB_TEXCOORD7 = 9,
}
glsl_attrib;

typedef enum shaderlanguage_e
{
	SHADERLANGUAGE_GLSL,
	SHADERLANGUAGE_COUNT
}
shaderlanguage_t;

/// this enum selects which of the glslshadermodeinfo entries should be used
typedef enum shadermode_e
{
	SHADERMODE_GENERIC, ///< (particles/HUD/etc) vertex color, optionally multiplied by one texture
	SHADERMODE_POSTPROCESS, ///< postprocessing shader (r_glsl_postprocess)
	SHADERMODE_DEPTH_OR_SHADOW, ///< (depthfirst/shadows) vertex shader only
	SHADERMODE_FLATCOLOR, ///< (lightmap) modulate texture by uniform color (q1bsp, q3bsp)
	SHADERMODE_VERTEXCOLOR, ///< (lightmap) modulate texture by vertex colors (q3bsp)
	SHADERMODE_LIGHTMAP, ///< (lightmap) modulate texture by lightmap texture (q1bsp, q3bsp)
	SHADERMODE_LIGHTDIRECTIONMAP_MODELSPACE, ///< (lightmap) use directional pixel shading from texture containing modelspace light directions (q3bsp deluxemap)
	SHADERMODE_LIGHTDIRECTIONMAP_TANGENTSPACE, ///< (lightmap) use directional pixel shading from texture containing tangentspace light directions (q1bsp deluxemap)
	SHADERMODE_LIGHTDIRECTIONMAP_FORCED_LIGHTMAP, // forced deluxemapping for lightmapped surfaces
	SHADERMODE_LIGHTDIRECTIONMAP_FORCED_VERTEXCOLOR, // forced deluxemapping for vertexlit surfaces
	SHADERMODE_LIGHTGRID, ///< (lightmap) use directional pixel shading from lightgrid data (q3bsp)
	SHADERMODE_LIGHTDIRECTION, ///< (lightmap) use directional pixel shading from fixed light direction (q3bsp)
	SHADERMODE_LIGHTSOURCE, ///< (lightsource) use directional pixel shading from light source (rtlight)
	SHADERMODE_REFRACTION, ///< refract background (the material is rendered normally after this pass)
	SHADERMODE_WATER, ///< refract background and reflection (the material is rendered normally after this pass)
	SHADERMODE_DEFERREDGEOMETRY, ///< (deferred) render material properties to screenspace geometry buffers
	SHADERMODE_DEFERREDLIGHTSOURCE, ///< (deferred) use directional pixel shading from light source (rtlight) on screenspace geometry buffers
	SHADERMODE_COUNT
}
shadermode_t;

#define SHADERPERMUTATION_DIFFUSE (1u<<0) ///< (lightsource) whether to use directional shading
#define SHADERPERMUTATION_VERTEXTEXTUREBLEND (1u<<1) ///< indicates this is a two-layer material blend based on vertex alpha (q3bsp)
#define SHADERPERMUTATION_VIEWTINT (1u<<2) ///< view tint (postprocessing only), use vertex colors (generic only)
#define SHADERPERMUTATION_COLORMAPPING (1u<<3) ///< indicates this is a colormapped skin
#define SHADERPERMUTATION_SATURATION (1u<<4) ///< saturation (postprocessing only)
#define SHADERPERMUTATION_FOGINSIDE (1u<<5) ///< tint the color by fog color or black if using additive blend mode
#define SHADERPERMUTATION_FOGOUTSIDE (1u<<6) ///< tint the color by fog color or black if using additive blend mode
#define SHADERPERMUTATION_FOGHEIGHTTEXTURE (1u<<7) ///< fog color and density determined by texture mapped on vertical axis
#define SHADERPERMUTATION_FOGALPHAHACK (1u<<8) ///< fog color and density determined by texture mapped on vertical axis
#define SHADERPERMUTATION_GAMMARAMPS (1u<<9) ///< gamma (postprocessing only)
#define SHADERPERMUTATION_CUBEFILTER (1u<<10) ///< (lightsource) use cubemap light filter
#define SHADERPERMUTATION_GLOW (1u<<11) ///< (lightmap) blend in an additive glow texture
#define SHADERPERMUTATION_BLOOM (1u<<12) ///< bloom (postprocessing only)
#define SHADERPERMUTATION_SPECULAR (1u<<13) ///< (lightsource or deluxemapping) render specular effects
#define SHADERPERMUTATION_POSTPROCESSING (1u<<14) ///< user defined postprocessing (postprocessing only)
#define SHADERPERMUTATION_REFLECTION (1u<<15) ///< normalmap-perturbed reflection of the scene infront of the surface, preformed as an overlay on the surface
#define SHADERPERMUTATION_OFFSETMAPPING (1u<<16) ///< adjust texcoords to roughly simulate a displacement mapped surface
#define SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING (1u<<17) ///< adjust texcoords to accurately simulate a displacement mapped surface (requires OFFSETMAPPING to also be set!)
#define SHADERPERMUTATION_SHADOWMAP2D (1u<<18) ///< (lightsource) use shadowmap texture as light filter
#define SHADERPERMUTATION_SHADOWMAPVSDCT (1u<<19) ///< (lightsource) use virtual shadow depth cube texture for shadowmap indexing
#define SHADERPERMUTATION_SHADOWMAPORTHO (1u<<20) ///< (lightsource) use orthographic shadowmap projection
#define SHADERPERMUTATION_DEFERREDLIGHTMAP (1u<<21) ///< (lightmap) read Texture_ScreenDiffuse/Specular textures and add them on top of lightmapping
#define SHADERPERMUTATION_ALPHAKILL (1u<<22) ///< (deferredgeometry) discard pixel if diffuse texture alpha below 0.5, (generic) apply global alpha
#define SHADERPERMUTATION_REFLECTCUBE (1u<<23) ///< fake reflections using global cubemap (not HDRI light probe)
#define SHADERPERMUTATION_NORMALMAPSCROLLBLEND (1u<<24) ///< (water) counter-direction normalmaps scrolling
#define SHADERPERMUTATION_BOUNCEGRID (1u<<25) ///< (lightmap) use Texture_BounceGrid as an additional source of ambient light
#define SHADERPERMUTATION_BOUNCEGRIDDIRECTIONAL (1u<<26) ///< (lightmap) use 16-component pixels in bouncegrid texture for directional lighting rather than standard 4-component
#define SHADERPERMUTATION_TRIPPY (1u<<27) ///< use trippy vertex shader effect
#define SHADERPERMUTATION_DEPTHRGB (1u<<28) ///< read/write depth values in RGB color coded format for older hardware without depth samplers
#define SHADERPERMUTATION_ALPHAGEN_VERTEX (1u<<29) ///< alphaGen vertex
#define SHADERPERMUTATION_SKELETAL (1u<<30) ///< (skeletal models) use skeletal matrices to deform vertices (gpu-skinning)
#define SHADERPERMUTATION_OCCLUDE (1u<<31) ///< use occlusion buffer for corona
#define SHADERPERMUTATION_COUNT 32u ///< size of shaderpermutationinfo array
// UBSan: unsigned literals because left shifting by 31 causes signed overflow, although it works as expected on x86.

// 1.0f / N table
extern float ixtable[4096];

// fog stuff
void FOG_clear(void);

// sky stuff
extern cvar_t r_sky;
extern cvar_t r_skyscroll1;
extern cvar_t r_skyscroll2;
extern cvar_t r_sky_scissor;
extern cvar_t r_q3bsp_renderskydepth;
extern int skyrenderlater, skyrendermasked;
extern int skyscissor[4];
int R_SetSkyBox(const char *sky);
void R_SkyStartFrame(void);
void R_Sky(void);
void R_ResetSkyBox(void);

// SHOWLMP stuff (Nehahra)
void SHOWLMP_decodehide(void);
void SHOWLMP_decodeshow(void);
void SHOWLMP_drawall(void);

// render profiling stuff
extern int r_timereport_active;

// lighting stuff
extern cvar_t r_ambient;
extern cvar_t gl_flashblend;

// vis stuff
extern cvar_t r_novis;

extern cvar_t r_trippy;
extern cvar_t r_fxaa;

extern cvar_t r_lerpsprites;
extern cvar_t r_lerpmodels;
extern cvar_t r_lerplightstyles;
extern cvar_t r_waterscroll;

extern cvar_t developer_texturelogging;

// shadow volume bsp struct with automatically growing nodes buffer
extern svbsp_t r_svbsp;

typedef struct rmesh_s
{
	// vertices of this mesh
	int maxvertices;
	int numvertices;
	float *vertex3f;
	float *svector3f;
	float *tvector3f;
	float *normal3f;
	float *texcoord2f;
	float *texcoordlightmap2f;
	float *color4f;
	// triangles of this mesh
	int maxtriangles;
	int numtriangles;
	int *element3i;
	// snapping epsilon
	float epsilon2;
}
rmesh_t;

// useful functions for rendering
void R_ModulateColors(float *in, float *out, int verts, float r, float g, float b);
void R_FillColors(float *out, int verts, float r, float g, float b, float a);
void R_Mesh_AddPolygon3f(rmesh_t *mesh, int numvertices, float *vertex3f);
void R_Mesh_AddBrushMeshFromPlanes(rmesh_t *mesh, int numplanes, mplane_t *planes);

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

extern cvar_t r_nearclip;

// forces all rendering to draw triangle outlines
extern cvar_t r_showoverdraw;
extern cvar_t r_showtris;
extern cvar_t r_shownormals;
extern cvar_t r_showlighting;
extern cvar_t r_showcollisionbrushes;
extern cvar_t r_showcollisionbrushes_polygonfactor;
extern cvar_t r_showcollisionbrushes_polygonoffset;
extern cvar_t r_showdisabledepthtest;

extern cvar_t r_drawentities;
extern cvar_t r_draw2d;
extern qbool r_draw2d_force;
extern cvar_t r_drawviewmodel;
extern cvar_t r_drawworld;
extern cvar_t r_speeds;
extern cvar_t r_fullbright;
extern cvar_t r_wateralpha;
extern cvar_t r_dynamic;

extern cvar_t r_q1bsp_lightmap_updates_enabled;
extern cvar_t r_q1bsp_lightmap_updates_combine;
extern cvar_t r_q1bsp_lightmap_updates_combine_full_texture;
extern cvar_t r_q1bsp_lightmap_updates_hidden_surfaces;

void R_NewExplosion(const vec3_t org);
void R_UpdateVariables(void); // must call after setting up most of r_refdef, but before calling R_RenderView
void R_RenderView(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture, int x, int y, int width, int height); // must set r_refdef and call R_UpdateVariables and CL_UpdateEntityShading first
void R_RenderView_UpdateViewVectors(void); // just updates r_refdef.view.{forward,left,up,origin,right,inverse_matrix}

float RSurf_FogPoint(const float *v);
float RSurf_FogVertex(const float *v);

typedef enum r_refdef_scene_type_s {
	RST_CLIENT,
	RST_MENU,
	RST_COUNT
} r_refdef_scene_type_t;

typedef enum r_viewport_type_e
{
	R_VIEWPORTTYPE_ORTHO,
	R_VIEWPORTTYPE_PERSPECTIVE,
	R_VIEWPORTTYPE_PERSPECTIVE_INFINITEFARCLIP,
	R_VIEWPORTTYPE_PERSPECTIVECUBESIDE,
	R_VIEWPORTTYPE_TOTAL
}
r_viewport_type_t;

typedef struct r_viewport_s
{
	matrix4x4_t cameramatrix;  ///< from entity (transforms from camera entity to world)
	matrix4x4_t viewmatrix;    ///< actual matrix for rendering (transforms to viewspace)
	matrix4x4_t projectmatrix; ///< actual projection matrix (transforms from viewspace to screen)
	int x;
	int y;
	int z;
	int width;
	int height;
	int depth;
	r_viewport_type_t type;
	float screentodepth[2];    ///< used by deferred renderer to calculate linear depth from device depth coordinates
}
r_viewport_t;

typedef struct r_refdef_view_s
{
	// view information (changes multiple times per frame)
	// if any of these variables change then r_refdef.viewcache must be regenerated
	// by calling R_View_Update
	// (which also updates viewport, scissor, colormask)

	// it is safe and expected to copy this into a structure on the stack and
	// call the renderer recursively, then restore from the stack afterward
	// (as long as R_View_Update is called)

	// eye position information
	matrix4x4_t matrix, inverse_matrix;
	vec3_t origin;
	vec3_t forward;
	vec3_t left;
	vec3_t right;
	vec3_t up;
	int numfrustumplanes;
	mplane_t frustum[6];
	qbool useclipplane;
	qbool usecustompvs; ///< uses r_refdef.viewcache.pvsbits as-is rather than computing it
	mplane_t clipplane;
	float frustum_x, frustum_y;
	vec3_t frustumcorner[4];
	/// if turned off it renders an ortho view
	int useperspective;
	/// allows visibility culling based on the view origin (e.g. pvs and R_CanSeeBox)
	/// this is turned off by:
	/// r_trippy
	/// !r_refdef.view.useperspective
	/// (sometimes) r_refdef.view.useclipplane
	int usevieworiginculling;
	float ortho_x, ortho_y;

	// screen area to render in
	int x;
	int y;
	int z;
	int width;
	int height;
	int depth;
	r_viewport_t viewport; ///< note: if r_viewscale is used, the viewport.width and viewport.height may be less than width and height

	/// which color components to allow (for anaglyph glasses)
	int colormask[4];

	/// global RGB color multiplier for rendering
	float colorscale;

	/// whether to call R_ClearScreen before rendering stuff
	qbool clear;
	/// if true, don't clear or do any post process effects (bloom, etc)
	qbool isoverlay;
	/// if true, this is the MAIN view (which is, after CSQC, copied into the scene for use e.g. by r_speeds 1, showtex, prydon cursor)
	qbool ismain;

	// whether to draw r_showtris and such, this is only true for the main
	// view render, all secondary renders (mirrors, portals, cameras,
	// distortion effects, etc) omit such debugging information
	qbool showdebug;

	// these define which values to use in GL_CullFace calls to request frontface or backface culling
	int cullface_front;
	int cullface_back;

	/// render quality (0 to 1) - affects r_drawparticles_drawdistance and others
	float quality;
}
r_refdef_view_t;

typedef struct r_refdef_viewcache_s
{
	// updated by gl_main_newmap()
	int maxentities;
	int world_numleafs;
	int world_numsurfaces;

	// these properties are generated by R_View_Update()

	/// which entities are currently visible for this viewpoint
	/// (the used range is 0...r_refdef.scene.numentities)
	unsigned char *entityvisible;

	// flag arrays used for visibility checking on world model
	// (all other entities have no per-surface/per-leaf visibility checks)
	unsigned char *world_pvsbits;
	unsigned char *world_leafvisible;
	unsigned char *world_surfacevisible;
	/// if true, the view is currently in a leaf without pvs data
	qbool world_novis;
}
r_refdef_viewcache_t;

// TODO: really think about which fields should go into scene and which one should stay in refdef [1/7/2008 Black]
// maybe also refactor some of the functions to support different setting sources (ie. fogenabled, etc.) for different scenes
typedef struct r_refdef_scene_s {
	/// (client gameworld) time for rendering time based effects
	double time;

	/// the world
	entity_render_t *worldentity;

	/// same as worldentity->model
	model_t *worldmodel;

	/// renderable entities (excluding world)
	entity_render_t **entities;
	int numentities;
	int maxentities;

	/// field of temporary entities that is reset each (client) frame
	entity_render_t *tempentities;
	int numtempentities;
	int maxtempentities;
	qbool expandtempentities;

	// renderable dynamic lights
	rtlight_t *lights[MAX_DLIGHTS];
	rtlight_t templights[MAX_DLIGHTS];
	int numlights;

	// intensities for light styles right now, controls rtlights
	float rtlightstylevalue[MAX_LIGHTSTYLES]; ///< float fraction of base light value
	// 8.8bit fixed point intensities for light styles
	// controls intensity lightmap layers
	unsigned short lightstylevalue[MAX_LIGHTSTYLES]; ///< 8.8 fraction of base light value

	// adds brightness to the whole scene, separate from lightmapintensity
	// see CL_UpdateEntityShading
	float ambientintensity;
	// brightness of lightmap and modellight lighting on materials
	// see CL_UpdateEntityShading
	float lightmapintensity;

	qbool rtworld;
	qbool rtworldshadows;
	qbool rtdlight;
	qbool rtdlightshadows;
} r_refdef_scene_t;

typedef struct r_refdef_s
{
	// these fields define the basic rendering information for the world
	// but not the view, which could change multiple times in one rendered
	// frame (for example when rendering textures for certain effects)

	// these are set for water warping before
	// frustum_x/frustum_y are calculated
	float frustumscale_x, frustumscale_y;

	// current view settings (these get reset a few times during rendering because of water rendering, reflections, etc)
	r_refdef_view_t view;
	r_refdef_viewcache_t viewcache;

	// minimum visible distance (pixels closer than this disappear)
	double nearclip;
	// maximum visible distance (pixels further than this disappear in 16bpp modes,
	// in 32bpp an infinite-farclip matrix is used instead)
	double farclip;

	// fullscreen color blend
	float viewblend[4];

	r_refdef_scene_t scene;

	float fogplane[4];
	float fogplaneviewdist;
	qbool fogplaneviewabove;
	float fogheightfade;
	float fogcolor[3];
	float fogrange;
	float fograngerecip;
	float fogmasktabledistmultiplier;
#define FOGMASKTABLEWIDTH 1024
	float fogmasktable[FOGMASKTABLEWIDTH];
	float fogmasktable_start, fogmasktable_alpha, fogmasktable_range, fogmasktable_density;
	float fog_density;
	float fog_red;
	float fog_green;
	float fog_blue;
	float fog_alpha;
	float fog_start;
	float fog_end;
	float fog_height;
	float fog_fadedepth;
	qbool fogenabled;
	qbool oldgl_fogenable;

	// new flexible texture height fog (overrides normal fog)
	char fog_height_texturename[64]; // note: must be 64 for the sscanf code
	unsigned char *fog_height_table1d;
	unsigned char *fog_height_table2d;
	int fog_height_tablesize; // enable
	float fog_height_tablescale;
	float fog_height_texcoordscale;
	char fogheighttexturename[64]; // detects changes to active fog height texture

	int draw2dstage; // 0 = no, 1 = yes, other value = needs setting up again

	// true during envmap command capture
	qbool envmap;

	// whether to draw world lights realtime, dlights realtime, and their shadows
	float polygonfactor;
	float polygonoffset;

	// how long R_RenderView took on the previous frame
	double lastdrawscreentime;

	// rendering stats for r_speeds display
	// (these are incremented in many places)
	int stats[r_stat_count];
}
r_refdef_t;

extern r_refdef_t r_refdef;

void R_SelectScene( r_refdef_scene_type_t scenetype );
r_refdef_scene_t * R_GetScenePointer( r_refdef_scene_type_t scenetype );

void R_SkinFrame_PrepareForPurge(void);
void R_SkinFrame_MarkUsed(skinframe_t *skinframe);
void R_SkinFrame_PurgeSkinFrame(skinframe_t *skinframe);
void R_SkinFrame_Purge(void);
// set last to NULL to start from the beginning
skinframe_t *R_SkinFrame_FindNextByName( skinframe_t *last, const char *name );
skinframe_t *R_SkinFrame_Find(const char *name, int textureflags, int comparewidth, int compareheight, int comparecrc, qbool add);
skinframe_t *R_SkinFrame_LoadExternal(const char *name, int textureflags, qbool complain, qbool fallbacknotexture);
skinframe_t *R_SkinFrame_LoadExternal_SkinFrame(skinframe_t *skinframe, const char *name, int textureflags, qbool complain, qbool fallbacknotexture);
skinframe_t *R_SkinFrame_LoadInternalBGRA(const char *name, int textureflags, const unsigned char *skindata, int width, int height, int comparewidth, int compareheight, int comparecrc, qbool sRGB);
skinframe_t *R_SkinFrame_LoadInternalQuake(const char *name, int textureflags, int loadpantsandshirt, int loadglowtexture, const unsigned char *skindata, int width, int height);
skinframe_t *R_SkinFrame_LoadInternal8bit(const char *name, int textureflags, const unsigned char *skindata, int width, int height, const unsigned int *palette, const unsigned int *alphapalette);
skinframe_t *R_SkinFrame_LoadMissing(void);
skinframe_t *R_SkinFrame_LoadNoTexture(void);
skinframe_t *R_SkinFrame_LoadInternalUsingTexture(const char *name, int textureflags, rtexture_t *tex, int width, int height, qbool sRGB);

rtexture_t *R_GetCubemap(const char *basename);

void R_View_WorldVisibility(qbool forcenovis);
void R_DrawParticles(void);
void R_DrawExplosions(void);

#define gl_solid_format 3
#define gl_alpha_format 4

qbool R_CullFrustum(const vec3_t mins, const vec3_t maxs);
qbool R_CullBox(const vec3_t mins, const vec3_t maxs, int numplanes, const mplane_t *planes);
qbool R_CanSeeBox(int numsamples, vec_t eyejitter, vec_t entboxenlarge, vec_t entboxexpand, vec_t pad, vec3_t eye, vec3_t entboxmins, vec3_t entboxmaxs);

#include "r_modules.h"

#include "meshqueue.h"

/// free all R_FrameData memory
void R_FrameData_Reset(void);
/// prepare for a new frame, recycles old buffers if a resize occurred previously
void R_FrameData_NewFrame(void);
/// allocate some temporary memory for your purposes
void *R_FrameData_Alloc(size_t size);
/// allocate some temporary memory and copy this data into it
void *R_FrameData_Store(size_t size, void *data);
/// set a marker that allows you to discard the following temporary memory allocations
void R_FrameData_SetMark(void);
/// discard recent memory allocations (rewind to marker)
void R_FrameData_ReturnToMark(void);

/// enum of the various types of hardware buffer object used in rendering
/// note that the r_buffermegs[] array must be maintained to match this
typedef enum r_bufferdata_type_e
{
	R_BUFFERDATA_VERTEX, /// vertex buffer
	R_BUFFERDATA_INDEX16, /// index buffer - 16bit (because D3D cares)
	R_BUFFERDATA_INDEX32, /// index buffer - 32bit (because D3D cares)
	R_BUFFERDATA_UNIFORM, /// uniform buffer
	R_BUFFERDATA_COUNT /// how many kinds of buffer we have
}
r_bufferdata_type_t;

/// free all dynamic vertex/index/uniform buffers
void R_BufferData_Reset(void);
/// begin a new frame (recycle old buffers)
void R_BufferData_NewFrame(void);
/// request space in a vertex/index/uniform buffer for the chosen data, returns the buffer pointer and offset, always successful
r_meshbuffer_t *R_BufferData_Store(size_t size, const void *data, r_bufferdata_type_t type, int *returnbufferoffset);

/// free all R_AnimCache memory
void R_AnimCache_Free(void);
/// clear the animcache pointers on all known render entities
void R_AnimCache_ClearCache(void);
/// get the skeletal data or cached animated mesh data for an entity (optionally with normals and tangents)
qbool R_AnimCache_GetEntity(entity_render_t *ent, qbool wantnormals, qbool wanttangents);
/// generate animcache data for all entities marked visible
void R_AnimCache_CacheVisibleEntities(void);

extern cvar_t r_render;
extern cvar_t r_renderview;
extern cvar_t r_waterwarp;

extern cvar_t r_glsl_offsetmapping;
extern cvar_t r_glsl_offsetmapping_reliefmapping;
extern cvar_t r_glsl_offsetmapping_scale;
extern cvar_t r_glsl_offsetmapping_lod;
extern cvar_t r_glsl_offsetmapping_lod_distance;
extern cvar_t r_glsl_deluxemapping;

extern cvar_t gl_polyblend;

extern cvar_t cl_deathfade;

extern cvar_t r_smoothnormals_areaweighting;

extern cvar_t r_test;

#include "gl_backend.h"

extern rtexture_t *r_texture_blanknormalmap;
extern rtexture_t *r_texture_white;
extern rtexture_t *r_texture_grey128;
extern rtexture_t *r_texture_black;
extern rtexture_t *r_texture_notexture;
extern rtexture_t *r_texture_whitecube;
extern rtexture_t *r_texture_normalizationcube;
extern rtexture_t *r_texture_fogattenuation;
extern rtexture_t *r_texture_fogheighttexture;

extern unsigned int r_queries[MAX_OCCLUSION_QUERIES];
extern unsigned int r_numqueries;
extern unsigned int r_maxqueries;

void R_TimeReport(const char *name);

// r_stain
void R_Stain(const vec3_t origin, float radius, int cr1, int cg1, int cb1, int ca1, int cr2, int cg2, int cb2, int ca2);

void R_CalcBeam_Vertex3f(float *vert, const float *org1, const float *org2, float width);
void R_CalcSprite_Vertex3f(float *vertex3f, const vec3_t origin, const vec3_t left, const vec3_t up, float scalex1, float scalex2, float scaley1, float scaley2);

extern mempool_t *r_main_mempool;

typedef struct rsurfacestate_s
{
	// current model array pointers
	// these may point to processing buffers if model is animated,
	// otherwise they point to static data.
	// these are not directly used for rendering, they are just another level
	// of processing
	//
	// these either point at array_model* buffers (if the model is animated)
	// or the model->surfmesh.data_* buffers (if the model is not animated)
	//
	// these are only set when an entity render begins, they do not change on
	// a per surface basis.
	//
	// this indicates the model* arrays are pointed at array_model* buffers
	// (in other words, the model has been animated in software)
	qbool                    forcecurrenttextureupdate; // set for RSurf_ActiveCustomEntity to force R_GetCurrentTexture to recalculate the texture parameters (such as entity alpha)
	qbool                    modelgeneratedvertex;
	// skeletal animation can be done by entity (animcache) or per batch,
	// batch may be non-skeletal even if entity is skeletal, indicating that
	// the dynamicvertex code path had to apply skeletal manually for a case
	// where gpu-skinning is not possible, for this reason batch has its own
	// variables
	int                         entityskeletalnumtransforms; // how many transforms are used for this mesh
	float                      *entityskeletaltransform3x4; // use gpu-skinning shader on this mesh
	const r_meshbuffer_t       *entityskeletaltransform3x4buffer; // uniform buffer
	int                         entityskeletaltransform3x4offset;
	int                         entityskeletaltransform3x4size;
	float                      *modelvertex3f;
	const r_meshbuffer_t       *modelvertex3f_vertexbuffer;
	int                         modelvertex3f_bufferoffset;
	float                      *modelsvector3f;
	const r_meshbuffer_t       *modelsvector3f_vertexbuffer;
	int                         modelsvector3f_bufferoffset;
	float                      *modeltvector3f;
	const r_meshbuffer_t       *modeltvector3f_vertexbuffer;
	int                         modeltvector3f_bufferoffset;
	float                      *modelnormal3f;
	const r_meshbuffer_t       *modelnormal3f_vertexbuffer;
	int                         modelnormal3f_bufferoffset;
	float                      *modellightmapcolor4f;
	const r_meshbuffer_t       *modellightmapcolor4f_vertexbuffer;
	int                         modellightmapcolor4f_bufferoffset;
	float                      *modeltexcoordtexture2f;
	const r_meshbuffer_t       *modeltexcoordtexture2f_vertexbuffer;
	int                         modeltexcoordtexture2f_bufferoffset;
	float                      *modeltexcoordlightmap2f;
	const r_meshbuffer_t       *modeltexcoordlightmap2f_vertexbuffer;
	int                         modeltexcoordlightmap2f_bufferoffset;
	unsigned char              *modelskeletalindex4ub;
	const r_meshbuffer_t       *modelskeletalindex4ub_vertexbuffer;
	int                         modelskeletalindex4ub_bufferoffset;
	unsigned char              *modelskeletalweight4ub;
	const r_meshbuffer_t       *modelskeletalweight4ub_vertexbuffer;
	int                         modelskeletalweight4ub_bufferoffset;
	int                        *modelelement3i;
	const r_meshbuffer_t       *modelelement3i_indexbuffer;
	int                         modelelement3i_bufferoffset;
	unsigned short             *modelelement3s;
	const r_meshbuffer_t       *modelelement3s_indexbuffer;
	int                         modelelement3s_bufferoffset;
	int                        *modellightmapoffsets;
	int                         modelnumvertices;
	int                         modelnumtriangles;
	const msurface_t           *modelsurfaces;
	// current rendering array pointers
	// these may point to any of several different buffers depending on how
	// much processing was needed to prepare this model for rendering
	// these usually equal the model* pointers, they only differ if
	// deformvertexes is used in a q3 shader, and consequently these can
	// change on a per-surface basis (according to rsurface.texture)
	qbool                    batchgeneratedvertex;
	qbool                    batchmultidraw;
	int                         batchmultidrawnumsurfaces;
	const msurface_t          **batchmultidrawsurfacelist;
	int                         batchfirstvertex;
	int                         batchnumvertices;
	int                         batchfirsttriangle;
	int                         batchnumtriangles;
	float                      *batchvertex3f;
	const r_meshbuffer_t       *batchvertex3f_vertexbuffer;
	int                         batchvertex3f_bufferoffset;
	float                      *batchsvector3f;
	const r_meshbuffer_t       *batchsvector3f_vertexbuffer;
	int                         batchsvector3f_bufferoffset;
	float                      *batchtvector3f;
	const r_meshbuffer_t       *batchtvector3f_vertexbuffer;
	int                         batchtvector3f_bufferoffset;
	float                      *batchnormal3f;
	const r_meshbuffer_t       *batchnormal3f_vertexbuffer;
	int                         batchnormal3f_bufferoffset;
	float                      *batchlightmapcolor4f;
	const r_meshbuffer_t       *batchlightmapcolor4f_vertexbuffer;
	int                         batchlightmapcolor4f_bufferoffset;
	float                      *batchtexcoordtexture2f;
	const r_meshbuffer_t       *batchtexcoordtexture2f_vertexbuffer;
	int                         batchtexcoordtexture2f_bufferoffset;
	float                      *batchtexcoordlightmap2f;
	const r_meshbuffer_t       *batchtexcoordlightmap2f_vertexbuffer;
	int                         batchtexcoordlightmap2f_bufferoffset;
	unsigned char              *batchskeletalindex4ub;
	const r_meshbuffer_t       *batchskeletalindex4ub_vertexbuffer;
	int                         batchskeletalindex4ub_bufferoffset;
	unsigned char              *batchskeletalweight4ub;
	const r_meshbuffer_t       *batchskeletalweight4ub_vertexbuffer;
	int                         batchskeletalweight4ub_bufferoffset;
	int                        *batchelement3i;
	const r_meshbuffer_t       *batchelement3i_indexbuffer;
	int                         batchelement3i_bufferoffset;
	unsigned short             *batchelement3s;
	const r_meshbuffer_t       *batchelement3s_indexbuffer;
	int                         batchelement3s_bufferoffset;
	int                         batchskeletalnumtransforms;
	float                      *batchskeletaltransform3x4;
	const r_meshbuffer_t       *batchskeletaltransform3x4buffer; // uniform buffer
	int                         batchskeletaltransform3x4offset;
	int                         batchskeletaltransform3x4size;

	// some important fields from the entity
	int ent_skinnum;
	int ent_qwskin;
	int ent_flags;
	int ent_alttextures; // used by q1bsp animated textures (pressed buttons)
	double shadertime; // r_refdef.scene.time - ent->shadertime
	// transform matrices to render this entity and effects on this entity
	matrix4x4_t matrix;
	matrix4x4_t inversematrix;
	// scale factors for transforming lengths into/out of entity space
	float matrixscale;
	float inversematrixscale;
	// animation blending state from entity
	frameblend_t frameblend[MAX_FRAMEBLENDS];
	skeleton_t *skeleton;
	// view location in model space
	vec3_t localvieworigin;
	// polygon offset data for submodels
	float basepolygonfactor;
	float basepolygonoffset;
	// current textures in batching code
	texture_t *texture;
	rtexture_t *lightmaptexture;
	rtexture_t *deluxemaptexture;
	// whether lightmapping is active on this batch
	// (otherwise vertex colored)
	qbool uselightmaptexture;
	// fog plane in model space for direct application to vertices
	float fograngerecip;
	float fogmasktabledistmultiplier;
	float fogplane[4];
	float fogheightfade;
	float fogplaneviewdist;

	// rtlight rendering
	// light currently being rendered
	const rtlight_t *rtlight;

	// this is the location of the light in entity space
	vec3_t entitylightorigin;
	// this transforms entity coordinates to light filter cubemap coordinates
	// (also often used for other purposes)
	matrix4x4_t entitytolight;
	// based on entitytolight this transforms -1 to +1 to 0 to 1 for purposes
	// of attenuation texturing in full 3D (Z result often ignored)
	matrix4x4_t entitytoattenuationxyz;
	// this transforms only the Z to S, and T is always 0.5
	matrix4x4_t entitytoattenuationz;

	// user wavefunc parameters (from csqc)
	float userwavefunc_param[Q3WAVEFUNC_USER_COUNT];

	// pointer to an entity_render_t used only by R_GetCurrentTexture and
	// RSurf_ActiveModelEntity as a unique id within each frame (see r_frame
	// also)
	entity_render_t *entity;
}
rsurfacestate_t;

extern rsurfacestate_t rsurface;

void R_HDR_UpdateIrisAdaptation(const vec3_t point);

void RSurf_ActiveModelEntity(const entity_render_t *ent, qbool wantnormals, qbool wanttangents, qbool prepass);
void RSurf_ActiveCustomEntity(const matrix4x4_t *matrix, const matrix4x4_t *inversematrix, int entflags, double shadertime, float r, float g, float b, float a, int numvertices, const float *vertex3f, const float *texcoord2f, const float *normal3f, const float *svector3f, const float *tvector3f, const float *color4f, int numtriangles, const int *element3i, const unsigned short *element3s, qbool wantnormals, qbool wanttangents);
void RSurf_SetupDepthAndCulling(bool ui);

extern int r_textureframe; ///< used only by R_GetCurrentTexture, incremented per view and per UI render
texture_t *R_GetCurrentTexture(texture_t *t);
void R_DrawModelSurfaces(entity_render_t *ent, qbool skysurfaces, qbool writedepth, qbool depthonly, qbool debug, qbool prepass, qbool ui);
void R_DrawCustomSurface(skinframe_t *skinframe, const matrix4x4_t *texmatrix, int materialflags, int firstvertex, int numvertices, int firsttriangle, int numtriangles, qbool writedepth, qbool prepass, qbool ui);
void R_DrawCustomSurface_Texture(texture_t *texture, const matrix4x4_t *texmatrix, int materialflags, int firstvertex, int numvertices, int firsttriangle, int numtriangles, qbool writedepth, qbool prepass, qbool ui);

#define BATCHNEED_ARRAY_VERTEX                (1<< 0) // set up rsurface.batchvertex3f
#define BATCHNEED_ARRAY_NORMAL                (1<< 1) // set up rsurface.batchnormal3f
#define BATCHNEED_ARRAY_VECTOR                (1<< 2) // set up rsurface.batchsvector3f and rsurface.batchtvector3f
#define BATCHNEED_ARRAY_VERTEXTINTCOLOR       (1<< 3) // set up rsurface.batchvertexcolor4f
#define BATCHNEED_ARRAY_VERTEXCOLOR           (1<< 4) // set up rsurface.batchlightmapcolor4f
#define BATCHNEED_ARRAY_TEXCOORD              (1<< 5) // set up rsurface.batchtexcoordtexture2f
#define BATCHNEED_ARRAY_LIGHTMAP              (1<< 6) // set up rsurface.batchtexcoordlightmap2f
#define BATCHNEED_ARRAY_SKELETAL              (1<< 7) // set up skeletal index and weight data for vertex shader
#define BATCHNEED_NOGAPS                      (1<< 8) // force vertex copying if firstvertex is not zero or there are gaps
#define BATCHNEED_ALWAYSCOPY                  (1<< 9) // force vertex copying unconditionally - useful if you want to modify colors
#define BATCHNEED_ALLOWMULTIDRAW              (1<<10) // allow multiple draws
void RSurf_PrepareVerticesForBatch(int batchneed, int texturenumsurfaces, const msurface_t **texturesurfacelist);
void RSurf_UploadBuffersForBatch(void);
void RSurf_DrawBatch(void);

void R_DecalSystem_SplatEntities(const vec3_t org, const vec3_t normal, float r, float g, float b, float a, float s1, float t1, float s2, float t2, float size);

typedef enum rsurfacepass_e
{
	RSURFPASS_BASE,
	RSURFPASS_BACKGROUND,
	RSURFPASS_RTLIGHT,
	RSURFPASS_DEFERREDGEOMETRY
}
rsurfacepass_t;

void R_SetupShader_Generic(rtexture_t *t, qbool usegamma, qbool notrippy, qbool suppresstexalpha);
void R_SetupShader_Generic_NoTexture(qbool usegamma, qbool notrippy);
void R_SetupShader_DepthOrShadow(qbool notrippy, qbool depthrgb, qbool skeletal);
void R_SetupShader_Surface(const float ambientcolor[3], const float diffusecolor[3], const float specularcolor[3], rsurfacepass_t rsurfacepass, int texturenumsurfaces, const msurface_t **texturesurfacelist, void *waterplane, qbool notrippy, qbool ui);
void R_SetupShader_DeferredLight(const rtlight_t *rtlight);

typedef struct r_rendertarget_s {
	// texcoords for sampling from the viewport (clockwise: 0,0 1,0 1,1 0,1)
	float texcoord2f[8];
	// textures are this size and type
	int texturewidth;
	int textureheight;
	// TEXTYPE for each color target - usually TEXTYPE_COLORBUFFER16F
	textype_t colortextype[4];
	// TEXTYPE for depth target - usually TEXTYPE_DEPTHBUFFER24 or TEXTYPE_SHADOWMAP24_COMP
	textype_t depthtextype;
	// if true the depth target will be a renderbuffer rather than a texture (still rtexture_t though)
	qbool depthisrenderbuffer;
	// framebuffer object referencing the textures
	int fbo;
	// there can be up to 4 color targets and 1 depth target, the depthtexture
	// may be a real texture (readable) or just a renderbuffer (not readable,
	// but potentially faster)
	rtexture_t *colortexture[4];
	rtexture_t *depthtexture;
	// a rendertarget will not be reused in the same frame (host.realtime == lastusetime),
	// on a new frame, matching rendertargets will be reused (texturewidth, textureheight, number of color and depth textures and their types),
	// when a new frame arrives the rendertargets can be reused by requests for matching texturewidth,textureheight and fbo configuration (the number of color and depth textures), when a rendertarget is not reused for > 200ms (host.realtime - lastusetime > 0.2) the rendertarget's resources will be freed (fbo, textures) and it can be reused for any target in future frames
	double lastusetime;
} r_rendertarget_t;

// called each frame after render to delete render targets that have not been used for a while
void R_RenderTarget_FreeUnused(qbool force);
// returns a rendertarget, creates rendertarget if needed or intelligently reuses targets across frames if they match and have not been used already this frame
r_rendertarget_t *R_RenderTarget_Get(int texturewidth, int textureheight, textype_t depthtextype, qbool depthisrenderbuffer, textype_t colortextype0, textype_t colortextype1, textype_t colortextype2, textype_t colortextype3);

typedef struct r_waterstate_waterplane_s
{
	r_rendertarget_t *rt_refraction; // MATERIALFLAG_WATERSHADER or MATERIALFLAG_REFRACTION
	r_rendertarget_t *rt_reflection; // MATERIALFLAG_WATERSHADER or MATERIALFLAG_REFLECTION
	r_rendertarget_t *rt_camera; // MATERIALFLAG_CAMERA
	mplane_t plane;
	int materialflags; // combined flags of all water surfaces on this plane
	unsigned char *pvsbits;
	qbool pvsvalid;
	int camera_entity;
	vec3_t mins, maxs;
}
r_waterstate_waterplane_t;

typedef struct r_waterstate_s
{
	int waterwidth, waterheight;
	int texturewidth, textureheight;
	int camerawidth, cameraheight;

	int maxwaterplanes; // same as MAX_WATERPLANES
	int numwaterplanes;
	r_waterstate_waterplane_t waterplanes[MAX_WATERPLANES];

	float screenscale[2];
	float screencenter[2];

	qbool enabled;

	qbool renderingscene; // true while rendering a refraction or reflection texture, disables water surfaces
	qbool hideplayer;
}
r_waterstate_t;

typedef struct r_framebufferstate_s
{
	textype_t textype; // type of color buffer we're using (dependent on r_viewfbo cvar)
	int screentexturewidth, screentextureheight; // dimensions of texture

	// rt_* fields are per-RenderView so we reset them in R_Bloom_StartFrame
	r_rendertarget_t *rt_screen;
	r_rendertarget_t *rt_bloom;

	rtexture_t *ghosttexture; // for r_motionblur (not recommended on multi-GPU hardware!)
	float ghosttexcoord2f[8]; // for r_motionblur

	int bloomwidth, bloomheight;

	// arrays for rendering the screen passes
	float offsettexcoord2f[8]; // temporary use while updating bloomtexture[]

	r_waterstate_t water;

	qbool ghosttexture_valid; // don't draw garbage on first frame with motionblur
	qbool usedepthtextures; // use depth texture instead of depth renderbuffer (faster if you need to read it later anyway)

	// rendertargets (fbo and viewport), these can be reused across frames
	memexpandablearray_t rendertargets;
}
r_framebufferstate_t;

extern r_framebufferstate_t r_fb;

extern cvar_t r_viewfbo;

void R_ResetViewRendering2D_Common(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight, float x2, float y2); // this is called by R_ResetViewRendering2D and _DrawQ_Setup and internal
void R_ResetViewRendering2D(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight);
void R_ResetViewRendering3D(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight);
void R_SetupView(qbool allowwaterclippingplane, int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight);
void R_DebugLine(vec3_t start, vec3_t end);
extern const float r_screenvertex3f[12];
extern cvar_t r_showspriteedges;
extern cvar_t r_showparticleedges;
extern cvar_t r_shadows;
extern cvar_t r_shadows_darken;
extern cvar_t r_shadows_drawafterrtlighting;
extern cvar_t r_shadows_castfrombmodels;
extern cvar_t r_shadows_throwdistance;
extern cvar_t r_shadows_throwdirection;
extern cvar_t r_shadows_focus;
extern cvar_t r_shadows_shadowmapscale;
extern cvar_t r_shadows_shadowmapbias;
extern cvar_t r_transparent_alphatocoverage;
extern cvar_t r_transparent_sortsurfacesbynearest;
extern cvar_t r_transparent_useplanardistance;
extern cvar_t r_transparent_sortarraysize;
extern cvar_t r_transparent_sortmindist;
extern cvar_t r_transparent_sortmaxdist;

extern qbool r_shadow_usingdeferredprepass;
extern rtexture_t *r_shadow_attenuationgradienttexture;
extern rtexture_t *r_shadow_attenuation2dtexture;
extern rtexture_t *r_shadow_attenuation3dtexture;
extern qbool r_shadow_usingshadowmap2d;
extern qbool r_shadow_usingshadowmaportho;
extern float r_shadow_modelshadowmap_texturescale[4];
extern float r_shadow_modelshadowmap_parameters[4];
extern float r_shadow_lightshadowmap_texturescale[4];
extern float r_shadow_lightshadowmap_parameters[4];
extern qbool r_shadow_shadowmapvsdct;
extern rtexture_t *r_shadow_shadowmap2ddepthbuffer;
extern rtexture_t *r_shadow_shadowmap2ddepthtexture;
extern rtexture_t *r_shadow_shadowmapvsdcttexture;
extern matrix4x4_t r_shadow_shadowmapmatrix;
extern int r_shadow_prepass_width;
extern int r_shadow_prepass_height;
extern rtexture_t *r_shadow_prepassgeometrydepthbuffer;
extern rtexture_t *r_shadow_prepassgeometrynormalmaptexture;
extern rtexture_t *r_shadow_prepasslightingdiffusetexture;
extern rtexture_t *r_shadow_prepasslightingspeculartexture;
extern int r_shadow_viewfbo;
extern rtexture_t *r_shadow_viewdepthtexture;
extern rtexture_t *r_shadow_viewcolortexture;
extern int r_shadow_viewx;
extern int r_shadow_viewy;
extern int r_shadow_viewwidth;
extern int r_shadow_viewheight;

void R_RenderScene(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight);
void R_RenderWaterPlanes(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight);

void R_Model_Sprite_Draw(entity_render_t *ent);

struct prvm_prog_s;
void R_UpdateFog(void);
qbool CL_VM_UpdateView(double frametime);
void CL_VM_DrawHud(double frametime);
void SCR_DrawConsole(void);
void R_Shadow_EditLights_DrawSelectedLightProperties(void);
void R_DecalSystem_Reset(decalsystem_t *decalsystem);
void R_Shadow_UpdateBounceGridTexture(void);
void R_DrawPortals(void);
void R_BuildLightMap(const entity_render_t *ent, msurface_t *surface, int combine);
void R_Water_AddWaterPlane(msurface_t *surface, int entno);
int R_Shadow_GetRTLightInfo(unsigned int lightindex, float *origin, float *radius, float *color);
dp_font_t *FindFont(const char *title, qbool allocate_new);
void LoadFont(qbool override, const char *name, dp_font_t *fnt, float scale, float voffset);

void Render_Init(void);

// these are called by Render_Init
void R_Textures_Init(void);
void GL_Draw_Init(void);
void GL_Main_Init(void);
void R_Shadow_Init(void);
void R_Sky_Init(void);
void GL_Surf_Init(void);
void R_Particles_Init(void);
void R_Explosion_Init(void);
void gl_backend_init(void);
void Sbar_Init(void);
void R_LightningBeams_Init(void);
void Mod_RenderInit(void);
void Font_Init(void);

qbool R_CompileShader_CheckStaticParms(void);
void R_GLSL_Restart_f(cmd_state_t *cmd);

#endif
