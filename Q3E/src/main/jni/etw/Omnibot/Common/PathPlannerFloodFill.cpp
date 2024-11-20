#include "PrecompCommon.h"
#include "PathPlannerFloodFill.h"
#include "IGameManager.h"
#include "IGame.h"
#include "GoalManager.h"
#include "NavigationFlags.h"
#include "AStarSolver.h"
#include "gmUtilityLib.h"

#include "QuadTree.h"
using namespace std;

//////////////////////////////////////////////////////////////////////////

//class FloodFiller
//{
//public:
//	enum Direction
//	{
//		NORTH,
//		SOUTH,
//		EAST,
//		WEST,
//		NUM_DIRS
//	};
//
//	void Reset();
//	void Start();
//	void Iterate(float aMaxTime);
//	void Pause();
//	
//	FloodFiller();
//private:
//	float				MaxStepHeight;
//	float				MaxJumpHeight;
//
//	float				StandHeight;
//	float				CrouchHeight;
//
//	float				GridStep;
//
//	AABB				CellAABB;
//
//	QuadTreePtr			mQuadTree;
//};
//
//void FloodFiller::Reset() 
//{
//}
//void FloodFiller::Start() 
//{
//}
//void FloodFiller::Iterate(float aMaxTime) 
//{
//}
//void FloodFiller::Pause() 
//{
//}
//
//FloodFiller::FloodFiller()
//	: MaxStepHeight()
//	, MaxJumpHeight()
//	, StandHeight()
//	, CrouchHeight()
//	, GridStep(16.f)
//{
//	CellAABB = AABB(Vector3f::ZERO);
//	CellAABB.Expand(Vector3f(GridStep, GridStep, 0.0f));
//	CellAABB.Expand(Vector3f(-GridStep, -GridStep, 0.0f));
//}

//////////////////////////////////////////////////////////////////////////

PathPlannerFloodFill *g_PlannerFloodFill = 0;

//////////////////////////////////////////////////////////////////////////
// Pathing vars
class PathFindFloodFill
{
public:
	struct PlanNode
	{
		PlanNode								*Parent;
		Vector3f								Position;
		/*PathPlannerFloodFill::NavSector			*Sector;
		
		
		const PathPlannerFloodFill::NavPortal		*Portal;

		int										BoundaryEdge;*/

		float									CostHeuristic;
		float									CostGiven;
		float									CostFinal;

		obint32 Hash() const
		{
			return 0;
			/*return (obint32)Utils::MakeId32(
				(obint16)Sector->m_Id,
				Portal ? (obint16)Portal->m_DestSector : (obint16)0xFF);*/
		}

		void Reset()
		{
			Parent = 0;
			/*Sector = 0;
			Portal = 0;
			BoundaryEdge = -1;*/

			CostHeuristic = 0.f;
			CostGiven = 0.f;
			CostFinal = 0.f;
		}
	};
	typedef std::vector<PlanNode*> NodeList;

	NodeList &GetSolution() { return m_Solution; }

	bool IsFinished() const
	{
		return fl.Finished;
	}
	bool FoundGoal() const
	{
		return FoundGoalIndex != -1;
	}
	int GetGoalIndex() const
	{
		return FoundGoalIndex;
	}
	void StartNewSearch()
	{
		m_CurrentGoals.resize(0);

		m_ClosedList.clear();
		m_OpenList.resize(0);
		m_Solution.resize(0);

		fl.Finished = 0;

		m_UsedNodes = 0;

		FoundGoalIndex = -1;
	}
	bool AddStart(const Vector3f &_pos)
	{
		/*PathPlannerFloodFill::NavSector *pSector = g_PlannerNavMesh->GetSectorAt(_pos,512.f);
		if(pSector)
		{
			PlanNode *pNode = _GetFreeNode();
			pNode->Parent = 0;
			pNode->Position = _pos;
			pNode->Sector = pSector;
			m_OpenList.push_back(pNode);
			return true;
		}*/
		return false;
	}
	bool AddGoal(const Vector3f &_pos)
	{
		/*GoalLocation gl;
		gl.Position = _pos;
		gl.Sector = g_PlannerNavMesh->GetSectorAt(_pos,512.f);
		if(gl.Sector)
		{
			m_CurrentGoals.push_back(gl);
			return true;
		}*/
		return false;
	}
	void Iterate(int _numsteps = 1)
	{
		while(!fl.Finished && _numsteps--)
		{
			// get the next node
			PlanNode *pCurNode = _GetNextOpenNode();
			if(!pCurNode)
			{
				fl.Finished = true;
				break;
			}

			// Push it on the list so it doesn't get considered again.
			_MarkClosed(pCurNode);

			// Is it the goal?
			GoalLocation *gl = _IsNodeGoalInSector(pCurNode);

			//////////////////////////////////////////////////////////////////////////
			if(gl /*&& SquaredLength(gl->Position,pCurNode->Position)<Mathf::Sqr(32.f)*/)
			{
				FoundGoalIndex = (int)(gl - &m_CurrentGoals[0]);
				fl.Finished = true;

				m_Solution.resize(0);
				
				// add all the nodes, goal->start
				while(pCurNode)
				{
					m_Solution.push_back(pCurNode);
					pCurNode = pCurNode->Parent;
				}
				break;
			}
			//////////////////////////////////////////////////////////////////////////

			// Expand the node
			_ExpandNode(pCurNode,gl);
		}
	}
	void Render()
	{
		static int NEXT_DRAW = 0;
		if(IGame::GetTime()>NEXT_DRAW)
		{
			NEXT_DRAW = IGame::GetTime() + 2000;
			for(int i = 0; i < m_UsedNodes; ++i)
			{
				PlanNode *pNode = &m_Nodes[i];

				obColor col = COLOR::BLACK;
				if(IsOnOpenList(pNode) != m_OpenList.end())
					col = COLOR::GREEN;

				if(pNode->Parent)
					Utils::DrawLine(pNode->Position,pNode->Parent->Position,col,2.f);
			}
		}
	}
	PathFindFloodFill()
	{
		fl.Finished = true;
		m_UsedNodes = 0;
	}
private:
	struct GoalLocation
	{
		/*PathPlannerFloodFill::NavSector	*Sector;
		Vector3f						Position;*/
	};
	typedef std::vector<GoalLocation> GoalList;
	GoalList	m_CurrentGoals;

	enum		{ MaxNodes=2048 };
	PlanNode	m_Nodes[MaxNodes];
	int			m_UsedNodes;	

	static bool NAV_COMP(const PlanNode* _n1, const PlanNode* _n2)
	{
		return _n1->CostFinal > _n2->CostFinal;
	}
	/*static int HashNavNode(const PlanNode *_n1)
	{
	return _n1->Hash;
	}*/

	//typedef boost::fast_pool_allocator< std::pair< const int, PlanNode* >, boost::default_user_allocator_new_delete, boost::details::pool::default_mutex, 769 > HashMapAllocator;
	//typedef FSBAllocator< std::pair< int, PlanNode* > > HashMapAllocator;

#ifdef WIN32
	typedef stdext::hash_compare<uintptr_t> HashMapCompare;
	typedef stdext::unordered_map<uintptr_t, PlanNode*, HashMapCompare/*, HashMapAllocator*/ > NavHashMap;
#else
	typedef stdext::hash<uintptr_t> HashMapCompare;
	typedef stdext::unordered_map<uintptr_t, PlanNode*, HashMapCompare, stdext::equal_to<uintptr_t>/*, HashMapAllocator*/ > NavHashMap;
#endif
	NodeList			m_OpenList;
	NodeList			m_Solution;

	//typedef std::multimap<int,PlanNode*> NodeClosedList; // temp?
	typedef NavHashMap NodeClosedList;
	NavHashMap			m_ClosedList;

	int									FoundGoalIndex;

	struct  
	{
		obuint32							Finished : 1;
	} fl;

	PlanNode *_GetFreeNode()
	{
		PlanNode *pNode = &m_Nodes[m_UsedNodes++];
		pNode->Reset();
		return pNode;
	}
	PlanNode *_GetNextOpenNode()
	{
		PlanNode *pCurNode = NULL;

		if(!m_OpenList.empty())
		{
			pCurNode = m_OpenList.front();
			std::pop_heap(m_OpenList.begin(), m_OpenList.end(), NAV_COMP);
			m_OpenList.pop_back();
		}

		return pCurNode;
	}
	void _MarkClosed(PlanNode *_node)
	{
		m_ClosedList[_node->Hash()] = _node;
		//m_ClosedList.insert(std::make_pair(_node->Sector->m_Id,_node));
	}
	NodeClosedList::iterator IsOnClosedList(PlanNode *_node)
	{
		NodeClosedList::iterator it = m_ClosedList.find(_node->Hash());
		return it;
		/*NodeClosedList::iterator it = m_ClosedList.lower_bound(_node->Sector->m_Id);
		NodeClosedList::iterator itEnd = m_ClosedList.upper_bound(_node->Sector->m_Id);
		while(it != itEnd)
		{
			if(SquaredLength(_node->Position,it->second->Position) < 32.f)
				return it;
			++it;
		}
		return m_ClosedList.end();*/
	}
	NodeList::iterator IsOnOpenList(PlanNode *_node)
	{
		//float fClosest = 1000000.f;
		NodeList::iterator itRet = m_OpenList.end();

		/*NodeList::iterator it = m_OpenList.begin(), itEnd = m_OpenList.end();
		for(; it != itEnd; ++it)
		{
			if((*it)->Sector == _node->Sector)
			{
				const float fSqDist = SquaredLength((*it)->Position, _node->Position);
				if(fSqDist < fClosest)
				{
					fClosest = fSqDist;
					itRet = it;					
				}
			}
		}*/
		return itRet;
	}

