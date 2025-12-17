//----------------------------------------------------------------------------------------------------------------
// Util defines for debugging.
// Copyright (c) 2011 Borzh.
//----------------------------------------------------------------------------------------------------------------

// Force documenting defines.
/** @file */

#ifndef __GOOD_DEFINES_H__
#define __GOOD_DEFINES_H__


#ifndef abstract
    /// Empty define for abstract class.
    #define abstract
#endif


#ifndef NULL
    // NULL should already be defined.
    #define NULL                               0
#endif


#ifdef _WIN32
    // NULL should already be defined.
    #define GOOD_DLL_PUBLIC                    __declspec(dllexport)
    #define GOOD_DLL_LOCAL                     __declspec(dllimport)
#else
    #define GOOD_DLL_PUBLIC                    __attribute__ ((visibility("default")))
    #define GOOD_DLL_LOCAL                     __attribute__ ((visibility("hidden")))
#endif


/// Char type.
typedef char TChar;


#ifndef TIME_INFINITE
    /// Infinite time.
    #define TIME_INFINITE                     0xFFFFFFFF
#endif


#ifndef MAX_UINT32
    /// Max of unsigned 32bit int.
    #define MAX_UINT32                        0xFFFFFFFF
#endif


#ifndef MAX_INT32
    /// Max of signed 32bit int.
    #define MAX_INT32                         0x7FFFFFFF
#endif


#ifndef MAX_UINT16
    /// Max of unsigned 16bit short.
    #define MAX_UINT16                        0xFFFF
#endif


#ifndef MAX_INT16
    /// Max of signed 16bit short.
    #define MAX_INT16                         0x7FFF
#endif


#ifndef MAX2
    /// Max of two numbers.
    #define MAX2(a, b)                        ((a)>=(b)?(a):(b))
#endif


#ifndef MIN2
    /// Min of two numbers.
    #define MIN2(a, b)                        ((a)<=(b)?(a):(b))
#endif


#ifndef SQR
    /// Square of a number.
    #define SQR(x)                            ((x)*(x))
#endif


/// Get first byte of a number.
#define GET_1ST_BYTE(x)                      ((x) & 0xFF)

/// Get second byte of a number.
#define GET_2ND_BYTE(x)                      ( ((x) >> 8) & 0xFF )

/// Get third byte of a number.
#define GET_3RD_BYTE(x)                      ( ((x) >> 16) & 0xFF )

/// Get fourth byte of a number.
#define GET_4TH_BYTE(x)                      ( ((x) >> 24) & 0xFF )



/// Make first byte of a number.
#define MAKE_1ST_BYTE(x)                     ((x) & 0xFF)

/// Make second byte of a number.
#define MAKE_2ND_BYTE(x)                     ( ((x) & 0xFF) << 8 )

/// Make third byte of a number.
#define MAKE_3RD_BYTE(x)                     ( ((x) & 0xFF) << 16 )

/// Make fourth byte of a number.
#define MAKE_4TH_BYTE(x)                     ( ((x) & 0xFF) << 24 )



/// Set first byte of a number.
#define SET_1ST_BYTE(b, number)              number = (number & 0xFFFFFF00)  |  ( ((b)&0xFF) )

/// Set second byte of a number.
#define SET_2ND_BYTE(b, number)              number = (number & 0xFFFF00FF)  |  ( ((b)&0xFF) << 8 )

/// Set third byte of a number.
#define SET_3RD_BYTE(b, number)              number = (number & 0xFF00FFFF)  |  ( ((b)&0xFF) << 16 )

/// Set fourth byte of a number.
#define SET_4TH_BYTE(b, number)              number = (number & 0x00FFFFFF)  |  ( ((b)&0xFF) << 24 )



/// Get first 16bits word of a number.
#define GET_1ST_WORD(x)                      ((x) & 0xFFFF)

/// Get second 16bits word of a number.
#define GET_2ND_WORD(x)                      ( ((x) >> 16) & 0xFFFF )


/// Set first 16bits word of a number.
#define SET_1ST_WORD(word, number)           number = ( (number & 0xFFFF0000) | ((word) & 0xFFFF) )

/// Set second 16bits word of a number.
#define SET_2ND_WORD(word, number)           number = ( (number & 0xFFFF)     | ((word) << 16) )



/// Enum to bit flag.
#define ENUM_TO_FLAG(x)                      ( 1<<(x) )


/// Set flag in flags.
#define FLAG_SET(flag, flags)                flags |= (flag)

/// Clear flag in flags.
#define FLAG_CLEAR(flag, flags)              flags &= ~(flag)

/// To know if some flag is cleared.
#define FLAG_CLEARED(flag, flags)            ( ((flags) & (flag)) == 0 )

/// Return true if some flag is set.
#ifdef _WIN32 // Fix for Windows forcing value to bool 'true' or 'false' (performance warning).
    #define FLAG_SOME_SET(flag, flags)       ( ((flag) & (flags)) != 0 )
