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
#pragma once

#include <minizip/unzip.h>

#ifdef __cplusplus
extern "C" {
#endif

/*#ifdef __ANDROID__
typedef u_int64_t ZPOS64_T;
#define unztell unzTell
#endif*/

extern unzFile unzReOpen( const char* path, unzFile file );
/* Re-Open a Zip file, i.e. clone an existing one and give it a new file descriptor. */

extern int ZEXPORT unzseek OF((unzFile file, z_off_t offset, int origin));
extern int ZEXPORT unzseek64 OF((unzFile file, ZPOS64_T offset, int origin));
/* Seek within the uncompressed data if compression method is storage. */

#ifdef __cplusplus
}
#endif
