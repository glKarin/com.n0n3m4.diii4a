// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __AASFILE_H__
#define __AASFILE_H__

/*
===============================================================================

	AAS File

===============================================================================
*/

#define AAS_FILE_ID							"ETQWAAS"
#define AAS_FILE_ID_BINARY					"ETQWAASB"
#define AAS_FILE_VERSION					"2.2"

// travel flags
#define TFL_INVALID							BIT(0)		// invalid
#define TFL_INVALID_GDF						BIT(1)		// not valid for GDF
#define TFL_INVALID_STROGG					BIT(2)		// not valid for STROGG
#define TFL_AIR								BIT(3)		// travel through air
#define TFL_WATER							BIT(4)		// travel through water
#define TFL_WALK							BIT(5)		// walking
#define TFL_WALKOFFLEDGE					BIT(6)		// walking off a ledge
#define TFL_WALKOFFBARRIER					BIT(7)		// walking off a barrier
#define TFL_BARRIERJUMP						BIT(8)		// jumping onto a barrier
#define TFL_JUMP							BIT(9)		// jumping
#define TFL_LADDER							BIT(10)		// climbing a ladder
#define TFL_SWIM							BIT(11)		// swimming
#define TFL_WATERJUMP						BIT(12)		// jump out of the water
#define TFL_TELEPORT						BIT(13)		// teleportation
#define TFL_ELEVATOR						BIT(14)		// travel by elevator

#define TFL_VALID_GDF						( ~( TFL_INVALID | TFL_INVALID_GDF ) )
#define TFL_VALID_STROGG					( ~( TFL_INVALID | TFL_INVALID_STROGG ) )
#define TFL_VALID_GDF_AND_STROGG			( ~( TFL_INVALID ) )

#define TFL_VALID_WALK_GDF					( TFL_WALK | TFL_AIR | TFL_WATER | TFL_INVALID_STROGG )
#define TFL_VALID_WALK_STROGG				( TFL_WALK | TFL_AIR | TFL_WATER | TFL_INVALID_GDF )
#define TFL_VALID_WALK_GDF_AND_STROGG		( TFL_WALK | TFL_AIR | TFL_WATER | TFL_INVALID_GDF | TFL_INVALID_STROGG )

// edge flags
#define AAS_EDGE_WALL						BIT(0)		// wall
#define AAS_EDGE_LEDGE						BIT(1)		// ledge
#define AAS_EDGE_WALL_CORNER				BIT(2)		// edge extending from a wall corner
#define AAS_EDGE_LEDGE_CORNER				BIT(3)		// edge extending from a ledge corner
#define AAS_EDGE_STEP_TOP					BIT(4)		// step top edge
#define AAS_EDGE_STEP_BOTTOM				BIT(5)		// step bottom edge
#define AAS_EDGE_VERTICAL					BIT(6)		// mostly vertical edge
#define AAS_EDGE_WATER						BIT(7)		// water edge
#define AAS_EDGE_LADDER						BIT(8)		// ladder edge

// area flags
#define AAS_AREA_LEDGE						BIT(0)		// if entered the AI bbox partly floats above a ledge
#define AAS_AREA_REACHABLE_WALK				BIT(1)		// area is reachable by walking or swimming
#define AAS_AREA_OUTSIDE					BIT(2)		// area is outside
#define AAS_AREA_HIGH_CEILING				BIT(3)		// area has a ceiling that is high enough to perform certain movements
#define AAS_AREA_NOPUSH						BIT(4)		// push into area failed because the area winding is malformed
#define AAS_AREA_CONTENTS_SOLID				BIT(8)		// solid, not a valid area
#define AAS_AREA_CONTENTS_WATER				BIT(9)		// area contains water
#define AAS_AREA_CONTENTS_CLUSTERPORTAL		BIT(10)		// area is a cluster portal
#define AAS_AREA_CONTENTS_OBSTACLE			BIT(11)		// area contains (part of) a binary obstacle
#define AAS_AREA_CONTENTS_TELEPORTER		BIT(12)		// area contains (part of) a teleporter trigger
#define AAS_AREA_FLOOD_VISITED				BIT(15)		// area visited during a flood routine.  this is a temporary flag that should be removed before the routine exits

