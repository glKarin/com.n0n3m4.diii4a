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

uniform highp float u_screenTex;
uniform sampler2D u_tex0;

in vec4 var_TexCoord0;
in vec4 var_Color;

out vec4 FragColor;

void main() {
	vec4 tex;
	if (u_screenTex == 1.0) {
		tex = var_TexCoord0;
		tex.xy /= tex.w;
		tex = tex * 0.5 + 0.5;
		tex = texture(u_tex0, tex.xy);
	}
	else {
		tex = textureProj(u_tex0, var_TexCoord0);
	}
	FragColor = tex * var_Color;
}
