
#ifndef __AASFILE_H__
#define __AASFILE_H__

/*
===============================================================================

	AAS File

===============================================================================
*/

#define AAS_FILEID					"DewmAAS"
#define AAS_FILEVERSION				"1.08"

// travel flags
#define TFL_INVALID					BIT(0)		// not valid
#define TFL_WALK					BIT(1)		// walking
#define TFL_CROUCH					BIT(2)		// crouching
#define TFL_WALKOFFLEDGE			BIT(3)		// walking of a ledge
#define TFL_BARRIERJUMP				BIT(4)		// jumping onto a barrier
#define TFL_JUMP					BIT(5)		// jumping
#define TFL_LADDER					BIT(6)		// climbing a ladder
#define TFL_SWIM					BIT(7)		// swimming
#define TFL_WATERJUMP				BIT(8)		// jump out of the water
#define TFL_TELEPORT				BIT(9)		// teleportation
#define TFL_ELEVATOR				BIT(10)		// travel by elevator
#define TFL_FLY						BIT(11)		// fly
#define TFL_SPECIAL					BIT(12)		// special
#define TFL_WATER					BIT(21)		// travel through water
#define TFL_AIR						BIT(22)		// travel through air

// face flags
#define FACE_SOLID					BIT(0)		// solid at the other side
#define FACE_LADDER					BIT(1)		// ladder surface
#define FACE_FLOOR					BIT(2)		// standing on floor when on this face
#define FACE_LIQUID					BIT(3)		// face seperating two areas with liquid
#define FACE_LIQUIDSURFACE			BIT(4)		// face seperating liquid and air

// area flags
#define AREA_FLOOR					BIT(0)		// AI can stand on the floor in this area
#define AREA_GAP					BIT(1)		// area has a gap
#define AREA_LEDGE					BIT(2)		// if entered the AI bbox partly floats above a ledge
#define AREA_LADDER					BIT(3)		// area contains one or more ladder faces
#define AREA_LIQUID					BIT(4)		// area contains a liquid
#define AREA_CROUCH					BIT(5)		// AI cannot walk but can only crouch in this area
#define AREA_REACHABLE_WALK			BIT(6)		// area is reachable by walking or swimming
#define AREA_REACHABLE_FLY			BIT(7)		// area is reachable by flying

// area contents flags
#define AREACONTENTS_SOLID			BIT(0)		// solid, not a valid area
#define AREACONTENTS_WATER			BIT(1)		// area contains water
#define AREACONTENTS_CLUSTERPORTAL	BIT(2)		// area is a cluster portal
#define AREACONTENTS_OBSTACLE		BIT(3)		// area contains (part of) a dynamic obstacle
#define AREACONTENTS_TELEPORTER		BIT(4)		// area contains (part of) a teleporter trigger

// bits for different bboxes
#define AREACONTENTS_BBOX_BIT		24


// RAVEN BEGIN
// cdr: AASTactical 

// feature bits
#define FEATURE_COVER				BIT(0)		// provides cover
#define FEATURE_LOOK_LEFT			BIT(1)		// attack by leaning left
#define FEATURE_LOOK_RIGHT			BIT(2)		// attack by leaning right
#define FEATURE_LOOK_OVER			BIT(3)		// attack by leaning over the cover
#define FEATURE_CORNER_LEFT			BIT(4)		// is a left corner
#define FEATURE_CORNER_RIGHT		BIT(5)		// is a right corner
#define FEATURE_PINCH				BIT(6)		// is a tight area connecting two larger areas
#define FEATURE_VANTAGE				BIT(7)		// provides a good view of the sampled area as a whole

// forward reference of sensor object
struct rvAASTacticalSensor;
struct rvMarker;

// RAVEN END


#define MAX_REACH_PER_AREA			256
#define MAX_AAS_TREE_DEPTH			128

#define MAX_AAS_BOUNDING_BOXES		4

typedef enum {

	RE_WALK,
	RE_WALKOFFLEDGE,
	RE_FLY,
	RE_SWIM,
	RE_WATERJUMP,
	RE_BARRIERJUMP,
	RE_SPECIAL
};


