// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MODEL_H__
#define __MODEL_H__

#include "Model_public.h"
#include "RenderWorld.h"

/*
===============================================================================

	Render Model

===============================================================================
*/

class idMaterial;


const int SHADOW_CAP_INFINITE	= 64;

enum primitiveModes_e {
	PM_TRIANGLE = 0,
	PM_POINTSPRITE
};

typedef enum {
	TRIPARM_DISTANCESCALE,
	TRIPARM_MAX
} srfTriParams_t;

typedef struct srfIndexTree_s
{
	idBounds bb;
	unsigned short range[4];
	int kids[2];
} srfIndexTree_t;

typedef struct srfInteractionRef_s {
	srfTriangles_t *tri;
	struct srfInteractionRef_s *next;
} srfInteractionRef_t;


// our only drawing geometry type
struct srfTriangles_t {
	idBounds					bounds;					// for culling

	int							ambientViewCount;		// if == tr.viewCount, it is visible this view

	bool						generateNormals;		// create normals from geometry, instead of using explicit ones
	bool						tangentsCalculated;		// set when the vertex tangents have been calculated
	bool						facePlanesCalculated;	// set when the face planes have been calculated
	bool						perfectHull;			// true if there aren't any dangling edges
	bool						deformedSurface;		// if true, indexes, silIndexes, mirrorVerts, and silEdges are
														// pointers into the original surface, and should not be freed
	bool						deformedSurfaceInstance;// like deformedSurface, but indexes, silIndexes and silEdges are
														// unique and should be freed
	bool						hardwareSkinnedSurface;	// if true, ambientSurface is a pointer into the original surface,
														// and should not be freed
	bool						sharedSurface;			// if true, verts are shared between instances and ambientSurface is
														// a pointer into the original surface and should not be freed

	//BEGIN RAVEN
	int							numAllocedVerts;
	int							numAllocedIndices;
	bool						frameAlloced;
	//END RAVEN

	int							numVerts;				// number of vertices
	idDrawVert *				verts;					// vertices, allocated with special allocator

	int							numIndexes;				// for shadows, this has both front and rear end caps and silhouette planes
	glIndex_t *					indexes;				// indexes, allocated with special allocator

	glIndex_t *					silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int							numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int *						mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int							numSilEdges;			// number of silhouette edges
	silEdge_t *					silEdges;				// silhouette edges

	int							numAllocatedFacePlanes;
	idPlane *					facePlanes;				// [numIndexes/3] plane equations

#if SD_SUPPORT_UNSMOOTHEDTANGENTS
	dominantTri_t *				dominantTris;			// [numVerts] for deformed surface fast tangent calculation
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS

	int							numShadowIndexesNoFrontCaps;	// shadow volumes with front caps omitted
	int							numShadowIndexesNoCaps;			// shadow volumes with the front and rear caps omitted

	int							shadowCapPlaneBits;		// bits 0-5 are set when that plane of the interacting light has triangles
														// projected on it, which means that if the view is on the outside of that
														// plane, we need to draw the rear caps of the shadow volume
														// turboShadows will have SHADOW_CAP_INFINITE

	shadowCache_t *				shadowVertexes;			// these will be copied to shadowCache when it is going to be drawn.
														// these are NULL when vertex programs are available

	struct srfTriangles_t *		ambientSurface;			// for light interactions, point back at the original surface that generated
														// the interaction, which we will get the ambientCache from

	struct srfTriangles_t *		nextDeferredFree;		// chain of tris to free next frame

	int							mode;					// what primitive time to render is GL_TRIANGLES by default

	int							dsFlags;

	// data in vertex object space, not directly readable by the CPU
	struct vertCache_s *		indexCache;				// int
	struct vertCache_s *		ambientCache;			// idDrawVert
	struct vertCache_s *		shadowCache;			// shadowCache_t
	struct vertCache_s *		weightCache;			// vertWeight_t

	int							numJoints;
	idJointMat *				joints;

	int							numIndexTree;
	srfIndexTree_t *			indexTree;

	float						params[TRIPARM_MAX];

	srfInteractionRef_t			*interactions;

	float						texCoordScale;

#ifdef SD_DEBUG_TRISURFMEMORY
	const char*				indexesAllocFileName;
	int						indexesAllocLineNumber;
	const char*				deferredFreeFileName;
	int						deferredFreeLineNumber;
	const char*				triAllocFileName;
	int						triAllocLineNumber;
	const char*				origAllocFileName;
	int						origAllocLineNumber;
	const char*				ambCacheFreeFileName;
	int						ambCacheFreeLineNumber;
	int						ambCacheFreeViewCount;
	int						vertsID;
	int						indexID;
	int						ID;
#endif
};

typedef idList<srfTriangles_t *> idTriList;

typedef struct modelSurface_s {
	int							id;
	const idMaterial *			material;
	srfTriangles_t *			geometry;
} modelSurface_t;

