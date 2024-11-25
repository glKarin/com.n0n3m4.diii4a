#ifndef __WAYPOINTSERIALIZERIMP_H__
#define __WAYPOINTSERIALIZERIMP_H__

#include "PathPlannerWaypoint.h"

#define CHECK_READ(x) if(!(x)) { LOGERR("Error Reading from Waypoint"); delete pCurrentWp; return false; };
#define CHECK_WRITE(x) if(!(x)) { LOGERR("Error Writing from Waypoint"); return false; };

// class: WaypointSerializerImp
//		In order to support all the versions of waypoint file formats that
//		could possibly be created, and retain backward compatibility, we're 
//		using a bridge pattern to allow any number of implementations of
//		the waypoint formats, so that based on the version of the waypoint
//		as determined in the waypoint header, the appropriate loading 
//		routines can be created and used.
//		
//		When changes to the waypoint file format are made, simply derive 
//		another serializer from this class, and implement the functions.
//		Then, simply expand the load function in the Pathplanner class 
//		to use the new implementation along with the new version number.
class WaypointSerializerImp
{
public:
	typedef struct 
	{
		unsigned int m_Index;
		unsigned int m_ConnectionFlags;
	} WaypointConnection;
	
	// function: Load
	//		Load a waypoint from an IO Proxy
	//
	// Parameters:
	//
	//		_proxy - the <File> to read the navigation data from.
	//		_wpl - the list of waypoints to load into.
	//
	// Returns:
	//		bool - true if success, false of failure
	virtual bool Load(File &_file, PathPlannerWaypoint::WaypointList &_wpl) = 0;

	// function: Save
	//		Save a waypoint to an IO Proxy
	//
	// Parameters:
	//
	//		_proxy - the <File> to read the navigation data from.
	//		_wpl - the list of waypoints to save.
	//
	// Returns:
	//		bool - true if success, false of failure
	virtual bool Save(File &_file, PathPlannerWaypoint::WaypointList &_wpl) = 0;

	WaypointSerializerImp() {};
	virtual ~WaypointSerializerImp() {};
protected:
};

// typedef: WpSerializerPtr
//		Used to point to instances of different waypoint serializers.
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<WaypointSerializerImp> WpSerializerPtr;
#else
typedef boost::shared_ptr<WaypointSerializerImp> WpSerializerPtr;
#endif

#endif
