#ifndef __GOOD_CIRCULAR_BUFFER_H__
#define __GOOD_CIRCULAR_BUFFER_H__


#include "good/vector.h"


namespace good {


    //************************************************************************************************************
    /// Fixed-size circular buffer.
    //************************************************************************************************************
    template <
        typename T,
        template <typename, typename> class Container = vector
    >
    class base_circular_buffer {
    public:

        //========================================================================================================
        /// Const iterator of vector.
        //========================================================================================================
        class const_iterator
        {
        public:
            friend class base_circular_buffer<T, Container>;

            // Constructor by value.
            const_iterator( T* n = NULL ): m_pCurrent(n) {}
            // Copy constructor.
            const_iterator( const_iterator const& itOther ): m_pCurrent(itOther.m_pCurrent) {}

            /// Operator <.
            bool operator< ( const const_iterator& itOther ) const { return m_pCurrent < itOther.m_pCurrent; }
            /// Operator <.
            bool operator<= ( const const_iterator& itOther ) const { return m_pCurrent <= itOther.m_pCurrent; }
            /// Operator <.
            bool operator> ( const const_iterator& itOther ) const { return m_pCurrent > itOther.m_pCurrent; }
            /// Operator <.
            bool operator>= ( const const_iterator& itOther ) const { return m_pCurrent >= itOther.m_pCurrent; }

            /// Operator ==.
            bool operator== ( const const_iterator& itOther ) const { return m_pCurrent == itOther.m_pCurrent; }
            /// Operator !=.
            bool operator!= ( const const_iterator& itOther ) const { return m_pCurrent != itOther.m_pCurrent; }

            /// Operator -.
            int operator- ( const const_iterator& itOther ) const { GoodAssert(m_pCurrent); return m_pCurrent - itOther.m_pCurrent; }

            /// Operator +.
            const_iterator operator+ ( int iOffset ) const { GoodAssert(m_pCurrent); return const_iterator(m_pCurrent + iOffset); }
            /// Operator +=.
            const_iterator& operator+= ( int iOffset ) { GoodAssert(m_pCurrent); m_pCurrent += iOffset; return *this; }

            /// Operator -.
            const_iterator operator- ( int iOffset ) const { GoodAssert(m_pCurrent); return const_iterator(m_pCurrent - iOffset); }
            /// Operator -=.
            const_iterator& operator-= ( int iOffset ) { GoodAssert(m_pCurrent); m_pCurrent -= iOffset; return *this; }

            /// Pre-increment.
            const_iterator& operator++() {GoodAssert(m_pCurrent);  m_pCurrent++; return *this; }
            /// Pre-decrement.
            const_iterator& operator--() { GoodAssert(m_pCurrent); m_pCurrent--; return *this; }

            /// Post-increment.
            const_iterator operator++ (int) { GoodAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent++; return tmp; }
            /// Post-decrement.
            const_iterator operator-- (int) { GoodAssert(m_pCurrent); const_iterator tmp(*this); m_pCurrent--; return tmp; }

            /// Dereference.
            const T& operator*() const { GoodAssert(m_pCurrent); return *m_pCurrent; }
            /// Dereference.
            const T& operator[] (int iOffset) const { GoodAssert(m_pCurrent); return m_pCurrent[iOffset]; }
            /// Element selection through pointer.
            const T* operator->() const { GoodAssert(m_pCurrent); return m_pCurrent; }

        protected:
            T* m_pCurrent;
        };


        //========================================================================================================
        /// Iterator of vector.
        //========================================================================================================
        class iterator: public const_iterator
        {
        public:
            typedef const_iterator base_class;

            // Constructor by value.
            iterator( T* n = NULL ): base_class(n) {}
            // Copy constructor.
            iterator( iterator const& itOther ): base_class(itOther) {}

            /// Operator +.
            iterator operator+ ( int iOffset ) const { GoodAssert(m_pCurrent); return iterator(m_pCurrent + iOffset); }
            /// Operator +=.
            iterator& operator+= ( int iOffset ) { GoodAssert(m_pCurrent); m_pCurrent += iOffset; return *this; }

