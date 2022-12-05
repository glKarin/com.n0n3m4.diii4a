
#ifndef __COLLISIONMODELMANAGER_H__
#define __COLLISIONMODELMANAGER_H__

/*
===============================================================================

	Trace model vs. polygonal model collision detection.

	Short translations are the least expensive. Retrieving contact points is
	about as cheap as a short translation. Position tests are more expensive
	and rotations are most expensive.

	There is no position test at the start of a translation or rotation. In other
	words if a translation with start != end or a rotation with angle != 0 starts
	in solid, this goes unnoticed and the collision result is undefined.

	A translation with start == end or a rotation with angle == 0 performs
	a position test and fills in the trace_t structure accordingly.

===============================================================================
*/

// contact type
enum contactType_t {
	CONTACT_NONE,							// no contact
	CONTACT_EDGE,							// trace model edge hits model edge
	CONTACT_MODELVERTEX,					// model vertex hits trace model polygon
	CONTACT_TRMVERTEX						// trace model vertex hits model polygon
};

// contact info
struct contactInfo_t {
	contactType_t			type;			// contact type
	idVec3					point;			// point of contact
	idVec3					normal;			// contact plane normal
	float					dist;			// contact plane distance
	float					separation;		// contact feature separation at initial position
	int						contents;		// contents at other side of surface
	const idMaterial *		material;		// surface material
	int						modelFeature;	// contact feature on model
	int						trmFeature;		// contact feature on trace model
	int						entityNum;		// entity the contact surface is a part of
	int						id;				// id of clip model the contact surface is part of
// RAVEN BEGIN
// jscott: for material type code
	const rvDeclMatType		*materialType;	// material type of texture (possibly indirected though a hit map)
// RAVEN END
};

// trace result
struct trace_t {
	float					fraction;		// fraction of movement completed, 1.0 = didn't hit anything
	idVec3					endpos;			// final position of trace model
	idMat3					endAxis;		// final axis of trace model
	contactInfo_t			c;				// contact information, only valid if fraction < 1.0
};

#define WORLD_MODEL_NAME	"worldMap"		// name of world model
#define CM_CLIP_EPSILON		0.25f			// always stay this distance away from any model
#define CM_BOX_EPSILON		1.0f			// should always be larger than clip epsilon
#define CM_MAX_TRACE_DIST	4096.0f			// maximum distance a trace model may be traced, point traces are unlimited

// collision model
class idCollisionModel {
public:
	virtual						~idCollisionModel() { }
								// Returns the name of the model.
	virtual const char *		GetName( void ) const = 0;
								// Gets the bounds of the model.
	virtual bool				GetBounds( idBounds &bounds ) const = 0;
								// Gets all contents flags of brushes and polygons of the model ored together.
	virtual bool				GetContents( int &contents ) const = 0;
								// Gets a vertex of the model.
	virtual bool				GetVertex( int vertexNum, idVec3 &vertex ) const = 0;
								// Gets an edge of the model.
	virtual bool				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const = 0;
								// Gets a polygon of the model.
	virtual bool				GetPolygon( int polygonNum, idFixedWinding &winding ) const = 0;
};

// collision model manager
class idCollisionModelManager {
public:
	virtual						~idCollisionModelManager( void ) {}

	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;

								// Loads collision models from a map file.
	virtual void				LoadMap( const idMapFile *mapFile, bool forceReload ) = 0;
								// Frees all the collision models for the given map.
	virtual void				FreeMap( const char *mapName ) = 0;

								// Loads a collision model.
	virtual idCollisionModel *	LoadModel( const char *mapName, const char *modelName ) = 0;
// RAVEN BEGIN
// mwhitlock: conversion from idRenderModel to MD5R fixes (to keep redundant collision surfaces out of the MD5R files).
	virtual idCollisionModel *	ExtractCollisionModel( idRenderModel *renderModel, const char *modelName ) = 0;
// RAVEN END
								// Precaches a collision model.
	virtual void				PreCacheModel( const char *mapName, const char *modelName ) = 0;
								// Free the given model.
	virtual void				FreeModel( idCollisionModel *model ) = 0;
								// Purge all unused models.
	virtual void				PurgeModels( void ) = 0;

								// Sets up a trace model for collision with other trace models.
	virtual idCollisionModel *	ModelFromTrm( const char *mapName, const char *modelName, const idTraceModel &trm, const idMaterial *material ) = 0;
								// Creates a trace model from a collision model, returns true if succesfull.
	virtual bool				TrmFromModel( const char *mapName, const char *modelName, idTraceModel &trm ) = 0;
								// Creates a trace model for each primitive of the collision model, returns the number of trace models.
	virtual int					CompoundTrmFromModel( const char *mapName, const char *modelName, idTraceModel *trms, int maxTrms ) = 0;

								// Translates a trace model and reports the first collision if any.
	virtual void				Translation( trace_t *results, const idVec3 &start, const idVec3 &end,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
									idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;
								// Rotates a trace model and reports the first collision if any.
	virtual void				Rotation( trace_t *results, const idVec3 &start, const idRotation &rotation,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
									idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;
								// Returns the contents touched by the trace model or 0 if the trace model is in free space.
	virtual int					Contents( const idVec3 &start,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
									idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;
								// Stores all contact points of the trace model with the model, returns the number of contacts.
	virtual int					Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
									idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;

								// Tests collision detection.
	virtual void				DebugOutput( const idVec3 &viewOrigin, const idMat3 &viewAxis ) = 0;
								// Draws a model.
	virtual void				DrawModel( idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis,
												const idVec3 &viewOrigin, const idMat3 &viewAxis, const float radius ) = 0;
								// Lists all loaded models.
	virtual void				ListModels( void ) = 0;
								// Prints model information, use -1 for accumulated model info.
	virtual void				ModelInfo( int num ) = 0;
	virtual void				PrintMemInfo( MemInfo *mi ) = 0;
	virtual bool				IsLoaded( void ) = 0;

								// Writes a collision model file for the given map entity.
	virtual bool				WriteCollisionModelForMapEntity( const idMapEntity *mapEnt, const char *filename, const bool testTraceModel = true ) = 0;
};

extern idCollisionModelManager *collisionModelManager;

#endif /* !__COLLISIONMODELMANAGER_H__ */
