#include "PrecompCommon.h"
#include "WaypointSerializer_V9.h"

bool WaypointSerializer_V9::Load(File& _file, PathPlannerWaypoint::WaypointList& _wpl)
{
	PathPlannerWaypoint::WaypointList::iterator it;
	for(it = _wpl.begin(); it != _wpl.end(); ++it)
	{
		*it = new Waypoint;
	}
	obuint32 maxUID = 0, prevUID = 0;
	obint32 index;
	for(it = _wpl.begin(), index = 0; it != _wpl.end(); ++it, ++index)
	{
		Waypoint* pCurrentWp = *it;

		obuint8 mask;
		if(!_file.ReadInt8(mask)) break;
		
		if(mask & MASK_UID) { if(!_file.ReadIntPk(pCurrentWp->m_UID)) break; }
		else pCurrentWp->m_UID = prevUID + 1;
		
		if(!_file.Read(&pCurrentWp->m_Position, sizeof(pCurrentWp->m_Position))) break;
		if(mask & MASK_FACING) { if(!_file.Read(&pCurrentWp->m_Facing, sizeof(pCurrentWp->m_Facing))) break; }
		
		if(mask & MASK_FLAGS) if(!_file.ReadIntPk(pCurrentWp->m_NavigationFlags)) break;
		
		if(mask & MASK_RADIUS_INT) { 
			obuint8 r;
			if(!_file.ReadInt8(r)) break; 
			pCurrentWp->m_Radius = r;
		}
		else if(mask & MASK_RADIUS_FLOAT) { 
			if(!_file.ReadFloat(pCurrentWp->m_Radius)) break; 
		}
		else pCurrentWp->m_Radius = 35;

		if(mask & MASK_NAME) if(!_file.ReadStringPk(pCurrentWp->m_WaypointName)) break;

		if(mask & MASK_PROPERTY)
		{
			obuint32 iNumProperties = 0;
			if(!_file.ReadIntPk(iNumProperties)) break;
			for(obuint32 p = 0; p < iNumProperties; ++p)
			{
				String name, value;
				if(!_file.ReadStringPk(name)) return false;
				if(!_file.ReadStringPk(value)) return false;
				pCurrentWp->GetPropertyMap().AddProperty(name, value);
			}
		}

		// Read connections
		obuint32 iNumConnections;
		if(!_file.ReadIntPk(iNumConnections)) break;
		for(obuint32 w = 0; w < iNumConnections; ++w)
		{
			obint32 i;
			if(!_file.ReadSignIntPk(i)) return false;
			obuint32 u = obuint32(i + index);
			if(u >= _wpl.size()) return false;
			Waypoint::ConnectionInfo conn = { _wpl[u], 0 };
			pCurrentWp->m_Connections.push_back(conn);
		}

		// Update the next guid
		if(pCurrentWp->m_UID == 0) pCurrentWp->m_UID = maxUID + 1;
		prevUID = pCurrentWp->m_UID;
		if(maxUID < prevUID) maxUID = prevUID;

		pCurrentWp->PostLoad();
	}

	Waypoint::m_NextUID = maxUID + 1;
	return it == _wpl.end();
}

bool WaypointSerializer_V9::Save(File& _file, PathPlannerWaypoint::WaypointList& _wpl)
{
	obuint32 uid = 0;
	obint32 index;
	PathPlannerWaypoint::WaypointList::const_iterator it;
	for(it = _wpl.begin(), index= 0; it != _wpl.end(); ++it, ++index)
	{
		Waypoint* pCurrentWp = *it;
		const PropertyMap::ValueMap& properties = pCurrentWp->GetPropertyMap().GetProperties();

		obuint8 mask = 0;
		if(pCurrentWp->m_UID != uid + 1) mask |= MASK_UID;
		uid = pCurrentWp->m_UID;
		if(!pCurrentWp->m_Facing.IsZero()) mask |= MASK_FACING;
		if(pCurrentWp->m_NavigationFlags != 0) mask |= MASK_FLAGS;
		
		float r = pCurrentWp->m_Radius;
		if(roundf(r) != r || r > 255 || r < 0) mask |= MASK_RADIUS_FLOAT;
		else if(r != 35) mask |= MASK_RADIUS_INT;
		
		if(!pCurrentWp->m_WaypointName.empty()) mask |= MASK_NAME;
		if(!properties.empty()) mask |= MASK_PROPERTY;
		if(!_file.WriteInt8(mask)) break;

		if(mask & MASK_UID) if(!_file.WriteIntPk(pCurrentWp->m_UID)) break;
		if(!_file.Write(&pCurrentWp->m_Position, sizeof(pCurrentWp->m_Position))) break;
		if(mask & MASK_FACING) if(!_file.Write(&pCurrentWp->m_Facing, sizeof(pCurrentWp->m_Facing))) break;
		if(mask & MASK_FLAGS) if(!_file.WriteIntPk(pCurrentWp->m_NavigationFlags)) break;
		if(mask & MASK_RADIUS_INT) if(!_file.WriteInt8((obuint8)pCurrentWp->m_Radius)) break;
		if(mask & MASK_RADIUS_FLOAT) if(!_file.WriteFloat(pCurrentWp->m_Radius)) break;
		if(mask & MASK_NAME) if(!_file.WriteStringPk(pCurrentWp->m_WaypointName)) break;

		// Write properties
		if(mask & MASK_PROPERTY)
		{
			if(!_file.WriteIntPk((obuint32)properties.size())) break;
			for(PropertyMap::ValueMap::const_iterator pIt = properties.begin(); pIt != properties.end(); ++pIt)
			{
				if(!_file.WriteStringPk(pIt->first)) return false;
				if(!_file.WriteStringPk(pIt->second)) return false;
			}
		}

		// Write the number of connections
		obuint32 iNumConnections = 0;
		for(Waypoint::ConnectionList::iterator it2 = pCurrentWp->m_Connections.begin(); it2 != pCurrentWp->m_Connections.end(); ++it2)
		{
			if(!(it2->m_ConnectionFlags & F_LNK_DONTSAVE)) iNumConnections++;
		}
		if(!_file.WriteIntPk(iNumConnections)) break;

		// Write connections
		for(Waypoint::ConnectionList::iterator it2 = pCurrentWp->m_Connections.begin(); it2 != pCurrentWp->m_Connections.end(); ++it2)
		{
			if(it2->m_ConnectionFlags & F_LNK_DONTSAVE) continue;

			// Look for this waypoint in the vector so we can get the index into the vector.
			PathPlannerWaypoint::WaypointList::iterator c = std::find(_wpl.begin(), _wpl.end(), it2->m_Connection);
			// write relative index
			if(c == _wpl.end() || !_file.WriteSignIntPk(obint32(c - _wpl.begin()) - index)) return false;
		}
	}

	return it == _wpl.end();
}
