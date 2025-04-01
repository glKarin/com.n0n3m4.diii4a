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

// window system independent camera view code

typedef struct
{
	int		width, height;

	idVec3	origin;			// at center of window
	float	scale;
} z_t;

extern z_t z;

void Z_Init (void);
void Z_MouseDown (int x, int y, int buttons);
void Z_MouseUp (int x, int y, int buttons);
void Z_MouseMoved (int x, int y, int buttons);
void Z_Draw (void);

