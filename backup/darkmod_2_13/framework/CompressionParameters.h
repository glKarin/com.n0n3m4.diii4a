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
#ifndef COMPRESSION_PARAMETERS_H
#define COMPRESSION_PARAMETERS_H

//stgatilov: Some types of files should not be compressed by pk4
//this header is used both in FileSystem and in tdm_update
static const char* PK4_UNCOMPRESSED_EXTENSIONS[] = {
	"ogg",	//4504
	"roq",	//4507
	"avi", "mp4", "m4v",	//4519
	"jpg", "png"	//5665
};
static const int PK4_UNCOMPRESSED_EXTENSIONS_COUNT = sizeof(PK4_UNCOMPRESSED_EXTENSIONS) / sizeof(PK4_UNCOMPRESSED_EXTENSIONS[0]);

#endif
