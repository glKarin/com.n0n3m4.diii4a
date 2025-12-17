//----------------------------------------------------------------------------------------------------------------
// Andersson tree(AA balanced) optimized implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_AATREE_H__
#define __GOOD_AATREE_H__


#include "good/defines.h"


//#define DEBUG_TREE_ITERATOR // Define it to show paths when incrementing iterator.

#ifdef DEBUG_TREE_ITERATOR
#   define AATreeDebugPrint        DebugPrint
#else
#   define AATreeDebugPrint(...)
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

namespace good
{

    //************************************************************************************************************
    /// Node of a tree.
    //************************************************************************************************************
    template <typename T>
    struct aatree_node
    {
        unsigned char level;   ///< Node level.
        T elem;                ///< Node element.
        aatree_node* parent;   ///< Parent of current node. We need it to be able to iterate on tree.
        aatree_node* child[2]; ///< Right child is child[0], left child is child[1].
    };


    //************************************************************************************************************
    /// Binary search tree, Arne Andersson tree implementation.
    // Note that an optimization has been used:
    //   if (condition) x = y else x = z
    // becomes:
    //   array choice[2] = { z, y };
    //   int direction = condition;
    //   x = choice[direction];
    // So if condition is true == 1, then x = choice[1] = y
    // And if condition is false == 0, then x = choice[0] = z
    //************************************************************************************************************
    template <
        typename T,                          ///< Type to store in tree.
        typename Less = good::less<T>,       ///< Operation to know order of elements.
        typename Alloc = good::allocator<T>  ///< Allocator for T.
    >
    class aatree
    {
    public:
        typedef aatree_node<T> node_t;

        //========================================================================================================
        /// Const iterator of tree.
        //========================================================================================================
        class const_iterator
        {
        public:
            friend class aatree<T, Less, Alloc>;

            // Constructor by value.
            const_iterator ( node_t* n = NULL ): m_pCurrent(n) {}
            // Copy constructor.
            const_iterator ( const_iterator const& itOther ): m_pCurrent(itOther.m_pCurrent) {}

            /// Operator ==.
            bool operator== ( const const_iterator& itOther ) const { return m_pCurrent == itOther.m_pCurrent; }

            /// Operator !=.
            bool operator!= ( const const_iterator& itOther ) const { return m_pCurrent != itOther.m_pCurrent; }

            /// Pre-increment.
            const_iterator& operator++() { GoodAssert(m_pCurrent); m_pCurrent = _get_next(m_pCurrent, 1); return *this; }
            /// Pre-decrement.
            const_iterator& operator--() { GoodAssert(m_pCurrent); m_pCurrent = _get_next(m_pCurrent, 0); return *this; }

            /// Post-increment.
            const_iterator operator++ (int) { GoodAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent = _get_next(m_pCurrent, 1); return tmp; }
            /// Post-decrement.
            const_iterator operator-- (int) { GoodAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent = _get_next(m_pCurrent, 0); return tmp; }

            /// Dereference.
            const T& operator*() const { GoodAssert(m_pCurrent); return m_pCurrent->elem; }
            /// Element selection through pointer.
            const T* operator->() const { GoodAssert(m_pCurrent); return &m_pCurrent->elem; }

        protected:
            node_t* m_pCurrent;
        };


        //========================================================================================================
        /// Iterator of tree.
        //========================================================================================================
        class iterator: public const_iterator
        {
        public:
            typedef const_iterator base_class;

            // Constructor by value.
            iterator ( node_t* n = NULL ): base_class(n) {}
            // Copy constructor.
            iterator ( iterator const& lOther ): base_class(lOther) {}

            /// Pre-increment.
            iterator& operator++() { GoodAssert(this->m_pCurrent); this->m_pCurrent = _get_next(this->m_pCurrent, 1); return *this; }
            /// Pre-decrement.
            iterator& operator--() { GoodAssert(this->m_pCurrent); this->m_pCurrent = _get_next(this->m_pCurrent, 0); return *this; }

            /// Post-increment.
            iterator operator++(int) { GoodAssert(this->m_pCurrent); iterator tmp(*this); this->m_pCurrent = _get_next(this->m_pCurrent, 1); return tmp; }
            /// Post-decrement.
            iterator operator--(int) { GoodAssert(this->m_pCurrent); iterator tmp(*this); this->m_pCurrent = _get_next(this->m_pCurrent, 0); return tmp; }

