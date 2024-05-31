#ifndef MODEL_Q3BSP_H
#define MODEL_Q3BSP_H

#include "bspfile.h"
#include "r_qshader.h"
#include "r_textures.h"
#include "matrixlib.h"

//Q3 bsp stuff

#define Q3BSPVERSION	46
#define Q3BSPVERSION_LIVE 47
#define Q3BSPVERSION_IG	48

#define	Q3LUMP_ENTITIES		0 // entities to spawn (used by server and client)
#define	Q3LUMP_TEXTURES		1 // textures used (used by faces)
#define	Q3LUMP_PLANES		2 // planes used (used by bsp nodes)
#define	Q3LUMP_NODES		3 // bsp nodes (used by bsp nodes, bsp leafs, rendering, collisions)
#define	Q3LUMP_LEAFS		4 // bsp leafs (used by bsp nodes)
#define	Q3LUMP_LEAFFACES	5 // array of ints indexing faces (used by leafs)
#define	Q3LUMP_LEAFBRUSHES	6 // array of ints indexing brushes (used by leafs)
#define	Q3LUMP_MODELS		7 // models (used by rendering, collisions)
#define	Q3LUMP_BRUSHES		8 // brushes (used by effects, collisions)
#define	Q3LUMP_BRUSHSIDES	9 // brush faces (used by brushes)
#define	Q3LUMP_VERTICES		10 // mesh vertices (used by faces)
#define	Q3LUMP_TRIANGLES	11 // mesh triangles (used by faces)
#define	Q3LUMP_EFFECTS		12 // fog (used by faces)
#define	Q3LUMP_FACES		13 // surfaces (used by leafs)
#define	Q3LUMP_LIGHTMAPS	14 // lightmap textures (used by faces)
#define	Q3LUMP_LIGHTGRID	15 // lighting as a voxel grid (used by rendering)
#define	Q3LUMP_PVS			16 // potentially visible set; bit[clusters][clusters] (used by rendering)
#define	Q3HEADER_LUMPS		17
#define	Q3LUMP_ADVERTISEMENTS 17 // quake live stuff written by zeroradiant's q3map2 (ignored by DP)
#define	Q3HEADER_LUMPS_LIVE	18
#define	Q3HEADER_LUMPS_MAX	18

typedef struct q3dheader_s
{
	int			ident;
	int			version;
	lump_t		lumps[Q3HEADER_LUMPS_MAX];
} q3dheader_t;

typedef struct q3dtexture_s
{
	char name[Q3PATHLENGTH];
	int surfaceflags;
	int contents;
}
q3dtexture_t;

// note: planes are paired, the pair of planes with i and i ^ 1 are opposites.
typedef struct q3dplane_s
{
	float normal[3];
	float dist;
}
q3dplane_t;

typedef struct q3dnode_s
{
	int planeindex;
	int childrenindex[2];
	int mins[3];
	int maxs[3];
}
q3dnode_t;

typedef struct q3dleaf_s
{
	int clusterindex; // pvs index
	int areaindex; // area index
	int mins[3];
	int maxs[3];
	int firstleafface;
	int numleaffaces;
	int firstleafbrush;
	int numleafbrushes;
}
q3dleaf_t;

typedef struct q3dmodel_s
{
	float mins[3];
	float maxs[3];
	int firstface;
	int numfaces;
	int firstbrush;
	int numbrushes;
}
q3dmodel_t;

typedef struct q3dbrush_s
{
	int firstbrushside;
	int numbrushsides;
	int textureindex;
}
q3dbrush_t;

typedef struct q3dbrushside_s
{
	int planeindex;
	int textureindex;
}
q3dbrushside_t;

typedef struct q3dbrushside_ig_s
{
	int planeindex;
	int textureindex;
	int surfaceflags;
}
q3dbrushside_ig_t;

typedef struct q3dvertex_s
{
	float origin3f[3];
	float texcoord2f[2];
	float lightmap2f[2];
	float normal3f[3];
	unsigned char color4ub[4];
}
q3dvertex_t;

typedef struct q3dmeshvertex_s
{
	int offset; // first vertex index of mesh
}
q3dmeshvertex_t;

typedef struct q3deffect_s
{
	char shadername[Q3PATHLENGTH];
	int brushindex;
	int unknown; // I read this is always 5 except in q3dm8 which has one effect with -1
}
q3deffect_t;

