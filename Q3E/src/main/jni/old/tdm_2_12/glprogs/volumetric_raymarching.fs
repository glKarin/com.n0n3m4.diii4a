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
#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "tdm_shadowmaps.glsl"

uniform sampler2D u_lightProjectionTexture;
uniform sampler2D u_lightFalloffTexture;
uniform sampler2D u_depthTexture;
uniform sampler2D u_shadowMap;

uniform vec3 u_viewOrigin;
uniform mat4 u_lightProject;
uniform vec4 u_lightFrustum[6];
uniform vec4 u_lightTextureMatrix[2];
uniform vec4 u_shadowRect;
uniform vec3 u_lightOrigin;
uniform int u_sampleCount;
uniform vec4 u_lightColor;
uniform int u_shadows;
uniform float u_dust;
uniform int u_randomize;
uniform int u_alphaMode;
uniform vec2 u_invDestResolution;
uniform vec2 u_subpixelShift;

in vec4 worldPosition;

out vec4 fragColor;

// 8x8 Bayer matrix
float DITHER_MATRIX[64] = float[](
	0, 32, 8, 40, 2, 34, 10, 42,
	48, 16, 56, 24, 50, 18, 58, 26,
	12, 44, 4, 36, 14, 46, 6, 38,
	60, 28, 52, 20, 62, 30, 54, 22,
	3, 35, 11, 43, 1, 33, 9, 41,
	51, 19, 59, 27, 49, 17, 57, 25,
	15, 47, 7, 39, 13, 45, 5, 37,
	63, 31, 55, 23, 61, 29, 53, 21
);

float ditherFraction() {
	int x = int(gl_FragCoord.x), y = int(gl_FragCoord.y);
	return DITHER_MATRIX[8 * (x&7) + (y&7)] / 64.0;
}

// get N samples from the fragment-view ray inside the frustum
vec3 calcWithSampling(vec3 rayStart, vec3 rayVec, float minParam, float maxParam, int samplesNum) {
	vec3 color = vec3(0.0);
	for (int i = 0; i < samplesNum; i++) { 
		float frac = 0.5;
		if (u_randomize != 0)
			frac = ditherFraction();
		float ratio = (i + frac) / samplesNum;
		vec3 samplePos = rayStart + rayVec * mix(minParam, maxParam, ratio);
		// shadow test
		vec3 light2fragment = samplePos - u_lightOrigin;
		float lit = 1;
		if (u_shadows != 0) {
			float depth = ShadowAtlasForVector(u_shadowMap, u_shadowRect, light2fragment);
			vec3 absL = abs(light2fragment);
			float maxAbsL = max(absL.x, max(absL.y, absL.z));
			lit = float(maxAbsL < depth);
		}
		vec4 texCoord = computeLightTex(u_lightProject, vec4(samplePos, 1));
		vec3 texColor = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, u_lightTextureMatrix, texCoord).rgb;
		color += lit * texColor;
	}
	return color / samplesNum;
}

vec3 calcAverage(vec3 rayStart, vec3 rayVec, float minParam, float maxParam) {
	// 1. suppose that nothing is shadowed
	// 2. take average of both projection and falloff textures
	vec4 avgProjection = textureLod(u_lightProjectionTexture, vec2(0.5), 10000);
	vec4 avgFalloff = textureLod(u_lightFalloffTexture, vec2(0.5), 10000);
	return avgProjection.xyz * avgFalloff.xyz;
}

void main() {
	//cast segment from viewer eye to the fragment
	vec3 rayStart = u_viewOrigin;
	vec3 rayVec = worldPosition.xyz - u_viewOrigin;
	
	//intersect the segment with light polytope
	float minParam = 0.0;
	float maxParam = 1.0;
	for (int i = 0; i < 6; i++) {
		float dotnp = dot(u_lightFrustum[i], vec4(rayStart, 1.0));
		float dotnv = dot(u_lightFrustum[i].xyz, rayVec);
		float param = -dotnp / dotnv;
		if (dotnv > 0)
			maxParam = min(maxParam, param);
		else
			minParam = max(minParam, param);
	}

	//only consider visible part (not occluded by opaque geometry)
	vec2 depthTexCoord = gl_FragCoord.xy * u_invDestResolution;
	float depth = texture2D(u_depthTexture, depthTexCoord + u_subpixelShift).r;
	float solidParam = depthToZ(u_projectionMatrix, depth) / depthToZ(u_projectionMatrix, gl_FragCoord.z);
	maxParam = min(maxParam, solidParam);

	if (minParam >= maxParam)
		discard;    //no intersection
	float litDistance = (maxParam - minParam) * length(rayVec);
	
	vec3 avgColor;
	if (u_sampleCount > 0)
		avgColor = calcWithSampling(rayStart, rayVec, minParam, maxParam, u_sampleCount);
	else
		avgColor = calcAverage(rayStart, rayVec, minParam, maxParam);
	fragColor.rgb = u_lightColor.rgb * avgColor;

	if (u_alphaMode == 0) {
		//volumetric light: uncapped linear growth
		fragColor.a = u_dust * litDistance;
	}
	else if (u_alphaMode == 1) {
		//fog light: exponential light with forced cap at distance 1 / u_dust = cap
		float q = u_dust * litDistance;
		//start with exponential attenuation: 0 at D = 0, (1 - e^-5) at D = cap
		float exponential = 1.0 - exp(-q * 5.0);
		//then force fog to be opaque at D = cap
		float transition = clamp((q - 0.9) / 0.1, 0, 1);
		//exponential up to 90% of cap, perfectly opaque fog at 100% of cap (linear interpolation in-between)
		float alpha = mix(exponential, 1, transition);
		//add a bit of unscientific dithering to smoothen out color banding
		if (u_randomize != 0)
			alpha = min(alpha + 1e-2 * ditherFraction(), 1.0);
		fragColor.a = alpha;
	}
}
