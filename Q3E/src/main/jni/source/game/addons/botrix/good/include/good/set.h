//----------------------------------------------------------------------------------------------------------------
// Set implementation, based on Anderssion tree.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_SET_H__
#define __GOOD_SET_H__


#include "good/aatree.h"


namespace good
{


    //************************************************************************************************************
    /// Set of unique elements of type T.
    //************************************************************************************************************
    template <
        typename T,
        typename Less = good::less<T>,
        typename Alloc = allocator <T>
    >
    class set: public aatree<T, Less, Alloc>
    {
    public:
        bool contains(T const& elem)
        {
            return this->find(elem) != this->end();
        }
    };


} // namespace good


#endif // __GOOD_SET_H__
