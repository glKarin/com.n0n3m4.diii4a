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


// upper design bounds

#define	MAX_MAP_HULLSDQ1	4
#define	MAX_MAP_HULLSDH2	8
#define	MAX_MAP_HULLSM		8
#define RBSP_STYLESPERSURF		4
#define Q1Q2BSP_STYLESPERSURF	4
#ifdef Q1BSPS
#define MAXCPULIGHTMAPS		16	//max lightmaps mixed by the cpu (vanilla q1bsp=4, fte extensions=no real cap, must be >=MAXRLIGHTMAPS)
#elif defined(Q1BSPS)
#define MAXCPULIGHTMAPS		Q1Q2BSP_STYLESPERSURF	//max lightmaps mixed by the cpu (vanilla q1bsp=4, fte extensions=no real cap, must be >=MAXRLIGHTMAPS)
#else
#define MAXCPULIGHTMAPS		MAXRLIGHTMAPS	//max lightmaps mixed by the cpu (vanilla q1bsp=4, fte extensions=no real cap, must be >=MAXRLIGHTMAPS)
#endif

//#define	MAX_MAP_MODELS		256
//#define	MAX_MAP_BRUSHES		0x8000
//#define	MAX_MAP_ENTITIES	1024
//#define	MAX_MAP_ENTSTRING	65536

//FIXME: make sure that any 16bit indexes are bounded properly
//FIXME: ensure that we don't get any count*size overflows
#define	SANITY_LIMIT(t)		((unsigned int)(0x7fffffffu/sizeof(t)))		//sanity limit for the array, to ensure a 32bit value cannot overflow us.
//#define	SANITY_MAX_MAP_PLANES		65536*64		//sanity
//#define	SANITY_MAX_MAP_NODES		65536*64		//sanity
//#define	SANITY_MAX_MAP_CLIPNODES	65536*64		//sanity
//#define	MAX_MAP_LEAFS				1		//pvs buffer size. not sanity.
//#define	SANITY_MAX_MAP_LEAFS		65536*64		//too many leafs results in massive amounts of ram used for pvs/phs caches.
//#define	SANITY_MAX_MAP_VERTS		65536		//sanity
//#define	SANITY_MAX_MAP_FACES		65536*64		//sanity
//#define	MAX_MAP_MARKSURFACES 65536	//sanity
//#define	MAX_MAP_TEXINFO		4096	//sanity
//#define	MAX_MAP_EDGES		256000
//#define	MAX_MAP_SURFEDGES	512000
//#define	MAX_MAP_MIPTEX		0x200000
//#define	MAX_MAP_LIGHTING	0x100000
//#define	MAX_MAP_VISIBILITY	0x200000

#define	SANITY_MAX_MAP_BRUSHSIDES	((~0u)/sizeof(q2cbrushside_t))

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024


//=============================================================================

#define BSPVERSIONQTEST		"\x17\0\0\0",4	//qtest
#define BSPVERSIONPREREL	"\x1C\0\0\0",4	//prerelease
#define BSPVERSION			"\x1D\0\0\0",4	//vanilla
#define BSPVERSIONHL		"\x1E\0\0\0",4	//HalfLife support
#define BSPVERSION_LONG1	"2PSB",4 /*RMQ support (2PSB). 32bits instead of shorts for all but bbox sizes*/
#define BSPVERSION_LONG2	"BSP2",4 /*BSP2 support. 32bits instead of shorts for everything*/
#define BSPVERSIONQ64 		" 46Q",4 /* Remastered BSP format used for Quake 64 addon */

typedef struct
{
	unsigned int		fileofs, filelen;
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

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLSDQ1];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} dq1model_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLSDH2];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} dh2model_t;

typedef struct
{
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
	int			nummiptex;
	int			dataofs[4];		// [nummiptex]
} dmiptexlump_t;

#define	MIPLEVELS	4
typedef struct miptex_s
{
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} miptex_t;

typedef struct q64miptex_s
{
	char		name[16];
	unsigned	width, height, scale;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} q64miptex_t;


typedef struct
{
	float	point[3];
} dvertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


