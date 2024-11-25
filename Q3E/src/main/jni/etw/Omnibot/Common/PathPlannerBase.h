#ifndef __PATHPLANNER_H__
#define __PATHPLANNER_H__

class Goal;
class GoalQueue;
class Client;
class Path;
class gmMachine;
struct Event_EntityConnection;

#include "EventReciever.h"
#include "CommandReciever.h"

namespace gcn
{
	class Graphics;
	class Widget;
}

namespace NavigationAssertions 
{
	BOOST_STATIC_ASSERT(sizeof(NavFlags) == 8); // 8 bytes = 64 bits
	//BOOST_STATIC_ASSERT(sizeof(obUserData) == 16); // cs: FIXME 64 bit struct size is different. do we really need this?
}

struct rcConfig;
struct EntityInstance;

// class: PathPlannerBase
//		Abstract Base Class for Path Planning
//		Provides the interface functions for running a pathfinding query. 
//		Shouldn't care about the underlying implementation of the pathing system.
class PathPlannerBase : public CommandReciever
{
public:

	enum BasePlannerFlags
	{
		NAV_VIEW,
		NAV_VIEWCONNECTIONS,
		NAV_FOUNDGOAL,
		NAV_SAVEFAILEDPATHS,
		NAV_AUTODETECTFLAGS,

		NUM_BASE_NAVFLAGS
	};

	virtual bool Init() = 0;
	virtual void Update() = 0;
	virtual void Shutdown() = 0;
	virtual bool IsReady() const = 0;
	virtual void DrawActiveFrame() {}

	virtual Vector3f GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team) = 0;

	virtual void PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team) = 0;
	virtual int PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team) = 0;
	virtual int PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team) = 0;

	virtual bool GetNavFlagByName(const String &_flagname, NavFlags &_flag) const = 0;

	virtual Vector3f GetDisplayPosition(const Vector3f &_pos) = 0; // deprecated

	bool IsViewOn() const { return m_PlannerFlags.CheckFlag(NAV_VIEW); }
	bool IsViewConnectionOn() const { return m_PlannerFlags.CheckFlag(NAV_VIEWCONNECTIONS); }
	bool IsAutoDetectFlagsOn() const { return m_PlannerFlags.CheckFlag(NAV_AUTODETECTFLAGS); }

	virtual bool IsDone() const= 0;
	virtual bool FoundGoal() const = 0;
	virtual bool Load(bool _dl = true);
	virtual bool Load(const String &_mapname,bool _dl = true) = 0;
	virtual bool Save(const String &_mapname) = 0;
	virtual void Unload() = 0;

	virtual int GetLatestFileVersion() const = 0;

	virtual void RegisterGameGoals() = 0;
	virtual void GetPath(Path &_path, int _smoothiterations = 3) = 0;

	virtual const char *GetPlannerName() const = 0;
	virtual int GetPlannerType() const = 0;

	virtual void RegisterNavFlag(const String &_name, const NavFlags &_bits) = 0;

	virtual void RegisterScriptFunctions(gmMachine *a_machine) = 0;

#ifdef ENABLE_DEBUG_WINDOW
	virtual void CreateGui() {}
	virtual void UpdateGui() {}
	virtual void RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics) {}
#endif 

	struct AvoidData 
	{
		int		m_Team;
		float	m_Radius;
		float	m_AvoidWeight;
	};

	virtual void MarkAvoid(const Vector3f &_pos, const AvoidData&_data) {}

	virtual bool GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name) = 0;

	virtual void AddEntityConnection(const Event_EntityConnection &_conn) = 0;
	virtual void RemoveEntityConnection(GameEntity _ent) = 0;
	virtual void EntityCreated(const EntityInstance &ei) {}
	virtual void EntityDeleted(const EntityInstance &ei) {}

#ifdef ENABLE_REMOTE_DEBUGGING
	virtual void Sync( RemoteLib::DataBuffer & db, bool fullSync ) { }
#endif

	PathPlannerBase() {};
	virtual ~PathPlannerBase() {};
protected:
	struct FailedPath
	{
		Vector3f	m_Start;
		Vector3f	m_End;
		int			m_NextRenderTime;
		bool		m_Render;
	};
	typedef std::list<FailedPath> FailedPathList;

	FailedPathList	m_FailedPathList;

	void AddFailedPath(const Vector3f &_start, const Vector3f &_end);

	void RenderFailedPaths();

	void InitCommands();
	void cmdLogFailedPaths(const StringVector &_args);
	void cmdShowFailedPaths(const StringVector &_args);
	void cmdBenchmarkPathFind(const StringVector &_args);
	void cmdBenchmarkGetNavPoint(const StringVector &_args);
	void cmdResaveNav(const StringVector &_args);

	BitFlag32			m_PlannerFlags;

	//////////////////////////////////////////////////////////////////////////
	// Required subclass functions
	virtual String _GetNavFileExtension() = 0;
	virtual void _BenchmarkPathFinder(const StringVector &_args);
	virtual void _BenchmarkGetNavPoint(const StringVector &_args);
};

#endif 
