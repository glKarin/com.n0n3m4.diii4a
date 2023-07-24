// Copyright (C) 2007 Id Software, Inc.
//

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

class idMaterial;
class sdDeclSurfaceType;

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
	const sdDeclSurfaceType *surfaceType;	// surface type
	idVec3					surfaceColor;	// surface color
	int						modelFeature;	// contact feature on model
	int						trmFeature;		// contact feature on trace model
	int						entityNum;		// entity the contact surface is a part of
	int						id;				// id of clip model the contact surface is part of
	int						selfId;
};

// trace result
struct trace_t {
	float					fraction;		// fraction of movement completed, 1.0 = didn't hit anything
	idVec3					endpos;			// final position of trace model
	idMat3					endAxis;		// final axis of trace model
	contactInfo_t			c;				// contact information, only valid if fraction < 1.0
};

#define WORLD_MODEL_NAME	"worldMap"		// name of world model
#define CM_CLIP_EPSILON		0.5f			// always stay this distance away from any model
const float CM_BOX_EPSILON	= 1.f;			// should always be larger than clip epsilon
const short CM_BOX_EPSILON_SHORT = ( short )CM_BOX_EPSILON;
#define CM_MAX_TRACE_DIST	4096.0f			// maximum distance a trace model may be traced, point traces are unlimited
#define MAIN_THREAD_ID		1


struct cm_contents_override_t {
	cm_contents_override_t() : contentsAdd( 0 ), contentsRemove( 0 ) {}
	int						contentsAdd;
	int						contentsRemove;
};


// the planes should have been transformed into world space
class collisionWorldBrushSide_t {
public:
	collisionWorldBrushSide_t() : material( 0 ) {}
	collisionWorldBrushSide_t( const idPlane& p, const idMaterial* m, const idWinding& w, cm_contents_override_t contentsOverride_ ) :
		plane( p ),
		material( m ),
		winding( w ),
		contentsOverride( contentsOverride_ ) {
	}

	idPlane					plane;
	const idMaterial*		material;
	idWinding				winding;
	cm_contents_override_t	contentsOverride;

	collisionWorldBrushSide_t& operator=( const collisionWorldBrushSide_t& rhs ) {
		if( this == &rhs ) {
			return *this;
		}
		plane = rhs.plane;
		material = rhs.material;
		winding = rhs.winding;
		contentsOverride = rhs.contentsOverride;
		return *this;
	}
};

// the surface patch should have been transformed into world space
class collisionWorldPatch_t {
public:
	collisionWorldPatch_t() : material( NULL ) {}
	collisionWorldPatch_t( const idSurface_Patch& patch, const idMaterial* mat, int w, int h, cm_contents_override_t contentsOverride_ ) : 
		width( w ), height( h ), surface( patch ), material( mat ), contentsOverride( contentsOverride_ ) {}

	idSurface_Patch			surface;
	const idMaterial*		material;
	int 					width;
	int 					height;
	cm_contents_override_t	contentsOverride;
};

struct collisionWorldMesh_t {
			collisionWorldMesh_t() {}
			collisionWorldMesh_t( const char* model, cm_contents_override_t contentsOverride_, const idVec3& origin_ = vec3_origin, const idMat3& axis_ = mat3_identity ) :
				modelName( model ),
				origin( origin_ ),
				axis( axis_ ),
				contentsOverride( contentsOverride_ ) {
			}

	idStr						modelName;
	idVec3						origin;
	idMat3						axis;
	cm_contents_override_t		contentsOverride;	
};

class collisionWorldEntity_t {
public:
	collisionWorldEntity_t() {}
	collisionWorldEntity_t( const idStr& n, const idStr& c, const idDict& d, const idVec3& t, const idMat3& r, const idBounds& b = bounds_zero ) :
		name( n ), classname( c ), epairs( d ), translation( t ), rotation( r ), bounds( b ) {}
	idDict							epairs;
	idStr							name;
	idStr							classname;
	idVec3							translation;
	idMat3							rotation;
	idBounds						bounds;
	
	idList< collisionWorldBrushSide_t >		sides;
	idList< int >							planeNums;
	idList< sdPair< int, int > >			brushSideIndices;	// [ first, second ), each pair indexes into the sides list, comprising a single brush
	idList< collisionWorldPatch_t >			patches;
	idList< collisionWorldMesh_t >			meshes;
    
};

struct cm_procNode_t {
	idPlane plane;
	int children[2];				// negative numbers are (-1 - areaNumber), 0 = solid
};

typedef void* (*cmPushTaskCallback)( void* /*handle*/, const char* /*desc*/, int /*low*/ , int /*high*/ );
typedef void(*cmUpdateTaskCallback)( void* /*handle*/, void* /*taskHandle*/, int /*value*/ );
typedef void(*cmPopTaskCallback)( void* /*handle*/, void* /*taskHandle*/, bool /*remove*/ );

