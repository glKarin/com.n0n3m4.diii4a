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

/*
	macros:
		BLINN_PHONG: using blinn-phong instead phong.
		_PBR: using PBR.
*/
#version 100
//#pragma optimize(off)

precision highp float;

//#define BLINN_PHONG
//#define _PBR

//#define HALF_LAMBERT

varying vec2 var_TexDiffuse;
varying vec2 var_TexNormal;
varying vec2 var_TexSpecular;
varying vec4 var_TexLight;
varying lowp vec4 var_Color;
varying vec3 var_L;
#if defined(BLINN_PHONG) || defined(_PBR)
varying vec3 var_H;
#endif
#if !defined(BLINN_PHONG) || defined(_PBR)
varying vec3 var_V;
#endif
#ifdef _PBR
varying vec3 var_Normal;
#endif

uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
uniform float u_specularExponent;

uniform sampler2D u_fragmentMap0;    /* u_bumpTexture */
uniform sampler2D u_fragmentMap1;    /* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;    /* u_lightProjectionTexture */
uniform sampler2D u_fragmentMap3;    /* u_diffuseTexture */
uniform sampler2D u_fragmentMap4;    /* u_specularTexture */
uniform sampler2D u_fragmentMap5;    /* u_specularFalloffTexture */

#if defined(_PBR)
float dot2_4(vec2 a, vec4 b) {
    return dot(vec4(a, 0.0, 0.0), b);
}

vec2 CenterScale( vec2 inTC, vec2 centerScale ) {
    float scaleX = centerScale.x;
    float scaleY = centerScale.y;
    vec4 tc0 = vec4( scaleX, 0.0, 0.0, 0.5 - ( 0.5 * scaleX ) );
    vec4 tc1 = vec4( 0.0, scaleY, 0.0, 0.5 - ( 0.5 * scaleY ) );
    vec2 finalTC;
    finalTC.x = dot2_4( inTC, tc0 );
    finalTC.y = dot2_4( inTC, tc1 );
    return finalTC;
}

vec2 Rotate2D( vec2 inTC, vec2 cs ) {
    float sinValue = cs.y;
    float cosValue = cs.x;

    vec4 tc0 = vec4( cosValue, -sinValue, 0.0, ( -0.5 * cosValue ) + ( 0.5 * sinValue ) + 0.5 );
    vec4 tc1 = vec4( sinValue, cosValue, 0.0, ( -0.5 * sinValue ) + ( -0.5 * cosValue ) + 0.5 );
    vec2 finalTC;
    finalTC.x = dot2_4( inTC, tc0 );
    finalTC.y = dot2_4( inTC, tc1 );
    return finalTC;
}

// better noise function available at https://github.com/ashima/webgl-noise
float rand( vec2 co ) {
    return fract( sin( dot( co.xy, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float PI = 3.14159265359;
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
#endif

void main(void)
{
//float u_specularExponent = 4.0;

vec3 L = normalize(var_L);
#if defined(BLINN_PHONG) || defined(_PBR)
vec3 H = normalize(var_H);
#endif
#if defined(BLINN_PHONG)
    vec3 N = 2.0 * texture2D(u_fragmentMap0, var_TexNormal.st).agb - 1.0;
#endif
#if !defined(BLINN_PHONG) || defined(_PBR)
    vec3 V = normalize(var_V);
    vec3 N = normalize(2.0 * texture2D(u_fragmentMap0, var_TexNormal.st).agb - 1.0);
#endif

float NdotL = clamp(dot(N, L), 0.0, 1.0);
#if defined(HALF_LAMBERT)
NdotL *= 0.5;
NdotL += 0.5;
NdotL = NdotL * NdotL;
#endif
#if defined(BLINN_PHONG) || defined(_PBR)
float NdotH = clamp(dot(N, H), 0.0, 1.0);
#endif

    vec3 lightProjection = texture2DProj(u_fragmentMap2, var_TexLight.xyw).rgb;
    vec3 lightFalloff = texture2D(u_fragmentMap1, vec2(var_TexLight.z, 0.5)).rgb;
    vec3 diffuseColor = texture2D(u_fragmentMap3, var_TexDiffuse).rgb * u_diffuseColor.rgb;
#if defined(_PBR)
    vec3 AN = mix(normalize(var_Normal), N, u_specularExponent);
    vec4 Cd = vec4(diffuseColor.rgb, 1.0);
    vec4 specTex = texture2D(u_fragmentMap4, var_TexSpecular);
    vec4 roughness = vec4(specTex.r, specTex.r, specTex.r, specTex.r);
    vec4 metallic = vec4(specTex.g, specTex.g, specTex.g, specTex.g);

    vec4 Cl = vec4(lightProjection * lightFalloff, 1.0);
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, Cd.xyz, metallic.xyz);

    // cook-torrance brdf
    float NDF = DistributionGGX(AN, H, roughness.x);        
    float G   = GeometrySmith(AN, V, L, roughness.x);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

    // vec3 kS = F;
    // vec3 kD = vec3(1.0) - kS;
    // kD *= 1.0 - metallic.r;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(AN, V), 0.0) * max(dot(AN, L), 0.0);
    vec3 pbr     = numerator / max(denominator, 0.001);  

   gl_FragColor = var_Color * Cl * NdotL * Cd + (vec4(pbr.x, pbr.y, pbr.z, 0.0) * (u_specularColor /* *Cl */));
#else
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
#endif
}