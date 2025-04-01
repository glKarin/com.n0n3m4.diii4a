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
#extension GL_ARB_texture_query_lod: enable

out vec4 FragColor;

#pragma tdm_include "tdm_utils.glsl"
#pragma tdm_include "stages/interaction/interaction.common.fs.glsl"
#pragma tdm_include "tdm_shadowstencilsoft.glsl"

uniform usampler2D u_stencilTexture;
uniform sampler2D u_depthTexture;

uniform sampler2D u_stencilMipmapsTexture;
uniform ivec2 u_stencilMipmapsLevel;
uniform vec4 u_stencilMipmapsScissor;

void main() {
	InteractionGeometry props;
	vec3 worldParallax;
	FragColor.rgb = computeInteraction(props, worldParallax);

	vec3 objectToLight = var_TangentBitangentNormalMatrix * var_LightDirLocal;
	vec3 objectNormal = var_TangentBitangentNormalMatrix[2];

	if (checkFlag(u_flags, SFL_INTERACTION_SHADOWS) && u_softShadowsQuality > 0) {
		FragColor.rgb *= computeStencilSoftShadow(
			u_stencilTexture, u_depthTexture,
			objectToLight, objectNormal,
			u_modelViewMatrix, u_projectionMatrix,
			u_softShadowsQuality, u_softShadowsRadius,
			u_stencilMipmapsTexture, u_stencilMipmapsLevel, u_stencilMipmapsScissor
		);
	}
	FragColor.a = 1.0;
}
