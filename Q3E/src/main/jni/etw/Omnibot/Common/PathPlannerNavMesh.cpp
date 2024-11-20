#include "PrecompCommon.h"
#include "PathPlannerNavMesh.h"
#include "IGameManager.h"
#include "IGame.h"
#include "GoalManager.h"
#include "NavigationFlags.h"
#include "AStarSolver.h"
#include "gmUtilityLib.h"

#include "QuadTree.h"
using namespace std;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

const float k_fWorldExtent = 1000.0f;
const float k_fGroundExtent = 10.0f;

//////////////////////////////////////////////////////////////////////////
namespace NavigationMeshOptions
{
	float CharacterHeight = 64.f;	
}
//////////////////////////////////////////////////////////////////////////

PathPlannerNavMesh *g_PlannerNavMesh = 0;

//////////////////////////////////////////////////////////////////////////
// Pathing vars
class PathFindNavMesh
{
public:
	struct PlanNode
	{
		PathPlannerNavMesh::NavSector			*Sector;
		Vector3f								Position;
		PlanNode								*Parent;
		const PathPlannerNavMesh::NavPortal		*Portal;

		int										BoundaryEdge;

		float									CostHeuristic;
		float									CostGiven;
		float									CostFinal;

		obint32 Hash() const
		{
			return (obint32)Utils::MakeId32(
				(obint16)Sector->m_Id,
				Portal ? (obint16)Portal->m_DestSector : (obint16)0xFF);
		}