            /// Operator -.
            int operator- ( const iterator& other ) const { GoodAssert(m_pCurrent); return (m_pCurrent - other.m_pCurrent); }

            /// Operator -.
            iterator operator- ( int iOffset ) const { GoodAssert(m_pCurrent); return iterator(m_pCurrent - iOffset); }
            /// Operator -=.
            iterator& operator-= ( int iOffset ) { GoodAssert(m_pCurrent); m_pCurrent -= iOffset; return *this; }

            /// Pre-increment.
            iterator& operator++() { GoodAssert(m_pCurrent); m_pCurrent++; return *this; }
            /// Pre-decrement.
            iterator& operator--() { GoodAssert(m_pCurrent); m_pCurrent--; return *this; }

            /// Post-increment.
            iterator operator++ (int) { GoodAssert(m_pCurrent); iterator tmp(*this); m_pCurrent++; return tmp; }
            /// Post-decrement.
            iterator operator-- (int) { GoodAssert(m_pCurrent); iterator tmp(*this); m_pCurrent--; return tmp; }

            /// Dereference.
            T& operator*() const { GoodAssert(m_pCurrent); return *m_pCurrent; }
            /// Dereference.
            T& operator[] (int iOffset) const { GoodAssert(m_pCurrent); return m_pCurrent[iOffset]; }
            /// Element selection through pointer.
            T* operator->() const { GoodAssert(m_pCurrent); return m_pCurrent; }
        };


        typedef util_reverse_iterator<iterator> reverse_iterator;

        /*//========================================================================================================
        /// Iterator of vector.
        //========================================================================================================
        class util_reverse_iterator: public iterator
        {
        public:
            typedef iterator base_class;

            // Constructor by value.
            util_reverse_iterator( T* n = NULL ): base_class(n) {}
            // Copy constructor.
            util_reverse_iterator( iterator const& itOther ): base_class(itOther) {}

            /// Pre-increment.
            util_reverse_iterator& operator++() { GoodAssert(m_pCurrent); m_pCurrent--; return *this; }
            /// Pre-decrement.
            util_reverse_iterator& operator--() { GoodAssert(m_pCurrent); m_pCurrent++; return *this; }

            /// Post-increment.
            util_reverse_iterator operator++ (int) { GoodAssert(m_pCurrent); iterator tmp(*this); m_pCurrent--; return tmp; }
            /// Post-decrement.
            util_reverse_iterator operator-- (int) { GoodAssert(m_pCurrent); iterator tmp(*this); m_pCurrent++; return tmp; }
        };*/


        //--------------------------------------------------------------------------------------------------------
        // Constructor with 0 slots.
        //--------------------------------------------------------------------------------------------------------
        base_circular_buffer(): m_iFirst()

        //--------------------------------------------------------------------------------------------------------
        // Constructor with iSize slots.
        //--------------------------------------------------------------------------------------------------------
        base_circular_buffer( int iSize ): m_iFirst(0), m_iLast(0)
        {
            m_cContainer.resize( iSize+1 );
        }

        //--------------------------------------------------------------------------------------------------------
        // Destructor.
        //--------------------------------------------------------------------------------------------------------
        ~base_circular_buffer();

        //--------------------------------------------------------------------------------------------------------
        /// Return underlaying const array.
        //--------------------------------------------------------------------------------------------------------
        const T* data() const { return m_cContainer.data(); }

        //--------------------------------------------------------------------------------------------------------
        /// Return underlaying array.
        //--------------------------------------------------------------------------------------------------------
        T* data() { return m_cContainer.data(); }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to the first element. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_iterator begin() const { return m_cContainer.begin() + iBegin; }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to the element past last one. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_iterator end() const { return m_cContainer.begin() + iEnd; }

        //--------------------------------------------------------------------------------------------------------
        /// Return reverse iterator to the last element. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        util_reverse_iterator rbegin() const { return m_cContainer.begin() + iEnd; }

