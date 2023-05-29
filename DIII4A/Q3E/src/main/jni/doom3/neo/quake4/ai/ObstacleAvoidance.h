// Copyright (C) 2007 Id Software, Inc.
//

#ifndef	__OBSTACLEAVOIDANCE_H__
#define	__OBSTACLEAVOIDANCE_H__

/*
================================================================================

idObstacleAvoidance

================================================================================
*/

class idAAS;

class idObstacleAvoidance
{
public:
	static const int OBSTACLE_ID_INVALID	= -1;

	struct obstaclePath_t
	{
		idVec3					seekPos;					// seek position avoiding obstacles
		idVec3					originalSeekPos;			// original, non-capped seek pos
		float					targetDist;					// how close the path around obstacles gets to the target
		int						firstObstacle;				// if != OBSTACLE_ID_INVALID the first obstacle along the path
		idVec3					startPosOutsideObstacles;	// start position outside obstacles
		int						startPosObstacle;			// if != OBSTACLE_ID_INVALID the obstacle containing the start position
		idVec3					seekPosOutsideObstacles;	// seek position outside obstacles
		int						seekPosObstacle;			// if != OBSTACLE_ID_INVALID the obstacle containing the seek position
		float					pathLength;					// length of the path
		bool					hasValidPath;				//mal: did we find a valid path?
	};


public:
	idObstacleAvoidance( void ) { }
	~idObstacleAvoidance( void )
	{
		pathNodeAllocator.Shutdown();
	}

	void		ClearObstacles( void );
	void		AddObstacle( const idBox& box, int id );
	void		RemoveObstacle( int id );

	bool		FindPathAroundObstacles( const idBounds& bounds, const float radius, const idAAS* aas, const idVec3& startPos, const idVec3& seekPos, obstaclePath_t& path, bool alwaysAvoidObstacles = false );
	bool		PointInObstacles( const idBounds& bounds, const float radius, const idAAS* aas, const idVec3& pos );

	bool		SaveLastQuery( const char* fileName );
	bool		TestQuery( const char* fileName, const idAAS* aas );

private:
	static const float	PUSH_OUTSIDE_OBSTACLES;
	static const float	CLIP_BOUNDS_EPSILON;
	static const int 	MAX_AAS_WALL_EDGES			= 256;
	static const int	MAX_PATH_NODES				= 256;
	static const int 	MAX_OBSTACLE_PATH			= 128;

	struct obstacle_t
	{
		idBox				box;			// oriented bounding box representing the obstacle
		idVec2				bounds[2];		// two-dimensional bounds (in x-y plane) for obstacle
		idWinding2D			winding;		// two-dimensional winding (in x-y plane) for obstacle
		int					id;				// obstacle id
	};

	struct pathNode_t
	{
		int						dir;
		idVec2					pos;
		idVec2					delta;
		float					dist;
		int						obstacle;
		int						edgeNum;
		int						numNodes;
		pathNode_t* 			parent;
		pathNode_t* 			children[2];
		idQueueNode<pathNode_t>	queueNode;
		void					Init();
	};

	struct query_t
	{
		idBounds			bounds;
		float				radius;
		idVec3				startPos;
		idVec3				seekPos;
	};

private:
	bool		LineIntersectsPath( const idVec2& start, const idVec2& end, const pathNode_t* node ) const;
	int			PointInsideObstacle( const idVec2& point ) const;
	bool		LineIntersectsWall( const idVec2& start, const idVec2& end ) const;
	void		GetPointOutsideObstacles( idVec2& point, int* obstacle, int* edgeNum );
	bool		GetFirstBlockingObstacle( int skipObstacle, const idVec2& startPos, const idVec2& delta, float& blockingScale, int& blockingObstacle, int& blockingEdgeNum );
	void		ProjectTopDown( idVec3& point ) const;
	void		DrawBox() const;
	int			GetObstacles( const idBounds& bounds, float radius, const idAAS* aas, int areaNum, const idVec3& startPos, const idVec3& seekPos, idBounds& clipBounds );
	void		FreePathTree_r( pathNode_t* node );
	void		DrawPathTree( const pathNode_t* root, const float height );
	bool		GetPathNodeDelta( pathNode_t* node, const idVec2& seekPos, bool blocked );
	pathNode_t*	BuildPathTree( const idBounds& clipBounds, const idVec2& startPos, const idVec2& seekPos, obstaclePath_t& path );
	void		PrunePathTree( pathNode_t* root, const idVec2& seekPos );
	int			OptimizePath( const pathNode_t* root, const pathNode_t* leafNode, idVec2 optimizedPath[MAX_OBSTACLE_PATH] );
	float		PathLength( const idVec2 optimizedPath[MAX_OBSTACLE_PATH], int numPathPoints, const idVec2& curDir );
	void		FindOptimalPath( const pathNode_t* root, const float height, const idVec3& curDir, idVec3& seekPos, float& targetDist, float& pathLength );

private:
	idList<obstacle_t>				obstacles;
	idBlockAlloc<pathNode_t, 128/*, 0 //k*/>	pathNodeAllocator;

	query_t							lastQuery;
};

#endif	/* __OBSTACLEAVOIDANCE_H__ */
