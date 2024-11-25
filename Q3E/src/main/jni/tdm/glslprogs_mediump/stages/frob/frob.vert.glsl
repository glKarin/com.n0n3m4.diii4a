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

in vec3 attr_Normal;
in vec4 attr_TexCoord;
in vec4 attr_Position;

out vec2 var_TexCoord;

uniform float u_depth;
uniform vec4 u_texMatrix[2];

void main() {
	vec4 transformed = tdm_transform(attr_Position);
	transformed.z -= u_depth * transformed.w;
	gl_Position = transformed;
	var_TexCoord.x = dot(attr_TexCoord, u_texMatrix[0]);
	var_TexCoord.y = dot(attr_TexCoord, u_texMatrix[1]);
}
