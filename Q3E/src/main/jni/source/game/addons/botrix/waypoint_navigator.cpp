#include "server_plugin.h"
#include "source_engine.h"
#include "waypoint_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------------------------
TPathDrawFlags CWaypointNavigator::iPathDrawFlags = FPathDrawNone;


//----------------------------------------------------------------------------------------------------------------
bool CWaypointNavigator::SearchSetup( TWaypointId iFrom, TWaypointId iTo,
                                      good::vector<TAreaId> const& aAvoidAreas, int iMaxWaypointsInLoop )
{
    BASSERT( CWaypoint::IsValid(iFrom) && CWaypoint::IsValid(iTo), return false );

    m_cAstar.set_graph(CWaypoints::m_cGraph, 1024);
    m_cAstar.setup_search(iFrom, iTo, CCanUseWaypoint(aAvoidAreas), iMaxWaypointsInLoop);

    m_bSearchStarted = true;
    m_bSearchEnded = false;
    m_iPathIndex = -1;

    return true;
}


//----------------------------------------------------------------------------------------------------------------
void CWaypointNavigator::DrawPath( unsigned char r, unsigned char g, unsigned char b, const Vector& vOrigin )
{
    if ( (iPathDrawFlags == FPathDrawNone) || (CBotrixPlugin::fTime < m_fNextDrawTime) ||
         !m_cAstar.has_path() || (m_iPathIndex == 0) || ( m_iPathIndex > (int)m_cAstar.path().size() ) )
        return;

    float fDrawTime = 0.1f; // Bug when drawing line for more time than 0.1 seconds.
    m_fNextDrawTime = CBotrixPlugin::fTime + fDrawTime;

    int index = m_iPathIndex - 1; // Bot is currently moving to waypoint in path[m_iPathIndex-1].
    astar_t::path_t& path = m_cAstar.path();

    CWaypoints::WaypointNode& first = CWaypoints::m_cGraph[path[index]];

    // Draw waypoints paths lower (can't see it when spectating bot, because its  height is at eye level).
    Vector v1(vOrigin), v2(first.vertex.vOrigin);
    v1.z -= CMod::GetVar( EModVarPlayerEye )/4;
    v2.z -= CMod::GetVar( EModVarPlayerEye )/4;

    if ( FLAG_ALL_SET_OR_0(FPathDrawBeam, iPathDrawFlags) )
        CUtil::DrawBeam(v1, v2, 4, fDrawTime, r, g, b);

    if ( FLAG_ALL_SET_OR_0(FPathDrawLine, iPathDrawFlags) )
        CUtil::DrawLine(v1, v2, fDrawTime, r, g, b);

    for ( int i = index; i < m_cAstar.path().size()-1; ++i )
    {
        v1 = CWaypoints::Get(path[i]).vOrigin;
        v2 = CWaypoints::Get(path[i+1]).vOrigin;
        v1.z -= CMod::GetVar( EModVarPlayerEye )/4;
        v2.z -= CMod::GetVar( EModVarPlayerEye )/4;
        if ( FLAG_ALL_SET_OR_0(FPathDrawBeam, iPathDrawFlags) )
            CUtil::DrawBeam(v1, v2, 4, fDrawTime, r, g, b);
        if ( FLAG_ALL_SET_OR_0(FPathDrawLine, iPathDrawFlags) )
            CUtil::DrawLine(v1, v2, fDrawTime, r, g, b);
    }
}
