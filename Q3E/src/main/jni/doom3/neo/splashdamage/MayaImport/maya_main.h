// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MAYA_MAIN_H__
#define __MAYA_MAIN_H__

/*
==============================================================

	Maya Import

==============================================================
*/

typedef bool ( *exporterDLLEntry_t )( int md5Version, const char* mayaVersion, idCommon* common, idSys* sys, idDeclManager* declManager );
typedef const char *( *exporterInterface_t )( const char* ospath, const char* commandline );
typedef void ( *exporterShutdown_t )( void );

#endif /* !__MAYA_MAIN_H__ */
