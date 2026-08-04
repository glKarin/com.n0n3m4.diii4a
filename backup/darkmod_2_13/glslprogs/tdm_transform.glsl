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

#pragma tdm_include "tdm_utils.glsl"

// stgatilov: it is recommended to use objectPosToClip from tdm_utils.glsl instead
// especially if you need to receive any matrix in different way...

uniform block {
	uniform mat4 u_projectionMatrix;
};
uniform mat4 u_modelViewMatrix;

vec4 tdm_transform(vec4 position) {
	return objectPosToClip(position, u_modelViewMatrix, u_projectionMatrix);
}
