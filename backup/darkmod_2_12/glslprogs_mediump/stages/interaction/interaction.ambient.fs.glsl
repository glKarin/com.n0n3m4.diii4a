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

precision highp float;

#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "tdm_interaction.glsl"
#pragma tdm_include "tdm_compression.glsl"

in vec2 var_TexDiffuse;
in vec2 var_TexSpecular;
in vec2 var_TexNormal;
in vec4 var_TexLight;
in vec4 var_Color;
in mat3 var_TangentBinormalNormalMatrix;
in vec3 var_worldViewDir;

out vec4 FragColor;

uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;

uniform sampler2D u_lightProjectionTexture;
uniform sampler2D u_lightFalloffTexture;
uniform bool u_cubic;
uniform samplerCube u_lightProjectionCubemap;   // TODO: is this needed?
uniform bool u_useNormalIndexedDiffuse, u_useNormalIndexedSpecular;
uniform samplerCube u_lightDiffuseCubemap;
uniform samplerCube u_lightSpecularCubemap;

uniform float u_gamma, u_minLevel;

uniform vec2 u_renderResolution;
uniform sampler2D u_ssaoTexture;
uniform int u_ssaoEnabled;

uniform vec4 u_lightTextureMatrix[2];
uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
uniform vec4 u_hasTextureDNS;
uniform float u_RGTC;
uniform highp mat4 u_modelMatrix;

void main() {
	vec3 lightColor;
	if (u_cubic)
		lightColor = projFalloffOfCubicLight(u_lightProjectionCubemap, var_TexLight).rgb;
	else
		lightColor = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, u_lightTextureMatrix, var_TexLight).rgb;

	vec3 localNormal = fetchSurfaceNormal(var_TexNormal, u_hasTextureDNS[1] != 0.0, u_normalTexture, u_RGTC != 0.0);
	AmbientGeometry props = computeAmbientGeometry(var_worldViewDir, localNormal, var_TangentBinormalNormalMatrix, mat3(u_modelMatrix));

	vec4 interactionColor = computeAmbientInteraction(
		props,
		u_diffuseTexture, u_diffuseColor.rgb, var_TexDiffuse,
		u_specularTexture, u_specularColor.rgb, var_TexSpecular,
		var_Color.rgb,
		u_useNormalIndexedDiffuse, u_useNormalIndexedSpecular, u_lightDiffuseCubemap, u_lightSpecularCubemap,
		u_minLevel, u_gamma
	);

	float ssao = 1.0;
	if (u_ssaoEnabled == 1) {
		ssao = texture(u_ssaoTexture, gl_FragCoord.xy / u_renderResolution).r;
	}

	FragColor = vec4(lightColor * ssao * interactionColor.rgb, interactionColor.a);
}