enum q1contents_e
{	//q1 and halflife bsp contents values.
	//also used for .skin for content forcing.
	Q1CONTENTS_EMPTY		= -1,
	Q1CONTENTS_SOLID		= -2,
	Q1CONTENTS_WATER		= -3,
	Q1CONTENTS_SLIME		= -4,
	Q1CONTENTS_LAVA			= -5,
	Q1CONTENTS_SKY			= -6,
//#define HLCONTENTS_ORIGIN	  -7	/*not known to engine - origin or something*/
	HLCONTENTS_CLIP			= -8,	/*solid to players+monsters, but not tracelines*/
	HLCONTENTS_CURRENT_0	= -9,	/*moves player*/
	HLCONTENTS_CURRENT_90	= -10,	/*moves player*/
	HLCONTENTS_CURRENT_180	= -11,	/*moves player*/
	HLCONTENTS_CURRENT_270	= -12,	/*moves player*/
	HLCONTENTS_CURRENT_UP	= -13,	/*moves player*/
	HLCONTENTS_CURRENT_DOWN	= -14,	/*moves player*/
	HLCONTENTS_TRANS		= -15,	/*empty, but blocks pvs (for opaque non-solid windows, like vanilla-q1 water)*/
	Q1CONTENTS_LADDER		= -16,	/*player can climb up/down*/
	Q1CONTENTS_MONSTERCLIP	= -17,	/*solid to monster movement*/
	Q1CONTENTS_PLAYERCLIP	= -18,	/*solid to player movement*/
	Q1CONTENTS_CORPSE		= -19,	/*solid to tracelines but not boxes*/
};

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	int			planenum;
	short		children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dsnode_t;
typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned int	firstface;
	unsigned int	numfaces;	// counting both sides
} dl1node_t;
typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	float		mins[3];		// for sphere culling
	float		maxs[3];
	unsigned int	firstface;
	unsigned int	numfaces;	// counting both sides
} dl2node_t;

typedef struct
{
	int			planenum;
	short		children[2];	// negative numbers are contents
} dsclipnode_t;
typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are contents
} dlclipnode_t;

typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are contents
} mclipnode_t;

typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			miptex;
	int			flags;
} texinfo_t;
#define	TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dsedge_t;
typedef struct
{
	unsigned int	v[2];		// vertex numbers
} dledge_t;

#ifdef RFBSPS
#define	MAXRLIGHTMAPS	4	//max lightmaps mixed by the gpu (rbsp=4, otherwise 1)
#else
#define	MAXRLIGHTMAPS	1	//max lightmaps mixed by the gpu (rbsp=4, otherwise 1)
#endif
typedef struct
{
	short		planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	qbyte		styles[Q1Q2BSP_STYLESPERSURF];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dsface_t;
typedef struct
{
	int			planenum;
	int			side;

	int			firstedge;		// we must support > 64k edges
	int			numedges;	
	int			texinfo;

// lighting info
	qbyte		styles[Q1Q2BSP_STYLESPERSURF];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dlface_t;



#define	AMBIENT_WATER	0
#define	AMBIENT_SKY		1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS			4		// automatic ambient sounds

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int			contents;
	int			visofs;				// -1 = no visibility info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned short		firstmarksurface;
	unsigned short		nummarksurfaces;

	qbyte		ambient_level[NUM_AMBIENTS];
} dsleaf_t;
typedef struct
{
	int			contents;
	int			visofs;				// -1 = no visibility info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned int		firstmarksurface;
	unsigned int		nummarksurfaces;

	qbyte		ambient_level[NUM_AMBIENTS];
} dl1leaf_t;
typedef struct
{
	int			contents;
	int			visofs;				// -1 = no visibility info

	float		mins[3];			// for frustum culling
	float		maxs[3];

	unsigned int		firstmarksurface;
	unsigned int		nummarksurfaces;

	qbyte		ambient_level[NUM_AMBIENTS];
} dl2leaf_t;

//============================================================================















#define	MIPLEVELS	4
typedef struct q2miptex_s
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} q2miptex_t;



/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define IDBSPHEADER	"IBSP",4
		// little-endian "IBSP"

