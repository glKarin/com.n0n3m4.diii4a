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

#extension GL_ARB_texture_gather: enable

#define STGATILOV_USEGATHER 1

precision mediump float;

out vec4 FragColor;

#pragma tdm_include "stages/interaction/interaction.common.fs.glsl"
#define TDM_allow_ARB_texture_gather STGATILOV_USEGATHER
#pragma tdm_include "tdm_shadowmaps.glsl"

uniform bool 	u_shadowMapCullFront;
uniform vec4	u_shadowRect;
uniform sampler2D u_shadowMap;
in vec3 var_WorldLightDir;

void main() {
	InteractionGeometry props;
	FragColor.rgb = computeInteraction(props);

	vec3 worldNormal = mat3(u_modelMatrix) * (var_TangentBitangentNormalMatrix * props.localN);

	if (u_shadows) {
		float shadowsCoeff = computeShadowMapCoefficient(
			var_WorldLightDir, worldNormal,
			u_shadowMap, u_shadowRect,
			u_softShadowsQuality, u_softShadowsRadius, u_shadowMapCullFront
		);
		FragColor.rgb *= shadowsCoeff;
	}
	FragColor.a = 1.0;
}