// reachability to another area
class idReachability {
public:
	int							travelType;			// type of travel required to get to the area
	short						toAreaNum;			// number of the reachable area
	short						fromAreaNum;		// number of area the reachability starts
	idVec3						start;				// start point of inter area movement
	idVec3						end;				// end point of inter area movement
	int							edgeNum;			// edge crossed by this reachability
	unsigned short				travelTime;			// travel time of the inter area movement
	byte						number;				// reachability number within the fromAreaNum (must be < 256)
	byte						disableCount;		// number of times this reachability has been disabled
	idReachability *			next;				// next reachability in list
	idReachability *			rev_next;			// next reachability in reversed list
	unsigned short *			areaTravelTimes;	// travel times within the fromAreaNum from reachabilities that lead towards this area
};

class idReachability_Walk : public idReachability {
};

class idReachability_BarrierJump : public idReachability {
};

class idReachability_WaterJump : public idReachability {
};

class idReachability_WalkOffLedge : public idReachability {
};

class idReachability_Swim : public idReachability {
};

class idReachability_Fly : public idReachability {
};

class idReachability_Special : public idReachability {
	friend class idAASFileLocal;
private:
	idDict						dict;
};

// index
typedef int aasIndex_t;

// vertex
typedef idVec3 aasVertex_t;

// edge
typedef struct aasEdge_s {
	int							vertexNum[2];		// numbers of the vertexes of this edge
} aasEdge_t;

// area boundary face
typedef struct aasFace_s {
	unsigned short				planeNum;			// number of the plane this face is on
	unsigned short				flags;				// face flags
	int							numEdges;			// number of edges in the boundary of the face
	int							firstEdge;			// first edge in the edge index
	short						areas[2];			// area at the front and back of this face
} aasFace_t;

// area with a boundary of faces
typedef struct aasArea_s {
	int							numFaces;			// number of faces used for the boundary of the area
	int							firstFace;			// first face in the face index used for the boundary of the area
	idBounds					bounds;				// bounds of the area
	idVec3						center;				// center of the area an AI can move towards
	float						ceiling;			// top of the area
	unsigned short				flags;				// several area flags
	unsigned short				contents;			// contents of the area
	short						cluster;			// cluster the area belongs to, if negative it's a portal
	short						clusterAreaNum;		// number of the area in the cluster
	int							travelFlags;		// travel flags for traveling through this area
	idReachability *			reach;				// reachabilities that start from this area
	idReachability *			rev_reach;			// reachabilities that lead to this area

	// RAVEN BEGIN
	// cdr: AASTactical
	unsigned short				numFeatures;		// number of features in this area
	unsigned short				firstFeature;		// first feature in the feature index within this area

	// cdr: Obstacle Avoidance
	rvMarker*					firstMarker;		// first obstacle avoidance threat in this area (0 if none)
	// RAVEN END
} aasArea_t;

// nodes of the bsp tree
typedef struct aasNode_s {
	unsigned short				planeNum;			// number of the plane that splits the subspace at this node
	int							children[2];		// child nodes, zero is solid, negative is -(area number)
} aasNode_t;

// cluster portal
typedef struct aasPortal_s {
	short						areaNum;			// number of the area that is the actual portal
	short						clusters[2];		// number of cluster at the front and back of the portal
	short						clusterAreaNum[2];	// number of this portal area in the front and back cluster
	unsigned short				maxAreaTravelTime;	// maximum travel time through the portal area
} aasPortal_t;

// cluster
typedef struct aasCluster_s {
	int							numAreas;			// number of areas in the cluster
	int							numReachableAreas;	// number of areas with reachabilities
	int							numPortals;			// number of cluster portals
	int							firstPortal;		// first cluster portal in the index
} aasCluster_t;

// RAVEN BEGIN
// cdr: AASTactical
typedef	struct aasFeature_s {
	short						x;					// 2 Bytes
	short						y;					// 2 Bytes
	short						z;					// 2 Bytes
	unsigned short				flags;				// 2 Bytes
	unsigned char				normalx;			// 1 Byte
	unsigned char				normaly;			// 1 Byte
	unsigned char				height;				// 1 Byte
	unsigned char				weight;				// 1 Byte

	idVec3&			Normal();
	idVec3&			Origin();

	void			DrawDebugInfo( int index=-1 );
  	int				GetLookPos( idVec3& lookPos, const idVec3& aimAtOrigin, const float leanDistance=16.0f );
} aasFeature_t;										//--------------------------------
													// 12 Bytes
// RAVEN END

// trace through the world
typedef struct aasTrace_s {
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
								aasTrace_s( void ) { areas = NULL; points = NULL; getOutOfSolid = false; flags = travelFlags = maxAreas = 0; }
} aasTrace_t;

