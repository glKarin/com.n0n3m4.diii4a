//----------------------------------------------------------------------------------------------------------------
// Max heap implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_HEAP_H__
#define __GOOD_HEAP_H__


namespace good
{


    //************************************************************************************************************
    // With less.
    //************************************************************************************************************
    /// Restore order in subtree from iPos.
    /**
     * This function walks down the tree, making sure that the heap property is met,
     * swapping parent with max child that are out of order.
     */
    //   8          9
    //  ( )   =>   ( )
    // 3   9      3   8   <- and repeat for this right child.
    template < typename T, typename Less >
    void heap_adjust_down( T* aHeap, int iPos, int iSize, Less cLess = Less() )
    {
        GoodAssert(iPos <= iSize);
        int left = (iPos << 1) + 1, child;
        if (left >= iSize) return;

        T violator = aHeap[iPos]; // Save element that violates heap property.
        while (left < iSize) // while iPos has at least one child...
        {
            bool isRight = (left+1 < iSize) && cLess(aHeap[left], aHeap[left+1]); // Right is bigger than left.
            child = left + isRight; // Candidate to swap with parent if candidate > parent.

            if ( !cLess(violator, aHeap[child]) ) // exit if violator >= aHeap[child]
                break;

            aHeap[iPos] = aHeap[child];

            iPos = child;
            left = (iPos << 1) + 1;
        }
        aHeap[iPos] = violator;
    }


    //------------------------------------------------------------------------------------------------------------
    /// Restore order up to tree from iPos.
    /** This function swaps element from iPos with its parents until heap property is restored. */
    //------------------------------------------------------------------------------------------------------------
    template < typename T, typename Less >
    void heap_adjust_up( T* aHeap, int iPos, Less cLess = Less() )
    {
        T violator = aHeap[iPos];
        int parent = (iPos-1) >> 1;
        while ( (iPos > 0) && cLess(aHeap[parent], violator) )
        {
            aHeap[iPos] = aHeap[parent];
            iPos = parent;
            parent = (parent-1) >> 1;
        }
        aHeap[iPos] = violator;
    }


    //------------------------------------------------------------------------------------------------------------
    /// This function will add new element to heap. Make sure that array aHeap has space to add new element.
    //------------------------------------------------------------------------------------------------------------
    template < typename T, typename Less >
    void heap_push( T* aHeap, T const& tElem, int iSize, Less cLess = Less() )
    {
        aHeap[iSize++] = tElem;
        heap_adjust_up( aHeap, iSize-1, cLess );
    }


    //------------------------------------------------------------------------------------------------------------
    /// Remove max element from heap and destroy it.
    //------------------------------------------------------------------------------------------------------------
    template < typename T, typename Less >
    void heap_pop( T* aHeap, int iSize, Less cLess = Less() )
    {
        GoodAssert(iSize > 0);
        aHeap[0] = aHeap[--iSize];
        heap_adjust_down(aHeap, 0, iSize, cLess);
    }


    //------------------------------------------------------------------------------------------------------------
    /// Restore heap property after modifying element at position iPos.
    //------------------------------------------------------------------------------------------------------------
    template < typename T, typename Less >
    void heap_modify( T* aHeap, int iPos, int iSize, Less cLess = Less() )
    {
        GoodAssert(0 <= iPos && iPos < iSize);
        if ( (iPos > 0) && cLess(aHeap[(iPos-1) >> 1], aHeap[iPos]) )
            heap_adjust_up(aHeap, iPos, cLess);
        else
            heap_adjust_down(aHeap, iPos, iSize, cLess);
    }


    //------------------------------------------------------------------------------------------------------------
    /// Make heap from array.
    //------------------------------------------------------------------------------------------------------------
    template < typename T, typename Less >
    void heap_make( T* aHeap, int iSize, Less cLess = Less() )
    {
        // Adjust heap for all nodes that have children, from bottom to top.
        int i = iSize >> 1;
        while ( i-- > 0 )
            heap_adjust_down( aHeap, i, iSize, cLess );
    }


    //------------------------------------------------------------------------------------------------------------
    /// Sort array that must be valid heap using heapsort algorithm.
    //------------------------------------------------------------------------------------------------------------
    template < typename T, typename Less >
    void heap_sort( T* aHeap, int iSize, Less cLess = Less() )
    {
        // Now we have a valid heap.
        while ( --iSize > 0 )
        {
            good::swap( aHeap[0], aHeap[iSize] );
            heap_adjust_down( aHeap, 0, iSize, cLess );
        }
    }




    //************************************************************************************************************
    // Without less.
    //************************************************************************************************************

    //************************************************************************************************************
    /// Restore order in subtree from iPos.
    /**
     * This function walks down the tree, making sure that the heap property is met,
     * swapping parent with max child that are out of order.
     */
    //************************************************************************************************************
    template < typename T >
    void heap_adjust_down( T* aHeap, int iPos, int iSize )
    {
        heap_adjust_down( aHeap, iPos, iSize, good::less<T>() );
    }

    //------------------------------------------------------------------------------------------------------------
    /// Restore order up to tree from iPos.
    /**
     * This function swaps element from iPos with its parents until heap
     * property is restored.
     */
    //------------------------------------------------------------------------------------------------------------
    template < typename T >
    void heap_adjust_up( T* aHeap, int iPos )
    {
        heap_adjust_up( aHeap, iPos, good::less<T>() );
    }

    //------------------------------------------------------------------------------------------------------------
    /// Restore heap property after modifying element at position iPos.
    //------------------------------------------------------------------------------------------------------------
    template < typename T >
    void heap_modify( T* aHeap, int iPos, int /*iSize*/ )
    {
        heap_modify( aHeap, iPos, good::less<T>() );
    }

    //------------------------------------------------------------------------------------------------------------
    /// This function will add new element to heap. Make sure that array aHeap has space to add new element.
    //------------------------------------------------------------------------------------------------------------
    template < typename T >
    void heap_push( T* aHeap, const T& tElem, int iSize )
    {
        heap_push( aHeap, tElem, iSize, good::less<T>() );
    }

    //------------------------------------------------------------------------------------------------------------
    /// Remove max element from heap and destroy it.
    //------------------------------------------------------------------------------------------------------------
    template < typename T >
    void heap_pop( T* aHeap, int iSize )
    {
        heap_pop( aHeap, iSize, good::less<T>() );
    }

    //------------------------------------------------------------------------------------------------------------
    /// Make heap from array.
    //------------------------------------------------------------------------------------------------------------
    template < typename T >
    void heap_make( T* aHeap, int iSize )
    {
        heap_make( aHeap, iSize, good::less<T>() );
    }

    //------------------------------------------------------------------------------------------------------------
    /// Sort array that must be valid heap using heapsort algorithm.
    //------------------------------------------------------------------------------------------------------------
    template < typename T >
    void heap_sort( T* aHeap, int iSize )
    {
        heap_sort( aHeap, iSize, good::less<T>() );
    }


} // namespace good


#endif // __GOOD_HEAP_H__
