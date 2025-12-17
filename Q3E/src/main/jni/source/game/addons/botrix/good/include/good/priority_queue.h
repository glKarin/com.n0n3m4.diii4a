//----------------------------------------------------------------------------------------------------------------
// Priority queue based on max heap implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_PRIORITY_QUEUE_H__
#define __GOOD_PRIORITY_QUEUE_H__


#include "good/vector.h"
#include "good/heap.h"


namespace good
{


    /**
     * Implementation of priority queue with binary heap.
     */
    template <
        typename T,                                                 ///< Type to save in prority queue.
        typename Less = good::less<T>,                               ///< Type to compare elements of type T.
        typename Alloc = allocator<T>,                         ///< Allocator for T.
        template <typename, typename> class Container = good::vector ///< Must be random access.
    >
    class priority_queue
    {
    public:
        typedef Container<T, Alloc> container_t;

        //--------------------------------------------------------------------------------------------------------
        /// Default constructor.
        //--------------------------------------------------------------------------------------------------------
        priority_queue(): m_cContainer(0) {}

        //--------------------------------------------------------------------------------------------------------
        /// Constructor with capacity.
        //--------------------------------------------------------------------------------------------------------
        priority_queue( int iCapacity ): m_cContainer(iCapacity) {}

        //--------------------------------------------------------------------------------------------------------
        /// Destructor.
        //--------------------------------------------------------------------------------------------------------
        ~priority_queue() {}

        //--------------------------------------------------------------------------------------------------------
        /// Get front of the queue (max element).
        //--------------------------------------------------------------------------------------------------------
        int size() const { return m_cContainer.size(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get front of the queue (max element).
        //--------------------------------------------------------------------------------------------------------
        bool empty() const { return m_cContainer.empty(); }

        //--------------------------------------------------------------------------------------------------------
        /// Clear container.
        //--------------------------------------------------------------------------------------------------------
        void clear() { m_cContainer.clear(); }

        //--------------------------------------------------------------------------------------------------------
        /// Reserve needed amount in container.
        //--------------------------------------------------------------------------------------------------------
        void reserve( int iSize ) { m_cContainer.reserve(iSize); }

        //--------------------------------------------------------------------------------------------------------
        /// Get const front of the queue (max element).
        //--------------------------------------------------------------------------------------------------------
        const T& top() const { return m_cContainer.front(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get front of the queue (max element).
        //--------------------------------------------------------------------------------------------------------
        T& top() { return m_cContainer.front(); }

        //--------------------------------------------------------------------------------------------------------
        /// Push element into the queue.
        //--------------------------------------------------------------------------------------------------------
        void push(const T& elem)
        {
            m_cContainer.push_back(elem);
            good::heap_adjust_up(&*m_cContainer.begin(), size()-1, m_cLess);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Pop max element from the queue.
        //--------------------------------------------------------------------------------------------------------
        void pop()
        {
            good::heap_pop(&*m_cContainer.begin(), size(), m_cLess);
            m_cContainer.pop_back();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Modify element in the queue.
        //--------------------------------------------------------------------------------------------------------
        void modify( const T& elem )
        {
            typename container_t::iterator it = good::find(m_cContainer.begin(), m_cContainer.end(), elem);
            GoodAssert(it != m_cContainer.end());

            good::heap_modify(&*m_cContainer.begin(), &*it - &*m_cContainer.begin(), size(), m_cLess);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get const container to be able to perform search there.
        //--------------------------------------------------------------------------------------------------------
        const container_t& get_container() const { return m_cContainer; }

    protected:
        container_t m_cContainer; // Container of T.
        Less m_cLess;             // Comparator functor.
    };


} // namespace good


#endif // __GOOD_PRIORITY_QUEUE_H__
