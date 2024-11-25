#ifndef __ASTARSOLVER_H__
#define __ASTARSOLVER_H__

#include <boost/pool/pool_alloc.hpp>

template<typename Node>
class AStar
{
public:
	typedef bool (*pfnIsGoal)(const Node &aNode);
	typedef int (*pfnExpandFromNode)(const Node &aNode, Node *aNodeOut, int aMaxNodeOut);

	void BeginSearch(const Node *aStart, int aNumStarts, const Node *aEnd, int aNumEnds)
	{
		m_OpenList.resize(0);
		m_ClosedList.clear();

		// Mark all the goal nodes.
		for(int i = 0; i < aNumEnds; ++i)
		{

		}
	}

	bool IsFinished()
	{
		return false;
	}

	void Iterate()
	{
		while(!m_OpenList.empty())
		{
			// get the next node
			NavNode *pCurNode = m_OpenList.front();
			std::pop_heap(m_OpenList.begin(), m_OpenList.end(), NAV_COMP);
			m_OpenList.pop_back();

			// Push it on the list so it doesn't get considered again.
			m_ClosedList[HashNavNode(pCurNode)] = pCurNode;

			// Is it the goal?

			// Expand the node
		}
	}

	AStar() {}
protected:
private:

	struct NavNode
	{
		float		m_GivenCost;
		float		m_FinalCost;
		float		m_HeuristicCost;
		NavNode		*m_Parent;

		Node		m_NavData;
	};

	static bool NAV_COMP(const NavNode* _n1, const NavNode* _n2)
	{
		return _n1->m_FinalCost > _n2->m_FinalCost;
	}
	static int HashNavNode(const NavNode *_n1)
	{
		return 0;
	}

	typedef boost::fast_pool_allocator< std::pair< const int, NavNode* >, boost::default_user_allocator_new_delete, boost::details::pool::default_mutex, 769 > HashMapAllocator;
	typedef stdext::hash<int> HashMapCompare;
	typedef stdext::unordered_map<int, NavNode*, HashMapCompare, stdext::equal_to<int>, HashMapAllocator > NavHashMap;

	typedef std::vector<NavNode> NodeList;

	NodeList		m_OpenList;
	NavHashMap		m_ClosedList;

};

#endif
