#include <stdio.h>

#include "good/file.h"
#include "good/string_buffer.h"
#include "good/ini_file.h"


// Disable obsolete warnings.
WIN_PRAGMA( warning(push) )
WIN_PRAGMA( warning(disable: 4996) )


namespace good
{

    //------------------------------------------------------------------------------------------------------------
    ini_section::const_iterator ini_section::find_escaped( const ini_string& sKey ) const
    {
        char szBuffer[1024];
        good::string_buffer sbBuffer(szBuffer, 1024, false);

        for ( const_iterator it = m_lKeyValues.begin(); it != m_lKeyValues.end(); ++it )
        {
            sbBuffer.assign( it->key.c_str(), it->key.size() );
            good::escape( sbBuffer );
            if ( sbBuffer == sKey )
                return it;
        }
        return m_lKeyValues.end();
    }


    //------------------------------------------------------------------------------------------------------------
    TIniFileError ini_file::save() const
    {
        // File contents will be:
        // junk (before 1rst section)
        // [Section.name](optionally ;junk)\n
        // Key = value(optionally ;junk)\n

        FILE* f = fopen(name.c_str(), "wb");

        if (f == NULL)
            return IniFileNotFound;

        fprintf(f, "%s", junkBeforeSections.c_str());
        for (const_iterator it = begin(); it != end(); ++it)
        {
            if (it->name.size() > 0)
                fprintf(f, "[%s]", it->name.c_str());
            if (it->junkAfterName.size() > 0)
            {
                fprintf(f, "%s", it->junkAfterName.c_str());
                if (it->eolAfterJunk)
                    fprintf(f, "\n");
            }
            else
                fprintf(f, "\n");
            for (ini_section::const_iterator confIt = it->begin(); confIt != it->end(); ++confIt)
            {
                if ( bTrimStrings )
                    fprintf(f, "%s = %s", confIt->key.c_str(), confIt->value.c_str());
                else
                    fprintf(f, "%s=%s", confIt->key.c_str(), confIt->value.c_str());

                if (confIt->junk.size() > 0)
                {
                    if (confIt->junkIsComment)
                    {
                        if ( bTrimStrings )
                            fprintf(f, " ;");
                        else
                            fprintf(f, ";");
                    }
                    else
                        fprintf(f, "\n");
                    fprintf(f, "%s", confIt->junk.c_str());
                    if (confIt->eolAterJunk)
                        fprintf(f, "\n");
                }
                else
                    fprintf(f, "\n");
            }
        }
        fclose(f);

        return IniFileNoError;
    }



