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
;
