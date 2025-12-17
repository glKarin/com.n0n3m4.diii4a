#ifndef __BOTRIX_WAYPOINT_NAVIGATOR_H__
#define __BOTRIX_WAYPOINT_NAVIGATOR_H__


#include <good/astar.h>

#include "defines.h"
#include "waypoint.h"


//****************************************************************************************************************
/// Class to get path between 2 waypoints.
//****************************************************************************************************************
class CWaypointNavigator
{

public:
    static const int MAX_WAYPOINTS_IN_LOOP = 32;  ///< Max count of waypoint in one loop step.
    static TPathDrawFlags iPathDrawFlags;      ///< Path drawing type.

    /// Constructor.
    CWaypointNavigator(): m_bSearchStarted(false), m_bSearchEnded(false), m_iPathIndex(-1) {}

    /// Setup searching a path between given waypoints, avoiding certain areas, and setting search step size (iMaxWaypointsInLoop).
    bool SearchSetup( TWaypointId iFrom, TWaypointId iTo, good::vector<TAreaId> const& aAvoidAreas, int iMaxWaypointsInLoop = MAX_WAYPOINTS_IN_LOOP );

    /// Resume searching path. Returns true if finished finding path.
    bool SearchStep()
    {
        if (m_bSearchStarted)
        {
            m_bSearchEnded = m_cAstar.step();
            if ( m_bSearchEnded )
            {
                BreakDebuggerIf( m_cAstar.has_path() && (m_cAstar.path().size() < 2) ); // Should not happen.
                m_iPathIndex = 0;
            }
        }
        return m_bSearchEnded;
    }

    /// Stop searching path and reset search info.
    void Stop() { m_bSearchStarted = false; m_bSearchEnded = false; m_iPathIndex = -1; }

    /// Return true if search was started.
    bool SearchStarted() const { return m_bSearchStarted; }

    /// Return true if search is ended.
    bool SearchEnded() const { return m_bSearchEnded; }

    /// Return same value of last call of SearchStep(), i.e. if path was found between given vectors.
    bool PathFound() const { return m_bSearchEnded && m_cAstar.has_path(); }

    /// Return true if founded path has more coordinates.
    bool HasMoreCoords() const { return 0 <= m_iPathIndex && m_iPathIndex < (int)m_cAstar.path().size(); }

    /// Return 2 next coordinates in founded path.
    void GetNextWaypoints( TWaypointId& iNext, TWaypointId& iAfterNext )
    {
        iNext = m_cAstar.path()[m_iPathIndex++];
        iAfterNext = ( m_iPathIndex == m_cAstar.path().size() ) ? -1 : m_cAstar.path()[m_iPathIndex];
    }

    /// Decrement path position. Used in case of some bot action didn't work, to perform waypoint touch again.
    void SetPreviousPathPosition()
    {
        if (m_iPathIndex > 0 )
            m_iPathIndex--;
    }

    /// Drawpath.
    void DrawPath( unsigned char r, unsigned char g, unsigned char b, Vector  const& vOrigin );


protected:
    //------------------------------------------------------------------------------------------------------------
    // Class for A* can use function. Use to know whether can use certain waypoint or not.
    //------------------------------------------------------------------------------------------------------------
    class CCanUseWaypoint
    {
    public:
        // Constructor.
        CCanUseWaypoint(good::vector<TAreaId> const& aAvoidedAreas): m_cAvoidAreas(aAvoidedAreas) {}

        // Default operator to know if can use waypoint.
        bool operator()( CWaypoints::WaypointNode const& w ) const
        {
            return find(m_cAvoidAreas.begin(), m_cAvoidAreas.end(), w.vertex.iAreaId) == m_cAvoidAreas.end(); // Waypoint's area is not in avoided areas.
        }

    protected:
        good::vector<TAreaId> const& m_cAvoidAreas; // Array of areas to avoid.
    };

    //------------------------------------------------------------------------------------------------------------
    // Class for waypoint search heuristic.
    //------------------------------------------------------------------------------------------------------------
    class CWaypointDistance
    {
    public:
        // Default operator for waypoint distance.
        float operator()( CWaypoint const& w1, CWaypoint const& w2 ) const
        {
            return w1.vOrigin.DistTo(w2.vOrigin);
        }
    };

    //------------------------------------------------------------------------------------------------------------
    // Class to know waypoint path length.
    //------------------------------------------------------------------------------------------------------------
    class CWaypointPathLength
    {
    public:
        // Default operator for waypoint path length.
        float operator ()( CWaypointPath const& cPath ) const { return cPath.fLength; }
    };



protected:
    bool m_bSearchStarted, m_bSearchEnded;
    int m_iPathIndex;
    typedef good::astar< CWaypoint, CWaypointPath, float, CWaypointDistance, CWaypointPathLength, CCanUseWaypoint > astar_t;
    astar_t m_cAstar;

    float m_fNextDrawTime;
};


#endif // __BOTRIX_WAYPOINT_NAVIGATOR_H__
