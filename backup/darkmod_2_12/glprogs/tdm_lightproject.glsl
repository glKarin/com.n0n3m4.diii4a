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

vec4 computeLightTex(mat4 lightProjectionFalloff, vec4 position) {
	//in: divisor in Z, falloff in W
	return ( position * lightProjectionFalloff ).xywz;
	//out: divisor in W, falloff in Z
}

vec4 projFalloffOfNormalLight(in sampler2D lightProjectionTexture, in sampler2D lightFalloffTexture, in vec4 texMatrix[2], vec4 texCoord) {
	float falloffCoord = texCoord.z;
	vec4 projCoords = texCoord;     //divided by last component
	projCoords.z = 0.0;

	vec3 projTexCoords;
	projTexCoords.x = dot(projCoords, texMatrix[0]);
	projTexCoords.y = dot(projCoords, texMatrix[1]);
	projTexCoords.z = projCoords.w;

	vec4 lightProjection = textureProj(lightProjectionTexture, projTexCoords);
	vec4 lightFalloff = texture(lightFalloffTexture, vec2(falloffCoord, 0.5));

	// stgatilov #5876: this "if" MUST be after texture fetches!
	// not because it is better, but because of AMD driver's stupidity
	// moving this "if" before texture fetches results in bright lines on light volume boundary
	if (
		projCoords.w <= 0 ||                                            //anything with inversed W
		projCoords.x < 0 || projCoords.x > projCoords.w ||              //proj U outside [0..1]
		projCoords.y < 0 || projCoords.y > projCoords.w ||              //proj V outside [0..1]
		falloffCoord < 0 || falloffCoord > 1.0                          //falloff outside [0..1]
	) {
		return vec4(0);
	}

	return lightProjection * lightFalloff;
}

vec4 projFalloffOfCubicLight(in samplerCube	lightProjectionCubemap, vec4 texLight) {
	vec3 cubeTC = texLight.xyz * 2.0 - 1.0;
	vec4 lightColor = texture(lightProjectionCubemap, cubeTC);
	float att = clamp(1.0 - length(cubeTC), 0.0, 1.0);
	lightColor *= att * att;
	return lightColor;
}
