// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MODEL_H__
#define __MODEL_H__

/*
===============================================================================

	Render Model

===============================================================================
*/

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#include "../idlib/rvMemSys.h"
// RAVEN END

// RAVEN BEGIN
// dluetscher: declare some classes for MD5R support
#ifdef _MD5R_SUPPORT
class rvMesh;
#endif
// RAVEN END

// shared between the renderer, game, and Maya export DLL
#define MD5_VERSION_STRING		"MD5Version"
#define MD5_MESH_EXT			"md5mesh"
#define MD5_ANIM_EXT			"md5anim"
#define MD5_CAMERA_EXT			"md5camera"
#define MD5_VERSION				10

// using shorts for triangle indexes can save a significant amount of traffic, but
// to support the large models that renderBump loads, they need to be 32 bits
#if 1

#define GL_INDEX_TYPE		GL_UNSIGNED_INT
typedef int glIndex_t;

#else

#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT
typedef short glIndex_t;

#endif


typedef struct {
	// NOTE: making this a glIndex is dubious, as there can be 2x the faces as verts
	glIndex_t					p1, p2;					// planes defining the edge
	glIndex_t					v1, v2;					// verts defining the edge
} silEdge_t;

// this is used for calculating unsmoothed normals and tangents for deformed models
typedef struct dominantTri_s {
	glIndex_t					v2, v3;
	float						normalizationScale[3];
} dominantTri_t;

typedef struct lightingCache_s {
	idVec3						localLightVector;		// this is the statically computed vector to the light
														// in texture space for cards without vertex programs
} lightingCache_t;

typedef struct shadowCache_s {
	idVec4						xyz;					// we use homogenous coordinate tricks
} shadowCache_t;

// RAVEN BEGIN
// DLuetscher: Added the vertex cache neded for penumbra map support
#ifdef _PENUMBRA_MAP_SUPPORT
typedef struct penumbraCache_s {
	idVec3						xyz;					// local coordinates
	idVec2						colorParam;				
} penumbraCache_t;
#endif
// RAVEN END

const int SHADOW_CAP_INFINITE	= 64;

