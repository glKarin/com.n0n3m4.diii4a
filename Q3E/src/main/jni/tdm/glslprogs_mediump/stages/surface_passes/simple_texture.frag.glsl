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

#pragma tdm_include "stages/surface_passes/texgen_shared.glsl"

uniform highp int u_texgen;
uniform sampler2D u_texture;
uniform samplerCube u_cubemap;

in vec4 var_TexCoord;
in vec4 var_Color;

out vec4 FragColor;

void main() {
	if (u_texgen == TEXGEN_EXPLICIT) {
		FragColor = var_Color * textureProj(u_texture, var_TexCoord);
	}
	else if (u_texgen == TEXGEN_SCREEN) {
		vec2 tc = var_TexCoord.xy / var_TexCoord.w * 0.5 + 0.5;
		FragColor = var_Color * texture(u_texture, tc);
	}
	else if (u_texgen == TEXGEN_CUBEMAP) {
		FragColor = texture(u_cubemap, var_TexCoord.xyz);
	}
}
