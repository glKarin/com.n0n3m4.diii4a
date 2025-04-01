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

uniform samplerCube u_texture0;

in vec3 var_normalLocal;
in vec3 var_toEyeLocal;
in vec4 var_color;

out vec4 draw_Color;

void main() {
	vec3 normal = normalize(var_normalLocal);
	vec3 toEye = normalize(var_toEyeLocal);

	// calculate reflection vector
	float dotEN = dot(toEye, normal);
	vec3 reflectionVector = 2 * normal * dotEN - toEye;

	draw_Color = texture(u_texture0, reflectionVector) * var_color;
}