#define BSPVERSION_Q2	38
#define BSPVERSION_Q2W	69 
#define BSPVERSION_Q3	46
#define BSPVERSION_RTCW	47
#define BSPVERSION_RBSP 1	//also fbsp(just bigger internal lightmaps)



// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	SANITY_MAX_Q2MAP_MODELS		MAX_PRECACHE_MODELS
//#define	MAX_Q2MAP_ENTITIES	2048
#define SANITY_MAX_MAP_BRUSHES (~0u/sizeof(*out))
#define	SANITY_MAX_MAP_LEAFFACES	262144		//sanity only

#define	MAX_Q2MAP_AREAS		(MAX_MAP_AREA_BYTES*8)
#define	MAX_Q2MAP_AREAPORTALS	1024
//#define	MAX_Q2MAP_VERTS		MAX_MAP_VERTS
//#define	MAX_Q2MAP_FACES		MAX_MAP_FACES
#define	SANITY_MAX_MAP_LEAFBRUSHES (65536*64)		//used in an array
//#define	MAX_Q2MAP_PORTALS		65536	//unused
//#define	MAX_Q2MAP_EDGES		128000		//unused
//#define	MAX_Q2MAP_SURFEDGES	256000		//unused
//#define	MAX_Q2MAP_LIGHTING	0x200000	//unused
//#define	MAX_Q2MAP_VISIBILITY	MAX_MAP_VISIBILITY

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

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

enum Q3LUMP
{	
	Q3LUMP_ENTITIES		=0,
	Q3LUMP_SHADERS		=1,
	Q3LUMP_PLANES		=2,
	Q3LUMP_NODES		=3,
	Q3LUMP_LEAFS		=4,
	Q3LUMP_LEAFSURFACES	=5,
	Q3LUMP_LEAFBRUSHES	=6,
	Q3LUMP_MODELS		=7,
	Q3LUMP_BRUSHES		=8,
	Q3LUMP_BRUSHSIDES	=9,
	Q3LUMP_DRAWVERTS	=10,
	Q3LUMP_DRAWINDEXES	=11,
	Q3LUMP_FOGS			=12,
	Q3LUMP_SURFACES		=13,
	Q3LUMP_LIGHTMAPS	=14,
	Q3LUMP_LIGHTGRID	=15,
	Q3LUMP_VISIBILITY	=16,
#ifdef RFBSPS
	RBSPLUMP_LIGHTINDEXES=17,
#endif
	Q3LUMPS_TOTAL
};

typedef struct
{
	int ident;
	int			version;	
	lump_t		lumps[50];
} q2dheader_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces
										// without walking the bsp tree
} q2dmodel_t;

typedef struct
{
	float mins[3];
	float maxs[3];
	int firstsurface;
	int num_surfaces;
	int firstbrush;
	int num_brushes;
} q3dmodel_t;



// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5




// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

#define	FTECONTENTS_EMPTY			0x00000000
#define	FTECONTENTS_SOLID			0x00000001
#define FTECONTENTS_WINDOW			0x00000002	//solid to bullets, but not sight/agro
//q2aux								0x00000004
#define	FTECONTENTS_LAVA			0x00000008
#define	FTECONTENTS_SLIME			0x00000010
#define	FTECONTENTS_WATER			0x00000020
#define FTECONTENTS_FLUID			(FTECONTENTS_WATER|FTECONTENTS_SLIME|FTECONTENTS_LAVA|FTECONTENTS_SKY)	//sky is a fluid for q1 code.
//q2mist							0x00000040
//q3notteam1						0x00000080
//q3notteam2						0x00000100
//q3nobotclip						0x00000200
//									0x00000400
//									0x00000800
//									0x00001000
//									0x00002000
#define FTECONTENTS_LADDER			0x00004000
//q2areaportal,q3areaportal			0x00008000
#define FTECONTENTS_PLAYERCLIP		0x00010000
#define FTECONTENTS_MONSTERCLIP		0x00020000
//q2current0,q3teleporter			0x00040000
//q2current90,q3jumppad				0x00080000
//q2current180,q3clusterportal		0x00100000
//q2current270,q3donotenter			0x00200000
//q2currentup,q3botclip				0x00400000
//q2currentdown,q3mover				0x00800000
//q2origin,q3origin					0x01000000	//could define, but normally removed by compiler, so why?
#define FTECONTENTS_BODY			0x02000000
#define FTECONTENTS_CORPSE			0x04000000
#define FTECONTENTS_DETAIL			0x08000000	//not very useful to us, but used by .map support
//q2translucent,q3structual			0x10000000
//q2ladder,q3translucent			0x20000000
//q3trigger							0x40000000
#define	FTECONTENTS_SKY/*q3nodrop*/	0x80000000