            /// Dereference.
            T& operator*() const { GoodAssert(this->m_pCurrent); return this->m_pCurrent->elem; }
            /// Element selection through pointer.
            T* operator->() const { GoodAssert(this->m_pCurrent); return &this->m_pCurrent->elem; }
        };


        //--------------------------------------------------------------------------------------------------------
        /// Tree constructor.
        //--------------------------------------------------------------------------------------------------------
        aatree()
        {
            _init();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Tree copy constructor.
        //--------------------------------------------------------------------------------------------------------
        aatree( aatree const& tOther )
        {
            _init();
            assign(tOther);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Destructor.
        //--------------------------------------------------------------------------------------------------------
        ~aatree()
        {
            _erase_subtree(m_pHead->parent);
            m_cAllocNode.deallocate(m_pHead, 1);
            m_cAllocNode.deallocate(nil, 1);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Note that this operator moves content, not copies it.
        //--------------------------------------------------------------------------------------------------------
        aatree& operator=( aatree const& tOther )
        {
            assign(tOther);
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Move other tree contents to this tree.
        //--------------------------------------------------------------------------------------------------------
        void assign(aatree const& tOther)
        {
            clear();
            good::swap(nil, ((aatree&)tOther).nil);
            good::swap(m_pHead, ((aatree&)tOther).m_pHead);
            good::swap(m_iSize, ((aatree&)tOther).m_iSize);
        }


        //--------------------------------------------------------------------------------------------------------
        /// Return size of tree.
        //--------------------------------------------------------------------------------------------------------
        int size() const { return m_iSize; }

        //--------------------------------------------------------------------------------------------------------
        /// Return true if the tree is empty.
        //--------------------------------------------------------------------------------------------------------
        bool empty() const { return m_iSize == 0; }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to first (smallest) element of the tree.
        //--------------------------------------------------------------------------------------------------------
        const_iterator begin() const { return const_iterator(m_pHead->child[0]); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to first (smallest) element of the tree.
        //--------------------------------------------------------------------------------------------------------
        iterator begin() { return iterator(m_pHead->child[0]); }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to end of tree.
        //--------------------------------------------------------------------------------------------------------
        const_iterator end() const { return const_iterator(nil); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to end of tree.
        //--------------------------------------------------------------------------------------------------------
        iterator end() { return iterator(nil); }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to last (largest) element of the tree.
        //--------------------------------------------------------------------------------------------------------
        const_iterator rbegin() const { return const_iterator(m_pHead->child[1]); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to last (largest) element of the tree.
        //--------------------------------------------------------------------------------------------------------
        iterator rbegin() { return iterator(m_pHead->child[1]); }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to end of tree.
        //--------------------------------------------------------------------------------------------------------
        const_iterator rend() const { return const_iterator(nil); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to end of tree.
        //--------------------------------------------------------------------------------------------------------
        iterator rend() { return iterator(nil); }

        //--------------------------------------------------------------------------------------------------------
        /// Remove all elements.
        //--------------------------------------------------------------------------------------------------------
        void clear()
        {
            _erase_subtree(m_pHead->parent);
            m_pHead->parent = m_pHead->child[0] = m_pHead->child[1] = nil;
            m_iSize = 0;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Find an element in a tree (const). Return end() if not found.
        //--------------------------------------------------------------------------------------------------------
        const_iterator find( const T& tData ) const
        {
            node_t* succ;
            int dir;
            return const_iterator( _search( m_pHead->parent, tData, succ, dir ) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Find an element in a tree. Return end() if not found.
        //--------------------------------------------------------------------------------------------------------
        iterator find( const T& tData )
        {
            node_t* succ;
            int dir;
            return iterator( _search( m_pHead->parent, tData, succ, dir ) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Insert element to tree. If element already exists and bReplace then element will be replaced.
        //--------------------------------------------------------------------------------------------------------
        iterator insert( const T& tData, bool bReplace = true ) { return iterator( _insert(tData, bReplace) ); }

        //--------------------------------------------------------------------------------------------------------
        /// Erase element. Return iterator to next element.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( const T& tData )
        {
            node_t* succ;
            int succDir;
            node_t* cand = _search( m_pHead->parent, tData, succ, succDir );

            if ( _is_nil(cand) )
                return iterator(nil);

            // Found tData.
            return iterator( _erase( cand, cand->parent->child[1] == cand, succ, succDir ) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Erase iterator. Return iterator to next element.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( iterator itElem )
        {
            node_t* node = itElem.m_pCurrent;

            if ( _is_nil(node) )
                return iterator(node);

            node_t* succ = _get_leaf( node->child[1], 0 );
            return iterator( _erase( node, node->parent->child[1] == node, succ, succ->parent->child[1] == succ ) );
        }



    protected: // Methods.
        //--------------------------------------------------------------------------------------------------------
        // Init tree.
        //--------------------------------------------------------------------------------------------------------
        void _init()
        {
            m_pHead = m_cAllocNode.allocate(1);
            m_pHead->child[0] = m_pHead->child[1] = m_pHead->parent = nil = m_cAllocNode.allocate(1);
            nil->child[0] = nil->child[1] = nil;
            nil->level = 0;
            nil->parent = m_pHead;
            m_iSize = 0;
        }

        //--------------------------------------------------------------------------------------------------------
        // Fast way to know if node is nil.
        //--------------------------------------------------------------------------------------------------------
        static bool _is_nil( node_t* n )
        {
            return n->level == 0;
        }

        //--------------------------------------------------------------------------------------------------------
        // Get smallest node if dir == 0 or largest one if dir == 1.
        //--------------------------------------------------------------------------------------------------------
        static node_t* _get_leaf( node_t* n, const int dir )
        {
            while ( !_is_nil(n->child[dir]) )
            {
#ifdef DEBUG_TREE_ITERATOR
                AATreeDebugPrint("%s", dir?"\\":"/");
#endif
                n = n->child[dir];
            }
            return n;
        }

        //--------------------------------------------------------------------------------------------------------
        // Get first right (iDir = 1) parent or first left (iDir = 0) parent.
        //--------------------------------------------------------------------------------------------------------
        static node_t* _get_parent( node_t* n, const int iDir )
        {
            node_t* nParent = n->parent;
            while ( !_is_nil(nParent) )
            {
                int dir = nParent->child[1] == n;
                if ( dir != iDir) // Child direction is opposite of parent direction.
                    return nParent;

                n = nParent;
                nParent = n->parent;
            }
            return nParent;
        }

        //--------------------------------------------------------------------------------------------------------
        // Get node with next/previous value of element.
        //--------------------------------------------------------------------------------------------------------
        static node_t* _get_next( node_t* n, const int dir )
        {
#ifdef DEBUG_TREE_ITERATOR
            AATreeDebugPrint(".%d", (int)n->level);
#endif
            if (_is_nil(n))
            {
                // n->parent points to m_pHead.
                node_t* aux[2] = { n->parent->child[1], n->parent->child[0] };
                return aux[dir];
            }
            else
            {
                node_t* aux = n->child[dir];
                if ( !_is_nil(aux) )
                {
#ifdef DEBUG_TREE_ITERATOR
                    AATreeDebugPrint("%s", dir?"\\":"/");
#endif
                    return _get_leaf( aux, !dir ); // Get smallest/largest of the right/left subtree.
                }
                else
                {
#ifdef DEBUG_TREE_ITERATOR
                    AATreeDebugPrint("^");
#endif
                    // Climb looking for first available right/left subtree.
                    while ( (!_is_nil(aux = n->parent)) && (aux->child[dir] == n) )
                    {
#ifdef DEBUG_TREE_ITERATOR
                        AATreeDebugPrint("^");
#endif
                        n = aux;
                    }
                    return aux;
                }
            }
        }

        //--------------------------------------------------------------------------------------------------------
        // Create one node. Leafs have level == 1, and nil has level == 0.
        //--------------------------------------------------------------------------------------------------------
        node_t* _make_node( const T& tData, node_t* nParent, unsigned char iLevel = 1 )
        {
            node_t* node = m_cAllocNode.allocate(1);
            m_cAlloc.construct( &node->elem, tData );
            node->level = iLevel;
            node->parent = nParent;
            node->child[0] = node->child[1] = nil;
            return node;
        }

        //--------------------------------------------------------------------------------------------------------
        //  save <- tRoot       save -> tRoot
        //  ( )       )   =>   (       ( )
        // A   B       R      A       B   R
        //--------------------------------------------------------------------------------------------------------
        static node_t* _skew( node_t* nRoot )
        {
            if ( (nRoot->child[0]->level == nRoot->level) && !_is_nil(nRoot) )
            {
                node_t* save = nRoot->child[0];
                node_t* B = save->child[1];
                nRoot->child[0] = B; B->parent = nRoot;
                save->child[1] = nRoot;

                B = nRoot->parent; // Swap tRoot and save parents.
                nRoot->parent = save;
                save->parent = B;
                return save;
            }
            return nRoot;
        }

        //--------------------------------------------------------------------------------------------------------
        //   tRoot -> save -> X     save+1
        //  (       (              (  )
        // A       B        =>   tRoot  X
        //                       ( )
        //                      A   B
        //--------------------------------------------------------------------------------------------------------
        static node_t* _split( node_t* nRoot )
        {
            if ( (nRoot->child[1]->child[1]->level == nRoot->level) && !_is_nil(nRoot) )
            {
                node_t* save = nRoot->child[1];
                node_t* B = save->child[0];
                nRoot->child[1] = B; B->parent = nRoot;
                save->child[0] = nRoot;
                ++save->level;

                B = nRoot->parent; // Swap tRoot and save parents.
                nRoot->parent = save;
                save->parent = B;
                return save;
            }
            return nRoot;
        }

        //--------------------------------------------------------------------------------------------------------
        // Recursively erase entire subtree.
        //--------------------------------------------------------------------------------------------------------
        void _erase_subtree( node_t* nRoot )
        {
            if ( _is_nil(nRoot) ) return;
            _erase_subtree(nRoot->child[0]);
            _erase_subtree(nRoot->child[1]);
            m_cAlloc.destroy(&nRoot->elem);
            m_cAllocNode.deallocate(nRoot, 1);
        }

        //--------------------------------------------------------------------------------------------------------
        // Search for a node, returing lower bound (min of >= tData), and direction where to insert.
        //--------------------------------------------------------------------------------------------------------
        node_t* _search( node_t* tRoot, const T& tData, node_t*& pSucc, int& iDir ) const
        {
            if ( _is_nil(tRoot) )
                return nil;

            node_t *it = tRoot, *cand = nil, *cands[2];
            int dir;
            do
            {
                // Note optimization without if's.
                dir = m_cLess( it->elem, tData );  // dir = 1, when tData >= it.
                cands[0] = it;                     // Current element is bigger or equal than tData, save it.
                cands[1] = cand;                   // Current element is less than tData, skip it.
                cand = cands[dir];                 // Candidate node to insert to.
                it = it->child[dir];
            } while ( !_is_nil(it) );

            pSucc = cands[0]; // Return leaf where insert to.
            iDir = dir;

            // At this point we know that cand is smallest of the elements but bigger than tData (tData <= cand).
            if ( !_is_nil(cand) && !m_cLess(tData, cand->elem) ) // !(tData < cand) => (tData >= cand), so (tData == cand).
                return cand;
            else
                return nil;
        }

        //--------------------------------------------------------------------------------------------------------
        // Insert element into tree. If element found and bReplace then element will be replaced.
        //--------------------------------------------------------------------------------------------------------
        node_t* _insert( const T& tData, bool bReplace )
        {
            if ( m_iSize == 0 )
            {
                m_iSize++;
                return m_pHead->child[0] = m_pHead->child[1] = m_pHead->parent = _make_node( tData, nil, 1 ); // Leafs have level 1.
            }
            else
            {
                node_t* it;
                int dir;
                node_t* cand = _search( m_pHead->parent, tData, it, dir );

                // At this point we know that cand is smallest of the elements but bigger than tData (tData <= cand).
                if ( !_is_nil(cand) && !m_cLess(tData, cand->elem) ) // !(tData < cand) => (tData >= cand), so (tData == cand).
                {
                    if ( bReplace )
                        cand->elem = tData;
                }
                else
                {
                    // Insert it at it->child[dir], which is last visited node.
                    it->child[dir] = cand = _make_node( tData, it );

                    // Go back visiting parents making _skew and _split.
                    while ( !_is_nil(it->parent) )
                    {
                        node_t* father = it->parent;
                        dir = father->child[1] == it;
                        it = _skew( it );
                        it = _split( it );
                        father->child[dir] = it;
                        it = father;
                    }

                    m_pHead->parent = _skew( m_pHead->parent );
                    m_pHead->parent = _split( m_pHead->parent );

                    // Update m_pHead->child[0], m_pHead->child[1] elements.
                    if ( m_cLess(tData, m_pHead->child[0]->elem) )
                    {
                        m_pHead->child[0] = cand;
                    }
                    else if ( m_cLess(m_pHead->child[1]->elem, tData) )
                    {
                        m_pHead->child[1] = cand;
                    }
                    nil->parent = m_pHead;
                    m_iSize++;
                }
                return cand;
            }
        }

        //--------------------------------------------------------------------------------------------------------
        // Rebalance pNode after deleting one node.
        //--------------------------------------------------------------------------------------------------------
        node_t* _check_balance( node_t* pNode )
        {
            node_t* left = pNode->child[0], *right = pNode->child[1];
            if ( (pNode->level - 1 > left->level)  ||  (pNode->level - 1 > right->level) )
            {
                --pNode->level;
                if ( right->level > pNode->level )
                    right->level = pNode->level;

                pNode = _skew ( pNode );
                right = pNode->child[1] = _skew ( pNode->child[1] );
                right->child[1] = _skew ( right->child[1] );
                pNode = _split ( pNode );
                pNode->child[1] = _split ( pNode->child[1] );
            }
            return pNode;
        }

        //--------------------------------------------------------------------------------------------------------
        // Erase tree node. n is node to erase, iNDir indicates if n is left or right child of his parent,
        // heir is it's succesor, iHeirDir indicates if heir is left or right child of his parent.
        // Returns successor.
        //--------------------------------------------------------------------------------------------------------
        node_t* _erase( node_t* n, const int iNDir, node_t* heir, const int iHeirDir)
        {
            node_t* heirParent, *nParent = n->parent, *succ = heir;
            int dir;

            // First update max/min.
            if ( n == m_pHead->child[1] )
                m_pHead->child[1] = nParent;
            if ( n == m_pHead->child[0] )
                m_pHead->child[0] = nParent;

            // If n has only one or less childs, then replace node n by it's child.
            if ( ( dir = _is_nil( n->child[0] ) )  ||  _is_nil( n->child[1] ) ) // n has level 1, it's.
            {
                // Case   nParent          nParent          nParent
                //           |                |                |
                //           n      or        n        =>     heir
                //          (                  )
                //       heir                  heir

                // If n has no right child, then successor will be first right parent.
                if ( _is_nil( n->child[1] ) )
                    succ = _get_parent( n, 1 ); // Get first right parent (i.e. parent for which nParent is left child).

                heir = n->child[dir];
                heir->parent = heirParent = nParent;

                if ( _is_nil( nParent ) )
                    m_pHead->parent = heir;
                else
                    nParent->child[iNDir] = heir;
            }
            else
            {
                // Replace n by it's successor. Note that successor has no left child.
                // n -> B
                //     /              n -> heir_parent
                // heir_parent   or       /             or  n=heir_parent -> heir -> R
                //   /                 heir -> R
                // heir -> R
                n->elem = heir->elem;

                heirParent = heir->parent;
                node_t* R = heir->child[1];

                heirParent->child[iHeirDir] = R;
                R->parent = heirParent;

                succ = n;
                n = heir; // Remove heir instead of n.
            }

            // Rebalance the tree.
            heir = heirParent;
            while ( !_is_nil(heirParent = heir->parent) )
            {
                dir = heirParent->child[1] == heir;
                heirParent->child[dir] = heir = _check_balance( heir );
                heir = heirParent;
            }
            m_pHead->parent = _check_balance( heir );

            // We might modify nil->parent when rebalancing, so assign it again to max element.
            nil->parent = m_pHead;
            m_iSize--;

            m_cAlloc.destroy(&n->elem);
            m_cAllocNode.deallocate(n, 1);

            return succ;
        }



    protected: // Types and members.
        typedef typename Alloc::template rebind<node_t>::other alloc_node_t;
        typedef typename Alloc::template rebind<T>::other alloc_t;

        Less m_cLess;
        alloc_t m_cAlloc;
        alloc_node_t m_cAllocNode;

        node_t *nil, *m_pHead;
        int m_iSize;
    };

} // namespace good


#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#endif // __GOOD_AATREE_H__
