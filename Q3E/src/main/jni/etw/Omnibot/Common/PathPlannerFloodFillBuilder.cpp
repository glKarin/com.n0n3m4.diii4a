#include "PrecompCommon.h"
#include "PathPlannerFloodFill.h"

#include "QuadTree.h"

//////////////////////////////////////////////////////////////////////////

const float g_QuadTreeNodeSize = 256.0f;

//////////////////////////////////////////////////////////////////////////

void PathPlannerFloodFill::_InitFloodFillData()
{
	AABB aabb;
	g_EngineFuncs->GetMapExtents(aabb);
	aabb.Expand(m_FloodFillOptions.m_GridRadius * 2.f);
	m_FloodFillData.reset(new FloodFillData);
	m_FloodFillData->m_WorldAABB = aabb;
	m_FloodFillData->m_CurrentCell = aabb.m_Mins;

	m_FloodFillData->m_NodeIndex = 0;			

	memset(&m_FloodFillData->m_Stats, 0, sizeof(m_FloodFillData->m_Stats));
	memset(m_FloodFillData->m_Nodes, 0, sizeof(m_FloodFillData->m_Nodes));

	m_FloodFillData->m_QuadTree.reset(new QuadTree(m_FloodFillData->m_WorldAABB, g_QuadTreeNodeSize));
	m_FloodFillData->m_TriMeshQuadTree.reset(new QuadTree(m_FloodFillData->m_WorldAABB, g_QuadTreeNodeSize));

	// Calculate the grid aabb
	m_FloodFillData->m_GridAABB = AABB(Vector3f::ZERO);
	m_FloodFillData->m_GridAABB.Expand(Vector3f(m_FloodFillOptions.m_GridRadius, m_FloodFillOptions.m_GridRadius, 0.0f));
	m_FloodFillData->m_GridAABB.Expand(Vector3f(-m_FloodFillOptions.m_GridRadius, -m_FloodFillOptions.m_GridRadius, 0.0f));
	m_FloodFillData->m_GridAABB.m_Maxs[2] = m_FloodFillOptions.m_CharacterHeight - m_FloodFillOptions.m_CharacterStepHeight;

	// Clear the vertex list.
	m_FloodFillData->m_VertIndex = 0;	
	m_FloodFillData->m_FaceIndex = 0;

	m_Sectors.clear();

	String strDataSize = Utils::FormatByteString(sizeof(FloodFillData));
	EngineFuncs::ConsoleMessage(va("FloodFill Data Pool: %s", strDataSize.c_str()));
}

bool PathPlannerFloodFill::_IsNodeGood(NavNode *_navnode, const AABB &bounds)
{
	obTraceResult tr;
	EngineFuncs::TraceLine(tr, 
		_navnode->m_Position,
		_navnode->m_Position,
		&bounds,
		TR_MASK_FLOODFILL,
		-1, 
		False);

	return (tr.m_Fraction == 1.0f);
}

bool PathPlannerFloodFill::_IsStartPositionValid(const Vector3f &_pos)
{
	return true;
}

void PathPlannerFloodFill::_GetNodeProperties(NavNode &_node)
{
	int iContents = g_EngineFuncs->GetPointContents(_node.m_Position);	
	if(iContents & CONT_WATER)
		_node.m_InWater = true;
	/*if(iContents & CONT_LADDER)
		_node.m_Ladder = true;
	if(iContents & CONT_MOVER)
		_node.m_Movable = true;*/
}

