//----------------------------------------------------------------------------------------------------------------
// Directed graph implementation based on node's adjacency list.
// Copyright (c) 2012 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_GRAPH_H__
#define __GOOD_GRAPH_H__


#include "good/vector.h"


namespace good
{


    //************************************************************************************************************
    /// Class that represents directional graph.
    //************************************************************************************************************
    template <
        typename Vertex,                                                  ///< Vertex can be any type.
        typename Edge,                                                    ///< Edge can be any type.
        template <typename, typename> class NodesContainer = good::vector, ///< Random access for fast operations.
        template <typename, typename> class ArcsContainer = good::vector,  ///< Any container class.
        typename Alloc = good::allocator<Vertex>                           ///< Memory allocator.
    >
    class graph
    {


    public:
        class node_t;                              // Forward declaration.

        typedef int node_id;                       ///< Type for node id (it is node index in nodes array).
        static const node_id invalid_node_id = -1; ///< Invalid node index.

        //========================================================================================================
        /// Node contains vertex and list of links to adjacent nodes.
        //========================================================================================================
        class arc_t
        {
        public:
            Edge edge; ///< Edge of arc.
            /// Target for arc (index in array of nodes, it is better than pointer/iterator in case of realloc).
            node_id target;

            /// Arc constructor.
            arc_t( Edge const& e, int t ): edge(e), target(t) {}
        };


        //========================================================================================================
        /// Node contains vertex and list of links to adjacent nodes.
        //========================================================================================================
        class node_t
        {
        public:
            typedef typename Alloc::template rebind<arc_t>::other alloc_t; ///< Type to allocate arcs of a node.
            typedef ArcsContainer<arc_t, alloc_t> arcs_t;                  ///< Container of outgoing arcs of a node.

            /// Node default constructor.
            node_t(): vertex(), neighbours(0) {}
            /// Node constructor.
            node_t( Vertex const& v, int iNeighboursSize = 0 ): vertex(v), neighbours()
            {
                if ( iNeighboursSize )
                    neighbours.reserve(iNeighboursSize);
            }

            /// Find arc from this node to given target node.
            typename arcs_t::iterator find_arc_to( node_id target )
            {
                for (arc_it arcIt = neighbours.begin(); arcIt != neighbours.end(); ++arcIt)
                    if (arcIt->target == target)
                        return arcIt;
                return neighbours.end();
            }

            /// Add arc with edge.
            typename arcs_t::iterator add_arc_to( node_id target, Edge const& e )
            {
                // Make sure that target belongs to graph.
#if defined(DEBUG) || defined(_DEBUG)
                GoodAssert( g->is_valid(target) );
#endif
                return neighbours.insert( neighbours.end(), arc_t(e, target) );
            }

            Vertex vertex;     ///< Vertex of this node.
            arcs_t neighbours; ///< Container of arcs going to adjacent nodes for this node.

#if defined(DEBUG) || defined(_DEBUG)
            graph* g;
#endif
        };


    public:
        typedef typename Alloc::template rebind<node_t>::other alloc_t;               ///< Allocator for graph nodes.
        typedef NodesContainer< node_t, alloc_t > node_container_t;                   ///< Container of graph nodes.

        typedef typename node_container_t::iterator node_it;                          ///< Node iterator.
        typedef typename node_container_t::const_iterator const_node_it;              ///< Node const iterator.

        typedef typename node_t::arcs_t::iterator arc_it;                             ///< Arcs iterator.
        typedef typename node_t::arcs_t::const_iterator const_arc_it;                 ///< Arcs const iterator.

        typedef typename node_t::arcs_t::reverse_iterator reverse_arc_it;             ///< Arcs iterator.
        typedef typename node_t::arcs_t::const_reverse_iterator const_reverse_arc_it; ///< Arcs const iterator.


        //--------------------------------------------------------------------------------------------------------
        /// Default constructor.
        //--------------------------------------------------------------------------------------------------------
        graph( int iSize = 0 ): m_cNodes() { m_cNodes.reserve(iSize); }

        //--------------------------------------------------------------------------------------------------------
        /// Count of nodes in this graph.
        //--------------------------------------------------------------------------------------------------------
        int size() const { return m_cNodes.size(); }

        //--------------------------------------------------------------------------------------------------------
        /// Array subscript.
        //--------------------------------------------------------------------------------------------------------
        node_t& operator [](node_id iNodeId) { return m_cNodes[iNodeId]; }

        //--------------------------------------------------------------------------------------------------------
        /// Array subscript const.
        //--------------------------------------------------------------------------------------------------------
        node_t const& operator [](node_id iNodeId) const { return m_cNodes[iNodeId]; }