// node flags
#define AAS_NODE_FLAG_NONE					0
#define AAS_NODE_FLAG_FLOOR_PLANE			BIT(0)		// this node stores a floor plane
#define AAS_NODE_FLAG_COLUMN_HEIGHT			BIT(1)		// the top most bits of the flags are used to store a height
#define AAS_NODE_FLAG_COLUMN_HEIGHT_BITS	( sizeof( ((aasNode_t*)0)->flags ) * 8 - 2 )
#define AAS_NODE_FLAG_COLUMN_HEIGHT_SHIFT	( sizeof( ((aasNode_t*)0)->flags ) * 8 - AAS_NODE_FLAG_COLUMN_HEIGHT_BITS )
#define AAS_NODE_FLAG_COLUMN_HEIGHT_MAX		( ( 1 << AAS_NODE_FLAG_COLUMN_HEIGHT_BITS ) - 1 )
#define AAS_NODE_FLAG_COLUMN_HEIGHT_OFFSET	( ( 1 << ( AAS_NODE_FLAG_COLUMN_HEIGHT_BITS - 1 ) ) )

// obstacle PVS run-length encoding
#define AAS_PVS_RLE_IMMEDIATE_BITS			7
#define AAS_PVS_RLE_1ST_COUNT_BITS			6
#define AAS_PVS_RLE_2ND_COUNT_BITS			8
#define AAS_PVS_RLE_RUN_GRANULARITY			1
#define AAS_PVS_RLE_RUN_BIT					(1 << 7)
#define AAS_PVS_RLE_RUN_LONG_BIT			(1 << 6)

// area travel time offset and reachability number encoding
#define AAS_REACH_MAX_NUMBER_BITS			8
#define AAS_REACH_MAX_PER_AREA				( 1 << AAS_REACH_MAX_NUMBER_BITS )
#define AAS_REACH_NUMBER_SHIFT				( sizeof( ((aasReachability_t*)0)->areaTTOfsAndNumber ) * 8 - AAS_REACH_MAX_NUMBER_BITS )
#define AAS_REACH_NUMBER_MASK				( ( 1 << AAS_REACH_MAX_NUMBER_BITS ) - 1 )
#define AAS_AREA_TRAVEL_TIME_OFFSET_MASK	( ( 1 << AAS_REACH_NUMBER_SHIFT ) - 1 )

#define AAS_MAX_NAME_LENGTH					128
#define AAS_MAX_TREE_DEPTH					128

// index
typedef int aasIndex_t;

// plane
typedef idPlane aasPlane_t;

// vertex
typedef idVec3 aasVertex_t;

// edge
struct aasEdge_t {
	int							vertexNum[2];		// numbers of the vertexes of this edge
	int							flags;
};

// reachability to another area
struct aasReachability_t {
	unsigned short				travelFlags;		// type of travel required to get to the area
	unsigned short				travelTime;			// travel time of the inter area movement
	unsigned short				fromAreaNum;		// number of area the reachability starts
	unsigned short				toAreaNum;			// number of area the reachability leads to
	short						start[3];			// start point of inter area movement
	short						end[3];				// end point of inter area movement
	unsigned int				areaTTOfsAndNumber;	// travel times in fromAreaNum from reachabilities that lead towards this area to this reachability, and the reachability number
	aasReachability_t *			next;				// next reachability in list
	aasReachability_t *			rev_next;			// next reachability in reversed list

	// v is the vector, d is the direction to snap towards
	void						SetStart( const idVec3 &v, const idVec3 &d ) { for ( int i = 0; i < 3; i++ ) start[i] = idMath::Ftoi( v[i] + idMath::Rint( d[i] ) ); }
	void						SetEnd( const idVec3 &v, const idVec3 &d )   { for ( int i = 0; i < 3; i++ ) end[i]   = idMath::Ftoi( v[i] + idMath::Rint( d[i] ) ); }

	const idVec3				GetStart() const { return idVec3( start[0], start[1], start[2] ); }
	const idVec3				GetEnd() const { return idVec3( end[0], end[1], end[2] ); }
};

assert_sizeof( aasReachability_t, 32 );

// area for navigation
struct aasArea_t {
	unsigned short				travelFlags;		// travel flags for traveling through this area
	unsigned short				flags;				// several area flags
	int							numEdges;			// number of edges in the boundary of the face
	int							firstEdge;			// first edge in the edge index
	short						cluster;			// cluster the area belongs to, if negative it's a portal
	unsigned short				clusterAreaNum;		// number of the area in the cluster
	unsigned int				obstaclePVSOffset;	// offset into obstacle PVS
	aasReachability_t *			reach;				// reachabilities that start from this area
	aasReachability_t *			rev_reach;			// reachabilities that lead to this area
};

assert_sizeof( aasArea_t, 28 );

// nodes of the bsp tree
struct aasNode_t {
	unsigned short				planeNum;			// number of the plane that splits the subspace at this node
	unsigned short				flags;				// node flags
	int							children[2];		// child nodes, zero is solid, negative is -(area number)
};

assert_sizeof( aasNode_t, 12 );

