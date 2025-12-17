//----------------------------------------------------------------------------------------------------------------
// Util classes: allocator, equal, less, pair and swap template function.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_UTILITY_H__
#define __GOOD_UTILITY_H__


#include <new> // Placement new

#ifndef _WIN32
    #include <stdlib.h> // TODO: why?
#endif

#include "good/defines.h"


namespace good
{


    //************************************************************************************************************
    /// Util class to compare elements.
    //************************************************************************************************************
    template <typename T>
    class equal
    {
    public:
        /// Default operator to compare 2 elements.
        bool operator() ( const T& tLeft, const T& tRight ) const
        {
            return (tLeft == tRight);
        }
    };


    //************************************************************************************************************
    /// Util class to know elements order.
    //************************************************************************************************************
    template <typename T>
    class less
    {
    public:
        /// Default operator to compare 2 elements.
        bool operator() ( const T& tLeft, const T& tRight ) const
        {
            return (tLeft < tRight);
        }
    };


    //************************************************************************************************************
    /// Util class to store 2 elements.
    //************************************************************************************************************
    template <typename T1, typename T2>
    class pair
    {
    public:
        typedef T1 first_type;
        typedef T2 second_type;

        T1 first;
        T2 second;

        /// Default constructor.
        pair(): first(T1()), second(T2()) {}

        /// Constructor by arguments.
        pair(const T1& t1, const T2& t2): first(t1), second(t2) {}

        /// Copy constructor.
        pair (const pair& other): first(other.first), second(other.second) {}
	
		/// Equal operator.
		bool operator==(const pair& other) { return first == other.first && second == other.second; }
	};


    //************************************************************************************************************
    /// Util function to swap variables values.
    //************************************************************************************************************
    template <typename T>
    void swap(T& t1, T& t2)
    {
        T tmp(t1);
        t1 = t2;
        t2 = tmp;
    }


    //************************************************************************************************************
    /// Util function search for element in container.
    //************************************************************************************************************
    template <typename T, typename Iterator>
    Iterator find( Iterator itBegin, Iterator itEnd, const T& elem )
    {
        for ( ; itBegin != itEnd; ++itBegin )
            if ( *itBegin == elem )
                return itBegin;
        return itEnd;
    }


    //************************************************************************************************************
    /// Util function search for element in container.
    //************************************************************************************************************
    template <typename T, typename Container>
    typename Container::const_iterator find( const Container& aContainer, const T& elem )
    {
        return good::find<T, typename Container::const_iterator>(aContainer.begin(), aContainer.end(), elem);
    }


    //************************************************************************************************************
    /// Util function search for element in container.
    //************************************************************************************************************
    template <typename T, typename Container>
    typename Container::iterator find( Container& aContainer, const T& elem )
    {
        return good::find<T, typename Container::iterator>(aContainer.begin(), aContainer.end(), elem);
    }


    //************************************************************************************************************
    /// Util function to get const element from non random access container.
    //************************************************************************************************************
    template <typename Container>
    typename Container::const_reference at( const Container& aContainer, typename Container::size_type iPos )
    {
        GoodAssert( (0 <= iPos) && (iPos < aContainer.size()) );
        typename Container::const_iterator it = aContainer.begin();
        for ( typename Container::size_type i = 0; ( i != iPos ) && (it != aContainer.end()); ++it, ++i );
        return *it;
    }


    //************************************************************************************************************
    /// Util function to get element from non random access container.
    //************************************************************************************************************
    template <typename Container>
    typename Container::reference& at( Container& aContainer, typename Container::size_type iPos )
    {
        GoodAssert( (0 <= iPos) && (iPos < aContainer.size()) );
        typename Container::iterator it = aContainer.begin();
        for ( typename Container::size_type i = 0; ( i != iPos ) && (it != aContainer.end()); ++it, ++i );
        return *it;
    }


    //************************************************************************************************************
    /// Template to create reverse iterator from simple iterator.
    //************************************************************************************************************
    template < typename Iterator >
    class util_reverse_iterator: public Iterator
    {
    public:
        typedef Iterator base_class;
        typedef typename base_class::reference reference;
        typedef typename base_class::pointer pointer;

        // Default constructor.
        util_reverse_iterator(): base_class(NULL) {}

        // Copy constructor.
        util_reverse_iterator( const base_class& itOther ): m_cCurrent(itOther) {}

        /// Pre-increment.
        util_reverse_iterator& operator++() { m_cCurrent--; return *this; }
        /// Pre-decrement.
        util_reverse_iterator& operator--() { m_cCurrent++; return *this; }

        /// Post-increment.
        util_reverse_iterator operator++ (int) { util_reverse_iterator tmp(*this); m_cCurrent--; return tmp; }
        /// Post-decrement.
        util_reverse_iterator operator-- (int) { util_reverse_iterator tmp(*this); m_cCurrent++; return tmp; }

        /// Dereference.
        reference operator*() const { return *(m_cCurrent-1); }
        /// Dereference.
        reference operator[] (int iOffset) const { return m_cCurrent[iOffset-1]; }
        /// Element selection through pointer.
        pointer operator->() const { return (m_cCurrent-1).operator->(); }

    protected:
        Iterator m_cCurrent;
    };
} // namespace good


#endif // __GOOD_UTILITY_H__
