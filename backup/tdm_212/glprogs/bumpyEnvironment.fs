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

uniform sampler2D u_normalTexture;
uniform samplerCube u_texture0;

in vec2 var_texCoord;
in vec3 var_toEyeWorld;
in mat3 var_TangentToWorldMatrix;

out vec4 draw_Color;

void main() {
	// per-pixel cubic reflextion map calculation

	// texture 0 is the environment cube map
	// texture 1 is the normal map

	// load the filtered normal map, then normalize to full scale,
	vec3 localNormal = texture(u_normalTexture, var_texCoord).rgb;
	localNormal = localNormal * 2 - vec3(1);
	localNormal.z = sqrt(max(0, 1 - localNormal.x * localNormal.x - localNormal.y * localNormal.y));
	localNormal = normalize(localNormal);

	// transform the surface normal by the local tangent space
	vec3 globalNormal = var_TangentToWorldMatrix * localNormal;

	// calculate reflection vector
	vec3 globalEye = normalize(var_toEyeWorld);
	float dotEN = dot(globalEye, globalNormal);
	vec3 globalReflect = 2 * (dotEN * globalNormal) - globalEye;

	// read the environment map with the reflection vector
	vec3 reflectedColor = texture(u_texture0, globalReflect).rgb;

	// calculate fresnel reflectance.
	float q = 1 - dotEN;
	float fresnel = 3.0 * q * q * q * q;
	reflectedColor *= (fresnel + 0.4);

	// tonemap to convert HDR values to range 0.0 - 1.0
	draw_Color.xyz = reflectedColor / (vec3(1.0) + reflectedColor);
}
