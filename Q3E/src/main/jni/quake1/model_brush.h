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

#ifndef MODEL_BRUSH_H
#define MODEL_BRUSH_H

#include "qtypes.h"
#include "qdefs.h"
#include "bspfile.h"

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/



//
// in memory representation
//
typedef struct mvertex_s
{
	vec3_t position;
}
mvertex_t;

#define SIDE_FRONT 0
#define SIDE_BACK 1
#define SIDE_ON 2


// plane_t structure
typedef struct mplane_s
{
	union
	{
		struct
		{
			vec3_t normal;
			vec_t dist;
		};
		vec4_t normal_and_dist;
	};
	// for texture axis selection and fast side tests
	int type; // set by PlaneClassify()
	int signbits; // set by PlaneClassify()
}
mplane_t;

#define SHADERSTAGE_SKY 0
#define SHADERSTAGE_NORMAL 1
#define SHADERSTAGE_COUNT 2

//#define SURF_PLANEBACK 2

// indicates that all triangles of the surface should be added to the BIH collision system
#define MATERIALFLAG_MESHCOLLISIONS 0x00000001
// use alpha blend on this material
#define MATERIALFLAG_ALPHA 0x00000002
// use additive blend on this material
#define MATERIALFLAG_ADD 0x00000004
// turn off depth test on this material
#define MATERIALFLAG_NODEPTHTEST 0x00000008
// multiply alpha by r_wateralpha cvar
#define MATERIALFLAG_WATERALPHA 0x00000010
// draw with no lighting
#define MATERIALFLAG_FULLBRIGHT 0x00000020
// drawn as a normal surface (alternative to SKY)
#define MATERIALFLAG_WALL 0x00000040
// this surface shows the sky in its place, alternative to WALL
// skipped if transparent
#define MATERIALFLAG_SKY 0x00000080
// swirling water effect (used with MATERIALFLAG_WALL)
#define MATERIALFLAG_WATERSCROLL 0x00000100
// skips drawing the surface
#define MATERIALFLAG_NODRAW 0x00000200
// probably used only on q1bsp water
#define MATERIALFLAG_LIGHTBOTHSIDES 0x00000400
// use alpha test on this material
#define MATERIALFLAG_ALPHATEST 0x00000800
// treat this material as a blended transparency (as opposed to an alpha test
// transparency), this causes special fog behavior, and disables glDepthMask
#define MATERIALFLAG_BLENDED 0x00001000
// render using a custom blendfunc
#define MATERIALFLAG_CUSTOMBLEND 0x00002000
// do not cast shadows from this material
#define MATERIALFLAG_NOSHADOW 0x00004000
// render using vertex alpha (q3bsp) as texture blend parameter between foreground (normal) skinframe and background skinframe
#define MATERIALFLAG_VERTEXTEXTUREBLEND 0x00008000
// disables GL_CULL_FACE on this texture (making it double sided)
#define MATERIALFLAG_NOCULLFACE 0x00010000
// render with a very short depth range (like 10% of normal), this causes entities to appear infront of most of the scene
#define MATERIALFLAG_SHORTDEPTHRANGE 0x00020000
// render water, comprising refraction and reflection (note: this is always opaque, the shader does the alpha effect)
#define MATERIALFLAG_WATERSHADER 0x00040000
// render refraction (note: this is just a way to distort the background, otherwise useless)
#define MATERIALFLAG_REFRACTION 0x00080000
// render reflection
#define MATERIALFLAG_REFLECTION 0x00100000
// use model lighting on this material (q1bsp lightmap sampling or q3bsp lightgrid, implies FULLBRIGHT is false)
#define MATERIALFLAG_MODELLIGHT 0x00200000
// causes RSurf_GetCurrentTexture to leave alone certain fields
#define MATERIALFLAG_CUSTOMSURFACE 0x00800000
// causes MATERIALFLAG_BLENDED to render a depth pass before rendering, hiding backfaces and other hidden geometry
#define MATERIALFLAG_TRANSDEPTH 0x01000000
// like refraction, but doesn't distort etc.
#define MATERIALFLAG_CAMERA 0x02000000
// disable rtlight on surface - does not disable other types of lighting (LIGHTMAP, MODELLIGHT)
#define MATERIALFLAG_NORTLIGHT 0x04000000
// alphagen vertex - should always be used with MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW (or MATERIALFLAG_ADD instead of MATERIALFLAG_ALPHA)
#define MATERIALFLAG_ALPHAGEN_VERTEX 0x08000000
// use occlusion buffer for corona
#define MATERIALFLAG_OCCLUDE 0x10000000
// use vertex color instead of lighting (e.g. particles and other glowy stuff), use with MATERIALFLAG_FULLBRIGHT
#define MATERIALFLAG_VERTEXCOLOR 0x20000000
// sample the q3bsp lightgrid in the shader rather than relying on MATERIALFLAG_MODELLIGHT
#define MATERIALFLAG_LIGHTGRID 0x40000000
// combined mask of all attributes that require depth sorted rendering
#define MATERIALFLAGMASK_DEPTHSORTED (MATERIALFLAG_BLENDED | MATERIALFLAG_NODEPTHTEST)
// combined mask of all attributes that cause some sort of transparency
#define MATERIALFLAGMASK_TRANSLUCENT (MATERIALFLAG_WATERALPHA | MATERIALFLAG_SKY | MATERIALFLAG_NODRAW | MATERIALFLAG_ALPHATEST | MATERIALFLAG_BLENDED | MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION)

