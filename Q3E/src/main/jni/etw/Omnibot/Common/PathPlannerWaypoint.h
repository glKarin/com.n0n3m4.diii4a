#ifndef __PATHPLANNERWAYPOINT_H__
#define __PATHPLANNERWAYPOINT_H__

#include "PathPlannerBase.h"
#include "Timer.h"
#include "Waypoint.h"
#include "gmbinder2.h"

class Waypoint;
class WaypointSerializerImp;

// class: PathPlannerWaypoint
//		Waypoint-based Path Planning Implementation
//		Uses a graph of 3d points and their connectivity information to
//		define traversable areas and allow easy path queries with A*.
class PathPlannerWaypoint : public PathPlannerBase
{
public:
	enum NavMeshFlags
	{
		WAYPOINT_ISDONE = NUM_BASE_NAVFLAGS,
		WAYPOINT_VIEW_FACING,
		WAYPOINT_DRAWAVOIDLEVEL,
	};

	typedef std::map<String, const NavFlags> FlagMap;

	// typedef: WaypointSerializers 
	//	A map that allows a serializer implementation to be looked
	//	up using the file version as the key. Used to allow backwards
	//	compatibility between each waypoint format by allowing each format
	//	to use the serializer that matches its version number.
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
	typedef std::map<unsigned char, std::shared_ptr<WaypointSerializerImp> > WaypointSerializers;
#else
	typedef std::map<unsigned char, boost::shared_ptr<WaypointSerializerImp> > WaypointSerializers;
#endif
	// typedef: WaypointList
	//	std::vector of pointers to waypoints.
	typedef std::vector<Waypoint*> WaypointList;

	static void SetMovementCapFlags(const NavFlags &_flags);

	// typedef: WaypointFlagMap
	//		Allows fast loopup of waypoints by flags, for a quick first step
	//		in checking if a waypoint with a required flag exists instead of 
	//		doing an exhaustive search for one.
	typedef std::multimap<NavFlags, Waypoint*> WaypointFlagMap;
	
	// typedef: ConnectionList
	typedef std::pair<Waypoint*, Waypoint::ConnectionInfo*> Link;
	typedef std::vector<Link> ConnectionList;

	typedef enum
	{
		B_PATH_OPEN,
		B_PATH_CLOSED,
		B_INVALID_FLAGS
	} BlockableStatus;

	struct ClosestLink
	{
		enum { NumWps = 2 };
		bool IsValid() const { return m_Wp[0] || m_Wp[1]; }

		Waypoint	*m_Wp[NumWps];
		Vector3f	m_Position;

		ClosestLink(Waypoint *_wp1 = 0, Waypoint *_wp2 = 0)
		{
			m_Wp[0] = _wp1;
			m_Wp[1] = _wp2;
		}
	};

	// typedef: pfbWpPathCheck
	//		A callback function to perform tests on a pair of waypoints.
	typedef BlockableStatus (*pfbWpPathCheck)(const Waypoint*, const Waypoint*, bool _draw);

	bool Init();
	void Update();
	void Shutdown();
	bool IsReady() const;
	void DrawActiveFrame();

	int GetLatestFileVersion() const;

	bool SetWaypointName(Waypoint *_wp, const String &_name);
	bool SetWaypointName(int _index, const String &_name);
	bool GetNavFlagByName(const String &_flagname, NavFlags &_flag) const;

	Vector3f GetDisplayPosition(const Vector3f &_pos);

	inline const FlagMap &GetNavFlagMap() const { return m_WaypointFlags; }

	Waypoint *GetWaypointByName(const String &_name) const;
	void GetWaypointsByName(const String &_name, WaypointList &_list) const;
	void GetWaypointsByExpr(const String &_expr, WaypointList &_list) const;
	Waypoint *GetWaypointByGUID(obuint32 _uid) const;
	Waypoint *_GetClosestWaypoint(const Vector3f &_pos, const NavFlags _team, const int _options, int *_index = NULL) const;

	Vector3f GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team);

	void PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team);
	int PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team);
	int PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team);

	bool IsDone() const;
	bool FoundGoal() const;
	bool Load(const String &_mapname, bool _dl = true);
	bool Save(const String &_mapname);
	void Unload();
	static void SetNavDir(String &navDir, const char *_file);

	void GetPath(Path &_path, int _smoothiterations);

	// waypoint creation methods
	Waypoint *AddWaypoint(const Vector3f &_pos, const Vector3f &_facing = Vector3f::ZERO, bool _blockdupe = false);
	bool DeleteWaypoint(const Vector3f &_pos);
	void DeleteWaypoint(Waypoint *pDeleteMe);

	bool _ConnectWaypoints(Waypoint *_wp1, Waypoint *_wp2);
	bool _DisConnectWaypoints(Waypoint *_wp1, Waypoint *_wp2);

	void RegisterNavFlag(const String &_name, const NavFlags &_func);
	//void RegisterLinkFlag(const String &_name, const NavFlags &_func);

	void RegisterGameGoals();
	void RegisterScriptFunctions(gmMachine *a_machine);

	bool GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name);

	void AddEntityConnection(const Event_EntityConnection &_conn);
	void RemoveEntityConnection(GameEntity _ent);

	void EntityCreated(const EntityInstance &ei);
	void EntityDeleted(const EntityInstance &ei);

