
#ifndef __MAYA_MAIN_H__
#define __MAYA_MAIN_H__

/*
==============================================================

	Maya Import

==============================================================
*/

// RAVEN BEGIN
// rhummer: unify allocation strategy to try to eliminate some of our crashes
#ifdef RV_UNIFIED_ALLOCATOR
typedef bool ( *exporterDLLEntry_t )( int version, idCommon *common, idSys *sys, void *(*allocator)(size_t size), void (*deallocator)(void *), size_t (*msize)(void *) );
#else
typedef bool ( *exporterDLLEntry_t )( int version, idCommon *common, idSys *sys );
#endif
// RAVEN END
// RAVEN BEGIN
// bdube: default src and dest ospath's
typedef const char *( *exporterInterface_t )( const char *src_ospath, const char* dst_ospath, const char *commandline );
// RAVEN END
typedef void ( *exporterShutdown_t )( void );

#endif /* !__MAYA_MAIN_H__ */
