//----------------------------------------------------------------------------------------------------------------
// String buffer implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_STRING_BUFFER_H__
#define __GOOD_STRING_BUFFER_H__


#include "good/string.h"


#define DEFAULT_STRING_BUFFER_ALLOC  0                 ///< Size of string buffer allocated by default.


// Disable obsolete warnings.
WIN_PRAGMA( warning(push) )
WIN_PRAGMA( warning(disable: 4996) )


namespace good
{


    //************************************************************************************************************
    /// Class that holds mutable string.
    /** Note that assignment/copy constructor actually copies given string into buffer's
     * content, so it is very different from string.
     * Avoid using in arrays, as it is expensive (it will copy content, not pointer as in case of string).
     * Buffer will double his size when resized. */
    //************************************************************************************************************
    template< class Char, class Alloc = allocator<Char> >
    class base_string_buffer: public base_string<Char, Alloc>
    {
    public:
        typedef base_string<Char, Alloc> base_class;
        typedef typename base_class::value_type value_type;
        typedef typename base_class::size_type size_type;



        //--------------------------------------------------------------------------------------------------------
        /// Default constructor with optional capacity parameter.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer( size_type iCapacity = DEFAULT_STRING_BUFFER_ALLOC )
        {
            GoodAssert( iCapacity >= 0 );
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer constructor, reserve %d\n", iCapacity );
#endif
            Init(iCapacity);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Move contents from other string buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer( const base_string_buffer& sbOther ): base_string<Char, Alloc>()
        {
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer copy constructor, sbOther: %s\n", sbOther.c_str() );
#endif
            Init( MAX2(sbOther.length()+1, DEFAULT_STRING_BUFFER_ALLOC) );
            copy_contents( sbOther.c_str(), sbOther.length() );
            this->m_pBuffer[sbOther.length()] = 0;
            this->m_iSize = sbOther.length();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Constructor giving buffer with capacity.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer( const Char* sbBuffer, int iCapacity, bool bDeallocate, bool bClear = true ):
            base_string<Char, Alloc>()
        {
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer constructor with buffer, reserved %d\n", iCapacity );
#endif
            GoodAssert(iCapacity > 0);

            m_iCapacity = iCapacity;
            this->m_iStatic = !bDeallocate;
            this->m_pBuffer = (Char*)sbBuffer;
            if ( bClear )
            {
                this->m_iSize = 0;
                this->m_pBuffer[0] = 0;
            }
            else
            {
                this->m_iSize = strlen(sbBuffer);
            }
        }

        //--------------------------------------------------------------------------------------------------------
        /// Copy from string constructor.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer( const string& sOther )
        {
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer copy constructor, str: %s\n", sOther.c_str() );
#endif
            size_t len = sOther.length();
            Init( MAX2(len+1, DEFAULT_STRING_BUFFER_ALLOC) );
            copy_contents( sOther.c_str(), len );
            this->m_pBuffer[len] = 0;
            this->m_iSize = len;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Copy other string contents into buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& operator=( const base_string_buffer& sOther )
        {
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer operator=, %s\n", sOther.c_str() );
#endif
            assign(sOther);
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator <<.
        //--------------------------------------------------------------------------------------------------------
        template <typename T>
        base_string_buffer& operator<<( const T& s ) { return append(s); }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Copy other string contents into buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& operator=( const string& sOther )
        {
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer operator=, %s\n", sOther.c_str() );
#endif
            assign(sOther);
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Copy other string contents into buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& operator=( const Char* szOther )
        {
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer operator=, %s\n", szOther );
#endif
            assign(szOther);
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Copy other string buffer contents into buffer.
        //--------------------------------------------------------------------------------------------------------
        void assign( const base_string_buffer& sbOther )
        {
            assign( sbOther.c_str(), sbOther.length() );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Copy other string contents into buffer.
        //--------------------------------------------------------------------------------------------------------
        void assign( const string& sOther )
        {
            assign( sOther.c_str(), sOther.length() );
        }


        //--------------------------------------------------------------------------------------------------------
        /// Copy other string contents into buffer.
        //--------------------------------------------------------------------------------------------------------
        void assign( const Char* szOther, size_type iOtherSize = base_class::npos )
        {
            if ( iOtherSize == base_class::npos )
                iOtherSize = (size_type)strlen(szOther);

            if ( this->m_iStatic )
            {
                GoodAssert( m_iCapacity >= iOtherSize+1 );
                if ( m_iCapacity <= iOtherSize )
                    iOtherSize = m_iCapacity-1; // For trailing 0.
            }
            else
            {
                if ( iOtherSize >= m_iCapacity )
                {
                    deallocate();
                    reserve( iOtherSize+1 );
                }
            }
            this->m_iSize = 0;
            copy_contents( szOther, iOtherSize );
            this->m_pBuffer[iOtherSize] = 0;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get base_string_buffer allocated capacity.
        //--------------------------------------------------------------------------------------------------------
        int capacity() const { return m_iCapacity; }

        //--------------------------------------------------------------------------------------------------------
        /// Append character to buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& append( Char c )
        {
            increment(1);
            this->m_pBuffer[ this->m_iSize++ ] = c;
            this->m_pBuffer[ this->m_iSize ] = 0;
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Append string to buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& append( const Char* szStr, size_type len = base_class::npos )
        {
            if ( len == base_class::npos )
                len = (size_type)strlen(szStr);
            increment( len );
            copy_contents( szStr, len, this->m_iSize );
            this->m_pBuffer[ this->m_iSize ] = 0;
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Append string to buffer.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& append( const string& sOther )
        {
            return append(sOther.c_str(), sOther.size());
        }

        //--------------------------------------------------------------------------------------------------------
        /// Insert string to buffer at requiered position. pos must be in range [ 0 .. length() ].
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& insert( const string& sOther, int iPos )
        {
            GoodAssert( (0 <= iPos) && (iPos <= this->length()) );
            GoodAssert( sOther.length() > 0 );
            increment( sOther.length() );
            memmove( &this->m_pBuffer[iPos + sOther.length()], &this->m_pBuffer[iPos], (this->length() - iPos) * sizeof(Char) );
            copy_contents( sOther.c_str(), sOther.length(), iPos );
            this->m_pBuffer[ this->m_iSize ] = 0;
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Replace all occurencies of string sFrom in buffer by sTo.
        //--------------------------------------------------------------------------------------------------------
        base_string_buffer& replace( const base_string<Char, Alloc>& sFrom, const base_string<Char, Alloc>& sTo )
        {
            int pos = 0, size = this->length();
            int toL = sTo.length(), fromL = sFrom.length(), diff = toL - fromL;
            while ( (pos = find(sFrom, pos)) != base_string<Char, Alloc>::npos )
            {
                if ( diff > 0 )
                    increment( diff );
                memmove( &this->m_pBuffer[pos+fromL], &this->m_pBuffer[pos+toL], (size - pos - fromL) * sizeof(Char) );
                strncpy( &this->m_pBuffer[pos], sTo.c_str() );
                this->m_iSize += diff;
                pos += toL;
            }
            return *this;
        }


        //--------------------------------------------------------------------------------------------------------
        /** Check if reserve is needed and reallocate internal buffer.
         * Note that buffer never shrinks. */
        //--------------------------------------------------------------------------------------------------------
        void reserve(size_type iCapacity )
        {
            GoodAssert( iCapacity > 0 );
            if ( this->m_pBuffer && (this->m_iCapacity >= iCapacity) )
                return;
            if ( this->m_iStatic )
            {
                this->m_pBuffer = (Char*)malloc( iCapacity * sizeof(Char) );
                this->m_iStatic = false;
            }
            else
                this->m_pBuffer = (Char*)realloc( this->m_pBuffer, iCapacity * sizeof(Char) );
            m_iCapacity = iCapacity;
#ifdef DEBUG_STRING_PRINT
            printf( "base_string_buffer reserve(): %d\n", m_iCapacity );
#endif
        }


    protected:
        //--------------------------------------------------------------------------------------------------------
        // malloc() buffer of size iAlloc.
        //--------------------------------------------------------------------------------------------------------
        void Init(size_type iCapacity )
        {
            if ( iCapacity > 0 )
            {
                this->m_pBuffer = (Char*) malloc( iCapacity * sizeof(Char) );
                this->m_pBuffer[0] = 0;
            }
            else
                this->m_pBuffer = NULL;
            this->m_iCapacity = iCapacity;
            this->m_iSize = 0;
            this->m_iStatic = false;
        }

        //--------------------------------------------------------------------------------------------------------
        // Copy string to already allocated buffer at pos. Don't include trailing 0. Increment size.
        //--------------------------------------------------------------------------------------------------------
        void copy_contents( const Char* szOther, size_type iOtherLen, size_type pos = 0 )
        {
            GoodAssert( iOtherLen >= 0 && pos >= 0 );
            GoodAssert( m_iCapacity >= pos + iOtherLen + 1 );
            strncpy( &this->m_pBuffer[pos], szOther, iOtherLen * sizeof(Char) );
            this->m_iSize += iOtherLen;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Increment buffer size.
        //--------------------------------------------------------------------------------------------------------
        void increment(size_type iBySize )
        {
            size_type desired = this->m_iSize + iBySize + 1; // Trailing 0.
            if ( desired > m_iCapacity )
            {
                GoodAssert( !this->m_iStatic );
                iBySize = m_iCapacity;
                if (iBySize == 0)
                    iBySize = DEFAULT_STRING_BUFFER_ALLOC;
                do {
                    iBySize = iBySize<<1; // Double buffer size.
                } while (iBySize < desired);
                reserve(iBySize);
            }
        }

        //--------------------------------------------------------------------------------------------------------
        // Deallocate if needed.
        //--------------------------------------------------------------------------------------------------------
        void deallocate()
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string_buffer deallocate(): %s; free: %d\n",
                        this->m_pBuffer ? this->m_pBuffer : "null", !this->m_iStatic && this->m_pBuffer );
#endif
            if ( !this->m_iStatic )
                free(this->m_pBuffer);
            this->m_pBuffer = NULL;
        }

    protected:
        size_type m_iCapacity; // Size of allocated buffer.
    };


    typedef good::base_string_buffer<char> string_buffer;


} // namespace good


WIN_PRAGMA( warning(pop) ) // Restore warnings.

#endif // __GOOD_STRING_BUFFER_H__
