//----------------------------------------------------------------------------------------------------------------
// Ini file implementation.
// Copyright (c) 2012 Borzh.
//----------------------------------------------------------------------------------------------------------------

#ifndef __GOOD_INI_FILE_H__
#define __GOOD_INI_FILE_H__


#include "good/list.h"
#include <good/string.h>


//****************************************************************************************************************
// Check out http://en.wikipedia.org/wiki/INI_file for details on ini file format.
// Considerations:
// - you may define all necesary defines below BEFORE including this file, so you don't need to modify
//   this file.
// - quote " character will be included in section name, key or value.
// - you can use escape character to insert special characters in section name, key or value:
//   \a \b \0 \t \r \n \\ \; \# \= \:
// - when reading ini file:
//     * text on same line before and after section name is not processed (considered junk, i.e.
//       "--[Section]--" is valid).
//     * '[' and ']' characters inside section name must be sinchronized (same amount at least). Still if they are
//       not escaped, it is syntax error.
//     * text without key-value separator is not processed (considered junk and it is not syntax error).
//     * first key-value separator is applied (i.e. "key = value = value" -> {"key", "value = value"}), but it is
//       considered syntax error. Use escape character (i.e. "key \= value = value" -> {"key = value", "value"}).
//     * '[' and ']' inside value considered syntax error. Use escape characters.
// - DebugPrint will show all syntax errors locations.
//****************************************************************************************************************


// Define this to have maximum files size. Don't undef it, because 1Mb of memory for ini file is too much,
// don't you think?
#ifndef MAX_INI_FILE_SIZE
    #define MAX_INI_FILE_SIZE 1*1024*1024 // 1Mb.
#endif


// Define this to stop processing ini file if there was syntax error.
//#define INI_FILE_STOP_ON_ERROR


// Define this if you want to change key-value separator. By default it is '='.
#ifndef INI_FILE_KV_SEPARATOR_1
    #define INI_FILE_KV_SEPARATOR_1 '='
#endif

// If you want to have only one key-value separator, define GOOD_INI_ONE_KV_SEPARATOR.
// If not, both key-value separators will be used (by default '=' and ':').
// Define INI_FILE_KV_SEPARATOR_2 if you want to change ':' key-value separator for something else.
#ifndef GOOD_INI_ONE_KV_SEPARATOR
    #ifndef INI_FILE_KV_SEPARATOR_2
        #define INI_FILE_KV_SEPARATOR_2 ':'
    #endif
#endif


// Redefine this if you want to change comment character.
#ifndef INI_FILE_COMMENT_CHAR_1
    #define INI_FILE_COMMENT_CHAR_1 ';'
#endif

// If you want to have only one comment character, define GOOD_INI_ONE_COMMENT_CHAR.
// If not, both comment characters will be used (by default ';' and '#').
// Define INI_FILE_COMMENT_CHAR_2 if you want to change '#' comment character for something else.
#if !defined(GOOD_INI_ONE_COMMENT_CHAR) && !defined(INI_FILE_COMMENT_CHAR_2)
    #define INI_FILE_COMMENT_CHAR_2 '#'
#endif


// Define this and typedef ini_string to say, good::string before including this file.
// But note that implementation with ini_string doesn't copies strings from file buffer
// so it's actually saves memory and is fast (because it just saves pointers to strings).
#ifndef INI_FILE_STRING_DEFINED
    #include <good/string.h>
    typedef good::string ini_string;
#endif




namespace good
{



    /// Type for errors returned by ini_file functions.
    enum TIniFileErrors
    {
        IniFileNoError = 0, ///< Everything is ok.
        IniFileBadSyntax,   ///< Ini file has bad syntax.
        IniFileNotFound,    ///< Ini file doesn't exists or can't be created.
        IniFileTooBig,      ///< Ini file exceedes MAX_INI_FILE_SIZE bytes.
    };
    typedef int TIniFileError;




    //************************************************************************************************************
    /// Class that represents one section of ini file.
    //************************************************************************************************************
    class ini_section
    {

    public: // Types.

        /// One config is pair (key, value) + junk (comments, multiple ends of line, etc).
        struct config
        {
            config(): key(), value(), junk(), junkIsComment(false), eolAterJunk(false) {}

            config( const ini_string& sKey, const ini_string& sValue, const ini_string& sJunk, bool bIsComment, bool bEOLAfterJunk )
                :key(sKey), value(sValue), junk(sJunk), junkIsComment(bIsComment), eolAterJunk(bEOLAfterJunk) {}

