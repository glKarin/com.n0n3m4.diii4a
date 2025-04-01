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
#ifndef COLORBUTTON_H_
#define COLORBUTTON_H_

void		ColorButton_DrawItem	( HWND hWnd, LPDRAWITEMSTRUCT dis );
void		ColorButton_SetColor	( HWND hWnd, COLORREF color );
void		ColorButton_SetColor	( HWND hWnd, const char* color );
COLORREF	ColorButton_GetColor	( HWND hWnd );

void		AlphaButton_SetColor	( HWND hWnd, const char* color );

void		AlphaButton_OpenPopup	( HWND button );

#endif // COLORBUTTON_H_