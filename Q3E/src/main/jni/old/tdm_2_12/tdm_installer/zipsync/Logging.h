#pragma once

#include <stdarg.h>
#include <stdexcept>

namespace ZipSync {

//how severe logged message is
enum Severity {
    sevVerbose = 1,
    sevDebug,
    sevInfo,
    sevWarning,
    sevError,   //throws exception
    sevFatal,   //terminates program immediately
};

//some messages are assigned nonzero "code"
//it allows intercepting them in error exceptions and in tests
enum LogCode {
    lcGeneric = 0,

    lcAssertFailed,             //ZipSyncAssert has failed
    lcCantOpenFile,             //unexpected fail when opening file
    lcMinizipError,             //unexpected error from minizip function
    lcUserInterrupt,            //generated because progress callback asked to interrupt
    lcDownloadTooSlow,          //curl download stopped as too slow

    //the remaining log codes are intercepted during testing
    lcRenameZipWithoutRepack,
    lcRepackZip,
};

//thrown when message with "error" severity is posted
class ErrorException : public std::runtime_error {
    int _code;
public:
    ErrorException(const char *message, int code = lcGeneric);
    int code() const { return _code; }
};

//base class of Logger
class Logger {
public:
    virtual ~Logger();
    virtual void Message(LogCode code, Severity severity, const char *message) = 0;

    void logf(Severity severity, LogCode code, const char *format, ...);
    void logv(Severity severity, LogCode code, const char *format, va_list args);

                 void verbosef   (LogCode code, const char *format, ...);
                 void debugf     (LogCode code, const char *format, ...);
                 void infof      (LogCode code, const char *format, ...);
                 void warningf   (LogCode code, const char *format, ...);
    [[noreturn]] void errorf     (LogCode code, const char *format, ...);
    [[noreturn]] void fatalf     (LogCode code, const char *format, ...);

                 void verbosef   (const char *format, ...);
                 void debugf     (const char *format, ...);
                 void infof      (const char *format, ...);
                 void warningf   (const char *format, ...);
    [[noreturn]] void errorf     (const char *format, ...);
    [[noreturn]] void fatalf     (const char *format, ...);
};

//global instance of logger, used for everything
extern Logger *g_logger;

std::string formatMessage(const char *format, ...);
std::string assertFailedMessage(const char *code, const char *file, int line);

#define ZipSyncAssert(cond) if (!(cond)) g_logger->errorf(lcAssertFailed, assertFailedMessage(#cond, __FILE__, __LINE__).c_str()); else ((void)0)
#define ZipSyncAssertF(cond, ...) if (!(cond)) g_logger->errorf(lcAssertFailed, __VA_ARGS__); else ((void)0);

}