            ini_string key;       ///< Key of this configuration.
            ini_string value;     ///< Value of this configuration.
            ini_string junk;      ///< Junk (comment, multiple ends of line, etc).
            bool junkIsComment:1; ///< True if was read comment symbol (';' or '#') before junk of this config.
            bool eolAterJunk:1;   ///< If true then end of line is needed after junk.
        };

        typedef good::list< struct config > configs;    ///< List of configurations of this ini file section.
        typedef configs::const_iterator const_iterator; ///< Const iterator of list of configurations.
        typedef configs::iterator iterator;             ///< Iterator of list of configurations.


    public: // Members.
        ini_string name;          ///< Section name (without brackets []).
        ini_string junkAfterName; ///< Comments and new lines after section name. "\n" by default.
        bool eolAfterJunk;        ///< If true, then there is end of line after junk (for correct syntax).


    public: // Methods.
        //--------------------------------------------------------------------------------------------------------
        /// Constructor.
        //--------------------------------------------------------------------------------------------------------
        ini_section(): name(""), junkAfterName(""), m_lKeyValues() {}

        //--------------------------------------------------------------------------------------------------------
        /// Constructor with name.
        //--------------------------------------------------------------------------------------------------------
        ini_section( const ini_string& sName ): name(sName), junkAfterName(""), m_lKeyValues() {}

        //--------------------------------------------------------------------------------------------------------
        /// Get count of key-values.
        //--------------------------------------------------------------------------------------------------------
        int size() const { return m_lKeyValues.size(); }