// settings
class idAASSettings {
public:
								// collision settings
	int							numBoundingBoxes;
	idBounds					boundingBoxes[MAX_AAS_BOUNDING_BOXES];
	bool						usePatches;
	bool						writeBrushMap;
	bool						playerFlood;
	bool						noOptimize;
	bool						allowSwimReachabilities;
	bool						allowFlyReachabilities;
// RAVEN BEGIN
// bkreimeier
	bool						generateAllFaces;
// cdr: AASTactical
	bool						generateTacticalFeatures;
// scork: AASOnly numbers
	int							iAASOnly;	// 0, else 32,48,96,250 or -1 for all
// RAVEN END
	idStr						fileExtension;
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
								// fixed travel times
	int							tt_barrierJump;
	int							tt_startCrouching;
	int							tt_waterJump;
	int							tt_startWalkOffLedge;

// RAVEN BEGIN 
// rjohnson: added more debug drawing
	idVec4						debugColor;
	bool						debugDraw;
// RAVEN END

public:
								idAASSettings( void );

	bool						FromFile( const idStr &fileName );
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
	bool						FromParser( Lexer &src );
// RAVEN END
	bool						FromDict( const char *name, const idDict *dict );
	bool						WriteToFile( idFile *fp ) const;
	bool						ValidForBounds( const idBounds &bounds ) const;
	bool						ValidEntity( const char *classname, bool* needFlyReachabilities=NULL ) const;

// RAVEN BEGIN
	float						Radius( float scale=1.0f ) const;
// RAVEN END


private:
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
	bool						ParseBool( Lexer &src, bool &b );
	bool						ParseInt( Lexer &src, int &i );
	bool						ParseFloat( Lexer &src, float &f );
	bool						ParseVector( Lexer &src, idVec3 &vec );
	bool						ParseBBoxes( Lexer &src );
// RAVEN END
};


/*

-	when a node child is a solid leaf the node child number is zero
-	two adjacent areas (sharing a plane at opposite sides) share a face
	this face is a portal between the areas
-	when an area uses a face from the faceindex with a positive index
	then the face plane normal points into the area
-	the face edges are stored counter clockwise using the edgeindex
-	two adjacent convex areas (sharing a face) only share One face
	this is a simple result of the areas being convex
-	the areas can't have a mixture of ground and gap faces
	other mixtures of faces in one area are allowed
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	the cluster number set to the negative portal number
-	edge zero is a dummy
-	face zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
-	portal zero is a dummy
-	cluster zero is a dummy

*/
typedef struct sizeEstimate_s {
	int			numEdgeIndexes;
	int			numFaceIndexes;
	int			numAreas;
	int			numNodes;
} sizeEstimate_t;


class idAASFile {
public:	
	virtual 						~idAASFile( void ) {}
// RAVEN BEGIN
// jscott: made pure virtual
	virtual class idAASFile	*		CreateNew( void ) = 0;
	virtual class idAASSettings *	CreateAASSettings( void ) = 0;
	virtual class idReachability *	CreateReachability( int type ) = 0;
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
	virtual bool					FromParser( class idAASSettings *edit, Lexer &src ) = 0;
// RAVEN END

	virtual	const char *			GetName( void ) const = 0;
	virtual	unsigned int			GetCRC( void ) const = 0;
	virtual	void					SetSizes( sizeEstimate_t size ) = 0;

	virtual	int						GetNumPlanes( void ) const = 0;
	virtual	idPlane &				GetPlane( int index ) = 0;
	virtual int						FindPlane( const idPlane &plane, const float normalEps, const float distEps ) = 0;

	virtual	int						GetNumVertices( void ) const = 0;
	virtual	aasVertex_t &			GetVertex( int index ) = 0;
	virtual int						AppendVertex( aasVertex_t &vert ) = 0;

	virtual	int						GetNumEdges( void ) const = 0;
	virtual	aasEdge_t &				GetEdge( int index ) = 0;
	virtual int						AppendEdge( aasEdge_t &edge ) = 0;

	virtual	int						GetNumEdgeIndexes( void ) const = 0;
	virtual	aasIndex_t &			GetEdgeIndex( int index ) = 0;
	virtual int						AppendEdgeIndex( aasIndex_t &edgeIdx ) = 0;

	virtual	int						GetNumFaces( void ) const = 0;
	virtual	aasFace_t &				GetFace( int index ) = 0;
	virtual int						AppendFace( aasFace_t &face ) = 0;

