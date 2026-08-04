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

#include "precompiled.h"
#pragma hdrstop



#include "AASFile.h"
#include "AASFile_local.h"

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class idAASFileManagerLocal : public idAASFileManager {
public:
	virtual						~idAASFileManagerLocal( void ) override {}

	virtual idAASFile *			LoadAAS( const char *fileName, const unsigned int mapFileCRC ) override;
	virtual void				FreeAAS( idAASFile *file ) override;
};

idAASFileManagerLocal			AASFileManagerLocal;
idAASFileManager *				AASFileManager = &AASFileManagerLocal;


/*
================
idAASFileManagerLocal::LoadAAS
================
*/
idAASFile *idAASFileManagerLocal::LoadAAS( const char *fileName, const unsigned int mapFileCRC ) {
	idAASFileLocal *file = new idAASFileLocal();
	if ( !file->Load( fileName, mapFileCRC ) ) {
		delete file;
		return NULL;
	}
	return file;
}

/*
================
idAASFileManagerLocal::FreeAAS
================
*/
void idAASFileManagerLocal::FreeAAS( idAASFile *file ) {
	delete file;
}
