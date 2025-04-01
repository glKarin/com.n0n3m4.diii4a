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
#version 330

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "tdm_transform.glsl"
#pragma tdm_include "volumetric_common.glsl"

out vec4 FragColor;

uniform sampler2D u_sourceTexture;
uniform sampler2D u_depthTexture;
uniform vec2 u_invDestResolution;
uniform vec2 u_subpixelShift;
uniform bool u_vertical;
uniform float u_blurSigma;

void main() {
	vec2 tcCenter = gl_FragCoord.xy * u_invDestResolution;

	vec2 delta = u_invDestResolution;
	// one-dimensional blur: vertical / horizontal depending on uniform
	if (u_vertical)
		delta.x = 0;
	else
		delta.y = 0;

	// save view Z of central sample
	vec2 distDerivs;
	float distCenter = sampleZWithDerivs(u_depthTexture, u_projectionMatrix, tcCenter + u_subpixelShift, u_invDestResolution, distDerivs);
	// since we render backfaces, set tangent plane as upper limit for view distance
	float exitDist = depthToZ(u_projectionMatrix, gl_FragCoord.z);
	vec2 exitDerivs = vec2(dFdx(exitDist), dFdy(exitDist));

	float sumWeight = 0;
	vec4 sumColor = vec4(0);

	int radius = int(u_blurSigma * 2.0 + 0.5);
	for (int i = -radius; i <= radius; i++) {
		vec2 tcThis = tcCenter + i * delta;
		float density = exp(-0.5 * (i/u_blurSigma) * (i/u_blurSigma));

		// blur is depth-aware, so edges are not blurred
		float distThis = depthToZ(u_projectionMatrix, texture(u_depthTexture, tcThis + u_subpixelShift).r);
		float depthWeight = depthDifferenceWeight(distCenter, distThis, tcCenter, tcThis, distDerivs, exitDist, exitDerivs);

		vec4 color = texture(u_sourceTexture, tcThis);
		sumWeight += depthWeight * density;
		sumColor += depthWeight * density * color;
	}

	FragColor = sumColor / max(sumWeight, 1e-3);
}
