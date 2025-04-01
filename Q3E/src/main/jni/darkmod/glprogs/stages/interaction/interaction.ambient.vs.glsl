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
#version 330 core

#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "tdm_interaction.glsl"
#pragma tdm_include "tdm_utils.glsl"

in vec4 attr_Position;
in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec4 attr_Color;

uniform int u_flags;

out vec2 var_TexNormal;
out vec2 var_TexParallax;
out vec2 var_TexDiffuse;
out vec2 var_TexSpecular;
out vec2 var_TexCoord;
out vec4 var_TexLight;
out vec4 var_Color;
out mat3 var_TangentBitangentNormalMatrix;
out vec3 var_worldViewDir;
out vec3 var_LightDirLocal;
out vec3 var_ViewDirLocal;

uniform vec3 u_globalViewOrigin;
uniform vec3 u_globalLightOrigin;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec4 u_bumpMatrix[2];
uniform vec4 u_parallaxMatrix[2];
uniform vec4 u_diffuseMatrix[2];
uniform vec4 u_specularMatrix[2];
uniform mat4 u_lightProjectionFalloff;
uniform vec4 u_colorModulate;
uniform vec4 u_colorAdd;
uniform mat4 u_modelMatrix;

void main( void ) {
	// transform vertex position into homogenous clip-space
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);

	// surface texcoords, tangent space, and color generation
	generateSurfaceProperties(
		attr_TexCoord, attr_Color,
		attr_Tangent, attr_Bitangent, attr_Normal,
		u_bumpMatrix, u_parallaxMatrix, u_diffuseMatrix, u_specularMatrix,
		u_colorModulate, u_colorAdd,
		u_flags,
		var_TexNormal, var_TexParallax, var_TexDiffuse, var_TexSpecular,
		var_Color, var_TangentBitangentNormalMatrix
	);
	var_TexCoord = attr_TexCoord.xy;

	// light projection texgen
	var_TexLight = computeLightTex(u_lightProjectionFalloff, attr_Position);

	var_worldViewDir = u_globalViewOrigin - (u_modelMatrix * attr_Position).xyz;

	var_LightDirLocal = (worldPosToObject(u_globalLightOrigin, u_modelMatrix) - attr_Position.xyz) * var_TangentBitangentNormalMatrix;
	var_ViewDirLocal = (worldPosToObject(u_globalViewOrigin, u_modelMatrix) - attr_Position.xyz) * var_TangentBitangentNormalMatrix;	
}