typedef enum {
	DM_STATIC,		// never creates a dynamic model
	DM_CACHED,		// once created, stays constant until the entity is updated (animating characters)
	DM_CONTINUOUS	// must be recreated for every single view (time dependent things like particles)
} dynamicModel_t;

typedef enum {
	INVALID_JOINT				= -1
} jointHandle_t;

#define COLLISION_JOINT_HANDLE( id )	( ( id ) >= 0 ? INVALID_JOINT : ((jointHandle_t) ( -1 - id )) )
#define JOINTHANDLE_FOR_TRACE( trace )	( trace ? COLLISION_JOINT_HANDLE( trace->c.id ) : INVALID_JOINT )

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
typedef struct deformInfo_s {
	int				numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int				numOutputVerts;

	int				numMirroredVerts;
	int *			mirroredVerts;

	int				numIndexes;
	glIndex_t *		indexes;

	glIndex_t *		silIndexes;

	int				numSilEdges;
	silEdge_t *		silEdges;

#if SD_SUPPORT_UNSMOOTHEDTANGENTS
	dominantTri_t *	dominantTris;
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS
} deformInfo_t;

struct imageuseinfo {
	int numpolys;
	int usecount;
	float surfacearea;
	int alphamat;
};

class idMD5Joint {
public:
								idMD5Joint() { parent = NULL; }
	idStr						name;
	const idMD5Joint *			parent;
};

const int MAX_GUISURFACE_POINTS = 16;
const int MAX_GUISURFACE_INDICES = ( MAX_GUISURFACE_POINTS - 2 ) * 3;
const int MAX_GUISURFACE_TRIANGLES = MAX_GUISURFACE_INDICES / 3;

struct guiSurface_t {
	int					guiNum;
	idBounds			bounds;
	idVec3				origin;
	idMat3				axis;
	jointHandle_t		joint;

	// geometry data used for tracing and culling
	idPlane				plane;
	idPlane				edgePlanes[ MAX_GUISURFACE_TRIANGLES ][ 2 ];
	int					numTris;
};

// the init methods may be called again on an already created model when
// a reloadModels is issued

class idRenderModel {
public:
								idRenderModel() { };
	virtual						~idRenderModel() {};

	// Loads static models only, dynamic models must be loaded by the modelManager
	virtual void				InitFromFile( const char *fileName ) = 0;

	// renderBump uses this to load the very high poly count models, skipping the
	// shadow and tangent generation, along with some surface cleanup to make it load faster
	virtual void				PartialInitFromFile( const char *fileName ) = 0;

	// this is used for dynamically created surfaces, which are assumed to not be reloadable.
	// It can be called again to clear out the surfaces of a dynamic model for regeneration.
	virtual void				InitEmpty( const char *name ) = 0;

	// dynamic model instantiations will be created with this
	// the geometry data will be owned by the model, and freed when it is freed
	// the geometry should be raw triangles, with no extra processing
	virtual void				AddSurface( modelSurface_t surface ) = 0;

	// cleans all the geometry and performs cross-surface processing
	// like shadow hulls
	// Creates the duplicated back side geometry for two sided, alpha tested, lit materials
	// This does not need to be called if none of the surfaces added with AddSurface require
	// light interaction, and all the triangles are already well formed.
	virtual void				FinishSurfaces( bool duplicateGeometry = true, bool cleanupGeometry = true, bool concatDupeGeom = false ) = 0;

	// Does consistency checks that would cause excessive spammage...
	virtual bool				Validate( void ) const = 0;

	// frees all the data, but leaves the class around for dangling references,
	// which can regenerate the data with LoadModel()
	virtual void				PurgeModel() = 0;

	// resets any model information that needs to be reset on a same level load etc.. 
	// currently only implemented for liquids
	virtual void				Reset() = 0;

	// used for initial loads, reloadModel, and reloading the data of purged models
	// Upon exit, the model will absolutely be valid, but possibly as a default model
	virtual void				LoadModel() = 0;

	// internal use
	virtual bool				IsLoaded() const = 0;
	virtual void				SetReferencedOutsideLevelLoad( bool referenced ) = 0;
	virtual bool				IsReferencedOutsideLevelLoad() const = 0;
	virtual void				SetLevelLoadReferenced( bool referenced ) = 0;
	virtual bool				IsLevelLoadReferenced() const = 0;

	// models that are already loaded at level start time
	// will still touch their data to make sure they
	// are kept loaded
	virtual void				TouchData() = 0;

	// dump any ambient caches on the model surfaces
	virtual void				FreeVertexCache() = 0;
	virtual void				DirtyVertexAmbientCache() = 0;

	// returns the name of the model
	virtual const char	*		Name() const = 0;

	// prints a detailed report on the model for printModel
	virtual void				Print() const = 0;

	// prints a single line report for listModels
	virtual void				List( bool csv ) const = 0;

	// prints a single line report for listTexUsage
	virtual void				TexUsage() const = 0;

	// 
	virtual void				Media( idFile *reportfile, sdHashMapGeneric< const idImage*, imageuseinfo > &texref ) const = 0;

	// reports the amount of memory (roughly) consumed by the model
	virtual int					Memory() const = 0;

