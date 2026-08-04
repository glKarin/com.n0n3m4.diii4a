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

#ifndef __AASFILEMANAGER_H__
#define __AASFILEMANAGER_H__

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class idAASFileManager {
public:
	virtual						~idAASFileManager( void ) {}

	virtual idAASFile *			LoadAAS( const char *fileName, const unsigned int mapFileCRC ) = 0;
	virtual void				FreeAAS( idAASFile *file ) = 0;
};

extern idAASFileManager *		AASFileManager;

#endif /* !__AASFILEMANAGER_H__ */
