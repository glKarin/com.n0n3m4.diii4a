#include <stdio.h>
#include <string.h>

#include "good/log.h"


WIN_PRAGMA( warning(push) )
WIN_PRAGMA( warning(disable: 4996) )


namespace good
{


bool log::bLogToStdOut = true;
bool log::bLogToStdErr = true;
TLogLevel log::iLogLevel = ELogLevelWarning;
TLogLevel log::iFileLogLevel = ELogLevelWarning;
TLogLevel log::iStdErrLevel = ELogLevelError;
FILE* log::m_fLog = NULL;

good::vector<good::log::log_field_t> log::m_aLogFields(8);


//----------------------------------------------------------------------------------------------------------------
char szLogMessage[GOOD_LOG_MAX_MSG_SIZE];
size_t iStartSize = 0;


//----------------------------------------------------------------------------------------------------------------
const good::string aLogLevelsUppercase[ELogLevelTotal] =
{
    "       ",
    "TRACE  ",
    "DEBUG  ",
    "INFO   ",
    "WARNING",
    "ERROR  ",
};
const good::string aLogLevelsLowercase[ELogLevelTotal] =
{
    "       ",
    "trace  ",
    "debug  ",
    "info   ",
    "warning",
    "error  ",
};
const good::string aLogLevelsLetterUppercase[ELogLevelTotal] =
{
    " ",
    "T",
    "D",
    "I",
    "W",
    "E",
};
const good::string aLogLevelsLetterLowercase[ELogLevelTotal] =
{
    " ",
    "t",
    "d",
    "i",
    "w",
    "e",
};

//----------------------------------------------------------------------------------------------------------------
bool log::set_prefix( const char* szPrefix )
{
    /*m_aLogFields.clear();
    iStartSize = 0;
    int iStart = 0, iEnd = 0;

    while ( szPrefix[iEnd] && (iEnd < GOOD_LOG_MAX_MSG_SIZE-1) )
    {
        if ( szPrefix[iEnd] == '%' )
        {
            ++iEnd;
            switch ( szPrefix[iEnd] )
            {
            case 'N':
            case 'F':
            case 'f':
            case 'L':
            case 'l':
            case 'D':
            case 'T':
            case 'p':

                break;
            default:
                break;
            }
        }
    }

    if ( iEnd-iStart > 0)
    {
        good::string sStr( &szPrefix[iStart], true, true, iEnd-iStart );
        m_aLogFields.push_back( log_field_t(sStr, iStartSize) );
        iStartSize += sStr.size();
    }

    int iSize = strnlen(szPrefix, GOOD_LOG_MAX_MSG_SIZE-1);
    if ( iSize == GOOD_LOG_MAX_MSG_SIZE-1 )
        return false;

    szLogMessage
    snprintf()*/

#ifdef GOOD_MULTI_THREAD
    good::lock cLock(m_cMutex);
#endif
    size_t iSize = strnlen(szPrefix, GOOD_LOG_MAX_MSG_SIZE-1);
    strncpy(szLogMessage, szPrefix, iSize);
    szLogMessage[iSize] = 0;
    iStartSize = iSize;
    return true;
}


//----------------------------------------------------------------------------------------------------------------
bool log::start_log_to_file( const char* szFile, bool bAppend /*= false*/ )
{
    m_fLog = fopen(szFile, bAppend ? "w+" : "w");
    return (m_fLog != NULL);
}


//----------------------------------------------------------------------------------------------------------------
void log::stop_log_to_file()
{
#ifdef GOOD_MULTI_THREAD
    good::lock cLock(m_cMutex);
#endif
    if ( m_fLog )
    {
        fflush(m_fLog);
        fclose(m_fLog);
        m_fLog = NULL;
    }
}


//----------------------------------------------------------------------------------------------------------------
size_t log::format( char* szOutput, size_t iOutputSize, const char* szFmt, ... )
{
    va_list argptr;
    va_start(argptr, szFmt);
    size_t iResult = format_va_list(szOutput, iOutputSize, szFmt, argptr);
    va_end(argptr);

    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
size_t log::printf( TLogLevel iLevel, const char* szFmt, ... )
{
    bool bLog = (iLevel >= iLogLevel);
    bool bFile = m_fLog && (iFileLogLevel >= iStdErrLevel);

    if ( !bLog || !bFile )
        return 0;

    va_list argptr;
    va_start(argptr, szFmt);
    size_t iResult = format_va_list(szLogMessage, GOOD_LOG_MAX_MSG_SIZE, szFmt, argptr);
    va_end(argptr);

    if ( iResult )
    {
        // Log to stdout or stderr. TODO: FILE* array[iLevel] for setLogToStdOut() & setLogToStdErr().
        bool bStderr = bLog && bLogToStdErr;
        bool bStdout = !bStderr && bLog && bLogToStdOut;
        FILE* fOut = bStderr ? stderr : (bStdout ? stdout : NULL );
        if ( fOut )
        {
            fputs(szLogMessage, fOut);
    #ifdef GOOD_LOG_FLUSH
            fflush(fOut);
    #endif
        }

        // Log to file.
        if ( bFile )
        {
            fputs(szLogMessage, m_fLog);
    #ifdef GOOD_LOG_FLUSH
            fflush(m_fLog);
    #endif
        }
    }
    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
void log::print( TLogLevel iLevel, const char* szFinal )
{
    bool bLog = (iLevel >= iLogLevel);
    bool bFile = m_fLog && (iLevel >= iFileLogLevel);

    // Log to stdout or stderr. TODO: FILE* array[iLevel] for setLogToStdOut() & setLogToStdErr().
    bool bStderr = bLog && bLogToStdErr;
    bool bStdout = !bStderr && bLog && bLogToStdOut;
    FILE* fOut = bStderr ? stderr : (bStdout ? stdout : NULL );
    if ( fOut )
    {
        fputs(szFinal, fOut);
#ifdef GOOD_LOG_FLUSH
        fflush(fOut);
#endif
    }

    // Log to file.
    if ( bFile )
    {
        fputs(szFinal, m_fLog);
#ifdef GOOD_LOG_FLUSH
        fflush(m_fLog);
#endif
    }
}


//----------------------------------------------------------------------------------------------------------------
size_t log::format_va_list( char* szOutput, size_t iOutputSize, const char* szFmt, va_list argptr )
{
    --iOutputSize; // Save 1 position for trailing 0.
    if ( szOutput != szLogMessage )
    {
        size_t iSize = MAX2(iOutputSize, iStartSize);
        strncpy(szOutput, szLogMessage, iSize);
    }

    if ( iStartSize >= iOutputSize )
    {
#ifdef GOOD_LOG_USE_ENDL
        szOutput[iOutputSize-1] = '\n';
#endif
        szOutput[iOutputSize] = 0;
        return iOutputSize;
    }

    size_t iTotal = vsnprintf(&szOutput[iStartSize], iOutputSize-iStartSize, szFmt, argptr);

    iTotal += iStartSize;
    if ( iTotal > iOutputSize )
#ifdef GOOD_LOG_USE_ENDL
        iTotal = iOutputSize-1;
    szOutput[iTotal++] = '\n';
#else
        iTotal = iOutputSize;
#endif
    szLogMessage[iTotal] = 0;
    return iTotal;
}


} // namespace good


WIN_PRAGMA( warning(pop) ) // Restore warnings.
