#ifndef __PATHPLANNERFLOODFILL_H__
#define __PATHPLANNERFLOODFILL_H__

#include "PathPlannerBase.h"
#include "InternalFsm.h"

class QuadTree;
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<QuadTree> QuadTreePtr;
#else
typedef boost::shared_ptr<QuadTree> QuadTreePtr;
#endif

//////////////////////////////////////////////////////////////////////////

//enum ToolStateFloodFill
//{
//	NoOp,
//	/*PlaceSector,
//	SliceSector,
//	SliceSectorWithSector,
//	EditSector,
//	SplitSector,
//	TraceSector,
//	GroundSector,
//	CommitSector,
//	MirrorSectors,
//	PlaceBorder,*/
//	NumToolStates
//};

namespace NavigationMeshOptions
{
	extern float CharacterHeight;
}

// class: PathPlannerFloodFill
//		Path planner interface for the navmesh system for hl2
class PathPlannerFloodFill : public PathPlannerBase, public RenderOverlayUser//, public InternalFSM<PathPlannerFloodFill,NumToolStates>
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

	enum
	{
		NUM_FLOOD_NODES = 262140,
		NUM_SECTOR_NODES = 32768,
	};

	struct FloodFillOptions
	{
		float		m_CharacterHeight;
		float		m_CharacterCrouchHeight;
		float		m_CharacterStepHeight;
		float		m_CharacterJumpHeight;
		float		m_GridRadius;
	};
	
	struct NavNode
	{
		Vector3f	m_Position;
		Vector3f	m_Normal;
		int			m_DistanceFromEdge;
		int			m_Region;
		int			m_Sector;
		bool		m_Open : 1;
		bool		m_ValidPos : 1;
		bool		m_Sectorized : 1;
		bool		m_InWater : 1;
		//bool		m_Movable : 1;
		bool		m_NearSolid : 1;
		bool		m_NearVoid : 1;
		bool		m_Crouch : 1;

		// link flags
		//bool		m_Jump : 1;
		//bool		m_Ladder : 1;
		//bool		m_Teleporter : 1;

		struct Connection
		{
			int			Index;
			obuint32	Jump : 1;
			obuint32	Ladder : 1;

			void Reset()
			{
				Index = -1;
				Jump = false;
				Ladder = false;
			}
		};

		Connection		m_Connections[NUM_DIRS];
	};

	struct NavLink
	{
		int	m_TargetCell;
		
		// flags
		// todo: move from above
	};

	struct FaceProps
	{
		bool		m_InWater : 1;
		bool		m_Jump : 1;
		bool		m_Ladder : 1;
		bool		m_Teleporter : 1;
		bool		m_Movable : 1;
		bool		m_Sectorized : 1;
		bool		m_NearSolid : 1;
		bool		m_NearVoid : 1;
	};

	typedef struct  
	{
		int		m_NumIterations;
		int		m_NumNodes;
		double	m_TotalTime;
	} GenerationStats;

	struct SectorLink
	{
		int			m_Sector;
		Vector3f	m_From, m_To;
	};

	typedef std::list<SectorLink> SectorLinks;
	typedef std::set<int> CellSet;

	struct Sector
	{
		AABB		m_SectorBounds;
		Vector3f	m_Normal;

		CellSet		m_ContainingCells;
		SectorLinks	m_SectorLinks;
	};

	struct FloodFillData
	{
		Timer				m_Timer;
		AABB				m_GridAABB;

		AABB				m_WorldAABB;
		Vector3f			m_CurrentCell;

		GenerationStats		m_Stats;

		QuadTreePtr			m_QuadTree;
		QuadTreePtr			m_TriMeshQuadTree;
		//////////////////////////////////////////////////////////////////////////
		// aabb generation
		obuint32			m_NodeIndex;
		NavNode				m_Nodes[NUM_FLOOD_NODES];

		//////////////////////////////////////////////////////////////////////////
		// Mesh generation
		struct Quad 
		{
			obuint32	m_Verts[4];
			Vector3f	m_Normal;
			FaceProps	m_Properties;
		};

		Vector3f	m_Vertices[NUM_FLOOD_NODES];
		obuint32	m_VertIndex;	
		Quad		m_Faces[NUM_FLOOD_NODES];
		obuint32	m_FaceIndex;
	};

	//////////////////////////////////////////////////////////////////////////

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
	void CreateGui();
	void UpdateGui();
	void RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics);
#endif

	const char *GetPlannerName() const { return "Flood Fill Path Planner"; } ;
	int GetPlannerType() const { return NAVID_FLOODFILL; };

	PathPlannerFloodFill();
	virtual ~PathPlannerFloodFill();