void PathPlannerFloodFill::_AddNode(const Vector3f &_pos)
{
	if(_IsStartPositionValid(_pos))
	{
		obTraceResult tr;
		Vector3f vPoint = _GetNearestGridPt(_pos);
		Vector3f vStart = vPoint + Vector3f::UNIT_Z * 32.0f;
		Vector3f vEnd = vPoint - Vector3f::UNIT_Z * m_FloodFillOptions.m_CharacterHeight;
		EngineFuncs::TraceLine(tr, 
			vStart, 
			vEnd, 
			0,
			TR_MASK_FLOODFILL,
			-1, 
			False);

		Vector3f vStartPosition;
		if(tr.m_Fraction < 1.0f)
			vStartPosition = tr.m_Endpos;
		else
		{
			EngineFuncs::ConsoleError("Error Getting Starting Floor Position");
			return;
		}

		vStartPosition.z += m_FloodFillOptions.m_CharacterStepHeight;

		// Store it in the quad tree with its index.
		m_FloodFillData->m_QuadTree->AddPoint(vStartPosition, m_FloodFillData->m_NodeIndex);

		// Add to the open list.
		m_FloodFillData->m_Nodes[m_FloodFillData->m_NodeIndex].m_Position = vStartPosition;
		m_FloodFillData->m_Nodes[m_FloodFillData->m_NodeIndex].m_ValidPos = true;
		m_FloodFillData->m_Nodes[m_FloodFillData->m_NodeIndex].m_Open = true;

		_GetNodeProperties(m_FloodFillData->m_Nodes[m_FloodFillData->m_NodeIndex]);

		m_FloodFillData->m_NodeIndex++;
	}
}

PathPlannerFloodFill::NavNode *PathPlannerFloodFill::_GetNextOpenNode()
{
	for(obuint32 i = 0; i < m_FloodFillData->m_NodeIndex; ++i)
	{
		if(m_FloodFillData->m_Nodes[i].m_Open)
			return &m_FloodFillData->m_Nodes[i];
	}
	return NULL;
}

PathPlannerFloodFill::NavNode *PathPlannerFloodFill::_GetExpansionNode(NavNode *_navnode, NodeDirection _dir, float _step)
{
	Vector3f Expand[4] =
	{
		Vector3f(1.0f, 0.0f, 0.0f),
		Vector3f(0.0f, 1.0f, 0.0f),
		Vector3f(-1.0f, 0.0f, 0.0f),		
		Vector3f(0.0f, -1.0f, 0.0f),		
	};

	NavNode *expNode = &m_FloodFillData->m_Nodes[m_FloodFillData->m_NodeIndex];
	memset(expNode, 0, sizeof(NavNode));
	expNode->m_Open = true;
	expNode->m_Position = _navnode->m_Position + Expand[_dir] * _step;
	expNode->m_Position.z += (m_FloodFillOptions.m_CharacterJumpHeight - m_FloodFillOptions.m_CharacterStepHeight);
	expNode->m_Normal = Vector3f::UNIT_Z;
	for(int c = 0; c < NUM_DIRS; ++c)
		expNode->m_Connections[c].Reset();

	return expNode;
}

