#ifndef __PATHPLANNERNAVMESH_H__
#define __PATHPLANNERNAVMESH_H__

#include "PathPlannerBase.h"
#include "InternalFsm.h"

class QuadTree;
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<QuadTree> QuadTreePtr;
#else
typedef boost::shared_ptr<QuadTree> QuadTreePtr;
#endif

//////////////////////////////////////////////////////////////////////////

enum ToolStateNavMesh
{
	NoOp,
	PlaceSector,
	SliceSector,
	SliceSectorWithSector,
	EditSector,
	SplitSector,
	TraceSector,
	GroundSector,
	CommitSector,
	MirrorSectors,
	PlaceBorder,
	NumToolStates
};

namespace NavigationMeshOptions
{
	extern float CharacterHeight;
}

// class: PathPlannerNavMesh
//		Path planner interface for the navmesh system for hl2
class PathPlannerNavMesh : public PathPlannerBase, public InternalFSM<PathPlannerNavMesh,NumToolStates>
{
public:
	enum NavMeshFlags
	{
		NAVMESH_STEPPROCESS = NUM_BASE_NAVFLAGS,
		NAVMESH_TAKESTEP,
	};

	enum NodeDirection
	{
		NORTH,
		SOUTH,
		EAST,
		WEST,
		NUM_DIRS
	};

	//////////////////////////////////////////////////////////////////////////

	struct NavSector
	{
		int				m_Id;
		int				m_StartPortal;
		int				m_NumPortals;
		Vector3f		m_Middle;
		Vector3List		m_Boundary;

		enum eMirror { MirrorNone,MirrorX,MirrorNX,MirrorY,MirrorNY,MirrorZ,MirrorNZ };
		obuint32		m_Mirror : 3;
		NavSector()
			: m_Id(0)
			, m_StartPortal(0)
			, m_NumPortals(0)
			, m_Mirror(MirrorNone)
		{
		}

		NavSector GetMirroredCopy(const Vector3f &offset) const;
		SegmentList GetEdgeSegments() const;
		void GetEdgeSegments(SegmentList &_list) const;
	};
	struct NavPortal
	{
		Segment3f		m_Segment;

		int				m_DestSector;
		int				m_DestPortal;

		BitFlag64		m_LinkFlags;
	};

	bool DoesSectorAlreadyExist(const NavSector &_ns);

	NavSector *GetSectorAt(const Vector3f &_pos, float _distance = 1024.f);
	NavSector *GetSectorAtFacing(const Vector3f &_pos, const Vector3f &_facing, float _distance = 1024.f);

	bool Init();
	void Update();
	void Shutdown();
	bool IsReady() const;

	int GetLatestFileVersion() const;

	Vector3f GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team);

	void PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team);
	int PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team);
	int PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team);

	bool GetNavFlagByName(const String &_flagname, NavFlags &_flag) const;

	Vector3f GetDisplayPosition(const Vector3f &_pos);

	bool IsDone() const;
	bool FoundGoal() const;
	bool Load(const String &_mapname, bool _dl = true);
	bool Save(const String &_mapname);
	void Unload();
	bool SetFileComments(const String &_text);

	void RegisterGameGoals();
	void GetPath(Path &_path, int _smoothiterations);

	virtual void RegisterNavFlag(const String &_name, const NavFlags &_bits) {}

	void RegisterScriptFunctions(gmMachine *a_machine);

	bool GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name);

	void AddEntityConnection(const Event_EntityConnection &_conn);
	void RemoveEntityConnection(GameEntity _ent);

#ifdef ENABLE_DEBUG_WINDOW
	void RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics);
#endif

	const char *GetPlannerName() const { return "Navigation Mesh Path Planner"; } ;
	int GetPlannerType() const { return NAVID_NAVMESH; };

	PathPlannerNavMesh();
	virtual ~PathPlannerNavMesh();
