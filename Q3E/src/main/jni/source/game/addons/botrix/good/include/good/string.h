//----------------------------------------------------------------------------------------------------------------
// String implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_STRING_H__
#define __GOOD_STRING_H__


#include <stdlib.h>
#include <string.h>


#include "good/memory.h"


// Define this to see debug prints of string memory allocation and deallocation.
//#define DEBUG_STRING_PRINT

// Define this so strings are moved around instead of copy. But don't define it if -std=c++11.
#ifndef __GXX_EXPERIMENTAL_CXX0X__
    // TODO:
    #define STRING_MOVE
#endif

// Disable obsolete warnings.
WIN_PRAGMA( warning(push) )
WIN_PRAGMA( warning(disable: 4996) )


namespace good
{


    /**
     * @brief Class that represents secuence of characters.
     */
    template <
        typename Char = TChar,
        typename Alloc = good::allocator<Char>
    >
    class base_string
    {
    public:

        typedef Char value_type;              ///< Typedef for char type.
        typedef int size_type;                ///< Typedef for string size.
        static const int npos = MAX_INT32>>1; ///< Invalid position in string.

        //--------------------------------------------------------------------------------------------------------
        /// Default constructor.
        //--------------------------------------------------------------------------------------------------------
        base_string(): m_pBuffer((char*)""), m_iSize(0), m_iStatic(1)
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string default constructor\n" );
#endif
        }

        //--------------------------------------------------------------------------------------------------------
        /// Copy constructor.
        //--------------------------------------------------------------------------------------------------------
        base_string( const base_string& other ): m_pBuffer((char*)""), m_iSize(0), m_iStatic(1)
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string copy constructor: %s.\n", other.c_str() );
#endif
            assign(other);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Copy constructor.
        //--------------------------------------------------------------------------------------------------------
        base_string( const base_string& other, bool bCopy ): m_pBuffer((char*)""), m_iSize(0), m_iStatic(1)
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string copy constructor: %s, copy %d.\n", other.c_str(), bCopy );
#endif
            assign(other, bCopy);
        }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
        //--------------------------------------------------------------------------------------------------------
        /// Move constructor.
        //--------------------------------------------------------------------------------------------------------
        base_string( base_string&& other ): m_pBuffer(other.m_pBuffer), m_iSize(other.m_iSize), m_iStatic(other.m_iStatic)
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string move constructor: %s.\n", other.c_str() );
#endif
            other.m_iStatic = 1; // Make other static to not deallocate.
        }