void PathPlannerFloodFill::_ExpandNode(NavNode *_navnode)
{
	//////////////////////////////////////////////////////////////////////////
	// Expand from this node.
	const AABB fullNodeSize = m_FloodFillData->m_GridAABB;

	const AABB &nodeSize = fullNodeSize;

	// Test this node for validity.
	if(_IsNodeGood(_navnode, nodeSize))
	{
		// For drawing?
		AddFaceToNavMesh(_navnode->m_Position, *_navnode);

		//////////////////////////////////////////////////////////////////////////
		// Attempt to expand in each direction.
		for(int i = 0; i < NUM_DIRS; ++i)
		{
			NavNode *pExpNode = _GetExpansionNode(_navnode,(NodeDirection)i,m_FloodFillOptions.m_GridRadius * 2.f);

			//////////////////////////////////////////////////////////////////////////
			// Attempt to drop the node to ground level.
			obTraceResult tr;
			static const float fDropDistance = 512.0f;
			EngineFuncs::TraceLine(tr, 
				pExpNode->m_Position, 
				pExpNode->m_Position + -Vector3f::UNIT_Z * fDropDistance,
				/*&m_FloodFillData->m_GridAABB*/0,
				TR_MASK_FLOODFILL,
				-1, 
				False);

			// If this expansion starts in a solid, mark the source node as near solid.
			if(tr.m_StartSolid)
			{
				/*Utils::DrawLine(expNode.m_Position, 
				expNode.m_Position + -Vector3f::UNIT_Z * fDropDistance, COLOR::RED, 60.f);*/

				_navnode->m_NearSolid = true;
				continue;
			}

			if(tr.m_Fraction < 1.0f)
			{
				// Adjust the drop point above the ground by the stepheight.
				pExpNode->m_Position.z = tr.m_Endpos[2] + m_FloodFillOptions.m_CharacterStepHeight;
				pExpNode->m_Normal = Vector3f(tr.m_Normal);

				// Determine if the expansion should be the result of a stepnode or jump.
				if(tr.m_Endpos[2] >= _navnode->m_Position.z + m_FloodFillOptions.m_CharacterStepHeight)
				{
					_navnode->m_Connections[i].Jump = true;
				}
				else if(pExpNode->m_Position.z < _navnode->m_Position.z-m_FloodFillOptions.m_CharacterJumpHeight)
				{
					_navnode->m_NearVoid = true;
					pExpNode->m_ValidPos = false;
					//continue;
				}
			}
			else
			{
				// Dropped too far from an edge, mark this node as near void.
				_navnode->m_NearVoid = true;
				pExpNode->m_ValidPos = false;
				continue;
			}
			//////////////////////////////////////////////////////////////////////////

			pExpNode->m_ValidPos = _IsNodeGood(pExpNode, nodeSize);

			// if it failed, try again with a crouch bounds.
			static bool bDoThis = true;
			if(bDoThis && !pExpNode->m_ValidPos)
			{
				static bool bdraw = false;
				if(bdraw)
					Utils::OutlineAABB(nodeSize.TranslateCopy(pExpNode->m_Position), COLOR::GREEN, 5.f);

				AABB crouchBounds = nodeSize;
				crouchBounds.m_Maxs[2] -= m_FloodFillOptions.m_CharacterHeight - m_FloodFillOptions.m_CharacterCrouchHeight;
				pExpNode->m_ValidPos = _IsNodeGood(pExpNode, crouchBounds);
				pExpNode->m_Crouch = pExpNode->m_ValidPos;

				/*if(!expNode.m_ValidPos)
				Utils::OutlineAABB(crouchBounds.TranslateCopy(expNode.m_Position), COLOR::BLUE, 60.f);*/
			}

			// Mark it near a solid if one of the
			if(!_navnode->m_NearSolid)
				_navnode->m_NearSolid = !pExpNode->m_ValidPos;

			if(!m_FloodFillData->m_QuadTree->Within(pExpNode->m_Position))
			{
				// TODO: script callback
				//gmCall call;

				SOFTASSERTALWAYS(0, "Attempted to expand out of bounds! %p", _navnode);
				continue;
			}

			// lookup with quad tree.
			float fClosestDistSq;
			QuadTree::QuadTreeData data;
			OBASSERT(m_FloodFillData->m_QuadTree->Within(pExpNode->m_Position), "Invalid Quad Tree Size");
			if(m_FloodFillData->m_QuadTree->ClosestPtSq(pExpNode->m_Position, data, fClosestDistSq))
			{
				if(fClosestDistSq < (m_FloodFillOptions.m_GridRadius*m_FloodFillOptions.m_GridRadius))
				{
					// Node already exists. make the connection, only if it's valid
					if(m_FloodFillData->m_Nodes[data.m_UserData].m_ValidPos)
						_navnode->m_Connections[i].Index = data.m_UserData;
					continue;
				}
			}
			//else
			{
				if(pExpNode->m_ValidPos)
					_navnode->m_Connections[i].Index = m_FloodFillData->m_NodeIndex;
			}

			//////////////////////////////////////////////////////////////////////////
			// Commit the node to the list.
			m_FloodFillData->m_QuadTree->AddPoint(pExpNode->m_Position, m_FloodFillData->m_NodeIndex);
			++m_FloodFillData->m_NodeIndex;
			//break;
		}
	}
}

bool PathPlannerFloodFill::_IsNodeValidForSectorizing(const Sector &_sector, const NavNode &_node) const
{
	//float fDot = _sector.m_Normal.Dot(_node.m_Normal);
	return /*(fDot > 0.99f) &&*/ _node.m_ValidPos && !_node.m_Sectorized;// &&
		//_node.m_DistanceFromEdge > 1;
}

