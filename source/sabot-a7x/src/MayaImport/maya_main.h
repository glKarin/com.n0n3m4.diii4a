
#ifndef __MAYA_MAIN_H__
#define __MAYA_MAIN_H__

/*
==============================================================

	Maya Import

==============================================================
*/


typedef bool ( *exporterDLLEntry_t )( int version, idCommon *common, idSys *sys );
typedef const char *( *exporterInterface_t )( const char *ospath, const char *commandline );
typedef void ( *exporterShutdown_t )( void );

#endif /* !__MAYA_MAIN_H__ */
