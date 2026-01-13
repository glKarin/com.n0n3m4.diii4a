
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
	PATHTYPE_BARRIERJUMP,
	PATHTYPE_JUMP
};

typedef struct aasPath_s {
	int							type;			// path type
	idVec3						moveGoal;		// point the AI should move towards
	int							moveAreaNum;	// number of the area the AI should move towards
	idVec3						secondaryGoal;	// secondary move goal for complex navigation
	const idReachability *		reachability;	// reachability used for navigation
} aasPath_t;


typedef struct aasGoal_s {
	int							areaNum;		// area the goal is in
	idVec3						origin;			// position of goal
} aasGoal_t;


typedef struct aasObstacle_s {
	idBounds					absBounds;		// absolute bounds of obstacle
	idBounds					expAbsBounds;	// expanded absolute bounds of obstacle
} aasObstacle_t;

class idAASCallback {
public:
	virtual	~idAASCallback ( void );
	
	enum testResult_t {
		TEST_OK,
		TEST_BADAREA,
		TEST_BADPOINT
	};
	
	virtual void				Init		( void );
	virtual void				Finish		( void );
	
	testResult_t				Test		( class idAAS *aas, int areaNum, const idVec3& origin, float minDistance, float maxDistance, const idVec3* point, aasGoal_t& goal );
	
protected:

	virtual bool				TestArea	( class idAAS *aas, int areaNum, const aasArea_t& area );
	virtual	bool				TestPoint	( class idAAS *aas, const idVec3& pos, const float zAllow=0.0f );
	
private:

	bool		TestPointDistance		( const idVec3& origin, const idVec3& point, float minDistance, float maxDistance );
};

typedef int aasHandle_t;

class idAAS {
public:
	static idAAS *				Alloc( void );
	virtual						~idAAS( void ) = 0;
								// Initialize for the given map.
	virtual bool				Init( const idStr &mapName, unsigned int mapFileCRC ) = 0;
// RAVEN BEGIN
// jscott: added
								// Prints out the memory used by this AAS
	virtual size_t				StatsSummary( void ) const = 0;
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual void				Shutdown( void ) = 0;
#endif
// RAVEN END
								// Print AAS stats.
	virtual void				Stats( void ) const = 0;
								// Test from the given origin.
	virtual void				Test( const idVec3 &origin ) = 0;
								// Get the AAS settings.
	virtual const idAASSettings *GetSettings( void ) const = 0;
								// Returns the number of the area the origin is in.
	virtual int					PointAreaNum( const idVec3 &origin ) const = 0;
								// Returns the number of the nearest reachable area for the given point.
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &bounds, const int areaFlags ) const = 0;
								// Returns the number of the first reachable area in or touching the bounds.
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const = 0;
								// Push the point into the area.
	virtual void				PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const = 0;
								// Returns a reachable point inside the given area.
	virtual idVec3				AreaCenter( int areaNum ) const = 0;
// RAVEN BEGIN
// bdube: added
								// Returns a reachable point inside the given area.
	virtual float				AreaRadius( int areaNum ) const = 0;
	virtual idBounds &			AreaBounds( int areaNum ) const = 0;
	virtual float				AreaCeiling( int areaNum ) const = 0;
// RAVEN END	
								// Returns the area flags.
	virtual int					AreaFlags( int areaNum ) const = 0;
								// Returns the travel flags for traveling through the area.
	virtual int					AreaTravelFlags( int areaNum ) const = 0;
								// Trace through the areas and report the first collision.
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
								// Get a plane for a trace.
	virtual const idPlane &		GetPlane( int planeNum ) const = 0;
								// Get wall edges.
	virtual int					GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, int *edges, int maxEdges ) const = 0;
								// Sort the wall edges to create continuous sequences of walls.
	virtual void				SortWallEdges( int *edges, int numEdges ) const = 0;
								// Get the vertex numbers for an edge.
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const = 0;
								// Get an edge.
	virtual void				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const = 0;
								// Find all areas within or touching the bounds with the given contents and disable/enable them for routing.
	virtual bool				SetAreaState( const idBounds &bounds, const int areaContents, bool disabled ) = 0;
								// Add an obstacle to the routing system.
	virtual aasHandle_t			AddObstacle( const idBounds &bounds ) = 0;
								// Remove an obstacle from the routing system.
	virtual void				RemoveObstacle( const aasHandle_t handle ) = 0;
								// Remove all obstacles from the routing system.
	virtual void				RemoveAllObstacles( void ) = 0;
								// Returns the travel time towards the goal area in 100th of a second.
	virtual int					TravelTimeToGoalArea( int areaNum, const idVec3 &origin, int goalAreaNum, int travelFlags ) const = 0;
								// Get the travel time and first reachability to be used towards the goal, returns true if there is a path.
	virtual bool				RouteToGoalArea( int areaNum, const idVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, idReachability **reach ) const = 0;
								// Creates a walk path towards the goal.
	virtual bool				WalkPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const = 0;
								// Returns true if one can walk along a straight line from the origin to the goal origin.
	virtual bool				WalkPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum ) const = 0;
								// Creates a fly path towards the goal.
	virtual bool				FlyPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const = 0;
								// Returns true if one can fly along a straight line from the origin to the goal origin.
	virtual bool				FlyPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum ) const = 0;
								// Show the walk path from the origin towards the area.
	virtual void				ShowWalkPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const = 0;
								// Show the fly path from the origin towards the area.
	virtual void				ShowFlyPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const = 0;
								// Find the nearest goal which satisfies the callback.
	virtual bool				FindNearestGoal( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, float minDistance, float maxDistance, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback ) const = 0;

// RAVEN BEGIN 
// CDR : Added Area Wall Extraction For AASTactical
	virtual idAASFile*			GetFile( void ) = 0;
// cdr: Alternate Routes Bug
	 virtual void				SetReachabilityState( idReachability* reach, bool enable ) = 0;

// rjohnson: added more debug drawing
	virtual void				ShowAreas( const idVec3 &origin, bool ShowProblemAreas = false ) const = 0;
	virtual bool				IsValid( void ) const = 0;
// RAVEN END
};

#endif /* !__AAS_H__ */
