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

#pragma tdm_include "tdm_compression.glsl"

uniform samplerCube u_environmentMap;
uniform sampler2D u_normalMap;
uniform bool u_RGTC;
uniform vec4 u_constant;
uniform vec4 u_fresnel;
uniform bool u_tonemapOutputColor;

in vec2 var_TexCoord;
in vec3 var_ToEyeWorld;
in mat3 var_TangentToWorldMatrix;

out vec4 FragColor;

void main() {
	vec3 normalTangent = unpackSurfaceNormal(texture(u_normalMap, var_TexCoord), true, u_RGTC);
	vec3 normalWorld = normalize(var_TangentToWorldMatrix * normalTangent);

	// calculate reflection vector
	vec3 eyeWorld = normalize(var_ToEyeWorld);
	float dotEN = dot(eyeWorld, normalWorld);
	vec3 reflectWorld = 2 * dotEN * normalWorld - eyeWorld;

	// read the environment map with the reflection vector
	vec4 reflectedColor = texture(u_environmentMap, reflectWorld);

	// calculate fresnel reflectance
	float q = 1 - dotEN;
	reflectedColor *= u_fresnel * (q * q * q * q) + u_constant;

	if (u_tonemapOutputColor) {
		// tonemap to convert HDR values to range 0.0 - 1.0
		reflectedColor.rgb = reflectedColor.rgb / (vec3(1.0) + reflectedColor.rgb);
	}
	FragColor = reflectedColor;
}
