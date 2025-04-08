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

#pragma tdm_include "tdm_utils.glsl"

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec3 u_globalViewOrigin;
uniform mat4 u_modelMatrix;

in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;

out vec2 var_TexCoord;
out vec3 var_ToEyeWorld;
out mat3 var_TangentToWorldMatrix;

void main() {
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);

	var_ToEyeWorld = u_globalViewOrigin - vec3(u_modelMatrix * attr_Position);

	mat3 tangentToLocalMatrix = mat3(
		clamp(attr_Tangent, vec3(-1.0), vec3(1.0)),
		clamp(attr_Bitangent, vec3(-1.0), vec3(1.0)),
		clamp(attr_Normal, vec3(-1.0), vec3(1.0))
	);
	var_TangentToWorldMatrix = mat3(u_modelMatrix) * tangentToLocalMatrix;

	var_TexCoord = attr_TexCoord;
}
