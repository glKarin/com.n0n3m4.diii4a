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

#pragma tdm_include "tdm_lightproject.glsl"
#pragma tdm_include "tdm_interaction.glsl"
#pragma tdm_include "tdm_compression.glsl"
#pragma tdm_include "tdm_parallax.glsl"
#pragma tdm_include "tdm_constants_shared.glsl"

in vec2 var_TexNormal;
in vec2 var_TexParallax;
in vec2 var_TexDiffuse;
in vec2 var_TexSpecular;
in vec2 var_TexCoord;
in vec4 var_TexLight;
in vec4 var_Color;
in mat3 var_TangentBitangentNormalMatrix;
in vec3 var_worldViewDir;
in vec3 var_LightDirLocal;
in vec3 var_ViewDirLocal;

out vec4 FragColor;

uniform int u_flags;

uniform sampler2D u_normalTexture;
uniform sampler2D u_diffuseTexture;
uniform sampler2D u_specularTexture;
uniform sampler2D u_parallaxTexture;

uniform sampler2D u_lightProjectionTexture;
uniform sampler2D u_lightFalloffTexture;
uniform samplerCube u_lightProjectionCubemap;   // TODO: is this needed?
uniform samplerCube u_lightDiffuseCubemap;
uniform samplerCube u_lightSpecularCubemap;

uniform float u_gamma, u_minLevel;

uniform vec2 u_renderResolution;
uniform sampler2D u_ssaoTexture;

uniform vec4 u_lightTextureMatrix[2];
uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
uniform mat4 u_modelMatrix;

uniform vec4 u_bumpMatrix[2];
uniform vec4 u_diffuseMatrix[2];
uniform vec4 u_specularMatrix[2];

uniform vec2 u_parallaxHeightScale;
uniform ivec3 u_parallaxIterations;
uniform float u_parallaxGrazingAngle;

void main() {
	vec4 diffuseTexColor, specularTexColor, normalTexColor;

	if (checkFlag(u_flags, SFL_SURFACE_HAS_PARALLAX_TEXTURE)) {
		vec3 offset = computeParallaxOffset(
			u_parallaxTexture, u_parallaxHeightScale,
			var_TexParallax, var_ViewDirLocal,
			u_parallaxGrazingAngle, u_parallaxIterations.xy
		);

		bool hasTextureMatrix = checkFlag(u_flags, SFL_SURFACE_HAS_TEXTURE_MATRIX);
		vec2 texNormal = var_TexNormal + transformSurfaceTexOffset(offset.xy, hasTextureMatrix, u_bumpMatrix);
		vec2 texDiffuse = var_TexDiffuse + transformSurfaceTexOffset(offset.xy, hasTextureMatrix, u_diffuseMatrix);
		vec2 texSpecular = var_TexSpecular + transformSurfaceTexOffset(offset.xy, hasTextureMatrix, u_specularMatrix);

		// use original gradients to avoid artifacts on relief silhouette
		normalTexColor = textureGrad(u_normalTexture, texNormal, dFdx(var_TexNormal), dFdy(var_TexNormal));
		diffuseTexColor = textureGrad(u_diffuseTexture, texDiffuse, dFdx(var_TexDiffuse), dFdy(var_TexDiffuse));
		specularTexColor = textureGrad(u_specularTexture, texSpecular, dFdx(var_TexSpecular), dFdy(var_TexSpecular));
	}
	else {
		normalTexColor = texture(u_normalTexture, var_TexNormal);
		diffuseTexColor = texture(u_diffuseTexture, var_TexDiffuse);
		specularTexColor = texture(u_specularTexture, var_TexSpecular);
	}

	vec3 lightColor;
	if (checkFlag(u_flags, SFL_LIGHT_CUBIC))
		lightColor = projFalloffOfCubicLight(u_lightProjectionCubemap, var_TexLight).rgb;
	else
		lightColor = projFalloffOfNormalLight(u_lightProjectionTexture, u_lightFalloffTexture, u_lightTextureMatrix, var_TexLight).rgb;

	vec3 localNormal = unpackSurfaceNormal(
		normalTexColor,
		checkFlag(u_flags, SFL_SURFACE_HAS_NORMAL_TEXTURE),
		checkFlag(u_flags, SFL_SURFACE_NORMAL_TEXTURE_RGTC)
	);
	AmbientGeometry props = computeAmbientGeometry(
		var_worldViewDir, localNormal, var_TangentBitangentNormalMatrix,
		mat3(u_modelMatrix)
	);

	vec4 interactionColor = computeAmbientInteraction(
		props,
		u_diffuseColor.rgb, diffuseTexColor,
		u_specularColor.rgb, specularTexColor,
		var_Color.rgb,
		checkFlag(u_flags, SFL_LIGHT_AMBIENT_HAS_DIFFUSE_CUBEMAP),
		checkFlag(u_flags, SFL_LIGHT_AMBIENT_HAS_SPECULAR_CUBEMAP),
		u_lightDiffuseCubemap, u_lightSpecularCubemap,
		u_minLevel, u_gamma
	);

	float ssao = 1;
	if (checkFlag(u_flags, SFL_LIGHT_AMBIENT_HAS_SSAO)) {
		ssao = texture(u_ssaoTexture, gl_FragCoord.xy / u_renderResolution).r;
	}

	FragColor = vec4(lightColor * ssao * interactionColor.rgb, interactionColor.a);
}
