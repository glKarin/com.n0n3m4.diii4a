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

in vec4 var_tc0;
in vec4 var_tc1;
in vec4 var_tc2;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// texture 0 is _currentRender
	// texture 1 is a normal map that we will use to deform texture 0
	// texture 2 is a mask texture
	//
	// env[0] is the 1.0 to _currentRender conversion
	// env[1] is the fragment.position to 0.0 - 1.0 conversion
	
	vec4 localNormal, mask, pos0, pos1, pos2, pos3, pos4;                                               //TEMP	localNormal, mask, pos0, pos1, pos2, pos3, pos4;
	vec4 coord0, coord1, coord2, coord3, coord4;                                                        //TEMP	coord0, coord1, coord2, coord3, coord4;
	vec4 input0, input1, input2, input3, input4;                                                        //TEMP	input0, input1, input2, input3, input4;
	
	vec4 subOne = vec4(-1, -1, -1, -1);                                                                 //PARAM	subOne = { -1, -1, -1, -1 };
	vec4 scaleTwo = vec4(2, 2, 2, 2);                                                                   //PARAM	scaleTwo = { 2, 2, 2, 2 };
	
	// load the distortion map
	mask = texture(u_texture2, var_tc0.xy);                                                             //TEX		mask, fragment.texcoord[0], texture[2], 2D;
	
	// kill the pixel if the distortion wound up being very small
	mask.xy = (mask.xy) - (vec2(0.01));                                                                 //SUB		mask.xy, mask, 0.01;
	if (any(lessThan(mask, vec4(0.0)))) discard;                                                        //KIL		mask;
	
	// load the filtered normal map and convert to -1 to 1 range
	localNormal = texture(u_texture1, var_tc1.xy);                                                      //TEX		localNormal, fragment.texcoord[1], texture[1], 2D;
//	localNormal.x = localNormal.a;                                                                      //MOV		localNormal.x, localNormal.a;
	localNormal = (localNormal) * (scaleTwo) + (subOne);                                                //MAD		localNormal, localNormal, scaleTwo, subOne;
	localNormal.z = sqrt(max(0, 1-localNormal.x*localNormal.x-localNormal.y*localNormal.y));
	localNormal = (localNormal) * (mask);                                                               //MUL		localNormal, localNormal, mask;
	
	// calculate the screen texcoord in the 0.0 to 1.0 range
	
	// greebo: Initialise the positions
	pos0 = gl_FragCoord;                                                                                //MOV pos0, fragment.position;
	pos1 = pos0;                                                                                        //MOV pos1, pos0;
	pos2 = pos0;                                                                                        //MOV pos2, pos0;
	pos3 = pos0;                                                                                        //MOV pos3, pos0;
	pos4 = pos0;                                                                                        //MOV pos4, pos0;
	
	// greebo: Offset the positions by a certain amount to the left/right/top/bottom
	pos1.y = (pos1.y) + (-3);                                                                           //ADD pos1.y, pos1.y, -3;
	pos2.x = (pos2.x) + (3);                                                                            //ADD pos2.x, pos2.x, 3;
	pos3.y = (pos3.y) + (4);                                                                            //ADD pos3.y, pos3.y, 4;
	pos4.x = (pos4.x) + (-5);                                                                           //ADD pos4.x, pos4.x, -5;
	
	// convert pixel's screen position to a fraction of the screen width & height
	// fraction will be between 0.0 and 1.0.
	// result is stored in temp1.
	coord0 = (pos0) * (u_scaleWindowToUnit);                                                            //MUL  coord0, pos0, program.env[1];
	coord1 = (pos1) * (u_scaleWindowToUnit);                                                            //MUL  coord1, pos1, program.env[1];
	coord2 = (pos2) * (u_scaleWindowToUnit);                                                            //MUL  coord2, pos2, program.env[1];
	coord3 = (pos3) * (u_scaleWindowToUnit);                                                            //MUL  coord3, pos3, program.env[1];
	coord4 = (pos4) * (u_scaleWindowToUnit);                                                            //MUL  coord4, pos4, program.env[1];
	
	// scale by the screen non-power-of-two-adjust
	coord0 = (coord0) * (u_scalePotToWindow);                                                           //MUL		coord0, coord0, program.env[0];
	coord1 = (coord1) * (u_scalePotToWindow);                                                           //MUL		coord1, coord1, program.env[0];
	coord2 = (coord2) * (u_scalePotToWindow);                                                           //MUL		coord2, coord2, program.env[0];
	coord3 = (coord3) * (u_scalePotToWindow);                                                           //MUL		coord3, coord3, program.env[0];
	coord4 = (coord4) * (u_scalePotToWindow);                                                           //MUL		coord4, coord4, program.env[0];
	
	// offset by the scaled localNormal and clamp it to 0.0 - 1.0
	coord0 = clamp((localNormal) * (var_tc2) + (coord0), 0.0, 1.0);                                     //MAD_SAT	coord0, localNormal, fragment.texcoord[2], coord0;
	coord1 = clamp((localNormal) * (var_tc2) + (coord1), 0.0, 1.0);                                     //MAD_SAT	coord1, localNormal, fragment.texcoord[2], coord1;
	coord2 = clamp((localNormal) * (var_tc2) + (coord2), 0.0, 1.0);                                     //MAD_SAT	coord2, localNormal, fragment.texcoord[2], coord2;
	coord3 = clamp((localNormal) * (var_tc2) + (coord3), 0.0, 1.0);                                     //MAD_SAT	coord3, localNormal, fragment.texcoord[2], coord3;
	coord4 = clamp((localNormal) * (var_tc2) + (coord4), 0.0, 1.0);                                     //MAD_SAT	coord4, localNormal, fragment.texcoord[2], coord4;
	
	// load the screen render
	input0 = texture(u_texture0, coord0.xy);                                                            //TEX		input0, coord0, texture[0], 2D;
	input1 = texture(u_texture0, coord1.xy);                                                            //TEX		input1, coord1, texture[0], 2D;
	input2 = texture(u_texture0, coord2.xy);                                                            //TEX		input2, coord2, texture[0], 2D;
	input3 = texture(u_texture0, coord3.xy);                                                            //TEX		input3, coord3, texture[0], 2D;
	input4 = texture(u_texture0, coord4.xy);                                                            //TEX		input4, coord4, texture[0], 2D;
	
	// greebo: Average the values and pump it into the fragment's color
	input0 = (input0) + (input1);                                                                       //ADD input0, input0, input1;
	input0 = (input0) + (input2);                                                                       //ADD input0, input0, input2;
	input0 = (input0) + (input3);                                                                       //ADD input0, input0, input3;
	input0 = (input0) + (input4);                                                                       //ADD input0, input0, input4;
	input0 = (input0) * (vec4(0.2));                                                                    //MUL input0, input0, 0.2;
	
	//draw_Color.xyz = input0.xyz;                                                                        //MOV		result.color.xyz, input0;
	draw_Color = input0;                                                                        //MOV		result.color.xyz, input0;
	
}