	static bool node_compare(const PlanNode* _node1, const PlanNode* _node2)
	{
		return _node1->CostFinal > _node2->CostFinal;
	}

	void HeapInsert(NodeList &_wpl, PlanNode *_node)
	{
		_wpl.push_back(_node);
		std::push_heap(_wpl.begin(), _wpl.end(), node_compare);
	}


	void _ExpandNode(PlanNode *_node, GoalLocation *_goal)
	{
		//if(_goal)
		//{
		//	PlanNode *pNextNode = _GetFreeNode();
		//	pNextNode->Portal = 0;
		//	pNextNode->Parent = _node;
		//	pNextNode->Sector = _node->Sector;
		//	pNextNode->Position = _goal->Position; // TODO: branch more			
		//	pNextNode->CostHeuristic = 0;
		//	pNextNode->CostGiven = _node->CostGiven + Length(_node->Position, pNextNode->Position);
		//	pNextNode->CostFinal = pNextNode->CostHeuristic + pNextNode->CostGiven;
		//	HeapInsert(m_OpenList, pNextNode);
		//	return;
		//}

		//for(int p = _node->Sector->m_StartPortal; 
		//	p < _node->Sector->m_StartPortal+_node->Sector->m_NumPortals; 
		//	++p)
		//{
		//	const PathPlannerFloodFill::NavPortal &portal = g_PlannerNavMesh->m_NavPortals[p];

		//	//for(int b = 0; b < )

		//	//////////////////////////////////////////////////////////////////////////
		//	PlanNode tmpNext;
		//	tmpNext.Reset();
		//	tmpNext.Portal = &portal;
		//	tmpNext.Parent = _node;
		//	tmpNext.Sector = &g_PlannerNavMesh->m_ActiveNavSectors[portal.m_DestSector];
		//	tmpNext.Position = portal.m_Segment.Origin; // TODO: branch more		

		//	//if(m_CurrentGoals.size()==1)
		//	//	tmpNext.CostHeuristic = Length(m_CurrentGoals.front().Position,tmpNext.Position); // USE IF 1 GOAL!
		//	//else
		//		tmpNext.CostHeuristic = 0;
		//	
		//	tmpNext.CostGiven = _node->CostGiven + Length(_node->Position, tmpNext.Position);
		//	tmpNext.CostFinal = tmpNext.CostHeuristic + tmpNext.CostGiven;

		//	//////////////////////////////////////////////////////////////////////////

		//	// Look in closed list for this. If this is better, re-open it
		//	NodeClosedList::iterator closedIt = IsOnClosedList(&tmpNext);
		//	if(closedIt != m_ClosedList.end())
		//	{
		//		PlanNode *OnClosed = closedIt->second;
		//		if(OnClosed->CostGiven > tmpNext.CostGiven)
		//		{
		//			*OnClosed = tmpNext;

		//			// and remove from the closed list.
		//			m_ClosedList.erase(closedIt);
		//			// ... back into the open list
		//			HeapInsert(m_OpenList, OnClosed);
		//		}
		//		continue;
		//	}

		//	// Look in open list for this. If this is better, update it.
		//	// Check the open list for this node to see if it's better
		//	NodeList::iterator openIt = IsOnOpenList(&tmpNext);
		//	if(openIt != m_OpenList.end())
		//	{
		//		PlanNode *pOnOpen = (*openIt);
		//		if(pOnOpen->CostGiven > tmpNext.CostGiven)
		//		{
		//			// Update the open list					
		//			*pOnOpen = tmpNext;

		//			// Remove it from the open list first.
		//			//m_OpenList.erase(openIt);
		//			// ... and re-insert it

		//			// Since we possibly removed from the middle, we need to fix the heap again.
		//			//std::make_heap(m_OpenList.begin(), m_OpenList.end(), waypoint_comp);
		//			std::push_heap(m_OpenList.begin(), openIt+1, node_compare);
		//		}
		//		continue;
		//	}

		//	// New node to explore
		//	PlanNode *pNextNode = _GetFreeNode();
		//	*pNextNode = tmpNext;
		//	HeapInsert(m_OpenList, pNextNode);
		//}
	}
	GoalLocation *_IsNodeGoalInSector(PlanNode *_node)
	{
		/*for(obuint32 i = 0; i < m_CurrentGoals.size(); ++i)
		{
			if(m_CurrentGoals[i].Sector == _node->Sector)
			{
				return &m_CurrentGoals[i];
			}
		}*/
		return NULL;
	}
};

PathFindFloodFill g_PathFinder;

//////////////////////////////////////////////////////////////////////////

//void PathPlannerFloodFill::InitCollision()
//{
//	if(m_NavSectors.empty())
//		return;
//
//	m_ActiveNavSectors.resize(0);
//
//	ReleaseCollision();
//
//	//////////////////////////////////////////////////////////////////////////
//	TA::Physics::CreateInstance();
//	TA::Physics& physics = TA::Physics::GetInstance();
//	physics.SetPreProcessCollisionCallBack(ColCb);
//	//////////////////////////////////////////////////////////////////////////
//
//	TA::StaticObject* NavMesh = NavMesh = TA::StaticObject::CreateNew();
//
//	obuint32 iNumIndices = 0;
//
//	TA::AABB aabb;
//
//	g_Vertices.clear();
//	g_Indices.clear();
//
//	//////////////////////////////////////////////////////////////////////////
//	std::vector<TA::u32> Attribs;
//	//////////////////////////////////////////////////////////////////////////
//	// Build the active sector list, includes mirrored sectors.
//	PolyAttrib attrib;
//	for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
//	{
//		const NavSector &ns = m_NavSectors[s];
//
//		attrib.Attrib = 0;
//
//		attrib.Fields.Mirrored = false;
//		attrib.Fields.ActiveId = m_ActiveNavSectors.size();
//		attrib.Fields.SectorId = s;
//		Attribs.push_back(attrib.Attrib);
//		
//		m_ActiveNavSectors.push_back(ns);
//		m_ActiveNavSectors.back().m_Id = attrib.Fields.ActiveId;
//		m_ActiveNavSectors.back().m_Middle = Utils::AveragePoint(m_ActiveNavSectors.back().m_Boundary);
//
//		if(ns.m_Mirror != NavSector::MirrorNone)
//		{
//			attrib.Fields.Mirrored = true;
//			attrib.Fields.ActiveId = m_ActiveNavSectors.size();
//			attrib.Fields.SectorId = s;
//			Attribs.push_back(attrib.Attrib);
//
//			m_ActiveNavSectors.push_back(ns.GetMirroredCopy(m_MapCenter));
//			m_ActiveNavSectors.back().m_Id = attrib.Fields.ActiveId;
//			m_ActiveNavSectors.back().m_Middle = Utils::AveragePoint(m_ActiveNavSectors.back().m_Boundary);
//		}
//	}
//	//////////////////////////////////////////////////////////////////////////
//
//	for(obuint32 s = 0; s < m_ActiveNavSectors.size(); ++s)
//	{
//		const NavSector &ns = m_ActiveNavSectors[s];
//
//		IndexList il;
//
//		// Add the vertices and build the index list.
//		for(obuint32 v = 0; v < ns.m_Boundary.size(); ++v)
//		{
//			il.push_back(g_Vertices.size());
//			g_Vertices.push_back(ns.m_Boundary[v]);
//			aabb.ExpandToFit(Convert(ns.m_Boundary[v]));
//		}
//
//		iNumIndices += il.size();
//
//		std::reverse(il.begin(),il.end());
//		g_Indices.push_back(il);
//	}
//
//	physics.SetWorldDimensions(aabb);
//	physics.SetGravity(TA::Vec3(0.0f, IGame::GetGravity(), 0.0f));
//	physics.SetupSimulation();
//
//	//////////////////////////////////////////////////////////////////////////
//
//	TA::CollisionObjectAABBMesh* StaticCollisionObject = TA::CollisionObjectAABBMesh::CreateNew();
//	StaticCollisionObject->Initialise(
//		g_Vertices.size(),
//		g_Indices.size(),
//		iNumIndices); 
//
//	// Add all verts
//	for(obuint32 v = 0; v < g_Vertices.size(); ++v)
//	{
//		const Vector3f &vec = g_Vertices[v];
//		StaticCollisionObject->AddVertex(Convert(vec));
//	}
//
//	// Add all polygons
//	for(obuint32 p = 0; p < g_Indices.size(); ++p)
//	{
//		const IndexList &il = g_Indices[p];
//		StaticCollisionObject->AddPolygon(il.size(), &il[0], Attribs[p]);	
//	}
//	StaticCollisionObject->FinishedAddingGeometry();
//
//	// Initialise the static object with the collision object.
//	NavMesh->Initialise(StaticCollisionObject);
//
//	StaticCollisionObject->Release();
//	StaticCollisionObject = 0;
//
//	physics.AddStaticObject(NavMesh);
//	
//	NavMesh->Release();
//	NavMesh = 0;
//}
//
//PathPlannerFloodFill::NavCollision PathPlannerFloodFill::FindCollision(const Vector3f &_from, const Vector3f &_to)
//{
//	if(!m_ActiveNavSectors.empty())
//	{
//		TA::Vec3 vFrom = Convert(_from);
//		TA::Vec3 vTo = Convert(_to);
//
//		TA::Collision col = TA::Physics::GetInstance().TestLineForCollision(vFrom, vTo, TA::Physics::FLAG_ALL_OBJECTS);
//
//		if(col.CollisionOccurred())
//		{
//			return NavCollision(true, Convert(col.GetPosition()), Convert(col.GetNormal()), col.GetAttributeB());
//		}
//	}
//	return NavCollision(false);
//}
//
//void PathPlannerFloodFill::ReleaseCollision()
//{
//	TA::Physics::DestroyInstance();
//}
//
//PathPlannerFloodFill::NavSector *PathPlannerFloodFill::GetSectorAt(const Vector3f &_pos, float _distance)
//{
//	const float CHAR_HALF_HEIGHT = NavigationMeshOptions::CharacterHeight /** 0.5f*/;
//	return GetSectorAtFacing(_pos+Vector3f(0,0,CHAR_HALF_HEIGHT), -Vector3f::UNIT_Z, _distance);
//}
//
//PathPlannerFloodFill::NavSector *PathPlannerFloodFill::GetSectorAtFacing(const Vector3f &_pos, const Vector3f &_facing, float _distance)
//{
//	if(!m_ActiveNavSectors.empty())
//	{
//		TA::Vec3 vFrom = Convert(_pos);
//		TA::Vec3 vTo = Convert(_pos + _facing * _distance);
//
//		TA::Collision col = TA::Physics::GetInstance().TestLineForCollision(
//			vFrom, vTo, TA::Physics::FLAG_ALL_OBJECTS);
//
//		if(col.CollisionOccurred())
//		{
//			PolyAttrib attr;
//			attr.Attrib = col.GetAttributeB();
//			return &m_ActiveNavSectors[attr.Fields.ActiveId];
//		}
//	}
//	return NULL;
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class NavigationMeshFF
{
public:

	bool SaveNavigationMesh(const String &_filename) { return true; }
	bool LoadNavigationMesh(const String &_filename) { return true; }

	obuint64 CalculateMemoryUsage() 
	{
		obuint64 iSize = m_NavigationSectors.size() * sizeof(NavSector);

		NavSectors::const_iterator cIt = m_NavigationSectors.begin(), cItEnd = m_NavigationSectors.end();
		for(; cIt != cItEnd; ++cIt)
			iSize += (*cIt).m_Connections.size() * sizeof(NavLink);

		return iSize;
	}

	struct NavLink
	{
		int			m_TargetNavSector;
		Vector3f	m_FromPosition;
		Vector3f	m_ToPosition;

		// Connection Flags
		bool		m_Jump : 1;
		bool		m_Ladder : 1;
		bool		m_Teleporter : 1;

		bool		m_RocketJump : 1;
		bool		m_ConcJump : 1;
	};
	typedef std::vector<NavLink> NavLinks;

	struct NavSector
	{
		AABB		m_Bounds;
		NavLinks	m_Connections;
	};
	typedef std::vector<NavSector> NavSectors;
private:

	NavSectors	m_NavigationSectors;
};