// our only drawing geometry type
typedef struct srfTriangles_s {
	idBounds					bounds;					// for culling

	int							ambientViewCount;		// if == tr.viewCount, it is visible this view

	bool						generateNormals;		// create normals from geometry, instead of using explicit ones
	bool						tangentsCalculated;		// set when the vertex tangents have been calculated
	bool						facePlanesCalculated;	// set when the face planes have been calculated
	bool						perfectHull;			// true if there aren't any dangling edges
	bool						deformedSurface;		// if true, indexes, silIndexes, mirrorVerts, and silEdges are
														// pointers into the original surface, and should not be freed

	int							numVerts;				// number of vertices
	idDrawVert *				verts;					// vertices, allocated with special allocator
// RAVEN BEGIN
// dluetscher: added support for the rvSilTraceVertT as a replacement for some system-memory idDrawVerts (MD5R case)
#ifdef _MD5R_SUPPORT
	rvSilTraceVertT *			silTraceVerts;			// sil-trace vertices (system memory copy of verts for sil-trace usage)
	rvSilTraceVertT *			silTraceVertsAlloc;		// if not NULL, same array of sil-trace vertices as above, but must be freed 
#elif defined( Q4SDK_MD5R )
// Q4SDK: maintain compatible structure padding
	void*						silTraceVerts;
	void*						silTraceVertsAlloc;
#endif
// RAVEN END

	int							numIndexes;				// for shadows, this has both front and rear end caps and silhouette planes
	glIndex_t *					indexes;				// indexes, allocated with special allocator

	glIndex_t *					silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int							numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int *						mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int							numDupVerts;			// number of duplicate vertexes
	int *						dupVerts;				// pairs of the number of the first vertex and the number of the duplicate vertex

	int							numSilEdges;			// number of silhouette edges
	silEdge_t *					silEdges;				// silhouette edges

	idPlane *					facePlanes;				// [numIndexes/3] plane equations

	dominantTri_t *				dominantTris;			// [numVerts] for deformed surface fast tangent calculation

	int							numShadowIndexesNoFrontCaps;	// shadow volumes with front caps omitted
	int							numShadowIndexesNoCaps;			// shadow volumes with the front and rear caps omitted

	int							shadowCapPlaneBits;		// bits 0-5 are set when that plane of the interacting light has triangles
														// projected on it, which means that if the view is on the outside of that
														// plane, we need to draw the rear caps of the shadow volume
														// turboShadows will have SHADOW_CAP_INFINITE

	shadowCache_t *				shadowVertexes;			// these will be copied to shadowCache when it is going to be drawn.
														// these are NULL when vertex programs are available

	struct srfTriangles_s *		ambientSurface;			// for light interactions, point back at the original surface that generated
														// the interaction, which we will get the ambientCache from

	struct srfTriangles_s *		nextDeferredFree;		// chain of tris to free next frame

	// data in vertex object space, not directly readable by the CPU
	struct vertCache_s *		indexCache;				// int
	struct vertCache_s *		ambientCache;			// idDrawVert
	struct vertCache_s *		lightingCache;			// lightingCache_t
	struct vertCache_s *		shadowCache;			// shadowCache_t

// RAVEN BEGIN
// dluetscher: Added the vertex cache neded for penumbra map support
#ifdef _PENUMBRA_MAP_SUPPORT
	struct vertCache_s *		penumbraCache;			// penumbraCache_t
#endif
// RAVEN END

// RAVEN BEGIN
// dluetscher: added support for new style of meshes (rvMesh used by rvRenderModelMD5R) that 
//			   are based on primitive batches of "static" geometry (can be skinned) whose
//			   vertices always live on the video card for the purposes of drawing
#ifdef _MD5R_SUPPORT
	rvMesh *					primBatchMesh;				// rvMesh that is based on static vertex buffers, index buffers, and primitive batches
	float *						skinToModelTransforms;		// array of skin-to-model transforms, 4x4, stored in row-major array ordering, with translation in last column (column-major matrix)
	float *						skinToModelTransformsAlloc;	// if not NULL, same array of skin-to-model transforms as above, but must be freed 
	int							numSkinToModelTransforms;	// the number of skin-to-model transforms stored in the above array
#elif defined( Q4SDK_MD5R )
// Q4SDK: maintain compatible structure padding
	void*						primBatchMesh;
	float*						skinToModelTransforms;
	float*						skinToModelTransformsAlloc;
	int							numSkinToModelTransforms;
#endif
// RAVEN END

// RAVEN BEGIN
// jscott: for modview
	bool						modviewSelected;
// jscott: for security
	int							numAllocedVerts;
	int							numAllocedIndices;
// rjohnson: attempt to fix editor crashes
	int							referenceCount;
	struct srfTriangles_s *		topAmbientSurface;
	int							myID;
	bool						noAmbientSurfaces;
	bool						didSilPremultiply;
	bool						tempAmbientCache;
#ifdef _DEBUG
	char						description[64];
#endif
// RAVEN END
} srfTriangles_t;

typedef idList<srfTriangles_t *> idTriList;

typedef struct modelSurface_s {
	int							id;
	const idMaterial *			shader;
	srfTriangles_t *			geometry;

// RAVEN BEGIN
// rjohnson: added block
	idStr*						mOriginalSurfaceName;
// RAVEN END
} modelSurface_t;

// RAVEN BEGIN
// bdube: tag system
typedef struct modelTag_s {
	idStr			name;
	idVec3			t;
	idMat3			m;	
} modelTag_t;
// RAVEN END

typedef enum {
	DM_STATIC,		// never creates a dynamic model
	DM_CACHED,		// once created, stays constant until the entity is updated (animating characters)
	DM_CONTINUOUS	// must be recreated for every single view (time dependent things like particles)
} dynamicModel_t;

typedef enum {
	INVALID_JOINT				= -1
} jointHandle_t;

struct jointWeight_t {
	float					weight;					// joint weight
	int						jointMatOffset;			// offset in bytes to the joint matrix
	int						nextVertexOffset;		// offset in bytes to the first weight for the next vertex
};

// offsets for SIMD code
#define BASEVECTOR_SIZE								(4*4)		// sizeof( idVec4 )
#define JOINTWEIGHT_SIZE							(3*4)		// sizeof( jointWeight_t )
#define JOINTWEIGHT_WEIGHT_OFFSET					(0*4)		// offsetof( jointWeight_t, weight )
#define JOINTWEIGHT_JOINTMATOFFSET_OFFSET			(1*4)		// offsetof( jointWeight_t, jointMatOffset )
#define JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET			(2*4)		// offsetof( jointWeight_t, nextVertexOffset )

