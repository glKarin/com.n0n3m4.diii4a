#define STRINGIFY2(arg) #arg
#define STRINGIFY(arg) STRINGIFY2(arg)

extern const char *buildstring;
const char *buildstring =
#ifdef VCREVISION
STRINGIFY(VCREVISION)
#else
"-"
#endif
#ifndef NO_BUILD_TIMESTAMPS
//" " __TIME__
" " __DATE__
#endif
#ifdef BUILDTYPE
" " STRINGIFY(BUILDTYPE)
#endif
#ifdef __clang__ // must be first because clang pretends to be GCC 4.2...
	" Clang "
//	STRINGIFY(__clang_major__)
//	"."
//	STRINGIFY(__clang_minor__)
#elif defined(__GNUC__)
	" GCC "
//	STRINGIFY(__GNUC__)
//	"."
//	STRINGIFY(__GNUC_MINOR__)
#elif defined(_MSC_VER)
	" MSC "
//	STRINGIFY(_MSC_VER)
#endif
;
