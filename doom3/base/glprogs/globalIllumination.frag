#version 100
//#pragma optimize(off)

//#define _BFG
//#define BLINN_PHONG
//#define HALF_LAMBERT

precision highp float;

varying vec2 var_TexDiffuse;
varying vec2 var_TexNormal;
varying vec2 var_TexSpecular;
varying lowp vec4 var_Color;
varying vec3 var_L;
varying vec3 var_H;
#if !defined(_BFG) && !defined(BLINN_PHONG)
varying vec3 var_V;
#endif

uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
uniform float u_specularExponent;

uniform sampler2D u_fragmentMap0;    /* u_bumpTexture */
uniform sampler2D u_fragmentMap1;    /* u_diffuseTexture */
uniform sampler2D u_fragmentMap2;    /* u_specularTexture */

void main(void)
{
#ifdef _BFG
    vec4 bumpMap = texture2D(u_fragmentMap0, var_TexNormal.st);
    vec3 diffuseMap = texture2D(u_fragmentMap1, var_TexDiffuse.st).xyz;
    vec3 specMap = texture2D(u_fragmentMap2, var_TexSpecular.st).xyz;

    float specularPower = 10.0;
    vec3 localNormal;
    localNormal.xy = bumpMap.wy - 0.5;
    localNormal.z = sqrt(abs(dot(localNormal.xy, localNormal.xy) - 0.25));
    localNormal = normalize(localNormal);
    float hDotN = dot(normalize(var_H), localNormal);
    float specularContribution = pow (abs(hDotN), specularPower);

    vec3 diffuseColor = diffuseMap * (u_diffuseColor.rgb/* * 0.5*/);
    vec3 specularColor = specMap * specularContribution * u_specularColor.rgb;

    vec3 lightVector = normalize(var_L);
    float halfLdotN = dot(localNormal, lightVector) * 0.5 + 0.5;
    halfLdotN *= halfLdotN;
    float lightColor = u_specularExponent;
    vec3 color = (diffuseColor + specularColor) * halfLdotN * lightColor;
    gl_FragColor = vec4(color, 1.0) * var_Color;

#else

    vec3 L = normalize(var_L);
    vec3 H = normalize(var_H);
    vec3 N = 2.0 * texture2D(u_fragmentMap0, var_TexNormal.st).agb - 1.0;
    float NdotL = clamp(dot(N, L), 0.0, 1.0);

#ifdef HALF_LAMBERT
    NdotL *= 0.5;
    NdotL += 0.5;
    NdotL = NdotL * NdotL;
#endif
    vec3 diffuseColor = texture2D(u_fragmentMap1, var_TexDiffuse).rgb * u_diffuseColor.rgb;
    vec3 specularColor = 2.0 * texture2D(u_fragmentMap2, var_TexSpecular).rgb * u_specularColor.rgb;

#ifdef BLINN_PHONG
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float specularFalloff = pow(NdotH, 12.0);
#else
    vec3 V = normalize(var_V);
    N = normalize(N);
    vec3 R = -reflect(L, N);
    float RdotV = clamp(dot(R, V), 0.0, 1.0);
    float specularFalloff = pow(RdotV, 3.0);
#endif

    vec3 color;
    color = diffuseColor;
    color += specularFalloff * specularColor;
    color *= NdotL * u_specularExponent;

    gl_FragColor = vec4(color, 1.0) * var_Color;
#endif
}
