#ifndef __BOTRIX_WAYPOINT_H__
#define __BOTRIX_WAYPOINT_H__


#include <good/bitset.h>
#include <good/graph.h>

#include "mod.h"
#include "source_engine.h"
#include "types.h"

#include "vector.h"


//****************************************************************************************************************
/// Class that represents waypoint on a map.
//****************************************************************************************************************
class CWaypoint
{
public: // Members and constants.
    static const int MIN_ANGLE_PITCH = -64;         ///< Minimum pitch that can be used in angle argument.
    static const int MAX_ANGLE_PITCH = +63;         ///< Maximun pitch that can be used in angle argument.
    static const int MIN_ANGLE_YAW = -180;          ///< Minimum yaw that can be used in angle argument.
    static const int MAX_ANGLE_YAW = +180;          ///< Maximun yaw that can be used in angle argument.

    static const int DRAW_INTERVAL = 1;             ///< Waypoint interval for drawing in seconds.
    static const int WIDTH = 8;                     ///< Waypoint width for drawing.
    static const int PATH_WIDTH = 4;                ///< Waypoint's path width for drawing.

    static const int MAX_RANGE = 256;               ///< Max waypoint range to invalidate current waypoint.

    static int iWaypointTexture;                    ///< Texture of waypoint for beam. Precached at CWaypoints::Load().

    static int iDefaultDistance;                    ///< Max waypoint distance to automatically add path to nearby waypoints.
    
    static int iUnreachablePathFailuresToDelete;    ///< Max failures to erase the failed path.

    static int iAnalyzeDistance;                    ///< Distance between waypoints when analyzing a map.
    static int iWaypointsMaxCountToAnalyzeMap;      ///< Maximum waypoints count to start analyzing a map.
    static float fAnalyzeWaypointsPerFrame;         ///< Positions per frame to analyze.
    static bool bShowAnalyzePotencialWaypoints;     ///< Show potencial waypoints with white line. Show from console command, hide from map change.

    static bool bSaveOnMapChange;                   ///< Save waypoints on map change.

    /// Trace against all entities in analyze. The issue is that ray tracing sometimes returns some entity (like box shell or object)
    /// instead of hit world. By enabling all trace, we make sure that we won't be adding waypoints at invalid positions.
    static bool bAnalyzeTraceAll;                   

    Vector vOrigin;                                 ///< Coordinates of waypoint (x, y, z).
    TWaypointFlags iFlags;                          ///< Waypoint flags.
    TWaypointArgument iArgument;                                  ///< Waypoint argument (button number).
    TAreaId iAreaId;                                ///< Area id where waypoint belongs to (like "Bombsite A" / "Base CT" in counter-strike).

public: // Methods.
    /// Default constructor.
    CWaypoint(): vOrigin(), iFlags(0), iArgument(0), iAreaId(0) {}

    /// Constructor with parameters.
    CWaypoint( const Vector& vOrigin, int iFlags = FWaypointNone, int iArgument = 0, TAreaId iAreaId = 0 ):
        vOrigin(vOrigin), iFlags(iFlags), iArgument(iArgument), iAreaId(iAreaId) {}

	/// Return true if waypoint id can be valid. Use CWaypoints::IsValid() to actualy verify waypoint range.
	static inline bool IsValid( TWaypointId id ) { return ( id >= 0 ); }

	/// Get waypoint flags for needed entity type (health, armor, weapon, ammo).
    static TWaypointFlags GetFlagsFor( TItemType iEntityType )
    {
        if ( iEntityType < EItemTypeCanPickTotal )
            return m_aFlagsForEntityType[ iEntityType ];
        else
            return FWaypointNone;
    }

    /// Get first angle from waypoint argument.
    static void GetFirstAngle( QAngle& a, int iArgument )
    {
        int iPitch = (iArgument << 16) >> (16+9);   // 7 higher bits of low word (-64..63).
        int iYaw = (iArgument << (16+7)) >> (16+7); // 9 lower bits of low word (-256..255 but used -180..+180).
        a.x = iPitch; a.y = iYaw; a.z = 0;
    }

