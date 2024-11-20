#ifndef __WAYPOINTSERIALIZER_V9_H__
#define __WAYPOINTSERIALIZER_V9_H__

#include "WaypointSerializerImp.h"

class WaypointSerializer_V9 : public WaypointSerializerImp
{
public:

	WaypointSerializer_V9() {};
	virtual ~WaypointSerializer_V9() {};
protected:
	virtual bool Load(File &_file, PathPlannerWaypoint::WaypointList &_wpl);
	virtual bool Save(File &_file, PathPlannerWaypoint::WaypointList &_wpl);
private:
	enum {
		MASK_FLAGS = 1<<0,
		MASK_NAME = 1<<1,
		MASK_PROPERTY = 1<<2,
		MASK_RADIUS_INT = 1<<3,
		MASK_RADIUS_FLOAT = 1<<4,
		MASK_UID = 1<<5,
		MASK_FACING = 1<<6,
	};

};

#endif