		void Reset()
		{
			Sector = 0;
			Parent = 0;

			Portal = 0;

			BoundaryEdge = -1;

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
		PathPlannerNavMesh::NavSector *pSector = g_PlannerNavMesh->GetSectorAt(_pos,512.f);
		if(pSector)
		{
			PlanNode *pNode = _GetFreeNode();
			pNode->Parent = 0;
			pNode->Position = _pos;
			pNode->Sector = pSector;
			m_OpenList.push_back(pNode);
			return true;
		}
		return false;
	}
	bool AddGoal(const Vector3f &_pos)
	{
		GoalLocation gl;
		gl.Position = _pos;
		gl.Sector = g_PlannerNavMesh->GetSectorAt(_pos,512.f);
		if(gl.Sector)
		{
			m_CurrentGoals.push_back(gl);
			return true;
		}
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
			if(gl != NULL && SquaredLength(gl->Position,pCurNode->Position)<Mathf::Sqr(32.f))
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
	PathFindNavMesh()
	{
		fl.Finished = true;
		m_UsedNodes = 0;
	}
private:
	struct GoalLocation
	{
		PathPlannerNavMesh::NavSector	*Sector;
		Vector3f						Position;
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
	typedef stdext::hash_compare<int> HashMapCompare;
	typedef stdext::unordered_map<int, PlanNode*, HashMapCompare/*, HashMapAllocator*/ > NavHashMap;
#else
	typedef stdext::hash<int> HashMapCompare;
	typedef stdext::unordered_map<int, PlanNode*, HashMapCompare, stdext::equal_to<int>/*, HashMapAllocator*/ > NavHashMap;
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
		float fClosest = 1000000.f;
		NodeList::iterator itRet = m_OpenList.end();

		NodeList::iterator it = m_OpenList.begin(), itEnd = m_OpenList.end();
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
		}
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
		if(_goal)
		{
			PlanNode *pNextNode = _GetFreeNode();
			pNextNode->Portal = 0;
			pNextNode->Parent = _node;
			pNextNode->Sector = _node->Sector;
			pNextNode->Position = _goal->Position; // TODO: branch more			
			pNextNode->CostHeuristic = 0;
			pNextNode->CostGiven = _node->CostGiven + Length(_node->Position, pNextNode->Position);
			pNextNode->CostFinal = pNextNode->CostHeuristic + pNextNode->CostGiven;
			HeapInsert(m_OpenList, pNextNode);
			return;
		}

		for(int p = _node->Sector->m_StartPortal; 
			p < _node->Sector->m_StartPortal+_node->Sector->m_NumPortals; 
			++p)
		{
			const PathPlannerNavMesh::NavPortal &portal = g_PlannerNavMesh->m_NavPortals[p];

			//for(int b = 0; b < )

			//////////////////////////////////////////////////////////////////////////
			PlanNode tmpNext;
			tmpNext.Reset();
			tmpNext.Portal = &portal;
			tmpNext.Parent = _node;
			tmpNext.Sector = &g_PlannerNavMesh->m_ActiveNavSectors[portal.m_DestSector];
			tmpNext.Position = portal.m_Segment.Origin; // TODO: branch more		

			//if(m_CurrentGoals.size()==1)
			//	tmpNext.CostHeuristic = Length(m_CurrentGoals.front().Position,tmpNext.Position); // USE IF 1 GOAL!
			//else
				tmpNext.CostHeuristic = 0;
			
			tmpNext.CostGiven = _node->CostGiven + Length(_node->Position, tmpNext.Position);
			tmpNext.CostFinal = tmpNext.CostHeuristic + tmpNext.CostGiven;

			//////////////////////////////////////////////////////////////////////////

			// Look in closed list for this. If this is better, re-open it
			NodeClosedList::iterator closedIt = IsOnClosedList(&tmpNext);
			if(closedIt != m_ClosedList.end())
			{
				PlanNode *OnClosed = closedIt->second;
				if(OnClosed->CostGiven > tmpNext.CostGiven)
				{
					*OnClosed = tmpNext;

					// and remove from the closed list.
					m_ClosedList.erase(closedIt);
					// ... back into the open list
					HeapInsert(m_OpenList, OnClosed);
				}
				continue;
			}

			// Look in open list for this. If this is better, update it.
			// Check the open list for this node to see if it's better
			NodeList::iterator openIt = IsOnOpenList(&tmpNext);
			if(openIt != m_OpenList.end())
			{
				PlanNode *pOnOpen = (*openIt);
				if(pOnOpen->CostGiven > tmpNext.CostGiven)
				{
					// Update the open list					
					*pOnOpen = tmpNext;

					// Remove it from the open list first.
					//m_OpenList.erase(openIt);
					// ... and re-insert it

					// Since we possibly removed from the middle, we need to fix the heap again.
					//std::make_heap(m_OpenList.begin(), m_OpenList.end(), waypoint_comp);
					std::push_heap(m_OpenList.begin(), openIt+1, node_compare);
				}
				continue;
			}

			// New node to explore
			PlanNode *pNextNode = _GetFreeNode();
			*pNextNode = tmpNext;
			HeapInsert(m_OpenList, pNextNode);
		}
	}
	GoalLocation *_IsNodeGoalInSector(PlanNode *_node)
	{
		for(obuint32 i = 0; i < m_CurrentGoals.size(); ++i)
		{
			if(m_CurrentGoals[i].Sector == _node->Sector)
			{
				return &m_CurrentGoals[i];
			}
		}
		return NULL;
	}
};

PathFindNavMesh g_PathFind;

//////////////////////////////////////////////////////////////////////////
//Vector3f Convert(const TA::Vec3 &v)
//{
//	return Vector3f(v.x,v.y,v.z);
//}
//TA::Vec3 Convert(const Vector3f &v)
//{
//	return TA::Vec3(v.x,v.y,v.z);
//}
//////////////////////////////////////////////////////////////////////////

Vector3List		g_Vertices;
PolyIndexList	g_Indices;

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

//bool TA_CALL_BACK ColCb(TA::PreCollision& collision)
//{
//	return true;
//}

void PathPlannerNavMesh::InitCollision()
{
	if(m_NavSectors.empty())
		return;

	m_ActiveNavSectors.resize(0);

	ReleaseCollision();

	//////////////////////////////////////////////////////////////////////////
	/*TA::Physics::CreateInstance();
	TA::Physics& physics = TA::Physics::GetInstance();
	physics.SetPreProcessCollisionCallBack(ColCb);
	//////////////////////////////////////////////////////////////////////////

	TA::StaticObject* NavMesh = NavMesh = TA::StaticObject::CreateNew();

	obuint32 iNumIndices = 0;

	TA::AABB aabb;

	g_Vertices.clear();
	g_Indices.clear();

	//////////////////////////////////////////////////////////////////////////
	std::vector<TA::u32> Attribs;
	//////////////////////////////////////////////////////////////////////////
	// Build the active sector list, includes mirrored sectors.
	PolyAttrib attrib;
	for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
	{
		const NavSector &ns = m_NavSectors[s];

		attrib.Attrib = 0;

		attrib.Fields.Mirrored = false;
		attrib.Fields.ActiveId = m_ActiveNavSectors.size();
		attrib.Fields.SectorId = s;
		Attribs.push_back(attrib.Attrib);
		
		m_ActiveNavSectors.push_back(ns);
		m_ActiveNavSectors.back().m_Id = attrib.Fields.ActiveId;
		m_ActiveNavSectors.back().m_Middle = Utils::AveragePoint(m_ActiveNavSectors.back().m_Boundary);

		if(ns.m_Mirror != NavSector::MirrorNone)
		{
			attrib.Fields.Mirrored = true;
			attrib.Fields.ActiveId = m_ActiveNavSectors.size();
			attrib.Fields.SectorId = s;
			Attribs.push_back(attrib.Attrib);

			m_ActiveNavSectors.push_back(ns.GetMirroredCopy(m_MapCenter));
			m_ActiveNavSectors.back().m_Id = attrib.Fields.ActiveId;
			m_ActiveNavSectors.back().m_Middle = Utils::AveragePoint(m_ActiveNavSectors.back().m_Boundary);
		}
	}
	//////////////////////////////////////////////////////////////////////////

	for(obuint32 s = 0; s < m_ActiveNavSectors.size(); ++s)
	{
		const NavSector &ns = m_ActiveNavSectors[s];

		IndexList il;

		// Add the vertices and build the index list.
		for(obuint32 v = 0; v < ns.m_Boundary.size(); ++v)
		{
			il.push_back(g_Vertices.size());
			g_Vertices.push_back(ns.m_Boundary[v]);
			aabb.ExpandToFit(Convert(ns.m_Boundary[v]));
		}

		iNumIndices += il.size();

		std::reverse(il.begin(),il.end());
		g_Indices.push_back(il);
	}

	physics.SetWorldDimensions(aabb);
	physics.SetGravity(TA::Vec3(0.0f, IGame::GetGravity(), 0.0f));
	physics.SetupSimulation();

	//////////////////////////////////////////////////////////////////////////

	TA::CollisionObjectAABBMesh* StaticCollisionObject = TA::CollisionObjectAABBMesh::CreateNew();
	StaticCollisionObject->Initialise(
		g_Vertices.size(),
		g_Indices.size(),
		iNumIndices); 

	// Add all verts
	for(obuint32 v = 0; v < g_Vertices.size(); ++v)
	{
		const Vector3f &vec = g_Vertices[v];
		StaticCollisionObject->AddVertex(Convert(vec));
	}

	// Add all polygons
	for(obuint32 p = 0; p < g_Indices.size(); ++p)
	{
		const IndexList &il = g_Indices[p];
		StaticCollisionObject->AddPolygon(il.size(), &il[0], Attribs[p]);	
	}
	StaticCollisionObject->FinishedAddingGeometry();

	// Initialise the static object with the collision object.
	NavMesh->Initialise(StaticCollisionObject);

	StaticCollisionObject->Release();
	StaticCollisionObject = 0;

	physics.AddStaticObject(NavMesh);
	
	NavMesh->Release();
	NavMesh = 0;*/
}

PathPlannerNavMesh::NavCollision PathPlannerNavMesh::FindCollision(const Vector3f &_from, const Vector3f &_to)
{
	/*if(!m_ActiveNavSectors.empty())
	{
		TA::Vec3 vFrom = Convert(_from);
		TA::Vec3 vTo = Convert(_to);

		TA::Collision col = TA::Physics::GetInstance().TestLineForCollision(vFrom, vTo, TA::Physics::FLAG_ALL_OBJECTS);

		if(col.CollisionOccurred())
		{
			return NavCollision(true, Convert(col.GetPosition()), Convert(col.GetNormal()), col.GetAttributeB());
		}
	}*/
	return NavCollision(false);
}

void PathPlannerNavMesh::ReleaseCollision()
{
	//TA::Physics::DestroyInstance();
}

PathPlannerNavMesh::NavSector *PathPlannerNavMesh::GetSectorAt(const Vector3f &_pos, float _distance)
{
	const float CHAR_HALF_HEIGHT = NavigationMeshOptions::CharacterHeight /** 0.5f*/;
	return GetSectorAtFacing(_pos+Vector3f(0,0,CHAR_HALF_HEIGHT), -Vector3f::UNIT_Z, _distance);
}

PathPlannerNavMesh::NavSector *PathPlannerNavMesh::GetSectorAtFacing(const Vector3f &_pos, const Vector3f &_facing, float _distance)
{
	if(!m_ActiveNavSectors.empty())
	{
		/*TA::Vec3 vFrom = Convert(_pos);
		TA::Vec3 vTo = Convert(_pos + _facing * _distance);

		TA::Collision col = TA::Physics::GetInstance().TestLineForCollision(
			vFrom, vTo, TA::Physics::FLAG_ALL_OBJECTS);

		if(col.CollisionOccurred())
		{
			PolyAttrib attr;
			attr.Attrib = col.GetAttributeB();
			return &m_ActiveNavSectors[attr.Fields.ActiveId];
		}*/
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class NavigationMesh
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

PathPlannerNavMesh::PathPlannerNavMesh()
{	
	// TEMP
	m_PlannerFlags.SetFlag(NAV_VIEW);
	m_PlannerFlags.SetFlag(NAVMESH_STEPPROCESS);

	m_CursorColor = COLOR::BLUE;

	m_MapCenter = Vector3f::ZERO;

	g_PlannerNavMesh = this;
}

PathPlannerNavMesh::~PathPlannerNavMesh()
{
	Shutdown();
	g_PlannerNavMesh = 0;
}

int PathPlannerNavMesh::GetLatestFileVersion() const
{
	return 1; // TODO
}

PathPlannerNavMesh::NavSector PathPlannerNavMesh::NavSector::GetMirroredCopy(const Vector3f &offset) const
{
	NavSector ns;
	ns.m_Mirror = NavSector::MirrorNone;

	Vector3f vAxis;
	switch(m_Mirror)
	{
	case NavSector::MirrorX:
		vAxis = Vector3f::UNIT_X;
		break;
	case NavSector::MirrorNX:
		vAxis = -Vector3f::UNIT_X;
		break;
	case NavSector::MirrorY:
		vAxis = Vector3f::UNIT_Y;
		break;
	case NavSector::MirrorNY:
		vAxis = -Vector3f::UNIT_Y;
		break;
	case NavSector::MirrorZ:
		vAxis = Vector3f::UNIT_Z;
		break;
	case NavSector::MirrorNZ:
		vAxis = -Vector3f::UNIT_Z;
		break;
	}

	Matrix3f mat(vAxis, Mathf::DegToRad(180.0f));
	ns.m_Boundary = m_Boundary;
	for(obuint32 m = 0; m < ns.m_Boundary.size(); ++m)
	{
		ns.m_Boundary[m] -= offset;
		ns.m_Boundary[m] = mat * ns.m_Boundary[m];
		ns.m_Boundary[m] += offset;
	}
	return ns;
}

void PathPlannerNavMesh::NavSector::GetEdgeSegments(SegmentList &_list) const
{
	for(obuint32 m = 0; m < m_Boundary.size()-1; ++m)
	{
		_list.push_back(Utils::MakeSegment(m_Boundary[m],m_Boundary[m+1]));
	}
	_list.push_back(Utils::MakeSegment(m_Boundary[m_Boundary.size()-1],m_Boundary[0]));
}

SegmentList PathPlannerNavMesh::NavSector::GetEdgeSegments() const
{
	SegmentList lst;
	GetEdgeSegments(lst);
	return lst;
}

bool PathPlannerNavMesh::Init()
{	
	InitCommands();
	return true;
}

void PathPlannerNavMesh::Update()
{
	Prof(PathPlannerNavMesh);

	UpdateFsm(IGame::GetDeltaTimeSecs());

	if(m_PlannerFlags.CheckFlag(NAV_VIEW))
	{
		g_PathFind.Render();

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
		obint32 iCurrentSector = -1;
		NavCollision nc = FindCollision(vLocalPos, vLocalPos + vLocalAim * 1024.f);
		if(nc.DidHit())
		{
			iCurrentSector = nc.HitAttrib().Fields.ActiveId;
		}
		//////////////////////////////////////////////////////////////////////////

		NavSector nsMirrored;

		static int iLastSector = -1;
		
		static int iNextCurSectorTimeUpdate = 0;
		if(iCurrentSector != -1 &&
			(iLastSector != iCurrentSector || IGame::GetTime() >= iNextCurSectorTimeUpdate))
		{
			iLastSector = iCurrentSector;
			iNextCurSectorTimeUpdate = IGame::GetTime() + 500;
			Utils::DrawPolygon(m_ActiveNavSectors[iCurrentSector].m_Boundary, COLOR::RED, 0.5f, false);
		}

		static int iNextTimeUpdate = 0;
		if(IGame::GetTime() >= iNextTimeUpdate)
		{
			iNextTimeUpdate = IGame::GetTime() + 2000;
			//////////////////////////////////////////////////////////////////////////
			// Draw our nav sectors
			Utils::DrawPolygon(m_CurrentSector, COLOR::GREEN.fade(100), 2.f, false);
			Utils::DrawLine(m_CurrentSector, COLOR::GREEN, 2.f, 2.f, COLOR::MAGENTA, true);

			for(obuint32 i = 0; i < m_NavSectors.size(); ++i)
			{
				const NavSector &ns = m_NavSectors[i];
				obColor col = COLOR::GREEN;
				Utils::DrawLine(ns.m_Boundary, col, 2.f, 2.f, COLOR::MAGENTA, true);
				
				//////////////////////////////////////////////////////////////////////////
				Vector3f vMid = Vector3f::ZERO;
				for(obuint32 v = 0; v < ns.m_Boundary.size(); ++v)
					vMid += ns.m_Boundary[v];
				vMid /= (float)ns.m_Boundary.size();

				const char *pMir[] =
				{
					0,
					"x",
					"-x",
					"y",
					"-y",
					"z",
					"-z",
				};

				if(pMir[ns.m_Mirror])
				{
					nsMirrored = ns.GetMirroredCopy(m_MapCenter);
					Utils::DrawLine(nsMirrored.m_Boundary, COLOR::CYAN, 2.f, 2.f, COLOR::MAGENTA, true);
				}

				/*if(pMir[ns.m_Mirror])
					Utils::PrintText(vMid, 2.f, "%d m(%s)", i, pMir[ns.m_Mirror]);
				else
					Utils::PrintText(vMid, 2.f, "%d", i);*/
			}

			if(m_PlannerFlags.CheckFlag(NAV_VIEWCONNECTIONS))
			{
				NavPortalList ::const_iterator pIt = m_NavPortals.begin();
				for(; pIt != m_NavPortals.end(); ++pIt)
				{
					const NavPortal &portal = (*pIt);
					Utils::DrawLine(portal.m_Segment.GetPosEnd(),
						portal.m_Segment.Origin+Vector3f(0,0,32),COLOR::MAGENTA,10.f);
					Utils::DrawLine(portal.m_Segment.GetNegEnd(),
						portal.m_Segment.Origin+Vector3f(0,0,32),COLOR::MAGENTA,10.f);
				}
			}
			//////////////////////////////////////////////////////////////////////////
		}

		//////////////////////////////////////////////////////////////////////////

		if(!m_WorkingSector.empty())
			Utils::DrawPolygon(m_WorkingSector, COLOR::GREEN.fade(100), 0.1f, false);
	}
}

void PathPlannerNavMesh::Shutdown()
{
	Unload();
}

bool PathPlannerNavMesh::Load(const String &_mapname, bool _dl)
{
	if(_mapname.empty())
		return false;

	gmMachine *pM = new gmMachine;
	pM->SetDebugMode(true);
	DisableGCInScope gcEn(pM);

	String waypointName		= _mapname + ".nav";

	File InFile;

	char strbuffer[1024] = {};
	sprintf(strbuffer, "nav/%s", waypointName.c_str());
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
	NavSectorList loadSectors;
	gmTableObject *pNavTbl = pM->GetGlobals()->Get(pM, "Navigation").GetTableObjectSafe();
	if(pNavTbl)
	{
		gmVariable vCenter = pNavTbl->Get(pM,"MapCenter");
		if(vCenter.IsVector())
			vCenter.GetVector(m_MapCenter);
		else
			m_MapCenter = Vector3f::ZERO;

		gmTableObject *pSectorsTable = pNavTbl->Get(pM, "Sectors").GetTableObjectSafe();
		if(pSectorsTable != NULL && bSuccess)
		{
			gmTableIterator sectorIt;
			for(gmTableNode *pSectorNode = pSectorsTable->GetFirst(sectorIt);
				pSectorNode && bSuccess;
				pSectorNode = pSectorsTable->GetNext(sectorIt))
			{
				gmTableObject *pSector = pSectorNode != NULL ? pSectorNode->m_value.GetTableObjectSafe() : NULL;
				if(pSector)
				{
					NavSector ns;

					//const char *pSectorName = pSector->Get(pM, "Name").GetCStringSafe();

					gmVariable vMirror = pSector->Get(pM, "Mirror");
					ns.m_Mirror = vMirror.IsInt() ? vMirror.GetInt() : NavSector::MirrorNone;

					gmTableObject *pVertTable = pSector->Get(pM, "Vertices").GetTableObjectSafe();
					if(pVertTable != NULL && bSuccess)
					{
						gmTableIterator vertIt;
						for(gmTableNode *pVertNode = pVertTable->GetFirst(vertIt);
							pVertNode && bSuccess;
							pVertNode = pVertTable->GetNext(vertIt))
						{
							Vector3f v;
							if(pVertNode != NULL && pVertNode->m_value.IsVector() && pVertNode->m_value.GetVector(v.x, v.y, v.z))
								ns.m_Boundary.push_back(v);
							else
								bSuccess = false;
						}
					}

					//////////////////////////////////////////////////////////////////////////
					if(bSuccess)
						loadSectors.push_back(ns);
					//////////////////////////////////////////////////////////////////////////
				}
			}
		}
	}

	if(bSuccess)
	{
		m_NavSectors.swap(loadSectors);
		InitCollision();
	}
	//////////////////////////////////////////////////////////////////////////

	delete pM;

	return bSuccess;
}

bool PathPlannerNavMesh::Save(const String &_mapname)
{
	if(_mapname.empty())
		return false;

	String waypointName		= _mapname + ".nav";
	String navPath	= String("nav/") + waypointName;

	gmMachine *pM = new gmMachine;
	pM->SetDebugMode(true);
	DisableGCInScope gcEn(pM);

	gmTableObject *pNavTbl = pM->AllocTableObject();
	pM->GetGlobals()->Set(pM, "Navigation", gmVariable(pNavTbl));

	pNavTbl->Set(pM,"MapCenter",gmVariable(m_MapCenter));

	gmTableObject *pSectorsTable = pM->AllocTableObject();
	pNavTbl->Set(pM, "Sectors", gmVariable(pSectorsTable));

	for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
	{
		const NavSector &ns = m_NavSectors[s];

		gmTableObject *pSector = pM->AllocTableObject();
		pSectorsTable->Set(pM, s, gmVariable(pSector));

		// NavSector properties
		pSector->Set(pM, "Mirror", gmVariable((obint32)ns.m_Mirror));

		gmTableObject *pSectorVerts = pM->AllocTableObject();
		pSector->Set(pM, "Vertices", gmVariable(pSectorVerts));

		for(obuint32 v = 0; v < ns.m_Boundary.size(); ++v)
		{
			pSectorVerts->Set(pM,v,gmVariable(ns.m_Boundary[v]));
		}		
	}

	gmUtility::DumpTable(pM,navPath.c_str(),"Navigation",gmUtility::DUMP_ALL);
	delete pM;

	return true;
}

bool PathPlannerNavMesh::IsReady() const
{
	return !m_ActiveNavSectors.empty();
}

bool PathPlannerNavMesh::GetNavFlagByName(const String &_flagname, NavFlags &_flag) const
{	
	_flag = 0;
	return false;
}

Vector3f PathPlannerNavMesh::GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team)
{
	Vector3f dest = _start;
	
	if(!m_ActiveNavSectors.empty())
	{
		const NavSector &randSector = m_ActiveNavSectors[rand()%m_ActiveNavSectors.size()];
		dest = Utils::AveragePoint(randSector.m_Boundary);
	}
	return dest;
}

//////////////////////////////////////////////////////////////////////////

int PathPlannerNavMesh::PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team)
{
	DestinationVector dst;
	for(obuint32 i = 0; i < _goals.size(); ++i)
		dst.push_back(Destination(_goals[i],32.f));
	return PlanPathToNearest(_client,_start,dst,_team);
}

int PathPlannerNavMesh::PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team)
{
	g_PathFind.StartNewSearch();
	g_PathFind.AddStart(_start);

	for(obuint32 i = 0; i < _goals.size(); ++i)
		g_PathFind.AddGoal(_goals[i].m_Position);

	while(!g_PathFind.IsFinished())
		g_PathFind.Iterate();

	return g_PathFind.GetGoalIndex();
}

