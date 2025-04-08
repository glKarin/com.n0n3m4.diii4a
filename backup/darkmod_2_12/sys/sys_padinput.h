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

extern idCVar in_padInverseRX;
extern idCVar in_padInverseRY;

void Sys_InitPadInput();
int Sys_PollGamepadInputEvents();
int Sys_ReturnGamepadInputEvent( int n, int &type, int &id, int &value );
void Sys_GetCombinedAxisDeflection( int &x, int &y );
void Sys_EndGamepadInputEvents();
