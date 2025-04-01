/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

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