//////////////////////////////////////////////////////////////////////////

//AStarSolver<Vector3f> g_AStarSolver;

//////////////////////////////////////////////////////////////////////////

const float g_CharacterHeight = 64.0f;
const float g_CharacterCrouchHeight = 48.0f;
const float g_CharacterStepHeight = 18.0f;
const float g_CharacterJumpHeight = 60.0f;
const float g_GridRadius = 16.0f;

//////////////////////////////////////////////////////////////////////////

PathPlannerFloodFill::PathPlannerFloodFill()
{
	m_FloodFillOptions.m_GridRadius = g_GridRadius;
	m_FloodFillOptions.m_CharacterHeight = g_CharacterHeight;
	m_FloodFillOptions.m_CharacterCrouchHeight = g_CharacterCrouchHeight;
	m_FloodFillOptions.m_CharacterJumpHeight = g_CharacterJumpHeight;
	m_FloodFillOptions.m_CharacterStepHeight = g_CharacterStepHeight;

	// TEMP
	m_PlannerFlags.SetFlag(NAV_VIEW);
	m_PlannerFlags.SetFlag(NAVMESH_STEPPROCESS);

	m_CursorColor = COLOR::BLUE;

	g_PlannerFloodFill = this;
}

PathPlannerFloodFill::~PathPlannerFloodFill()
{
	Shutdown();
	g_PlannerFloodFill = 0;
}

int PathPlannerFloodFill::GetLatestFileVersion() const
{
	return 1; // TODO
}

//void PathPlannerFloodFill::NavSector::GetEdgeSegments(SegmentList &_list) const
//{
//	for(obuint32 m = 0; m < m_Boundary.size()-1; ++m)
//	{
//		_list.push_back(Utils::MakeSegment(m_Boundary[m],m_Boundary[m+1]));
//	}
//	_list.push_back(Utils::MakeSegment(m_Boundary[m_Boundary.size()-1],m_Boundary[0]));
//}
//
//SegmentList PathPlannerFloodFill::NavSector::GetEdgeSegments() const
//{
//	SegmentList lst;
//	GetEdgeSegments(lst);
//	return lst;
//}

bool PathPlannerFloodFill::Init()
{	
	InitCommands();
	return true;
}

void PathPlannerFloodFill::Update()
{
	Prof(PathPlannerFloodFill);

	//UpdateFsm(IGame::GetDeltaTimeSecs());

	if(m_PlannerFlags.CheckFlag(NAV_VIEW))
	{
		g_PathFinder.Render();

		//////////////////////////////////////////////////////////////////////////
		Vector3f vLocalPos, vLocalAim, vAimPos, vAimNormal;
		Utils::GetLocalEyePosition(vLocalPos);
		Utils::GetLocalFacing(vLocalAim);
		if(Utils::GetLocalAimPoint(vAimPos, &vAimNormal))
		{
			Utils::DrawLine(vAimPos, 
				vAimPos + vAimNormal * 16.f, 
				m_CursorColor, 
				IGame::GetDeltaTimeSecs()*2.f);

			Utils::DrawLine(vAimPos, 
				vAimPos + vAimNormal.Perpendicular() * 16.f, m_CursorColor, 
				IGame::GetDeltaTimeSecs()*2.f);
		}
		m_CursorColor = COLOR::BLUE; // back to default
		//////////////////////////////////////////////////////////////////////////
		/*obint32 iCurrentSector = -1;
		NavCollision nc = FindCollision(vLocalPos, vLocalPos + vLocalAim * 1024.f);
		if(nc.DidHit())
		{
			iCurrentSector = nc.HitAttrib().Fields.ActiveId;
		}*/
		//////////////////////////////////////////////////////////////////////////

		//NavSector nsMirrored;

		//static int iLastSector = -1;
		//
		//static int iNextCurSectorTimeUpdate = 0;
		//if(iCurrentSector != -1 &&
		//	(iLastSector != iCurrentSector || IGame::GetTime() >= iNextCurSectorTimeUpdate))
		//{
		//	iLastSector = iCurrentSector;
		//	iNextCurSectorTimeUpdate = IGame::GetTime() + 500;
		//	Utils::DrawPolygon(m_ActiveNavSectors[iCurrentSector].m_Boundary, COLOR::RED, 0.5f, false);
		//}

		//static int iNextTimeUpdate = 0;
		//if(IGame::GetTime() >= iNextTimeUpdate)
		//{
		//	iNextTimeUpdate = IGame::GetTime() + 2000;
		//	//////////////////////////////////////////////////////////////////////////
		//	// Draw our nav sectors
		//	Utils::DrawPolygon(m_CurrentSector, COLOR::GREEN.fade(100), 2.f, false);
		//	Utils::DrawLine(m_CurrentSector, COLOR::GREEN, 2.f, 2.f, COLOR::MAGENTA, true);

		//	for(obuint32 i = 0; i < m_NavSectors.size(); ++i)
		//	{
		//		const NavSector &ns = m_NavSectors[i];
		//		obColor col = COLOR::GREEN;
		//		Utils::DrawLine(ns.m_Boundary, col, 2.f, 2.f, COLOR::MAGENTA, true);
		//		
		//		//////////////////////////////////////////////////////////////////////////
		//		Vector3f vMid = Vector3f::ZERO;
		//		for(obuint32 v = 0; v < ns.m_Boundary.size(); ++v)
		//			vMid += ns.m_Boundary[v];
		//		vMid /= (float)ns.m_Boundary.size();

		//		const char *pMir[] =
		//		{
		//			0,
		//			"x",
		//			"-x",
		//			"y",
		//			"-y",
		//			"z",
		//			"-z",
		//		};

		//		if(pMir[ns.m_Mirror])
		//		{
		//			nsMirrored = ns.GetMirroredCopy(m_MapCenter);
		//			Utils::DrawLine(nsMirrored.m_Boundary, COLOR::CYAN, 2.f, 2.f, COLOR::MAGENTA, true);
		//		}

		//		/*if(pMir[ns.m_Mirror])
		//			Utils::PrintText(vMid, 2.f, "%d m(%s)", i, pMir[ns.m_Mirror]);
		//		else
		//			Utils::PrintText(vMid, 2.f, "%d", i);*/
		//	}

		//	if(m_PlannerFlags.CheckFlag(NAV_VIEWCONNECTIONS))
		//	{
		//		NavPortalList ::const_iterator pIt = m_NavPortals.begin();
		//		for(; pIt != m_NavPortals.end(); ++pIt)
		//		{
		//			const NavPortal &portal = (*pIt);
		//			Utils::DrawLine(portal.m_Segment.GetPosEnd(),
		//				portal.m_Segment.Origin+Vector3f(0,0,32),COLOR::MAGENTA,10.f);
		//			Utils::DrawLine(portal.m_Segment.GetNegEnd(),
		//				portal.m_Segment.Origin+Vector3f(0,0,32),COLOR::MAGENTA,10.f);
		//		}
		//	}
		//	//////////////////////////////////////////////////////////////////////////
		//}

		//////////////////////////////////////////////////////////////////////////

		Vector3List::const_iterator cIt = m_StartPositions.begin();
		for(; cIt != m_StartPositions.end(); ++cIt)
		{
			AABB aabb(Vector3f::ZERO);
			aabb.Expand(Vector3f(m_FloodFillOptions.m_GridRadius, m_FloodFillOptions.m_GridRadius, 0.0f));
			aabb.Expand(Vector3f(-m_FloodFillOptions.m_GridRadius, -m_FloodFillOptions.m_GridRadius, 0.0f));
			aabb.m_Maxs[2] = m_FloodFillOptions.m_CharacterHeight - m_FloodFillOptions.m_CharacterStepHeight;
			aabb.Translate(*cIt);
			Utils::OutlineAABB(aabb, COLOR::BLACK, 0.1f);
		}

		_RenderFloodFill();
		_RenderSectors();
	}
}

