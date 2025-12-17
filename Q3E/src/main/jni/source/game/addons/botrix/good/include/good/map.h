//----------------------------------------------------------------------------------------------------------------
// Map implementation, based on Anderssion tree.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_MAP_H__
#define __GOOD_MAP_H__


#include "good/aatree.h"


namespace good
{


    //************************************************************************************************************
    /**
     * @brief Util class to perform binary operations between two elements of type pair<T1, T2>.
     * Binary operation applies to first elements.
     */
    //************************************************************************************************************
    template <typename T, typename Op>
    class pair_first_op
    {
    public:
        /// Default operatorto compare 2 elements.
        bool operator() ( const T& tLeft, const T& tRight ) const
        {
            return op(tLeft.first, tRight.first);
        }
    protected:
        Op op;
    };


    //************************************************************************************************************
    /// Map from Key to Value.
    //************************************************************************************************************
    template <
        typename Key,
        typename Value,
        typename Less = good::less<Key>,
        typename Alloc = allocator< good::pair<Key, Value> >
    >
    class map: public aatree<
                             good::pair<Key, Value>,
                             pair_first_op< good::pair<Key, Value>, Less >,
                             Alloc
                            >
    {
    public:
        typedef good::pair<Key, Value> key_value_t;
        typedef aatree< good::pair<Key, Value>, pair_first_op<good::pair<Key, Value>, Less>, Alloc> base_class;

        typedef typename base_class::node_t node_t;
        typedef typename base_class::const_iterator const_iterator;
        typedef typename base_class::iterator iterator;

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Note that this operator moves content, not copies it.
        //--------------------------------------------------------------------------------------------------------
        //map& operator= (map const& tOther)
        //{
        //	assign(tOther);
        //	return *this;
        //}

        //--------------------------------------------------------------------------------------------------------
        /// Get constant iterator to a key.
        //--------------------------------------------------------------------------------------------------------
        const_iterator find( const Key& key ) const
        {
            return this->find( key_value_t(key, Value()) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get iterator to a key.
        //--------------------------------------------------------------------------------------------------------
        iterator find( const Key& key )
        {
            return base_class::find( key_value_t(key, Value()) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Remove element at map iterator. Return next element.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( iterator it )
        {
            return base_class::erase(it);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Remove key->value association.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( const Key& key )
        {
            node_t* succ = NULL; // Void assign to remove warning.
            int succDir;
            node_t* cand = this->_search( this->m_pHead->parent, key_value_t(key, Value()), succ, succDir );

            if ( this->_is_nil(cand) )
                return iterator(this->nil);

            // Found key->value.
            return iterator( this->_erase( cand, cand->parent->child[1] == cand, succ, succDir ) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get mutable value for a key.
        /**
         * If key not found, insert (key, empty value) into map, returning reference to that value.
         * Use find() function if you need to know if key has some value.
         */
        //--------------------------------------------------------------------------------------------------------
        Value& operator[]( const Key& key )
        {
            key_value_t tmp( key, Value() );
            iterator it = this->insert(tmp, false); // Insert if not exists but don't replace.
            return it->second;
        }

        //--------------------------------------------------------------------------------------------------------
        // Get const value for a key.  Use find() function if you need to know if key has some value.
        // TODO: why doesn't work.
        //--------------------------------------------------------------------------------------------------------
        //const Value& operator[]( const Key& key ) const
        //{
        //	key_value_t tmp( key, Value() );
        //	const_iterator it = find(tmp);
        //	return it->second;
        //}

    };


} // namespace good


#endif // __GOOD_MAP_H__
