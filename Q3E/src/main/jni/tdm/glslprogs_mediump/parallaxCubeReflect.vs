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

#pragma tdm_include "tdm_transform.glsl"

in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec3 attr_Tangent;
in vec4 attr_Color;
in vec4 attr_Position;
in vec2 attr_TexCoord;

out vec2 var_uvNormal;
out vec3 var_globalEye;
out mat3 var_tbn;
out vec3 var_worldPos;
out vec3 var_cubeMapCapturePos;
out vec3 var_proxyAABBMin;
out vec3 var_proxyAABBMax;
out float var_normalWeight;

uniform mat4 u_modelMatrix;
uniform vec4 u_viewOriginLocal;
uniform vec4 u_localParam0;  // cubemap capture position, normal weight factor
uniform vec4 u_localParam1;  // geometry proxy AABB min
uniform vec4 u_localParam2;  // geometry proxy AABB max

void main() {
    var_uvNormal = attr_TexCoord;
    
    mat3 localToGlobal = mat3(u_modelMatrix);
	
	// calculate the vector to the eye in global coordinates
	vec3 localEye = u_viewOriginLocal.xyz - attr_Position.xyz;
    var_globalEye = localToGlobal * localEye;
	
    // calculate global TBN space
    vec3 globalTangent = localToGlobal * attr_Tangent;
    vec3 globalBitangent = localToGlobal * attr_Bitangent;
    vec3 globalNormal = localToGlobal * attr_Normal;
    var_tbn = mat3(globalTangent, globalBitangent, globalNormal);
    var_worldPos = (u_modelMatrix * attr_Position).xyz;
    
    var_cubeMapCapturePos = u_localParam0.xyz;
    var_normalWeight = u_localParam0.w;
    var_proxyAABBMin = u_localParam1.xyz;
    var_proxyAABBMax = u_localParam2.xyz;
	
	gl_Position = tdm_transform(attr_Position);
}