// lower bits are stronger, and will eat weaker brushes completely
#define	Q2CONTENTS_SOLID		FTECONTENTS_SOLID		//0x00000001
#define	Q2CONTENTS_WINDOW								  0x00000002		// translucent, but not watery
#define	Q2CONTENTS_AUX									  0x00000004
#define	Q2CONTENTS_LAVA			FTECONTENTS_LAVA		//0x00000008
#define	Q2CONTENTS_SLIME		FTECONTENTS_SLIME		//0x00000010
#define	Q2CONTENTS_WATER		FTECONTENTS_WATER		//0x00000020
#define	Q2CONTENTS_MIST									  0x00000040
														//0x00000080
														//0x00000100
														//0x00000200
														//0x00000400
														//0x00000800
														//0x00001000
														//0x00002000
								//FTECONTENTS_LADDER	//0x00004000
// remaining contents are non-visible, and don't eat brushes
#define	Q2CONTENTS_AREAPORTAL							  0x00008000
#define	Q2CONTENTS_PLAYERCLIP	FTECONTENTS_PLAYERCLIP	//0x00010000
#define	Q2CONTENTS_MONSTERCLIP	FTECONTENTS_MONSTERCLIP	//0x00020000
// currents can be added to any other contents, and may be mixed
#define	Q2CONTENTS_CURRENT_0							  0x00040000
#define	Q2CONTENTS_CURRENT_90							  0x00080000
#define	Q2CONTENTS_CURRENT_180							  0x00100000
#define	Q2CONTENTS_CURRENT_270							  0x00200000
#define	Q2CONTENTS_CURRENT_UP							  0x00400000
#define	Q2CONTENTS_CURRENT_DOWN							  0x00800000
#define	Q2CONTENTS_ORIGIN								  0x01000000	// removed before bsping an entity
#define	Q2CONTENTS_MONSTER		FTECONTENTS_BODY		//0x02000000	// should never be on a brush, only in game
#define	Q2CONTENTS_DEADMONSTER	FTECONTENTS_CORPSE		//0x04000000
#define	Q2CONTENTS_DETAIL		FTECONTENTS_DETAIL		// 0x08000000	// brushes to be added after vis leafs
#define	Q2CONTENTS_TRANSLUCENT							  0x10000000	// auto set if any surface has trans
#define	Q2CONTENTS_LADDER								  0x20000000
														//0x40000000
								//FTECONTENTS_SKY		//0x80000000


#define	Q3CONTENTS_SOLID		FTECONTENTS_SOLID		//0x00000001	// should never be on a brush, only in game
														//0x00000002
														//0x00000004
#define	Q3CONTENTS_LAVA			FTECONTENTS_LAVA		//0x00000008
#define	Q3CONTENTS_SLIME		FTECONTENTS_SLIME		//0x00000010
#define	Q3CONTENTS_WATER		FTECONTENTS_WATER		//0x00000020
														//0x00000040
#define Q3CONTENTS_NOTTEAM1								  0x00000080
#define Q3CONTENTS_NOTTEAM2								  0x00000100
#define Q3CONTENTS_NOBOTCLIP							  0x00000200
														//0x00000400
														//0x00000800
														//0x00001000
														//0x00002000
								//FTECONTENTS_LADDER	//0x00004000
