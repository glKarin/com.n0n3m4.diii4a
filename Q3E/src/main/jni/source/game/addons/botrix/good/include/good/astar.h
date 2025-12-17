//----------------------------------------------------------------------------------------------------------------
// A* search algorithm on graph.
// Copyright (c) 2012 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_ASTAR_H__
#define __GOOD_ASTAR_H__


#include "good/graph.h"
#include "good/priority_queue.h"


namespace good
{


    //************************************************************************************************************
    /// Class for finding minimal path between 2 vertices of a graph.
    // Implementation: for a graph we will have an same size array of nodes that will have closed flag,
    // father of this node, and costs (global and heuristic). This implementation is faster than to
    // have closed set + maps of node to nodes  for path recovery + maps of heuristic and global costs
    // (check http://en.wikipedia.org/wiki/A*_search_algorithm for details).
    //************************************************************************************************************
    template <
        typename Vertex,                                                  ///< Type representing vertex of a graph node.
        typename Edge,                                                    ///< Edge can be any type, but you need to provide EdgeLength.
        typename EdgeLengthType,                                          ///< Type of length of Edge (must be numerical obviously).
        typename Heuristic,                                               ///< Heuristic functor, 2 vertices as argument.
        typename EdgeLength,                                              ///< Edge length functor, Edge as argument.
        typename CanUse,                                                  ///< Functor to know if can use certain node in search. Vertex as argument.
        template <typename, typename> class NodesContainer = good::vector, ///< Container for arcs of a graph node.
        template <typename, typename> class ArcsContainer = good::vector,  ///< Container for graph nodes.
        typename Alloc = good::allocator<Vertex>                           ///< Memory allocator.
    >
    class astar
    {

    public:
        /// Graph type that will be used to find path.
        typedef graph<Vertex, Edge, NodesContainer, ArcsContainer, Alloc> graph_t;  ///< Type for graph.
        typedef typename graph_t::node_t node_t;                                    ///< Type for graph node.

        typedef typename graph_t::node_it node_it;                                  ///< Type for node iterator.
        typedef typename graph_t::const_node_it const_node_it;                      ///< Type for const node iterator.

        typedef typename graph_t::arc_it arc_it;                                    ///< Type for arc iterator.
        typedef typename graph_t::const_arc_it const_arc_it;                        ///< Type for const arc iterator.

        typedef typename graph_t::node_id node_id;                                  ///< Index of graph node in nodes array.

        typedef good::vector<node_id, Alloc> path_t;                                ///< Type for path of nodes.

    protected:

        //========================================================================================================
        /// A* node contains vertex and list of links to adjacent nodes.
        //========================================================================================================
        struct astar_node_t
        {
            /// A* node constructor.
            astar_node_t(): g_score(0), h_score(0), prev(-1), visited(0) {}

            EdgeLengthType g_score;   ///< Distance from source to this node.
            EdgeLengthType h_score;   ///< Heuristic estimation to target from this node.
            node_id prev;             ///< Previous node that forms a path.
            int visited;              ///< Count of previous nodes, forming path, 0 if not visited.
        };

        typedef typename Alloc::template rebind<astar_node_t>::other alloc_anode_t; ///< Type for allocator for A* node.
        typedef good::vector< astar_node_t, alloc_anode_t > anode_container_t;      ///< Type for container of A* nodes.

        typedef typename anode_container_t::iterator anode_it;                      ///< Type for A* node iterator.
        typedef typename anode_container_t::const_iterator const_anode_it;          ///< Type for A* node const iterator.



    public: // Methods.
        //--------------------------------------------------------------------------------------------------------
        /// Constructor.
        //--------------------------------------------------------------------------------------------------------
        astar(): m_hFunc(), m_cArcLength(), m_pQueue(), m_path(), m_nTarget(-1), m_bFound(false) {}


