////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Lightweight STL-like Map Template
//  Copyright (c) 2006 Dan Moulding
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  See COPYING.txt for the full terms of the GNU Lesser General Public License.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef VLDBUILD
#error \
"This header should only be included by Visual Leak Detector when building it from source. \
Applications should never include this header."
#endif

#include "tree.h" // Provides access to the Tree template class.

////////////////////////////////////////////////////////////////////////////////
//
//  The Pair Template Class
//
//  This is a lightweight STL-like pair template, for use with the lightweight
//  Map class template.
//
//  Note that while this is a STL-like class, it is not a full STL-compliant
//  implementation of the STL pair utility class. It provides just the bare
//  minimum functionality required by the Visual Leak Detector Map template
//  class below.
//
template <typename Tf, typename Ts>
class Pair {
public:
    // Constructor
    Pair ()
    {
        first = Tf();
        second = Ts();
    }

    // Constructor with initializers.
    Pair (const Tf &f, const Ts &s)
    {
        first = f;
        second = s;
    }

    // Less-than operator - Compares this Pair with another Pair to determine
    //   if this Pair is less than the other Pair. The Pair with the smallest
    //   "first" value is the lesser Pair.
    //
    //  - other (IN): The other pair to compare against.
    //
    //  Return Value:
    //
    //    Returns true if this Pair is less than the other Pair. Otherwise
    //    returns false.
    //
    BOOL operator < (const Pair &other) const
    {
        return (first < other.first);
    }

    // Public data.
    Tf first;  // The first value of the pair.
    Ts second; // The second value of the pair.
};

////////////////////////////////////////////////////////////////////////////////
//
//  The Map Template Class
//
//  This is a lightweidht STL-like map template. It makes use of the Tree class
//  template to enable fast insert, find, and erase operations.
//
//  Note that while this is a STL-like class, it is not a full STL-compliant
//  implementation of the STL map container. It contains just the bare minimum
//  functionality required by Visual Leak Detector. Because of its "lightweight"
//  nature, this map class has a noticeable performance advantage over some
//  other standard STL map implementations.
//
template <typename Tk, typename Tv>
class Map {
public:
    class Iterator {
    public:
        // Constructor
        Iterator ()
        {
            // Plainly constructed iterators don't reference anything.
            m_node = NULL;
            m_tree = NULL;
        }

        // operator != - Inequality operator for Map Iterators. Two Map
        //   Iterators are considered equal if and only if they both reference
        //   the same key/value pair in the same Map.
        //
        //  - other (IN): The other Map Iterator to compare against.
        //
        //  Return Value:
        //
        //    Returns true if the specified Map Iterator is not equal to this
        //    Map Iterator; otherwise, returns false.
        //
        BOOL operator != (const Iterator &other) const
        {
            return ((m_tree != other.m_tree) || (m_node != other.m_node));
        }

        // operator * - Dereference operator for Map Iterators.
        //
        //  Note:  The reference returned by this function is "const", so the
        //    value referenced by the Iterator may not be modified through the
        //    Iterator. This is a departure from STL iterator behavior.
        //
        //    Also, dereferencing an Iterator which does not reference a valid
        //    value in the Map is undefined and will almost certainly cause a
        //    crash.
        //
        //  Return Value:
        //
        //    Returns a const reference to the key/value pair in the Map
        //    referenced by the Iterator.
        //
        const Pair<Tk, Tv>& operator * () const
        {
            return m_node->key;
        }

        // operator ++ - Prefix increment operator for Map Iterators. Causes the
        //   Iterator to reference the in-oder successor of the key/value pair
        //   currently referenced by the Iterator. If the Iterator is currently
        //   referencing the largest key/value pair in the Map, then the
        //   resulting Iterator will reference the Map's end (the NULL pair).
        //
        //  Note: Incrementing an Iterator which does not reference a valid
        //    key/value pair in the Map is undefined and will almost certainly
        //    cause a crash.
        //
        //  Return Value:
        //
        //    Returns the Iterator after it has been incremented.
        // 
        Iterator& operator ++ (int)
        {
            m_node = m_tree->next(m_node);
            return *this;
        }

        // operator ++ - Postfix increment operator for Map Iterators. Causes
        //   the Iterator to reference the in-order successor of the key/value
        //   pair currently referenced by the Iterator. If the Iterator is
        //   currently referencing the largest key/value pair in the Map, then
        //   the resulting Iterator will reference the Map's end (the NULL
        //   pair).
        //
        //  Note: Incrementing an Iterator which does not reference a valid
        //    key/value pair in the Map is undefined and will almost certainly
        //    cause a crash.
        //
        //  Return Value:
        //
        //    Returns the Iterator before it has been incremented.
        // 
        Iterator operator ++ ()
        {
            typename Tree<Pair<Tk, Tv> >::node_t *cur = m_node;

            m_node = m_tree->next(m_node);
            return Iterator(m_tree, cur);
        }

