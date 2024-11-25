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

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "tdm_transform.glsl"
#pragma tdm_include "volumetric_common.glsl"

out vec4 FragColor;

uniform sampler2D u_resultTexture;
uniform sampler2D u_depthTexture;
uniform vec2 u_invDestResolution;
uniform vec2 u_subpixelShift;
uniform bool u_upsampling;


vec2 invSrcResolution;
vec2 texelToTc(ivec2 texel) {
	// converts texel index to texcoord on source texture
	return (texel + vec2(0.5)) * invSrcResolution;
}


void main() {
	vec2 thisTC = gl_FragCoord.xy * u_invDestResolution;

	if (u_upsampling) {
		// depth-aware upsampling
		invSrcResolution = vec2(1) / vec2(textureSize(u_resultTexture, 0));
		vec2 factor = textureSize(u_resultTexture, 0) * u_invDestResolution;

		// canonical view Z we'll compare with
		vec2 distDerivs;
		float thisDist = sampleZWithDerivs(u_depthTexture, u_projectionMatrix, thisTC, u_invDestResolution, distDerivs);
		// since we render backfaces, set tangent plane as upper limit for view distance
		float exitDist = depthToZ(u_projectionMatrix, gl_FragCoord.z);
		vec2 exitDerivs = vec2(dFdx(exitDist), dFdy(exitDist));

		// find low-resolution pixel/blocks: nearest and right-bottom ones
		vec2 lowBlock = gl_FragCoord.xy * factor;
		ivec2 nearestBlock = ivec2(lowBlock);
		ivec2 rightBlock = ivec2(lowBlock + vec2(0.5));
		vec2 rightWeight = lowBlock + vec2(0.5) - rightBlock;

		// sample color and depth in nearest 4 blocks for custom bilinear filtering
		vec2 tc11 = texelToTc(rightBlock + ivec2(0, 0));
		vec2 tc01 = texelToTc(rightBlock + ivec2(-1, 0));
		vec2 tc10 = texelToTc(rightBlock + ivec2(0, -1));
		vec2 tc00 = texelToTc(rightBlock + ivec2(-1, -1));
		vec4 color11 = texture(u_resultTexture, tc11);
		vec4 color01 = texture(u_resultTexture, tc01);
		vec4 color10 = texture(u_resultTexture, tc10);
		vec4 color00 = texture(u_resultTexture, tc00);
		float dist11 = depthToZ(u_projectionMatrix, texture(u_depthTexture, tc11 + u_subpixelShift).r);
		float dist01 = depthToZ(u_projectionMatrix, texture(u_depthTexture, tc01 + u_subpixelShift).r);
		float dist10 = depthToZ(u_projectionMatrix, texture(u_depthTexture, tc10 + u_subpixelShift).r);
		float dist00 = depthToZ(u_projectionMatrix, texture(u_depthTexture, tc00 + u_subpixelShift).r);
		float weight11 =   rightWeight.x   * rightWeight.y   *   depthDifferenceWeight(thisDist, dist11, thisTC, tc11, distDerivs, exitDist, exitDerivs);
		float weight01 = (1-rightWeight.x) * rightWeight.y   *   depthDifferenceWeight(thisDist, dist01, thisTC, tc01, distDerivs, exitDist, exitDerivs);
		float weight10 =   rightWeight.x   * (1-rightWeight.y) * depthDifferenceWeight(thisDist, dist10, thisTC, tc10, distDerivs, exitDist, exitDerivs);
		float weight00 = (1-rightWeight.x) * (1-rightWeight.y) * depthDifferenceWeight(thisDist, dist00, thisTC, tc00, distDerivs, exitDist, exitDerivs);

		// compute weighted sum (depth-aware bilinear filtering)
		float weightSum = weight00 + weight01 + weight10 + weight11;
		vec4 colorSum = weight11 * color11 + weight01 * color01 + weight10 * color10 + weight00 * color00;
		// find color of nearest block
		vec4 colorNearest = texture(u_resultTexture, texelToTc(nearestBlock));
		// since weightSum can be low (even zero), lerp between bilinear average and nearest color
		FragColor = mix(colorNearest, colorSum / max(weightSum, 1e-3), smoothstep(0.03, 0.1, weightSum));
	}
	else {
		FragColor = texture(u_resultTexture, thisTC);
	}
}
