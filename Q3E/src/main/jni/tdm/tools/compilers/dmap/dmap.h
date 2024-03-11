/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "../../../renderer/tr_local.h"

//meta-cvar
extern idCVar dmap_compatibility;
//TDM 2.08:
extern idCVar dmap_fixBrushOpacityFirstSide;
extern idCVar dmap_bspAllSidesOfVisportal;
extern idCVar dmap_fixVisportalOutOfBoundaryEffects;
//TDM 2.10:
extern idCVar dmap_planeHashing;
extern idCVar dmap_fasterPutPrimitives;
extern idCVar dmap_dontSplitWithFuncStaticVertices;
extern idCVar dmap_fixVertexSnappingTjunc;
extern idCVar dmap_fasterShareMapTriVerts;
extern idCVar dmap_optimizeTriangulation;
extern idCVar dmap_optimizeExactTjuncIntersection;
extern idCVar dmap_fasterAasMeltPortals;
extern idCVar dmap_fasterAasBrushListMerge;
extern idCVar dmap_pruneAasBrushesChopping;
extern idCVar dmap_fasterAasWaterJumpReachability;
extern idCVar dmap_disableCellSnappingTjunc;


typedef struct primitive_s {
	struct primitive_s *next;

	// only one of these will be non-NULL
	struct bspbrush_s *	brush;
	struct mapTri_s *	tris;
} primitive_t;


typedef struct {
	struct optimizeGroup_s	*groups;
	
	//stgatilov: this data exists temporarily while PutPrimitivesInAreas runs
	//it provides faster groups search (by planeNum), but is dropped when function ends
	struct groupsPerPlane_s *groupsPerPlane;
} uArea_t;

typedef struct {
	idMapEntity *		mapEntity;		// points into mapFile_t data
	const char *		nameEntity;		//

	idVec3				origin;
	primitive_t *		primitives;
	struct tree_s *		tree;

	int					numAreas;
	uArea_t *			areas;
} uEntity_t;


// chains of mapTri_t are the general unit of processing
typedef struct mapTri_s {
	struct mapTri_s *	next;

	const idMaterial *	material;
	void *				mergeGroup;		// we want to avoid merging triangles
											// from different fixed groups, like guiSurfs and mirrors
	int					planeNum;			// not set universally, just in some areas

	idDrawVert			v[3];
	const struct hashVert_s *hashVert[3];
	struct optVertex_s *optVert[3];
} mapTri_t;


typedef struct {
	int					width, height;
	idDrawVert *		verts;
} mesh_t;


#define	MAX_PATCH_SIZE	32

#define	PLANENUM_LEAF		-1

typedef struct parseMesh_s {
	struct parseMesh_s *next;
	mesh_t				mesh;
	const idMaterial *	material;
} parseMesh_t;

typedef struct bspface_s {
	struct bspface_s *	next;
	int					planenum;
	bool				portal;			// all portals will be selected before
										// any non-portals
	bool				checked;		// used by SelectSplitPlaneNum()
	idWinding *			w;
} bspface_t;

typedef struct {
	idVec4		v[2];		// the offset value will always be in the 0.0 to 1.0 range
} textureVectors_t;

typedef struct side_s {
	int					planenum;

	const idMaterial *	material;
	textureVectors_t	texVec;

	idWinding *			winding;		// only clipped to the other sides of the brush
	idWinding *			visibleHull;	// also clipped to the solid parts of the world
} side_t;


typedef struct bspbrush_s {
	struct bspbrush_s *	next;
	struct bspbrush_s *	original;	// chopped up brushes will reference the originals

	int					entitynum;			// editor numbering for messages
	int					brushnum;			// editor numbering for messages

	const idMaterial *	contentShader;	// one face's shader will determine the volume attributes

	int					contents;
	bool				opaque;
	int					outputNumber;		// set when the brush is written to the file list

	idBounds			bounds;
	int					numsides;
	side_t				sides[6];			// variably sized
} uBrush_t;


typedef struct drawSurfRef_s {
	struct drawSurfRef_s *	nextRef;
	int						outputNumber;
} drawSurfRef_t;