	virtual	int						GetNumFaceIndexes( void ) const = 0;
	virtual	aasIndex_t &			GetFaceIndex( int index ) = 0;
	virtual int						AppendFaceIndex( aasIndex_t &faceIdx ) = 0;

	virtual	int						GetNumAreas( void ) const = 0;
	virtual	aasArea_t &				GetArea( int index ) = 0;
	virtual int						AppendArea( aasArea_t &area ) = 0;

	virtual	int						GetNumNodes( void ) const = 0;
	virtual	aasNode_t &				GetNode( int index ) = 0;
	virtual int						AppendNode( aasNode_t &node ) = 0;
	virtual void					SetNumNodes( int num ) = 0;

	virtual	int						GetNumPortals( void ) const = 0;
	virtual	aasPortal_t &			GetPortal( int index ) = 0;
	virtual int						AppendPortal( aasPortal_t &portal ) = 0;

	virtual	int						GetNumPortalIndexes( void ) const = 0;
	virtual	aasIndex_t &			GetPortalIndex( int index ) = 0;
	virtual int						AppendPortalIndex( aasIndex_t &portalIdx, int clusterNum ) = 0;

	virtual	int						GetNumClusters( void ) const = 0;
	virtual	aasCluster_t &			GetCluster( int index ) = 0;
	virtual int						AppendCluster( aasCluster_t &cluster ) = 0;

	// RAVEN BEGIN
	// cdr: AASTactical
	virtual void					ClearTactical( void ) = 0;

	virtual	int						GetNumFeatureIndexes( void ) const = 0;
	virtual	aasIndex_t &			GetFeatureIndex( int index ) = 0;
	virtual int						AppendFeatureIndex( aasIndex_t &featureIdx ) = 0;

	virtual	int						GetNumFeatures( void ) const = 0;
	virtual	aasFeature_t &			GetFeature( int index ) = 0;
	virtual int						AppendFeature( aasFeature_t &cluster ) = 0;
	// RAVEN END

	virtual	idAASSettings &			GetSettings( void ) = 0;
	virtual	void					SetSettings( const idAASSettings &in ) = 0;

	virtual void					SetPortalMaxTravelTime( int index, int time ) = 0;
	virtual void					SetAreaTravelFlag( int index, int flag ) = 0;
	virtual void					RemoveAreaTravelFlag( int index, int flag ) = 0;
// RAVEN END

	virtual idVec3					EdgeCenter( int edgeNum ) const = 0;
	virtual idVec3					FaceCenter( int faceNum ) const = 0;
	virtual idVec3					AreaCenter( int areaNum ) const = 0;

	virtual idBounds				EdgeBounds( int edgeNum ) const = 0;
	virtual idBounds				FaceBounds( int faceNum ) const = 0;
	virtual idBounds				AreaBounds( int areaNum ) const = 0;

	virtual int						PointAreaNum( const idVec3 &origin ) const = 0;
	virtual int						PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual int						BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual void					PushPointIntoAreaNum( int areaNum, idVec3 &point ) const = 0;
	virtual bool					Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
	virtual void					PrintInfo( void ) const = 0;
// RAVEN BEGIN
// jscott: added
	virtual size_t					GetMemorySize( void ) = 0;

	virtual	void					Init( void ) = 0;
	virtual bool					Load( const idStr &fileName, unsigned int mapFileCRC ) = 0;
	virtual	bool					Write( const idStr &fileName, unsigned int mapFileCRC ) = 0;
	virtual	void					Clear( void ) = 0;
	virtual	void					FinishAreas( void ) = 0;
	virtual	void					ReportRoutingEfficiency( void ) const = 0;
	virtual	void					LinkReversedReachability( void ) = 0;
	virtual	void					DeleteReachabilities( void ) = 0;
	virtual	void					DeleteClusters( void ) = 0;
	virtual	void					Optimize( void ) = 0;
	virtual bool					IsDummyFile( unsigned int mapFileCRC ) = 0;
// RAVEN END

	virtual const idDict &			GetReachabilitySpecialDict( idReachability *reach ) const = 0;
	virtual void					SetReachabilitySpecialDictKeyValue( idReachability *reach, const char *key, const char *value ) = 0;
};

// RAVEN BEGIN
extern idAASFile		*AASFile;
// RAVEN END

#endif /* !__AASFILE_H__ */