void PathPlannerFloodFill::_RenderSectors()
{
	Prof(RenderSectors);

	GameEntity ge = Utils::GetLocalEntity();
	if(!ge.IsValid())
		return;

	// Old Floodfill stuff
	static int iNextTimeUpdate = 0;
	if(m_FloodFillData && IGame::GetTime() >= iNextTimeUpdate)
	{
		Vector3f vPosition, vFacing;
		g_EngineFuncs->GetEntityPosition(ge, vPosition);
		g_EngineFuncs->GetEntityOrientation(ge, vFacing, 0, 0);

		int iSectorNum = 0;
		SectorList::const_iterator sIt = m_Sectors.begin();
		for(; sIt != m_Sectors.end(); ++sIt)
		{
			const Sector &sec = (*sIt);

			Vector3f vSectorCenter;
			sec.m_SectorBounds.CenterBottom(vSectorCenter);

			if(Utils::InFieldOfView(vFacing, vSectorCenter-vPosition, 120.0f))
			{
				Utils::PrintText(vSectorCenter,COLOR::BLUE,2.f,"%d", iSectorNum);

				Utils::OutlineAABB(sec.m_SectorBounds, COLOR::GREY, 2.f, AABB::DIR_BOTTOM);

				// draw connections
				if(m_PlannerFlags.CheckFlag(NAV_VIEWCONNECTIONS))
				{
					SectorLinks::const_iterator linkIt = sec.m_SectorLinks.begin(),
						linkItEnd = sec.m_SectorLinks.end();
					for(; linkIt != linkItEnd; ++linkIt)
					{
						obColor linkColor = COLOR::MAGENTA;
						// todo: base color on link type
						Utils::DrawLine(linkIt->m_From, linkIt->m_To, linkColor, 2.f);
					}
				}
			}
			iSectorNum++;
		}

		iNextTimeUpdate = IGame::GetTime() + 2000;
	}

	//////////////////////////////////////////////////////////////////////////
	// New Sector Drawing
	/*if(!m_WorkingSector.empty())
		Utils::DrawPolygon(m_WorkingSector, COLOR::GREEN.fade(100), 0.1f, false);*/
}

void PathPlannerFloodFill::_RenderFloodFill()
{
	Prof(RenderFloodFill);
return;
	GameEntity ge = Utils::GetLocalEntity();
	if(!ge.IsValid())
		return;

	static int iNextTimeUpdate = 0;
	if(m_FloodFillData && IGame::GetTime() >= iNextTimeUpdate)
	{
#if(0)
		Vector3f vPosition, vFacing;
		g_EngineFuncs->GetClientPosition(ge, vPosition);
		g_EngineFuncs->GetClientOrientation(ge, vFacing, 0, 0);

		for(obuint32 f = 0; f < m_FloodFillData->m_FaceIndex; ++f)
		{
			const FloodFillData::Quad &qd = m_FloodFillData->m_Faces[f];
			Vector3f v1 = m_FloodFillData->m_Vertices[qd.m_Verts[0]];
			Vector3f v2 = m_FloodFillData->m_Vertices[qd.m_Verts[1]];
			Vector3f v3 = m_FloodFillData->m_Vertices[qd.m_Verts[2]];
			Vector3f v4 = m_FloodFillData->m_Vertices[qd.m_Verts[3]];

			Vector3f vMid = (v1 + v2 + v3 + v4) / 4.0f;

			// Limit view distance and fov
			static float fRenderDistance = 1024.0f;	
			Vector3f vToNode = (vMid - vPosition);
			float fDist = vToNode.Normalize();
			if(fDist < fRenderDistance && Client::InFieldOfView(vFacing, vToNode, Mathf::DegToRad(120.0f)))
			{
				obColor col = GetFaceColor(qd);
				Utils::DrawLine(v1, v2, col, 2.f);
				Utils::DrawLine(v2, v3, col, 2.f);
				Utils::DrawLine(v3, v4, col, 2.f);
				Utils::DrawLine(v4, v1, col, 2.f);
				//Utils::DrawLine(v3, v1, COLOR::GREY, 2.0f);

				/*Utils::DrawLine(vMid, 
				vMid + m_FloodFillData->m_Faces[f].m_Normal * 32.0f, COLOR::ORANGE, 2.0f);*/
			}
		}
#endif
		//////////////////////////////////////////////////////////////////////////
		Vector3f vPosition, vFacing;
		g_EngineFuncs->GetEntityPosition(ge, vPosition);
		g_EngineFuncs->GetEntityOrientation(ge, vFacing, 0, 0);

		Vector3f vAimPt;
		Utils::GetLocalAimPoint(vAimPt);

		std::set<int> drawnLinks;

		obint32 ClosestNodeIndex = -1;
		float ClosestNodeDist = 1000000.f;

		for(obint32 n = 0; n < (obint32)m_FloodFillData->m_NodeIndex; ++n)
		{
			const Vector3f nodePos = m_FloodFillData->m_Nodes[n].m_Position;
			//////////////////////////////////////////////////////////////////////////
			const float DistToNode = Length(vAimPt, nodePos);
			if(ClosestNodeIndex==-1 || DistToNode<ClosestNodeDist)
			{
				ClosestNodeIndex = n;
				ClosestNodeDist = DistToNode;
			}
			//////////////////////////////////////////////////////////////////////////
			// Translate to world aabb
			static float fRenderDistance = 1024.0f;		

			if((vPosition - nodePos).SquaredLength() < (fRenderDistance*fRenderDistance))
			{
				// limit to field of view.
				Vector3f vToNode = (m_FloodFillData->m_Nodes[n].m_Position - vPosition);
				vToNode.Normalize();
				if(Utils::InFieldOfView(vFacing, vToNode, 90.0f))
				{
					AABB worldAABB = m_FloodFillData->m_GridAABB.TranslateCopy(m_FloodFillData->m_Nodes[n].m_Position);

					if(!m_FloodFillData->m_Nodes[n].m_ValidPos)
						continue;

					if(m_FloodFillData->m_Nodes[n].m_Sectorized)
						continue;

					obColor col = COLOR::GREY;

					/*if(m_FloodFillData->m_Nodes[n].m_Ladder)
						col = COLOR::ORANGE;
					elseif(m_FloodFillData->m_Nodes[n].m_Movable)
						 col = COLOR::BROWN;
					else*/ if(m_FloodFillData->m_Nodes[n].m_Open)
						col = COLOR::GREEN;
					else if(m_FloodFillData->m_Nodes[n].m_InWater)
						col = COLOR::BLUE;
					/*else if(m_FloodFillData->m_Nodes[n].m_Jump)
						col = COLOR::MAGENTA;*/
					else if(m_FloodFillData->m_Nodes[n].m_NearSolid)
						col = COLOR::RED;
					else if(m_FloodFillData->m_Nodes[n].m_NearVoid)
						col = COLOR::GREEN;

					int iNumBorderNeighbors = 0;
					for(int dir = 0; dir < NUM_DIRS; ++dir)
					{
						if(m_FloodFillData->m_Nodes[n].m_Connections[dir].Index != -1)
						{
							const NavNode &neighbor = 
								m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[n].m_Connections[dir].Index];
							if(neighbor.m_NearSolid)
								iNumBorderNeighbors++;
						}
					}
					if(iNumBorderNeighbors>2)
						col = COLOR::BLACK;

					if(m_FloodFillData->m_Nodes[n].m_Region>0)
					{
						Utils::PrintText(
							m_FloodFillData->m_Nodes[n].m_Position, 
							COLOR::WHITE, 
							2.f,
							"%d", 
							m_FloodFillData->m_Nodes[n].m_Region);
					}

					if(m_FloodFillData->m_Nodes[n].m_DistanceFromEdge != 0 &&
						!m_FloodFillData->m_Nodes[n].m_Sectorized)
					{
						Utils::PrintText(
							m_FloodFillData->m_Nodes[n].m_Position, 
							COLOR::BLACK, 
							2.f,
							"%d", 
							m_FloodFillData->m_Nodes[n].m_DistanceFromEdge);
					}

					//Utils::OutlineAABB(worldAABB, col, 2.0f, AABB::DIR_BOTTOM);

					//// Render connections
					//if(m_PlannerFlags.CheckFlag(NAVMESH_CONNECTIONVIEW))
					//{
					//	for(int dir = 0; dir < NUM_DIRS; ++dir)
					//	{
					//		if(m_FloodFillData->m_Nodes[n].m_Connections[dir] != -1)
					//		{
					//			Utils::DrawLine(
					//				m_FloodFillData->m_Nodes[n].m_Position + Vector3f(0.f, 0.f, 5.f),
					//				m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[n].m_Connections[dir]].m_Position,
					//				COLOR::CYAN, 2.f);
					//		}
					//	}
					//}

					for(int dir = 0; dir < NUM_DIRS; ++dir)
					{
						const NavNode &nn = m_FloodFillData->m_Nodes[n];
						const NavNode::Connection &conn = nn.m_Connections[dir];
						const int iConnectedNode = conn.Index;
						if(iConnectedNode != -1)
						{
							if(conn.Jump)
								col = COLOR::MAGENTA;

							if(drawnLinks.find(Utils::MakeId32((obuint16)n, (obuint16)iConnectedNode)) == drawnLinks.end())
							{
								Utils::DrawLine(
									m_FloodFillData->m_Nodes[n].m_Position,
									m_FloodFillData->m_Nodes[iConnectedNode].m_Position,
									col, 2.f);

								drawnLinks.insert(Utils::MakeId32((obuint16)iConnectedNode, (obuint16)n));
							}
						}
					}
				}
			}
		}

		if(ClosestNodeIndex!=-1)
		{
			Vector3f vNudge(0,0,10);
			const NavNode &nn = m_FloodFillData->m_Nodes[ClosestNodeIndex];
			Utils::DrawLine(
				nn.m_Position+vNudge,
				nn.m_Position+vNudge + Vector3f(0,0,32.f),
				COLOR::BLUE, 2.f);
			for(int dir = 0; dir < NUM_DIRS; ++dir)
			{				
				if(nn.m_Connections[dir].Index != -1)
				{
					const NavNode::Connection &con = nn.m_Connections[dir];
					const NavNode &cn = m_FloodFillData->m_Nodes[con.Index];

					obColor col = COLOR::CYAN;
					if(con.Jump)
						col = COLOR::MAGENTA;
					if(con.Ladder)
						col = COLOR::ORANGE;

					Utils::DrawLine(
						nn.m_Position+vNudge,
						cn.m_Position+vNudge,
						col, 2.f);
				}
			}
		}

		iNextTimeUpdate = IGame::GetTime() + 2000;
	}
}