// cluster portal
struct aasPortal_t {
	unsigned short				areaNum;			// number of the area that is the actual portal
	short						clusters[2];		// number of cluster at the front and back of the portal
	unsigned short				clusterAreaNum[2];	// number of this portal area in the front and back cluster
	unsigned short				maxAreaTravelTime;	// maximum travel time through the portal area
};

// cluster
struct aasCluster_t {
	int							numAreas;			// number of areas in the cluster
	int							numReachableAreas;	// number of areas with reachabilities
	int							numPortals;			// number of cluster portals
	int							firstPortal;		// first cluster portal in the index
};

// obstacle PVS
typedef byte aasObstaclePVS_t;

// names
struct aasName_t {
	char						name[AAS_MAX_NAME_LENGTH];
	int							index;
};

// trace through the world
struct aasTrace_t {
								aasTrace_t() { areas = NULL; points = NULL; getOutOfSolid = false; flags = travelFlags = maxAreas = 0; }

								// parameters
	int							flags;				// areas with these flags block the trace
	int							travelFlags;		// areas with these travel flags block the trace
	int							maxAreas;			// size of the 'areas' array
	int							getOutOfSolid;		// trace out of solid if the trace starts in solid
								// output
	float						fraction;			// fraction of trace completed
	idVec3						endpos;				// end position of trace
	int							planeNum;			// plane hit
	int							lastAreaNum;		// number of last area the trace went through
	int							blockingAreaNum;	// area that could not be entered
	int							numAreas;			// number of areas the trace went through
	int *						areas;				// array to store areas the trace went through
	idVec3 *					points;				// points where the trace entered each new area
};

struct aasTraceHeight_t {
	int							maxPoints;
	int							numPoints;			// number of areas the trace went through
	idVec3 *					points;				// points where the trace entered each new area
};

struct aasTraceFloor_t {
	float						fraction;			// fraction of trace completed
	idVec3						endpos;				// end position of trace
	int							lastAreaNum;		// number of last area the trace went through
	int							lastEdgeNum;		// number of last edge the trace went through or edge that stopped the trace
};

// settings
class idAASSettings {
public:

	enum type_t {
		AAS_PLAYER,
		AAS_VEHICLE
	};

	enum aasPrimitiveMode_t {
		AAS_PRIMITIVE_MODE_DEFAULT		= 0,		// compile if marked or part of the world
		AAS_PRIMITIVE_MODE_NEVER		= 1,		// never compile
		AAS_PRIMITIVE_MODE_ALWAYS		= 2,		// always compile
		AAS_PRIMITIVE_MODE_EXPLICIT		= 3			// compile only if the primitive is marked
	};

	int							type;
	idStr						fileExtensionAAS;

								// collision settings
	idBounds					boundingBox;
	int							primitiveModeBrush;
	int							primitiveModePatch;
	int							primitiveModeModel;
	int							primitiveModeTerrain;

								// physics settings
	idVec3						gravity;
	idVec3						gravityDir;
	idVec3						invGravityDir;
	float						gravityValue;
	float						maxStepHeight;
	float						maxBarrierHeight;
	float						maxWaterJumpHeight;
	float						maxFallHeight;
	float						minFloorCos;
	float						minHighCeiling;
	float						groundSpeed;				// in units per second
	float						waterSpeed;					// in units per second
	float						ladderSpeed;				// in units per second

								// navigation settings
	float						wallCornerEdgeRadius;
	float						ledgeCornerEdgeRadius;
	float						obstaclePVSRadius;

								// fixed travel times
	int							tt_barrierJump;
	int							tt_waterJump;
	int							tt_startWalkOffLedge;
	int							tt_startLadderClimb;

public:
								idAASSettings();

	bool						ReadFromDict( const char *name, const idDict *dict );

	bool						WriteToFile( idFile *fp ) const;
	bool						ReadFromFile( idLexer &src );

	bool						WriteToFile( const char *fileName ) const;
	bool						ReadFromFile( const char *fileName );

	bool						WriteToFileBinary( idFile *fp ) const;
	bool						ReadFromFileBinary( idFile *fp );

	bool						ValidForBounds( const idBounds &bounds ) const;
	bool						ValidEntity( const idMapFile *mapFile, const char *classname ) const;

private:
	bool						ParseBool( idLexer &src, bool &b );
	bool						ParseInt( idLexer &src, int &i );
	bool						ParseFloat( idLexer &src, float &f );
	bool						ParseVector( idLexer &src, idVec3 &vec );
	bool						ParseBounds( idLexer &src, idBounds &bounds );

	bool						ParseAASTypeKey( const idDict *dict, const char *keyName, int &aasType );
	bool						ParseBoundsKey( const idDict *dict, const char *keyName, idBounds &bounds );
	bool						ParsePrimitiveModeKey( const idDict *dict, const char *keyName, int &primitiveMode );
};


