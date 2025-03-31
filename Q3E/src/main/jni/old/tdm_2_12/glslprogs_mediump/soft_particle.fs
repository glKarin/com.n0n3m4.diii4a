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

uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform vec4 u_scaleDepthCoords;
uniform vec4 u_softParticleBlend;
uniform vec4 u_softParticleParams;

in vec4 var_color;
in vec2 var_texCoord;

out vec4 draw_Color;

void main() {
	// == Fragment Program ==
	//
	// Input textures
	//   texture[0]   particle diffusemap
	//   texture[1]   _currentDepth
	//
	// Constants set by the engine:
	//   program.env[4] is reciprocal of _currentDepth size. Lets us convert a screen position to a texcoord in _currentDepth
	//   program.env[5] is the particle radius, given as { radius, 1/(fadeRange), 1/radius }
	//		fadeRange is the particle diameter for alpha blends (like smoke), but the particle radius for additive
	// 		blends (light glares), because additive effects work differently. Fog is half as apparent when a wall
	// 		is in the middle of it. Light glares lose no visibility when they have something to reflect off.
	//   program.env[6] is the color channel mask. Particles with additive blend need their RGB channels modifying to blend them out.
	//                                             Particles with an alpha blend need their alpha channel modifying.
	//
	// Hard-coded constants
	//    depth_consts allows us to recover the original depth in Doom units of anything in the depth
	//    buffer. TDM's projection matrix differs slightly from the classic projection matrix as it
	//    implements a "nearly-infinite" zFar. The matrix is hard-coded in the engine, so we use hard-coded
	//    constants here for efficiency. depth_consts is derived from the numbers in that matrix.
	//

	// fetch background depth from depth texture
	vec2 screenTc = gl_FragCoord.xy * u_scaleDepthCoords.xy;
	float sceneDepth = texture(u_texture1, screenTc).r;
	sceneDepth = min(sceneDepth, 0.9994);

	// convert depth to linear for background and particle
	sceneDepth = 1.0 / (sceneDepth * 0.33333333 - 0.33316667);
	float particleDepth = 1.0 / (gl_FragCoord.z * 0.33333333 - 0.33316667);

	// Scale the depth difference by the particle diameter to calc an alpha
	// value based on how much of the 3d volume represented by the particle
	// is in front of the solid scene
	float tmp = particleDepth - sceneDepth + u_softParticleParams.x;
	float sceneFade = clamp(tmp * u_softParticleParams.y, 0.0, 1.0);                                             //MUL_SAT  fade, tmp, particle_radius.y;

	// Also fade if the particle is too close to our eye position, so it doesn't 'pop' in and out of view
	// Start a linear fade at ??? distance from the particle.
	float nearFade = clamp(particleDepth * -u_softParticleParams.z, 0.0, 1.0);                            //MUL_SAT  near_fade, particle_depth, -particle_radius.z;

	// Calculate final fade and apply the channel mask
	vec4 fade = clamp(vec4(sceneFade * nearFade) + u_softParticleBlend, 0.0, 1.0);

	// multiply color by vertex/fragment color as that's how the particle system fades particles in and out
	draw_Color = texture(u_texture0, var_texCoord) * fade * var_color;
}
