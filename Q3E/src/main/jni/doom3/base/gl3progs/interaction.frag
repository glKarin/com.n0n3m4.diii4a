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
		_STENCIL_SHADOW_TRANSLUCENT: for translucent stencil shadow
		_STENCIL_SHADOW_SOFT: soft stencil shadow
*/
#version 300 es
//#pragma optimize(off)

precision highp float;

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

in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;
in lowp vec4 var_Color;
in vec3 var_L;
#if defined(BLINN_PHONG)
in vec3 var_H;
#else
in vec3 var_V;
#endif

uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
uniform float u_specularExponent;

uniform sampler2D u_fragmentMap0;	/* u_bumpTexture */
uniform sampler2D u_fragmentMap1;	/* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;	/* u_lightProjectionTexture */
uniform sampler2D u_fragmentMap3;	/* u_diffuseTexture */
uniform sampler2D u_fragmentMap4;	/* u_specularTexture */
uniform sampler2D u_fragmentMap5;	/* u_specularFalloffTexture */
#ifdef _STENCIL_SHADOW_TRANSLUCENT
uniform mediump float u_uniformParm0; // shadow alpha
#endif
#ifdef _STENCIL_SHADOW_SOFT
uniform mediump usampler2D u_fragmentMap6;	/* stencil shadow texture */
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;
uniform lowp float u_uniformParm0; // shadow alpha
uniform lowp float u_uniformParm1; // sampler bias
#endif

out vec4 _gl_FragColor;

void main(void)
{
    //float u_specularExponent = 4.0;

    vec3 L = normalize(var_L);
#if defined(BLINN_PHONG)
    vec3 H = normalize(var_H);
    vec3 N = 2.0 * texture(u_fragmentMap0, var_TexNormal.st).agb - 1.0;
#else
    vec3 V = normalize(var_V);
    vec3 N = normalize(2.0 * texture(u_fragmentMap0, var_TexNormal.st).agb - 1.0);
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

    vec3 lightProjection = textureProj(u_fragmentMap2, var_TexLight.xyw).rgb;
    vec3 lightFalloff = texture(u_fragmentMap1, vec2(var_TexLight.z, 0.5)).rgb;
    vec3 diffuseColor = texture(u_fragmentMap3, var_TexDiffuse).rgb * u_diffuseColor.rgb;
    vec3 specularColor = 2.0 * texture(u_fragmentMap4, var_TexSpecular).rgb * u_specularColor.rgb;

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

#ifdef _STENCIL_SHADOW_TRANSLUCENT
    _gl_FragColor = vec4(color, 1.0) * var_Color * u_uniformParm0;
#elif defined(_STENCIL_SHADOW_SOFT)
#define SAMPLES 17
    vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](
                                          vec2( -0.94201624, -0.39906216 ),
                                          vec2( 0.94558609, -0.76890725 ),
                                          vec2( -0.094184101, -0.92938870 ),
                                          vec2( 0.34495938, 0.29387760 ),
                                          vec2( -0.91588581, 0.45771432 ),
                                          vec2( -0.81544232, -0.87912464 ),
                                          vec2( -0.38277543, 0.27676845 ),
                                          vec2( 0.97484398, 0.75648379 ),
                                          vec2( 0.44323325, -0.97511554 ),
                                          vec2( 0.53742981, -0.47373420 ),
                                          vec2( -0.26496911, -0.41893023 ),
                                          vec2( 0.79197514, 0.19090188 ),
                                          vec2( -0.24188840, 0.99706507 ),
                                          vec2( -0.81409955, 0.91437590 ),
                                          vec2( 0.19984126, 0.78641367 ),
                                          vec2( 0.14383161, -0.14100790 ),
                                          vec2( 0.0, 0.0 )
                                      );

    float shadow = 0.0;
    for (int i = 0; i < SAMPLES; ++i)
    {
        vec2 screenTexCoord = gl_FragCoord.xy * u_windowCoords.xy;
        screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;
        /*
          vec2 texSize = vec2(textureSize(u_fragmentMap6, 0));
          vec2 pixSize = vec2(1.0, 1.0) / texSize;
          vec2 baseTexCoord = gl_FragCoord.xy * pixSize;
          screenTexCoord = baseTexCoord;
        */
        screenTexCoord += sampleOffsetTable[i] * u_nonPowerOfTwo.zw * u_uniformParm1;
        float t = float(texture(u_fragmentMap6, screenTexCoord).r);
        float f= clamp(129.0 - t, u_uniformParm0, 1.0);
        shadow += f;
    }
    const highp float sampleAvg = 1.0 / float(SAMPLES);
    shadow *= sampleAvg;
    color *= shadow;
    _gl_FragColor = vec4(color, 1.0) * var_Color;
#else
    _gl_FragColor = vec4(color, 1.0) * var_Color;
#endif
}
