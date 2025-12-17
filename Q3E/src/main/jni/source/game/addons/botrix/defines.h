#ifndef __BOTRIX_DEFINES_H__
#define __BOTRIX_DEFINES_H__

#include <good/log.h>

#include "source_engine.h"


// Define for logging.
#define BLOG(user, level, ...)\
    do {\
        extern int iLogBufferSize;\
        extern char* szLogBuffer;\
        int iMin = MIN2(good::log::iFileLogLevel, CUtil::iLogLevel);\
        if ( level >= iMin ) {\
            good::log::format(szLogBuffer, iLogBufferSize, __VA_ARGS__);\
            good::log::print(level, szLogBuffer);\
            CUtil::Message(level, user, szLogBuffer);\
        }\
    } while (false)

// Botrix log with level. Log to server.
#define BLOG_T(...)        BLOG(NULL, good::ELogLevelTrace,   __VA_ARGS__)
#define BLOG_D(...)        BLOG(NULL, good::ELogLevelDebug,   __VA_ARGS__)
#define BLOG_I(...)        BLOG(NULL, good::ELogLevelInfo,    __VA_ARGS__)
#define BLOG_W(...)        BLOG(NULL, good::ELogLevelWarning, __VA_ARGS__)
#define BLOG_E(...)        BLOG(NULL, good::ELogLevelError,   __VA_ARGS__)

// Log to server and also log to user.
#define BULOG_T(user, ...) BLOG(user, good::ELogLevelTrace,   __VA_ARGS__)
#define BULOG_D(user, ...) BLOG(user, good::ELogLevelDebug,   __VA_ARGS__)
#define BULOG_I(user, ...) BLOG(user, good::ELogLevelInfo,    __VA_ARGS__)
#define BULOG_W(user, ...) BLOG(user, good::ELogLevelWarning, __VA_ARGS__)
#define BULOG_E(user, ...) BLOG(user, good::ELogLevelError,   __VA_ARGS__)

// Non fatal assert.
#define BASSERT(exp, ...)\
    do {\
        if ( !(exp) )\
        {\
            BLOG_E("Assert failed: (%s); in %s(), file %s, line %d\n", #exp, __FUNCTION__, __FILE__, __LINE__);\
            BreakDebugger();\
            __VA_ARGS__;\
        }\
    } while (false)


#endif // __BOTRIX_DEFINES_H__