protected:

	void InitCommands();
	void cmdNavSave(const StringVector &_args);
	void cmdNavLoad(const StringVector &_args);
	void cmdNavView(const StringVector &_args);
	void cmdNavViewConnections(const StringVector &_args);
	void cmdNavStep(const StringVector &_args);
	void cmdNavEnableStep(const StringVector &_args);
	void cmdAddFloodStart(const StringVector &_args);
	void cmdClearFloodStarts(const StringVector &_args);
	void cmdSaveFloodStarts(const StringVector &_args);
	void cmdLoadFloodStarts(const StringVector &_args);
	void cmdNavMeshFloodFill(const StringVector &_args);
	void cmdNavMeshTrimSectors(const StringVector &_args);
	void cmdAutoBuildFeatures(const StringVector &_args);

	/*void cmdLoadObj(const StringVector &_args);
	void cmdLoadMap(const StringVector &_args);*/

	void cmdSectorSetProperty(const StringVector &_args);

	void cmdNext(const StringVector &_args);

	// Process Functions
	int Process_FloodFill();

	//////////////////////////////////////////////////////////////////////////
	void AddFloodStart(const Vector3f &_vec);
	void ClearFloodStarts();
	void SaveFloodStarts();
	void LoadFloodStarts();
	void FloodFill(const FloodFillOptions &_options);
	void TrimSectors(float _trimarea);

	//////////////////////////////////////////////////////////////////////////
	// Friend functions
	friend int GM_CDECL gmfFloodFillView(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillViewConnections(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillEnableStep(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillStep(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillAddFloodStart(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillClearFloodStarts(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillLoadFloodStarts(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillSaveFloodStarts(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillFloodFill(gmThread *a_thread);
	friend int GM_CDECL gmfFloodFillTrimSectors(gmThread *a_thread);

	//////////////////////////////////////////////////////////////////////////

	friend class PathFindFloodFill;
protected:
	obColor GetFaceColor(const FloodFillData::Quad &_qd);

	void AddFaceToNavMesh(const Vector3f &_pos, const NavNode &_navnode);
	Vector3f _GetNearestGridPt(const Vector3f &_pt);

	void _InitFloodFillData();
	bool _IsNodeGood(NavNode *_navnode, const AABB &_bounds);
	void _GetNodeProperties(NavNode &_node);
	void _AddNode(const Vector3f &_pos);
	NavNode *_GetNextOpenNode();
	bool _IsStartPositionValid(const Vector3f &_pos);
	void _ExpandNode(NavNode *_navnode);
	NavNode *_GetExpansionNode(NavNode *_navnode, NodeDirection _dir, float _step);

	bool _IsNodeValidForSectorizing(const Sector &_sector, const NavNode &_node) const;
	void _MarkNodesSectorized(const std::vector<int> &_nodelist);
	void _BuildSector(int _floodNodeIndex);

	void _RenderFloodFill();
	void _RenderSectors();

	Vector3f _SectorVertWithin(const Vector3f &_pos1, const Vector3f &_pos2, float _range, bool *_snapped = NULL);

	//////////////////////////////////////////////////////////////////////////
	/*struct AttribFields
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
			: m_HitSomething(_hit)
			, m_HitPosition(_pos)
			, m_HitNormal(_normal)
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
	void ReleaseCollision();*/

	//////////////////////////////////////////////////////////////////////////

	#if __cplusplus >= 201103L //karin: using C++11 instead of boost
	typedef std::shared_ptr<FloodFillData> FloodFillDataPtr;
	#else
	typedef boost::shared_ptr<FloodFillData> FloodFillDataPtr;
	#endif
	
	FloodFillOptions	m_FloodFillOptions;
	FloodFillDataPtr	m_FloodFillData;
	Vector3List			m_StartPositions;
	
	typedef std::vector<Sector> SectorList;
	SectorList			m_Sectors;

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Tool state machine
	//STATE_PROTOTYPE(NoOp);

	//////////////////////////////////////////////////////////////////////////
	// Current tool variables
	obColor				m_CursorColor;
	/*Vector3List			m_WorkingSector;
	Vector3f			m_WorkingSectorStart;
	Vector3f			m_WorkingSectorNormal;

	Vector3List			m_WorkingManualSector;

	Plane3f				m_WorkingSectorPlane;
	Plane3f				m_WorkingSlicePlane;

	bool				m_ToolCancelled;*/

	//////////////////////////////////////////////////////////////////////////
	// Internal Implementations of base class functionality
	String _GetNavFileExtension() { return ".nav"; }
	virtual void _BenchmarkPathFinder(const StringVector &_args);
	virtual void _BenchmarkGetNavPoint(const StringVector &_args);

	private:
		void OverlayRender(RenderOverlay *overlay, const ReferencePoint &viewer);
};

#endif