void PathPlannerFloodFill::Shutdown()
{
	Unload();
}

bool PathPlannerFloodFill::Load(const String &_mapname, bool _dl)
{
	if(_mapname.empty())
		return false;

	gmMachine *pM = new gmMachine;
	pM->SetDebugMode(true);
	DisableGCInScope gcEn(pM);

	String waypointName		= _mapname + ".nav";

	File InFile;

	char strbuffer[1024] = {};
	sprintf(strbuffer, "user/%s", waypointName.c_str());
	InFile.OpenForRead(strbuffer, File::Binary);
	if(InFile.IsOpen())
	{
		obuint32 fileSize = (obuint32)InFile.FileLength();
#if __cplusplus >= 201103L //karin: using std::shared_ptr<T[]> instead of boost::shared_array<T>
		compat::shared_array<char> pBuffer(new char[fileSize+1]);
#else
		boost::shared_array<char> pBuffer(new char[fileSize+1]);
#endif

		InFile.Read(pBuffer.get(), fileSize);
		pBuffer[fileSize] = 0;
		InFile.Close();

		int errors = pM->ExecuteString(pBuffer.get());
		if(errors)
		{
			ScriptManager::LogAnyMachineErrorMessages(pM);
			delete pM;
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool bSuccess = true;
	//////////////////////////////////////////////////////////////////////////
	//NavSectorList loadSectors;
	//gmTableObject *pNavTbl = pM->GetGlobals()->Get(pM, "Navigation").GetTableObjectSafe();
	//if(pNavTbl)
	//{
	//	gmVariable vCenter = pNavTbl->Get(pM,"MapCenter");
	//	if(vCenter.IsVector())
	//		vCenter.GetVector(m_MapCenter);
	//	else
	//		m_MapCenter = Vector3f::ZERO;

	//	gmTableObject *pSectorsTable = pNavTbl->Get(pM, "Sectors").GetTableObjectSafe();
	//	if(pSectorsTable && bSuccess)
	//	{
	//		gmTableIterator sectorIt;
	//		for(gmTableNode *pSectorNode = pSectorsTable->GetFirst(sectorIt);
	//			pSectorNode && bSuccess;
	//			pSectorNode = pSectorsTable->GetNext(sectorIt))
	//		{
	//			gmTableObject *pSector = pSectorNode->m_value.GetTableObjectSafe();
	//			if(pSector)
	//			{
	//				NavSector ns;

	//				//const char *pSectorName = pSector->Get(pM, "Name").GetCStringSafe();

	//				gmVariable vMirror = pSector->Get(pM, "Mirror");
	//				ns.m_Mirror = vMirror.IsInt() ? vMirror.GetInt() : NavSector::MirrorNone;

	//				gmTableObject *pVertTable = pSector->Get(pM, "Vertices").GetTableObjectSafe();
	//				if(pVertTable && bSuccess)
	//				{
	//					gmTableIterator vertIt;
	//					for(gmTableNode *pVertNode = pVertTable->GetFirst(vertIt);
	//						pVertNode && bSuccess;
	//						pVertNode = pVertTable->GetNext(vertIt))
	//					{
	//						Vector3f v;
	//						if(pVertNode->m_value.IsVector() && pVertNode->m_value.GetVector(v.x, v.y, v.z))
	//							ns.m_Boundary.push_back(v);
	//						else
	//							bSuccess = false;
	//					}
	//				}

	//				//////////////////////////////////////////////////////////////////////////
	//				if(bSuccess)
	//					loadSectors.push_back(ns);
	//				//////////////////////////////////////////////////////////////////////////
	//			}
	//		}
	//	}
	//}

	//if(bSuccess)
	//{
	//	m_NavSectors.swap(loadSectors);
	//}
	//////////////////////////////////////////////////////////////////////////

	delete pM;

	return bSuccess;
}

bool PathPlannerFloodFill::Save(const String &_mapname)
{
	if(_mapname.empty())
		return false;

	String waypointName		= _mapname + ".nav";
	//String navPath	= String("nav/") + waypointName;

	gmMachine *pM = new gmMachine;
	pM->SetDebugMode(true);
	DisableGCInScope gcEn(pM);

	gmTableObject *pNavTbl = pM->AllocTableObject();
	pM->GetGlobals()->Set(pM, "Navigation", gmVariable(pNavTbl));

	pNavTbl->Set(pM,"MapCenter",gmVariable(Vector3f::ZERO));

	gmTableObject *pSectorsTable = pM->AllocTableObject();
	pNavTbl->Set(pM, "Sectors", gmVariable(pSectorsTable));

	Vector3List vlist;
	vlist.reserve(4);

	for(obuint32 s = 0; s < m_Sectors.size(); ++s)
	{
		const Sector &ns = m_Sectors[s];
		vlist.resize(0);

		gmTableObject *pSector = pM->AllocTableObject();
		pSectorsTable->Set(pM, s, gmVariable(pSector));

		// NavSector properties
		pSector->Set(pM, "Mirror", gmVariable((obint32)0));

		gmTableObject *pSectorVerts = pM->AllocTableObject();
		pSector->Set(pM, "Vertices", gmVariable(pSectorVerts));

		Vector3List vlist;
		Utils::GetAABBBoundary(ns.m_SectorBounds,vlist);

		for(obuint32 v = 0; v < vlist.size(); ++v)
		{
			pSectorVerts->Set(pM,v,gmVariable(vlist[v]));
		}		
	}

	gmUtility::DumpTable(pM,waypointName.c_str(),"Navigation",gmUtility::DUMP_ALL);
	delete pM;

	return true;
}

bool PathPlannerFloodFill::IsReady() const
{
	return true;//!m_ActiveNavSectors.empty();
}

bool PathPlannerFloodFill::GetNavFlagByName(const String &_flagname, NavFlags &_flag) const
{	
	_flag = 0;
	return false;
}

Vector3f PathPlannerFloodFill::GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team)
{
	Vector3f dest = _start;
	
	/*if(!m_ActiveNavSectors.empty())
	{
		const NavSector &randSector = m_ActiveNavSectors[rand()%m_ActiveNavSectors.size()];
		dest = Utils::AveragePoint(randSector.m_Boundary);
	}*/
	return dest;
}

//////////////////////////////////////////////////////////////////////////

int PathPlannerFloodFill::PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team)
{
	DestinationVector dst;
	for(obuint32 i = 0; i < _goals.size(); ++i)
		dst.push_back(Destination(_goals[i],32.f));
	return PlanPathToNearest(_client,_start,dst,_team);
}

int PathPlannerFloodFill::PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team)
{
	g_PathFinder.StartNewSearch();
	g_PathFinder.AddStart(_start);

	for(obuint32 i = 0; i < _goals.size(); ++i)
		g_PathFinder.AddGoal(_goals[i].m_Position);

	while(!g_PathFinder.IsFinished())
		g_PathFinder.Iterate();

	return g_PathFinder.GetGoalIndex();
}

void PathPlannerFloodFill::PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team)
{
	DestinationVector dst;
	dst.push_back(Destination(_goal,32.f));
	PlanPathToNearest(_client,_start,dst,_team);
}

bool PathPlannerFloodFill::IsDone() const
{
	return g_PathFinder.IsFinished();
}
bool PathPlannerFloodFill::FoundGoal() const
{
	return g_PathFinder.FoundGoal();
}

void PathPlannerFloodFill::Unload()
{	
}

void PathPlannerFloodFill::RegisterGameGoals()
{
}