typedef struct medge_s
{
	unsigned int v[2];
}
medge_t;

struct entity_render_s;
struct texture_s;
struct msurface_s;

typedef struct mnode_s
{
	//this part shared between node and leaf
	mplane_t *plane; // != NULL
	struct mnode_s *parent;
	struct mportal_s *portals;
	// for bounding box culling
	vec3_t mins;
	vec3_t maxs;
	// supercontents from all brushes inside this node or leaf
	int combinedsupercontents;

	// this part unique to node
	struct mnode_s *children[2];

	// q1bsp specific
	unsigned int firstsurface;
	unsigned int numsurfaces;
}
mnode_t;

typedef struct mleaf_s
{
	//this part shared between node and leaf
	mplane_t *plane; // == NULL
	struct mnode_s *parent;
	struct mportal_s *portals;
	// for bounding box culling
	vec3_t mins;
	vec3_t maxs;
	// supercontents from all brushes inside this node or leaf
	int combinedsupercontents;

	// this part unique to leaf
	// common
	int clusterindex; // -1 is not in pvs, >= 0 is pvs bit number
	int areaindex; // q3bsp
	int containscollisionsurfaces; // indicates whether the leafsurfaces contains q3 patches
	int numleafsurfaces;
	int *firstleafsurface;
	int numleafbrushes; // q3bsp
	int *firstleafbrush; // q3bsp
	unsigned char ambient_sound_level[NUM_AMBIENTS]; // q1bsp
	int contents; // q1bsp: // TODO: remove (only used temporarily during loading when making collision hull 0)
	int portalmarkid; // q1bsp // used by see-polygon-through-portals visibility checker
}
mleaf_t;

typedef struct mclipnode_s
{
	int			planenum;
	int			children[2];	// negative numbers are contents
} mclipnode_t;

typedef struct hull_s
{
	mclipnode_t *clipnodes;
	mplane_t *planes;
	int firstclipnode;
	int lastclipnode;
	vec3_t clip_mins;
	vec3_t clip_maxs;
	vec3_t clip_size;
}
hull_t;

typedef struct mportal_s
{
	struct mportal_s *next; // the next portal on this leaf
	mleaf_t *here; // the leaf this portal is on
	mleaf_t *past; // the leaf through this portal (infront)
	int numpoints;
	mvertex_t *points;
	vec3_t mins, maxs; // culling
	mplane_t plane;
	double tracetime; // refreshed to realtime by traceline tests
}
mportal_t;

typedef struct svbspmesh_s
{
	struct svbspmesh_s *next;
	int numverts, maxverts;
	int numtriangles, maxtriangles;
	float *verts;
	int *elements;
}
svbspmesh_t;

typedef struct model_brush_lightstyleinfo_s
{
	int style;
	int value;
	int numsurfaces;
	int *surfacelist;
}
model_brush_lightstyleinfo_t;

