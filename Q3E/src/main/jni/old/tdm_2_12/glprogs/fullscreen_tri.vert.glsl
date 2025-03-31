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
#version 330

out vec2 var_TexCoord;

void main() {
    var_TexCoord.x = gl_VertexID == 1 ? 2 : 0;
    var_TexCoord.y = gl_VertexID == 2 ? 2 : 0;
    
    gl_Position = vec4(var_TexCoord * 2 - 1, 1, 1);
}