#endif

        //--------------------------------------------------------------------------------------------------------
        /// Constructor by 0-terminating string. Make sure that iSize reflects string size properly or is npos.
        //--------------------------------------------------------------------------------------------------------
        base_string( const Char* szStr, bool bCopy = false, bool bDealloc = false, size_type iSize = npos ): m_iStatic(true)
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string constructor: %s, copy %d, dealloc %d\n", szStr, bCopy, bDealloc );
#endif
            m_iSize = ( iSize == npos ) ? strlen(szStr) : iSize;

            if ( m_iSize > 0 ) // szStr is not null nor empty.
            {
                if ( bCopy )
                {
                    copy_contents(szStr);
                    m_iStatic = 0;
                }
                else
                {
                    m_pBuffer = (Char*)szStr;
                    m_iStatic = !bDealloc;
                }
            }
            else
            {
                m_pBuffer = (char*)"";
                m_iStatic = 1;
            }
        }

        //--------------------------------------------------------------------------------------------------------
        /// Destructor.
        //--------------------------------------------------------------------------------------------------------
        virtual ~base_string() { deallocate(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get length of this string.
        //--------------------------------------------------------------------------------------------------------
        size_type length() const { return m_iSize; }

        //--------------------------------------------------------------------------------------------------------
        /// Get length of this string.
        //--------------------------------------------------------------------------------------------------------
        size_type size() const { return m_iSize; }

        //--------------------------------------------------------------------------------------------------------
        /// Get 0-terminating string.
        //--------------------------------------------------------------------------------------------------------
        const Char* c_str() const { return m_pBuffer; }

        //--------------------------------------------------------------------------------------------------------
        /// Const array subscript.
        //--------------------------------------------------------------------------------------------------------
        const Char& operator[] ( size_type iIndex ) const
        {
            GoodAssert( (0 <= iIndex) && (iIndex < m_iSize) );
            return m_pBuffer[iIndex];
        }

        //--------------------------------------------------------------------------------------------------------
        /// Array subscript.
        //--------------------------------------------------------------------------------------------------------
        Char& operator[] ( size_type iIndex )
        {
            GoodAssert( (0 <= iIndex) && (iIndex <= m_iSize) );
            return m_pBuffer[iIndex];
        }

        //--------------------------------------------------------------------------------------------------------
        /// Assign this string to another one. Note that by default this will move content, not copy it.
        //--------------------------------------------------------------------------------------------------------
        base_string& assign( const Char* s, size_type iSize = npos, bool bCopy = false )
        {
            if ( iSize == npos )
                iSize = (size_type)strlen(s);

            if ( bCopy )
            {
                if ( m_iStatic )
                    m_pBuffer = m_cAlloc.allocate(iSize+1);
                else if (m_iSize < iSize)
                    m_pBuffer = m_cAlloc.reallocate(m_pBuffer, (iSize+1)*sizeof(Char), (m_iSize+1)*sizeof(Char));
                memcpy(m_pBuffer, s, iSize*sizeof(Char) );
            }
            else
                m_pBuffer = (Char*)s;
            m_pBuffer[iSize] = 0;

            m_iStatic = !bCopy;
            m_iSize = iSize;

            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Assign this string to another one. Note that by default this will move content, not copy it.
        //--------------------------------------------------------------------------------------------------------
        base_string& assign( const base_string& other, bool bCopy = false )
        {
#ifdef DEBUG_STRING_PRINT
            DebugPrint( "base_string assign: %s\n", other.c_str() );
#endif
            if ( bCopy )
                return assign( other.c_str(), other.size(), bCopy );

            deallocate();
            m_pBuffer = other.m_pBuffer;
            m_iSize = other.m_iSize;
            m_iStatic = other.m_iStatic;
            if ( !m_iStatic )
            {
                ((base_string&)other).m_pBuffer = (char*)"";
                ((base_string&)other).m_iSize = 0;
                ((base_string&)other).m_iStatic = -1;
            }
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Operator =. Note that this operator moves content, not copies it.
        //--------------------------------------------------------------------------------------------------------
        base_string& operator= ( const base_string& other )
        {
            return assign(other);
        }

        //--------------------------------------------------------------------------------------------------------
        /// String comparison.
        //--------------------------------------------------------------------------------------------------------
        bool operator< ( const base_string& other ) const
        {
            GoodAssert( (c_str() !=  NULL) && (other.c_str() !=  NULL) );
            return strcmp(c_str(), other.c_str()) < 0;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Equality operator.
        //--------------------------------------------------------------------------------------------------------
        bool operator== ( const base_string& other ) const
        {
            return ( m_iSize == other.m_iSize ) && ( ( c_str() == other.c_str() )  ||  ( strcmp( c_str(), other.c_str() ) == 0 ) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Equality operator.
        //--------------------------------------------------------------------------------------------------------
        bool operator== ( const Char* other ) const
        {
            GoodAssert(other != NULL);
            return (c_str() == other)  ||  ( (other != NULL) &&  (strcmp( c_str(), other ) == 0) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Not equality operator.
        //--------------------------------------------------------------------------------------------------------
        bool operator!= ( const base_string& other ) const
        {
            return !(*this == other);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Not equality operator.
        //--------------------------------------------------------------------------------------------------------
        bool operator!= ( const Char* other ) const
        {
            return !(*this == other);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get new string concatinating this string and sRight.
        //--------------------------------------------------------------------------------------------------------
        base_string& operator+= ( const base_string& sRight ) const
        {
            return concat_with( sRight.c_str(), sRight.size() );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get new string concatinating this string and szRight.
        //--------------------------------------------------------------------------------------------------------
        base_string operator+ ( const Char* szRight ) const
        {
            GoodAssert(szRight);
            return concat_with( szRight, strlen(szRight) );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get new string concatinating this string and sRight.
        //--------------------------------------------------------------------------------------------------------
        base_string operator+ ( const base_string& sRight ) const
        {
            return concat_with( sRight.c_str(), sRight.m_iSize );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get new string concatinating this string and szRight.
        //--------------------------------------------------------------------------------------------------------
        /*base_string operator+ ( const Char* szRight ) const
        {
            GoodAssert(szRight);
            return concat_with( szRight, strlen(szRight) );
        }*/

        //--------------------------------------------------------------------------------------------------------
        /// Erase iCount characters from buffer at requiered position iPos.
        //--------------------------------------------------------------------------------------------------------
        base_string& erase( size_type iPos = 0, size_type iCount = npos )
        {
            if (iCount == npos)
                iCount = length() - iPos;
            if (iCount == 0)
                return *this;
            GoodAssert( (iCount > 0) && (iPos >= 0) && (iPos < this->length()) && (iPos+iCount <= length()) );
            memmove( &m_pBuffer[iPos], &m_pBuffer[iPos+iCount], (length() - iPos + 1) * sizeof(Char) );
            m_iSize -= iCount;
            return *this;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get string by duplicating this string.
        //--------------------------------------------------------------------------------------------------------
        base_string duplicate() const
        {
            size_type len = m_iSize;
            Char* buffer = m_cAlloc.allocate(len+1);
            strncpy( buffer, m_pBuffer, (len + 1) * sizeof(Char) );
            return base_string( buffer, false, true, len );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Find first occurrence of string str in this string.
        //--------------------------------------------------------------------------------------------------------
        size_type find( const base_string& str, size_type iFrom = 0 ) const
        {
            GoodAssert( iFrom <= m_iSize );
            if ( m_iSize - iFrom < str.m_iSize )
                return npos;

            Char* result = strstr( &m_pBuffer[iFrom], str.c_str() );
            if (result)
                return (size_type)( result - m_pBuffer );
            else
                return npos;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Find first occurrence of Char c in this string.
        //--------------------------------------------------------------------------------------------------------
        size_type find( Char c, size_type iFrom = 0 ) const
        {
            GoodAssert( iFrom <= m_iSize );
            Char* result = strchr( &m_pBuffer[iFrom], (size_type)c );
            if (result)
                return (size_type)( result - m_pBuffer );
            else
                return npos;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Find last occurrence of Char c in this string.
        //--------------------------------------------------------------------------------------------------------
        size_type rfind( Char c, size_type iFrom = npos ) const
        {
            if ( iFrom >= m_iSize )
                iFrom = m_iSize - 1;
            for ( Char* pCurr = m_pBuffer + iFrom; pCurr >= m_pBuffer; --pCurr )
                if ( *pCurr == c )
                    return (size_type)(pCurr - m_pBuffer);
            return npos;
        }

        // TODO: rfind with string.

        //--------------------------------------------------------------------------------------------------------
        /// Get substring from position iFrom and size iSize.
        /** If bAlloc is false then it will modify current string inserting \0 character at iFrom + iSize + 1. */
        //--------------------------------------------------------------------------------------------------------
        base_string substr( size_type iFrom, size_type iSize = npos, bool bAlloc = true ) const
        {
            size_type maxSize = m_iSize - iFrom;
            if ( iSize + iFrom > length() )
                iSize = maxSize;

            GoodAssert( iFrom + iSize <= m_iSize );

            if (bAlloc)
            {
                Char* buffer = m_cAlloc.allocate(iSize+1);
                strncpy( buffer, &m_pBuffer[iFrom], iSize );
                buffer[iSize] = 0;
                return base_string( buffer, false, true, iSize );
            }
            else
            {
                m_pBuffer[iFrom + iSize] = 0;
                return base_string( &m_pBuffer[iFrom], false, false, iSize );
            }
        }

    public: // Static methods.
        //--------------------------------------------------------------------------------------------------------
        /// Get string by adding two strings.
        //--------------------------------------------------------------------------------------------------------
        static base_string concatenate( const base_string& s1, const base_string& s2 )
        {
            size_type len3 = s1.m_iSize + s2.m_iSize;
            Char* buffer = alloc_t().allocate(len3 + 1);
            if (s1.size() > 0)
                strncpy( buffer, s1.c_str(), s1.m_iSize * sizeof(Char) );
            if (s2.size() > 0)
                strncpy( &buffer[s1.m_iSize], s2.c_str(), s2.m_iSize * sizeof(Char) );
            buffer[len3] = 0;
            base_string result;
            result.m_pBuffer = buffer;
            result.m_iSize = len3;
            result.m_iStatic = false;
            return result;
        }


    protected: // Methods.
        //--------------------------------------------------------------------------------------------------------
        // Deallocate if needed.
        //--------------------------------------------------------------------------------------------------------
        void deallocate()
        {
            if ( !m_iStatic )
            {
#ifdef DEBUG_STRING_PRINT
                DebugPrint( "base_string deallocate(): %s; free: %d\n", (m_iSize?m_pBuffer:"null"), !m_iStatic && m_pBuffer );
#endif
                m_cAlloc.deallocate( m_pBuffer );
            }
        }

        //--------------------------------------------------------------------------------------------------------
        // Copy 0-termination base_string.
        //--------------------------------------------------------------------------------------------------------
        void copy_contents(const Char* szFrom)
        {
            m_pBuffer = m_cAlloc.allocate(m_iSize+1);
            strncpy( m_pBuffer, szFrom, m_iSize * sizeof(Char) );
            m_pBuffer[m_iSize] = 0;
        }

        //--------------------------------------------------------------------------------------------------------
        // Return base_string = this + szRight.
        //--------------------------------------------------------------------------------------------------------
        base_string concat_with( const Char* szRight, size_type iRightSize ) const
        {
            size_type len = m_iSize;
            Char* buffer = m_cAlloc.allocate(len + iRightSize + 1);
            strncpy( buffer, m_pBuffer, len * sizeof(Char) );
            strncpy( &buffer[m_iSize], szRight, (iRightSize+1) * sizeof(Char) );
            return base_string ( buffer, false, true );
        }

    protected:
        typedef typename Alloc::template rebind<Char>::other alloc_t; ///< Allocator object for Char.

        alloc_t m_cAlloc;       ///< Allocator for Char.
        Char* m_pBuffer;        ///< Buffer. It contains an extra int before string (at position -1) which is a reference counter.
        size_type m_iSize:31;   ///< String size.
        size_type m_iStatic:1;  ///< If true, then base_string must NOT be deallocated.
    };


    typedef base_string<char> string; ///< Typedef for base_string of chars.


} // namespace good


WIN_PRAGMA( warning(pop) ) // Restore warnings.


#endif // __GOOD_STRING_H__