	// for reloadModels
	virtual unsigned			Timestamp() const = 0;

	// returns the number of surfaces
	virtual int					NumSurfaces() const = 0;

	// NumBaseSurfaces will not count any overlays added to dynamic models
	virtual int					NumBaseSurfaces() const = 0;

	// get a pointer to a surface
	virtual const modelSurface_t *Surface( int surfaceNum ) const = 0;

	// Allocates surface triangles.
	// Allocates memory for srfTriangles_t::verts and srfTriangles_t::indexes
	// The allocated memory is not initialized.
	// srfTriangles_t::numVerts and srfTriangles_t::numIndexes are set to zero.
	virtual srfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const = 0;

	// Frees surfaces triangles.
	virtual void				FreeSurfaceTriangles( srfTriangles_t *tris ) const = 0;

	// models of the form "_area*" may have a prelight shadow model associated with it
	virtual bool				IsStaticWorldModel() const = 0;

	// models parsed from inside map files or dynamically created cannot be reloaded by
	// reloadmodels
	virtual bool				IsReloadable() const = 0;

	// md3, md5, particles, etc
	virtual dynamicModel_t		IsDynamicModel() const = 0;

	// if the load failed for any reason, this will return true
	virtual bool				IsDefaultModel() const = 0;

	// dynamic models should return a fast, conservative approximation
	// static models should usually return the exact value
	virtual idBounds			Bounds( const renderEntity_t *ent = NULL ) const = 0;

	// returns value != 0.0f if the model requires the depth hack
	virtual float				DepthHack() const = 0;

	// returns a static model based on the definition and view
	// currently, this will be regenerated for every view, even though
	// some models, like character meshes, could be used for multiple (mirror)
	// views in a frame, or may stay static for multiple frames (corpses)
	// The renderer will delete the returned dynamic model the next view
	// This isn't const, because it may need to reload a purged model if it
	// wasn't precached correctly.
	virtual void				InstantiateDynamicModel( class idRenderEntityLocal *ent, const struct viewDef_s *view, int lod = 0 ) = 0;
	virtual void				UpdateDeferredSurface( class idRenderEntityLocal *def, modelSurface_t *surf ) = 0;

	// Returns the number of joints or 0 if the model is not an MD5
	virtual int					NumJoints( void ) const = 0;

	// Returns the MD5 joints or NULL if the model is not an MD5
	virtual const idMD5Joint *	GetJoints( void ) const = 0;

	// Returns the handle for the joint with the given name.
	virtual jointHandle_t		GetJointHandle( const char *name ) const = 0;

	// Returns the name for the joint with the given handle.
	virtual const char *		GetJointName( jointHandle_t handle ) const = 0;

	// Returns the default animation pose or NULL if the model is not an MD5.
	virtual const idJointQuat *	GetDefaultPose( void ) const = 0;

	// Returns number of the joint nearest to the given triangle.
	virtual int					NearestJoint( int surfaceNum, int a, int c, int b ) const = 0;

	// Returns the number of GUI surfaces
	virtual int					NumGUISurfaces( void ) const = 0;

	// Returns the GUI surfaces
	virtual const guiSurface_t*	GetGUISurface( int guiSurfaceNum ) const = 0;

	// Writing to and reading from a demo file.
	virtual void				ReadFromDemoFile( class idDemoFile *f ) = 0;
	virtual void				WriteToDemoFile( class idDemoFile *f ) = 0;

	// Current lod
	virtual int					GetCurrentLod( void ) const = 0;
	virtual void				SetCurrentLod( const int lod ) = 0;

	// Update current lod
	virtual bool				UpdateLod( const struct renderEntity_t *ent, const struct viewDef_s *view, idRenderModel *cachedModel, int& newLod ) const = 0;

	// Returns true if the dynamic cached model needs purging and reinstatiating
	virtual bool				NeedsReinstantiating( class idRenderEntityLocal *def, const struct viewDef_s *view, int lod = 0 ) const = 0;

	// Returns the id of the surface with the given name (-1 if not supported or not found)
	virtual int					FindSurfaceId( const char *surfaceName ) = 0;

	virtual void				SetBounds( idBounds const &bb ) = 0;

	// Purges any partial loadable images referenced by this model
	virtual void				PurgePartialLoadableImages( void ) = 0;

	// Schedules loading of any partial loadable images referenced by this model
	virtual void				LoadPartialLoadableImages( bool blocking = false ) = 0;

	// All surfaces have finished any pending partial image loads
	virtual bool				IsFinishedPartialLoading( void ) const = 0;

	virtual idList<int>*		GetFixedAreas( void ) = 0;

	virtual void				SetFixedAreas( idList<int> const &a ) = 0;

	virtual int					NumMeshes( const int lod = 0 ) const = 0;
	virtual idBounds			CalcMeshBounds( int meshIndex, const idJointMat *joints, const idVec3 &offset, const idMat3 &axis, bool useDefaultAnim ) = 0;
};

#endif /* !__MODEL_H__ */
