#version 300 es
//#pragma optimize(off)

precision highp float;

//#define BLINN_PHONG
//#define _PBR
//#define _AMBIENT
//#define _TRANSLUCENT
//#define _SOFT

//#define HALF_LAMBERT

in vec2 var_TexDiffuse;
in vec2 var_TexNormal;
in vec2 var_TexSpecular;
in vec4 var_TexLight;
in lowp vec4 var_Color;
in vec3 var_L;
#if defined(BLINN_PHONG) || defined(_PBR)
in vec3 var_H;
#endif
#if (!defined(BLINN_PHONG) && !defined(_AMBIENT)) || defined(_PBR)
in vec3 var_V;
#endif
#if defined(_PBR)
in vec3 var_Normal;
#endif

uniform vec4 u_diffuseColor;
uniform vec4 u_specularColor;
#if defined(_PBR)
uniform vec2 u_specularExponent;
#else
uniform float u_specularExponent;
#endif

uniform sampler2D u_fragmentMap0;    /* u_bumpTexture */
uniform sampler2D u_fragmentMap1;    /* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;    /* u_lightProjectionTexture */
uniform sampler2D u_fragmentMap3;    /* u_diffuseTexture */
#if defined(_AMBIENT)
uniform samplerCube u_fragmentCubeMap4;
in mat3 var_TangentToWorldMatrix;
#else
uniform sampler2D u_fragmentMap4;    /* u_specularTexture */
uniform sampler2D u_fragmentMap5;    /* u_specularFalloffTexture */
#endif
#ifdef _SOFT
uniform mediump usampler2D u_fragmentMap6;    /* stencil shadow texture */
uniform highp vec4 u_nonPowerOfTwo;
uniform highp vec4 u_windowCoords;
uniform lowp float u_uniformParm1; // sampler bias
#endif
#if defined(_TRANSLUCENT) || defined(_SOFT)
uniform lowp float u_uniformParm0; // shadow alpha
#endif
out vec4 _gl_FragColor;

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
#if defined(BLINN_PHONG) || defined(_AMBIENT)
    vec3 N = 2.0 * texture(u_fragmentMap0, var_TexNormal.st).agb - 1.0;
#endif
#if (!defined(BLINN_PHONG) && !defined(_AMBIENT)) || defined(_PBR)
    vec3 V = normalize(var_V);
    vec3 N = normalize(2.0 * texture(u_fragmentMap0, var_TexNormal.st).agb - 1.0);
#endif

#if defined(_AMBIENT)
    vec3 globalNormal = var_TangentToWorldMatrix * N; // transform into ambient map space
    vec3 ambientValue = texture(u_fragmentCubeMap4, globalNormal).rgb; // load the ambient value
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

    vec3 lightProjection = textureProj(u_fragmentMap2, var_TexLight.xyw).rgb;
    vec3 lightFalloff = texture(u_fragmentMap1, vec2(var_TexLight.z, 0.5)).rgb;
    vec3 diffuseColor = texture(u_fragmentMap3, var_TexDiffuse).rgb * u_diffuseColor.rgb;
#if defined(_PBR)
    vec3 AN = normalize(mix(normalize(var_Normal), N, u_specularExponent.y));
    vec4 Cd = vec4(diffuseColor.rgb, 1.0);
    vec4 specTex = texture(u_fragmentMap4, var_TexSpecular);
    vec4 roughness = vec4(specTex.r, specTex.r, specTex.r, specTex.r);
    vec4 metallic = vec4(specTex.g, specTex.g, specTex.g, specTex.g);

    vec4 Cl = vec4(lightProjection * lightFalloff, 1.0);
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, Cd.xyz, metallic.xyz);

    // cook-torrance brdf
    float NDF = DistributionGGX(AN, H, roughness.x);        
    float G   = GeometrySmith(AN, V, L, roughness.x);      
    vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0); //max       

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic.r;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(AN, V), 0.0) * max(dot(AN, L), 0.0);
    vec3 pbr     = numerator / max(denominator, 0.001);  

#if 1
    vec4 color = (Cd/* * vec4(kD, 1.0)*/ + (u_specularExponent.x * vec4(pbr.rgb, 0.0) * u_specularColor)) * NdotL * Cl;
    color = vec4(color.rgb, 1.0) * var_Color;
#else
    vec4 color = var_Color * Cl * NdotL * Cd + (u_specularExponent.x * vec4(pbr.rgb, 0.0) * (u_specularColor/* * Cl*/));
#endif
#elif defined(_AMBIENT)
    vec3 color = u_specularExponent * /*ambientValue * */diffuseColor * lightProjection * lightFalloff;
#else
    vec3 specularColor = 2.0 * texture(u_fragmentMap4, var_TexSpecular).rgb * u_specularColor.rgb;

#if defined(BLINN_PHONG)
    float specularFalloff = pow(NdotH, u_specularExponent);
#else
    vec3 R = -reflect(L, N);
    float RdotV = clamp(dot(R, V), 0.0, 1.0);
    float specularFalloff = pow(RdotV, u_specularExponent);
#endif
#endif

#ifdef _SOFT
#define SAMPLES 12
#define SAMPLE_MULTIPLICATOR (1.0 / 12.0)
vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](
    vec2( 0.6111618, 0.1050905 ), 
    vec2( 0.1088336, 0.1127091 ), 
    vec2( 0.3030421, -0.6292974 ), 
    vec2( 0.4090526, 0.6716492 ), 
    vec2( -0.1608387, -0.3867823 ), 
    vec2( 0.7685862, -0.6118501 ), 
    vec2( -0.1935026, -0.856501 ), 
    vec2( -0.4028573, 0.07754025 ), 
    vec2( -0.6411021, -0.4748057 ), 
    vec2( -0.1314865, 0.8404058 ), 
    vec2( -0.7005203, 0.4596822 ), 
    vec2( -0.9713828, -0.06329931 ) 
    // , vec2( 0.0, 0.0 )
);

    vec2 screenTexCoord = gl_FragCoord.xy * u_windowCoords.xy;
    screenTexCoord = screenTexCoord * u_nonPowerOfTwo.xy;
    vec2 factor = u_nonPowerOfTwo.zw * u_uniformParm1;
    float shadow = 0.0;
    for (int i = 0; i < SAMPLES; ++i) {
        vec2 stc = screenTexCoord + sampleOffsetTable[i] * factor;
        float t = float(texture(u_fragmentMap6, stc).r);
        float shadowColor = clamp(129.0 - t, u_uniformParm0, 1.0);
        shadow += shadowColor;
    }
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR;
    shadow *= sampleAvg;
#endif // _SOFT

#if defined(_PBR)
#ifdef _SOFT
    _gl_FragColor = vec4(color.rgb * shadow, color.a);
#elif defined(_TRANSLUCENT)
    _gl_FragColor = color * u_uniformParm0;
#else
    _gl_FragColor = color;
#endif
#elif defined(_AMBIENT)
#ifdef _SOFT
    color *= shadow;
#endif // _SOFT
    _gl_FragColor = vec4(color, 1.0) * var_Color
#ifdef _TRANSLUCENT
         * u_uniformParm0
#endif // _TRANSLUCENT
        ;
#else
    vec3 color;
    color = diffuseColor;
    color += specularFalloff * specularColor;
    color *= NdotL * lightProjection;
    color *= lightFalloff;
#ifdef _SOFT
    color *= shadow;
#endif // _SOFT

    _gl_FragColor = vec4(color, 1.0) * var_Color
#ifdef _TRANSLUCENT
         * u_uniformParm0
#endif // _TRANSLUCENT
        ;
#endif
}