        //--------------------------------------------------------------------------------------------------------
        /// Set graph for searches.
        //--------------------------------------------------------------------------------------------------------
        void set_graph( const graph_t& rGraph, int iReserveSize = 0 )
        {
            m_cANodes.clear();
            if ( iReserveSize > rGraph.size() )
                m_cANodes.reserve( iReserveSize );
            m_cANodes.resize( rGraph.size() );
            m_pGraph = &rGraph;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Start searching path from nFrom to nTo. Return true if search finishes (either success or failure).
        ///
        /// If iMaxNodes parameter is not 0, then search will check only iMaxNodes nodes, exiting function.
        /// You will need to call manually step() function to resume search.
        //--------------------------------------------------------------------------------------------------------
        void setup_search( node_id nFrom, node_id nTo, CanUse const& cCanUse, int iMaxNodes = 0 )
        {
            _clean();
            m_cANodes.resize( m_pGraph->size() );

            int size = (int)m_pGraph->size();
            for (node_id id = 0; id < size; ++id)
                if ( !cCanUse( m_pGraph->at(id) ) )
                    m_cANodes[id].visited = 1; // Force nodes that can't use to visited.

            m_nTarget = nTo;
            m_iMaxNodes = iMaxNodes > 0 ? iMaxNodes : MAX_INT32;

            anode_it aFrom = m_cANodes.begin() + nFrom;

            aFrom->g_score = 0;
            //nFrom->h_score = m_hFunc(nFrom->vertex, nTo->vertex); // No need for this, will be top of queue anyway.
            aFrom->prev = graph_t::invalid_node_id;

            m_pQueue.push(aFrom);
        }


        //--------------------------------------------------------------------------------------------------------
        /// Will continue with search, only iMaxNodes nodes trying to reach target.
        //--------------------------------------------------------------------------------------------------------
        bool step()
        {
            if ( m_pGraph->size() > m_cANodes.size() )
            {
                if ( m_cANodes.capacity() >= m_pGraph->size() )
                    m_cANodes.resize( m_pGraph->size() );
                else
                {
                    // We can't resize m_cANodes as this will realloc to new memory (and m_pQueue become invalid,
                    // as it contains pointers to m_cANodes). We will just return false, as if there is no path.
                    m_pQueue.clear();
                    return true;
                }
            }

            for (m_iCurrLoop = 0; !m_pQueue.empty() && (m_iCurrLoop < m_iMaxNodes); ++m_iCurrLoop)
            {
                anode_it current = m_pQueue.top();
                m_pQueue.pop();

                node_id curr_id = current - m_cANodes.begin();
                const_node_it curr_node = m_pGraph->begin() + curr_id;

                node_id prev_id = current->prev;
                current->visited = prev_id == graph_t::invalid_node_id ? 1 : m_cANodes[prev_id].visited+1;

                if (curr_id == m_nTarget)
                {
                    m_bFound = true;
                    _get_path();
                    return true;
                }

                for (const_arc_it arcIt = curr_node->neighbours.begin(); arcIt != curr_node->neighbours.end(); ++arcIt)
                {
                    node_id neighbour_id = arcIt->target;
                    const_node_it neighbour_node = m_pGraph->begin() + neighbour_id;
                    anode_it neighbour = m_cANodes.begin() + neighbour_id;
                    if ( neighbour->visited )
                        continue;

                    EdgeLengthType g_score = current->g_score + m_cArcLength(arcIt->edge);

                    if (neighbour->prev == graph_t::invalid_node_id)
                    {
                        // First time visiting neighbour.
                        neighbour->prev = curr_id;
                        neighbour->g_score = g_score;
                        neighbour->h_score = m_hFunc(neighbour_node->vertex, neighbour_node->vertex);
                        m_pQueue.push(neighbour);
                    }
                    else
                    {
                        // There is a path already from source to neigbour, check this one is better.
                        if (g_score <= neighbour->g_score) // Choose path with lesser distance.
                        {
                            neighbour->prev = curr_id;
                            neighbour->g_score = g_score;
                            m_pQueue.modify(neighbour);
                        }
                    }
                }
            }
            return m_pQueue.empty();
        }


        //--------------------------------------------------------------------------------------------------------
        /// Return true if there is a path from source to target.
        //--------------------------------------------------------------------------------------------------------
        bool has_path() const { return m_bFound; }


        //--------------------------------------------------------------------------------------------------------
        /// Return founded path.
        //--------------------------------------------------------------------------------------------------------
        path_t& path()
        {
            GoodAssert(m_bFound);
            return m_path;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Return founded path.
        //--------------------------------------------------------------------------------------------------------
        const path_t& path() const
        {
            GoodAssert(m_bFound);
            return m_path;
        }


    protected: // Methods.
        //--------------------------------------------------------------------------------------------------------
        void _clean()
        {
            for (anode_it it = m_cANodes.begin(); it != m_cANodes.end(); ++it)
            {
                it->prev = graph_t::invalid_node_id;
                it->visited = 0;
            }
            m_path.clear();
            m_pQueue.clear();
            m_bFound = false;
        }

        //--------------------------------------------------------------------------------------------------------
        void _get_path()
        {
            int total = m_cANodes[m_nTarget].visited;
            m_path.resize(total);
            for (node_id id = m_nTarget; id != graph_t::invalid_node_id; id = m_cANodes[id].prev)
                m_path[--total] = id;
        }

        //************************************************************************************************************
        /// Less class operator for A* node.
        //************************************************************************************************************
        class anode_less
        {
        public:
            /**
             * @brief Operator to know order of left and right node, return true if left > right.
             * So this is not "less" but "more" operator, because queue top is max element (max priority queue),
             * when actually we need min element, so we change order in this operation.
             */
            bool operator()(anode_it const& left, anode_it const& right)
            {
                return (right->g_score + right->h_score) < (left->g_score + left->h_score);
            }
        };


    protected: // Members.
        typedef typename Alloc::template rebind<anode_it>::other alloc_anode_it_t; // Type for allocator of astar node iterator.
        typedef priority_queue< anode_it, anode_less, alloc_anode_it_t > queue_t;  // Type for queue of pointer to nodes.

        Heuristic m_hFunc;                     // Heuristic function (approx. distance from node to node).
        EdgeLength m_cArcLength;               // Function to get arc length.

        anode_container_t m_cANodes;           // Container of nodes.
        const graph_t* m_pGraph;               // Search graph.

        queue_t m_pQueue;                      // Priority queue for A* search algorithn.
        path_t m_path;                         // Founded path.
        node_id m_nTarget;                     // Target of search.
        int m_iMaxNodes, m_iCurrLoop;          // Max iterations for one step of A* algorithm.
        bool m_bFound;                         // True if search is succesfull.
    };


} // namespace good

#endif // __GOOD_ASTAR_H__