#ifdef ENABLE_DEBUG_WINDOW
	void RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics);
#endif

	const WaypointList &GetWaypointList() const { return m_WaypointList; }
	const WaypointList &GetSelectedWaypointList() const { return m_SelectedWaypoints; }

	obuint32 SelectWaypoints(const AABB &_box);

	bool GroundPosition(Vector3f &out, const Vector3f &p, bool offsetforwp = true);
	void SliceLink(Waypoint *wp0, Waypoint *wp1, float _maxlen);

	// struct: Waypoint_Header
	//	The first thing that appears in the Waypoint file formats.
	//	Used to immediately get the waypoint version, map name,
	//	number of waypoints, and any comments added by the author.
	//	This is read first so that the application will then know
	//	the version of the waypoints so that it can use the proper
	//	serializer.
	//	#pragma pack(1)'d to eliminate any compiler padding.
	//	
#pragma pack(1)
	typedef struct 
	{
		unsigned char		m_WaypointVersion;
		unsigned int		m_NumWaypoints;
		char				m_TimeStamp[32];
		char				m_Comments[232];
		AABB				m_WorldAABB;
	} Waypoint_Header;
#pragma pack() // go back to default

#pragma pack(1)
	typedef struct 
	{
		unsigned char		m_VisVersion;
		unsigned int		m_NumWaypoints;
		char				m_TimeStamp[32];
	} VisFile_Header;
#pragma pack() // go back to default

	struct Sector
	{
		AABB		m_SectorBounds;
		Vector3f	m_Normal;
	};

	typedef std::vector<Sector> SectorList;
	SectorList g_SectorList;
	Sector m_CreatingSector;
	
	static NavFlags			m_CallbackFlags;
	static NavFlags			m_BlockableMask;

	//void BuildVisTable();
	void BuildBlockableList();
	void ClearBlockable(Waypoint* _waypoint);
	int CheckBlockable();

	const char *GetPlannerName() const { return "Waypoint Path Planner"; } ;
	int GetPlannerType() const { return NAVID_WP; };

#ifdef ENABLE_REMOTE_DEBUGGING
	void Sync( RemoteLib::DataBuffer & db, bool fullSync );
#endif

	PathPlannerWaypoint();
	virtual ~PathPlannerWaypoint();