assert_sizeof( idVec4,								BASEVECTOR_SIZE );
assert_sizeof( jointWeight_t,						JOINTWEIGHT_SIZE );
assert_offsetof( jointWeight_t, weight,				JOINTWEIGHT_WEIGHT_OFFSET );
assert_offsetof( jointWeight_t, jointMatOffset,		JOINTWEIGHT_JOINTMATOFFSET_OFFSET );
assert_offsetof( jointWeight_t, nextVertexOffset,	JOINTWEIGHT_NEXTVERTEXOFFSET_OFFSET );

class idMD5Joint {
public:
								idMD5Joint() { parent = NULL; }
	idStr						name;
	const idMD5Joint *			parent;
};

// RAVEN BEGIN
// AReis: Used for callback.
class idRenderModel;
typedef bool(*modelCallback_t)( idRenderModel *model, void *callbackData );
// RAVEN END

// RAVEN BEGIN
// Used for Fluid Interaction.
typedef struct fluidImpact_s
{
	// The absolute position of the impact.
	idVec3 vAbsPos;

	// The force of the impact.
	float fForce;
	
	float radius;
	
} fluidImpact_t;
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)

// Create a new idRenderModel instance or one of it's derivitive classes, such
// that any further memory allocated by the instance can be directed to the same
// heap.
template <class T>
T* NewRenderModel(Rv_Sys_Heap_ID_t heapID)
{
	// Push the heap context to be used for both instance and all allocations
	// that the instance makes.
	RV_PUSH_SYS_HEAP_ID(heapID);

	T* model=new T;
	assert(static_cast<idRenderModel*>(model)!=0);
#if defined(_DEBUG)
	if(model)
	{
		model->heapID=heapID;
	}
#endif

	// Pop the heap context.
	RV_POP_HEAP();

	return model;
}

// Create a new idRenderModel instance or one of it's derivitive classes, such
// that any further memory allocated by the instance can be directed to the same
// heap.
template <class T>
T* NewRenderModel(const idRenderModel* m)
{
	// Push the heap context to be used for both instance and all allocations
	// that the instance makes.
	bool ok=rvPushHeapContainingMemory(m);

	T* model=new T;
	assert(static_cast<idRenderModel*>(model)!=0);
#if defined(_DEBUG)
	if(model)
	{
		model->heapPtr=currentHeapArena->GetHeap(const_cast<void*>((const void*)m));
	}
#endif

	// Pop the heap context.
	if(ok)
	{
		RV_POP_HEAP();
	}

	return model;
}

#else

// Single heap memory model.
template <class T>
T* NewRenderModel(Rv_Sys_Heap_ID_t)
{
	return new T;
}

// Single heap memory model.
template <class T>
T* NewRenderModel(const idRenderModel*)
{
	return new T;
}

#endif
// RAVEN END

// the init methods may be called again on an already created model when
// a reloadModels is issued
class idRenderModel {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT) && defined(_DEBUG)
public:
	// Debug info... the heap this instance was allocated into.
	Rv_Sys_Heap_ID_t			heapID;
	rvHeap*						heapPtr;
#endif
// RAVEN END

public:
// RAVEN BEGIN
// AReis: Needed to send data to model.
	// Callbacks to a model specified function.
	modelCallback_t				callback;

// mwhitlock: Xenon texture streaming
#if defined(_XENON)
	// All the materials that are referenced by an instance of an idRenderModel.
	idList<idMaterial*>			allMaterials;
#endif

// AReis: Specific just to a fluid model.
	// Dampen a grid element that intersects the world.
	virtual void				DampenFluidGrid( int iX, int iY, float fAmount ) {}

// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT) && defined(_DEBUG)
	idRenderModel( void ) :
		heapID(RV_HEAP_ID_DEFAULT),
		heapPtr(0)
	{
	}
#endif

// rjohnson: added debugging code to try and catch a free error
	// purges all the data before deleting
	virtual						~idRenderModel( void );
// RAVEN END

	// Loads static models only, dynamic models must be loaded by the modelManager
	virtual void				InitFromFile( const char *fileName ) = 0;

	// renderBump uses this to load the very high poly count models, skipping the
	// shadow and tangent generation, along with some surface cleanup to make it load faster
	virtual void				PartialInitFromFile( const char *fileName ) = 0;

	// this is used for dynamically created surfaces, which are assumed to not be reloadable.
	// It can be called again to clear out the surfaces of a dynamic model for regeneration.
	virtual void				InitEmpty( const char *name ) = 0;

// RAVEN BEGIN
// AReis: Added this function for the height map model.
	// Like InitEmpty but allows a set of arguments to be passed in through a dict.
	virtual void				InitEmptyFromArgs( const char *name, idDict &Args ) = 0;
