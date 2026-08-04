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
#version 330

in vec2 var_TexCoord;
out vec4 draw_Color;

uniform sampler2D u_diffuse;
uniform vec4 u_color;
uniform vec4 u_colorAdd;

uniform float u_alphaTest;

void main() {
	vec4 diffuse = texture(u_diffuse, var_TexCoord);
	if (u_alphaTest >= 0) {
		if (diffuse.a <= u_alphaTest)
			discard;
	}
	draw_Color.rgb = u_color.rgb * diffuse.rgb + u_colorAdd.rgb;
	draw_Color.a = diffuse.a;
}
