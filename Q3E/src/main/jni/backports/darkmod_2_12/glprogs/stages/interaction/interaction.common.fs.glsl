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
// Contains common formulas for computing interaction.
// Includes: illumination model, fetching surface and light properties
// Excludes: shadows

#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "tdm_interaction.glsl"
#pragma tdm_include "tdm_compression.glsl"

in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;
in vec4 var_Color;
in mat3 var_TangentBitangentNormalMatrix; 
in vec3 var_LightDirLocal;
in vec3 var_ViewDirLocal;

uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;

uniform sampler2D u_lightFalloffTexture;
uniform sampler2D u_lightProjectionTexture;
uniform bool u_cubic;
uniform samplerCube	u_lightProjectionCubemap;

uniform bool u_shadows;
uniform int u_softShadowsQuality;
uniform float u_softShadowsRadius;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_modelMatrix;
uniform mat4 u_projectionMatrix;
uniform vec4 u_lightTextureMatrix[2];
uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
uniform vec4 u_hasTextureDNS;
uniform int u_useBumpmapLightTogglingFix;
uniform float u_RGTC;


vec3 computeInteraction(out InteractionGeometry props) {
	vec3 lightColor;
	if (u_cubic)
		lightColor = projFalloffOfCubicLight(u_lightProjectionCubemap, var_TexLight).rgb;
	else
		lightColor = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, u_lightTextureMatrix, var_TexLight).rgb;

	vec3 localNormal = fetchSurfaceNormal(var_TexNormal, u_hasTextureDNS[1] != 0.0, u_normalTexture, u_RGTC != 0.0);
	props = computeInteractionGeometry(var_LightDirLocal, var_ViewDirLocal, localNormal);

	vec3 interactionColor = computeAdvancedInteraction(
		props,
		u_diffuseTexture, u_diffuseColor.rgb, var_TexDiffuse,
		u_specularTexture, u_specularColor.rgb, var_TexSpecular,
		var_Color.rgb,
		u_useBumpmapLightTogglingFix != 0
	);

	return interactionColor * lightColor;
}