// RAVEN END

	// dynamic model instantiations will be created with this
	// the geometry data will be owned by the model, and freed when it is freed
	// the geoemtry should be raw triangles, with no extra processing
	virtual void				AddSurface( modelSurface_t surface ) = 0;

	// cleans all the geometry and performs cross-surface processing
	// like shadow hulls
	// Creates the duplicated back side geometry for two sided, alpha tested, lit materials
	// This does not need to be called if none of the surfaces added with AddSurface require
	// light interaction, and all the triangles are already well formed.
	virtual void				FinishSurfaces( void ) = 0;

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
	virtual bool				IsLoaded() = 0;
	virtual void				SetLevelLoadReferenced( bool referenced ) = 0;
	virtual bool				IsLevelLoadReferenced() = 0;

	// models that are already loaded at level start time
	// will still touch their data to make sure they
	// are kept loaded
	virtual void				TouchData() = 0;

	// dump any ambient caches on the model surfaces
	virtual void				FreeVertexCache() = 0;

	// returns the name of the model
	virtual const char	*		Name() const = 0;

	// prints a detailed report on the model for printModel
	virtual void				Print() const = 0;

	// prints a single line report for listModels
	virtual void				List() const = 0;

	// reports the amount of memory (roughly) consumed by the model
	virtual int					Memory() const = 0;

	// for reloadModels
	virtual unsigned int		Timestamp() const = 0;

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

	// created at load time by stitching together all surfaces and sharing
	// the maximum number of edges.  This may be incorrect if a skin file
	// remaps surfaces between shadow casting and non-shadow casting, or
	// if some surfaces are noSelfShadow and others aren't
	virtual srfTriangles_t	*	ShadowHull( void ) const = 0;

	// models of the form "_area*" may have a prelight shadow model associated with it
	virtual bool				IsStaticWorldModel( void ) const = 0;

	// models parsed from inside map files or dynamically created cannot be reloaded by
	// reloadmodels
	virtual bool				IsReloadable( void ) const = 0;

	// md3, md5, particles, etc
	virtual dynamicModel_t		IsDynamicModel( void ) const = 0;

	// if the load failed for any reason, this will return true
	virtual bool				IsDefaultModel( void ) const = 0;

	// dynamic models should return a fast, conservative approximation
	// static models should usually return the exact value
	virtual idBounds			Bounds( const struct renderEntity_s *ent = NULL ) const = 0;

	// returns value != 0.0f if the model requires the depth hack
	virtual float				DepthHack( void ) const = 0;

// RAVEN BEGIN
// dluetscher: added call to determine if a collision surface exists within this model
	virtual bool				HasCollisionSurface( const struct renderEntity_s *ent ) const = 0;
// RAVEN END

	// returns a static model based on the definition and view
	// currently, this will be regenerated for every view, even though
	// some models, like character meshes, could be used for multiple (mirror)
	// views in a frame, or may stay static for multiple frames (corpses)
	// The renderer will delete the returned dynamic model the next view
	// This isn't const, because it may need to reload a purged model if it
	// wasn't precached correctly.
// RAVEN BEGIN
// dluetscher: added surface mask parameter
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask = ~SURF_COLLISION  ) = 0;
// RAVEN END

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

	// Writing to and reading from a demo file.
	virtual void				ReadFromDemo( class idDemoFile *f ) = 0;
	virtual void				WriteToDemo( class idDemoFile *f ) = 0;

// RAVEN BEGIN
// bdube: surface flag manipulation
	virtual int					GetSurfaceMask ( const char* surface ) const = 0;;

// jscott: for portal skies
	virtual void				SetHasSky( bool on ) = 0;
	virtual bool				GetHasSky( void ) const = 0;
// ddynerman: Wolf LOD code
	virtual void				SetViewEntity( const struct viewEntity_s *ve ) = 0;
// RAVEN END

// RAVEN BEGIN 
#if defined( _MD5R_SUPPORT ) 
// dluetscher: added method to determine if model maintains system-memory dynamic meshes (used 
//			   for traces and silhouette determination) that are separate from pairs of video-memory 
//			   meshes - one used for shadow volume drawing and one used for normal interaction drawing
//			   NOTE: currently, only MD5R models return true, all others return false
	virtual bool				HasSeparateSilTraceMeshes( void ) const;		
#endif
// RAVEN END

// RAVEN END
};

#endif /* !__MODEL_H__ */