typedef struct node_s {
	// both leafs and nodes
	int					planenum;	// -1 = leaf node
	struct node_s *		parent;
	idBounds			bounds;		// valid after portalization

	// nodes only
	side_t *			side;		// the side that created the node
	struct node_s *		children[2];
	int					nodeNumber;	// set after pruning

	// leafs only
	bool				opaque;		// view can never be inside

	uBrush_t *			brushlist;	// fragments of all brushes in this leaf
									// needed for FindSideForPortal

	int					area;		// determined by flood filling up to areaportals
	int					occupied;	// 1 or greater can reach entity
	uEntity_t *			occupant;	// for leak file testing

	struct uPortal_s *	portals;	// also on nodes during construction
} node_t;


typedef struct uPortal_s {
	idPlane		plane;
	node_t		*onnode;		// NULL = outside box
	node_t		*nodes[2];		// [0] = front side of plane
	struct uPortal_s	*next[2];
	idWinding	*winding;
} uPortal_t;

// a tree_t is created by FaceBSP()
typedef struct tree_s {
	node_t		*headnode;
	node_t		outside_node;
	idBounds	bounds;
	int			nodeCnt, leafCnt;
} tree_t;

#define	MAX_QPATH			256			// max length of a game pathname

typedef struct {
	idRenderLightLocal	def;
	char		name[MAX_QPATH];		// for naming the shadow volume surface and interactions
	srfTriangles_t	*shadowTris;
} mapLight_t;

#define	MAX_GROUP_LIGHTS	16

typedef struct optimizeGroup_s {
	struct optimizeGroup_s	*nextGroup;

	idBounds			bounds;			// set in CarveGroupsByLight

	// all of these must match to add a triangle to the triList
	bool				smoothed;				// curves will never merge with brushes
	int					planeNum;
	int					areaNum;
	const idMaterial *	material;
	int					numGroupLights;
	mapLight_t *		groupLights[MAX_GROUP_LIGHTS];	// lights effecting this list
	void *				mergeGroup;		// if this differs (guiSurfs, mirrors, etc), the
										// groups will not be combined into model surfaces
										// after optimization
	textureVectors_t	texVec;

	bool				surfaceEmited;

	mapTri_t *			triList;
	mapTri_t *			regeneratedTris;	// after each island optimization
	idVec3				axis[2];			// orthogonal to the plane, so optimization can be 2D
} optimizeGroup_t;

// all primitives from the map are added to optimizeGroups, creating new ones as needed
// each optimizeGroup is then split into the map areas, creating groups in each area
// each optimizeGroup is then divided by each light, creating more groups
// the final list of groups is then tjunction fixed against all groups, then optimized internally
// multiple optimizeGroups will be merged together into .proc surfaces, but no further optimization
// is done on them

idStr ReportWorldPositionInOptimizeGroup(const idVec2 &pos, optimizeGroup_t *group);

//=============================================================================

// dmap.cpp

typedef enum {
	SO_NONE,			// 0
	SO_MERGE_SURFACES,	// 1
	SO_CULL_OCCLUDED,	// 2
	SO_CLIP_OCCLUDERS,	// 3
	SO_CLIP_SILS,		// 4
	SO_SIL_OPTIMIZE		// 5
} shadowOptLevel_t;

// Added in #4123: "Unbind VSync from DMAP". Allow more concise dmap to speed it up.
typedef enum {
	VL_CONCISE = 0,			// Suppress most console output, the new default
	VL_ORIGDEFAULT,			// The default mode pre-TDM 2.04
	VL_VERBOSE				// The original extra-verbose mode
} verbosityLevel_t;