        //--------------------------------------------------------------------------------------------------------
        /// Element at index.
        //--------------------------------------------------------------------------------------------------------
        node_t& at(node_id iNodeId) { return m_cNodes[iNodeId]; }

        //--------------------------------------------------------------------------------------------------------
        /// Element at index.
        //--------------------------------------------------------------------------------------------------------
        node_t const& at(node_id iNodeId) const { return m_cNodes[iNodeId]; }

        //--------------------------------------------------------------------------------------------------------
        /// Random access for nodes.
        //--------------------------------------------------------------------------------------------------------
        bool is_valid(node_id iNodeId) { return (0 <= iNodeId) && (iNodeId < (int)m_cNodes.size()); }

        //--------------------------------------------------------------------------------------------------------
        /// Add node.
        //--------------------------------------------------------------------------------------------------------
        node_it add_node( Vertex const& vertex, int iNeighboursSize = 0 )
        {
            node_t node( vertex, iNeighboursSize );
#if defined(DEBUG) || defined(_DEBUG)
            node.g = this;
#endif
            return m_cNodes.insert(m_cNodes.end(), node);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Delete node and all arcs that are going to this node.
        //--------------------------------------------------------------------------------------------------------
        node_it delete_node( node_it it )
        {
            node_id node = it - begin();
            for (node_it nodeIt = m_cNodes.begin(); nodeIt != m_cNodes.end(); nodeIt++ )
            {
                arc_it arcIt = nodeIt->neighbours.begin();
                while ( arcIt != nodeIt->neighbours.end() )
                {
                    if (arcIt->target == node)
                        arcIt = nodeIt->neighbours.erase( arcIt );
                    else
                    {
                        if (arcIt->target > node)
                            arcIt->target--;
                        arcIt++;
                    }
                }
            }

            return m_cNodes.erase(it);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Add arc between node ids.
        //--------------------------------------------------------------------------------------------------------
        arc_it add_arc( node_id from, node_id to, Edge const& edge )
        {
            return m_cNodes[from].add_arc_to(to, edge);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Add arc between nodes iterators.
        //--------------------------------------------------------------------------------------------------------
        arc_it add_arc( node_it from, node_it to, Edge const& edge )
        {
            return from->add_arc_to(to - begin(), edge);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Delete arc.
        //--------------------------------------------------------------------------------------------------------
        arc_it delete_arc( node_it from, node_it to )
        {
            node_id idTo = to - begin();
            for (arc_it arcIt = from->neighbours.begin(); arcIt != from->neighbours.end(); arcIt++)
                if (arcIt->target == idTo)
                    return from->neighbours.erase(arcIt);
            return from->neighbours.end();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Delete arc.
        //--------------------------------------------------------------------------------------------------------
        arc_it delete_arc( node_it from, arc_it arc )
        {
            return from->neighbours.erase(arc);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Reserve.
        //--------------------------------------------------------------------------------------------------------
        void reserve(int iCapacity) { m_cNodes.reserve(iCapacity); }

        //--------------------------------------------------------------------------------------------------------
        /// Resize.
        //--------------------------------------------------------------------------------------------------------
        void resize(int iSize) { m_cNodes.resize(iSize); }

        //--------------------------------------------------------------------------------------------------------
        /// Remove all nodes.
        //--------------------------------------------------------------------------------------------------------
        void clear() { m_cNodes.clear(); }

        //--------------------------------------------------------------------------------------------------------
        /// Iterator to first node.
        //--------------------------------------------------------------------------------------------------------
        node_it begin() { return m_cNodes.begin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Iterator to one position after last node.
        //--------------------------------------------------------------------------------------------------------
        node_it end() { return m_cNodes.end(); }

        //--------------------------------------------------------------------------------------------------------
        /// Reverse iterator to last node.
        //--------------------------------------------------------------------------------------------------------
        node_it rbegin() { return m_cNodes.rbegin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Reverse iterator to one position before first node.
        //--------------------------------------------------------------------------------------------------------
        node_it rend() { return m_cNodes.rend(); }

        //--------------------------------------------------------------------------------------------------------
        /// Const iterator to first node.
        //--------------------------------------------------------------------------------------------------------
        const_node_it begin() const { return m_cNodes.begin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Const iterator to one position after last node.
        //--------------------------------------------------------------------------------------------------------
        const_node_it end() const { return m_cNodes.end(); }

        //--------------------------------------------------------------------------------------------------------
        /// Const reverse iterator to last node.
        //--------------------------------------------------------------------------------------------------------
        const_node_it rbegin() const { return m_cNodes.rbegin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Const reverse iterator to one position before first node.
        //--------------------------------------------------------------------------------------------------------
        const_node_it rend() const { return m_cNodes.rend(); }

    protected:
        node_container_t m_cNodes;
    };


} // namespace good


#endif // __GOOD_GRAPH_H__
