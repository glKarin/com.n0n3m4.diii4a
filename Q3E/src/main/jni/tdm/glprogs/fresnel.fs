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
#version 140
// !!ARBfp1.0

in vec4 var_tc2;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	vec4 A, C;                                                                                          //TEMP A, C;
	
	A = (gl_FragCoord) * (u_scaleWindowToUnit);                                                         //MUL A, fragment.position, program.env[1];
	
	C = texture(u_texture0, A.xy);                                                                      //TEX C, A, texture[0], 2D;
	
	draw_Color = C;                                                                                     //MOV result.color, C;
	
	draw_Color.a = var_tc2.x;                                                                           //MOV result.color.a, fragment.texcoord[2].x;
	
}