    /// Get first angle from waypoint argument.
    static void SetFirstAngle( int iPitch, int iYaw, int& iArgument )
    {
        //BASSERT( -64 <= iPitch && iPitch <= 63 && -128 <= iYaw && iYaw <= 128 );
        SET_1ST_WORD( ((iPitch & 0x7F) << 9) | (iYaw &0x1FF), iArgument );
    }

    /// Get second angle from waypoint argument.
    static void GetSecondAngle( QAngle& a, int iArgument )
    {
        int iPitch = iArgument >> (16+9);   // 7 higher bits of high word (-64..63).
        int iYaw = (iArgument << 7) >> (16+7); // 9 lower bits of low word (-256..255 but used -180..+180).
        a.x = iPitch; a.y = iYaw; a.z = 0;
    }

    /// Get second angle from waypoint argument.
    static void SetSecondAngle( int iPitch, int iYaw, int& iArgument )
    {
        //BASSERT( -64 <= iPitch && iPitch <= 63 && -128 <= iYaw && iYaw <= 128 );
        SET_2ND_WORD( ((iPitch & 0x7F) << 9) | (iYaw &0x1FF), iArgument );
    }


    /// Get ammo from waypoint argument. If the ammo is for secondary weapon function (right click), then bIsSecondary is set.
    static int GetAmmo( bool& bIsSecondary, int iArgument )
    {
        int iResult = GET_1ST_BYTE(iArgument);
        bIsSecondary = (iResult & 0x80) != 0;
        return iResult & 0x7F;
    }

    /// Set ammo for waypoint argument.
    static void SetAmmo( int iAmmo, bool bIsSecondary, int& iArgument ) { SET_1ST_BYTE(iAmmo | (bIsSecondary ? 0x80 : 0), iArgument); }


    /// Get weapon index from waypoint argument.
    static TWeaponId GetWeaponId( int iArgument ) { return GET_2ND_BYTE(iArgument); }

    /// Set weapon for waypoint argument.
    static void SetWeaponId( TWeaponId iId, int& iArgument ) { SET_2ND_BYTE( iId, iArgument ); }


    /// Get armor from waypoint argument.
    static int GetArmor( int iArgument ) { return GET_3RD_BYTE(iArgument); }

    /// Set armor for waypoint argument.
    static void SetArmor( int iArmor, int& iArgument ) { SET_3RD_BYTE(iArmor, iArgument); }


    /// Get health from waypoint argument.
    static int GetHealth( int iArgument ) { return GET_4TH_BYTE(iArgument); }

    /// Set health for waypoint argument.
    static void SetHealth( int iHealth, int& iArgument ) { SET_4TH_BYTE(iHealth, iArgument); }


	/// Get button index from waypoint argument.
	static TItemIndex GetButton( int iArgument ) { return GET_3RD_BYTE( iArgument ) - 1; }

	/// Set button index for waypoint argument.
	static void SetButton( TItemIndex iButton, int& iArgument ) { SET_3RD_BYTE( iButton + 1, iArgument ); }


	/// Get button's door index from waypoint argument.
	static TItemIndex GetDoor( int iArgument ) { return GET_4TH_BYTE( iArgument ) - 1; }

	/// Set button's door index for waypoint argument.
	static void SetDoor( TItemIndex iDoor, int& iArgument ) { SET_4TH_BYTE( iDoor + 1, iArgument ); }


	/// Get button's elevator index from waypoint argument.
	static TItemIndex GetElevator( int iArgument ) { return GetDoor(iArgument); }

	/// Set button's elevator index for waypoint argument.
	static void SetElevator( TItemIndex iElevator, int& iArgument ) { SetDoor( iElevator, iArgument ); }


	/// Return true if point v 'touches' this waypoint.
    inline bool IsTouching( const Vector& v, bool bOnLadder ) const
    {
        return CUtil::IsPointTouch3d(vOrigin, v, bOnLadder ? CMod::iPointTouchLadderSquaredZ : CMod::iPointTouchSquaredZ, CMod::iPointTouchSquaredXY);
    }

    /// Get color of waypoint.
    void GetColor( unsigned char& r, unsigned char& g, unsigned char& b ) const;

    /// Draw this waypoint for given amount of time.
    void Draw( TWaypointId iWaypointId, TWaypointDrawFlags iDrawType, float fDrawTime ) const;

protected:
    static const TWaypointFlags m_aFlagsForEntityType[ EItemTypeCanPickTotal ];
};


