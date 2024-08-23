/*
	macros:
		BLINN_PHONG: using blinn-phong instead phong.
		_PBR: using PBR.
		_PACK_FLOAT: pack float when using RGBA texture.
		_POINT_LIGHT: light type is point light.
		_PARALLEL_LIGHT: light type is parallel light.
		_SPOT_LIGHT: light type is spot light.
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
#ifdef _PBR
uniform vec2 u_specularExponent;
#else
uniform float u_specularExponent;
#endif

uniform sampler2D u_fragmentMap0;    /* u_bumpTexture */
uniform sampler2D u_fragmentMap1;    /* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;    /* u_lightProjectionTexture */
uniform sampler2D u_fragmentMap3;    /* u_diffuseTexture */
uniform sampler2D u_fragmentMap4;    /* u_specularTexture */
uniform sampler2D u_fragmentMap5;    /* u_specularFalloffTexture */
#ifdef _POINT_LIGHT
uniform highp samplerCube u_fragmentCubeMap6;    /* u_shadowMapCubeTexture */
#else
uniform highp sampler2D u_fragmentMap6;    /* u_shadowMapTexture */
#endif
uniform sampler2D u_fragmentMap7;    /* u_jitterMapTexture */

uniform highp vec4 globalLightOrigin;
uniform mediump float u_uniformParm0; // shadow alpha
uniform highp vec4 u_uniformParm1; // (shadow map texture size, 1.0 / shadow map texture size, sampler factor, 0)
uniform highp vec4 u_uniformParm2; // (1.0 / screen width, 1.0 / screen height, jitter texture size, 1.0 / jitter texture size)
uniform highp float u_uniformParm3; // shadow bias for test
// uniform highp float u_uniformParm4; // sample size
#define SHADOW_ALPHA u_uniformParm0
#define SHADOW_MAP_SIZE u_uniformParm1.x
#define SHADOW_MAP_SIZE_MULTIPLICATOR u_uniformParm1.y
#define JITTER_SCALE u_uniformParm1.z
#define SCREEN_SIZE_MULTIPLICATOR u_uniformParm2.xy
#ifdef _POINT_LIGHT
    varying highp vec3 var_LightToVertex;
    varying highp vec4 var_VertexPosition;
    uniform highp mat4 shadowMVPMatrix[6];
bool plane_ray_intersect(highp vec3 plane_point, highp vec3 plane_normal, highp vec3 dir, out highp vec3 ret) {
	highp float dotProduct = dot(dir, plane_normal);
	highp float dotProductFactor = 1.0 / dotProduct;
	// if(dotProduct == 0.0) return false;
	highp float l2 = dot(plane_normal, plane_point) * dotProductFactor;
	// if (l2 < 0.0) return false;
	highp float distance = dot(plane_normal, plane_point);
	highp float t = - (dot(plane_normal, dir) - distance) * dotProductFactor;
	ret = dir * t;
	return true;
}
#else
    varying highp vec4 var_ShadowCoord;
#endif

#if !defined(_USING_DEPTH_TEXTURE) && defined(_PACK_FLOAT)
highp float unpack (vec4 colour)
{
    const highp vec4 bitShifts = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);
    highp float dotc = dot(colour , bitShifts);
    return /*all(lessThan(colour, vec4(1.0, 1.0, 1.0, 1.0)))*/ colour.r < 1.0 ? dotc : 1.0;
}
#define DC(x) (unpack(x))
#else
#define DC(x) ((x).r)
#endif

#ifdef _DYNAMIC_BIAS
   #ifdef _PARALLEL_LIGHT
       #define BIAS_SCALE 0.999991
   #else
       #define BIAS_SCALE 0.999
   #endif
   #define BIAS(x) ((x) * BIAS_SCALE)
#elif defined(_FIXED_BIAS)
    #define BIAS_OFFSET 0.001
   #define BIAS(x) ((x) - BIAS_OFFSET)
#else
   #define BIAS(x) (x)
#endif

