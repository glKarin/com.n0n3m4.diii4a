/*
 * Copyright (C) 2012  Oliver McFadden <omcfadde@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#version 100
//#pragma optimize(off)

precision mediump float;

/*
 * Pixel values between vertices are interpolated by Gouraud shading by default,
 * rather than the more computationally-expensive Phong shading.
 */
//#define BLINN_PHONG

/*
 * To soften the diffuse contribution from local lights, the dot product from
 * the Lambertian model is scaled by 1/2, add 1/2 and squared.  The result is
 * that this dot product, which normally lies in the range of -1 to +1, is
 * instead in the range of 0 to 1 and has a more pleasing falloff.
 */
//#define HALF_LAMBERT

varying vec2 var_TexDiffuse;
varying vec2 var_TexNormal;
varying vec2 var_TexSpecular;
varying vec4 var_TexLight;
varying lowp vec4 var_Color;
varying vec3 var_L;
#if defined(BLINN_PHONG)
varying vec3 var_H;
#else
varying vec3 var_V;
#endif

uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
//uniform float u_specularExponent;

uniform sampler2D u_fragmentMap0;	/* u_bumpTexture */
uniform sampler2D u_fragmentMap1;	/* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;	/* u_lightProjectionTexture */
uniform sampler2D u_fragmentMap3;	/* u_diffuseTexture */
uniform sampler2D u_fragmentMap4;	/* u_specularTexture */
uniform sampler2D u_fragmentMap5;	/* u_specularFalloffTexture */

void main(void)
{
	float u_specularExponent = 4.0;

	vec3 L = normalize(var_L);
#if defined(BLINN_PHONG)
	vec3 H = normalize(var_H);
	vec3 N = 2.0 * texture2D(u_fragmentMap0, var_TexNormal.st).agb - 1.0;
#else
	vec3 V = normalize(var_V);
	vec3 N = normalize(2.0 * texture2D(u_fragmentMap0, var_TexNormal.st).agb - 1.0);
#endif

	float NdotL = clamp(dot(N, L), 0.0, 1.0);
#if defined(HALF_LAMBERT)
	NdotL *= 0.5;
	NdotL += 0.5;
	NdotL = NdotL * NdotL;
#endif
#if defined(BLINN_PHONG)
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
#endif

	vec3 lightProjection = texture2DProj(u_fragmentMap2, var_TexLight.xyw).rgb;
	vec3 lightFalloff = texture2D(u_fragmentMap1, vec2(var_TexLight.z, 0.5)).rgb;
	vec3 diffuseColor = texture2D(u_fragmentMap3, var_TexDiffuse).rgb * u_diffuseColor.rgb;
	vec3 specularColor = 2.0 * texture2D(u_fragmentMap4, var_TexSpecular).rgb * u_specularColor.rgb;

#if defined(BLINN_PHONG)
	float specularFalloff = pow(NdotH, u_specularExponent);
#else
	vec3 R = -reflect(L, N);
	float RdotV = clamp(dot(R, V), 0.0, 1.0);
	float specularFalloff = pow(RdotV, u_specularExponent);
#endif

	vec3 color;
	color = diffuseColor;
	color += specularFalloff * specularColor;
	color *= NdotL * lightProjection;
	color *= lightFalloff;

	gl_FragColor = vec4(color, 1.0) * var_Color;
}
