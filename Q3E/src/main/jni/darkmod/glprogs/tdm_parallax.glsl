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

#pragma tdm_include "tdm_dither.glsl"
#pragma tdm_include "tdm_querylod.glsl"

// smoothened version of min(x, 1) for x >= 0
float smoothstep_01below(float x) {
	return max(2.0 * smoothstep(-1.5, 1.5, x) - 1.0, 1e-5);
}

vec3 computeParallaxOffset(
	sampler2D heightmap, vec2 heightScale,
	vec2 texcoords, vec3 viewDirLocal,
	float grazingAngle, ivec2 iterations
) {
	int linearSteps = iterations.x;
	int refineSteps = iterations.y;

	// gradually flatten heightmap at grazing angles
	float slope = viewDirLocal.z / length(viewDirLocal.xy);
	float fader = smoothstep_01below(slope / grazingAngle);
	heightScale *= fader;

	// x < y (even if input parameter is inverted)
	vec2 hgtRange = vec2(min(heightScale.x, heightScale.y), max(heightScale.x, heightScale.y));

	// fix LOD level for the whole parallax computation
	// texture samples are much faster without all the filtering
	float lod = queryTextureLod(heightmap, texcoords);

	// trace linearly by height decreasing until first "inside" point
	float goodRayH = hgtRange.y, badRayH = hgtRange.x;
	for (int s = linearSteps - 1; s >= 1; s--) {
		float rayH = mix(hgtRange.x, hgtRange.y, float(s) / linearSteps);
		vec2 tc = texcoords + rayH * viewDirLocal.xy / viewDirLocal.z;
		float normH = mix(heightScale.x, heightScale.y, textureLod(heightmap, tc, lod).r);
		if (normH > rayH) {
			badRayH = rayH;
			break;
		}
		goodRayH = rayH;
	}

	// do binary search between "outside" and "inside" points
	for (int s = 0; s < refineSteps; s++) {
		float rayH = (goodRayH + badRayH) * 0.5;
		vec2 tc = texcoords + rayH * viewDirLocal.xy / viewDirLocal.z;
		float normH = mix(heightScale.x, heightScale.y, textureLod(heightmap, tc, lod).r);
		if (normH > rayH)
			badRayH = rayH;
		else
			goodRayH = rayH;
	}

	return vec3(viewDirLocal.xy / viewDirLocal.z * goodRayH, goodRayH / fader);
}

// this is pretty hacky approach, and it is not very smooth...
// it would be better if "scale" between texcoords and model space was passed as vertex attribute
vec3 scaleTexcoordOffsetToModelSpace(vec3 offset, vec2 texcoords, vec3 positionModel, mat3 matrixTBN) {
	mat2 tcDer = mat2(dFdx(texcoords), dFdy(texcoords));
	mat2x3 modDer = mat2x3(dFdx(positionModel), dFdy(positionModel));
	mat2x3 tcToMod = modDer * inverse(tcDer);
	float scaleX = dot(tcToMod[0], matrixTBN[0]);
	float scaleY = dot(tcToMod[1], matrixTBN[1]);
	return offset * vec3(scaleX, scaleY, (abs(scaleX) + abs(scaleY)) * 0.5);
}

float computeParallaxShadow(
	sampler2D heightmap, vec2 heightScale,
	vec2 texcoords, vec3 tcOffset, vec3 lightDirLocal,
	int shadowSteps, float shadowSoftness
) {
	// x < y (even if input parameter is inverted)
	vec2 hgtRange = vec2(min(heightScale.x, heightScale.y), max(heightScale.x, heightScale.y));

	float randomizer = 0.5 + ditherFractionBayer8();

	// fix LOD level for the whole parallax computation
	// texture samples are much faster without all the filtering
	float lod = queryTextureLod(heightmap, texcoords.xy);

	vec3 tcTotal = vec3(texcoords, 0.0) + tcOffset;

	// trace linearly to check for obstacles
	float step = (hgtRange.y - hgtRange.x) / shadowSteps;
	float maxDiff = -1.0;
	for (float deltaH = step * randomizer; tcTotal.z + deltaH < hgtRange.y; deltaH += step) {
		vec2 tc = tcTotal.xy + deltaH * (lightDirLocal.xy / lightDirLocal.z);
		float obstH = mix(heightScale.x, heightScale.y, textureLod(heightmap, tc, lod).r);
		float diff = obstH - (tcTotal.z + deltaH);
		maxDiff = max(maxDiff, diff);
	}

	return 1.0 - clamp(maxDiff / shadowSoftness, 0, 1);
}