    //------------------------------------------------------------------------------------------------------------
    TIniFileError ini_file::load()
    {
        // Read entire file into memory.
        size_t fsize = file::file_size(name.c_str());

        if (fsize == FILE_OPERATION_FAILED)
            return IniFileNotFound;

        if (fsize > MAX_INI_FILE_SIZE)
            return IniFileTooBig;

        clear();
        TIniFileError result = IniFileNoError;

        char* buf = (char*)malloc(fsize+1);
        if ( !buf )
            return IniFileTooBig;

        size_t read = good::file::file_to_memory(name.c_str(), buf, fsize);
        if ( fsize != read )
            return IniFileTooBig;
        buf[fsize] = 0;

        char *section = NULL, *key = NULL, *value = NULL, *junk = NULL;
        char *line = buf, *first_junk = buf, *junk_after_section_name = NULL;

        int lineNumber = 1, section_end = 0;

        bool comment = false, junkIsComment = false;

        ini_string Key, Value, Junk;

        iterator currentSection = m_lSections.end();

        for ( long pos = 0; pos < fsize; ++pos )
        {
            //----------------------------------------------------------------------------
            // Fast skip comments.
            //----------------------------------------------------------------------------
            if (comment && buf[pos] != '\n')
                continue;

            char c = buf[pos];

            switch ( c )
            {
            //----------------------------------------------------------------------------
            // End of line.
            //----------------------------------------------------------------------------
            case '\n':
                lineNumber++;
                comment = false; // End of line is end of comment.
                section_end = 0;

                section = NULL;
                line = &buf[pos+1];

                // If there was key-value on this line (without comment),
                // then junk will be continued on next line.
                if ( key && (junk == NULL) )
                {
                    junk = line;
                    junkIsComment = false;
                    if (line > buf)
                        *(line-1) = 0; // Make sure value stops at this line.
                }
                break;

            //----------------------------------------------------------------------------
            // Escape character. Don't do nothing, process them later.
            //----------------------------------------------------------------------------
            case '\\':
                /*if (buf[pos+1] == 'x') // Unicode character (example \x002B). Still not supported.
                    pos +=5; else*/
                if (buf[pos+1] == '\r') // "\\r\n"
                    pos +=2;
                else // One of the following: \a \b \0 \t \r \n \\ \; \# \= \:
                    ++pos;
                break;

            //----------------------------------------------------------------------------
            // Start of comment.
            //----------------------------------------------------------------------------
            case INI_FILE_COMMENT_CHAR_1:
#ifdef INI_FILE_COMMENT_CHAR_2
            case INI_FILE_COMMENT_CHAR_2:
#endif
                if (section)
                {
                    section = NULL;
                    DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
                    DebugPrint("no ']', end of section character.\n");
                    result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
                    goto ini_file_end_loop;
#endif
                }
                else if ( key && (junk == NULL) )
                {
                    buf[pos] = 0;
                    junk = &buf[pos+1];
                    junkIsComment = true;
                }
                comment = true;
                break;

            //----------------------------------------------------------------------------
            // Start of a new section.
            //----------------------------------------------------------------------------
            case '[':

                if ( (section == NULL) && (key != line) ) // Section is not started yet and there is no key-value separator before.
                {
                    section_end++;
                    section = &buf[pos+1];
                }
                else
                {
                    if (section)
                        section_end++; // There is need to be 1 more symbol for end of section now.
                }
                break;

            //----------------------------------------------------------------------------
            // End of section.
            //----------------------------------------------------------------------------
            case ']':
                if (section)
                {
                    if (--section_end > 0)
                        continue;

                    *(section - 1) = 0; // Stop everything before section name.

                    // Save junk after section name.
                    if (junk_after_section_name)
                    {
                        currentSection->junkAfterName = junk_after_section_name;
                        currentSection->eolAfterJunk = false;
                        junk_after_section_name = NULL;
                    }

                    // Save previous key-value-junk.
                    if (junk)
                    {
                        if ( m_lSections.empty() )
                        {
                            currentSection = m_lSections.insert(m_lSections.end(), ini_section());
                            junkBeforeSections = first_junk;
                        }

                        ini_string Key(key), Value(value);
                        if ( bTrimStrings )
                        {
                            good::trim(Key);
                            good::trim(Value);
                        }

                        currentSection->add(Key, Value, junk, junkIsComment, false);
                        key = value = junk = NULL;
                        junkIsComment = false;
                    }


                    buf[pos] = 0;
                    ini_string Section(section);

                    if ( bTrimStrings )
                        good::trim(Section);

                    if ( m_lSections.empty() )
                        junkBeforeSections = first_junk;

                    currentSection = m_lSections.insert(m_lSections.end(), ini_section());
                    currentSection->name = Section;
                    section = NULL;
                    junk_after_section_name = &buf[pos+1];
                    comment = true; // Force to be junk until end of line.
                }
                break;

            //----------------------------------------------------------------------------
            // Separator of key-value.
            //----------------------------------------------------------------------------
            case INI_FILE_KV_SEPARATOR_1:
#ifdef INI_FILE_KV_SEPARATOR_2
            //case INI_FILE_KV_SEPARATOR_2:
    //#error  WTF? INI_FILE_KV_SEPARATOR_2 is not defined...
#endif
                if (section == NULL)
                {
                    if (key == NULL)
                    {
                        if (line > buf)
                            *(line-1) = 0; // Make sure last junk stops at this line.

                        if (junk_after_section_name)
                        {
                            currentSection->junkAfterName = junk_after_section_name;
                            currentSection->eolAfterJunk = true;
                            junk_after_section_name = NULL;
                        }

                        buf[pos] = 0;
                        key = line;
                        value = &buf[pos+1];
                        junkIsComment = false;
                        junk = NULL;
                    }
                    else
                    {
                        if (key != line) // Key-value-junk are on some previous line, save them.
                        {
                            if ( m_lSections.empty() )
                            {
                                currentSection = m_lSections.insert(m_lSections.end(), ini_section());
                                junkBeforeSections = first_junk;
                            }

                            // Truncate junk by putting end of string to the end of last line.
                            if (line > buf)
                            {
                                *(line-1) = 0;
                                if (line == junk)
                                    junk = (char*)"";
                            }

                            ini_string Key(key), Value(value);
                            if ( bTrimStrings )
                            {
                                good::trim(Key);
                                good::trim(Value);
                            }

                            currentSection->add(Key, Value, junk, junkIsComment, true);

                            buf[pos] = 0;
                            key = line;
                            value = &buf[pos+1];
                            junk = NULL;
                            junkIsComment = false;
                        }
                        else // Key-value-junk are on same line, = in value.
                        {
                            DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
                            DebugPrint("key-value separator in value.\n");
                            result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
                            goto ini_file_end_loop;
#endif
                        }
                    }
                }
                else
                {
                    DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
                    DebugPrint("key-value separator at section name.\n");
                    result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
                    goto ini_file_end_loop;
#endif
                }
            }
        }


#ifdef INI_FILE_STOP_ON_ERROR
ini_file_end_loop:
#endif
        // Save last junk after section name.
        if (junk_after_section_name)
        {
            currentSection->junkAfterName = junk_after_section_name;
            currentSection->eolAfterJunk = false;
            junk_after_section_name = NULL;
        }

        // Save last key-value-junk.
        else if (key)
        {
            if ( m_lSections.empty() )
            {
                currentSection = m_lSections.insert(m_lSections.end(), ini_section());
                junkBeforeSections = first_junk;
            }

            ini_string Key(key), Value(value);
            if ( bTrimStrings )
            {
                good::trim(Key);
                good::trim(Value);
            }

            currentSection->add(Key, Value, junk, junkIsComment, false);
        }


#ifdef INI_FILE_STOP_ON_ERROR
        if (result != IniFileNoError)
            clear();
        else
#endif
            m_pBuffer = buf;

        return result;
    }


} // namespace


WIN_PRAGMA( warning(pop) ) // Restore warnings.
