//----------------------------------------------------------------------------------------------------------------
// String implementation.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_STRING_UTILS_H__
#define __GOOD_STRING_UTILS_H__


#include <stdlib.h>

#include "good/defines.h"
#include "good/string.h"


// Disable obsolete warnings.
WIN_PRAGMA( warning(push) )
WIN_PRAGMA( warning(disable: 4996) )


namespace good
{


    //--------------------------------------------------------------------------------------------------------
    /// Return true if string @p sStr starts with char @p c.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    bool starts_with( const String& sStr, typename String::value_type c )
    {
        return ( sStr.size() > 0 ) ? (sStr[0] == c) : false;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return true if string sStr starts with sStart.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    bool starts_with( const String& sStr, const String& sStart )
    {
        if ( sStart.size() > sStr.size() )
            return false;
        return strncmp( sStr.c_str(), sStart.c_str(), sStart.size() ) == 0; // TODO: use compare
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return true if string sStr ends with sEnd.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    bool starts_with( const String& sStr, const typename String::value_type* szStart )
    {
        String sStart(szStart);
        if ( sStart.size() > sStr.size() )
            return false;
        return strncmp( sStr.c_str(), sStart.c_str(), sStart.size() ) == 0; // TODO: use compare
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return true if string @p sStr ends with char @p c.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    bool ends_with( const String& sStr, typename String::value_type c )
    {
        return ( sStr.size() > 0 ) ? (sStr[sStr.size()-1] == c) : false;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return true if string sStr ends with sEnd.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    bool ends_with( const String& sStr, const String& sEnd )
    {
        if ( sEnd.size() > sStr.size() )
            return false;
        return strncmp( &sStr[ sStr.size() - sEnd.size() ], sEnd.c_str(), sEnd.size() ) == 0;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return true if string sStr ends with sEnd.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    bool ends_with( const String& sStr, const typename String::value_type* szEnd )
    {
        String sEnd(szEnd);
        if ( sEnd.size() > sStr.size() )
            return false;
        return strncmp( &sStr[ sStr.size() - sEnd.size() ], sEnd.c_str(), sEnd.size() ) == 0;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return lowercase string.
    //--------------------------------------------------------------------------------------------------------
    template <typename Char>
    Char* lower_case( Char* szStr )
    {
        for ( int i = 0; szStr[i]; ++i )
            if ( ('A' <= szStr[i]) && (szStr[i] <= 'Z') )
                szStr[i] = szStr[i] - 'A' + 'a';
        return szStr;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Return lowercase string.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    String& lower_case( String& sStr )
    {
        for ( typename String::size_type i = 0; i < sStr.size(); ++i )
            if ( ('A' <= sStr[i]) && (sStr[i] <= 'Z') )
                sStr[i] = sStr[i] - 'A' + 'a';
        return sStr;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Remove leading and trailing whitespaces(space, tab, line feed - LF, carriage return - CR).
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    String& trim( String& sStr )
    {
        if (sStr.size() == 0)
            return sStr;

        typename String::size_type begin = 0, end = sStr.size() - 1;

        for ( ; begin <= end; ++begin )
        {
            char c = sStr[begin];
            if ( (c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') )
                break;
        }

        if ( begin > 0 )
            sStr.erase(0, begin);

        end = sStr.size() - 1;
        for ( ; end > 0; --end )
        {
            char c = sStr[end];
            if ( (c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') )
                break;
        }

        ++end;
        if ( end < sStr.size() )
            sStr.erase(end);

        return sStr;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Process escape characters: \n, \r, \t, \0, else \ and next char are transformed to that char.
    //--------------------------------------------------------------------------------------------------------
    template <typename String>
    String& escape( String& sStr )
    {
        // Skip start sequence.
        typename String::size_type start = sStr.find('\\'), end = start, count = 0;
        if ( end == String::npos )
            return sStr;

        while ( end < sStr.size()-1 )
        {
            end++;
            count++;
            switch ( sStr[end] )
            {
            case 'n': sStr[start] = '\n'; break;
            case 'r': sStr[start] = '\r'; break;
            case 't': sStr[start] = '\t'; break;
            case '0': sStr[start] = 0; break;
            default:  sStr[start] = sStr[end];
            }

            start++; end++;
            while ( (end < sStr.size()) && (sStr[end] != '\\') )
                sStr[start++] = sStr[end++];
        }

        sStr.erase(sStr.size() - count);
        return sStr;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Split string into several strings by separator, optionally trimming resulting strings. Put result in given array.
    //--------------------------------------------------------------------------------------------------------
    template <template <typename, typename> class Container, typename String, template <typename> class Alloc>
    void split( const String& sStr, Container< String, Alloc<String> >& aContainer, char separator = ' ', bool bTrim = false )
    {
        typename String::size_type start = 0;
        int end;
        do
        {
            end = sStr.find(separator, start);
            String s = sStr.substr(start, end - start);
            if ( bTrim )
                trim(s);
            aContainer.push_back(s);
            start = end+1;
        } while ( end != String::npos );
    }


} // namespace good


WIN_PRAGMA( warning(pop) ) // Restore warnings.

#endif // __GOOD_STRING_UTILS_H__
