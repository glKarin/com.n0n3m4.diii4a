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

#pragma tdm_include "tdm_constants_shared.glsl"

// various helpers for transforming texcoords for different interaction stages
vec2 transformSurfaceTexCoords(vec2 attrTexCoord, bool hasMatrix, vec4 texMatrix[2]) {
	if (!hasMatrix)
		return attrTexCoord;
	mat2 matr = mat2(texMatrix[0].xy, texMatrix[1].xy);
	return attrTexCoord * matr + vec2(texMatrix[0].w, texMatrix[1].w);
}
vec2 transformSurfaceTexOffset(vec2 attrTexCoord, bool hasMatrix, vec4 texMatrix[2]) {
	if (!hasMatrix)
		return attrTexCoord;
	mat2 matr = mat2(texMatrix[0].xy, texMatrix[1].xy);
	return attrTexCoord * matr;
}

// computes all vertex shader outputs related to surface texturing & coloring
void generateSurfaceProperties(
	vec4 attrTexCoord, vec4 attrColor,
	vec3 attrTangent, vec3 attrBitangent, vec3 attrNormal,
	vec4 normalMatrix[2], vec4 parallaxMatrix[2], vec4 diffuseMatrix[2], vec4 specularMatrix[2],
	vec4 colorModulate, vec4 colorAdd,
	int flags,
	out vec2 texNormal, out vec2 texParallax, out vec2 texDiffuse, out vec2 texSpecular,
	out vec4 vertexColor, out mat3 matTangentToLocal
) {
	// per-stage texcoords generation
	bool hasMatrix = checkFlag(flags, SFL_SURFACE_HAS_TEXTURE_MATRIX);
	texNormal = transformSurfaceTexCoords(attrTexCoord.xy, hasMatrix, normalMatrix);
	texParallax = transformSurfaceTexCoords(attrTexCoord.xy, hasMatrix, parallaxMatrix);
	texDiffuse = transformSurfaceTexCoords(attrTexCoord.xy, hasMatrix, diffuseMatrix);
	texSpecular = transformSurfaceTexCoords(attrTexCoord.xy, hasMatrix, specularMatrix);

	// color generation
	vertexColor = attrColor * colorModulate + colorAdd;

	// tangent space matrix (mainly for bumpmapping)
	matTangentToLocal = mat3(
		clamp(attrTangent, -1, 1),
		clamp(attrBitangent, -1, 1),
		clamp(attrNormal, -1, 1)
	);
}


// describes local geometry of surface, light and view origin in tangent space
struct InteractionGeometry {
	vec3 localL;	// unit direction from fragment to light source
	vec3 localV;	// unit direction from fragment to view origin
	vec3 localH;	// unit direction: bisector between to-light and to-view
	vec3 localN;	// unit normal (bump/normal map)
	float NdotV;
	float NdotL;
	float NdotH;
};

InteractionGeometry computeInteractionGeometry(vec3 localToLight, vec3 localToView, vec3 localNormal) {
	InteractionGeometry props;
	props.localL = normalize(localToLight);
	props.localV = normalize(localToView);
	props.localN = localNormal;	// should be normalized in unpackSurfaceNormal
	props.localH = normalize(props.localV + props.localL);
	// must be done in tangent space, otherwise smoothing will suffer (see #4958)
	props.NdotL = clamp(dot(props.localN, props.localL), 0.0, 1.0);
	props.NdotV = clamp(dot(props.localN, props.localV), 0.0, 1.0);
	props.NdotH = clamp(dot(props.localN, props.localH), 0.0, 1.0);
	return props;
}

//-------------------------------------------------------------------------------------

float applyBumpmapTogglingFix(InteractionGeometry props, bool enabled) {
	if (enabled) {
		// stgatilov: hacky coefficient to make lighting smooth when L is almost in surface tangent plane
		float MNdotL = max(props.localL.z, 0);       	// dot(mesh_normal, light_dir) in tangent space
		if (MNdotL < min(0.25, props.NdotL))
			return mix(MNdotL, props.NdotL, MNdotL / 0.25);
	}
	return props.NdotL;
}

struct FresnelRimCoeffs {
	float rimLight;
	float fresnelCoeff;
	float R2f;
};
FresnelRimCoeffs computeFresnelRimCoefficients(InteractionGeometry props) {
	// fresnel part, ported from test_direct.vfp
	float fresnelTerm = pow(1.0 - props.NdotV, 4.0);
	FresnelRimCoeffs res;
	res.rimLight = fresnelTerm * clamp(props.NdotL - 0.3, 0.0, 0.5) * 1.8;
	res.fresnelCoeff = fresnelTerm * 0.23 + 0.023;
	res.R2f = clamp(props.localL.z * 4.0, 0.0, 1.0);
	return res;
}