        //--------------------------------------------------------------------------------------------------------
        /// Return const iterator to the element before first one. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        const_reverse_iterator rend() const { return m_cContainer.begin() + iBegin; }


        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to the first element. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        iterator begin() { return iterator(m_pBuffer); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to the element past last one. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        iterator end() { return iterator(m_pBuffer + m_iSize); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to the last element. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        util_reverse_iterator rbegin() { return iterator(m_pBuffer + m_iSize - 1); }

        //--------------------------------------------------------------------------------------------------------
        /// Return iterator to the element before first one. Notice that this is a random access iterator.
        //--------------------------------------------------------------------------------------------------------
        util_reverse_iterator rend() { return iterator(m_pBuffer - 1); }


        //--------------------------------------------------------------------------------------------------------
        /// Return true ifarray has no elements.
        //--------------------------------------------------------------------------------------------------------
        bool empty() const { return m_iSize == 0; }

        //--------------------------------------------------------------------------------------------------------
        /// Get vector size, that is, count of elements.
        //--------------------------------------------------------------------------------------------------------
        int size() const { return m_iSize; }

        //--------------------------------------------------------------------------------------------------------
        /// Get vector capacity.
        //--------------------------------------------------------------------------------------------------------
        int capacity() const { return m_iCapacity; }

        //--------------------------------------------------------------------------------------------------------
        /// Assignment.
        //--------------------------------------------------------------------------------------------------------
        void assign( const vector& aOther )
        {
            clear();
            good::swap(m_pBuffer, ((vector&)aOther).m_pBuffer);
            good::swap(m_iCapacity, ((vector&)aOther).m_iCapacity);
            good::swap(m_iSize, ((vector&)aOther).m_iSize);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Note that this operator moves content, not copies it.
        //--------------------------------------------------------------------------------------------------------
        vector& operator= ( const vector& aOther )
        {
#ifdef DEBUG_VECTOR_PRINT
            DebugPrint( "vector operator=: %d elements\n", aOther.size() );
#endif
            assign(aOther);
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Array subscript.
        //--------------------------------------------------------------------------------------------------------
        T& operator[] ( int iIndex ) { GoodAssert(0 <= iIndex && iIndex < m_iSize); return m_pBuffer[iIndex]; }

        //--------------------------------------------------------------------------------------------------------
        /// Array subscript const.
        //--------------------------------------------------------------------------------------------------------
        T const& operator[] ( int iIndex ) const { GoodAssert(0 <= iIndex && iIndex < m_iSize); return m_pBuffer[iIndex]; }

        //--------------------------------------------------------------------------------------------------------
        /// Element at index.
        //--------------------------------------------------------------------------------------------------------
        T& at( int iIndex ) { GoodAssert(0 <= iIndex && iIndex < m_iSize); return m_pBuffer[iIndex]; }

        //--------------------------------------------------------------------------------------------------------
        /// Element at index.
        //--------------------------------------------------------------------------------------------------------
        T const& at( int iIndex ) const { GoodAssert(0 <= iIndex && iIndex < m_iSize); return m_pBuffer[iIndex]; }

        //--------------------------------------------------------------------------------------------------------
        /// Get first array element.
        //--------------------------------------------------------------------------------------------------------
        T& front() { GoodAssert(m_iSize > 0); return m_pBuffer[0]; }

        //--------------------------------------------------------------------------------------------------------
        /// Get last array element.
        //--------------------------------------------------------------------------------------------------------
        T& back() { GoodAssert(m_iSize > 0); return m_pBuffer[m_iSize-1]; }

        //--------------------------------------------------------------------------------------------------------
        /// Add element to end of buffer.
        //--------------------------------------------------------------------------------------------------------
        void push_back( const T& tElem ) { insert(m_iSize, tElem); }

        //--------------------------------------------------------------------------------------------------------
        /// Add element to end of buffer.
        //--------------------------------------------------------------------------------------------------------
        void push_front( const T& tElem ) { insert(0, tElem); }

        //--------------------------------------------------------------------------------------------------------
        /// Remove element from end of buffer.
        //--------------------------------------------------------------------------------------------------------
        void pop_back() { erase(m_iSize-1); }

        //--------------------------------------------------------------------------------------------------------
        /// Add element to end of buffer.
        //--------------------------------------------------------------------------------------------------------
        void pop_front() { erase(0); }

        //--------------------------------------------------------------------------------------------------------
        /// Insert element tElem at vector position iPos.
        //--------------------------------------------------------------------------------------------------------
        iterator insert( int iPos, const T& tElem )
        {
            GoodAssert( iPos <= m_iSize );
            increment(1);
            if ( iPos < m_iSize )
                memmove( &m_pBuffer[iPos + 1], &m_pBuffer[iPos],(m_iSize - iPos) * sizeof(T) );
            m_cAlloc.construct( &m_pBuffer[iPos], tElem );
            m_iSize++;
            return iterator(m_pBuffer + iPos);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Insert element tElem before iterator position iPos. Return iterator pointing to next element.
        //--------------------------------------------------------------------------------------------------------
        iterator insert( iterator it, const T& tElem )
        {
            int iPos = it.m_pCurrent - m_pBuffer;
            insert( iPos, tElem );
            return iterator(m_pBuffer + iPos);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Insert element tElem at vector position iPos. Return iterator pointing to next element.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( int iPos )
        {
            GoodAssert( iPos < m_iSize );
            m_cAlloc.destroy(&m_pBuffer[iPos]);
            m_iSize--;
            if ( iPos < m_iSize )
                memmove( &m_pBuffer[iPos], &m_pBuffer[iPos+1], (m_iSize - iPos) * sizeof(T) );
            return iterator(m_pBuffer + iPos);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Delete element at iterator. Return iterator pointing to next element.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( iterator it )
        {
            int iPos = it.m_pCurrent - m_pBuffer;
            erase(iPos);
            return it;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Clear the list.
        //--------------------------------------------------------------------------------------------------------
        void clear()
        {
            for ( int i=0; i<m_iSize; ++i )
                m_cAlloc.destroy(&m_pBuffer[i]);
            m_iSize = 0;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Reserve buffer size.
        //--------------------------------------------------------------------------------------------------------
        void reserve( int iCapacity )
        {
            if ( m_iCapacity >= iCapacity ) return;
            m_pBuffer = m_cAlloc.reallocate( m_pBuffer, iCapacity, m_iCapacity );
            m_iCapacity = iCapacity;
#ifdef DEBUG_VECTOR_PRINT
            DebugPrint( "vector reserve(): %d\n", m_iCapacity );
#endif
        }

        //--------------------------------------------------------------------------------------------------------
        /// Resize array.
        //--------------------------------------------------------------------------------------------------------
        void resize( int iSize, T const& elem = T() )
        {
            if (iSize >= m_iSize)
            {
                reserve(iSize);
                for (int i=m_iSize; i<iSize; ++i)
                    m_cAlloc.construct(&m_pBuffer[i], elem);
            }
            else
            {
                for (int i=iSize; i<m_iSize; ++i)
                    m_cAlloc.destroy(&m_pBuffer[i]);
            }
            m_iSize = iSize;
        }

    protected:
        Container m_cContainer;
        int m_iFirst, m_iLast;
    };



    // Implementation.
    CircularBuffer::CircularBuffer(int slots) {
      if (slots <= 0) {
        num_of_slots_ = 10; /*pre-assigned value */
      } else {
          num_of_slots_ = slots;
      }
      clear();
    }

    CircularBuffer::~CircularBuffer() {
      delete[] data_;
    }

    void CircularBuffer::write(int value) {
      data_[write_index_] = value;
      if (read_index_ == -1) {
        //if buffer is empty, set the read index to the
     //current write index. because that will be the first
     //slot to be read later.
        read_index_ = write_index_;
      }
      write_index_ = (write_index_ + 1) % num_of_slots_;
    }

    int CircularBuffer::read() {
      if (read_index_ == -1) {
        // buffer empty
        return -1;
      }
      int ret_val = data_[read_index_];
      read_index_ = (read_index_ + 1) % num_of_slots_;
      if (read_index_ == write_index_) {
        /*all available data is read, now buffer is empty*/
        read_index_ = -1;
      }

      return ret_val;
    }

    void CircularBuffer::clear() {
      read_index_ = -1; /* buffer empty */
      write_index_ = 0; /* first time writing to that buffer*/
      delete [] data_;
      data_ = new int[num_of_slots_]; /* allocate space for buffer */
    }

} // namespace good

#endif // __GOOD_CIRCULAR_BUFFER_H__
