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

// returns surface normal in tangent space
// should handle all the specifics of normalmap format
vec3 unpackSurfaceNormal(vec4 normalTexColor, bool useNormalTexture, bool RGTC) {
	if (useNormalTexture) {
		// fetch RGB, convert from [0, 1] to [-1, 1] range
		vec3 bumpTexel = normalTexColor.xyz * 2.0 - 1.0;

		if (RGTC) {
			// RGTC compression: add positive Z value
			float xyNormSqr = dot(bumpTexel.xy, bumpTexel.xy);
			return vec3(bumpTexel.x, bumpTexel.y, sqrt(max(1.0 - xyNormSqr, 0)));
		}
		else {
			// full RGB texture
			return normalize(bumpTexel.xyz);
		}
	}
	else {
		// flat surface
		return vec3(0, 0, 1);
	}
}