typedef struct dmapGlobals_s {
	// mapFileBase will contain the qpath without any extension: "maps/test_box"
	char		mapFileBase[1024];

	idMapFile	*dmapFile;

	idPlaneSet	mapPlanes;

	int			num_entities;
	uEntity_t	*uEntities;

	int			entityNum;

	idList<mapLight_t*>	mapLights;

	verbosityLevel_t	verbose;

	bool	glview;
	bool	noOptimize;
	bool	verboseentities;
	bool	noCurves;
	bool	fullCarve;
	bool	noModelBrushes;
	bool	noTJunc;
	bool	nomerge;
	bool	noFlood;
	bool	noClipSides;		// don't cut sides by solid leafs, use the entire thing
	bool	noLightCarve;		// extra triangle subdivision by light frustums
	shadowOptLevel_t	shadowOptLevel;
	bool	noShadow;			// don't create optimized shadow volumes

	idBounds	drawBounds;
	bool	drawflag;

	int		totalShadowTriangles;
	int		totalShadowVerts;
} dmapGlobals_t;

extern dmapGlobals_t dmapGlobals;

int FindFloatPlane( const idPlane &plane, bool *fixedDegeneracies = NULL );

void PrintIfVerbosityAtLeast( verbosityLevel_t vl, const char* fmt, ... );	// Added #4123. Filter console output by verbosity level.
void PrintEntityHeader( verbosityLevel_t vl, const uEntity_t* e );		// Also #4123

//=============================================================================

// brush.cpp

#ifndef CLIP_EPSILON
#define	CLIP_EPSILON	0.1f
#endif

#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4

int	CountBrushList (uBrush_t *brushes);
uBrush_t *AllocBrush (int numsides);
void FreeBrush (uBrush_t *brushes);
void FreeBrushList (uBrush_t *brushes);
uBrush_t *CopyBrush (uBrush_t *brush);
void DrawBrushList (uBrush_t *brush);
void PrintBrush (uBrush_t *brush);
bool BoundBrush (uBrush_t *brush);
bool CreateBrushWindings (uBrush_t *brush);
uBrush_t	*BrushFromBounds( const idBounds &bounds );
float BrushVolume (uBrush_t *brush);
void WriteBspBrushMap( const char *name, uBrush_t *list );

void FilterBrushesIntoTree( uEntity_t *e );

void SplitBrush( uBrush_t *brush, int planenum, uBrush_t **front, uBrush_t **back);
node_t *AllocNode( void );


//=============================================================================

// map.cpp

bool 		LoadDMapFile( const char *filename );
void		FreeOptimizeGroupList( optimizeGroup_t *groups );
void		FreeDMapFile( void );

//=============================================================================

// draw.cpp -- draw debug views either directly, or through glserv.exe

void Draw_ClearWindow( void );
void DrawWinding( const idWinding *w );
void DrawAuxWinding( const idWinding *w );

void DrawLine( idVec3 v1, idVec3 v2, int color );

void GLS_BeginScene( void );
void GLS_Winding( const idWinding *w, int code );
void GLS_Triangle( const mapTri_t *tri, int code );
void GLS_EndScene( void );



//=============================================================================

// portals.cpp

#define	MAX_INTER_AREA_PORTALS	(10<<10)

typedef struct interAreaPortal_s {
	int		area0, area1;
	side_t	*side;
	uBrush_t *brush;
} interAreaPortal_t;

extern	interAreaPortal_t interAreaPortals[MAX_INTER_AREA_PORTALS];
extern	int					numInterAreaPortals;

bool FloodEntities( tree_t *tree );
void FillOutside( uEntity_t *e );
void FloodAreas( uEntity_t *e );
void MakeTreePortals( tree_t *tree );
void FreePortal( uPortal_t *p );
bool IsPortalSame( interAreaPortal_s *a, interAreaPortal_s *b );

//=============================================================================

// glfile.cpp -- write a debug file to be viewd with glview.exe

void OutputWinding( idWinding *w, idFile *glview );
void WriteGLView( tree_t *tree, char *source );

//=============================================================================

// leakfile.cpp

void LeakFile( tree_t *tree );

//=============================================================================

// facebsp.cpp

tree_t *AllocTree( void );

void FreeTree( tree_t *tree );

void FreeTree_r( node_t *node );
void FreeTreePortals_r( node_t *node );


bspface_t	*MakeStructuralBspFaceList( primitive_t *list );
bspface_t	*MakeVisibleBspFaceList( primitive_t *list );
tree_t		*FaceBSP( bspface_t *list );

