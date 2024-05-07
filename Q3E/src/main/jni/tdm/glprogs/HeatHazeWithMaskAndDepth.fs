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
uniform sampler2D u_texture3;
uniform vec4 u_scaleDepthCoords;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	// texture 0 is _currentRender
	// texture 1 is a normal map that we will use to deform texture 0
	// texture 2 is a mask texture
	// texture 3 is _currentDepth
	//
	// env[0] is the 1.0 to _currentRender conversion
	// env[1] is the fragment.position to 0.0 - 1.0 conversion
	// env[4] holds the _currentDepth mapping constants
	
	vec4 localNormal, mask, R0;                                                                         //TEMP	localNormal, mask, R0;
	
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
	R0 = (gl_FragCoord) * (u_scaleWindowToUnit);                                                        //MUL		R0, fragment.position, program.env[1];
	
	// offset by the scaled localNormal and clamp it to 0.0 - 1.0
	R0 = clamp((localNormal) * (var_tc2) + (R0), 0.0, 1.0);                                             //MAD_SAT	R0, localNormal, fragment.texcoord[2], R0;
	
	// scale by the screen non-power-of-two-adjust
	R0 = (R0) * (u_scalePotToWindow);                                                                   //MUL		R0, R0, program.env[0];
	
	// Get the displaced color
	vec4 displacedColor;                                                                                //TEMP 	displacedColor;
	displacedColor = texture(u_texture0, R0.xy);                                                        //TEX		displacedColor, R0, texture[0], 2D;
	
	// Fix foreground object distortion. Get another sample from the opposite direction, 
	// to use if the sample came from the foreground. 
	vec4 altCoord, altColor, origCoord;                                                                 //TEMP  	altCoord, altColor, origCoord;
	origCoord = (u_scalePotToWindow) * (u_scaleWindowToUnit);                                           //MUL		origCoord, program.env[0], program.env[1];		
	origCoord = (origCoord) * (gl_FragCoord.xyxy);                                                      //MUL		origCoord, origCoord, fragment.position.xyxy;	
	altCoord.xy = (R0.xy) - (origCoord.xy);                                                             //SUB	  	altCoord.xy, R0, origCoord; 	
	altCoord.zw = vec2(0.0);    //stgatilov: avoid uninitialized warning
	altCoord = (origCoord) + (-altCoord);                                                               //ADD   	altCoord, origCoord, -altCoord;	
	altColor = texture(u_texture0, altCoord.xy);                                                        //TEX   	altColor, altCoord, texture[0], 2D;
	
	// Test whether the samples came from the foreground, rejecting them if so.
	// Start with the original undistorted color. We'll use this if both samples came 
	// from the foreground.
	vec4 depth, chosenColor;                                                                            //TEMP	depth, chosenColor;
	chosenColor = texture(u_texture0, origCoord.xy);                                                    //TEX		chosenColor, origCoord, texture[0], 2D;
	// Test whether the alt color came from the background, and use it in preference if so
	altCoord.xy = (altCoord.xy) * (u_scaleDepthCoords.zw);                                              //MUL     altCoord.xy, altCoord, program.env[4].zwzw; 
	depth = texture(u_texture3, altCoord.xy);                                                           //TEX		depth, altCoord, texture[3], 2D;
	depth = (depth.zzzz) - (gl_FragCoord.zzzz);                                                         //SUB		depth, depth.z, fragment.position.z;
	chosenColor = mix(chosenColor, altColor, step(vec4(0.0), depth));                                   //CMP		chosenColor, depth, chosenColor, altColor;
	// Test whether the displaced color came from the background, and use it in preference if so
	R0.xy = (R0.xy) * (u_scaleDepthCoords.zw);                                                          //MUL     R0.xy, R0, program.env[4].zwzw;
	depth = texture(u_texture3, R0.xy);                                                                 //TEX		depth, R0, texture[3], 2D;
	depth = (depth.zzzz) - (gl_FragCoord.zzzz);                                                         //SUB		depth, depth.z, fragment.position.z;
	draw_Color = mix(chosenColor, displacedColor, step(vec4(0.0), depth));                              //CMP		result.color, depth, chosenColor, displacedColor;
	
}
