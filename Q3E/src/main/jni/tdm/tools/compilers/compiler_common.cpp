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

#include "compiler_common.h"

bool FindMapFile(idStr &mapfile) {
	idStr passedName = mapfile;		// may have an extension
	passedName.BackSlashesToSlashes();
	if ( passedName.Icmpn( "maps/", 4 ) != 0 ) {
		passedName = "maps/" + passedName;
	}

	// taaaki - support map files from darkmod/fms/<mission>/maps as well as darkmod/maps
	//          this is done by opening the file to get the true full path, then converting
	//          the path back to a RelativePath based off fs_devpath
	passedName.SetFileExtension( "map" );
	idFile *fp = idLib::fileSystem->OpenFileRead( passedName, "" );
	if ( fp ) {
		mapfile = idLib::fileSystem->OSPathToRelativePath(fp->GetFullPath());
		idLib::fileSystem->CloseFile( fp );
		return true;
	}
	else {
		mapfile = passedName;
		return false;
	}
}