vec3 computeSpecularTerm(InteractionGeometry props, vec3 specularTexColor, FresnelRimCoeffs fresnelRim) {
	float specularPower = mix(10.0, 30.0, specularTexColor.z);
	float specularCoeff = pow(props.NdotH, specularPower) * 120.0;
	return specularCoeff * fresnelRim.fresnelCoeff * specularTexColor;
}

vec3 computeAdvancedInteraction(
	// interaction properties:
	InteractionGeometry props,
	// surface color:
	vec3 diffuseParamColor, vec4 diffuseTexColorA,
	vec3 specularParamColor, vec4 specularTexColorA,
	vec3 vertexColor,
	// light parameters:
	bool bumpmapTogglingFixEnabled
) {
	vec3 diffuseTexColor = diffuseTexColorA.rgb;
	vec3 specularTexColor = specularTexColorA.rgb;

	FresnelRimCoeffs fresnelRim = computeFresnelRimCoefficients(props);
	vec3 specularTerm = computeSpecularTerm(props, specularTexColor, fresnelRim);

	vec3 surfaceTerm = specularParamColor * specularTerm * fresnelRim.R2f + diffuseParamColor * diffuseTexColor;
	float NdotL_adjusted = applyBumpmapTogglingFix(props, bumpmapTogglingFixEnabled);
	float globalMultiplier = NdotL_adjusted + fresnelRim.rimLight * fresnelRim.R2f;

	vec3 totalColor = surfaceTerm * globalMultiplier * vertexColor;
	return totalColor;
}


struct AmbientGeometry {
	vec3 worldV;	// unit direction from fragment to view origin
	vec3 worldN;	// unit normal (bump/normal map)
	vec3 worldR;    // direction to view, mirrored relative to normal
	float NdotV;
};

AmbientGeometry computeAmbientGeometry(vec3 globalToView, vec3 localNormal, mat3 matTangentToObject, mat3 modelMatrix) {
	AmbientGeometry props;
	props.worldV = normalize(globalToView);
	props.worldN = normalize(modelMatrix * (matTangentToObject * localNormal));
	props.NdotV = clamp(dot(props.worldN, props.worldV), 0.0, 1.0);
	props.worldR = 2 * props.worldN * props.NdotV - props.worldV;
	return props;
}

vec4 computeAmbientInteraction(
	// interaction properties:
	AmbientGeometry props,
	// surface color:
	vec3 diffuseParamColor, vec4 diffuseTexColorA,
	vec3 specularParamColor, vec4 specularTexColorA,
	vec3 vertexColor,
	// light properties
	bool useNormalIndexedDiffuse, bool useNormalIndexedSpecular, samplerCube normalIndexedDiffuse, samplerCube normalIndexedSpecular,
	// ambient hack for general brightness
	float ambientMinLevel, float ambientGamma
) {
	// compute the diffuse term
	vec3 diffuseTexColor = diffuseTexColorA.rgb;
	float diffuseTexAlpha = diffuseTexColorA.a;
	vec3 specularTexColor = specularTexColorA.rgb;

	// diffuse term
	vec3 diffuseTerm = vec3(1);
	if (useNormalIndexedDiffuse)
		diffuseTerm = texture(normalIndexedDiffuse, props.worldN).rgb;
	diffuseTerm *= diffuseParamColor;

	// tweaking brightness by messing with ambient
	if (ambientMinLevel != 0)
		diffuseTerm = mix(diffuseTerm, vec3(1), ambientMinLevel);

	// specular term
	vec3 specularTerm = vec3(0);
	if (useNormalIndexedSpecular)
		specularTerm = texture(normalIndexedSpecular, props.worldR).rgb;
	specularTerm *= specularParamColor;

	vec3 surfaceTerm = (diffuseTerm * diffuseTexColor.rgb + specularTerm * specularTexColor) * vertexColor;
	vec4 result = vec4(surfaceTerm, diffuseTexAlpha);

	// avoid negative values, which with floating point render buffers can lead to NaN artefacts
	result = max(result, vec4(0));
	// tweaking brightness by messing with ambient
	if (ambientGamma != 1)
		result.rgb = pow(result.rgb, vec3(1.0 / ambientGamma));

	return result;
}
