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
#pragma tdm_include "tdm_poissondisk.glsl"  // note: adds uniform

vec3 CubeMapDirectionToUv(vec3 v, out int faceIdx) {
	vec3 v1 = abs(v);
	float maxV = max(v1.x, max(v1.y, v1.z));
	faceIdx = 0;
	if(maxV == v.x) {
		v1 = -v.zyx;
	}
	else if(maxV == -v.x) {
		v1 = v.zyx * vec3(1, -1, 1);
		faceIdx = 1;
	}
	else if(maxV == v.y) {
		v1 = v.xzy * vec3(1, 1, -1);
		faceIdx = 2;
	}
	else if(maxV == -v.y) {
		v1 = v.xzy * vec3(1, -1, 1);
		faceIdx = 3;
	}
	else if(maxV == v.z) {
		v1 = v.xyz * vec3(1, -1, -1);
		faceIdx = 4;
	}
	else { //if(maxV == -v.z) {
		v1 = v.xyz * vec3(-1, -1, 1);
		faceIdx = 5;
	}
	v1.xy /= -v1.z;
	return v1;
}

//shadowRect defines where one face of this light is within the map atlas:
//  (offsetX, offsetY, ?, size)     (all from 0..1)
//lightVec is world-space vector from the light origin to the fragment being considered
//returns positive distance along face normal of cube map (i.e. along coordinate where lightVec has maximum absolute value)
float ShadowAtlasForVector(in sampler2D shadowMapTexture, vec4 shadowRect, vec3 lightVec) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(lightVec, faceIdx);
	vec2 texSize = textureSize(shadowMapTexture, 0);
	vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * shadowRect.ww + shadowRect.xy;
	shadow2d.x += (shadowRect.w + 1./texSize.x) * faceIdx;
	float d = textureLod(shadowMapTexture, shadow2d, 0).r;
	return 1 / (1 - d);
}
#if TDM_allow_ARB_texture_gather
vec4 ShadowAtlasForVector4(in sampler2D shadowMapTexture, vec4 shadowRect, vec3 lightVec, out vec4 sampleWeights) {
	int faceIdx;
	vec3 v1 = CubeMapDirectionToUv(lightVec, faceIdx);
	vec2 texSize = textureSize(shadowMapTexture, 0);
	vec2 shadow2d = (v1.xy * .5 + vec2(.5) ) * shadowRect.ww + shadowRect.xy;
	shadow2d.x += (shadowRect.w + 1./texSize.x) * faceIdx;
#if GL_ARB_texture_gather
	vec4 d = textureGather(shadowMapTexture, shadow2d);
#else
	vec4 d = textureLod(shadowMapTexture, shadow2d, 0).rrrr;
#endif
	vec2 wgt = fract(shadow2d * texSize - 0.5);
	vec2 mwgt = vec2(1) - wgt;
	sampleWeights = vec4(mwgt.x, wgt.x, wgt.x, mwgt.x) * vec4(wgt.y, wgt.y, mwgt.y, mwgt.y);
	return vec4(1) / (vec4(1) - d);
}
#endif


