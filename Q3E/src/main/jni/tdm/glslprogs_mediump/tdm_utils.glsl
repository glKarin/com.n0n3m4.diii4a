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

// transforms homogeneous 3D position according to MVP matrices (for vertex shaders)
// note: you must use this exact code, otherwise you'll get depth fighting due to numeric differences!
vec4 objectPosToClip(vec4 position, mat4 modelView, mat4 projection) {
	return projection * (modelView * position);
}

// transforms 3D position from world space to object/model space using inverse model transform
vec3 worldPosToObject(vec3 worldPosition, mat4 modelMatrix) {
	return (worldPosition - vec3(modelMatrix[3])) * mat3(modelMatrix);
}

// returns view Z coordinate with reversed sign (monotonically increasing with depth)
// in other words, it is eye-fragment distance along view direction
float depthToZ(mat4 projectionMatrix, float depth) {
	float clipZ = 2.0 * depth - 1.0;
	float A = projectionMatrix[2].z;
	float B = projectionMatrix[3].z;
	// note: D3 uses nonstandard far-at-infinity projection matrix (see R_SetupProjection)
	// it means that d > 0.999 is invalid range
	// the clamping to -eps from below ensures that such depth (e.g. depth = 1) produce large distance
	// (according to R_SetupProjection, eps = 1e-5 gives distance ~= 6e+5)
	return B / min(A + clipZ, -1e-5);
}

// samples depth texture and returns negated view Z with derivatives
// tcDelta should equal inverse resolution of depth texture (sampling distance)
// Zderivs is mathematically equivalent to (dFdx(return), dFdy(return)) if depth and render resolutions are equal
// but this function returns good values even for texels on discontinuity edge
float sampleZWithDerivs(sampler2D depthTexture, mat4 projectionMatrix, vec2 tcPoint, vec2 tcDelta, out vec2 Zderivs) {
	// set cross pattern
	vec2 tcMoreX = tcPoint + vec2(tcDelta.x, 0);
	vec2 tcLessX = tcPoint - vec2(tcDelta.x, 0);
	vec2 tcMoreY = tcPoint + vec2(0, tcDelta.y);
	vec2 tcLessY = tcPoint - vec2(0, tcDelta.y);
	// evaluate all samples
	float valueCenter = depthToZ(projectionMatrix, texture(depthTexture, tcPoint).r);
	float valueMoreX = depthToZ(projectionMatrix, texture(depthTexture, tcMoreX).r);
	float valueLessX = depthToZ(projectionMatrix, texture(depthTexture, tcLessX).r);
	float valueMoreY = depthToZ(projectionMatrix, texture(depthTexture, tcMoreY).r);
	float valueLessY = depthToZ(projectionMatrix, texture(depthTexture, tcLessY).r);
	// set infinite values for texcoords outside [0..1]
	if (tcMoreX.x > 1.0) valueMoreX = 1e+20;
	if (tcLessX.x < 0.0) valueLessX = 1e+20;
	if (tcMoreY.x > 1.0) valueMoreY = 1e+20;
	if (tcLessY.x < 0.0) valueLessY = 1e+20;
	// compute all finite differences
	float derivMoreX = valueMoreX - valueCenter;
	float derivLessX = -(valueLessX - valueCenter);
	float derivMoreY = valueMoreY - valueCenter;
	float derivLessY = -(valueLessY - valueCenter);
	// among two differences, choose one with smaller magnitude
	Zderivs.x = (abs(derivLessX) < abs(derivMoreX) ? derivLessX : derivMoreX);
	Zderivs.y = (abs(derivLessY) < abs(derivMoreY) ? derivLessY : derivMoreY);
	return valueCenter;
}

