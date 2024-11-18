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

uniform vec3 u_fogColor;
uniform float u_fogAlpha;
uniform float u_eyeDistanceCap;

in float var_eyeDistance;
in float var_eyeHeight;
in float var_fragHeight;

out vec4 FragColor;

// restored from R_FogImage C++ function
float falloffRatio(float depthRatio) {
	float remains = pow(0.009737, max(depthRatio, 1e-10));
	// note: original code worked with finalizer == 0, having a jump from 0.991 to 1.0 on far end
	float finalizer = clamp(depthRatio - 0.9, 0.0, 1.0) / 0.1;
	return 1.0 - mix(remains, 0.0, finalizer);
}

// restored from R_FogEnterImage + FogFraction C++ mess
float fogEnterRatio(float eyeHeight, float fragHeight, float enterDistance, float deepDistance) {
	if (eyeHeight > 0.0 && fragHeight > 0.0)
		return 0.0;
	if (eyeHeight < -enterDistance && fragHeight < -enterDistance)
		return 1.0;
	float above = max(max(eyeHeight, fragHeight), 0.0);
	float top = min(max(eyeHeight, fragHeight), 0.0);
	float bottom = max(min(eyeHeight, fragHeight), -enterDistance);
	float ramp = (1.0 + 0.5 * (top + bottom) / enterDistance) * (top - bottom);
	float frac = 1.0 - (above + ramp) / max(abs(fragHeight - eyeHeight), 1e-3);
	float deepest = min(eyeHeight, fragHeight);
	float deepFrac = clamp(-deepest / deepDistance, 0.0, 1.0);
	frac = mix(frac, 1.0, deepFrac);
	return frac;
}

void main() {
	float falloffFactor = falloffRatio(var_eyeDistance / u_eyeDistanceCap);
	// enterDistance = 1000 * RAMP_RANGE / FOG_ENTER
	// deepDistance = 1000 * DEEP_RANGE / FOG_ENTER
	float enterFactor = fogEnterRatio(var_eyeHeight, var_fragHeight, 1000.0 * 8.0 / 64.0, 1000.0 * 30.0 / 64.0);

	vec4 result = vec4(u_fogColor, falloffFactor * enterFactor * u_fogAlpha);
	FragColor = result;
}