//****************************************************************************************************************
/// Class that represents path between 2 adjacent waypoints.
//****************************************************************************************************************
class CWaypointPath
{
public:
    CWaypointPath( float fLength, TPathFlags iFlags = FPathNone, unsigned short iArgument = 0 ):
        fLength(fLength), iFlags(iFlags), iArgument(iArgument) {}

	static const int INVALID_BYTE_ARGUMENT = 0;

    bool IsActionPath() { return iFlags != 0; }

	bool IsDoor() { return FLAG_SOME_SET( FPathDoor, iFlags ); }
    bool HasDoorNumber() { return IsDoor() && GetDoorNumber() >= 0; }
	TItemIndex GetDoorNumber() { return GET_1ST_BYTE( iArgument ) - 1; }
	void SetDoorNumber( TItemIndex iDoor ) { SET_1ST_BYTE( iDoor + 1, iArgument ); }

	bool IsElevator() { return FLAG_SOME_SET( FPathElevator, iFlags ); }
	bool HasElevatorNumber() { return IsElevator() && GetElevatorNumber() >= 0; }
	TItemIndex GetElevatorNumber() { return GetDoorNumber(); }
	void SetElevatorNumber( TItemIndex iElevator ) { SetDoorNumber( iElevator ); }

    bool HasButtonNumber() { 
        return FLAG_SOME_SET( FPathDoor | FPathElevator, iFlags ) && GET_2ND_BYTE( iArgument ) != INVALID_BYTE_ARGUMENT;
    }
	TItemIndex GetButtonNumber() { return GET_2ND_BYTE( iArgument ) - 1; }
	void SetButtonNumber( TItemIndex iButton ) { SET_2ND_BYTE( iButton + 1, iArgument ); }

	int ActionTime() { return GET_1ST_BYTE( iArgument ); }
	void SetActionTime( int iTime ) { SET_1ST_BYTE( iTime, iArgument ); }

	int ActionDuration() { return GET_2ND_BYTE( iArgument ); }
	void SetActionDuration( int iTime ) { SET_2ND_BYTE( iTime, iArgument ); }

    bool HasDemo() { return FLAG_SOME_SET(FPathDemo, iFlags); }
	int GetDemoNumber() { return iArgument; }
	void SetDemoNumber( int iDemo ) { iArgument = iDemo; }

    float fLength;                           ///< Path length. Needed for route calculation.
    TPathFlags iFlags;                       ///< Path flags.

    /// Path argument. At 1rst byte there is time to wait before action (in deciseconds). 2nd byte is action duration.
    TPathArgument iArgument;
};


class CClient; // Forward declaration.


//****************************************************************************************************************
/// Class that represents set of waypoints on a map.
//****************************************************************************************************************
class CWaypoints
{

public: // Types and constants.
    /// Graph that represents graph of waypoints.
    typedef good::graph< CWaypoint, CWaypointPath, good::vector, good::vector > WaypointGraph;
    typedef WaypointGraph::node_t WaypointNode;     ///< Node of waypoint graph.
    typedef WaypointGraph::arc_t WaypointArc;       ///< Graph arc, i.e. path between two waypoints.
    typedef WaypointGraph::node_it WaypointNodeIt;  ///< Node iterator.
    typedef WaypointGraph::arc_it WaypointArcIt;    ///< Arc iterator.
    //typedef WaypointGraph::node_id TWaypointId;     ///< Type for node identifier.

    static bool bValidVisibilityTable;              ///< When waypoints are modified vis-table needs to be recalculated.
    static float fNextDrawWaypointsTime;            ///< Next draw time of waypoints (draw once per second).

public: // Methods.
    /// Return true if waypoint id is valid. Verifies that waypoint is actually exists.
    static bool IsValid( TWaypointId id ) { return m_cGraph.is_valid(id); }

    /// Return waypoints count.
    static int Size() { return (int)m_cGraph.size(); }

    /// Clear waypoints.
    static void Clear();

    /// Save waypoints to a file.
    static bool Save();

    /// Load waypoints from file (but you must clear them first).
    static bool Load();


