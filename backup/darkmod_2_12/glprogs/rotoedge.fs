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
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	vec4 huetemp;                                                                                       //TEMP huetemp;
	
	
	//sobel edge filter algorithm by John Rittenhouse
	
	vec4 R0, R1, R2, R3, R4, R5, R6, XEdge, YEdge, sobel;                                               //TEMP	R0,R1,R2,R3,R4,R5,R6, XEdge,YEdge,sobel;
	
	vec4 off00 = vec4(-0.5, -0.5, 0, 0);                                                                //PARAM off00 = {-0.5, -0.5, 0, 0 };
	vec4 off01 = vec4(-0.5, 0.0, 0, 0);                                                                 //PARAM off01 = {-0.5,  0.0, 0, 0 };
	vec4 off02 = vec4(-0.5, 0.5, 0, 0);                                                                 //PARAM off02 = {-0.5,  0.5, 0, 0 };
	vec4 off03 = vec4(0.5, -0.5, 0, 0);                                                                 //PARAM off03 = { 0.5, -0.5, 0, 0 };
	vec4 off04 = vec4(0.5, 0.0, 0, 0);                                                                  //PARAM off04 = { 0.5,  0.0, 0, 0 };
	vec4 off05 = vec4(0.5, 0.5, 0, 0);                                                                  //PARAM off05 = { 0.5,  0.5, 0, 0 };
	vec4 off06 = vec4(0.0, -0.5, 0, 0);                                                                 //PARAM off06 = { 0.0, -0.5, 0, 0 };
	vec4 off07 = vec4(0.0, 0.5, 0, 0);                                                                  //PARAM off07 = { 0.0,  0.5, 0, 0 };
	
	vec4 specExp = vec4(0.5, 0, 0, 0);                                                                  //PARAM specExp = { 0.5, 0, 0, 0 };
	
	// calculate the screen texcoord in the 0.0 to 1.0 range
	R0 = (gl_FragCoord) * (u_scaleWindowToUnit);                                                        //MUL		R0, fragment.position, program.env[1];
	
	// scale by the screen non-power-of-two-adjust to give us normal screen coords in R0
	R0 = (R0) * (u_scalePotToWindow);                                                                   //MUL		R0, R0, program.env[0];
	
	// Calculate offsets
	R1 = (var_tc0) * (off01) + (R0);                                                                    //MAD		R1, fragment.texcoord[0], off01, R0;
	R2 = (var_tc0) * (off04) + (R0);                                                                    //MAD		R2, fragment.texcoord[0], off04, R0;
	R3 = (var_tc0) * (off06) + (R0);                                                                    //MAD		R3, fragment.texcoord[0], off06, R0;
	R4 = (var_tc0) * (off07) + (R0);                                                                    //MAD		R4, fragment.texcoord[0], off07, R0;
	
	// Look at the values along the center
	R1 = texture(u_texture0, R1.xy);                                                                    //TEX		R1, R1, texture[0], 2D;
	R3 = texture(u_texture0, R3.xy);                                                                    //TEX		R3, R3, texture[0], 2D;
	R2 = texture(u_texture0, R2.xy);                                                                    //TEX		R2, R2, texture[0], 2D;
	R4 = texture(u_texture0, R4.xy);                                                                    //TEX		R4, R4, texture[0], 2D;
	
	// Each of them have a scale value of two in
	XEdge = R1;                                                                                         //MOV		XEdge, R1;
	YEdge = R3;                                                                                         //MOV		YEdge, R3;
	XEdge = (XEdge) + (R1);                                                                             //ADD		XEdge, XEdge, R1;
	YEdge = (YEdge) + (R3);                                                                             //ADD		YEdge, YEdge, R3;
	XEdge = (XEdge) - (R2);                                                                             //SUB		XEdge, XEdge, R2;
	YEdge = (YEdge) - (R4);                                                                             //SUB		YEdge, YEdge, R4;
	XEdge = (XEdge) - (R2);                                                                             //SUB		XEdge, XEdge, R2;
	YEdge = (YEdge) - (R4);                                                                             //SUB		YEdge, YEdge, R4;
	
	// Calculate offsets for the corners
	R1 = (var_tc0) * (off00) + (R0);                                                                    //MAD		R1, fragment.texcoord[0], off00, R0;
	R2 = (var_tc0) * (off02) + (R0);                                                                    //MAD		R2, fragment.texcoord[0], off02, R0;
	R3 = (var_tc0) * (off03) + (R0);                                                                    //MAD		R3, fragment.texcoord[0], off03, R0;
	R4 = (var_tc0) * (off05) + (R0);                                                                    //MAD		R4, fragment.texcoord[0], off05, R0;
	
	// Look at the values along the center
	R1 = texture(u_texture0, R1.xy);                                                                    //TEX		R1, R1, texture[0], 2D;
	R2 = texture(u_texture0, R2.xy);                                                                    //TEX		R2, R2, texture[0], 2D;
	R3 = texture(u_texture0, R3.xy);                                                                    //TEX		R3, R3, texture[0], 2D;
	R4 = texture(u_texture0, R4.xy);                                                                    //TEX		R4, R4, texture[0], 2D;
	
	XEdge = (XEdge) + (R1);                                                                             //ADD		XEdge, XEdge, R1;
	YEdge = (YEdge) + (R1);                                                                             //ADD		YEdge, YEdge, R1;
	XEdge = (XEdge) + (R2);                                                                             //ADD		XEdge, XEdge, R2;
	YEdge = (YEdge) - (R2);                                                                             //SUB		YEdge, YEdge, R2;
	XEdge = (XEdge) - (R3);                                                                             //SUB		XEdge, XEdge, R3;
	YEdge = (YEdge) + (R3);                                                                             //ADD		YEdge, YEdge, R3;
	XEdge = (XEdge) - (R4);                                                                             //SUB		XEdge, XEdge, R4;
	YEdge = (YEdge) - (R4);                                                                             //SUB		YEdge, YEdge, R4;
	
	//Square each and add and take the sqrt
	XEdge = (XEdge) * (XEdge);                                                                          //MUL		XEdge, XEdge, XEdge;
	YEdge = (YEdge) * (YEdge);                                                                          //MUL		YEdge, YEdge, YEdge;
	R1 = (XEdge) + (YEdge);                                                                             //ADD		R1, XEdge, YEdge;
	
	R1.x = max(R1.x, R1.y);                                                                             //MAX		R1.x, R1.x, R1.y;
	R1.x = max(R1.x, R1.z);                                                                             //MAX		R1.x, R1.x, R1.z;
	
	//##write result from edge detection into sobel variable
	//POW		sobel, R1.x, specExp.x;
	
	sobel = R1;                                                                                         //MOV         sobel,R1;
	
	//##load texture information
	R0 = texture(u_texture0, R0.xy);                                                                    //TEX R0, R0, texture[0], 2D;
	
	//##normalize the color hue
	
	//calculate [sum RGB]
	huetemp.x = (R0.x) + (R0.y);                                                                        //ADD huetemp.x, R0.x, R0.y;
	huetemp.x = (huetemp.x) + (R0.z);                                                                   //ADD huetemp.x, huetemp.x, R0.z;
	
	//calculate 1/[sum RGB]
	huetemp.y = 1.0 / huetemp.x;                                                                        //RCP huetemp.y, huetemp.x;
	
	//multiply pixel color with 1/[sum RGB]
	R0 = (R0) * (huetemp.yyyy);                                                                         //MUL R0, R0, huetemp.y;
	
	//##calculate the pixel intensity
	
	// make everything with [sum RGB] <.2 black
	
	// define darktones ([sum RGB] >.2) and store in channel z
	huetemp.z = float((huetemp.x) >= (.2));                                                             //SGE huetemp.z, huetemp.x, .2;
	// darktones will be normalized HUE * 0.5
	huetemp.z = (huetemp.z) * (0.5);                                                                    //MUL huetemp.z, huetemp.z, 0.5;
	
	// define midtones ([sum RGB] >.8) and store in channel y
	huetemp.y = float((huetemp.x) >= (0.8));                                                            //SGE huetemp.y, huetemp.x, 0.8;
	// midtones will be normalized HUE * (0.5+0.5)
	huetemp.y = (huetemp.y) * (0.5);                                                                    //MUL huetemp.y, huetemp.y, 0.5;
	
	// define brighttones ([sum RGB] >1.5) and store in channel x
	huetemp.x = float((huetemp.x) >= (1.5));                                                            //SGE huetemp.x, huetemp.x, 1.5;
	// brighttones will be normalized HUE * (0.5+0.5+1.5)
	huetemp.x = (huetemp.x) * (1.5);                                                                    //MUL huetemp.x, huetemp.x, 1.5;
	
	// sum darktones+midtones+brighttones to calculate final pixel intensity
	huetemp.x = (huetemp.x) + (huetemp.y);                                                              //ADD huetemp.x, huetemp.x, huetemp.y;
	huetemp.x = (huetemp.x) + (huetemp.z);                                                              //ADD huetemp.x, huetemp.x, huetemp.z;
	
	//multiply pixel color with rotoscope intensity
	R0 = (R0) * (huetemp.xxxx);                                                                         //MUL R0, R0, huetemp.x;
	
	//generate edge mask -> make 1 for all regions not belonging to an edge
	//use 0.8 as threshold for edge detection
	sobel.x = float((sobel.x) < (0.1));                                                                 //SLT sobel.x, sobel.x, 0.1;
	
	//blend white into edge artifacts (occuring in bright areas)
	//find pixels with intensity>2
	sobel.y = float((huetemp.x) >= (2));                                                                //SGE sobel.y, huetemp.x, 2;
	//set the edge mask to 1 in such areas
	sobel.x = max(sobel.x, sobel.y);                                                                    //MAX sobel.x, sobel.x, sobel.y;
	
	//multiply cleaned up edge mask with final color image
	R0 = (R0) * (sobel.xxxx);                                                                           //MUL R0, R0, sobel.x;
	
	//transfer image to output
	draw_Color = R0;                                                                                    //MOV result.color, R0;
	//MOV result.color, sobel.x;
	
}