#define Q3CONTENTS_AREAPORTAL							  0x00008000
#define	Q3CONTENTS_PLAYERCLIP	FTECONTENTS_PLAYERCLIP	//0x00010000
#define	Q3CONTENTS_MONSTERCLIP	FTECONTENTS_MONSTERCLIP	//0x00020000
#define	Q3CONTENTS_TELEPORTER							  0x00040000
#define	Q3CONTENTS_JUMPPAD								  0x00080000
#define Q3CONTENTS_CLUSTERPORTAL						  0x00100000
#define Q3CONTENTS_DONOTENTER							  0x00200000
#define Q3CONTENTS_BOTCLIP								  0x00400000
#define Q3CONTENTS_MOVER								  0x00800000
#define	Q3CONTENTS_ORIGIN		Q2CONTENTS_ORIGIN		//0x01000000
#define	Q3CONTENTS_BODY			FTECONTENTS_BODY		//0x02000000
#define	Q3CONTENTS_CORPSE		FTECONTENTS_CORPSE		//0x04000000
#define	Q3CONTENTS_DETAIL		FTECONTENTS_DETAIL		//0x08000000
#define	Q3CONTENTS_STRUCTURAL							  0x10000000
#define Q3CONTENTS_TRANSLUCENT							  0x20000000
#define	Q3CONTENTS_TRIGGER								  0x40000000
#define	Q3CONTENTS_NODROP		FTECONTENTS_SKY			//0x80000000

//qc compat only. not used internally.
#define DPCONTENTS_SOLID		1 // hit a bmodel, not a bounding box
#define DPCONTENTS_WATER		2
#define DPCONTENTS_SLIME		4
#define DPCONTENTS_LAVA			8
#define DPCONTENTS_SKY			16
#define DPCONTENTS_BODY			32 // hit a bounding box, not a bmodel
#define DPCONTENTS_CORPSE		64 // hit a SOLID_CORPSE entity
#define DPCONTENTS_NODROP		128 // an area where backpacks should not spawn
#define DPCONTENTS_PLAYERCLIP	256 // blocks player movement
#define DPCONTENTS_MONSTERCLIP	512 // blocks monster movement
#define DPCONTENTS_DONOTENTER	1024 // AI hint brush
#define DPCONTENTS_BOTCLIP		2048 // AI hint brush
#define DPCONTENTS_OPAQUE		4096 // only fully opaque brushes get this (may be useful for line of sight checks)


//Texinfo flags - warning: these mix with q3 surface flags
#define	TI_LIGHT		0x1		// value will hold the light strength

#define	TI_SLICK		0x2		// effects game physics

#define	TI_SKY			0x4		// don't draw, but add to skybox
#define	TI_WARP			0x8		// turbulent water warp
#define	TI_TRANS33		0x10
#define TI_TRANS66		0x20
#define	TI_FLOWING		0x40	// scroll towards angle
#define	TI_NODRAW		0x80	// don't bother referencing the texture
//#define	TI_HINT			0x100	// handled by tools, engine can ignore.
//#define	TI_SKIP			0x200	// handled by tools, engine can ignore.

//#define TI_KINGPIN_SPECULAR	0x400
//#define TI_KINGPIN_DIFFUSE	0x800
#define TI_KINGPIN_ALPHATEST	0x1000	//regular alphatest
//#define TI_KINGPIN_MIRROR		0x2000
//#define TI_KINGPIN_WNDW33		0x4000
//#define TI_KINGPIN_WNDW64		0x8000

#define TI_Q2EX_ALPHATEST	(1u<<25)	//seems to be a thing in other pre q2e engines too.
#define TI_N64_UV			(1u<<28)	//2 qu per texel instead of 1... (still 16qu per luxel)
#define TI_N64_SCROLL_X		(1u<<29)	//scrolls fully right each second.
#define TI_N64_SCROLL_Y		(1u<<30)	//scrolls fully down each second.
#define TI_N64_SCROLL_FLIP	(1u<<31)	//reverses the scroll dirs.

