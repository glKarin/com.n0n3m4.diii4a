//----------------------------------------------------------------------------------------------------------------
// List implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_LIST_H__
#define __GOOD_LIST_H__


#include "good/memory.h"
#include "good/utility.h"


namespace good
{

    //************************************************************************************************************
    /// Bidirectional list of items of type T.
    //************************************************************************************************************
    template <
        typename T,
        typename Alloc = good::allocator<T>
    >
    class list
    {


    protected:
        //========================================================================================================
        /// Node of bidirectional list.
        //========================================================================================================
        struct list_node_t
        {
            T elem;                                 ///< Element of the list.
            list_node_t* next;                      ///< Next list element.
            list_node_t* prev;                      ///< Previous list element.
        };

        typedef struct list_node_t node_t;


    public:
        typedef T value_type;                       ///< Typedef to value type.
        typedef int size_type;                      ///< Typedef to size type.
        typedef T& reference;                       ///< Typedef to reference.
        typedef const T& const_reference;           ///< Typedef to const reference.

        //========================================================================================================
        /// Const iterator of list.
        //========================================================================================================
        class const_iterator
        {
        public:
            friend class list<T, Alloc>;

            // Constructor by value.
            const_iterator ( node_t* n = NULL ): m_pCurrent(n) {}
            // Copy constructor.
            const_iterator ( const_iterator const& itOther ): m_pCurrent(itOther.m_pCurrent) {}

            /// Return true if this iterator is invalid.
            bool invalid() { return m_pCurrent == NULL; }

            /// Operator ==.
            bool operator== ( const_iterator const& itOther ) const { return m_pCurrent == itOther.m_pCurrent; }

            /// Operator !=.
            bool operator!= ( const_iterator const& itOther ) const { return m_pCurrent != itOther.m_pCurrent; }

            /// Pre-increment.
            const_iterator& operator++() { GoodAssert(m_pCurrent); m_pCurrent = m_pCurrent->next; return *this; }
            /// Pre-decrement.
            const_iterator& operator--() { GoodAssert(m_pCurrent); m_pCurrent = m_pCurrent->prev; return *this; }

            /// Post-increment.
            const_iterator operator++(int) { GoodAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent = m_pCurrent->next; return tmp; }
            /// Post-decrement.
            const_iterator operator--(int) { GoodAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent = m_pCurrent->prev; return tmp; }

            /// Dereference.
            const T& operator*() const { GoodAssert(m_pCurrent); return m_pCurrent->elem; }
            /// Element selection through pointer.
            const T* operator->() const { GoodAssert(m_pCurrent); return &m_pCurrent->elem; }

        protected:
            node_t* m_pCurrent;
        };


        //========================================================================================================
        /// Iterator of list.
        //========================================================================================================
        class iterator: public const_iterator
        {
        public:
            typedef const_iterator base_class;

            // Constructor by value.
            iterator ( node_t* n = NULL ): base_class(n) {}
            // Constructor with const_iterator.
            iterator ( const_iterator const& itOther ): base_class(itOther) {}
            // Copy constructor.
            iterator ( iterator const& itOther ): base_class(itOther) {}

            /// Pre-increment.
            iterator& operator++() { GoodAssert(this->m_pCurrent); this->m_pCurrent = this->m_pCurrent->next; return *this; }
            /// Pre-decrement.
            iterator& operator--() { GoodAssert(this->m_pCurrent); this->m_pCurrent = this->m_pCurrent->prev; return *this; }

            /// Post-increment.
            iterator operator++(int) { GoodAssert(this->m_pCurrent); iterator tmp(*this); this->m_pCurrent = this->m_pCurrent->next; return tmp; }
            /// Post-decrement.
            iterator operator--(int) { GoodAssert(this->m_pCurrent); iterator tmp(*this); this->m_pCurrent = this->m_pCurrent->prev; return tmp; }

            /// Dereference.
            T& operator*() const { GoodAssert(this->m_pCurrent); return this->m_pCurrent->elem; }
            /// Element selection through pointer.
            T* operator->() const { GoodAssert(this->m_pCurrent); return &this->m_pCurrent->elem; }
        };

        //--------------------------------------------------------------------------------------------------------
        /// List contructor.
        //--------------------------------------------------------------------------------------------------------
        list(): m_iSize(0)
        {
            m_pTail = m_cAllocNode.allocate(1);
            m_pTail->next = m_pTail;
            m_pTail->prev = m_pTail;
        }

        //--------------------------------------------------------------------------------------------------------
        /// List contructor from another list. Will move all elements to this one.
        //--------------------------------------------------------------------------------------------------------
        list(list const& other): m_iSize(0)
        {
            m_pTail = m_cAllocNode.allocate(1);
            m_pTail->next = m_pTail;
            m_pTail->prev = m_pTail;
            assign(other);
        }

