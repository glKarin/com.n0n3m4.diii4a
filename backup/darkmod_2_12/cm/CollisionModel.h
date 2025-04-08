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
typedef enum {
	CONTACT_NONE,							// no contact
	CONTACT_EDGE,							// trace model edge hits model edge
	CONTACT_MODELVERTEX,					// model vertex hits trace model polygon
	CONTACT_TRMVERTEX						// trace model vertex hits model polygon
} contactType_t;

// contact info
struct contactInfo_t {
	contactType_t			type;			// contact type
	idVec3					point;			// point of contact
	idVec3					normal;			// contact plane normal
	float					dist;			// contact plane distance
	int						contents;		// contents at other side of surface
	const idMaterial *		material;		// surface material
	int						modelFeature;	// contact feature on model
	int						trmFeature;		// contact feature on trace model
	int						entityNum;		// entity the contact surface is a part of
	int						id;				// id of clip model the contact surface is part of
};

// trace result
typedef struct trace_s {
	float					fraction;		// fraction of movement completed, 1.0 = didn't hit anything
	idVec3					endpos;			// final position of trace model
	idMat3					endAxis;		// final axis of trace model
	contactInfo_t			c;				// contact information, only valid if fraction < 1.0
} trace_t;

typedef int cmHandle_t;

#define CM_CLIP_EPSILON		0.25f			// always stay this distance away from any model
#define CM_BOX_EPSILON		1.0f			// should always be larger than clip epsilon
#define CM_MAX_TRACE_DIST	4096.0f			// maximum distance a trace model may be traced, point traces are unlimited

class idCollisionModelManager {
public:
	virtual					~idCollisionModelManager( void ) {}

	// Loads collision models from a map file.
	virtual void			LoadMap( const idMapFile *mapFile ) = 0;
	// Frees all the collision models.
	virtual void			FreeMap( void ) = 0;

	// Gets the clip handle for a model.
	virtual cmHandle_t		LoadModel( const char *modelName, const bool precache, const idDeclSkin* skin = NULL ) = 0; // skin added #4232 SteveL
	// Sets up a trace model for collision with other trace models.
	virtual cmHandle_t		SetupTrmModel( const idTraceModel &trm, const idMaterial *material ) = 0;
	// Creates a trace model from a collision model, returns true if succesfull.
	virtual bool			TrmFromModel( const char *modelName, idTraceModel &trm ) = 0;

	// Gets the name of a model.
	virtual const char *	GetModelName( cmHandle_t model ) const = 0; // NB will include ~skin_name if model is skinned #4232 SteveL
	// Gets the bounds of a model.
	virtual bool			GetModelBounds( cmHandle_t model, idBounds &bounds ) const = 0;
	// Gets all contents flags of brushes and polygons of a model ored together.
	virtual bool			GetModelContents( cmHandle_t model, int &contents ) const = 0;
	// Gets a vertex of a model.
	virtual bool			GetModelVertex( cmHandle_t model, int vertexNum, idVec3 &vertex ) const = 0;
	// Gets an edge of a model.
	virtual bool			GetModelEdge( cmHandle_t model, int edgeNum, idVec3 &start, idVec3 &end ) const = 0;
	// Gets a polygon of a model.
	virtual bool			GetModelPolygon( cmHandle_t model, int polygonNum, idFixedWinding &winding ) const = 0;

	// Translates a trace model and reports the first collision if any.
	virtual void			Translation( trace_t *results, const idVec3 &start, const idVec3 &end,
								const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;
	// Rotates a trace model and reports the first collision if any.
	virtual void			Rotation( trace_t *results, const idVec3 &start, const idRotation &rotation,
								const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;
	// Returns the contents touched by the trace model or 0 if the trace model is in free space.
	virtual int				Contents( const idVec3 &start,
								const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;
	// Stores all contact points of the trace model with the model, returns the number of contacts.
	virtual int				Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
								const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) = 0;

	// Tests collision detection.
	virtual void			DebugOutput( const idVec3 &origin ) = 0;
	// Draws a model.
	virtual void			DrawModel( cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis,
												const idVec3 &viewOrigin, const float radius ) = 0;
	// Prints model information, use -1 handle for accumulated model info.
	virtual void			ModelInfo( cmHandle_t model ) = 0;
	// Lists all loaded models.
	virtual void			ListModels( void ) = 0;
	// Writes a collision model file for the given map entity.
	virtual bool			WriteCollisionModelForMapEntity( const idMapEntity *mapEnt, const char *filename, const bool testTraceModel = true ) = 0;
};

extern idCollisionModelManager *		collisionModelManager;

#endif /* !__COLLISIONMODELMANAGER_H__ */
