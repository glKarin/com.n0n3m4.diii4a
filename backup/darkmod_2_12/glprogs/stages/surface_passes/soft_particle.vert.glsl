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

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec4 u_colorMul;
uniform vec4 u_colorAdd;

in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec4 attr_Color;

out vec4 var_Color;
out vec2 var_TexCoord;

void main() {
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);
	var_TexCoord = attr_TexCoord;
	var_Color = attr_Color * u_colorMul + u_colorAdd;
}
