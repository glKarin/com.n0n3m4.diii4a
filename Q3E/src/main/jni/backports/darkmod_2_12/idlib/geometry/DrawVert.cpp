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


/*
=============
idDrawVert::Normalize
=============
*/
void idDrawVert::Normalize( void ) {
	normal.Normalize();
	tangents[1].Cross( normal, tangents[0] );
	tangents[1].Normalize();
	tangents[0].Cross( tangents[1], normal );
	tangents[0].Normalize();
}
