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

#ifndef BSPFILE_H
#define BSPFILE_H

#define MAX_MAP_HULLS 16 // Q1BSP has 4, Hexen2 Q1BSP has 8, MCBSP has 16

//=============================================================================


#define BSPVERSION	29
#define HLBSPVERSION 30
#define _2PSBVERSION ('2' + 'P' * 256 + 'S' * 65536 + 'B' * 16777216)
#define BSP2VERSION ('B' + 'S' * 256 + 'P' * 65536 + '2' * 16777216)

typedef struct lump_s
{
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES	0
#define	LUMP_PLANES		1
#define	LUMP_TEXTURES	2
#define	LUMP_VERTEXES	3
#define	LUMP_VISIBILITY	4
#define	LUMP_NODES		5
#define	LUMP_TEXINFO	6
#define	LUMP_FACES		7
#define	LUMP_LIGHTING	8
#define	LUMP_CLIPNODES	9
#define	LUMP_LEAFS		10
#define	LUMP_MARKSURFACES 11
#define	LUMP_EDGES		12
#define	LUMP_SURFEDGES	13
#define	LUMP_MODELS		14
#define	HEADER_LUMPS	15

typedef struct hullinfo_s
{
	int			filehulls;
	float		hullsizes[MAX_MAP_HULLS][2][3];
} hullinfo_t;

typedef struct mmodel_s
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLS];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} mmodel_t;

/*
// WARNING: this struct does NOT match q1bsp's disk format because MAX_MAP_HULLS has been changed by Sajt's MCBSP code, this struct is only being used in memory as a result
typedef struct dmodel_s
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLS];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} dmodel_t;

typedef struct dheader_s
{
	int			version;
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct dmiptexlump_s
{
	int			nummiptex;
	int			dataofs[4];		// [nummiptex]
} dmiptexlump_t;
*/

#define	MIPLEVELS	4
/*
typedef struct miptex_s
{
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} miptex_t;


typedef struct dvertex_s
{
	float	point[3];
} dvertex_t;
*/


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

/*
typedef struct dplane_s
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;
*/


// contents values in Q1 maps
#define CONTENTS_EMPTY			-1
#define CONTENTS_SOLID			-2
#define CONTENTS_WATER			-3
#define CONTENTS_SLIME			-4
#define CONTENTS_LAVA				-5
#define CONTENTS_SKY				-6
// these were #ifdef QUAKE2 in the quake source
#define CONTENTS_ORIGIN			-7 // removed at csg time
#define CONTENTS_CLIP				-8 // changed to contents_solid
#define CONTENTS_CURRENT_0		-9
#define CONTENTS_CURRENT_90		-10
#define CONTENTS_CURRENT_180		-11
#define CONTENTS_CURRENT_270		-12
#define CONTENTS_CURRENT_UP		-13
#define CONTENTS_CURRENT_DOWN		-14

//contents flags in Q2 maps
#define CONTENTSQ2_SOLID			0x00000001 // an eye is never valid in a solid
#define CONTENTSQ2_WINDOW			0x00000002 // translucent, but not watery
#define CONTENTSQ2_AUX				0x00000004
#define CONTENTSQ2_LAVA				0x00000008
#define CONTENTSQ2_SLIME			0x00000010
#define CONTENTSQ2_WATER			0x00000020
#define CONTENTSQ2_MIST				0x00000040
#define CONTENTSQ2_AREAPORTAL		0x00008000
#define CONTENTSQ2_PLAYERCLIP		0x00010000
#define CONTENTSQ2_MONSTERCLIP		0x00020000
#define CONTENTSQ2_CURRENT_0		0x00040000
#define CONTENTSQ2_CURRENT_90		0x00080000
#define CONTENTSQ2_CURRENT_180		0x00100000
#define CONTENTSQ2_CURRENT_270		0x00200000
#define CONTENTSQ2_CURRENT_UP		0x00400000
#define CONTENTSQ2_CURRENT_DOWN		0x00800000
#define CONTENTSQ2_ORIGIN			0x01000000 // removed before bsping an entity
#define CONTENTSQ2_MONSTER			0x02000000 // should never be on a brush, only in game
#define CONTENTSQ2_DEADMONSTER		0x04000000
#define CONTENTSQ2_DETAIL			0x08000000 // brushes to be added after vis leafs
#define CONTENTSQ2_TRANSLUCENT		0x10000000 // auto set if any surface has trans
#define CONTENTSQ2_LADDER			0x20000000