#define Q3FACETYPE_FLAT 1 // common
#define Q3FACETYPE_PATCH 2 // common
#define Q3FACETYPE_MESH 3 // common
#define Q3FACETYPE_FLARE 4 // rare (is this ever used?)

typedef struct q3dface_s
{
	int textureindex;
	int effectindex; // -1 if none
	int type; // Q3FACETYPE
	int firstvertex;
	int numvertices;
	int firstelement;
	int numelements;
	int lightmapindex; // -1 if none
	int lightmap_base[2];
	int lightmap_size[2];
	union
	{
		struct
		{
			// corrupt or don't care
			int blah[14];
		}
		unknown;
		struct
		{
			// Q3FACETYPE_FLAT
			// mesh is a collection of triangles on a plane, renderable as a mesh (NOT a polygon)
			float lightmap_origin[3];
			float lightmap_vectors[2][3];
			float normal[3];
			int unused1[2];
		}
		flat;
		struct
		{
			// Q3FACETYPE_PATCH
			// patch renders as a bezier mesh, with adjustable tesselation
			// level (optionally based on LOD using the bbox and polygon
			// count to choose a tesselation level)
			// note: multiple patches may have the same bbox to cause them to
			// be LOD adjusted together as a group
			int unused1[3];
			float mins[3]; // LOD bbox
			float maxs[3]; // LOD bbox
			int unused2[3];
			int patchsize[2]; // dimensions of vertex grid
		}
		patch;
		struct
		{
			// Q3FACETYPE_MESH
			// mesh renders as simply a triangle mesh
			int unused1[3];
			float mins[3];
			float maxs[3];
			int unused2[5];
		}
		mesh;
		struct
		{
			// Q3FACETYPE_FLARE
			// flare renders as a simple sprite at origin, no geometry
			// exists, nor does it have a radius, a cvar controls the radius
			// and another cvar controls distance fade
			// (they were not used in Q3 I'm told)
			float origin[3];
			int unused1[11];
		}
		flare;
	}
	specific;
}
q3dface_t;

typedef struct q3dlightmap_s
{
	unsigned char rgb[128*128*3];
}
q3dlightmap_t;

typedef struct q3dlightgrid_s
{
	unsigned char ambientrgb[3];
	unsigned char diffusergb[3];
	unsigned char diffusepitch;
	unsigned char diffuseyaw;
}
q3dlightgrid_t;

typedef struct q3dpvs_s
{
	int numclusters;
	int chainlength;
	// unsigned char chains[];
	// containing bits in 0-7 order (not 7-0 order),
	// pvschains[mycluster * chainlength + (thatcluster >> 3)] & (1 << (thatcluster & 7))
}
q3dpvs_t;

// surfaceflags from bsp
#define Q3SURFACEFLAG_NODAMAGE 1
#define Q3SURFACEFLAG_SLICK 2
#define Q3SURFACEFLAG_SKY 4
#define Q3SURFACEFLAG_LADDER 8 // has no surfaceparm
#define Q3SURFACEFLAG_NOIMPACT 16
#define Q3SURFACEFLAG_NOMARKS 32
#define Q3SURFACEFLAG_FLESH 64 // has no surfaceparm
#define Q3SURFACEFLAG_NODRAW 128
#define Q3SURFACEFLAG_HINT 256
#define Q3SURFACEFLAG_SKIP 512 // has no surfaceparm
#define Q3SURFACEFLAG_NOLIGHTMAP 1024
#define Q3SURFACEFLAG_POINTLIGHT 2048
#define Q3SURFACEFLAG_METALSTEPS 4096
#define Q3SURFACEFLAG_NOSTEPS 8192 // has no surfaceparm
#define Q3SURFACEFLAG_NONSOLID 16384
#define Q3SURFACEFLAG_LIGHTFILTER 32768
#define Q3SURFACEFLAG_ALPHASHADOW 65536
#define Q3SURFACEFLAG_NODLIGHT 131072
#define Q3SURFACEFLAG_DUST 262144