void PathPlannerFloodFill::GetPath(Path &_path, int _smoothiterations)
{
	const float CHAR_HALF_HEIGHT = NavigationMeshOptions::CharacterHeight * 0.75f;

	PathFindFloodFill::NodeList &nl = g_PathFinder.GetSolution();

	//////////////////////////////////////////////////////////////////////////
	//if(nl.size() > 2)
	//{
	//	for(int i = 0; i < _smoothiterations; ++i)
	//	{
	//		bool bSmoothed = false;

	//		// solution is goal to start
	//		for(obuint32 n = 1; n < nl.size()-1; ++n)
	//		{
	//			PathFind::PlanNode *pFrom = nl[n+1];
	//			PathFind::PlanNode *pTo = nl[n-1];
	//			PathFind::PlanNode *pMid = nl[n];
	//			if(!pMid->Portal /*|| pMid->Portal->m_LinkFlags & teleporter*/)
	//				continue;

	//			Segment3f portalSeg = pMid->Portal->m_Segment;
	//			portalSeg.Extent -= 32.f;
	//			Segment3f seg = Utils::MakeSegment(pFrom->Position,pTo->Position);
	//			//DistancePointToLine(_seg1.Origin,_seg2.GetNegEnd(),_seg2.GetPosEnd(),&cp);

	//			Vector3f intr;
	//			if(Utils::intersect2D_Segments(seg,portalSeg,&intr))
	//			{
	//				// adjust the node position
	//				if(SquaredLength(intr,pMid->Position) > Mathf::Sqr(16.f))
	//				{
	//					//Utils::DrawLine(pMid->Position+Vector3f(0,0,32),intr,COLOR::YELLOW,15.f);
	//					bSmoothed = true;
	//					pMid->Position = intr;
	//				}					
	//			}
	//		}

	//		if(!bSmoothed)
	//			break;
	//	}
	//}
	//////////////////////////////////////////////////////////////////////////

	while(!nl.empty())
	{
		Vector3f vNodePos = nl.back()->Position;

		_path.AddPt(vNodePos + Vector3f(0,0,CHAR_HALF_HEIGHT),32.f)
			/*.Flags(m_Solution.back()->GetNavigationFlags())
			.OnPathThrough(m_Solution.back()->OnPathThrough())
			.OnPathThroughParam(m_Solution.back()->OnPathThroughParam())*/;

		nl.pop_back();
	}
}

Vector3f PathPlannerFloodFill::_GetNearestGridPt(const Vector3f &_pt)
{
	float fGridSize = m_FloodFillOptions.m_GridRadius * 2.f;

	Vector3f vOut = _pt;
	vOut.x += fGridSize + 0.5f;
	vOut.y += fGridSize + 0.5f;

	vOut.x = fGridSize * Mathf::Floor(vOut.x / fGridSize);
	vOut.y = fGridSize * Mathf::Floor(vOut.y / fGridSize);

	//vOut.z Leave height the same.
	return vOut;
}

//////////////////////////////////////////////////////////////////////////

obColor PathPlannerFloodFill::GetFaceColor(const FloodFillData::Quad &_qd)
{
	obColor col = COLOR::LIGHT_GREY;

	if(_qd.m_Properties.m_Ladder)
		col = COLOR::ORANGE;
	else if(_qd.m_Properties.m_Movable)
		col = COLOR::YELLOW;
	else if(_qd.m_Properties.m_InWater)
		col = COLOR::BLUE;
	else if(_qd.m_Properties.m_Jump)
		col = COLOR::MAGENTA;
	else if(_qd.m_Properties.m_Teleporter)
		col = COLOR::BLACK;
	else if(_qd.m_Properties.m_NearVoid)
		col = COLOR::CYAN;
	else if(_qd.m_Properties.m_NearSolid)
		col = COLOR::GREY;
	return col;
}

//Vector3f CalculateFaceNormal(const Vector3f &_v1, const Vector3f &_v2, const Vector3f &_v3)
//{
//	Vector3f vSide1 = _v2-_v1;
//	vSide1.Normalize();
//
//	Vector3f vSide2 = _v3-_v2;
//	vSide2.Normalize();
//
//	return vSide2.Cross(vSide1);
//}

void PathPlannerFloodFill::AddFaceToNavMesh(const Vector3f &_pos, const NavNode &_navnode)
{
	AABB nodeaabb = m_FloodFillData->m_GridAABB;
	nodeaabb.Translate(_pos);

	Vector3f vCorner[4];
	int	iCornerVertexIndices[4] = { -1,-1,-1,-1 };
	nodeaabb.GetBottomCorners(vCorner[0], vCorner[1], vCorner[2], vCorner[3]);

	//////////////////////////////////////////////////////////////////////////
	// Calculate normals
	const Vector3f vTraceOffset = Vector3f(0.0f, 0.0f, m_FloodFillOptions.m_CharacterStepHeight * 1.5f);
	Vector3f vCornerNormals[4];
	obTraceResult tr;

	for(int i = 0; i < 4; ++i)
	{
		EngineFuncs::TraceLine(tr, 
			vCorner[i] + Vector3f(0.0f, 0.0f, 10.0f), 
			vCorner[i] - vTraceOffset, 
			0, TR_MASK_FLOODFILL, 0, False);

		/*if(tr.m_Fraction == 1.0f)
		vCornerContacts[i] = Vector3f::UNIT_Z;
		else*/
		vCornerNormals[i] = tr.m_Normal;
	}
	//////////////////////////////////////////////////////////////////////////

	float fDist = 0.0f;
	QuadTree::QuadTreeData data;
	for(int i = 0; i < 4; ++i)
	{
		if(m_FloodFillData->m_TriMeshQuadTree->ClosestPtSq(vCorner[i], data, fDist))
		{
			// If the points match up from a top down point, consider them the same
			// if the vertical difference is within stepheight.
			Vector3f vDiff = data.m_Point - vCorner[i];
			float fzDiff = Mathf::FAbs(vDiff.z);
			vDiff.z = 0.0f;

			if(vDiff.SquaredLength() < Mathf::EPSILON && fzDiff <= m_FloodFillOptions.m_CharacterStepHeight)
			{
				iCornerVertexIndices[i] = data.m_UserData;
			}
		}
	}

	// Any verts that didnt match an index, add it to the vertex list
	for(int i = 0; i < 4; ++i)
	{
		if(iCornerVertexIndices[i] == -1)
		{
			m_FloodFillData->m_TriMeshQuadTree->AddPoint(vCorner[i], m_FloodFillData->m_VertIndex);

			m_FloodFillData->m_Vertices[m_FloodFillData->m_VertIndex] = vCorner[i];
			iCornerVertexIndices[i] = m_FloodFillData->m_VertIndex;
			++m_FloodFillData->m_VertIndex;
		}
	}

	// Now add both faces.
	Vector3f vNormal1 = /*CalculateFaceNormal*/(vCornerNormals[0]+vCornerNormals[1]+vCornerNormals[2]+vCornerNormals[3]);
	vNormal1.Normalize();

	FloodFillData::Quad face1;
	face1.m_Verts[0] = iCornerVertexIndices[0]; 
	face1.m_Verts[1] = iCornerVertexIndices[1]; 
	face1.m_Verts[2] = iCornerVertexIndices[2];
	face1.m_Verts[3] = iCornerVertexIndices[3];
	face1.m_Normal = vNormal1;
	//face1.m_Merged = false;
	//face1.m_Flat = vNormal1.Dot(Vector3f::UNIT_Z) > 0.98f ? true : false;

	face1.m_Properties.m_InWater = _navnode.m_InWater;
	/*face1.m_Properties.m_Jump = _navnode.m_Jump;
	face1.m_Properties.m_Ladder = _navnode.m_Ladder;
	face1.m_Properties.m_Teleporter = _navnode.m_Teleporter;
	face1.m_Properties.m_Movable = _navnode.m_Movable;*/
	face1.m_Properties.m_NearSolid = _navnode.m_NearSolid;
	face1.m_Properties.m_NearVoid = _navnode.m_NearVoid;

	m_FloodFillData->m_Faces[m_FloodFillData->m_FaceIndex++] = face1;

	//Vector3f vNormal2 = /*CalculateFaceNormal*/(vCornerNormals[2]+vCornerNormals[3]+vCornerNormals[0]);
	//vNormal2.Normalize();

	//FloodFillData::Face face2;
	//face2.m_Verts[0] = iCornerVertexIndices[2]; 
	//face2.m_Verts[1] = iCornerVertexIndices[3]; 
	//face2.m_Verts[2] = iCornerVertexIndices[0];
	//face2.m_Normal = vNormal2;
	//face2.m_Merged = false;
	//face2.m_Flat = vNormal2.Dot(Vector3f::UNIT_Z) > 0.98f ? true : false;

	//m_FloodFillData->m_Faces[m_FloodFillData->m_FaceIndex++] = face2;
}

