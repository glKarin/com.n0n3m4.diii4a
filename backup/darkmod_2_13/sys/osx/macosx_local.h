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

#include <sys/wait.h>

#include "../sys_public.h"

void	OutputDebugString( const char *text );

// input
void 	Sys_InitInput( void );
void 	Sys_ShutdownInput( void );

void	IN_DeactivateMouse( void);
void	IN_ActivateMouse( void);

void	IN_Activate (bool active);
void	IN_Frame (void);

void * wglGetProcAddress(const char *name);

void	Sleep( const int time );

void	Sys_UpdateWindowMouseInputRect( void );
