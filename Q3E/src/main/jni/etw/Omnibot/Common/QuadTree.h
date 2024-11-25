#ifndef __QUADTREE_H__
#define __QUADTREE_H__

class QuadTree
{
public:

	struct QuadTreeData
	{
		Vector3f	m_Point;
		int			m_UserData;
	};
	typedef std::vector<QuadTreeData> DataList;

	struct QuadTreeNode;
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
	typedef std::shared_ptr<QuadTree> QuadTreeNodePtr;
#else
	typedef boost::shared_ptr<QuadTree> QuadTreeNodePtr;
#endif

	void Render(obColor _color) const;

	bool AddPoint(const Vector3f &_pos, int _data);

	//bool IsEmpty() const;

	bool ClosestPtSq(const Vector3f &_pos, QuadTreeData &_outPt, float &_outDist);

	bool Within(const Vector3f &_pos) const;

	void Split(float _minNodeSize);

	void Clear();

	QuadTree(const AABB &_aabb, float _minNodeSize);
	QuadTree() {}
	~QuadTree();
private:
	AABB			m_WorldAABB;
	QuadTreeNodePtr	m_NorthE;
	QuadTreeNodePtr	m_SouthE;
	QuadTreeNodePtr	m_NorthW;
	QuadTreeNodePtr	m_SouthW;

	DataList		m_Points;

	static int m_QuadDepth;
	static int m_NumNodes;
};

#endif