//contents flags in Q3 maps
#define CONTENTSQ3_SOLID			0x00000001 // solid (opaque and transparent)
#define CONTENTSQ3_LAVA				0x00000008 // lava
#define CONTENTSQ3_SLIME			0x00000010 // slime
#define CONTENTSQ3_WATER			0x00000020 // water
#define CONTENTSQ3_FOG				0x00000040 // unused?
#define CONTENTSQ3_AREAPORTAL		0x00008000 // areaportal (separates areas)
#define CONTENTSQ3_PLAYERCLIP		0x00010000 // block players
#define CONTENTSQ3_MONSTERCLIP		0x00020000 // block monsters
#define CONTENTSQ3_TELEPORTER		0x00040000 // hint for Q3's bots
#define CONTENTSQ3_JUMPPAD			0x00080000 // hint for Q3's bots
#define CONTENTSQ3_CLUSTERPORTAL	0x00100000 // hint for Q3's bots
#define CONTENTSQ3_DONOTENTER		0x00200000 // hint for Q3's bots
#define CONTENTSQ3_BOTCLIP			0x00400000 // hint for Q3's bots
#define CONTENTSQ3_ORIGIN			0x01000000 // used by origin brushes to indicate origin of bmodel (removed by map compiler)
#define CONTENTSQ3_BODY				0x02000000 // used by bbox entities (should never be on a brush)
#define CONTENTSQ3_CORPSE			0x04000000 // used by dead bodies (SOLID_CORPSE in darkplaces)
#define CONTENTSQ3_DETAIL			0x08000000 // brushes that do not split the bsp tree (decorations)
#define CONTENTSQ3_STRUCTURAL		0x10000000 // brushes that split the bsp tree
#define CONTENTSQ3_TRANSLUCENT		0x20000000 // leaves surfaces that are inside for rendering
#define CONTENTSQ3_TRIGGER			0x40000000 // used by trigger entities
#define CONTENTSQ3_NODROP			0x80000000 // remove items that fall into this brush

#define SUPERCONTENTS_SOLID			0x00000001
#define SUPERCONTENTS_WATER			0x00000002
#define SUPERCONTENTS_SLIME			0x00000004
#define SUPERCONTENTS_LAVA			0x00000008
#define SUPERCONTENTS_SKY			0x00000010
#define SUPERCONTENTS_BODY			0x00000020
#define SUPERCONTENTS_CORPSE		0x00000040
#define SUPERCONTENTS_NODROP		0x00000080
#define SUPERCONTENTS_PLAYERCLIP	0x00000100
#define SUPERCONTENTS_MONSTERCLIP	0x00000200
#define SUPERCONTENTS_DONOTENTER	0x00000400
#define SUPERCONTENTS_BOTCLIP		0x00000800
#define SUPERCONTENTS_OPAQUE		0x00001000
// TODO: is there any reason to define:
//   fog?
//   areaportal?
//   teleporter?
//   jumppad?
//   clusterportal?
//   detail?         (div0) no, game code should not be allowed to differentiate between structural and detail
//   structural?     (div0) no, game code should not be allowed to differentiate between structural and detail
//   trigger?        (div0) no, as these are always solid anyway, and that's all that matters for trigger brushes
#define SUPERCONTENTS_LIQUIDSMASK	(SUPERCONTENTS_LAVA | SUPERCONTENTS_SLIME | SUPERCONTENTS_WATER)
#define SUPERCONTENTS_VISBLOCKERMASK	SUPERCONTENTS_OPAQUE

/*
#define SUPERCONTENTS_DEADMONSTER	0x00000000
#define SUPERCONTENTS_CURRENT_0		0x00000000
#define SUPERCONTENTS_CURRENT_90	0x00000000
#define SUPERCONTENTS_CURRENT_180	0x00000000
#define SUPERCONTENTS_CURRENT_270	0x00000000
#define SUPERCONTENTS_CURRENT_DOWN	0x00000000
#define SUPERCONTENTS_CURRENT_UP	0x00000000
#define SUPERCONTENTS_AREAPORTAL	0x00000000
#define SUPERCONTENTS_AUX			0x00000000
#define SUPERCONTENTS_CLUSTERPORTAL	0x00000000
#define SUPERCONTENTS_DETAIL		0x00000000
#define SUPERCONTENTS_STRUCTURAL	0x00000000
#define SUPERCONTENTS_DONOTENTER	0x00000000
#define SUPERCONTENTS_JUMPPAD		0x00000000
#define SUPERCONTENTS_LADDER		0x00000000
#define SUPERCONTENTS_MONSTER		0x00000000
#define SUPERCONTENTS_MONSTERCLIP	0x00000000
#define SUPERCONTENTS_PLAYERCLIP	0x00000000
#define SUPERCONTENTS_TELEPORTER	0x00000000
#define SUPERCONTENTS_TRANSLUCENT	0x00000000
#define SUPERCONTENTS_TRIGGER		0x00000000
#define SUPERCONTENTS_WINDOW		0x00000000
*/

/*
typedef struct dnode_s
{
	int			planenum;
	short		children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dnode_t;

typedef struct dclipnode_s
{
	int			planenum;
	short		children[2];	// negative numbers are contents
} dclipnode_t;


typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			miptex;
	int			flags;
} texinfo_t;
*/
#define	TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
/*
typedef struct dedge_s
{
	unsigned short	v[2];		// vertex numbers
} dedge_t;
*/

#define	MAXLIGHTMAPS	4
/*
typedef struct dface_s
{
	// LadyHavoc: changed from short to unsigned short for q2 support
	unsigned short	planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;
	short		texinfo;

// lighting info
	unsigned char		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dface_t;
*/



#define	AMBIENT_WATER	0
#define	AMBIENT_SKY		1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS			4		// automatic ambient sounds

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
/*
typedef struct dleaf_s
{
	int			contents;
	int			visofs;				// -1 = no visibility info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned short		firstmarksurface;
	unsigned short		nummarksurfaces;

	unsigned char		ambient_level[NUM_AMBIENTS];
} dleaf_t;
*/

#endif