int PathPlannerFloodFill::Process_FloodFill()
{
	Prof(Process_FloodFill);

	enum FloodFillStatus
	{
		Process_PrepareData,
		Process_FloodFill,
		Process_FloodBorder,
		Process_MergeSectors,
		Process_MakeRegions,
		Process_ConnectSectors,
		Process_Cleanup
	};

	//////////////////////////////////////////////////////////////////////////
	static FloodFillStatus status = Process_PrepareData;
	if(!m_FloodFillData)
		status = Process_PrepareData;

	static obint32 ConnectSector = 0;
	static DynBitSet32 HandledCells;

	switch(status)
	{
	case Process_PrepareData:
		{
			_InitFloodFillData();
			for(obuint32 i = 0; i < m_StartPositions.size(); ++i)
				_AddNode(m_StartPositions[i]);
			status = Process_FloodFill;

			ConnectSector = 0;

			EngineFuncs::ConsoleMessage("Initializing Flood Fill.");
			break;
		}
	case Process_FloodFill:
		{
			Prof(FloodFill);

			Timer tme;
			for(;;)
			{
				//////////////////////////////////////////////////////////////////////////
				NavNode *pNavNode = _GetNextOpenNode();
				if(!pNavNode)
				{
					if(!m_PlannerFlags.CheckFlag(NAVMESH_STEPPROCESS) ||
						m_PlannerFlags.CheckFlag(NAVMESH_TAKESTEP))
					{
						m_PlannerFlags.ClearFlag(NAVMESH_TAKESTEP);
						EngineFuncs::ConsoleMessage("Tagging node border distance.");
						status = Process_FloodBorder;
						//status = Process_Recast;
					}
					break;
				}
				//////////////////////////////////////////////////////////////////////////
				// Overflow protection
				if(m_FloodFillData->m_FaceIndex >= NUM_FLOOD_NODES)
				{
					EngineFuncs::ConsoleError("AutoNav: Out of Nodes but still need to expand.");
					status = Process_Cleanup;
					break;
				}
				// Close this node so it won't be explored again.
				pNavNode->m_Open = false;

				_ExpandNode(pNavNode);

				// Time splice.
				if(tme.GetElapsedSeconds() > 0.01)
					break;
			}

			m_FloodFillData->m_Stats.m_TotalTime += tme.GetElapsedSeconds();
			break;
		}	
	case Process_FloodBorder:
		{
			Prof(FloodBorder);

			Timer tme;
			for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
			{
				if(m_FloodFillData->m_Nodes[i].m_NearSolid || 
					m_FloodFillData->m_Nodes[i].m_NearVoid)
				{
					m_FloodFillData->m_Nodes[i].m_DistanceFromEdge = 1;
				}
			}

			int iCurrentNum = 1;
			bool bDidSomething = true;
			while(bDidSomething)
			{
				bDidSomething = false;
				for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
				{
					if(m_FloodFillData->m_Nodes[i].m_DistanceFromEdge == iCurrentNum)
					{
						for(int side = 0; side < NUM_DIRS; ++side)
						{
							if(m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[i].m_Connections[side].Index].m_DistanceFromEdge == 0)
							{
								m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[i].m_Connections[side].Index].m_DistanceFromEdge = iCurrentNum+1;
								bDidSomething = true;
							}
						}
					}
				}
				++iCurrentNum;
			}

			m_FloodFillData->m_Stats.m_TotalTime += tme.GetElapsedSeconds();

			if(!m_PlannerFlags.CheckFlag(NAVMESH_STEPPROCESS) ||
				m_PlannerFlags.CheckFlag(NAVMESH_TAKESTEP))
			{
				m_PlannerFlags.ClearFlag(NAVMESH_TAKESTEP);
				EngineFuncs::ConsoleMessage("Merging Sectors.");
				//status = Process_MergeSectors;
				status = Process_MakeRegions;
				m_Sectors.clear();
			}

			//status = Process_MergeSectors;
			status = Process_MakeRegions;
			HandledCells.resize(m_FloodFillData->m_NodeIndex);
			m_Sectors.clear();

			break;
		}
	case Process_MakeRegions:
		{
			Prof(MakeRegions);

			static int Cycles = 1;

			if(Cycles == 0)
				break;

			Cycles--;

			int LargestDistance = 0;
			for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
			{
				m_FloodFillData->m_Nodes[i].m_Region = 0;
				if(m_FloodFillData->m_Nodes[i].m_DistanceFromEdge > LargestDistance)
				{
					LargestDistance = m_FloodFillData->m_Nodes[i].m_DistanceFromEdge;
				}
			}

			// Mark all the regions to start
			for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
			{
				if(m_FloodFillData->m_Nodes[i].m_DistanceFromEdge == LargestDistance)
				{
					m_FloodFillData->m_Nodes[i].m_Region = -1;
				}
			}

			// isolate the region ids
			std::list<obint32> il;

			int CurrentRegion = 1;
			for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
			{
				if(m_FloodFillData->m_Nodes[i].m_Region == -1)
				{
					m_FloodFillData->m_Nodes[i].m_Region = CurrentRegion;

					il.push_back(i);
					while(!il.empty())
					{
						const int CurrentIndex = il.front();
						il.pop_front();

						for(int side = 0; side < NUM_DIRS; ++side)
						{
							const int neighbor = m_FloodFillData->m_Nodes[CurrentIndex].m_Connections[side].Index;
							if(m_FloodFillData->m_Nodes[neighbor].m_Region==-1)
							{
								m_FloodFillData->m_Nodes[neighbor].m_Region = CurrentRegion;
								il.push_back(neighbor);
							}							
						}
					}

					CurrentRegion++;
				}
			}

			break;
		}
	case Process_MergeSectors:
		{
			Prof(MergeSectors);

			Timer tme;

			//////////////////////////////////////////////////////////////////////////
			for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
			{
				if(m_FloodFillData->m_Nodes[i].m_NearSolid || 
					m_FloodFillData->m_Nodes[i].m_NearVoid ||
					HandledCells.test(i))
				{
					m_FloodFillData->m_Nodes[i].m_DistanceFromEdge = 1;
				}
			}

			int iCurrentNum = 1;
			bool bDidSomething = true;
			while(bDidSomething)
			{
				bDidSomething = false;
				for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
				{
					if(m_FloodFillData->m_Nodes[i].m_DistanceFromEdge == iCurrentNum)
					{
						for(int side = 0; side < NUM_DIRS; ++side)
						{
							if(m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[i].m_Connections[side].Index].m_DistanceFromEdge == 0)
							{
								m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[i].m_Connections[side].Index].m_DistanceFromEdge = iCurrentNum+1;
								bDidSomething = true;
							}
						}
					}
				}
				++iCurrentNum;
			}
			//////////////////////////////////////////////////////////////////////////
			
			for(;;)
			{
				int iNodeDepth = 0;
				int iDeepestNode = -1;
				for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
				{
					if((m_FloodFillData->m_Nodes[i].m_DistanceFromEdge > iNodeDepth) && 
						!m_FloodFillData->m_Nodes[i].m_Sectorized)
					{
						iDeepestNode = i;
						iNodeDepth = m_FloodFillData->m_Nodes[i].m_DistanceFromEdge;
					}
				}

				// Out of nodes!
				if(iDeepestNode == -1)
				{
					EngineFuncs::ConsoleMessage(va("%d Sectors Merged.", m_Sectors.size()));
					status = Process_ConnectSectors;
					EngineFuncs::ConsoleMessage("Connecting Sectors.");
					break;
				}

				m_FloodFillData->m_Nodes[iDeepestNode].m_Sectorized = true;
				_BuildSector(iDeepestNode);

				//////////////////////////////////////////////////////////////////////////
				// mark all cells
				CellSet::iterator cIt = m_Sectors.back().m_ContainingCells.begin();
				for(; cIt != m_Sectors.back().m_ContainingCells.end(); ++cIt)
				{
					HandledCells.set(*cIt,true);
				}
				//////////////////////////////////////////////////////////////////////////

				// Time splice.
				if(tme.GetElapsedSeconds() > 0.01)
					break;
			}
			m_FloodFillData->m_Stats.m_TotalTime += tme.GetElapsedSeconds();
			break;
		}
	case Process_ConnectSectors:
		{
			Prof(ConnectSectors);

			Timer tme;
			while(ConnectSector < (int)m_Sectors.size())
			{
				Sector &currentSector = m_Sectors[ConnectSector];

				CellSet::iterator cIt = currentSector.m_ContainingCells.begin(), 
					cItEnd = currentSector.m_ContainingCells.end();
				for(; cIt != cItEnd; ++cIt)
				{
					const int iSectorCell = (*cIt);

					// Look for all connections that are part of a different sector
					for(int conn = 0; conn < NUM_DIRS; ++conn)
					{
						const int iConnectedCell = m_FloodFillData->m_Nodes[iSectorCell].m_Connections[conn].Index;

						if(iConnectedCell != -1)
						{
							const int iConnectedSector = m_FloodFillData->m_Nodes[iConnectedCell].m_Sector;
							if(iConnectedSector != ConnectSector)
							{
								// found a connection to another sector.
								SectorLink lnk;
								lnk.m_Sector = iConnectedSector;
								lnk.m_From = m_FloodFillData->m_Nodes[iSectorCell].m_Position;
								lnk.m_To = m_FloodFillData->m_Nodes[iConnectedCell].m_Position;
								currentSector.m_SectorLinks.push_back(lnk);
							}
						}
					}
				}

				// next
				++ConnectSector;
				if(tme.GetElapsedSeconds() > 0.01)
					return Function_InProgress;
			}

			obuint64 iSize = m_Sectors.size() * sizeof(NavigationMeshFF::NavSector);
			for(obuint32 s = 0; s < m_Sectors.size(); ++s)
				iSize += m_Sectors[s].m_SectorLinks.size() * sizeof(NavigationMeshFF::NavLink);

			EngineFuncs::ConsoleMessage(va("Approx size of stored Navigation Mesh: %s", 
				Utils::FormatByteString(iSize).c_str()));

			status = Process_Cleanup;
			break;
		}
	case Process_Cleanup:
		{			
			// Print status
			int iNumGood = 0, iNumBad = 0;
			for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
			{
				if(m_FloodFillData->m_Nodes[i].m_ValidPos)
					iNumGood++;
				else
					iNumBad++;
			}

			EngineFuncs::ConsoleMessage(va("%d good nodes, %d bad nodes, in %f seconds", 
				iNumGood, 
				iNumBad, 
				m_FloodFillData->m_Stats.m_TotalTime));

			// Release the node data.
			//m_FloodFillData.reset();

			//////////////////////////////////////////////////////////////////////////

			String strMap = g_EngineFuncs->GetMapName();
			strMap += ".off";
			fs::path filepath = Utils::GetNavFolder() / strMap;

			std::fstream fl;
			fl.open(filepath.string().c_str(), std::ios_base::out);
			if(fl.is_open())
			{
				fl << "OFF" << std::endl;

				fl << m_FloodFillData->m_VertIndex << " " << 
					m_FloodFillData->m_FaceIndex << " " << 
					m_FloodFillData->m_FaceIndex * 4 << std::endl << std::endl;

				for(obuint32 i = 0; i < m_FloodFillData->m_VertIndex; ++i)
				{
					fl << m_FloodFillData->m_Vertices[i].x << " " <<
						m_FloodFillData->m_Vertices[i].y << " " <<
						m_FloodFillData->m_Vertices[i].z << std::endl;
				}

				for(obuint32 i = 0; i < m_FloodFillData->m_FaceIndex; ++i)
				{
					obColor col = GetFaceColor(m_FloodFillData->m_Faces[i]);

					fl << "4 " <<
						m_FloodFillData->m_Faces[i].m_Verts[0] << " " <<
						m_FloodFillData->m_Faces[i].m_Verts[1] << " " <<
						m_FloodFillData->m_Faces[i].m_Verts[2] << " " <<
						m_FloodFillData->m_Faces[i].m_Verts[3] << std::endl;
					/*" 3 " <<
					col.rF() << " " <<
					col.gF() << " " <<
					col.bF() << " " << std::endl;*/
				}
				fl.close();

				return Function_Finished;
			}

			//////////////////////////////////////////////////////////////////////////

			return Function_Finished;
		}
	}

	return Function_InProgress;
}

