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

uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec4 u_blendColor;

in vec4 var_TexCoord0;
in vec4 var_TexCoord1;

out vec4 FragColor;

void main() {
	vec4 texel0 = texture(u_texture0, var_TexCoord0.xy);
	vec4 texel1 = texture(u_texture1, var_TexCoord1.xy);
	FragColor = u_blendColor*texel0*texel1;	
}