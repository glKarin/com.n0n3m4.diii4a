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
#include "macosx_local.h"

@class NSDictionary;

extern NSDictionary *Sys_GetMatchingDisplayMode( glimpParms_t parms );

extern void Sys_StoreGammaTables();
extern void Sys_GetGammaTable(glwgamma_t *table);
extern void Sys_SetScreenFade(glwgamma_t *table, float fraction);

extern void Sys_FadeScreens();
extern void Sys_FadeScreen(CGDirectDisplayID display);
extern void Sys_UnfadeScreens();
extern void Sys_UnfadeScreen(CGDirectDisplayID display, glwgamma_t *table);
extern void Sys_ReleaseAllDisplays();

