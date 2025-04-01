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

/**
 * greebo: We just wrap to the central precompiled.h in idlib/. We do this
 * to force gcc to create a separate precompiled header for each project,
 * as they have different compiler settings and switches. gcc will put the
 * precompiled.h.gch file right next to this one, so we can't just use
 * the one in idlib/, as the .gch file would be placed right there, 
 * overwriting the .gch file of the previous run.
 */
#include "../idlib/precompiled.h"