        //--------------------------------------------------------------------------------------------------------
        /** Add key, value and optionally junk, which is empty by default.
         *  If junk is not empty, when saving it, if bIsComment is true then ';' will be used before; and if bEOL
         *  is true then end of line will be used after junk. */
        //--------------------------------------------------------------------------------------------------------
        iterator add( const ini_string& sKey, const ini_string& sValue, const ini_string& sJunk = "", bool bIsComment = false, bool bEOLAfterJunk = false )
        {
            return m_lKeyValues.insert( m_lKeyValues.end(), config(sKey, sValue, sJunk, bIsComment, bEOLAfterJunk));
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get value for a key.
        //--------------------------------------------------------------------------------------------------------
        const_iterator find( const ini_string& sKey ) const
        {
            for ( const_iterator it = m_lKeyValues.begin(); it != m_lKeyValues.end(); ++it )
                if ( it->key == sKey )
                    return it;
            return m_lKeyValues.end();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get value for a escaped key.
        //--------------------------------------------------------------------------------------------------------
        inline iterator find( const ini_string& sKey )
        {
            const_iterator it( const_cast<const ini_section*>(this)->find(sKey) );
            return iterator(it);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get value for a key.
        //--------------------------------------------------------------------------------------------------------
        const_iterator find_escaped( const ini_string& sKey ) const;

        //--------------------------------------------------------------------------------------------------------
        /// Get value for a key.
        //--------------------------------------------------------------------------------------------------------
        iterator find_escaped( const ini_string& sKey )
        {
            const_iterator it( const_cast<const ini_section*>(this)->find_escaped(sKey) );
            return iterator(it);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get value for a key. Insert empty value if not exists.
        //--------------------------------------------------------------------------------------------------------
        ini_string& operator[]( const ini_string& sKey )
        {
            iterator it = find(sKey);
            if ( it == end() )
                it = m_lKeyValues.insert( m_lKeyValues.end(),
                                          config(sKey, ini_string(""), ini_string(""), false, false) );
            return it->value;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get junk for a key.
        //--------------------------------------------------------------------------------------------------------
        ini_string& junk( const ini_string& sKey )
        {
            iterator it = find(sKey);
            return it->value;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Remove all junk (comments and stuff from incorrect syntax).
        //--------------------------------------------------------------------------------------------------------
        void remove_junk()
        {
            junkAfterName = "";
            for ( iterator it = m_lKeyValues.begin(); it != m_lKeyValues.end(); ++it )
                it->junk = "";
        }

        //--------------------------------------------------------------------------------------------------------
        // Erase key-value-junk configuration.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( iterator elem )
        {
            return m_lKeyValues.erase(elem);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get const iterator to first element of configurations.
        //--------------------------------------------------------------------------------------------------------
        const_iterator begin() const { return m_lKeyValues.begin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get const iterator to end of configurations.
        //--------------------------------------------------------------------------------------------------------
        const_iterator end() const { return m_lKeyValues.end(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get iterator to first element of configurations.
        //--------------------------------------------------------------------------------------------------------
        iterator begin() { return m_lKeyValues.begin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get iterator to end of configurations.
        //--------------------------------------------------------------------------------------------------------
        iterator end() { return m_lKeyValues.end(); }


    protected: // Members.
        configs m_lKeyValues;

    }; // ini_section




    //************************************************************************************************************
    /// Class that represents ini file.
    //************************************************************************************************************
    class ini_file
    {

    public: // Types.

        typedef good::list<ini_section> container_t;         ///< Type of container of sections.
        typedef container_t::const_iterator const_iterator;  ///< Const iterator of sections.
        typedef container_t::iterator iterator;              ///< Iterator of sections.


    public: // Members.
        ini_string name;               ///< File name.
        ini_string junkBeforeSections; ///< Comments and new lines before first section. Normally empty.
        bool bTrimStrings;             ///< Set to true to trim keys/values.


    public: // Methods.
        //--------------------------------------------------------------------------------------------------------
        /// Constructor with file name as parameter.
        //--------------------------------------------------------------------------------------------------------
        ini_file(): name(""), junkBeforeSections(), bTrimStrings(true), m_pBuffer(NULL), m_lSections() {}

        //--------------------------------------------------------------------------------------------------------
        /// Destructor.
        //--------------------------------------------------------------------------------------------------------
        ~ini_file() { if (m_pBuffer) free(m_pBuffer); }

        //--------------------------------------------------------------------------------------------------------
        /// Save ini file.
        //--------------------------------------------------------------------------------------------------------
        TIniFileError save() const;

        //--------------------------------------------------------------------------------------------------------
        /**
         * Load ini file. Note that if no sections are present in the file, then it is assumed that there is only
         * one "default" section.
         */
        //--------------------------------------------------------------------------------------------------------
        TIniFileError load();

        //--------------------------------------------------------------------------------------------------------
        /// Clear all memory.
        //--------------------------------------------------------------------------------------------------------
        void clear()
        {
            junkBeforeSections = "";
            if (m_pBuffer)
            {
                free(m_pBuffer);
                m_pBuffer = NULL;
            }
            m_lSections.clear();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get section iterator from section name.
        //--------------------------------------------------------------------------------------------------------
        ini_section& operator[]( const ini_string& sSection )
        {
            for (iterator it = m_lSections.begin(); it != m_lSections.end(); ++it)
                if (it->name == sSection)
                    return *it;
            return *m_lSections.insert(m_lSections.end(), ini_section(sSection));;
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get section iterator from section name.
        //--------------------------------------------------------------------------------------------------------
        const_iterator find( const ini_string& sSection ) const
        {
            for ( const_iterator it = m_lSections.begin(); it != m_lSections.end(); ++it )
                if ( it->name == sSection )
                    return it;
            return m_lSections.end();
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get section iterator from section name.
        //--------------------------------------------------------------------------------------------------------
        iterator find( const ini_string& sSection )
        {
            for ( iterator it = m_lSections.begin(); it != m_lSections.end(); ++it )
                if ( it->name == sSection )
                    return it;
            return m_lSections.end();
        }

        //--------------------------------------------------------------------------------------------------------
        // Erase section.
        //--------------------------------------------------------------------------------------------------------
        iterator erase( iterator elem )
        {
            return m_lSections.erase(elem);
        }

        //--------------------------------------------------------------------------------------------------------
        /// Get const iterator to first section.
        //--------------------------------------------------------------------------------------------------------
        const_iterator begin() const { return m_lSections.begin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get const iterator to the end of last section.
        //--------------------------------------------------------------------------------------------------------
        const_iterator end() const { return m_lSections.end(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get iterator to first section.
        //--------------------------------------------------------------------------------------------------------
        iterator begin() { return m_lSections.begin(); }

        //--------------------------------------------------------------------------------------------------------
        /// Get iterator to the end of last section.
        //--------------------------------------------------------------------------------------------------------
        iterator end() { return m_lSections.end(); }

    protected: // Members.
        char* m_pBuffer;         // Buffer, that contains strings from read file.
        container_t m_lSections; // Sections of ini file.

    }; // ini_file


} // namespace good


#endif // __GOOD_INI_FILE_H__