        // operator - - Subtraction operator for Map Iterators. Causes the
        //   the Iterator to reference a key/value pair that is an in-order
        //   predecessor of the currently refereced key/value pair.
        //
        //  - num (IN): Number indicating the number of preceding key/value
        //      pairs to decrement the iterator.
        //
        //  Return Value:
        //
        //    Returns an Iterator referencing the key/value pair that precedes
        //    the original Iterator by "num" pairs.
        //
        Iterator operator - (SIZE_T num) const
        {
            SIZE_T                                count;
            typename Tree<Pair<Tk, Tv> >::node_t *cur = m_node;

            cur = m_tree->prev(m_node);
            for (count = 0; count < num; count++)  {
                cur = m_tree->prev(m_node);
                if (cur == NULL) {
                    return Iterator(m_tree, NULL);
                }
            }
            return Iterator(m_tree, cur);
        }

        // operator == - Equality operator for Map Iterators. Map Iterators are
        //   considered equal if and only if they both refernce the same
        //   key/value pair in the same Map.
        //
        //  - other (IN): The other Map Iterator to compare against.
        //
        //  Return Value:
        //
        //    Returns true if the specified Map Iterator is equal to this Map
        //    Iterator; otherwise returns false.
        //
        BOOL operator == (const Iterator &other) const
        {
            return ((m_tree == other.m_tree) && (m_node == other.m_node));
        }

    private:
        // Private constructor. Only the Map class itself may use this
        //   constructor. It is used for constructing Iterators which reference
        //   specific nodes in the internal tree's structure.
        Iterator (const Tree<Pair<Tk, Tv> > *tree, typename Tree<Pair<Tk, Tv> >::node_t *node)
        {
            m_node = node;
            m_tree = tree;
        }

        typename Tree<Pair<Tk, Tv> >::node_t *m_node; // Pointer to the node referenced by the Map Iterator.
        const Tree<Pair<Tk, Tv> >            *m_tree; // Pointer to the tree containing the referenced node.

        // The Map class is a friend of Map Iterators.
        friend class Map<Tk, Tv>;
    };

    // begin - Obtains an Iterator referencing the beginning of the Map (i.e.
    //   the lowest key/value pair currently stored in the Map).
    //
    //  Return Value:
    //
    //    Returns an Iterator referencing the first key/value pair in the Map.
    //    If no key/value pairs are currenly stored in the map, returns the
    //    "NULL" Iterator.
    //
    Iterator begin () const
    {
        return Iterator(&m_tree, m_tree.begin());
    }

    // end - Obtains an Iterator referencing the end of the Map. The end of
    //   the map does not reference an actual key/value pair. Instead it
    //   represents a "null" key/value pair which signifies the end (i.e. just
    //   beyond largest key/value pair currently stored in the Map). Also
    //   known as the "NULL" Iterator.
    //
    //  Return Value:
    //
    //    Returns the "NULL" Iterator, signifying the end of the Map.
    //
    Iterator end () const
    {
        return Iterator(&m_tree, NULL);
    }

    // erase - Erases a key/value pair from the map.
    //
    //  - it (IN): Iterator referencing the key/value pair to be erased from
    //      the map.
    //
    //  Return Value:
    //
    //    None.
    //
    VOID erase (Iterator& it)
    {
        m_tree.erase(it.m_node);
    }

    // erase - Erases a key/value pair from the map.
    //
    //  - key (IN): The key corresponding to the key/value pair to be erased
    //      from the map.
    //
    //  Return Value:
    //
    //    None.
    //
    VOID erase (const Tk &key)
    {
        m_tree.erase(Pair<Tk, Tv>(key, Tv()));
    }

    // find - Finds a key/value pair in the map.
    //
    //  - key (IN): The key corresponding to the key/value pair to be found.
    //
    //  Return Value:
    //
    //    Returns an Iterator referencing the found key/value pair. If no
    //    key/value pair with the specified key could be found, then the "NULL"
    //    Iterator is returned.
    //
    Iterator find (const Tk &key) const
    {
        return Iterator(&m_tree, m_tree.find(Pair<Tk, Tv>(key, Tv())));
    }

    // insert - Inserts a key/value pair into the map.
    //
    //  - key (IN): The key of the key/value pair to be inserted.
    //
    //  - data (IN): The value of the key/value pair to be inserted.
    //
    //  Return Value:
    //
    //    Returns an Iterator referencing the resulting key/value pair after
    //    if has been inserted into the map.
    //
    Iterator insert (const Tk &key, const Tv &data)
    {
        return Iterator(&m_tree, m_tree.insert(Pair<Tk, Tv>(key, data)));
    }

    // reserve - Sets the reserve size of the map. The reserve size is the
    //   number of key/value pairs for which space should be pre-allocated
    //   to avoid frequent heap hits when inserting new key/value pairs into
    //   the map.
    //
    //  - count (IN): The number of key/value pairs for which to reserve space
    //      in advance.
    //
    //  Return Value:
    //
    //    Returns the reserve size previously in use by the map.
    //
    size_t reserve (size_t count)
    {
        return m_tree.reserve(count);
    }

private:
    // Private data
    Tree<Pair<Tk, Tv> > m_tree; // The key/value pairs are actually stored in a tree.
};