//=============================================================================

// surface.cpp

mapTri_t *CullTrisInOpaqueLeafs( mapTri_t *triList, tree_t *tree );
void	ClipSidesByTree( uEntity_t *e );
void	SplitTrisToSurfaces( mapTri_t *triList, tree_t *tree );
void	PutPrimitivesInAreas( uEntity_t *e );
void	Prelight( uEntity_t *e );

//=============================================================================

// tritjunction.cpp

// TODO stgatilov: remove old tritjunction and this struct
typedef struct hashVert_s {
	struct hashVert_s	*next;
	idVec3				v;
	int					iv[3];
	int					idx;
} hashVert_t;

void	FreeTJunctionHash( void );
int		CountGroupListTris( const optimizeGroup_t *groupList );
void	FixEntityTjunctions( uEntity_t *e );
void	FixAreaGroupsTjunctions( optimizeGroup_t *groupList );
void	FixGlobalTjunctions( uEntity_t *e );

//=============================================================================

// optimize.cpp -- trianlge mesh reoptimization

// the shadow volume optimizer call internal optimizer routines, normal triangles
// will just be done by OptimizeEntity()


typedef struct optVertex_s {
	idDrawVert	v;
	idVec3	pv;					// projected against planar axis, third value is 0
	struct optEdge_s *edges;
	struct optVertex_s	*islandLink;
	bool	addedToIsland;
	bool	emited;			// when regenerating triangles
	int idx;
} optVertex_t;

typedef struct optEdge_s {
	optVertex_t	*v1, *v2;
	struct optEdge_s	*islandLink;
	bool	addedToIsland;
	bool	created;		// not one of the original edges
	bool	combined;		// combined from two or more colinear edges
	struct optTri_s	*frontTri, *backTri;
	struct optEdge_s *v1link, *v2link;
} optEdge_t;

typedef struct optTri_s {
	struct optTri_s	*next;
	idVec3		midpoint;
	optVertex_t	*v[3];
	bool	filled;
} optTri_t;

typedef struct {
	optimizeGroup_t	*group;
	optVertex_t	*verts;
	optEdge_t	*edges;
	optTri_t	*tris;
} optIsland_t;


void	OptimizeEntity( uEntity_t *e );
void	OptimizeGroupList( optimizeGroup_t *groupList );

//=============================================================================

// tritools.cpp

mapTri_t	*AllocTri( void );
void		FreeTri( mapTri_t *tri );
int			CountTriList( const mapTri_t *list );
mapTri_t	*MergeTriLists( mapTri_t *a, mapTri_t *b );
mapTri_t	*CopyTriList( const mapTri_t *a );
void		FreeTriList( mapTri_t *a );
mapTri_t	*CopyMapTri( const mapTri_t *tri );
float		MapTriArea( const mapTri_t *tri );
mapTri_t	*RemoveBadTris( const mapTri_t *tri );
void		BoundTriList( const mapTri_t *list, idBounds &b );
void		DrawTri( const mapTri_t *tri );
void		FlipTriList( mapTri_t *tris );
void		TriVertsFromOriginal( mapTri_t *tri, const mapTri_t *original );
void		PlaneForTri( const mapTri_t *tri, idPlane &plane );
idWinding	*WindingForTri( const mapTri_t *tri );
mapTri_t	*WindingToTriList( const idWinding *w, const mapTri_t *originalTri );
void		ClipTriList( const mapTri_t *list, const idPlane &plane, float epsilon, mapTri_t **front, mapTri_t **back );

//=============================================================================

// output.cpp

srfTriangles_t	*ShareMapTriVerts( const mapTri_t *tris );
void WriteOutputFile( void );

//=============================================================================

// shadowopt.cpp

srfTriangles_t *CreateLightShadow( optimizeGroup_t *shadowerGroups, const mapLight_t *light );
void		FreeBeamTree( struct beamTree_s *beamTree );

void		CarveTriByBeamTree( const struct beamTree_s *beamTree, const mapTri_t *tri, mapTri_t **lit, mapTri_t **unLit );
