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

in vec4 attr_Position;
in vec2 attr_TexCoord;

uniform mat4 u_projectionMatrix;
uniform mat4 u_modelViewMatrix;

out vec3 var_TexCoord;

void main() {
	gl_Position = u_projectionMatrix * (u_modelViewMatrix * attr_Position);
    var_TexCoord = attr_Position.xyz;
}
