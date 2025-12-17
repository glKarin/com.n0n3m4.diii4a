//----------------------------------------------------------------------------------------------------------------
// File operations.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_FILE_H__
#define __GOOD_FILE_H__


#include <sys/stat.h>

#include "good/string_buffer.h"
#include "good/string_utils.h"


#define FILE_OPERATION_FAILED       -1


// Disable obsolete warnings.
WIN_PRAGMA( warning(push) )
WIN_PRAGMA( warning(disable: 4996) )



#ifdef _WIN32
    #define PATH_SEPARATOR          '\\'
    #define PATH_SEPARATOR_STRING   "\\"
#else
    #define PATH_SEPARATOR          '/'
    #define PATH_SEPARATOR_STRING   "/"
#endif


namespace good
{

    //************************************************************************************************************
    /// File utilities.
    //************************************************************************************************************
    class file
    {
    public:


        //--------------------------------------------------------------------------------------------------------
        /// Get file size. Returns FILE_OPERATION_FAILED if file doesn't exists.
        //--------------------------------------------------------------------------------------------------------
        static size_t file_size( const TChar* szFileName );

        //--------------------------------------------------------------------------------------------------------
        /// Read bytes from position iPos of the file in given buffer. Return false if file doesn't exists.
        //--------------------------------------------------------------------------------------------------------
        static size_t file_to_memory( const TChar* szFileName, void* pBuffer, size_t iBufferSize, long iPos = 0 );

        //--------------------------------------------------------------------------------------------------------
        /// Make folders for a file if they don't exist.
        //--------------------------------------------------------------------------------------------------------
        static bool make_folders( const TChar *szFileName );

        //-------------------------------------------------------------------------------------------------
        /// Append sPath1 to sPath2, using path separator if necessary.
        //-------------------------------------------------------------------------------------------------
        static good::string append_path( const good::string& sPath1, const good::string& sPath2 )
        {
            good::string_buffer result(sPath1.length() + sPath2.length() + 2);
            result.append(sPath1);
            if ( !ends_with(result, PATH_SEPARATOR) )
                result.append(PATH_SEPARATOR);
            result.append(sPath2);
            return (good::string)result;
        }

        //-------------------------------------------------------------------------------------------------
        /// Append sPath to sResult, using path separator if necessary.
        //-------------------------------------------------------------------------------------------------
        static void append_path( good::string_buffer& sResult, const good::string& sPath )
        {
            sResult.reserve(sResult.length() + sPath.length() + 2);
            if ( !ends_with(sResult, PATH_SEPARATOR) )
                sResult.append(PATH_SEPARATOR);
            sResult.append(sPath);
        }



        //--------------------------------------------------------------------------------------------------------
        /// Get file directory (characters before last path separator).
        //--------------------------------------------------------------------------------------------------------
        template <typename String>
        static String dir( const String& sPath )
        {
            typename String::size_type pos = sPath.rfind(PATH_SEPARATOR);
            if ( pos == String::npos )
                return "";
            else
                return sPath.substr(0, pos);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get file directory (characters before last path separator).
        //--------------------------------------------------------------------------------------------------------
        template <typename StringBuffer>
        inline static StringBuffer& dir( StringBuffer& sPath )
        {
            typename StringBuffer::size_type pos = sPath.rfind(PATH_SEPARATOR);
            if ( pos == StringBuffer::npos )
                pos = 0;
            sPath.erase( pos, sPath.size()-pos );
            return sPath;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get file name (characters after last path separator).
        //--------------------------------------------------------------------------------------------------------
        template <typename String>
        static String fname( const String& sPath )
        {
            typename String::size_type pos = sPath.rfind(PATH_SEPARATOR);
            if ( pos == String::npos )
                return sPath;
            else
                return sPath.substr(pos+1);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get file extension (characters after last '.').
        //--------------------------------------------------------------------------------------------------------
        template <typename String>
        static String ext( const String& sPath )
        {
            typename String::size_type posExt = sPath.rfind('.');
            if ( posExt == String::npos )
                return "";

            // Make sure that this is file extension and not of some directory.
            typename String::size_type posFileName = sPath.rfind(PATH_SEPARATOR);
            if ( (posExt > posFileName) ||              // dir\name.ext ok
                 (posFileName == String::npos) )  // name.ext ok
                return sPath.substr(posExt+1);
            else
                return "";
        }

        //--------------------------------------------------------------------------------------------------------
        /// Check if file @p szPath exists.
        //--------------------------------------------------------------------------------------------------------
        inline static bool exists( const TChar* szPath )
        {
            GoodAssert(szPath);
            struct stat cStat;
            return ( stat(szPath, &cStat) == 0 );
        }

        //--------------------------------------------------------------------------------------------------------
        /// Check if file path is absolute.
        //--------------------------------------------------------------------------------------------------------
        inline static bool absolute( const TChar* szPath )
        {
            GoodAssert(szPath);
#ifdef _WIN32
            return ( (szPath[0] != 0) && (szPath[1] == ':') ); // i.e. "c:"
#else
            return ( szPath[0] == '/' );
#endif
        }

    };


} // namespace good


WIN_PRAGMA( warning(pop) ) // Restore warnings.

#endif // __GOOD_FILE_H__
