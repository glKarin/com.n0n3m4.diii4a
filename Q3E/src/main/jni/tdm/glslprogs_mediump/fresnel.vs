#version 320 es
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

precision mediump float;
// !!ARBvp1.0

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Normal;
in vec4 attr_Position;
out vec4 var_tc2;
uniform vec4 u_localParam0;
uniform vec4 u_viewOriginLocal;

void main() {
	// OPTION ARB_position_invariant;
	
	vec4 R1, R2, R3, R4;                                                                                //TEMP R1, R2, R3, R4;
	
	R1 = (u_viewOriginLocal) - (attr_Position);                                                         //SUB R1, program.env[5], vertex.position;
	R2 = vec4(dot(R1.xyz, R1.xyz));                                                                     //DP3 R2, R1, R1;
	R2 = vec4(1.0 / sqrt(R2.x));                                                                        //RSQ R2, R2.x;
	R1 = (R1) * (R2.xxxx);                                                                              //MUL R1, R1, R2.x;
	
	R2 = vec4(dot(R1.xyz, vec4(attr_Normal, 1).xyz));                                                   //DP3 R2, R1, vertex.normal;
	
	R3 = (vec4(1.0)) - (R2);                                                                            //SUB R3, 1.0, R2;
	
	R4 = (R3) * (R3);                                                                                   //MUL R4, R3, R3;
	
	R4 = (R4) * (R3);                                                                                   //MUL R4, R4, R3;
	
	R4 = (R4) * (R3);                                                                                   //MUL R4, R4, R3;
	
	R4 = (R4) * (R3);                                                                                   //MUL R4, R4, R3;
	
	R3 = (vec4(1.0)) - (u_localParam0);                                                                 //SUB R3, 1.0, program.local[0];
	
	var_tc2.x = (R4.x) * (R3.x) + (u_localParam0.x);                                                    //MAD result.texcoord[2].x, R4, R3, program.local[0];
	
	gl_Position = tdm_transform(attr_Position);
}