// surfaceparms from shaders
#define Q3SURFACEPARM_ALPHASHADOW 1
#define Q3SURFACEPARM_AREAPORTAL 2
#define Q3SURFACEPARM_CLUSTERPORTAL 4
#define Q3SURFACEPARM_DETAIL 8
#define Q3SURFACEPARM_DONOTENTER 16
#define Q3SURFACEPARM_FOG 32
#define Q3SURFACEPARM_LAVA 64
#define Q3SURFACEPARM_LIGHTFILTER 128
#define Q3SURFACEPARM_METALSTEPS 256
#define Q3SURFACEPARM_NODAMAGE 512
#define Q3SURFACEPARM_NODLIGHT 1024
#define Q3SURFACEPARM_NODRAW 2048
#define Q3SURFACEPARM_NODROP 4096
#define Q3SURFACEPARM_NOIMPACT 8192
#define Q3SURFACEPARM_NOLIGHTMAP 16384
#define Q3SURFACEPARM_NOMARKS 32768
#define Q3SURFACEPARM_NOMIPMAPS 65536
#define Q3SURFACEPARM_NONSOLID 131072
#define Q3SURFACEPARM_ORIGIN 262144
#define Q3SURFACEPARM_PLAYERCLIP 524288
#define Q3SURFACEPARM_SKY 1048576
#define Q3SURFACEPARM_SLICK 2097152
#define Q3SURFACEPARM_SLIME 4194304
#define Q3SURFACEPARM_STRUCTURAL 8388608
#define Q3SURFACEPARM_TRANS 16777216
#define Q3SURFACEPARM_WATER 33554432
#define Q3SURFACEPARM_POINTLIGHT 67108864
#define Q3SURFACEPARM_HINT 134217728
#define Q3SURFACEPARM_DUST 268435456
#define Q3SURFACEPARM_BOTCLIP 536870912
#define Q3SURFACEPARM_LIGHTGRID 1073741824
#define Q3SURFACEPARM_ANTIPORTAL 2147483648u

typedef struct q3mbrush_s
{
	struct colbrushf_s *colbrushf;
	int numbrushsides;
	struct q3mbrushside_s *firstbrushside;
	struct texture_s *texture;
}
q3mbrush_t;

typedef struct q3mbrushside_s
{
	struct mplane_s *plane;
	struct texture_s *texture;
}
q3mbrushside_t;

typedef struct model_brushq3_s
{
	int num_models;
	q3dmodel_t *data_models;

	// used only during loading - freed after loading!
	int num_vertices;
	float *data_vertex3f;
	float *data_normal3f;
	float *data_texcoordtexture2f;
	float *data_texcoordlightmap2f;
	float *data_color4f;

	// freed after loading!
	int num_triangles;
	int *data_element3i;

	int num_effects;
	q3deffect_t *data_effects;

	// lightmap textures
	int num_originallightmaps;
	int num_mergedlightmaps;
	int num_lightmapmergedwidthpower;
	int num_lightmapmergedheightpower;
	int num_lightmapmergedwidthheightdeluxepower;
	int num_lightmapmerge;
	rtexture_t **data_lightmaps;
	rtexture_t **data_deluxemaps;

	// voxel light data with directional shading - data for cpu sampling of it...
	int num_lightgrid;
	q3dlightgrid_t *data_lightgrid;
	// size of each cell (may vary by map, typically 64 64 128)
	float num_lightgrid_cellsize[3];
	// 1.0 / num_lightgrid_cellsize
	float num_lightgrid_scale[3];
	// dimensions of the world model in lightgrid cells
	int num_lightgrid_imins[3];
	int num_lightgrid_imaxs[3];
	int num_lightgrid_isize[3];
	// transform modelspace coordinates to lightgrid index
	matrix4x4_t num_lightgrid_indexfromworld;
	// parameters for fragment shader to sample the texture version of it:
	int lightgridtexturesize[3]; // 3 layers tall (ambient, lightcolor, lightdir)
	matrix4x4_t lightgridworldtotexturematrix;
	rtexture_t *lightgridtexture;

	// true if this q3bsp file has been detected as using deluxemapping
	// (lightmap texture pairs, every odd one is never directly refernced,
	//  and contains lighting normals, not colors)
	qbool deluxemapping;
	// true if the detected deluxemaps are the modelspace kind, rather than
	// the faster tangentspace kind
	qbool deluxemapping_modelspace;
	// size of lightmaps (128 by default, but may be another poweroftwo if
	// external lightmaps are used (q3map2 -lightmapsize)
	int lightmapsize;
}
model_brushq3_t;

#endif