protected:

	void InitCommands();
	void cmdNavSave(const StringVector &_args);
	void cmdNavLoad(const StringVector &_args);
	void cmdNavView(const StringVector &_args);
	void cmdNavViewConnections(const StringVector &_args);
	void cmdNavStep(const StringVector &_args);
	void cmdNavEnableStep(const StringVector &_args);
	void cmdAutoBuildFeatures(const StringVector &_args);
	void cmdStartPoly(const StringVector &_args);
	void cmdUndoPoly(const StringVector &_args);
	/*void cmdLoadObj(const StringVector &_args);
	void cmdLoadMap(const StringVector &_args);*/
	void cmdCreatePlanePoly(const StringVector &_args);
	void cmdCreateSlicePoly(const StringVector &_args);
	void cmdCreateSliceSector(const StringVector &_args);
	void cmdCommitPoly(const StringVector &_args);
	void cmdDeleteSector(const StringVector &_args);
	void cmdMirrorSectors(const StringVector &_args);
	void cmdSectorSetProperty(const StringVector &_args);
	void cmdEditSector(const StringVector &_args);
	void cmdSplitSector(const StringVector &_args);
	void cmdGroundSector(const StringVector &_args);
	void cmdSectorCreateConnections(const StringVector &_args);
	void cmdSetMapCenter(const StringVector &_args);
	
	void cmdNext(const StringVector &_args);

	//////////////////////////////////////////////////////////////////////////
	// Friend functions
	friend int GM_CDECL gmfNavMeshView(gmThread *a_thread);
	friend int GM_CDECL gmfNavMeshViewConnections(gmThread *a_thread);
	friend int GM_CDECL gmfNavMeshEnableStep(gmThread *a_thread);
	friend int GM_CDECL gmfNavMeshStep(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////

	friend class PathFindNavMesh;
protected:

	//////////////////////////////////////////////////////////////////////////
	struct AttribFields
	{		
		obuint32		Mirrored : 1;
		obuint32		ActiveId : 12;
		obuint32		SectorId : 12;
		obuint32		UnUsed : 7;
	};
	union PolyAttrib
	{
		obuint32		Attrib;
		AttribFields	Fields;
	};
	//////////////////////////////////////////////////////////////////////////
	class NavCollision
	{
	public:

		bool DidHit() const { return m_HitSomething; }
		const PolyAttrib HitAttrib() const { return m_HitAttrib; }
		const Vector3f &HitPosition() const { return m_HitPosition; }
		const Vector3f &HitNormal() const { return m_HitNormal; }

		NavCollision(bool _hit, const Vector3f &_pos = Vector3f::ZERO, const Vector3f &_normal = Vector3f::ZERO, obint32 _attrib = 0)
			: m_HitPosition(_pos)
			, m_HitNormal(_normal)
			, m_HitSomething(_hit)
		{
			m_HitAttrib.Attrib = _attrib;
		}
	private:
		Vector3f	m_HitPosition;
		Vector3f	m_HitNormal;
		PolyAttrib  m_HitAttrib;
		bool		m_HitSomething : 1;

		NavCollision();
	};
	void InitCollision();
	enum ColFlags { IgnoreMirrored=1, };
	NavCollision FindCollision(const Vector3f &_from, const Vector3f &_to);
	void ReleaseCollision();

	//////////////////////////////////////////////////////////////////////////
	Vector3f			m_MapCenter;

	typedef std::vector<NavSector> NavSectorList;
	NavSectorList		m_NavSectors;
	Vector3List			m_CurrentSector;
	Vector3f			m_CurrentSectorStart;

	NavSectorList		m_ActiveNavSectors;

	typedef std::vector<NavPortal> NavPortalList;	
	NavPortalList		m_NavPortals;	
	//////////////////////////////////////////////////////////////////////////
	// Tool state machine
	STATE_PROTOTYPE(NoOp);
	STATE_PROTOTYPE(PlaceSector);
	STATE_PROTOTYPE(SliceSector);
	STATE_PROTOTYPE(SliceSectorWithSector);
	STATE_PROTOTYPE(EditSector);
	STATE_PROTOTYPE(SplitSector);
	STATE_PROTOTYPE(TraceSector);
	STATE_PROTOTYPE(GroundSector);	
	STATE_PROTOTYPE(CommitSector);
	STATE_PROTOTYPE(MirrorSectors);
	STATE_PROTOTYPE(PlaceBorder);

	//////////////////////////////////////////////////////////////////////////
	// Current tool variables
	obColor				m_CursorColor;
	Vector3List			m_WorkingSector;
	Vector3f			m_WorkingSectorStart;
	Vector3f			m_WorkingSectorNormal;

	Vector3List			m_WorkingManualSector;

	Plane3f				m_WorkingSectorPlane;
	Plane3f				m_WorkingSlicePlane;

	bool				m_ToolCancelled;

	Vector3f _SectorVertWithin(const Vector3f &_pos1, const Vector3f &_pos2, float _range, bool *_snapped = NULL);

	//////////////////////////////////////////////////////////////////////////
	// Internal Implementations of base class functionality
	String _GetNavFileExtension() { return ".nav"; }
	virtual void _BenchmarkPathFinder(const StringVector &_args);
	virtual void _BenchmarkGetNavPoint(const StringVector &_args);
};

#endif
