#include "PrecompCommon.h"
#include "WaypointSerializer_V2.h"
#include "Waypoint.h"

bool WaypointSerializer_V2::Load(File &_file, PathPlannerWaypoint::WaypointList &_wpl)
{
	// temporary structure for reading in the connection indices since we
	// can't set up the correct pointers till after the waypoints have all
	// been read in and created.
	
	typedef std::multimap<unsigned int, WaypointConnection> WaypointConnections;
	WaypointConnections connections;

	obuint32 iSize = (obuint32)_wpl.size();
	for(unsigned int i = 0; i < iSize; ++i)
	{
		Waypoint *pCurrentWp = new Waypoint;

		// Read the position in.
		CHECK_READ(_file.Read(&pCurrentWp->m_Position, sizeof(pCurrentWp->m_Position)));
		pCurrentWp->m_Position.z += 24.0f;
		
		CHECK_READ(_file.ReadInt64(pCurrentWp->m_NavigationFlags));
		
		obuint8 iNumConnections = 0;
		CHECK_READ(_file.ReadInt8(iNumConnections));

		obuint32 index = 0, flags = 0;
		for(int w = 0; w < iNumConnections; ++w)
		{
			CHECK_READ(_file.ReadInt32(index));
			CHECK_READ(_file.ReadInt32(flags));
			flags = 0; // deprecated
			WaypointConnection conn = { index, flags };
			connections.insert(std::pair<unsigned int, WaypointConnection>(i, conn));
		}

		CHECK_READ(_file.ReadFloat(pCurrentWp->m_Radius));
		CHECK_READ(_file.Read(&pCurrentWp->m_Facing, sizeof(pCurrentWp->m_Facing)));

		//////////////////////////////////////////////////////////////////////////
		// Update the next guid
		if(Waypoint::m_NextUID <= pCurrentWp->m_UID)
			Waypoint::m_NextUID = pCurrentWp->m_UID+1;

		// Give it a new UID
		if(pCurrentWp->GetUID() == 0)
			pCurrentWp->AssignNewUID();
		//////////////////////////////////////////////////////////////////////////

		_wpl[i] = pCurrentWp;
	}

	// Connect the waypoints.
	for(obuint32 i = 0; i < _wpl.size(); ++i)
	{
		WaypointConnections::iterator it;
		for (it = connections.lower_bound(i); 
			it != connections.upper_bound(i); 
			++it) 
		{
			Waypoint::ConnectionInfo conn = { _wpl[it->second.m_Index], it->second.m_ConnectionFlags };
			if(it->second.m_Index < _wpl.size())
				_wpl[i]->m_Connections.push_back(conn);
			else 
				return false;
		}
	}
	return true;
}

bool WaypointSerializer_V2::Save(File &_file, PathPlannerWaypoint::WaypointList &_wpl)
{
	OBASSERT(0, "This Shouldn't get called!");
	return false;
}