float computeShadowMapCoefficient(
	vec3 worldFromLight, vec3 worldNormal,
	sampler2D shadowMap, vec4 shadowRect,
	int softQuality, float softRadius, bool shadowMapCullFrontHack
) {
	float shadowMapResolution = (textureSize(shadowMap, 0).x * shadowRect.w);

	//get unit direction from light to current fragment
	vec3 L = normalize(worldFromLight);
	//find maximum absolute coordinate in light vector
	vec3 absL = abs(worldFromLight);
	float maxAbsL = max(absL.x, max(absL.y, absL.z));

	//note: using different normal affects near-tangent lighting greatly
	float lightFallAngle = -dot(worldNormal, L);

	//note: choosing normal and how to cap angles is the hardest question for now
	//this has large effect on near-tangent surfaces (mostly the curved ones)
	float fallAngleLimiter = smoothstep(0.01, 0.05, lightFallAngle);
	//some very generic error estimation...
	float errorMargin = 5.0 * maxAbsL / ( shadowMapResolution * max(lightFallAngle, 0.1) );
	if (shadowMapCullFrontHack)
	   errorMargin *= -.5;

	//process central shadow sample
	float centerFragZ = maxAbsL;
#if STGATILOV_USEGATHER && defined(GL_ARB_texture_gather)
	vec4 wgt;
	vec4 centerBlockerZ = ShadowAtlasForVector4(shadowMap, shadowRect, L, wgt);
	float lit = dot(wgt, step(centerFragZ - errorMargin, centerBlockerZ));
#else
	float centerBlockerZ = ShadowAtlasForVector(shadowMap, shadowRect, L);
	float lit = float(centerBlockerZ >= centerFragZ - errorMargin);
#endif
	if (softQuality == 0)
		return lit * fallAngleLimiter;

	float lightDist = length(worldFromLight);
	//this is (1 / cos(phi)), where phi is angle between light direction and normal of the pierced cube face
	float secFallAngle = lightDist / maxAbsL;
	//find two unit directions orthogonal to light direction
	vec3 nonCollinear = vec3(1, 0, 0);
	if (absL.x == maxAbsL)
		nonCollinear = vec3(0, 1, 0);
	vec3 orthoAxisX = normalize(cross(L, nonCollinear));
	vec3 orthoAxisY = cross(L, orthoAxisX);

	//search for blockers in a cone with rather large angle
	float searchAngle = 0.03 * softRadius;    //TODO: this option is probably very important
	float avgBlockerZ = 0;
	int blockerCnt = 0;
	for (int i = 0; i < softQuality; i++) {
		//note: copy/paste from sampling code below
		vec3 perturbedLightDir = normalize(L + searchAngle * (u_softShadowsSamples[i].x * orthoAxisX + u_softShadowsSamples[i].y * orthoAxisY));
		float blockerZ = ShadowAtlasForVector(shadowMap, shadowRect, perturbedLightDir);
		float dotDpL = max(max(abs(perturbedLightDir.x), abs(perturbedLightDir.y)), abs(perturbedLightDir.z));
		float distCoeff = lightFallAngle / max(-dot(worldNormal, perturbedLightDir), 1e-3) * (dotDpL * secFallAngle);
		float fragZ = centerFragZ * distCoeff;
		//note: only things which may potentially occlude are averaged
		if (blockerZ < fragZ - errorMargin) {
			avgBlockerZ += blockerZ;
			blockerCnt++;
		}
	}
	//shortcut: no blockers in search angle => fully lit
	if (blockerCnt == 0)
		return fallAngleLimiter;
	/* Bad optimization!
	 * Go to St. Alban's Collateral and execute:
	 *   setviewpos  -114.57 1021.61 130.95   -1.0 147.8 0.0
	 * and you'll notice artefacts if you enable this piece of code.
	//shortcut: all blockers in search angle => fully occluded
	if (blockerCnt == softQuality && lit == 0) {
		return 0.0;
	}
	*/
	avgBlockerZ /= blockerCnt;

	//radius of light source
	float lightRadius = softRadius;
	//radius of one-point penumbra at the considered point (in world coordinates)
	//note that proper formula is:  lightRadius * (lightDist - occlDist) / occlDist;
	float blurRadiusWorld = lightRadius * (centerFragZ - avgBlockerZ) / avgBlockerZ;
	//blur radius relative to light distance
	//note: it is very important to limit blur angle <= search angle !
	float blurRadiusRel = min(blurRadiusWorld / lightDist, searchAngle);
	//minor radius of the ellipse, which is: the intersection of penumbra cone with cube's face
	float blurRadiusCube = blurRadiusRel * secFallAngle;
	//the same radius in shadowmap pixels
	float blurRadiusPixels = blurRadiusCube * shadowMapResolution;

	//limit blur radius from below: blur must cover at least (2*M) pixels in any direction
	//otherwise user will see the shitty pixelated shadows, which is VERY ugly
	//we prefer blurring shadows to hell instead of showing pixelation...
	const float minBlurInShadowPixels = 5.0;
	if (blurRadiusPixels < minBlurInShadowPixels) {
		float coeff = minBlurInShadowPixels / (blurRadiusPixels + 1e-7);
		//note: other versions of blurRadius are not used below
		blurRadiusRel *= coeff;
	}
	for (int i = 0; i < softQuality; i++) {
		//unit vector L' = perturbed version of L
		vec3 perturbedLightDir = normalize(L + blurRadiusRel * (u_softShadowsSamples[i].x * orthoAxisX + u_softShadowsSamples[i].y * orthoAxisY));
		//exactly raytrace tangent plane of the surface
		//the multiplier is:  (cos(D ^ L') / cos(N ^ L')) / (cos(D ^ L) / cos(N ^ L))
		//where L and L' is central/perturbed light direction, D is normal of active cubemap face, and N is normal of tangent plane
		float dotDpL = max(max(abs(perturbedLightDir.x), abs(perturbedLightDir.y)), abs(perturbedLightDir.z));
		float distCoeff = lightFallAngle / max(-dot(worldNormal, perturbedLightDir), 1e-3) * (dotDpL * secFallAngle);
		float fragZ = centerFragZ * distCoeff;
		float blockerZ = ShadowAtlasForVector(shadowMap, shadowRect, perturbedLightDir);
		lit += float(blockerZ >= fragZ - errorMargin);
	}
	lit /= softQuality + 1;

	return lit * fallAngleLimiter;
}