void PathPlannerNavMesh::PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team)
{
	DestinationVector dst;
	dst.push_back(Destination(_goal,32.f));
	PlanPathToNearest(_client,_start,dst,_team);
}

bool PathPlannerNavMesh::IsDone() const
{
	return g_PathFind.IsFinished();
}
bool PathPlannerNavMesh::FoundGoal() const
{
	return g_PathFind.FoundGoal();
}

void PathPlannerNavMesh::Unload()
{	
}

void PathPlannerNavMesh::RegisterGameGoals()
{
}

void PathPlannerNavMesh::GetPath(Path &_path, int _smoothiterations)
{
	const float CHAR_HALF_HEIGHT = NavigationMeshOptions::CharacterHeight * 0.75f;

	PathFindNavMesh::NodeList &nl = g_PathFind.GetSolution();

	//////////////////////////////////////////////////////////////////////////
	if(nl.size() > 2)
	{
		for(int i = 0; i < _smoothiterations; ++i)
		{
			bool bSmoothed = false;

			// solution is goal to start
			for(obuint32 n = 1; n < nl.size()-1; ++n)
			{
				PathFindNavMesh::PlanNode *pFrom = nl[n+1];
				PathFindNavMesh::PlanNode *pTo = nl[n-1];
				PathFindNavMesh::PlanNode *pMid = nl[n];
				if(!pMid->Portal /*|| pMid->Portal->m_LinkFlags & teleporter*/)
					continue;

				Segment3f portalSeg = pMid->Portal->m_Segment;
				portalSeg.Extent -= 32.f;
				Segment3f seg = Utils::MakeSegment(pFrom->Position,pTo->Position);
				//DistancePointToLine(_seg1.Origin,_seg2.GetNegEnd(),_seg2.GetPosEnd(),&cp);

				Vector3f intr;
				if(Utils::intersect2D_Segments(seg,portalSeg,&intr))
				{
					// adjust the node position
					if(SquaredLength(intr,pMid->Position) > Mathf::Sqr(16.f))
					{
						//Utils::DrawLine(pMid->Position+Vector3f(0,0,32),intr,COLOR::YELLOW,15.f);
						bSmoothed = true;
						pMid->Position = intr;
					}					
				}
			}

			if(!bSmoothed)
				break;
		}
	}
	//////////////////////////////////////////////////////////////////////////

	while(!nl.empty())
	{
		PathFindNavMesh::PlanNode *pn = nl.back();
		Vector3f vNodePos = pn->Position;

		_path.AddPt(vNodePos + Vector3f(0,0,CHAR_HALF_HEIGHT),32.f)
			.NavId(pn->Sector->m_Id)
			/*.Flags(m_Solution.back()->GetNavigationFlags())
			.OnPathThrough(m_Solution.back()->OnPathThrough())
			.OnPathThroughParam(m_Solution.back()->OnPathThroughParam())*/;

		nl.pop_back();
	}
}

//////////////////////////////////////////////////////////////////////////

bool PathPlannerNavMesh::GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name)
{

	return false;
}

void PathPlannerNavMesh::AddEntityConnection(const Event_EntityConnection &_conn)
{

}

void PathPlannerNavMesh::RemoveEntityConnection(GameEntity _ent)
{

}

Vector3f PathPlannerNavMesh::GetDisplayPosition(const Vector3f &_pos)
{
	return _pos;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
