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
#version 330 core

#pragma tdm_include "tdm_utils.glsl"

uniform sampler2D u_particleColorTexture;
uniform sampler2D u_depthTexture;
uniform mat4 u_projectionMatrix;
uniform float u_invParticleRadius;
uniform float u_invSceneFadeCoeff;
uniform vec2 u_invDepthTextureSize;
uniform vec4 u_fadeMask;

in vec4 var_Color;
in vec2 var_TexCoord;

out vec4 FragColor;

void main() {
	vec2 screenTc = gl_FragCoord.xy * u_invDepthTextureSize;

	// take depth of particle and background
	float sceneDepth = texture(u_depthTexture, screenTc).r;
	float particleDepth = gl_FragCoord.z;
	// convert to linear Z (positive)
	sceneDepth = depthToZ(u_projectionMatrix, sceneDepth);
	particleDepth = depthToZ(u_projectionMatrix, particleDepth);
	float depthDelta = sceneDepth - particleDepth;

	// scale the depth difference by the particle diameter to calc an alpha value
	// based on how much of the 3d volume represented by the particle is in front of the solid scene
	float sceneFade = clamp((depthDelta * u_invParticleRadius + 1.0) * u_invSceneFadeCoeff, 0.0, 1.0);
	// also fade if the particle is too close to our eye position, so it doesn't 'pop' in and out of view
	// start a linear fade at u_particleRadius distance from the particle.
	float nearFade = clamp(particleDepth * u_invParticleRadius, 0.0, 1.0);

	// calculate final fade and apply the channel mask
	vec4 fade = clamp(vec4(sceneFade * nearFade) + u_fadeMask, 0.0, 1.0);

	// multiply color by vertex/fragment color as that's how the particle system fades particles in and out
	FragColor = texture(u_particleColorTexture, var_TexCoord) * fade * var_Color;
}