protected:
	WaypointList		m_WaypointList;	
	ConnectionList		m_BlockableList;
	WaypointList		m_SelectedWaypoints;

	WaypointList		m_Solution;
	WaypointList		m_OpenList;
	int					m_OpenCount;
	int					m_ClosedCount;

	Client				*m_Client;
	ClosestLink			m_Start;
	//ClosestLink			m_Goal;

	RegulatorPtr		m_BlockableRegulator;
	RegulatorPtr		m_RadiusMarkRegulator;

	float				m_DefaultWaypointRadius;

	//float				m_HeuristicWeight;

	//Timer				m_Timer;
	//float				m_TimeSliceSeconds;

	int					m_SelectedWaypoint;
	Waypoint			*m_ConnectWp;

	int					m_GoodPathQueries;
	int					m_BadPathQueries;

	pfbWpPathCheck		m_PathCheckCallback;

	// typedef: VisibilityTable
	//		List of bitsets for determining visibility between waypoints
	//		Arranged for easy bit operations.
	/*typedef std::vector<boost::dynamic_bitset<obuint64> > VisibilityTable;
	VisibilityTable	m_VisTable;*/

	// Utility Functions
	void BuildSpatialDatabase();

	FlagMap				m_WaypointFlags;

	// Commands
	virtual void InitCommands();
	void cmdWaypointAdd(const StringVector &_args);
	void cmdWaypointAddX(const StringVector &_args);
	void cmdWaypointDelete(const StringVector &_args);
	void cmdWaypointDeleteX(const StringVector &_args);
	void cmdWaypointStats(const StringVector &_args);
	void cmdWaypointSave(const StringVector &_args);
	void cmdWaypointLoad(const StringVector &_args);
	void cmdWaypointAutoBuild(const StringVector &_args);
	void cmdWaypointDisconnectAll(const StringVector &_args);
	void cmdWaypointView(const StringVector &_args);
	void cmdWaypointViewFacing(const StringVector &_args);
	void cmdWaypointConnect(const StringVector &_args);
	void cmdWaypointConnectX(const StringVector &_args);
	void cmdWaypointConnect_Helper(const StringVector &_args, Waypoint *_waypoint);
	void cmdWaypointConnect2Way(const StringVector &_args);
	void cmdWaypointConnect2WayX(const StringVector &_args);
	void cmdWaypointConnect2Way_Helper(const StringVector &_args, Waypoint *_waypoint);
	void cmdWaypointAutoFlag(const StringVector &_args);
	void cmdWaypointBenchmark(const StringVector &_args);
	void cmdWaypointBenchmarkGetClosest(const StringVector &_args);
	void cmdTraceBenchmark(const StringVector &_args);
	void cmdWaypointSetDefaultRadius(const StringVector &_args);
	void cmdWaypointSetRadius(const StringVector &_args);
	void cmdWaypointChangeRadius(const StringVector &_args);
	void cmdWaypointSetFacing(const StringVector &_args);
	void cmdWaypointInfo(const StringVector &_args);
	void cmdWaypointGoto(const StringVector &_args);
	void cmdWaypointAddFlag(const StringVector &_args);
	void cmdWaypointAddFlagX(const StringVector &_args);
	void cmdWaypointAddFlag_Helper(const StringVector &_args, Waypoint *_waypoint);	
	void cmdWaypointClearAllFlags(const StringVector &_args);
	void cmdWaypointClearConnections(const StringVector &_args);
	void cmdWaypointSetName(const StringVector &_args);
	void cmdWaypointSetProperty(const StringVector &_args);
	void cmdWaypointShowProperty(const StringVector &_args);
	void cmdWaypointClearProperty(const StringVector &_args);
	void cmdWaypointAutoRadius(const StringVector &_args);
	void cmdWaypointMove(const StringVector &_args);
	void cmdWaypointTranslate(const StringVector &_args);
	void cmdWaypointMirror(const StringVector &_args);
	void cmdWaypointDeleteAxis(const StringVector &_args);
	void cmdWaypointGetWpNames(const StringVector &_args);
	void cmdWaypointColor(const StringVector &_args);
	void cmdWaypointAddAvoidWeight(const StringVector &_args);
	void cmdSelectWaypoints(const StringVector &_args);
	void cmdSelectWaypoints_Helper(const Vector3f &_pos, float _radius);
	void cmdLockSelected(const StringVector &_args);
	void cmdUnlockAll(const StringVector &_args);
	void cmdMinRadius(const StringVector &_args);
	void cmdMaxRadius(const StringVector &_args);
	void cmdAutoBuildFeatures(const StringVector &_args);
	void cmdBoxSelect(const StringVector &_args);
	void cmdBoxSelectRoom(const StringVector &_args);
	void cmdWaypointSlice(const StringVector &_args);
	void cmdWaypointSplit(const StringVector &_args);
	void cmdWaypointUnSplit(const StringVector &_args);
	void cmdWaypointGround(const StringVector &_args);

	Waypoint_Header		m_WaypointHeader;
	VisFile_Header		m_VisFileHeader;
	String	m_NavDir;

	typedef enum
	{
		CLOSEST_WP			= 1<<0,
		ALL_IN_RADIUS		= 1<<1,
		ALL_IN_RADIUS_LOS	= 1<<2,
	} MarkOptions;

	typedef enum
	{
		NOFILTER			= 1<<0,
		SKIP_NO_CONNECTIONS	= 1<<1,
	} GetClosestFlags;

	struct EntityConnection
	{
		GameEntity					Entity;
		int							ConnectionId;
		ConnDir						Direction;
		
		Waypoint *					Wp;

		EntityConnection()
			: ConnectionId(0)
			, Direction(CON_TWO_WAY)
			, Wp(0)
		{
		}
	};

	enum { MaxEntityConnections = 32 };
	EntityConnection				EntityConnections[MaxEntityConnections];

	void _RunDijkstra(const NavFlags _team);
	void _RunAStar(const NavFlags _team, const Vector3f &_goalPosition);

	int m_PathSerial;

	void _FindAllReachable(Client *_client, const Vector3f &_pos, const NavFlags &_team, WaypointList & reachable);

	ClosestLink _GetClosestLink(const Vector3f &_pos, const NavFlags _team) const;
	void HeapInsert(WaypointList &_wpl, Waypoint *_wp);

	obuint32 m_WaypointMark;
	obuint32 m_GoalIndex;
	int _MarkWaypointsInRadius(const Vector3f &_pos, const NavFlags _team, int _flags);

	bool LoadFromFile(const String &_file);

	//bool _LoadVisFromFile(const String &_file);
	//bool _SaveVisToFile(const String &_file);

	/*bool _SaveVis(const String &_file, File &_proxy);
	bool _LoadVis(const String &_file, File &_proxy);*/
	int	m_MovingWaypointIndex;

	Vector3f	m_BoxStart;
	AABB boxselect;

	void UpdateNavRender();
	void UpdateSelectedWpRender();

	WaypointSerializers	m_WaypointSerializer;

	gmGCRoot<gmUserObject> m_WpRef;

	// Internal Implementations of base class functionality
	String _GetNavFileExtension() { return ".way"; }
	virtual void _BenchmarkPathFinder(const StringVector &_args);
	virtual void _BenchmarkGetNavPoint(const StringVector &_args);
};

namespace NavigationAssertions 
{
	BOOST_STATIC_ASSERT(sizeof(PathPlannerWaypoint::Waypoint_Header) == 293);
}

#endif