    /// Get waypoint.
    static inline CWaypoint& Get( TWaypointId id ) { return m_cGraph[id].vertex; }

    /// Check if waypoint @p iTo is visible from @p iFrom.
    static inline bool IsVisible( TWaypointId iFrom, TWaypointId iTo )
    {
        return bValidVisibilityTable ? m_aVisTable[iFrom].test(iTo) : false;
    }

    /// Get random neighbour which is visible to given waypoint.
    static TWaypointId GetRandomNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible );

    /// Get nearest neighbour to given waypoint.
    static TWaypointId GetNearestNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible );

    /// Get farest neighbour to given waypoint.
    static TWaypointId GetFarestNeighbour( TWaypointId iWaypoint, TWaypointId iTo, bool bVisible );

    /// Return true if there is a path from waypoint source to waypoint dest.
    static inline bool HasPath( TWaypointId source, TWaypointId dest )
    {
        WaypointNode& w = m_cGraph[source];
        return w.find_arc_to(dest) != w.neighbours.end();
    }

    /// Get waypoint path.
    static CWaypointPath* GetPath( TWaypointId iFrom, TWaypointId iTo );

    /// Get waypoint node (waypoint + neighbours).
    static WaypointNode& GetNode( TWaypointId id ) { return m_cGraph[id]; }

    /// Add waypoint.
    static TWaypointId Add( const Vector& vOrigin, TWaypointFlags iFlags = FWaypointNone, int iArgument = 0, int iAreaId = 0 );

    /// Remove waypoint.
    static void Remove( TWaypointId id, bool bResetPlayers = true );

    /// Move waypoint to new position.
    static inline void Move( TWaypointId id, const Vector& vOrigin )
    {
        GoodAssert( CWaypoint::IsValid(id) );
        CWaypoint& cWaypoint = Get(id);
        RemoveLocation( id, cWaypoint.vOrigin );
        AddLocation(id, vOrigin);
        cWaypoint.vOrigin = vOrigin;
    }

    /// Add path from waypoint iFrom to waypoint iTo.
    static bool AddPath( TWaypointId iFrom, TWaypointId iTo, float fDistance = 0.0f, TPathFlags iFlags = FPathNone );

    /// Remove path from waypoint iFrom to waypoint iTo.
    static bool RemovePath( TWaypointId iFrom, TWaypointId iTo );

    /// Create waypoint paths if reachable, from iFrom to iTo and viceversa.
    static void CreatePathsWithAutoFlags( TWaypointId iWaypoint1, TWaypointId iWaypoint2, bool bIsCrouched, 
                                          int iMaxDistance = CWaypoint::iDefaultDistance, bool bShowHelp = true );

    /// Create waypoint paths to nearests waypoints.
    static void CreateAutoPaths( TWaypointId id, bool bIsCrouched, float fMaxDistance = CWaypoint::iDefaultDistance, bool bShowHelp = true );


    /// Get nearest waypoint to given position.
    static TWaypointId GetNearestWaypoint( const Vector& vOrigin, const good::bitset* aOmit = NULL, bool bNeedVisible = true,
                                           float fMaxDistance = CWaypoint::MAX_RANGE, TWaypointFlags iFlags = FWaypointNone );

    static void GetNearestWaypoints( good::vector<TWaypointId>& aResult, const Vector& vOrigin, bool bNeedVisible, float fMaxDistance );

    /// Get any waypoint with some of the given flags set.
    static TWaypointId GetAnyWaypoint( TWaypointFlags iFlags = FWaypointNone );


    /// Get areas names for current map.
    static StringVector& GetAreas() { return m_aAreas; }

    /// Get area id from name.
    static TAreaId GetAreaId( const good::string& sName )
    {
        StringVector::const_iterator it( good::find(m_aAreas.begin(), m_aAreas.end(), sName) );
        return ( it == m_aAreas.end() )  ?  EAreaIdInvalid  :  ( it - m_aAreas.begin() );
    }

    /// Add new area name.
    static TAreaId AddAreaName( const good::string& sName ) { m_aAreas.push_back(sName); return m_aAreas.size()-1; }

    /// Get waypoint at which player is looking at.
    static TWaypointId GetAimedWaypoint( const Vector& vOrigin, const QAngle& ang );

    /// Draw nearest waypoints around player.
    static void Draw( CClient* pClient );


    /// Mark 2 waypoints as unreachable.
    static void MarkUnreachablePath( TWaypointId iWaypointFrom, TWaypointId iWaypointTo );

    /// Clear unreachable paths.
    static void ClearUnreachablePaths() { m_aUnreachablePaths.clear(); }


    /// Analyze waypoints.
    static void Analyze( edict_t* pClient, bool bShowLines = false );

    /// Stop analyzing waypoints.
    static void StopAnalyzing();

    /// Return true if analyzing.
    static bool IsAnalyzing() { return m_iAnalyzeStep != EAnalyzeStepTotal; }

    /// Analyze step.
    static void AnalyzeStep();


    // For console commands: waypoint analyze debug / omit.
    enum
    {
        EAnalyzeWaypointsAdd = 0,
        EAnalyzeWaypointsOmit,
        EAnalyzeWaypointsDebug,
        EAnalyzeWaypointsTotal,
    };
    typedef int TAnalyzeWaypoints;

    /// Clear omit/debug waypoints.
    static void AnalyzeClear( TAnalyzeWaypoints iWhich ) { m_aWaypointsToAddOmitInAnalyze[ iWhich ].clear(); }

    /// Omit/debug waypoint for analization next time.
    static void AnalyzeAddPosition( TWaypointId iWaypoint, bool bAdd, TAnalyzeWaypoints iWhich ) {
        good::vector<Vector>& aWhich = m_aWaypointsToAddOmitInAnalyze[ iWhich ];
        if ( bAdd )
            aWhich.push_back( Get( iWaypoint ).vOrigin );
        else
        {
            good::vector<Vector>::iterator it = good::find( aWhich, Get( iWaypoint ).vOrigin );
            if ( it != aWhich.end() )
                aWhich.erase( it );
        }
    }


