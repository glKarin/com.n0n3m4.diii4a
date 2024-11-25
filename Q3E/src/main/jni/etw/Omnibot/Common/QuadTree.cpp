#include "PrecompCommon.h"
#include "QuadTree.h"

int QuadTree::m_QuadDepth = 0;
int QuadTree::m_NumNodes = 0;

QuadTree::QuadTree(const AABB &_aabb, float _minNodeSize)
{
	// Get the world center.
	Vector3f vCenter;
	_aabb.CenterPoint(vCenter);

	// Make the bounds square.
	Vector3f vExtents;
	vExtents.x = Mathf::Max(_aabb.m_Maxs[0], Mathf::FAbs(_aabb.m_Mins[0]));
	vExtents.y = Mathf::Max(_aabb.m_Maxs[1], Mathf::FAbs(_aabb.m_Mins[1]));

	float fExtents = Mathf::Max(vExtents.x, vExtents.y);

	m_WorldAABB = _aabb;
	m_WorldAABB.Expand(Vector3f(fExtents, fExtents, vCenter.z));
	m_WorldAABB.Expand(Vector3f(-fExtents, -fExtents, vCenter.z));	

	QuadTree::m_NumNodes = 0;
	QuadTree::m_QuadDepth = 0;

	Split(_minNodeSize);
	
	// Count the quad tree depth.
	QuadTreeNodePtr tmp = m_NorthE;
	while(tmp)
	{
		QuadTree::m_QuadDepth++;
		tmp = tmp->m_NorthE;
	}

	EngineFuncs::ConsoleMessage(va("Quadtree Generated %d node, %d deep.", 
		QuadTree::m_NumNodes, QuadTree::m_QuadDepth));
}

QuadTree::~QuadTree()
{
}

void QuadTree::Split(float _minNodeSize)
{
	if(m_WorldAABB.GetAxisLength(0) > _minNodeSize && 
		m_WorldAABB.GetAxisLength(1) > _minNodeSize)
	{
		Vector3f vCenter;
		m_WorldAABB.CenterPoint(vCenter);

		//float fNewSize = m_WorldAABB.m_Maxs[0] * 0.5f;

		m_NorthE.reset(new QuadTree);
		m_NorthE->m_WorldAABB = m_WorldAABB;
		m_NorthE->m_WorldAABB.m_Mins[0] = vCenter[0];
		m_NorthE->m_WorldAABB.m_Mins[1] = vCenter[1];

		m_SouthE.reset(new QuadTree);
		m_SouthE->m_WorldAABB = m_WorldAABB;
		m_SouthE->m_WorldAABB.m_Mins[0] = vCenter[0];
		m_SouthE->m_WorldAABB.m_Maxs[1] = vCenter[1];
		
		m_NorthW.reset(new QuadTree);
		m_NorthW->m_WorldAABB = m_WorldAABB;
		m_NorthW->m_WorldAABB.m_Maxs[0] = vCenter[0];
		m_NorthW->m_WorldAABB.m_Mins[1] = vCenter[1];

		m_SouthW.reset(new QuadTree);
		m_SouthW->m_WorldAABB = m_WorldAABB;
		m_SouthW->m_WorldAABB.m_Maxs[0] = vCenter[0];
		m_SouthW->m_WorldAABB.m_Maxs[1] = vCenter[1];

		m_NorthE->Split(_minNodeSize);
		m_SouthE->Split(_minNodeSize);
		m_NorthW->Split(_minNodeSize);
		m_SouthW->Split(_minNodeSize);

		QuadTree::m_NumNodes += 4;
	}
}

bool QuadTree::AddPoint(const Vector3f &_pos, int _data)
{
	if(m_WorldAABB.Contains(_pos))
	{
		if(m_NorthE && m_NorthE->AddPoint(_pos, _data))
			return true;
		if(m_SouthE && m_SouthE->AddPoint(_pos, _data))
			return true;
		if(m_NorthW && m_NorthW->AddPoint(_pos, _data))
			return true;
		if(m_SouthW && m_SouthW->AddPoint(_pos, _data))
			return true;

		QuadTreeData data = { _pos, _data };
		m_Points.push_back(data);
		return true;
	}
	return false;
}

//bool QuadTree::IsEmpty() const
//{
//	if(m_NorthE && !m_NorthE->IsEmpty())
//		return false;
//	if(m_SouthE && !m_SouthE->IsEmpty())
//		return false;
//	if(m_NorthW && !m_NorthW->IsEmpty())
//		return false;
//	if(m_SouthW && !m_SouthW->IsEmpty())
//		return false;
//
//	return m_Points.empty();
//}

bool QuadTree::ClosestPtSq(const Vector3f &_pos, QuadTreeData &_outPt, float &_outDist)
{
	if(m_NorthE && m_NorthE->m_WorldAABB.Contains(_pos) && m_NorthE->ClosestPtSq(_pos, _outPt, _outDist))
		return true;
	if(m_SouthE && m_SouthE->m_WorldAABB.Contains(_pos) && m_SouthE->ClosestPtSq(_pos, _outPt, _outDist))
		return true;
	if(m_NorthW && m_NorthW->m_WorldAABB.Contains(_pos) && m_NorthW->ClosestPtSq(_pos, _outPt, _outDist))
		return true;
	if(m_SouthW && m_SouthW->m_WorldAABB.Contains(_pos) && m_SouthW->ClosestPtSq(_pos, _outPt, _outDist))
		return true;

	if(!m_Points.empty())
	{
		QuadTreeData vClosestData = m_Points[0];
		float fClosestPtDist = (vClosestData.m_Point - _pos).SquaredLength();
		for(obuint32 i = 1; i < m_Points.size(); ++i)
		{
			float fSqLen = (m_Points[i].m_Point - _pos).SquaredLength();
			if(fSqLen < fClosestPtDist)
			{
				vClosestData = m_Points[i];
				fClosestPtDist = fSqLen;
			}
		}

		_outPt = vClosestData;
		_outDist = fClosestPtDist;

		return true;
	}
	
	return false;
}

void QuadTree::Render(obColor _color) const
{
	Utils::OutlineAABB(m_WorldAABB, _color, 0.0f);

	if(m_NorthE)
		m_NorthE->Render(_color);
	if(m_SouthE)
		m_SouthE->Render(_color);
	if(m_NorthW)
		m_NorthW->Render(_color);
	if(m_SouthW)
		m_SouthW->Render(_color);
}

void QuadTree::Clear()
{
	if(m_NorthE)
		m_NorthE->Clear();
	if(m_SouthE)
		m_SouthE->Clear();
	if(m_NorthW)
		m_NorthW->Clear();
	if(m_SouthW)
		m_SouthW->Clear();

	m_Points.clear();
}

bool QuadTree::Within(const Vector3f &_pos) const
{
	return m_WorldAABB.Contains(_pos);
}