/*

-	when a node child is a solid leaf the node child number is zero
-	the edges are stored counter clockwise using the edgeindex
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	the cluster number set to the negative portal number
-	edge zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
-	portal zero is a dummy
-	cluster zero is a dummy

*/

class idAASFile {
public:
	virtual 					~idAASFile() {}

	const char *				GetName() const { return name.c_str(); }
	unsigned int				GetCRC() const { return crc; }

	int							GetNumPlanes() const { return planes.Num(); }
	const aasPlane_t &			GetPlane( int index ) const { return planes[index]; }
	int							GetNumVertices() const { return vertices.Num(); }
	const aasVertex_t &			GetVertex( int index ) const { return vertices[index]; }
	int							GetNumEdges() const { return edges.Num(); }
	const aasEdge_t &			GetEdge( int index ) const { return edges[index]; }
	int							GetNumEdgeIndexes() const { return edgeIndex.Num(); }
	const aasIndex_t &			GetEdgeIndex( int index ) const { return edgeIndex[index]; }
	int							GetNumReachabilities() const { return reachabilities.Num(); }
	const aasReachability_t &	GetReachability( int index ) const { return reachabilities[index]; }
	int							GetNumAreas() const { return areas.Num(); }
	const aasArea_t &			GetArea( int index ) { return areas[index]; }
	int							GetNumNodes() const { return nodes.Num(); }
	const aasNode_t &			GetNode( int index ) const { return nodes[index]; }
	int							GetNumPortals() const { return portals.Num(); }
	const aasPortal_t &			GetPortal( int index ) { return portals[index]; }
	int							GetNumPortalIndexes() const { return portalIndex.Num(); }
	const aasIndex_t &			GetPortalIndex( int index ) const { return portalIndex[index]; }
	int							GetNumClusters() const { return clusters.Num(); }
	const aasCluster_t &		GetCluster( int index ) const { return clusters[index]; }
	int							GetNumObstaclePVS() const { return obstaclePVS.Num(); }
	const aasObstaclePVS_t &	GetObstaclePVS( int index ) const { return obstaclePVS[index]; }
	int							GetNumReachabilityNames() const { return reachabilityNames.Num(); }
	const aasName_t &			GetReachabilityName( int index ) const { return reachabilityNames[index]; }

	const idAASSettings &		GetSettings() const { return settings; }

	void						SetPortalMaxTravelTime( int index, int time ) { portals[index].maxAreaTravelTime = time; }
	void						SetAreaTravelFlag( int index, int flag ) { areas[index].travelFlags |= flag; }
	void						RemoveAreaTravelFlag( int index, int flag ) { areas[index].travelFlags &= ~flag; }

	virtual int					FindReachabilityByName( const char *name ) const = 0;
	void						SetReachabilityTravelFlag( int index, int flag ) { reachabilities[index].travelFlags |= flag; }
	void						RemoveReachabilityTravelFlag( int index, int flag ) { reachabilities[index].travelFlags &= ~flag; }

	virtual size_t				MemorySize() const = 0;
	virtual size_t				MemoryUsed() const = 0;
	virtual size_t				MaxRoutingCacheSize() const = 0;
	virtual size_t				AreaTravelTimeCacheSize() const = 0;

	virtual idVec3				EdgeCenter( int edgeNum ) const = 0;
	virtual idVec3				AreaCenter( int areaNum ) const = 0;
	virtual idBounds			EdgeBounds( int edgeNum ) const = 0;
	virtual idBounds			AreaBounds( int areaNum ) const = 0;

	virtual int					PointAreaNum( const idVec3 &origin ) const = 0;
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual bool				PushPointIntoArea( int areaNum, idVec3 &point ) const = 0;

	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
	virtual bool				TraceHeight( aasTraceHeight_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
	virtual bool				TraceFloor( aasTraceFloor_t &trace, const idVec3 &start, int startAreaNum, const idVec3 &end, int endAreaNum, int travelFlags ) const = 0;

protected:
	idStr						name;
	unsigned int				crc;

	idAASSettings				settings;

	idList<aasPlane_t>			planes;
	idList<aasVertex_t>			vertices;
	idList<aasEdge_t>			edges;
	idList<aasIndex_t>			edgeIndex;
	idList<aasReachability_t>	reachabilities;
	idList<aasArea_t>			areas;
	idList<aasNode_t>			nodes;
	idList<aasPortal_t>			portals;
	idList<aasIndex_t>			portalIndex;
	idList<aasCluster_t>		clusters;
	idList<aasObstaclePVS_t>	obstaclePVS;
	idList<aasName_t>			reachabilityNames;
};

#endif /* !__AASFILE_H__ */