#else
    #define FLAG_SOME_SET(flag, flags)       ( (flag) & (flags) )
#endif

/// Return true if all flags are set.
#define FLAG_ALL_SET(flag, flags)            ( ((flag) & (flags)) == (flag) )

/// Return true if some flag is set or flag is 0.
#define FLAG_SOME_SET_OR_0(flag, flags)      ( ((flag) & (flags)) || ((flag) == 0) )

/// Return true if all flags are set.
#define FLAG_ALL_SET_OR_0(flag, flags)       ( ( ((flag) & (flags)) == (flag) ) || ((flag) == 0) )


/// Get needed size of bit array in bytes.
#define BIT_ARRAY_SIZE(size)                 ( ((size)>>3) + (((size)&7) ? 1 : 0) )

/// Set bit in bit's array.
#define BIT_ARRAY_SET(index, arr)            arr[(index)>>3] |= 1 << ((index)&7)

/// Clear bit in bit's array.
#define BIT_ARRAY_CLEAR(index, arr)          arr[(index)>>3] &= ~( 1 << ((index)&7) )

/// Return true if bit in bit's array is set.
#define BIT_ARRAY_IS_SET(index, arr)         ( arr[(index)>>3] & (1 << ((index)&7)) )


/// Show warnings for format functions.
#ifdef __GNUC__
    #define FORMAT_FUNCTION(s, n)            __attribute__ (( format( __printf__, s, n )))
#else
    #define FORMAT_FUNCTION(s, n)
#endif


/// Define for Windows pragma.
#ifdef _WIN32
    #define WIN_PRAGMA(...)                  __pragma(__VA_ARGS__)
#else
    #define WIN_PRAGMA(...)
#endif


/// Define to start a scope.
#define GOOD_SCOPE_START \
    do {

/// Define to end a scope.
#define GOOD_SCOPE_END \
    WIN_PRAGMA( warning(push) )\
    WIN_PRAGMA( warning(disable: 4127) )\
    } while(false)\
    WIN_PRAGMA( warning(pop) )
// Warning C4127: conditional expression is constant.


/// Debug break, stopping execution.
#ifndef BreakDebugger
    #if defined(DEBUG) || defined(_DEBUG)
        #ifdef _WIN32
            #if defined(_M_X64)
                #define BreakDebugger()      __debugbreak()
            #else
                #define BreakDebugger()      __asm { int 3 }
            #endif
        #else
            #include <signal.h>
            #define BreakDebugger()          raise(SIGTRAP)
        #endif
    #else
        #define BreakDebugger(...)           void(0)
    #endif
#endif

#define BreakDebuggerIf(cond)    do { if ( (cond) ) BreakDebugger(); } while (false)

//#ifndef BreakDebuggerIf
//    #if defined(DEBUG) || defined(_DEBUG)
//        #ifdef _WIN32
//            /// Break in debugger if condition.
//            #define BreakDebuggerIf(cond)    do { if ( (cond) ) BreakDebugger(); } while (false)
//        #else
//            #include <signal.h>
//            /// Break in debugger.
//            #define BreakDebuggerIf(cond)    do { if ( (cond) ) raise(SIGTRAP); } while (false)
//        #endif
//    #else
//        /// No debugger break.
//        #define BreakDebuggerIf(cond)        void(0)
//    #endif
//#endif


#ifndef ReleasePrint
    #include <stdio.h>

    /// Release print, will print always.
    #define ReleasePrint(...)                GOOD_SCOPE_START printf(__VA_ARGS__); fflush(stdout); GOOD_SCOPE_END
#endif


#ifndef DebugPrint
    #if defined(DEBUG) || defined(_DEBUG) || defined (BETA_VERSION)
        /// Debug print. Will print in debug build only.
        #define DebugPrint(...)              ReleasePrint(__VA_ARGS__)
    #else
        /// Debug print will do nothing.
        #define DebugPrint(...)              void(0)
    #endif
#endif



// Check condition, executing instructions if condition fails.
#if defined(DEBUG) || defined(_DEBUG) || defined (BETA_VERSION)
    #define GoodCheck_(exp, start, ...) \
        GOOD_SCOPE_START \
            if ( !(exp) )\
            {\
                DebugPrint(start " failed: (" #exp "); in %s(), file %s, line %d\n", __FUNCTION__, __FILE__, __LINE__);\
                BreakDebugger();\
                __VA_ARGS__;\
            }\
        GOOD_SCOPE_END
#else
    #define GoodCheck_(...)
#endif

/// Check condition, executing instructions if @p exp fails.
#define GoodCheck(exp, ...)                  GoodCheck_(exp, "Check", __VA_ARGS__)

/// Debug assert. If @p exp is false, will produce debugger break.
#define GoodAssert(exp)                      GoodCheck_(exp, "Assert", void(0))


#endif // __GOOD_DEFINES_H__
