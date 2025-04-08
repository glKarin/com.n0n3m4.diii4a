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

in vec2 var_TexCoord;
out vec4 draw_Color;

uniform sampler2D u_texture;
uniform sampler2D u_bloomTex;
uniform float u_bloomWeight;

void main() {
	vec4 color = texture(u_texture, var_TexCoord.xy);
	vec4 bloom = texture(u_bloomTex, var_TexCoord.xy);
	draw_Color = color + u_bloomWeight * bloom;
}