#ifdef _PBR
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
    vec3 AN = normalize(mix(normalize(var_Normal), N, u_specularExponent.y));
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
#else
    vec3 specularColor = 2.0 * texture2D(u_fragmentMap4, var_TexSpecular).rgb * u_specularColor.rgb;

#if defined(BLINN_PHONG)
    float specularFalloff = pow(NdotH, u_specularExponent);
#else
    vec3 R = -reflect(L, N);
    float RdotV = clamp(dot(R, V), 0.0, 1.0);
    float specularFalloff = pow(RdotV, u_specularExponent);
#endif
#endif

    highp float shadow = 0.0;

#ifdef _POINT_LIGHT
    #define SAMPLES 20
    #define SAMPLE_MULTIPLICATOR (1.0 / 20.0)
    vec3 sampleOffsetTable[SAMPLES];
    sampleOffsetTable[0] = vec3(1.0, 1.0, 1.0); sampleOffsetTable[1] = vec3(1.0, -1.0, 1.0); sampleOffsetTable[2] = vec3(-1.0, -1.0, 1.0); sampleOffsetTable[3] = vec3(-1.0, 1.0, 1.0);
    sampleOffsetTable[4] = vec3(1.0, 1.0, -1.0); sampleOffsetTable[5] = vec3(1.0, -1.0, -1.0); sampleOffsetTable[6] = vec3(-1.0, -1.0, -1.0); sampleOffsetTable[7] = vec3(-1.0, 1.0, -1.0);
    sampleOffsetTable[8] = vec3(1.0, 1.0, 0.0); sampleOffsetTable[9] = vec3(1.0, -1.0, 0.0); sampleOffsetTable[10] = vec3(-1.0, -1.0, 0.0); sampleOffsetTable[11] = vec3(-1.0, 1.0, 0.0);
    sampleOffsetTable[12] = vec3(1.0, 0.0, 1.0); sampleOffsetTable[13] = vec3(-1.0, 0.0, 1.0); sampleOffsetTable[14] = vec3(1.0, 0.0, -1.0); sampleOffsetTable[15] = vec3(-1.0, 0.0, -1.0);
    sampleOffsetTable[16] = vec3(0.0, 1.0, 1.0); sampleOffsetTable[17] = vec3(0.0, -1.0, 1.0); sampleOffsetTable[18] = vec3(0.0, -1.0, -1.0); sampleOffsetTable[19] = vec3(0.0, 1.0, -1.0);
    // sampleOffsetTable[20] = vec3(0.0, 0.0, 0.0);

    highp vec3 toLightGlobal = normalize( -var_LightToVertex );
    int shadowIndex = 0;    highp float axis[6];
    axis[0] = -toLightGlobal.x;
    axis[1] =  toLightGlobal.x;
    axis[2] = -toLightGlobal.y;
    axis[3] =  toLightGlobal.y;
    axis[4] = -toLightGlobal.z;
    axis[5] =  toLightGlobal.z;
    for( int i = 0; i < 6; i++ ) {
        //if( axis[i] > axis[shadowIndex] ) {        shadowIndex = i;    }
        shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
    }
    highp vec3 plane_normal;
    highp vec3 plane_point;
    highp float radius = SHADOW_MAP_SIZE * 0.5;
    if(shadowIndex == 0) { // +X 
        plane_point = vec3(radius, 0.0, 0.0);
        plane_normal = vec3(-1.0, 0.0, 0.0);
    } else if(shadowIndex == 1) { // -X 
        plane_point = vec3(-radius, 0.0, 0.0);
        plane_normal = vec3(1.0, 0.0, 0.0);
    } else if(shadowIndex == 2) { // +Y 
        plane_point = vec3(0.0, radius, 0.0);
        plane_normal = vec3(0.0, -1.0, 0.0);
    } else if(shadowIndex == 3) { // -Y 
        plane_point = vec3(0.0, -radius, 0.0);
        plane_normal = vec3(0.0, 1.0, 0.0);
    } else if(shadowIndex == 4) { // +Z 
        plane_point = vec3(0.0, 0.0, radius);
        plane_normal = vec3(0.0, 0.0, -1.0);
    } else { // -Z 
        plane_point = vec3(0.0, 0.0, -radius);
        plane_normal = vec3(0.0, 0.0, 1.0);
    }
    highp vec3 texcoordCube;
    plane_ray_intersect(plane_point, plane_normal, normalize(var_LightToVertex), texcoordCube);
    highp vec4 shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
    shadowPosition.xyz /= shadowPosition.w;
    highp float currentDepth = BIAS(shadowPosition.z);
    highp float distance = JITTER_SCALE * 0.5;
    for (int i = 0; i < SAMPLES; ++i) {
        highp vec3 jitter = sampleOffsetTable[i];
        highp float shadowDepth = DC(textureCube(u_fragmentCubeMap6, normalize(texcoordCube + jitter * distance)));
        highp float visibility = currentDepth - shadowDepth;
        shadow += 1.0 - step(0.0, visibility) * SHADOW_ALPHA;
        //shadow += visibility < 0.0 ? 1.0 : SHADOW_ALPHA;
    }