typedef struct model_brush_s
{
	// true if this model is a HalfLife .bsp file
	qbool ishlbsp;
	// true if this model is a BSP2rmqe .bsp file (expanded 32bit bsp format for rmqe)
	qbool isbsp2rmqe;
	// true if this model is a BSP2 .bsp file (expanded 32bit bsp format for DarkPlaces, others?)
	qbool isbsp2;
	// true if this model is a Quake2 .bsp file (IBSP38)
	qbool isq2bsp;
	// true if this model is a Quake3 .bsp file (IBSP46)
	qbool isq3bsp;
	// true if this model is a Quake1/Quake2 .bsp file where skymasking capability exists
	qbool skymasking;
	// string of entity definitions (.map format)
	char *entities;

	// if not NULL this is a submodel
	struct model_s *parentmodel;
	// (this is the number of the submodel, an index into submodels)
	int submodel;

	// number of submodels in this map (just used by server to know how many
	// submodels to load)
	int numsubmodels;
	// pointers to each of the submodels
	struct model_s **submodels;

	int num_planes;
	mplane_t *data_planes;

	int num_nodes;
	mnode_t *data_nodes;

	// visible leafs, not counting 0 (solid)
	int num_visleafs;
	// number of actual leafs (including 0 which is solid)
	int num_leafs;
	mleaf_t *data_leafs;

	int num_leafbrushes;
	int *data_leafbrushes;

	int num_leafsurfaces;
	int *data_leafsurfaces;

	int num_portals;
	mportal_t *data_portals;

	int num_portalpoints;
	mvertex_t *data_portalpoints;

	int num_brushes;
	struct q3mbrush_s *data_brushes;

	int num_brushsides;
	struct q3mbrushside_s *data_brushsides;

	// pvs
	int num_pvsclusters;
	int num_pvsclusterbytes;
	unsigned char *data_pvsclusters;
	// example
	//pvschain = model->brush.data_pvsclusters + mycluster * model->brush.num_pvsclusterbytes;
	//if (pvschain[thatcluster >> 3] & (1 << (thatcluster & 7)))

	// collision geometry for q3 curves
	int num_collisionvertices;
	int num_collisiontriangles;
	float *data_collisionvertex3f;
	int *data_collisionelement3i;

	// a mesh containing all shadow casting geometry for the whole model (including submodels), portions of this are referenced by each surface's num_firstshadowmeshtriangle
	struct shadowmesh_s *shadowmesh;

	// a mesh containing all SUPERCONTENTS_SOLID surfaces for this model or submodel, for physics engines to use
	struct shadowmesh_s *collisionmesh;

	// common functions
	int (*SuperContentsFromNativeContents)(int nativecontents);
	int (*NativeContentsFromSuperContents)(int supercontents);
	unsigned char *(*GetPVS)(struct model_s *model, const vec3_t p);
	size_t (*FatPVS)(struct model_s *model, const vec3_t org, vec_t radius, unsigned char **pvsbuffer, mempool_t *pool, qbool merge);
	int (*BoxTouchingPVS)(struct model_s *model, const unsigned char *pvs, const vec3_t mins, const vec3_t maxs);
	int (*BoxTouchingLeafPVS)(struct model_s *model, const unsigned char *pvs, const vec3_t mins, const vec3_t maxs);
	int (*BoxTouchingVisibleLeafs)(struct model_s *model, const unsigned char *visibleleafs, const vec3_t mins, const vec3_t maxs);
	int (*FindBoxClusters)(struct model_s *model, const vec3_t mins, const vec3_t maxs, int maxclusters, int *clusterlist);
	void (*LightPoint)(struct model_s *model, const vec3_t p, vec3_t ambientcolor, vec3_t diffusecolor, vec3_t diffusenormal);
	void (*FindNonSolidLocation)(struct model_s *model, const vec3_t in, vec3_t out, vec_t radius);
	mleaf_t *(*PointInLeaf)(struct model_s *model, const vec3_t p);
	// these are actually only found on brushq1, but NULL is handled gracefully
	void (*AmbientSoundLevelsForPoint)(struct model_s *model, const vec3_t p, unsigned char *out, int outsize);
	void (*RoundUpToHullSize)(struct model_s *cmodel, const vec3_t inmins, const vec3_t inmaxs, vec3_t outmins, vec3_t outmaxs);
	// trace a line of sight through this model (returns false if the line if sight is definitely blocked)
	qbool (*TraceLineOfSight)(struct model_s *model, const vec3_t start, const vec3_t end, const vec3_t acceptmins, const vec3_t acceptmaxs);

	char skybox[MAX_QPATH];

	struct skinframe_s *solidskyskinframe;
	struct skinframe_s *alphaskyskinframe;

	qbool supportwateralpha;

	// QuakeWorld
	int qw_md4sum;
	int qw_md4sum2;
}
model_brush_t;

// the first cast is to shut up a stupid warning by clang, the second cast is to make both sides have the same type
#define CHECKPVSBIT(pvs,b) ((b) >= 0 ? (unsigned char) ((pvs)[(b) >> 3] & (1 << ((b) & 7))) : (unsigned char) false)
#define SETPVSBIT(pvs,b) (void) ((b) >= 0 ? (unsigned char) ((pvs)[(b) >> 3] |= (1 << ((b) & 7))) : (unsigned char) false)
#define CLEARPVSBIT(pvs,b) (void) ((b) >= 0 ? (unsigned char) ((pvs)[(b) >> 3] &= ~(1 << ((b) & 7))) : (unsigned char) false)

#endif
