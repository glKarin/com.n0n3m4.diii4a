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

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "stages/surface_passes/texgen_shared.glsl"

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform vec4 u_colorMul;
uniform vec4 u_colorAdd;

uniform int u_texgen;
uniform mat4 u_textureMatrix;
uniform vec3 u_viewOrigin;

in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec4 attr_Color;

out vec4 var_TexCoord;
out vec4 var_Color;

void main() {
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);

	var_Color = attr_Color * u_colorMul + u_colorAdd;

	if (u_texgen == TEXGEN_EXPLICIT) {
		var_TexCoord = u_textureMatrix * vec4(attr_TexCoord, 0, 1);
	}
	else if (u_texgen == TEXGEN_SCREEN) {
		var_TexCoord = gl_Position;
	}
	else if (u_texgen == TEXGEN_CUBEMAP) {
		var_TexCoord = vec4(vec3(attr_Position) - u_viewOrigin, 0);
	}
}
