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

#ifndef SPINBUTTON_H_
#define SPINBUTTON_H_

void SpinButton_SetIncrement ( HWND hWnd, float inc );
void SpinButton_HandleNotify ( NMHDR* hdr );
void SpinButton_SetRange	 ( HWND hWnd, float min, float max );

#endif // SPINBUTOTN_H_
