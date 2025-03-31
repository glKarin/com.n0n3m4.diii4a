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
#pragma tdm_include "tdm_shadowstencilsoft_shared.glsl"
#pragma tdm_include "tdm_poissondisk.glsl"  // note: adds uniform

float fetchStencilShadowTexture(usampler2D stencilTexture, vec2 texCoord) {
	float stTex = float(texture(stencilTexture, texCoord).r);
	return clamp(129.0 - stTex, 0.0, 1.0);
}

// note: this function must be run while rendering the shadowed surface
// it uses gl_FragCoord and dfdx/dfdy (including depth/W components)
float computeStencilSoftShadow(
	usampler2D stencilTexture, sampler2D depthTexture,
	vec3 objectToLight, vec3 objectNormal,
	mat4 modelViewMatrix, mat4 projectionMatrix,
	int softQuality, float softRadius,
	sampler2D stencilMipmapsTexture, ivec2 stencilMipmapsLevel, vec4 stencilMipmapsScissor 
) {
	vec2 texSize = vec2(textureSize(stencilTexture, 0));
	vec2 pixSize = vec2(1.0, 1.0) / texSize;
	vec2 baseTexCoord = gl_FragCoord.xy * pixSize;

	//check stencil mipmaps if there is constant value in vicinity
	if (stencilMipmapsLevel.x >= 0) {
		vec2 pixelCoords = clamp(gl_FragCoord.xy, stencilMipmapsScissor.xy, stencilMipmapsScissor.zw);
		vec2 mipmapsFullSize = textureSize(stencilMipmapsTexture, 0) * (1 << stencilMipmapsLevel.y);
		float mipmapValue = textureLod(
			stencilMipmapsTexture,
			pixelCoords / mipmapsFullSize,
			stencilMipmapsLevel.x - stencilMipmapsLevel.y
		).r;
		if (mipmapValue == 0.0)
			return 0.0;
		else if (mipmapValue == 1.0)
			return 1.0;
	}

	float stencil = fetchStencilShadowTexture(stencilTexture, baseTexCoord);
	float sumWeight = 1.0;
	if (softQuality == 0) {
		//actually, this case is handled very differently on C++ side
		return stencil;
	}

	float lightDist = min(length(objectToLight), 1e3); // crutch !
	//radius of light source
	float lightRadius = softRadius;
	//radius of one-point penumbra at the consided point (in world coordinates)
	//note that proper formula is:  lightRadius * (lightDist - occlDist) / occlDist;
	float blurRadiusWorld = lightRadius * lightDist / 66.6666;  //TODO: revert?!

	//project direction to light onto surface
	vec3 alongDirW = normalize(objectToLight - dot(objectToLight, objectNormal) * objectNormal);
	//get orthogonal direction on surface
	vec3 orthoDirW = cross(objectNormal, alongDirW);
	//multiply the two axes by penumbra radius
	float NdotL = dot(normalize(objectToLight), objectNormal);
	alongDirW *= blurRadiusWorld / max(NdotL, 0.2);  //penumbra is longer by (1/cos(a)) in light direction
	orthoDirW *= blurRadiusWorld;

	//convert both vectors into clip space (get only X and Y components)
	mat4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
	vec2 alongDir = (mat3(modelViewProjectionMatrix) * alongDirW).xy;
	vec2 orthoDir = (mat3(modelViewProjectionMatrix) * orthoDirW).xy;
	//now also get W component from multiplication by gl_ModelViewProjectionMatrix
	vec3 mvpRow3 = vec3(modelViewProjectionMatrix[0][3], modelViewProjectionMatrix[1][3], modelViewProjectionMatrix[2][3]);
	float along_w = dot(mvpRow3, alongDirW);
	float ortho_w = dot(mvpRow3, orthoDirW);
	//this is perspective correction: it is necessary because W component in clip space also varies
	//if you remove it and look horizontally parallel to a wall, then vertical shadow boundaries on this wall won't be blurred
	vec2 thisNdc = (2 * baseTexCoord - vec2(1));
	alongDir -= thisNdc * along_w;
	orthoDir -= thisNdc * ortho_w;
	//divide by clip W to get NDC coords (screen coords are half of them)
	alongDir *= gl_FragCoord.w / 2;
	orthoDir *= gl_FragCoord.w / 2;
	//Note: if you want to check the math just above, consider how screen position changes when a point moves in specified direction:
	//  F(t) = divideByW(gl_ModelViewProjectionMatrix * (var_Position + dir_world * t)).xy
	//the converted vector must be equal to the derivative by parameter:
	//  dir_screen = dF/dt (0)
	//(here [dir_world, dir_screen] are either [alongDirW, alongDir] or [orthoDirW, orthoDir])

	//estimate the length of spot ellipse vectors (in pixels)
	float lenX = length(alongDir * texSize);
	float lenY = length(orthoDir * texSize);
	//make sure vectors are sufficiently sampled
	float maxBlurAxisLength = computeMaxBlurAxisLength(texSize.y, softQuality);
	float oversize = max(lenX, lenY) / maxBlurAxisLength;
	if (oversize > 1) {
		alongDir /= oversize;
		orthoDir /= oversize;
	}

	//compute partial derivatives of eye -Z by screen X and Y (normalized)
	float Z00 = depthToZ(projectionMatrix, gl_FragCoord.z);
	vec2 dzdxy = vec2(dFdx(Z00), dFdy(Z00));
	//rescale to derivatives by texture coordinates (not pixels)
	dzdxy *= texSize;
	//compute Z derivatives on a theoretical wall visible under 45-degree angle
	vec2 tanFovHalf = vec2(1.0 / projectionMatrix[0][0], 1.0 / projectionMatrix[1][1]);
	vec2 canonDerivs = 2.0 * Z00 * tanFovHalf;

	for( int i = 0; i < softQuality; i++ ) {
		vec2 delta = u_softShadowsSamples[i].x * alongDir + u_softShadowsSamples[i].y * orthoDir;
		vec2 sampleTexCoord = baseTexCoord + delta;

		// ignore samples with drastically different Z/depth
		// otherwise we get obvious "halo effect" around objects
		float Zdiff = depthToZ(projectionMatrix, texture(depthTexture, sampleTexCoord).r) - Z00;
		float tangentZdiff = dot(dzdxy, delta);
		float deg45diff = dot(canonDerivs, abs(delta));
		float ZdiffTol = abs(tangentZdiff) * 0.5 + deg45diff * 0.2;
		float weight = float(abs(Zdiff - tangentZdiff) <= ZdiffTol);

		stencil += fetchStencilShadowTexture(stencilTexture, sampleTexCoord) * weight;
		sumWeight += weight;
	}

	return stencil / sumWeight;
}
