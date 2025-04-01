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
in vec4 var_tc3;
in vec4 var_tc4;
in vec4 var_tc5;
in vec4 var_tc7;
out vec4 draw_Color;
uniform sampler2D u_texture1;
uniform sampler2D u_texture2;
uniform sampler2D u_texture3;
uniform samplerCube u_texture0;

void main() {
	// OPTION ARB_precision_hint_fastest;
	
	// per-pixel cubic reflextion map calculation
	
	// texture 0 is the environment cube map
	// texture 1 is the normal map
	//
	// texCoord[0] is the normal map texcoord
	// texCoord[1] is the vector to the eye in global space
	// texCoord[3] is the surface tangent to global coordiantes
	// texCoord[4] is the surface bitangent to global coordiantes
	// texCoord[5] is the surface normal to global coordiantes
	
	vec4 globalEye, localNormal, globalNormal, R0, R1, R2, ambientContrib;                              //TEMP	globalEye, localNormal, globalNormal, R0, R1, R2, ambientContrib;
	vec4 colDiffuse, colSpecular, fresnelFactor, globalSkyDir1, globalSkyDir2;                          //TEMP	colDiffuse, colSpecular, fresnelFactor, globalSkyDir1, globalSkyDir2;
	
	vec4 subOne = vec4(-1.4, -1.4, -1, -1);                                                             //PARAM	subOne				= { -1.4, -1.4, -1, -1 };
	vec4 scaleTwo = vec4(2.8, 2.8, 2, 2);                                                               //PARAM	scaleTwo			= { 2.8, 2.8, 2, 2 };
	
	// Respectively : Ambient Rim Scale, Ambient constant Reflection Factor, Ambient Rim Reflection Scale; 
	vec4 rimStrength = vec4(1.0, .1, 2.0, 1.0);                                                         //PARAM	rimStrength		= {1.0, .1, 2.0 };	
	
	//  Respectively : unused, Ambient Reflection Scale
	vec4 ambientParms = vec4(0., 55, 0.0, 1.0);                                                         //PARAM	ambientParms	= {0., 55, 0.0 };
	
	vec4 texCoord = vec4(.1, .1, 0.0, 0.0);                                                             //PARAM	texCoord	= {.1, .1, 0.0, 0.0 };
	
	vec4 dirFromSky = vec4(.0, 0.0, 1.0, 1.0);                                                          //PARAM	dirFromSky		= { .0, 0.0, 1.0 };
	
	
	vec4 colGround = vec4(.44, .4, .4, 1.0);                                                            //PARAM	colGround				= { .44, .4, .4, 1.0 };    
	vec4 colSky = vec4(1.0, 1.0, 1.05, 1.0);                                                            //PARAM	colSky					= { 1.0, 1.0, 1.05, 1.0 };  
	
	
	// Load the filtered normal map, then normalize to full scale,
	localNormal = texture(u_texture1, var_tc0.zw);                                                      //TEX		localNormal, fragment.texcoord[0].zwxx, texture[1], 2D;
	localNormal.x = localNormal.a;                                                                      //MOV		localNormal.x, localNormal.a;				
	localNormal = (localNormal) * (scaleTwo) + (subOne);                                                //MAD		localNormal, localNormal, scaleTwo, subOne;
	R1 = vec4(dot(localNormal.xyz, localNormal.xyz));                                                   //DP3		R1, localNormal,localNormal;
	R1 = vec4(1.0 / sqrt(R1.x));                                                                        //RSQ		R1, R1.x;
	localNormal.xyz = (localNormal.xyz) * (R1.xyz);                                                     //MUL		localNormal.xyz, localNormal, R1;
	
	//  Load Diffuse map
	colDiffuse = texture(u_texture2, var_tc0.xy);                                                       //TEX		colDiffuse, fragment.texcoord[0].xyxx, texture[2], 2D;
	
	// Load Specular map
	colSpecular = texture(u_texture3, var_tc7.xy);                                                      //TEX		colSpecular, fragment.texcoord[7].xyxx, texture[3], 2D;
	R1 = (vec4(1)) - (colDiffuse);                                                                      //SUB		R1, 1, colDiffuse;
	colSpecular = (R1) * (vec4(0.17)) + (colSpecular);                                                  //MAD		colSpecular, R1, 0.17, colSpecular;			
	
	
	// transform the surface normal by the local tangent space 
	globalNormal.x = dot(localNormal.xyz, var_tc2.xyz);                                                 //DP3		globalNormal.x, localNormal, fragment.texcoord[2];
	globalNormal.y = dot(localNormal.xyz, var_tc3.xyz);                                                 //DP3		globalNormal.y, localNormal, fragment.texcoord[3];
	globalNormal.z = dot(localNormal.xyz, var_tc4.xyz);                                                 //DP3		globalNormal.z, localNormal, fragment.texcoord[4];
	globalNormal.w = 0.0;   //stgatilov: avoid uninitialized warning
	
	
	// normalize vector to eye
	R0 = vec4(dot(var_tc1.xyz, var_tc1.xyz));                                                           //DP3		R0, fragment.texcoord[1], fragment.texcoord[1];
	R0 = vec4(1.0 / sqrt(R0.x));                                                                        //RSQ		R0, R0.x;
	globalEye = (var_tc1) * (R0);                                                                       //MUL		globalEye, fragment.texcoord[1], R0;
	
	// calculate reflection vector
	R0 = vec4(dot(globalEye.xyz, globalNormal.xyz));                                                    //DP3		R0, globalEye, globalNormal;
	//-----------------------------------------
	//		Calculate fresnel reflectance.
	//-----------------------------------------
	fresnelFactor.x = clamp((1) - (R0.x), 0.0, 1.0);                                                    //SUB_SAT	fresnelFactor.x, 1, R0.x;
	fresnelFactor.x = (fresnelFactor.x) * (fresnelFactor.x);                                            //MUL		fresnelFactor.x, fresnelFactor.x, fresnelFactor.x;
	fresnelFactor.x = (fresnelFactor.x) * (fresnelFactor.x);                                            //MUL		fresnelFactor.x, fresnelFactor.x, fresnelFactor.x;
	//-----------------------------------------
	
	R0 = (R0) * (globalNormal);                                                                         //MUL		R0, R0, globalNormal;
	R0 = (R0) * (vec4(2.0)) + (-globalEye);                                                             //MAD		R0, R0, 2.0, -globalEye;
	
	// read the environment map with the reflection vector
	R0 = texture(u_texture0, R0.xyz);                                                                   //TEX		R0, R0, texture[0], CUBE;
	
	//---------------------------------------------------------
	// Calculate Hemispheric Ambient Lighting 
	//---------------------------------------------------------
	ambientContrib = vec4(dot(globalNormal.xyz, dirFromSky.xyz));                                       //DP3		ambientContrib, globalNormal, dirFromSky;
	ambientContrib = (ambientContrib) * (vec4(0.5)) + (vec4(0.5));                                      //MAD		ambientContrib, ambientContrib, 0.5, 0.5;	
	ambientContrib = mix(colGround, colSky, ambientContrib);                                            //LRP		ambientContrib, ambientContrib, colSky, colGround;
	
	
	// Modulate with ambient rim scale
	R1.x = (fresnelFactor.x) * (rimStrength.x);                                                         //MUL		R1.x, fresnelFactor.x, rimStrength.x;	
	R2 = (colSpecular) * (vec4(.6)) + (vec4(.4));                                                       //MAD		R2, colSpecular, .6, .4;
	R1 = (R1.xxxx) * (R2);                                                                              //MUL		R1, R1.x, R2;
	
	// Multiply with ambient reflection rim factor and add constant reflection factor
	fresnelFactor.x = (fresnelFactor.x) * (rimStrength.z) + (rimStrength.y);                            //MAD		fresnelFactor.x, fresnelFactor.x, rimStrength.z, rimStrength.y;	
	
	ambientContrib = (ambientContrib) * (R1) + (ambientContrib);                                        //MAD		ambientContrib, ambientContrib, R1, ambientContrib; 
	
	colSpecular = (colSpecular) * (colDiffuse);                                                         //MUL		colSpecular, colSpecular, colDiffuse;
	R1 = (colSpecular) * (fresnelFactor.xxxx);                                                          //MUL		R1,	colSpecular, fresnelFactor.xxxx; 
	R1 = min(vec4(0.04), R1);                                                                           //MIN		R1, 0.04, R1;
	R1 = (R1) * (R0);                                                                                   //MUL		R1,	R1, R0;
	R1 = (R1) * (ambientParms.yyyy);                                                                    //MUL		R1,	R1, ambientParms.y;				
	R0 = (ambientContrib) * (colDiffuse) + (R1);                                                        //MAD		R0, ambientContrib, colDiffuse, R1;			
	draw_Color.xyz = (R0.xyz) * (var_tc5.xyz);                                                          //MUL		result.color.xyz, R0,  fragment.texcoord[5];
	
	//Modulate by vertex color.
	//MUL		R0, R0, fragment.color;
	//---------------------------------------------------------
	// Tone Map to convert HDR values to range 0.0 - 1.0
	//---------------------------------------------------------
	//ADD		R1, 1.0, R0;
	//RCP		R1.x, R1.x;
	//RCP		R1.y, R1.y;
	//RCP		R1.z, R1.z;
	//---------------------------------------------------------
	
	//MUL		result.color.xyz, R0, R1;
	
}
