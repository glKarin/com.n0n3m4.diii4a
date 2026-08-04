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

in vec4 var_color;
in vec4 var_tc1;
in vec4 var_tc2;
out vec4 draw_Color;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;
uniform vec4 u_scaleDepthCoords;
uniform vec4 u_scalePotToWindow;
uniform vec4 u_scaleWindowToUnit;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// texture 0 is _currentRender
	// texture 1 is a normal map that we will use to deform texture 0
	// texture 2 is _currentDepth
	//
	// env[0] is the 1.0 to _currentRender conversion
	// env[1] is the fragment.position to 0.0 - 1.0 conversion
	//
	// Hard-coded constants
	//    depth_consts allows us to recover the original depth in Doom units of anything in the depth 
	//    buffer. TDM's projection matrix differs slightly from the classic projection matrix as it 
	//    implements a "nearly-infinite" zFar. The matrix is hard-coded in the engine, so we use hard-coded 
	//    constants here for efficiency. depth_consts is derived from the numbers in that matrix.
	//
	
	vec4 localNormal, R0;                                                                               //TEMP	localNormal, R0;
	
	vec4 subOne = vec4(-1, -1, -1, -1);                                                                 //PARAM	subOne = { -1, -1, -1, -1 };
	vec4 scaleTwo = vec4(2, 2, 2, 2);                                                                   //PARAM	scaleTwo = { 2, 2, 2, 2 };
	
	// load the filtered normal map and convert to -1 to 1 range
	localNormal = texture(u_texture1, var_tc1.xy);                                                      //TEX		localNormal, fragment.texcoord[1], texture[1], 2D;
//	localNormal.x = localNormal.a;                                                                      //MOV		localNormal.x, localNormal.a;
	localNormal = (localNormal) * (scaleTwo) + (subOne);                                                //MAD		localNormal, localNormal, scaleTwo, subOne;
	localNormal.z = sqrt(max(0, 1-localNormal.x*localNormal.x-localNormal.y*localNormal.y));
	
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
	vec4 altCoord, altColor, origCoord, chosenColor;                                                    //TEMP  	altCoord, altColor, origCoord, chosenColor;
	origCoord = (u_scalePotToWindow) * (u_scaleWindowToUnit);                                           //MUL		origCoord, program.env[0], program.env[1];		
	origCoord = (origCoord) * (gl_FragCoord.xyxy);                                                      //MUL		origCoord, origCoord, fragment.position.xyxy;	
	altCoord.xy = (R0.xy) - (origCoord.xy);                                                             //SUB	  	altCoord.xy, R0, origCoord; 	
	altCoord.zw = vec2(0.0);    //stgatilov: avoid uninitialized warning
	altCoord = (origCoord) + (-altCoord);                                                               //ADD   	altCoord, origCoord, -altCoord;	
	altColor = texture(u_texture0, altCoord.xy);                                                        //TEX   	altColor, altCoord, texture[0], 2D;
	
	// Test whether the samples came from the foreground, rejecting them if so.
	// Start with the original undistorted color. We'll use this if both samples came 
	// from the foreground.
	chosenColor = texture(u_texture0, origCoord.xy);                                                    //TEX		chosenColor, origCoord, texture[0], 2D;
	
	// Next 4 lines are needed for #4306 -- alpha depth
	// #4306 In each case record the depth of the sample in the alpha channel w
	vec4 depth;                                                                                         //TEMP	depth;
	origCoord.xy = (origCoord.xy) * (u_scaleDepthCoords.zw);                                            //MUL     origCoord.xy, origCoord, program.env[4].zwzw; 
	depth = texture(u_texture2, origCoord.xy);                                                          //TEX     depth, origCoord, texture[2], 2D;
	chosenColor.w = depth.x;                                                                            //MOV     chosenColor.w, depth.x;
	
	// Test whether the alt color came from the background, and use it in preference if so
	altCoord.xy = (altCoord.xy) * (u_scaleDepthCoords.zw);                                              //MUL     altCoord.xy, altCoord, program.env[4].zwzw; 
	depth = texture(u_texture2, altCoord.xy);                                                           //TEX		depth, altCoord, texture[2], 2D;
	altColor.w = depth.x;                                                                               //MOV     altColor.w, depth.x;  
	depth = (depth.xxxx) - (gl_FragCoord.zzzz);                                                         //SUB		depth, depth.x, fragment.position.z;
	chosenColor = mix(chosenColor, altColor, step(vec4(0.0), depth));                                   //CMP		chosenColor, depth, chosenColor, altColor;
	
	// Test whether the displaced color came from the background, and use it in preference if so
	R0.xy = (R0.xy) * (u_scaleDepthCoords.zw);                                                          //MUL     R0.xy, R0, program.env[4].zwzw;
	depth = texture(u_texture2, R0.xy);                                                                 //TEX		depth, R0, texture[2], 2D;
	displacedColor.w = depth.x;                                                                         //MOV     displacedColor.w, depth.x;  
	depth = (depth.xxxx) - (gl_FragCoord.zzzz);                                                         //SUB		depth, depth.x, fragment.position.z;
	chosenColor = mix(chosenColor, displacedColor, step(vec4(0.0), depth));                             //CMP		chosenColor, depth, chosenColor, displacedColor;
	
	// #4306 Calculate the opacity of the material using the depth difference between the 
	// transparent surface and the solid background
	
	vec4 depth_myconsts = vec4(2, -2, 0.0, 0.0);                                                        //PARAM   depth_consts = { 2, -2, 0.0, 0.0 }; 
	vec4 scene_depth, surface_depth, final_rgba;                                                        //TEMP    scene_depth, surface_depth, final_rgba;
	
	// Calculate original depth in doom units from the nonlinear depth buffer values we captured earlier
	chosenColor.w = min(chosenColor.w, 0.9994);                                                         //MIN	  chosenColor.w, chosenColor.w, 0.9994;	
	                                            
	                                            
	                                            
	scene_depth = (chosenColor.wwww) * (depth_myconsts.xxxx) + (depth_myconsts.yyyy);                   //MAD   scene_depth, chosenColor.w, depth_consts.x, depth_consts.y;
	scene_depth = vec4(1.0 / scene_depth.x);                                                            //RCP   scene_depth, scene_depth.x;
	
	// Find the depth of the transparent surface in doom units too
	surface_depth = (gl_FragCoord.zzzz) * (depth_myconsts.xxxx) + (depth_myconsts.yyyy);                //MAD   surface_depth, fragment.position.z, depth_consts.x, depth_consts.y;
	surface_depth = vec4(1.0 / surface_depth.x);                                                        //RCP   surface_depth, surface_depth.x;
	
	// Calculate the depth diff, divide by the opaque threshold and clamp
	// NB depths are negative. We use -tmp in the division to give a positive 
	// value where the surface is visible in front of the solid background
	final_rgba = (scene_depth) + (-surface_depth);                                                      //ADD       final_rgba, scene_depth, -surface_depth;
	final_rgba.a = clamp((-final_rgba.r) * (var_color.r), 0.0, 1.0);                                    //MUL_SAT   final_rgba.a, -final_rgba.r, fragment.color.r;   
	final_rgba.rgb = chosenColor.xyz;                                                                   //MOV       final_rgba.rgb, chosenColor;
	draw_Color = final_rgba;                                                                            //MOV       result.color, final_rgba;
	
}
