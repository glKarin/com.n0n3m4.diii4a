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
uniform vec4 u_fogPlane;
uniform vec3 u_viewOrigin;

in vec4 attr_Position;

out float var_eyeDistance;
out float var_eyeHeight;
out float var_fragHeight;

void main() {
	gl_Position = objectPosToClip(attr_Position, u_modelViewMatrix, u_projectionMatrix);
	var_eyeDistance = -(u_modelViewMatrix * attr_Position).z;
	var_eyeHeight = dot(u_fogPlane, vec4(u_viewOrigin, 1));
	var_fragHeight = dot(u_fogPlane, attr_Position);
}