class collisionWorldFile_t {
public:
	collisionWorldFile_t() :
		crc( 0 ),
		pushTask( NULL ),
		updateTask( NULL ),
		popTask( NULL ),
		taskHandle( NULL ) {}

	idList< cm_procNode_t >				procNodes;
	idList< collisionWorldEntity_t >	entities;
	idStr								name;
	int									crc;

	cmPushTaskCallback					pushTask;
	cmUpdateTaskCallback				updateTask;
	cmPopTaskCallback					popTask;
	void*								taskHandle;
};

// collision model
class idCollisionModel {
public:
	virtual						~idCollisionModel() { }
								// Returns the name of the model.
	virtual const char *		GetName( void ) const = 0;
								// Gets the bounds of the model.
	virtual const idBounds&		GetBounds( void ) const = 0;
								// Gets the bounds of the model, excluding/including surfaces of the appropriate surface type
	virtual void				GetBounds( idBounds& bounds, int surfaceMask, bool inclusive ) const = 0;
								// Gets all contents flags of brushes and polygons of the model ored together.
	virtual int					GetContents( void ) const = 0;
								// Gets a vertex of the model.
	virtual const idVec3&		GetVertex( int vertexNum ) const = 0;
								// Gets an edge of the model.
	virtual void				GetEdge( int edgeNum, idVec3& start, idVec3& end ) const = 0;
								// Gets a polygon of the model.
	virtual void				GetPolygon( int polygonNum, idFixedWinding &winding ) const = 0;
								// Draws surfaces which don't/do have the surface mask
	virtual void				Draw( int surfaceMask, bool inclusive ) const = 0;

	virtual int					GetNumBrushPlanes( void ) const = 0;
	virtual const idPlane&		GetBrushPlane( int planeNum ) const = 0;

	virtual const idMaterial*	GetPolygonMaterial( int polygonNum ) const = 0;
	virtual const idPlane&		GetPolygonPlane( int polygonNum ) const = 0;
	virtual int					GetNumPolygons( void ) const = 0;

	virtual bool				IsTraceModel( void ) const = 0;
	virtual bool				IsConvex( void ) const = 0;

	virtual bool				IsWorld( void ) const = 0;
	virtual void				SetWorld( bool tf ) = 0;
};

class idGame;

// collision model manager
class idCollisionModelManager {
public:
	virtual						~idCollisionModelManager( void ) {}

	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;

	virtual void				AllocThread( void ) = 0;
	virtual void				FreeThread( void ) = 0;
	virtual int					GetThreadId( void ) = 0;
	virtual int					GetThreadCount( void ) = 0;

								// Loads collision models from a map file.
	virtual void				LoadMap( const char* fileName, bool forceReload ) = 0;

								// Loads a collision model.
	virtual idCollisionModel*	LoadModel( const char *mapName, const char *modelName ) = 0;
								// Free the given model.
	virtual void				FreeModel( idCollisionModel *model ) = 0;
								// Purge all unused models.
	virtual void				PurgeModels( void ) = 0;

								// Sets up a trace model for collision with other trace models.
	virtual idCollisionModel *	ModelFromTrm( const char *mapName, const char *modelName, const idTraceModel &trm, bool includeBrushes ) = 0;
								// Creates a trace model from a collision model, returns true if succesfull.
	virtual bool				TrmFromModel( const char *mapName, const char *modelName, idTraceModel &trm ) = 0;
								// Creates a trace model for each primitive of the collision model, returns the number of trace models.
	virtual int					CompoundTrmFromModel( const char *mapName, const char *modelName, idTraceModel *trms, int maxTrms ) = 0;

	virtual void				CreateCollisionFromWorld( const collisionWorldFile_t& world ) = 0;

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
	virtual int					Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
									idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;

								// Tests collision detection.
	virtual void				DebugOutput( const idVec3 &viewOrigin, const idMat3 &viewAxis ) = 0;
								// Draws a model.
	virtual void				DrawModel( idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis,
												const idVec3 &viewOrigin, const idMat3 &viewAxis, const float radius, int lifetime ) = 0;
								// Lists all loaded models.
	virtual void				ListModels( void ) = 0;
								// Prints model information, use -1 for accumulated model info.
	virtual void				ModelInfo( int num ) = 0;

	virtual void				GetFullModelName( idStr& out, const char* mapName, const char* modelName ) const = 0;

	virtual void				DumpCollisionModelStats( void ) = 0;
};

extern idCollisionModelManager *collisionModelManager;

#endif /* !__COLLISIONMODELMANAGER_H__ */