protected:
    friend class CWaypointNavigator; // Get access to m_cGraph (for A* search implementation).

    // Add ladder dismounts waypoints.
    static void AddLadderDismounts( ICollideable *pLadder, float fPlayerWidth, float fPlayerEye, TWaypointId iBottom, TWaypointId iTop );

    // Analyze one waypoint (for AnalyzeStep()). Return true, if waypoint has nearby waypoints or new waypoint is added.
    static bool AnalyzeWaypoint( TWaypointId iWaypoint, Vector& vPos, Vector& vNew, float fPlayerEye, float fAnalyzeDistance,
                                 float fAnalyzeDistanceExtra, float fAnalyzeDistanceExtraSqr, float fHalfPlayerWidth );

    // Get path color.
    static void GetPathColor( TPathFlags iFlags, unsigned char& r, unsigned char& g, unsigned char& b );

    // Draw waypoint paths.
    static void DrawWaypointPaths( TWaypointId id, TPathDrawFlags iPathDrawFlags );

    // Draw visibles waypoints.
    static void DrawVisiblePaths( TWaypointId id, TPathDrawFlags iPathDrawFlags );

    // Buckets are 3D areas that we will use to optimize nearest waypoints finding.
    static const int BUCKETS_SIZE_X = 96;
    static const int BUCKETS_SIZE_Y = 96;
    static const int BUCKETS_SIZE_Z = 96;

    static const int BUCKETS_SPACE_X = CUtil::iMaxMapSize/BUCKETS_SIZE_X; // 341.33 = 341
    static const int BUCKETS_SPACE_Y = CUtil::iMaxMapSize/BUCKETS_SIZE_Y;
    static const int BUCKETS_SPACE_Z = CUtil::iMaxMapSize/BUCKETS_SIZE_Z;

    // Get bucket indexes in array of buckets. Normalize position first to be 0..32768 instead of -16384..16384 (iHalfMaxMapSize).
    static int GetBucketX( float fPositionX ) { return (int)(fPositionX + CUtil::iHalfMaxMapSize) / BUCKETS_SPACE_X; }
    static int GetBucketY( float fPositionY ) { return (int)(fPositionY + CUtil::iHalfMaxMapSize) / BUCKETS_SPACE_Y; }
    static int GetBucketZ( float fPositionZ ) { return (int)(fPositionZ + CUtil::iHalfMaxMapSize) / BUCKETS_SPACE_Z; }

    // Get adjacent buckets.
    static void GetBuckets( int x, int y, int z, int& minX, int& minY, int& minZ, int& maxX, int& maxY, int& maxZ )
    {
		// Note that maxX = 32768 / 341 = 96.09 = 96, so x or y or z can be 96! We treat it like 95!
		// minX = x-1
		minX = ( x <= 1 ) ? 0 : x - 1;
		minY = ( y <= 1 ) ? 0 : y - 1;
		minZ = ( z <= 1 ) ? 0 : z - 1;

		// maxX = x+1
		maxX = ( x < BUCKETS_SIZE_X - 1 ) ? x + 1 : BUCKETS_SIZE_X - 1;
		maxY = ( y < BUCKETS_SIZE_Y - 1 ) ? y + 1 : BUCKETS_SIZE_Y - 1;
		maxZ = ( z < BUCKETS_SIZE_Z - 1 ) ? z + 1 : BUCKETS_SIZE_Z - 1;
    }

    // Add location for waypoint.
    static inline void AddLocation( TWaypointId id, const Vector& vOrigin )
    {
        m_cBuckets[GetBucketX(vOrigin.x)][GetBucketY(vOrigin.y)][GetBucketZ(vOrigin.z)].push_back(id);
    }

    // Add location for waypoint.
    static inline void RemoveLocation( TWaypointId id, const Vector& vOrigin )
    {
        Bucket& cBucket = m_cBuckets[GetBucketX(vOrigin.x)][GetBucketY(vOrigin.y)][GetBucketZ(vOrigin.z)];
        cBucket.erase( find(cBucket, id) );
    }

    // Add location for waypoint.
    static void DecrementLocationIds( TWaypointId id );

    // Clear all locations.
    static void ClearLocations()
    {
        if ( m_cGraph.size() > 0 )
        {
            for (int x=0; x<BUCKETS_SIZE_X; ++x)
                for (int y=0; y<BUCKETS_SIZE_Y; ++y)
                    for (int z=0; z<BUCKETS_SIZE_Z; ++z)
                        m_cBuckets[x][y][z].clear();
        }
    }

    typedef good::vector<TWaypointId> Bucket;
    static Bucket m_cBuckets[BUCKETS_SIZE_X][BUCKETS_SIZE_Y][BUCKETS_SIZE_Z]; // 3D hash table of arrays of waypoint IDs.

    static StringVector m_aAreas;  // Areas names.

    static WaypointGraph m_cGraph; // Waypoints graph.
    static good::vector< good::bitset > m_aVisTable;

    // Unreachable path with failed count.
    typedef struct
    {
        Vector vFrom;
        Vector vTo;
        int iFailedCount;
    } unreachable_path_t;

    static good::vector<unreachable_path_t> m_aUnreachablePaths; // Paths marked from bots that are unreachable.

    // Next fields are for analyzing functions.
    enum
    {
        EAnalyzeStepNeighbours,    // Check for waypoint neighbours in all directions (left/right/up/down).
        EAnalyzeStepInters,        // Check for positions between adjacent neighbours (that are not added).
        EAnalyzeStepDeleteOrphans, // Delete waypoints that don't have neighbours, or don't have incoming path.
        EAnalyzeStepTotal
    };

    typedef int TAnalyzeStep;
    class CNeighbour { public: bool a[ 3 ][ 3 ]; }; // From left bottom to right upper.

    static good::vector<CNeighbour> m_aWaypointsNeighbours; // To know if waypoint made check for neighbours near, initialized during first step.
    static good::vector<Vector> m_aWaypointsToAddOmitInAnalyze[EAnalyzeWaypointsTotal];

    static TAnalyzeStep m_iAnalyzeStep;
    static TWaypointId m_iCurrentAnalyzeWaypoint; // Current waypoint index being analyzed.
    static float m_fAnalyzeWaypointsForNextFrame; // Positions for next frame to analyze.
    static bool m_bIsAnalyzeStepAddedWaypoints;
    static edict_t* m_pAnalyzer; // The person who launched console command "analyze".
};


#endif // __BOTRIX_WAYPOINT_H__
