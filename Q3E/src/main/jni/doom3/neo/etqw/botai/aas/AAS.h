// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __AAS_H__
#define __AAS_H__

/*
===============================================================================

	Area Awareness System

===============================================================================
*/

enum {
	PATHTYPE_WALK,
	PATHTYPE_WALKOFFLEDGE,
	PATHTYPE_WALKOFFBARRIER,
	PATHTYPE_BARRIERJUMP,
	PATHTYPE_JUMP,
	PATHTYPE_LADDER,
	PATHTYPE_TELEPORT,
	PATHTYPE_ELEVATOR,
	MAX_PATHTYPE
};

struct idAASPath {
	idAASPath() {
		type = MAX_PATHTYPE;
	}
	int							type;			// path type
	idVec3						moveGoal;		// point the AI should move towards
	int							moveAreaNum;	// number of the area the AI should move towards
	const aasReachability_t *	reachability;	// reachability used for navigation
	idVec3						viewGoal;		// a place for the bots to look at
	int							travelTime;		// how long to get from start to goal end
};

struct idAASHopPathParms {
	idAASHopPathParms() {
		maxDistance = 8192; //4096.0f; //mal: was 2048.0f;
		maxHeight = 2048.0f; //1024.0f;
		minHeight = 64.0f;
		maxSlope = 2.0f;
	}
	float						maxDistance;
	float						maxHeight;
	float						minHeight;
	float						maxSlope;
};

struct idAASPathPoint {
	int							areaNum;		// area path goes through
	idVec3						origin;			// position in this area
	const aasReachability_t *	next;			// reachability that goes to next point
	int							travelTime;		// travel time to this area
};

struct idAASGoal {
	int							areaNum;		// area the goal is in
	idVec3						origin;			// position of goal
};

class idAASCallback {
public:
	virtual						~idAASCallback() {};

	virtual bool				PathValid( const class idAAS *aas, const idVec3 &start, const idVec3 &end ) { return true; }
	virtual int					AdditionalTravelTimeForPath( const class idAAS *aas, const idVec3 &start, const idVec3 &end ) { return 0; }
	virtual	bool				AreaIsGoal( const class idAAS *aas, int areaNum ) = 0;
};

ID_INLINE bool					IsInObstaclePVS( const byte *pvs, int areaNum ) { return ( pvs[areaNum >> 3] & ( 1 << ( areaNum & 7 ) )) != 0; }

typedef int aasHandle_t;

class idAAS {
public:
	static idAAS *				Alloc( void );
	virtual						~idAAS( void ) = 0;

								// Initialize for the given map.
	virtual bool				Init( const char *mapName, unsigned int mapFileCRC ) = 0;
								// Print AAS stats.
	virtual void				Stats( void ) const = 0;
								// Test from the given origin.
	virtual void				Test( const idVec3 &origin ) = 0;
								// Get the AAS settings.
	virtual const idAASSettings *GetSettings( void ) const = 0;
								// Returns the number of the area the origin is in.
	virtual int					PointAreaNum( const idVec3 &origin ) const = 0;
								// Returns the number of the nearest reachable area for the given point.
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &bounds, const int areaFlags, int excludeTravelFlags ) const = 0;
								// Returns the number of the first reachable area in or touching the bounds.
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, int excludeTravelFlag ) const = 0;
								// Push the point into the area.
	virtual void				PushPointIntoArea( int areaNum, idVec3 &origin ) const = 0;
								// Returns a reachable point inside the given area.
	virtual idVec3				AreaCenter( int areaNum ) const = 0;
								// Trace through the areas and report the first collision.
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
								// Trace through columns and sample the height.
	virtual bool				TraceHeight( aasTraceHeight_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
								// Trace along the floor with step checking.
	virtual bool				TraceFloor( aasTraceFloor_t &trace, const idVec3 &start, int startAreaNum, const idVec3 &end, int travelFlags ) const = 0;
								// Decodes the obstacle PVS for the given area and returns a pointer to the decoded PVS.
	virtual const byte *		GetObstaclePVS( int areaNum ) const = 0;
								// Get wall edges in the obstacle PVS.
	virtual int					GetObstaclePVSWallEdges( int areaNum, int *edges, int maxEdges ) const = 0;
								// Get wall edges.
	virtual int					GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, float height, int *edges, int maxEdges ) const = 0;
								// Get the vertex numbers for an edge.
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const = 0;
								// Get an edge.
	virtual void				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const = 0;
								// Gets the flags for an edge.
	virtual int					GetEdgeFlags( int edgeNum ) const = 0;
								// Get the flags for an area.
	virtual int					GetAreaFlags( int areaNum ) const = 0;
								// Set the area travel flag
	virtual void				SetAreaTravelFlags( int areaNum, int travelFlags ) = 0;
								// Find all areas within or touching the bounds with the given area flags and set/remove the given travel flags.
	virtual bool				ChangeAreaTravelFlags( const idBounds &bounds, const int areaFlags, int travelFlags, bool set ) = 0;
								// Change the given travel flags on the reachability with the given name.
	virtual bool				ChangeReachabilityTravelFlags( const char *name, int travelFlags, bool set ) = 0;
								// Returns the travel time towards the goal area in 100th of a second.
	virtual int					TravelTimeToGoalArea( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, int travelFlags ) const = 0;
								// Get the travel time and first reachability to be used towards the goal, returns false if there is no path.
	virtual bool				RouteToGoalArea( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, int travelFlags, int &travelTime, const aasReachability_t **reach ) const = 0;
								// Find the nearest goal which satisfies the callback.
	virtual bool				FindNearestGoal( idAASGoal &goal, int startAreaNum, const idVec3 &startOrigin, int travelFlags, idAASCallback &callback ) const = 0;
								// Creates a walk path towards the goal.
	virtual bool				WalkPathToGoal( idAASPath &path, int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags ) const = 0;
								// Extend a walk path to allow for short parabolic flights.
	virtual bool				ExtendHopPathToGoal( idAASPath &path, int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags, const idAASHopPathParms &parms ) const = 0;
								// Show the walk path from the start to the goal. Note: this is not thread safe.
	virtual void				ShowWalkPath( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags ) const = 0;
								// Show the hop path from the start to the goal. Note: this is not thread safe.
	virtual void				ShowHopPath( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags, const idAASHopPathParms &parms ) const = 0;
								// draw the area specified
	virtual void				DrawArea( int areaNum ) const = 0;
};

#endif /* !__AAS_H__ */