        //--------------------------------------------------------------------------------------------------------
        /// List destructor.
        //--------------------------------------------------------------------------------------------------------
        ~list()
        {
            clear();
            m_cAllocNode.deallocate(m_pTail, 1);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Move contents of other list to this one.
        //--------------------------------------------------------------------------------------------------------
        void duplicate(list const& other)
        {
            clear();
            for (const_iterator it=other.begin(); it != other.end(); ++it)
                push_back(*it);
        }


        //--------------------------------------------------------------------------------------------------------
        /// Returns const iterator of first element of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_iterator begin() const { return const_iterator(m_pTail->next); }

        //--------------------------------------------------------------------------------------------------------
        /// Returns const iterator of end of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_iterator end() const { return const_iterator(m_pTail); }

        //--------------------------------------------------------------------------------------------------------
        /// Returns const iterator of last element of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_iterator rbegin() const { return const_iterator(m_pTail->prev); }

        //--------------------------------------------------------------------------------------------------------
        /// Returns const iterator of beginning of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_iterator rend() const { return const_iterator(m_pTail); }


        //--------------------------------------------------------------------------------------------------------
        /// Returns iterator of first element of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        iterator begin() { return iterator(m_pTail->next); }

        //--------------------------------------------------------------------------------------------------------
        /// Returns iterator of end of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        iterator end() { return iterator(m_pTail); }

        //--------------------------------------------------------------------------------------------------------
        /// Returns iterator of last element of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        iterator rbegin() { return iterator(m_pTail->prev); }

        //--------------------------------------------------------------------------------------------------------
        /// Returns iterator of beginning of the list. Notice that this is not a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        iterator rend() { return iterator(m_pTail); }


        //--------------------------------------------------------------------------------------------------------
        /// Returns first element of the list.
        //--------------------------------------------------------------------------------------------------------
        T& front() { GoodAssert(m_iSize > 0); return m_pTail->next->elem; }

        //--------------------------------------------------------------------------------------------------------
        /// Returns last element of the list.
        //--------------------------------------------------------------------------------------------------------
        T& back() { GoodAssert(m_iSize > 0); return m_pTail->prev->elem; }


        //--------------------------------------------------------------------------------------------------------
        /// Return true if list is empty.
        //--------------------------------------------------------------------------------------------------------
        bool empty() { return m_iSize == 0; }

        //--------------------------------------------------------------------------------------------------------
        /// Clear the list.
        //--------------------------------------------------------------------------------------------------------
        void clear()
        {
            node_t* current = m_pTail->next;
            while ( current != m_pTail )
            {
                m_cAlloc.destroy(&current->elem);
                node_t* tmp = current;
                current = current->next;
                m_cAllocNode.deallocate(tmp, 1);
            }
            m_pTail->next = m_pTail;
            m_pTail->prev = m_pTail;
            m_iSize = 0;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Move contents of other list to this one.
        //--------------------------------------------------------------------------------------------------------
        void assign(list const& other)
        {
            clear();
            good::swap(m_pTail, ((list&)other).m_pTail);
            good::swap(m_iSize, ((list&)other).m_iSize);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Note that this operator moves content, not copies it.
        //--------------------------------------------------------------------------------------------------------
        list& operator= (list const& other)
        {
            assign(other);
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Return number of elements of the list.
        //--------------------------------------------------------------------------------------------------------
        int size() const { return m_iSize; }

        //--------------------------------------------------------------------------------------------------------
        /// Insert new element before position.
        //--------------------------------------------------------------------------------------------------------
        iterator insert( iterator position, const T& elem )
        {
            node_t* pNode = m_cAllocNode.allocate(1);
            m_cAlloc.construct( &pNode->elem, elem );

            node_t* before = position.m_pCurrent->prev;
            node_t* after = position.m_pCurrent;

            pNode->prev = before;
            pNode->next = after;

            before->next = pNode;
            after->prev = pNode;

            m_iSize++;
            return iterator(pNode);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Erase element at position and set position pointing to next element.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( iterator position )
        {
            GoodAssert(m_iSize > 0);
            GoodAssert(position.m_pCurrent != m_pTail);

            node_t* prev = position.m_pCurrent->prev;
            node_t* next = position.m_pCurrent->next;

            prev->next = next;
            next->prev = prev;

            m_cAlloc.destroy(&position.m_pCurrent->elem);
            m_cAllocNode.deallocate(position.m_pCurrent, 1);

            m_iSize--;
            return iterator(next);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Add element to begining of list.
        //--------------------------------------------------------------------------------------------------------
        void push_front( const T& elem ) { insert( begin(), elem ); }

        //--------------------------------------------------------------------------------------------------------
        /// Add element to end of list.
        //--------------------------------------------------------------------------------------------------------
        void push_back( const T& elem ) { insert( end(), elem ); }

        //--------------------------------------------------------------------------------------------------------
        /// Remove element from begining of list.
        //--------------------------------------------------------------------------------------------------------
        void pop_front() { erase( begin() ); }

        //--------------------------------------------------------------------------------------------------------
        /// Remove element from end of list.
        //--------------------------------------------------------------------------------------------------------
        void pop_back() { erase( --end() ); }

    protected:
        typedef typename Alloc::template rebind<node_t>::other alloc_node_t; // Allocator object for pointers to nodes.
        typedef typename Alloc::template rebind<T>::other alloc_t; // Allocator object for T.

        alloc_node_t m_cAllocNode; // Allocator for node for T.
        alloc_t m_cAlloc;          // Allocator for T.

        node_t* m_pTail;           // Last node is this one, and next node is start node (because it is circular list).
        int m_iSize;               // Count of elements in the list.
    };


} // namespace good


#endif // __GOOD_LIST_H__
