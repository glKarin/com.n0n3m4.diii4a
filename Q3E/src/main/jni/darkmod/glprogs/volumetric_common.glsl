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

float depthDifferenceWeight(float distCorrect, float distHave, vec2 tcCorrect, vec2 tcHave, vec2 distDerivs, float capDist, vec2 capDistDerivs) {
	vec2 tcDelta = tcHave - tcCorrect;
	// if both pixels are behind light frustum, then weight = 1, i.e. full blur
	// this means that depth variation in background does not harm volumetric light
	if (distCorrect > capDist && distHave > capDist + dot(capDistDerivs, tcDelta))
		return 1.0;
	// using high-res derivatives of view Z, do linear estimate of true difference
	vec2 distDelta = distDerivs * tcDelta;
	float distEstim = distCorrect + distDelta.x + distDelta.y;
	float distError = abs(distHave - distEstim);
	// use composite tolerance: absolute + relative + variation
	float tolerance = 10.0 + 0.04 * distCorrect + 0.5 * (abs(distDelta.x) + abs(distDelta.y));
	return 1 - smoothstep(0.5, 1.0, distError / tolerance);
}
