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

uniform mat4 u_textureMatrix;
uniform vec4 u_colorMul;
uniform vec4 u_colorAdd;
uniform highp float u_screenTex;

in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec4 attr_Color;

out vec4 var_TexCoord0;
out vec4 var_Color;

void main() {
	gl_Position = tdm_transform(attr_Position);
	var_Color = attr_Color * u_colorMul + u_colorAdd;
	if (u_screenTex == 1.0)
		var_TexCoord0 = gl_Position;
	else
		var_TexCoord0 = u_textureMatrix * vec4(attr_TexCoord, 0, 1);
}
