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

#pragma tdm_include "heatHazeCommon.glsl"

in vec4 attr_Position;
in vec4 attr_TexCoord;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec4 u_localParam0;	// texcoord scroll
uniform vec4 u_localParam1;	// deform multiplier

out vec2 texcoordOriginal;
out vec2 texcoordScrolled;
out vec2 deformationMagnitude;

void main() {
	gl_Position = heatHazeVertexShader(
		attr_Position, attr_TexCoord,
		u_modelViewMatrix, u_projectionMatrix,
		u_localParam0.xy, u_localParam1.xy,
		texcoordOriginal, texcoordScrolled, deformationMagnitude
	);
}