//Surface flags
//#define Q3SURFACEFLAG_NODAMAGE	0x1		// never give falling damage
//#define Q3SURFACEFLAG_SLICK		0x2		// effects game physics
//#define Q3SURFACEFLAG_SKY			0x4		// lighting from environment map
#define	Q3SURFACEFLAG_LADDER		0x8
//#define Q3SURFACEFLAG_NOIMPACT	0x10	// don't make missile explosions
//#define Q3SURFACEFLAG_NOMARKS		0x20	// don't leave missile marks
//#define Q3SURFACEFLAG_FLESH		0x40	// make flesh sounds and effects
//#define Q3SURFACEFLAG_NODRAW		0x80	// don't generate a drawsurface at all
//#define Q3SURFACEFLAG_HINT		0x100	// make a primary bsp splitter
//#define Q3SURFACEFLAG_SKIP		0x200	// completely ignore, allowing non-closed brushes
//#define Q3SURFACEFLAG_NOLIGHTMAP	0x400	// surface doesn't need a lightmap
//#define Q3SURFACEFLAG_POINTLIGHT	0x800	// generate lighting info at vertexes
//#define Q3SURFACEFLAG_METALSTEPS	0x1000	// clanking footsteps
//#define Q3SURFACEFLAG_NOSTEPS		0x2000	// no footstep sounds
//#define Q3SURFACEFLAG_NONSOLID	0x4000	// don't collide against curves with this set
//#define Q3SURFACEFLAG_LIGHTFILTER	0x8000	// act as a light filter during q3map -light
//#define Q3SURFACEFLAG_ALPHASHADOW	0x10000	// do per-pixel light shadow casting in q3map
//#define Q3SURFACEFLAG_NODLIGHT	0x20000	// don't dlight even if solid (solid lava, skies)
//#define Q3SURFACEFLAG_DUST		0x40000 // leave a dust trail when walking on this surface

// content masks. Allow q2contents_window in here
//#define	MASK_ALL				(-1)
#define	MASK_WORLDSOLID				(FTECONTENTS_SOLID|FTECONTENTS_WINDOW)	/*default trace type for something simple that ignores non-bsp stuff*/
#define	MASK_POINTSOLID				(FTECONTENTS_SOLID|FTECONTENTS_WINDOW|FTECONTENTS_BODY)	/*default trace type for an entity of no size*/
#define	MASK_BOXSOLID				(FTECONTENTS_SOLID|FTECONTENTS_PLAYERCLIP|Q2CONTENTS_WINDOW|FTECONTENTS_BODY) /*default trace type for an entity that does have size*/
#define	MASK_PLAYERSOLID			MASK_BOXSOLID
//#define	MASK_DEADSOLID			(Q2CONTENTS_SOLID|Q2CONTENTS_PLAYERCLIP|Q2CONTENTS_WINDOW)
//#define	MASK_MONSTERSOLID		(Q2CONTENTS_SOLID|Q2CONTENTS_MONSTERCLIP|Q2CONTENTS_WINDOW|Q2CONTENTS_MONSTER)
#define	MASK_WATER					(FTECONTENTS_WATER|FTECONTENTS_LAVA|FTECONTENTS_SLIME)
//#define	MASK_OPAQUE				(Q2CONTENTS_SOLID|Q2CONTENTS_SLIME|Q2CONTENTS_LAVA)
//#define	MASK_SHOT				(Q2CONTENTS_SOLID|Q2CONTENTS_MONSTER|Q2CONTENTS_WINDOW|Q2CONTENTS_DEADMONSTER)
#define Q2MASK_CURRENT			(Q2CONTENTS_CURRENT_0|Q2CONTENTS_CURRENT_90|Q2CONTENTS_CURRENT_180|Q2CONTENTS_CURRENT_270|Q2CONTENTS_CURRENT_UP|Q2CONTENTS_CURRENT_DOWN)



typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} q2dsnode_t;

typedef struct
{
	int plane;
	int children[2];
	int mins[3];
	int maxs[3];
} q3dnode_t;


typedef struct q2texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
	int			value;			// light emission, etc
	char		texture[32];	// texture name (textures/ *.wal)
	int			nexttexinfo;	// for animations, -1 = end of chain
} q2texinfo_t;



typedef struct
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
} q2dsleaf_t;
typedef struct
{
	int				contents;			// OR of all brushes (not needed?)

	int			cluster;
	int			area;

	float			mins[3];			// for frustum culling
	float			maxs[3];

	unsigned int	firstleafface;
	unsigned int	numleaffaces;

	unsigned int	firstleafbrush;
	unsigned int	numleafbrushes;
} q2dlleaf_t;