void PathPlannerFloodFill::_MarkNodesSectorized(const std::vector<int> &_nodelist)
{
	for(obuint32 i = 0; i < _nodelist.size(); ++i)
	{
		m_FloodFillData->m_Nodes[_nodelist[i]].m_Sectorized = true;
	}
}
void PathPlannerFloodFill::_BuildSector(int _floodRoot)
{
	struct Grid
	{
		enum { MaxSectorSize=256 };
		int Cell[MaxSectorSize][MaxSectorSize];

		void Set(int x, int y, int idx)
		{
			Cell[MaxSectorSize/2+x][MaxSectorSize/2+y] = idx;
		}
		int Get(int x, int y)
		{
			return Cell[MaxSectorSize/2+x][MaxSectorSize/2+y];
		}
		void Reset()
		{
			for(int x=0;x<MaxSectorSize;++x)
				for(int y=0;y<MaxSectorSize;++y)
					Cell[x][y] = -1;
		}
		Grid()
		{
			Reset();
		}
	};
	Grid SectorGrid;
	Grid WorkingGrid;

#define EARLYBREAK { bExpanded=false;break; }
#define NOT_MERGABLE (!pNode.m_ValidPos || pNode.m_Sectorized)

	SectorGrid.Set(0,0,_floodRoot);

	int minx = 0,maxx = 0;
	int miny = 0,maxy = 0;

	BitFlag32 bfExpansion;
	bfExpansion.SetFlag(AABB::DIR_NORTH);
	bfExpansion.SetFlag(AABB::DIR_EAST);
	bfExpansion.SetFlag(AABB::DIR_SOUTH);
	bfExpansion.SetFlag(AABB::DIR_WEST);

	// Attempt to expand in each direction.
	while(bfExpansion.AnyFlagSet())
	{
		WorkingGrid = SectorGrid; // work on a copy

		for(int i = 0; i < 4; ++i)
		{
			// skip directions that have previously failed.
			if(!bfExpansion.AnyFlagSet())
				break;
			if(!bfExpansion.CheckFlag(i))
				continue;

			bool bExpanded = true;

			// Get the proper starting corner
			switch(i)
			{
			case AABB::DIR_NORTH:
				{
					const int y = maxy;

					int LastNodeIdx = -1;

					for(int x = minx; x <= maxx; ++x)
					{
						const NavNode::Connection &conn = m_FloodFillData->m_Nodes[WorkingGrid.Get(x,y)].m_Connections[AABB::DIR_NORTH];
						const int conIdx = conn.Index;
						if(conIdx==-1)
							EARLYBREAK;
						NavNode &pNode = m_FloodFillData->m_Nodes[conIdx];

						if(NOT_MERGABLE)
							EARLYBREAK;

						if(pNode.m_Connections[AABB::DIR_SOUTH].Jump)
							EARLYBREAK;

						if(LastNodeIdx != -1)
						{
							const NavNode &lastNode = m_FloodFillData->m_Nodes[LastNodeIdx];
							if(lastNode.m_Connections[AABB::DIR_EAST].Index != conIdx)
								EARLYBREAK;
							if(lastNode.m_Connections[AABB::DIR_EAST].Jump || pNode.m_Connections[AABB::DIR_WEST].Jump)
								EARLYBREAK;
							if(pNode.m_Connections[AABB::DIR_WEST].Index != LastNodeIdx)
								EARLYBREAK;							
						}
						if(conn.Jump || conn.Ladder)
							EARLYBREAK;

						WorkingGrid.Set(x,y+1,conIdx);
						LastNodeIdx = conIdx;
					}

					if(bExpanded)
					{
						// commit the working copy if it completed
						SectorGrid = WorkingGrid;
						maxy++;
					}
					else
					{
						// clear this direction so it doesn't get tried again
						bfExpansion.ClearFlag(i);
					}
					break;
				}
			case AABB::DIR_EAST:
				{
					const int x = maxx;

					int LastNodeIdx = -1;					

					for(int y = miny; y <= maxy; ++y)
					{
						const NavNode::Connection &conn = m_FloodFillData->m_Nodes[WorkingGrid.Get(x,y)].m_Connections[AABB::DIR_EAST];
						const int conIdx = conn.Index;
						if(conIdx==-1)
							EARLYBREAK;
						NavNode &pNode = m_FloodFillData->m_Nodes[conIdx];

						if(NOT_MERGABLE)
							EARLYBREAK;

						if(pNode.m_Connections[AABB::DIR_WEST].Jump)
							EARLYBREAK;

						if(LastNodeIdx != -1)
						{
							const NavNode &lastNode = m_FloodFillData->m_Nodes[LastNodeIdx];
							if(lastNode.m_Connections[AABB::DIR_NORTH].Index != conIdx)
								EARLYBREAK;
							if(lastNode.m_Connections[AABB::DIR_NORTH].Jump || pNode.m_Connections[AABB::DIR_SOUTH].Jump)
								EARLYBREAK;
							if(pNode.m_Connections[AABB::DIR_SOUTH].Index != LastNodeIdx)
								EARLYBREAK;
						}
						if(conn.Jump || conn.Ladder)
							EARLYBREAK;

						WorkingGrid.Set(x+1,y,conIdx);
						LastNodeIdx = conIdx;
					}

					if(bExpanded)
					{
						// commit the working copy if it completed
						SectorGrid = WorkingGrid;
						maxx++;
					}
					else
					{
						// clear this direction so it doesn't get tried again
						bfExpansion.ClearFlag(i);
					}
					break;
				}
			case AABB::DIR_SOUTH:
				{
					const int y = miny;

					int LastNodeIdx = -1;					

					for(int x = minx; x <= maxx; ++x)
					{
						const NavNode::Connection &conn = m_FloodFillData->m_Nodes[WorkingGrid.Get(x,y)].m_Connections[AABB::DIR_SOUTH];
						const int conIdx = conn.Index;
						if(conIdx==-1)
							EARLYBREAK;
						NavNode &pNode = m_FloodFillData->m_Nodes[conIdx];

						if(NOT_MERGABLE)
							EARLYBREAK;

						if(pNode.m_Connections[AABB::DIR_NORTH].Jump)
							EARLYBREAK;

						if(LastNodeIdx != -1)
						{
							const NavNode &lastNode = m_FloodFillData->m_Nodes[LastNodeIdx];
							if(lastNode.m_Connections[AABB::DIR_EAST].Index != conIdx)
								EARLYBREAK;
							if(lastNode.m_Connections[AABB::DIR_EAST].Jump || pNode.m_Connections[AABB::DIR_WEST].Jump)
								EARLYBREAK;
							if(pNode.m_Connections[AABB::DIR_WEST].Index != LastNodeIdx)
								EARLYBREAK;
						}
						if(conn.Jump || conn.Ladder)
							EARLYBREAK;

						WorkingGrid.Set(x,y-1,conIdx);
						LastNodeIdx = conIdx;
					}

					if(bExpanded)
					{
						// commit the working copy if it completed
						SectorGrid = WorkingGrid;
						miny--;
					}
					else
					{
						// clear this direction so it doesn't get tried again
						bfExpansion.ClearFlag(i);
					}
					break;
				}
			case AABB::DIR_WEST:
				{
					const int x = minx;

					int LastNodeIdx = -1;					

					for(int y = miny; y <= maxy; ++y)
					{
						const NavNode::Connection &conn = m_FloodFillData->m_Nodes[WorkingGrid.Get(x,y)].m_Connections[AABB::DIR_WEST];
						const int conIdx = conn.Index;
						if(conIdx==-1)
							EARLYBREAK;
						NavNode &pNode = m_FloodFillData->m_Nodes[conIdx];

						if(NOT_MERGABLE)
							EARLYBREAK;

						if(pNode.m_Connections[AABB::DIR_EAST].Jump)
							EARLYBREAK;

						if(LastNodeIdx != -1)
						{
							const NavNode &lastNode = m_FloodFillData->m_Nodes[LastNodeIdx];
							if(lastNode.m_Connections[AABB::DIR_NORTH].Index != conIdx)
								EARLYBREAK;
							if(lastNode.m_Connections[AABB::DIR_NORTH].Jump || pNode.m_Connections[AABB::DIR_SOUTH].Jump)
								EARLYBREAK;
							if(pNode.m_Connections[AABB::DIR_SOUTH].Index != LastNodeIdx)
								EARLYBREAK;
						}
						if(conn.Jump || conn.Ladder)
							EARLYBREAK;

						WorkingGrid.Set(x-1,y,conIdx);
						LastNodeIdx = conIdx;
					}

					if(bExpanded)
					{
						// commit the working copy if it completed
						SectorGrid = WorkingGrid;
						minx--;
					}
					else
					{
						// clear this direction so it doesn't get tried again
						bfExpansion.ClearFlag(i);
					}
					break;
				}
			default:
				OBASSERT(0, "ERROR");
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Done expanding this sector.
	Sector newSector;
	newSector.m_SectorBounds = 
		m_FloodFillData->m_GridAABB.TranslateCopy(m_FloodFillData->m_Nodes[WorkingGrid.Get(minx,miny)].m_Position);

	for(int x = minx; x <= maxx; ++x)
	{
		for(int y = miny; y <= maxy; ++y)
		{
			// mark sectorized
			NavNode &nn = m_FloodFillData->m_Nodes[WorkingGrid.Get(x,y)];
			nn.m_Sectorized = true;
			nn.m_Sector = (int)m_Sectors.size();

			// make part of this sector
			newSector.m_ContainingCells.insert(WorkingGrid.Get(x,y));

			// update sector bounds
			newSector.m_SectorBounds.Expand(m_FloodFillData->m_GridAABB.TranslateCopy(nn.m_Position));
		}
	}
	m_Sectors.push_back(newSector);
}

//void PathPlannerFloodFill::_BuildSector(int _floodRoot)
//{
//	//////////////////////////////////////////////////////////////////////////
//	struct SweepInfo 
//	{
//		int			m_AabbAxis;
//		Vector3f	m_ExpandDirection;
//	};
//	static SweepInfo sweepStart[4] =
//	{
//		{ 0, Vector3f(1.0f, 0.0f, 0.0f) }, // AABB::DIR_NORTH
//		{ 1, Vector3f(0.0f, -1.0f, 0.0f) }, // AABB::DIR_EAST
//		{ 0, Vector3f(1.0f, 0.0f, 0.0f) }, // AABB::DIR_SOUTH
//		{ 1, Vector3f(0.0f, 1.0f, 0.0f) }, // AABB::DIR_WEST
//	};
//
//	float fStepHeightSquared = m_FloodFillOptions.m_CharacterStepHeight * m_FloodFillOptions.m_CharacterStepHeight;
//
//	//////////////////////////////////////////////////////////////////////////
//	//////////////////////////////////////////////////////////////////////////
//
//	// Reset the internal state
//	std::vector<int> nodeList;
//	nodeList.resize(0);
//	nodeList.push_back(_floodRoot);
//
//	// Set up the sector bounds in world space.
//	Sector newSector;
//	newSector.m_Normal = m_FloodFillData->m_Nodes[_floodRoot].m_Normal;
//	newSector.m_SectorBounds = 
//		m_FloodFillData->m_GridAABB.TranslateCopy(m_FloodFillData->m_Nodes[_floodRoot].m_Position);
//	
//	std::vector<int> expandedCells;
//
//	bool bTryToExpandSector = true;
//	while(bTryToExpandSector)
//	{
//		bTryToExpandSector = false;
//
//		Vector3f vCenter;
//		newSector.m_SectorBounds.CenterPoint(vCenter);
//		
//		// Attempt to expand in each direction.
//		for(int i = 0; i < 4; ++i)
//		{
//			// Clear the expansion cells so we can re-build it
//			expandedCells.resize(0);
//
//			// operate on a copy.
//			AABB aabbExpand = newSector.m_SectorBounds;
//
//			// Get the proper starting corner
//			float fEdgeLength = 0;
//			Vector3f vStartPos, vExpandPos;
//			switch(i)
//			{
//			case AABB::DIR_NORTH:
//				vStartPos = aabbExpand.m_Mins;
//				vStartPos.y += aabbExpand.GetAxisLength(1) + m_FloodFillOptions.m_GridRadius;
//				vStartPos.x += m_FloodFillOptions.m_GridRadius;
//				fEdgeLength = aabbExpand.GetAxisLength(0);
//				vExpandPos = aabbExpand.m_Maxs;
//				vExpandPos.y += m_FloodFillOptions.m_GridRadius * 2.0f;
//				break;
//			case AABB::DIR_EAST:
//				vStartPos = aabbExpand.m_Mins;
//				vStartPos.x += aabbExpand.GetAxisLength(0) + m_FloodFillOptions.m_GridRadius;
//				vStartPos.y += m_FloodFillOptions.m_GridRadius;
//				fEdgeLength = aabbExpand.GetAxisLength(1);
//				vExpandPos = aabbExpand.m_Maxs;
//				vExpandPos.x += m_FloodFillOptions.m_GridRadius * 2.0f;
//				break;
//			case AABB::DIR_SOUTH:
//				vStartPos = aabbExpand.m_Mins;
//				vStartPos.y -= m_FloodFillOptions.m_GridRadius;
//				vStartPos.x += m_FloodFillOptions.m_GridRadius;
//				fEdgeLength = aabbExpand.GetAxisLength(0);
//				vExpandPos = aabbExpand.m_Mins;
//				vExpandPos.y -= m_FloodFillOptions.m_GridRadius * 2.0f;
//				break;
//			case AABB::DIR_WEST:
//				vStartPos = aabbExpand.m_Mins;
//				vStartPos.x -= m_FloodFillOptions.m_GridRadius;
//				vStartPos.y += m_FloodFillOptions.m_GridRadius;
//				fEdgeLength = aabbExpand.GetAxisLength(1);
//				vExpandPos = aabbExpand.m_Mins;
//				vExpandPos.x -= m_FloodFillOptions.m_GridRadius * 2.0f;
//				break;
//			default:
//				OBASSERT(0, "ERROR");
//			}
//
//			const float fIncrement = m_FloodFillOptions.m_GridRadius * 2.0f;
//
//			bool bExpansionGood = true;
//
//			// Attempt to merge all cells along this border, if any fail, the whole expansion fails.
//			Vector3f vTestPt = vStartPos;
//			for(float fStep = 0.0f;
//				fStep < fEdgeLength-m_FloodFillOptions.m_GridRadius;
//				fStep += fIncrement, vTestPt[sweepStart[i].m_AabbAxis] += fIncrement)
//			{
//				float fClosestDistSq;
//				QuadTree::QuadTreeData data;
//				if(m_FloodFillData->m_QuadTree->ClosestPtSq(vTestPt, data, fClosestDistSq))
//				{
//					// Exclude some flagged waypoints from sectors?
//					if(fClosestDistSq > (fStepHeightSquared) || 
//						!_IsNodeValidForSectorizing(newSector, m_FloodFillData->m_Nodes[data.m_UserData]))
//					{
//						//Utils::DrawLine(vTestPt, data.m_Point, COLOR::RED, 10.f);
//						bExpansionGood = false;
//						break;
//					}
//					else
//					{
//						vTestPt.z = data.m_Point.z; // TEMP? adjust the step point to allow up slopes.
//						expandedCells.push_back(data.m_UserData);
//					}
//				}
//				else
//				{
//					bExpansionGood = false;
//					break;
//				}
//			}
//
//			// If at least 1 side expanded, let's keep testing the cell in further updates.
//			if(bExpansionGood)
//			{
//				aabbExpand.Expand(vExpandPos);
//
//				// Success! Mark the expansion nodes that make up this edge as sectorized.
//				for(obuint32 e = 0; e < expandedCells.size(); ++e)
//				{
//					nodeList.push_back(expandedCells[e]);
//				}
//
//				// Update the original sector bounds.
//				newSector.m_SectorBounds = aabbExpand;
//				bTryToExpandSector = true;
//			}
//		}
//	}
//
//	// Done expanding this sector.
//	for(obuint32 n = 0; n < nodeList.size(); ++n)
//		newSector.m_ContainingCells.insert(nodeList[n]);
//	_MarkNodesSectorized(nodeList);
//	m_Sectors.push_back(newSector);
//}