#else // end point light
    #define SAMPLES 12
    #define SAMPLE_MULTIPLICATOR (1.0 / 12.0)
    vec2 sampleOffsetTable[SAMPLES];
      sampleOffsetTable[0] = vec2( 0.6111618, 0.1050905 );
      sampleOffsetTable[1] = vec2( 0.1088336, 0.1127091 );
      sampleOffsetTable[2] = vec2( 0.3030421, -0.6292974 );
      sampleOffsetTable[3] = vec2( 0.4090526, 0.6716492 );
      sampleOffsetTable[4] = vec2( -0.1608387, -0.3867823 );
      sampleOffsetTable[5] = vec2( 0.7685862, -0.6118501 );
      sampleOffsetTable[6] = vec2( -0.1935026, -0.856501 );
      sampleOffsetTable[7] = vec2( -0.4028573, 0.07754025 );
      sampleOffsetTable[8] = vec2( -0.6411021, -0.4748057 );
      sampleOffsetTable[9] = vec2( -0.1314865, 0.8404058 );
      sampleOffsetTable[10] = vec2( -0.7005203, 0.4596822 );
      sampleOffsetTable[11] = vec2( -0.9713828, -0.06329931 );
// sampleOffsetTable[12] = vec2( 0.0, 0.0 );

    // highp float random = (gl_FragCoord.z + shadowPosition.z) * 0.5;
    highp float random = texture2D( u_fragmentMap7, gl_FragCoord.xy * SCREEN_SIZE_MULTIPLICATOR ).r;
    random *= 3.141592653589793;
    highp vec2 rot;
    rot.x = cos( random );
    rot.y = sin( random );
    highp float distance = JITTER_SCALE * SHADOW_MAP_SIZE_MULTIPLICATOR;
    highp vec3 shadowCoord = var_ShadowCoord.xyz / var_ShadowCoord.w;
    highp float currentDepth = BIAS(shadowCoord.z);
    for (int i = 0; i < SAMPLES; ++i) {
        highp vec2 jitter = sampleOffsetTable[i];
        highp vec2 jitterRotated;
        jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
        jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;
        highp float shadowDepth = DC(texture2D(u_fragmentMap6, shadowCoord.st + jitterRotated * distance));
        highp float visibility = currentDepth - shadowDepth;
        shadow += 1.0 - step(0.0, visibility) * SHADOW_ALPHA;
        //shadow += visibility < 0.0 ? 1.0 : SHADOW_ALPHA;
    }
#endif
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR;
    shadow *= sampleAvg;

#if defined(_PBR)
    gl_FragColor = vec4(color.rgb * shadow, color.a);
#else
    vec3 color;
    color = diffuseColor;
    color += specularFalloff * specularColor;
    color *= NdotL * lightProjection;
    color *= lightFalloff;
    color *= shadow;

    gl_FragColor = vec4(color, 1.0) * var_Color;
#endif
}
