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

#pragma tdm_include "tdm_transform.glsl"

uniform mat4 u_modelMatrix;
uniform vec4 u_viewOriginLocal;

in vec4 attr_Position;
in vec2 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;

out vec2 var_texCoord;
out vec3 var_toEyeWorld;
out mat3 var_TangentToWorldMatrix;

void main() {
	gl_Position = tdm_transform(attr_Position);

	var_toEyeWorld = mat3(u_modelMatrix) * vec3(u_viewOriginLocal - attr_Position);

	mat3 matTangentToLocal = mat3(
		clamp(attr_Tangent, -1, 1),
		clamp(attr_Bitangent, -1, 1),
		clamp(attr_Normal, -1, 1)
	);
	var_TangentToWorldMatrix = mat3(u_modelMatrix) * matTangentToLocal;

	var_texCoord = attr_TexCoord;
}
