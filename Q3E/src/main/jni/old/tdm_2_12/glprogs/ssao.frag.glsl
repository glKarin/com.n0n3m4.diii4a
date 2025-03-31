#version 140
#extension GL_ARB_gpu_shader5 : enable

/**
 Based on the SAO algorithm by Morgan McGuire and Michael Mara, NVIDIA Research

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

in vec2 var_TexCoord;
in vec2 var_ViewRayXY;
out vec3 occlusionAndDepth;

uniform sampler2D u_depthTexture;

uniform float u_intensityDivR6;
// Adjustable SSAO parameters
uniform int u_numSamples;
uniform int u_numSpiralTurns;
uniform float u_sampleRadius;
uniform float u_depthBias;
uniform float u_baseValue;

uniform block {
	mat4 u_projectionMatrix;
};

float nearZ = -0.5 * u_projectionMatrix[3][2];
vec2 minusTwohalfTanFov = -2 * vec2(1 / u_projectionMatrix[0][0], 1 / u_projectionMatrix[1][1]);
vec2 invTextureSize = vec2(1.0, 1.0) / textureSize(u_depthTexture, 0);

// The height in pixels of an object of height 1 world unit at distance z = -1 world unit.
// Used to scale the radius of the sampling disc appropriately
float projectionScale = textureSize(u_depthTexture, 0).y * (0.5 * u_projectionMatrix[1][1]);
const float tdmToMetres = 0.02309;

vec3 currentTexelViewPos() {
	vec3 viewPos;
	viewPos.z = texelFetch(u_depthTexture, ivec2(gl_FragCoord.xy), 0).r;
	viewPos.xy = var_ViewRayXY * viewPos.z;
	return viewPos * tdmToMetres;
}

vec3 deriveViewSpaceNormal(vec3 viewPos) {
	return normalize(cross(dFdx(viewPos), dFdy(viewPos)));
}

/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
vec2 tapLocation(int sampleNumber, float spinAngle, out float ssR){
	// Radius relative to ssR
	float alpha = float(sampleNumber + 0.5) * (1.0 / u_numSamples);
	float angle = alpha * (u_numSpiralTurns * 6.28) + spinAngle;

	ssR = alpha;
	return vec2(cos(angle), sin(angle));
}

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET (3)

// This must be less than or equal to the MAX_DEPTH_MIPS defined in AmbientOcclusionStage
uniform int u_maxMipLevel;

/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1 */
vec3 getOffsetPosition(ivec2 ssC, vec2 unitOffset, float ssR) {
	// Derivation:
	//  mipLevel = floor(log(ssR / MAX_OFFSET));
	// #   ifdef GL_ARB_gpu_shader5
	// int msb = findMSB(int(ssR));
	// #   else
	int msb = int(floor(log2(ssR)));
	// #   endif
	int mipLevel = clamp(msb - LOG_MAX_OFFSET, 0, u_maxMipLevel);

	ivec2 ssP = ivec2(ssR * unitOffset) + ssC;

	vec3 P;

	// We need to divide by 2^mipLevel to read the appropriately scaled coordinate from a MIP-map.
	// Manually clamp to the texture size because texelFetch bypasses the texture unit
	ivec2 mipP = clamp(ssP >> mipLevel, ivec2(0), textureSize(u_depthTexture, mipLevel) - ivec2(1));
	P.z = texelFetch(u_depthTexture, mipP, mipLevel).r;

	// Offset to pixel center
	vec2 pixCenter = vec2(ssP) + vec2(0.5);
	P.xy = minusTwohalfTanFov * (invTextureSize * pixCenter - 0.5) * P.z;

	return P * tdmToMetres;
}

/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius

    Note that units of H() in the HPG12 paper are meters, not
    unitless.  The whole falloff/sampling function is therefore
    unitless.  In this implementation, we factor out (9 / radius).

    Four versions of the falloff function are implemented below
*/
float radiusInMetres = u_sampleRadius * tdmToMetres;
float radiusSqr = radiusInMetres * radiusInMetres;
float sampleAO(in ivec2 ssC, in vec3 C, in vec3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle) {
	// Offset on the unit disk, spun for this pixel
	float ssR;
	vec2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
	ssR *= ssDiskRadius;

	// The occluding point in camera space
	vec3 Q = getOffsetPosition(ssC, unitOffset, ssR);

	vec3 v = Q - C;

	float vv = dot(v, v);
	float vn = dot(v, n_C);

	const float epsilon = 0.01;

	// A: From the HPG12 paper
	// Note large epsilon to avoid overdarkening within cracks
	// return float(vv < radiusSqr) * max((vn - u_depthBias) / (epsilon + vv), 0.0) * radiusSqr * 0.6;

	// B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
	float f = max(radiusSqr - vv, 0.0);
	return f * f * f * max((vn - u_depthBias) / (epsilon + vv), 0.0);

	// C: Medium contrast (which looks better at high radii), no division.  Note that the
	// contribution still falls off with radius^2, but we've adjusted the rate in a way that is
	// more computationally efficient and happens to be aesthetically pleasing.
	//return 4.0 * max(1.0 - vv / radiusSqr, 0.0) * max(vn - u_depthBias, 0.0);

	// D: Low contrast, no division operation
	// return 2.0 * float(vv < radiusSqr) * max(vn - u_depthBias, 0.0);
}

// we don't have an actual far Z, but this value is a "cutoff" used for packing the Z values for the edge-aware blur filter
const float farZ = -1500.0 * tdmToMetres;

vec2 packViewSpaceZ(float viewSpaceZ) {
	float compressedZ = clamp(viewSpaceZ * (1.0 / farZ), 0, 1);
	float temp = floor(compressedZ * 256.0);
	float integerPart = temp * (1.0 / 256.0);
	float fractionalPart = compressedZ * 256.0 - temp;
	return vec2(integerPart, fractionalPart);
}

void main() {
	ivec2 screenPos = ivec2(gl_FragCoord.xy);
	vec3 position = currentTexelViewPos();

	if (position.z > 0) {
		// these values are leftovers from subviews, e.g. the skybox
		discard;
	}

	vec3 normal = deriveViewSpaceNormal(position);
	// "random" rotation factor from a hash function proposed by the AlchemyAO HPG12 paper
	float randomPatternRotationAngle = mod(3 * screenPos.x ^ screenPos.y + screenPos.x * screenPos.y, 3.14159);
	// calculate screen-space sample radius from view space radius
	// note: factor 0.5625 is just an adjustment to keep radius values consistent after a math change
	float screenDiskRadius = -0.5625 * projectionScale * radiusInMetres / position.z;

	float sum = 0.0;
	for (int i = 0; i < u_numSamples; ++i) {
		sum += sampleAO(screenPos, position, normal, screenDiskRadius, i, randomPatternRotationAngle);
	}

	float occlusion = max(u_baseValue, 1.0 - sum * u_intensityDivR6 * (2.5 / u_numSamples));

	// Bilateral box-filter over a quad for free, respecting depth edges
	// (the difference that this makes is subtle)
	if (abs(dFdx(position.z)) < 0.02) {
		occlusion -= dFdx(occlusion) * ((screenPos.x & 1) - 0.5);
	}
	if (abs(dFdy(position.z)) < 0.02) {
		occlusion -= dFdy(occlusion) * ((screenPos.y & 1) - 0.5);
	}

	occlusionAndDepth.r = occlusion;

	// pack the Z value for the edge-aware blur filter
	occlusionAndDepth.gb = packViewSpaceZ(position.z);
}