typedef struct
{
	int cluster;
	int area;
	int mins[3];
	int maxs[3];
	int firstleafsurface;
	int num_leafsurfaces;
	int firstleafbrush;
	int num_leafbrushes;
} q3dleaf_t;


typedef struct
{
	unsigned short	planenum;		// facing out of the leaf
	short	texinfo;
} q2dsbrushside_t;
typedef struct
{
	unsigned int	planenum;		// facing out of the leaf
	int	texinfo;
} q2dlbrushside_t;

typedef struct
{
	int planenum;
	int texinfo;
} q3dbrushside_t;
typedef struct
{
	int planenum;
	int texinfo;
	int facenum;
} rbspbrushside_t;

typedef struct
{
	int			firstside;
	int			numsides;
	int			contents;
} q2dbrush_t;


typedef struct
{
	int firstside;
	int num_sides;
	int shadernum;
} q3dbrush_t;

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// qbyte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
typedef struct
{
	int			numclusters;
	int			bitofs[8][2];	// bitofs[numclusters][2]
} q2dvis_t;

typedef struct
{
	int				numclusters;
	int				rowsize;
	unsigned char	data[1];
} q3dvis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct
{
	int		portalnum;
	int		otherarea;
} q2dareaportal_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} q2darea_t;



















typedef struct
{
	char shadername[OLD_MAX_QPATH];
	int surfflags;
	int contents;
} dq3shader_t;

typedef struct
{
	float n[3];
	float d;
} Q3PLANE_t;

struct Q3MODEL
{
	float mins[3];
	float maxs[3];
	int firstsurface;
	int num_surfaces;
	int firstbrush;
	int num_brushes;
};


typedef struct
{
	float point[3];
	float texcoords[2][2];
	float normal[3];
	unsigned char color[4];
} q3dvertex_t;

typedef struct
{
	float point[3];
	float stcoords[2];
	float lmtexcoords[RBSP_STYLESPERSURF][2];
	float normal[3];
	unsigned char color[RBSP_STYLESPERSURF][4];
} rbspvertex_t;

struct Q3FOG
{
	char shadername[OLD_MAX_QPATH] ;
	int brushnum;
	int visibleside;
};

enum q3surfacetype
{
	MST_BAD=0,
	MST_PLANAR=1,
	MST_PATCH=2,
	MST_TRIANGLE_SOUP=3,
	MST_FLARE=4,
	MST_FOLIAGE=5,	//added in wolf/et
	MST_PATCH_FIXED=256 //fte, fixed tessellation. Uses high parts of surf->patchwidth/height. if 0 then uses exact CPs instead.
};

typedef struct
{
	int shadernum;
	int fognum;
	int facetype;
	int firstvertex;
	int num_vertices;
	int firstindex;
	int num_indexes;
	int lightmapnum;
	int lightmap_offs[2];
	int lightmap_width;
	int lightmap_height;
	float lightmap_origin[3];
	float lightmap_vecs[2][3];
	float normal[3];
	int patchwidth;
	int patchheight;
} q3dface_t;

typedef struct
{
	int shadernum;
	int fognum;
	int facetype;
	int firstvertex;
	int num_vertices;
	int firstindex;
	int num_indexes;
	unsigned char lm_styles[RBSP_STYLESPERSURF];
	unsigned char vt_styles[RBSP_STYLESPERSURF];
	int lightmapnum[RBSP_STYLESPERSURF];
	int lightmap_offs[2][RBSP_STYLESPERSURF];	//yes, weird ordering.
	int lightmap_width;
	int lightmap_height;
	float lightmap_origin[3];
	float lightmap_vecs[2][3];
	float normal[3];
	int patchwidth;
	int patchheight;
} rbspface_t;

#define	MAX_ENT_LEAFS	32
typedef struct pvscache_s
{
	int				num_leafs;	//negative generally means resort-to-headnode.
	unsigned int	leafnums[MAX_ENT_LEAFS];
	int				areanum;
	int				areanum2;
	int				headnode;
} pvscache_t;