void PathPlannerFloodFill::AddFloodStart(const Vector3f &_vec)
{
	m_StartPositions.push_back(_GetNearestGridPt(_vec));
	EngineFuncs::ConsoleMessage("Added Flood Fill Start");
}

void PathPlannerFloodFill::ClearFloodStarts()
{
	EngineFuncs::ConsoleMessage(va("Clearing %d flood start nodes.", m_StartPositions.size()));
	m_StartPositions.clear();
}

void PathPlannerFloodFill::SaveFloodStarts()
{
	String strMap = g_EngineFuncs->GetMapName();
	strMap += ".navstarts";

	char strBuffer[1024] = {};
	sprintf(strBuffer, "nav/%s", strMap.c_str());

	File f;
	f.OpenForWrite(strBuffer, File::Text);
	if(f.IsOpen())
	{
		f.WriteInt32((obuint32)m_StartPositions.size());
		Vector3List::const_iterator cIt = m_StartPositions.begin();
		for(; cIt != m_StartPositions.end(); ++cIt)
		{
			f.WriteFloat((*cIt).x);
			f.WriteFloat((*cIt).y);
			f.WriteFloat((*cIt).z);
			f.WriteNewLine();
		}
		f.Close();
	}
	EngineFuncs::ConsoleMessage(va("Saved %d nav starts from %s", 
		m_StartPositions.size(), strMap.c_str()));
}

void PathPlannerFloodFill::LoadFloodStarts()
{
	String strMap = g_EngineFuncs->GetMapName();
	strMap += ".navstarts";

	char strBuffer[1024] = {};
	sprintf(strBuffer, "nav/%s", strMap.c_str());

	File f;
	f.OpenForRead(strBuffer, File::Text);
	if(f.IsOpen())
	{
		obuint32 iNumPositions = 0;
		f.ReadInt32(iNumPositions);

		m_StartPositions.resize(0);
		m_StartPositions.reserve(iNumPositions);

		Vector3f vPos;
		for(obuint32 i = 0; i < iNumPositions; ++i)
		{
			f.ReadFloat(vPos.x);
			f.ReadFloat(vPos.y);
			f.ReadFloat(vPos.z);
			m_StartPositions.push_back(_GetNearestGridPt(vPos));
		}
		f.Close();
	}
	EngineFuncs::ConsoleMessage(va("Loaded %d nav starts from %s", 
		m_StartPositions.size(), strMap.c_str()));
}

void PathPlannerFloodFill::FloodFill(const FloodFillOptions &_options)
{
	if(IGameManager::GetInstance()->RemoveUpdateFunction("NavMesh_FloodFill"))
		return;

	m_FloodFillOptions = _options;

	m_FloodFillData.reset();
	FunctorPtr f(new ObjFunctor<PathPlannerFloodFill>(this, &PathPlannerFloodFill::Process_FloodFill));
	IGameManager::GetInstance()->AddUpdateFunction("NavMesh_FloodFill", f);
}

void PathPlannerFloodFill::TrimSectors(float _trimarea)
{
	SectorList::iterator sIt = m_Sectors.begin();
	for(; sIt != m_Sectors.end(); )
	{
		float fArea = (*sIt).m_SectorBounds.GetAxisLength(0) * 
			(*sIt).m_SectorBounds.GetAxisLength(1);
		if(fArea < _trimarea)
		{
			Utils::OutlineAABB((*sIt).m_SectorBounds, COLOR::RED, 10.f);
			m_Sectors.erase(sIt++);
		}
		else
			++sIt;
	}
}

//bool PathPlannerFloodFill::DoesSectorAlreadyExist(const NavSector &_ns)
//{
//	Vector3f vAvg = Utils::AveragePoint(_ns.m_Boundary);
//
//	for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
//	{
//		Vector3f vSecAvg = Utils::AveragePoint(m_NavSectors[s].m_Boundary);
//		if(Length(vAvg, vSecAvg) < Mathf::EPSILON)
//			return true;
//	}
//	return false;
//}

//////////////////////////////////////////////////////////////////////////

bool PathPlannerFloodFill::GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name)
{

	return false;
}

void PathPlannerFloodFill::AddEntityConnection(const Event_EntityConnection &_conn)
{

}

void PathPlannerFloodFill::RemoveEntityConnection(GameEntity _ent)
{

}

Vector3f PathPlannerFloodFill::GetDisplayPosition(const Vector3f &_pos)
{
	return _pos;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void PathPlannerFloodFill::OverlayRender(RenderOverlay *overlay, const ReferencePoint &viewer)
{
	if(m_FloodFillData)
	{
		for(obint32 n = 0; n < (obint32)m_FloodFillData->m_NodeIndex; ++n)
		{
			const Vector3f nodePos = m_FloodFillData->m_Nodes[n].m_Position;

			//////////////////////////////////////////////////////////////////////////
			// Translate to world aabb
			static float fRenderDistance = 1024.0f;		

			{
				// limit to field of view.
				Vector3f vToNode = (m_FloodFillData->m_Nodes[n].m_Position - viewer.EyePos);
				vToNode.Normalize();
				if(Utils::InFieldOfView(viewer.Facing, vToNode, 90.0f))
				{
					AABB worldAABB = m_FloodFillData->m_GridAABB.TranslateCopy(m_FloodFillData->m_Nodes[n].m_Position);

					if(!m_FloodFillData->m_Nodes[n].m_ValidPos)
						continue;

					if(m_FloodFillData->m_Nodes[n].m_Sectorized)
						continue;

					obColor col = COLOR::GREY;

					/*if(m_FloodFillData->m_Nodes[n].m_Ladder)
					col = COLOR::ORANGE;
					elseif(m_FloodFillData->m_Nodes[n].m_Movable)
					col = COLOR::BROWN;
					else*/ if(m_FloodFillData->m_Nodes[n].m_Open)
						col = COLOR::GREEN;
					else if(m_FloodFillData->m_Nodes[n].m_InWater)
						col = COLOR::BLUE;
					/*else if(m_FloodFillData->m_Nodes[n].m_Jump)
					col = COLOR::MAGENTA;*/
					else if(m_FloodFillData->m_Nodes[n].m_NearSolid)
						col = COLOR::RED;
					else if(m_FloodFillData->m_Nodes[n].m_NearVoid)
						col = COLOR::GREEN;

					int iNumBorderNeighbors = 0;
					for(int dir = 0; dir < NUM_DIRS; ++dir)
					{
						if(m_FloodFillData->m_Nodes[n].m_Connections[dir].Index != -1)
						{
							const NavNode &neighbor = 
								m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[n].m_Connections[dir].Index];
							if(neighbor.m_NearSolid)
								iNumBorderNeighbors++;
						}
					}
					if(iNumBorderNeighbors>2)
						col = COLOR::BLACK;

					/*if(m_FloodFillData->m_Nodes[n].m_Region>0)
					{
						Utils::PrintText(
							m_FloodFillData->m_Nodes[n].m_Position, 
							COLOR::WHITE, 
							2.f,
							"%d", 
							m_FloodFillData->m_Nodes[n].m_Region);
					}

					if(m_FloodFillData->m_Nodes[n].m_DistanceFromEdge != 0 &&
						!m_FloodFillData->m_Nodes[n].m_Sectorized)
					{
						Utils::PrintText(
							m_FloodFillData->m_Nodes[n].m_Position, 
							COLOR::BLACK, 
							2.f,
							"%d", 
							m_FloodFillData->m_Nodes[n].m_DistanceFromEdge);
					}*/

					//Utils::OutlineAABB(worldAABB, col, 2.0f, AABB::DIR_BOTTOM);

					//// Render connections
					//if(m_PlannerFlags.CheckFlag(NAVMESH_CONNECTIONVIEW))
					//{
					//	for(int dir = 0; dir < NUM_DIRS; ++dir)
					//	{
					//		if(m_FloodFillData->m_Nodes[n].m_Connections[dir] != -1)
					//		{
					//			Utils::DrawLine(
					//				m_FloodFillData->m_Nodes[n].m_Position + Vector3f(0.f, 0.f, 5.f),
					//				m_FloodFillData->m_Nodes[m_FloodFillData->m_Nodes[n].m_Connections[dir]].m_Position,
					//				COLOR::CYAN, 2.f);
					//		}
					//	}
					//}

					for(int dir = 0; dir < NUM_DIRS; ++dir)
					{
						const NavNode &nn = m_FloodFillData->m_Nodes[n];
						const NavNode::Connection &conn = nn.m_Connections[dir];
						const int iConnectedNode = conn.Index;
						if(iConnectedNode != -1)
						{
							if(conn.Jump)
								col = COLOR::MAGENTA;

							/*if(drawnLinks.find(Utils::MakeId32((obuint16)n, (obuint16)iConnectedNode)) == drawnLinks.end())
							{
								Utils::DrawLine(
									m_FloodFillData->m_Nodes[n].m_Position,
									m_FloodFillData->m_Nodes[iConnectedNode].m_Position,
									col, 2.f);

								drawnLinks.insert(Utils::MakeId32((obuint16)iConnectedNode, (obuint16)n));
							}*/
							overlay->SetColor(col);
							overlay->DrawLine(
								m_FloodFillData->m_Nodes[n].m_Position,
								m_FloodFillData->m_Nodes[iConnectedNode].m_Position);
						}
					}
				}
			}
		}
	}
}
