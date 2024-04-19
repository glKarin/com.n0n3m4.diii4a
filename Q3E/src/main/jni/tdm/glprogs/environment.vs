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

uniform vec4 u_viewOriginLocal;

in vec4 attr_Position;
in vec3 attr_Normal;
in vec4 attr_Color;

out vec3 var_normalLocal;
out vec3 var_toEyeLocal;
out vec4 var_color;

void main() {
	gl_Position = tdm_transform(attr_Position);

	var_normalLocal = attr_Normal;
	var_toEyeLocal = vec3(u_viewOriginLocal - attr_Position);

	var_color = attr_Color;
}
