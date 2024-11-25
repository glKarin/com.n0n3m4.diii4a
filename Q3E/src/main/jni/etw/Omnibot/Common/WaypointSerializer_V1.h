#ifndef __WAYPOINTSERIALIZER_V1_H__
#define __WAYPOINTSERIALIZER_V1_H__

#include "WaypointSerializerImp.h"

// class: WaypointSerializer_V1
//		Implementation for waypoint file format version 1
//		File format specifications:
//		- Waypoint Header
//			+ has file version *(1 byte)*
//			+ has number of waypoints *(4 bytes)*
//			+ has map name *(32 bytes)*
//			+ has author comments *(256 bytes)*
//		- For _n_ waypoints
//			+ write the vector3 position out *(4 bytes x 3)*
//			+ write (__int64) the waypoint flags *(8 bytes)*
//			+ write (unsigned char) representing number of connections this wp has *(1 byte)*
//			- for _n_ connections
//				+ write (unsigned int) numeric index of waypoint for each connection *(4 bytes)*
//				+ write (unsigned int) connection flags for this connection *(4 bytes)*
class WaypointSerializer_V1 : public WaypointSerializerImp
{
public:

	WaypointSerializer_V1() {};
	virtual ~WaypointSerializer_V1() {};
protected:
	virtual bool Load(File &_file, PathPlannerWaypoint::WaypointList &_wpl);
	virtual bool Save(File &_file, PathPlannerWaypoint::WaypointList &_wpl);

};

#endif
