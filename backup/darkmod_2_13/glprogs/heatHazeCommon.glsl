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

vec4 heatHazeVertexShader(
	vec4 attrPosition, vec4 attrTexCoord,
	mat4 modelViewMatrix, mat4 projectionMatrix,
	vec2 texcoordScroll, vec2 deformMultiplier,
	out vec2 texcoordOriginal, out vec2 texcoordScrolled, out vec2 deformationMagnitude
) {
	// texture 0 takes the texture coordinates unmodified
	texcoordOriginal = attrTexCoord.xy;
	// texture 1 takes the texture coordinates and adds a scroll
	texcoordScrolled = attrTexCoord.xy + texcoordScroll;

	// consider a point at the fragment being rendered
	// if we move it slightly in view XY plane, how much will NDC coordinates change?
	// (basically, compute derivative of NDC coord X by view X)
	float viewZ = dot(transpose(modelViewMatrix)[2], attrPosition);
	float clipW = projectionMatrix[2][3] * viewZ + projectionMatrix[3][3];
	float derClipX = projectionMatrix[0][0];
	float derNdcX = derClipX / max(clipW, 1e-10);

	// clamp the distance so the the deformations don't get too wacky near the view
	deformationMagnitude = deformMultiplier * min(derNdcX, 0.02);

	return objectPosToClip(attrPosition, modelViewMatrix, projectionMatrix);
}

vec3 heatHazeFragmentShader(
	vec2 texcoordOriginal, vec2 texcoordScrolled, vec2 deformationMagnitude,
	vec4 glFragCoord, vec2 invRenderSize,
	in sampler2D currentRender, in sampler2D displacementTexture,
	bool saveForeground, in sampler2D currentDepth, 
	bool useMask, in sampler2D maskTexture,
	float blurDistance
) {
	// load displacement vector from the texture
	// note: it is also called "normal texture" for some weird reason
	vec4 displacementRawTex = texture(displacementTexture, texcoordScrolled);
	// unpack / rescale to [-1 .. 1]
	// note: engine often/always uses RGTC compression on this texture
	// we can easily unpack XY coords regardless of RGTC, but Z coordinate is not available
	vec2 displacementTex = 2 * displacementRawTex.xy - vec2(1);

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 originalPos = glFragCoord.xy * invRenderSize;

	// total displacement for sampling frame
	vec2 displacement = displacementTex * deformationMagnitude;

	if (useMask) {
		// multiply by mask sampled by original texcoord
		displacement *= texture(maskTexture, texcoordOriginal).rg;
	}

	// get displaced color
	vec2 displacedPos = clamp(originalPos + displacement, 0.0, 1.0);
	vec3 displacedColor = texture(currentRender, displacedPos).rgb;
	vec3 chosenColor = displacedColor;

	if (saveForeground) {
		vec2 depthRenderSizeRatio = textureSize(currentDepth, 0) * invRenderSize;

		// get another sample from the opposite direction, to use if the sample came from the foreground
		vec2 oppositePos = clamp(originalPos - displacement, 0.0, 1.0);
		vec3 oppositeColor = texture(currentRender, oppositePos).rgb;

		// test whether the samples came from the foreground, rejecting them if so.
		// original sample > displaced sample > opposite sample
		vec3 originalColor = texture(currentRender, originalPos).rgb;
		chosenColor = originalColor;

		// test whether the opposite sample came from the background, use it if so
		vec2 oppositePosD = oppositePos * depthRenderSizeRatio;
		float oppositeDepth = texture(currentDepth, oppositePosD).z;
		if (oppositeDepth >= glFragCoord.z)
			chosenColor = oppositeColor;

		// test whether the displaced sample came from the background, use it if so
		vec2 displacedPosD = displacedPos * depthRenderSizeRatio;
		float displacedDepth = texture(currentDepth, displacedPosD).z;
		if (displacedDepth >= glFragCoord.z)
			chosenColor = displacedColor;
	}

	if (blurDistance > 0.0) {
		vec3 sum = vec3(0);
		vec2 pos;
		pos = displacedPos + vec2(blurDistance, 0) * invRenderSize;
		sum += texture(currentRender, pos).rgb;
		pos = displacedPos + vec2(0, blurDistance) * invRenderSize;
		sum += texture(currentRender, pos).rgb;
		pos = displacedPos + vec2(-blurDistance, 0) * invRenderSize;
		sum += texture(currentRender, pos).rgb;
		pos = displacedPos + vec2(0, -blurDistance) * invRenderSize;
		sum += texture(currentRender, pos).rgb;
		chosenColor = (chosenColor + sum) * 0.2;
	}

	return chosenColor;
}
