#ifndef MODEL_Q2BSP_H
#define MODEL_Q2BSP_H

#include "bspfile.h"

#define Q2BSPMAGIC ('I' + 'B' * 256 + 'S' * 65536 + 'P' * 16777216)
#define Q2BSPVERSION	38

// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits

//=============================================================================

#define	Q2LUMP_ENTITIES		0
#define	Q2LUMP_PLANES			1
#define	Q2LUMP_VERTEXES		2
#define	Q2LUMP_VISIBILITY		3
#define	Q2LUMP_NODES			4
#define	Q2LUMP_TEXINFO		5
#define	Q2LUMP_FACES			6
#define	Q2LUMP_LIGHTING		7
#define	Q2LUMP_LEAFS			8
#define	Q2LUMP_LEAFFACES		9
#define	Q2LUMP_LEAFBRUSHES	10
#define	Q2LUMP_EDGES			11
#define	Q2LUMP_SURFEDGES		12
#define	Q2LUMP_MODELS			13
#define	Q2LUMP_BRUSHES		14
#define	Q2LUMP_BRUSHSIDES		15
#define	Q2LUMP_POP			16
#define	Q2LUMP_AREAS			17
#define	Q2LUMP_AREAPORTALS	18
#define	Q2HEADER_LUMPS		19

typedef struct q2dheader_s
{
	int			ident;
	int			version;
	lump_t		lumps[Q2HEADER_LUMPS];
} q2dheader_t;

typedef struct q2dmodel_s
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces
										// without walking the bsp tree
} q2dmodel_t;

// planes (x&~1) and (x&~1)+1 are always opposites

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in q_shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	Q2CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	Q2CONTENTS_WINDOW			2		// translucent, but not watery
#define	Q2CONTENTS_AUX			4
#define	Q2CONTENTS_LAVA			8
#define	Q2CONTENTS_SLIME			16
#define	Q2CONTENTS_WATER			32
#define	Q2CONTENTS_MIST			64
#define	Q2LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes

#define	Q2CONTENTS_AREAPORTAL		0x8000

#define	Q2CONTENTS_PLAYERCLIP		0x10000
#define	Q2CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	Q2CONTENTS_CURRENT_0		0x40000
#define	Q2CONTENTS_CURRENT_90		0x80000
#define	Q2CONTENTS_CURRENT_180	0x100000
#define	Q2CONTENTS_CURRENT_270	0x200000
#define	Q2CONTENTS_CURRENT_UP		0x400000
#define	Q2CONTENTS_CURRENT_DOWN	0x800000

#define	Q2CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	Q2CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	Q2CONTENTS_DEADMONSTER	0x4000000
#define	Q2CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	Q2CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	Q2CONTENTS_LADDER			0x20000000



#define	Q2SURF_LIGHT		0x1		// value will hold the light strength

#define	Q2SURF_SLICK		0x2		// effects game physics

#define	Q2SURF_SKY		0x4		// don't draw, but add to skybox
#define	Q2SURF_WARP		0x8		// turbulent water warp
#define	Q2SURF_TRANS33	0x10
#define	Q2SURF_TRANS66	0x20
#define	Q2SURF_FLOWING	0x40	// scroll towards angle
#define	Q2SURF_NODRAW		0x80	// don't bother referencing the texture

#define Q2SURF_HINT		0x100   // make a primary bsp splitter
#define Q2SURF_SKIP		0x200   // completely ignore, allowing non-closed brushes

#define Q2SURF_ALPHATEST 0x02000000	// alpha test masking of color 255 in wal textures (supported by modded engines)


/*
typedef struct q2dnode_s
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} q2dnode_t;

typedef struct q2texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
	int			value;			// light emission, etc
	char		texture[32];	// texture name (textures/something.wal)
	int			nexttexinfo;	// for animations, -1 = end of chain
} q2texinfo_t;

typedef struct q2dleaf_s
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
} q2dleaf_t;

typedef struct q2dbrushside_s
{
	unsigned short	planenum;		// facing out of the leaf
	short	texinfo;
} q2dbrushside_t;

typedef struct q2dbrush_s
{
	int			firstside;
	int			numsides;
	int			contents;
} q2dbrush_t;


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	Q2DVIS_PVS	0
#define	Q2DVIS_PHS	1
typedef struct q2dvis_s
{
	int			numclusters;
	int			bitofs[8][2];	// bitofs[numclusters][2]
} q2dvis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct q2dareaportal_s
{
	int		portalnum;
	int		otherarea;
} q2dareaportal_t;

typedef struct q2darea_s
{
	int		numareaportals;
	int		firstareaportal;
} q2darea_t;
*/

#endif
