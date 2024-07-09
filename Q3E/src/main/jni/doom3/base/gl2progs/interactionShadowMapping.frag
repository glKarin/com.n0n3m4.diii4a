/*
	macros:
		BLINN_PHONG: using blinn-phong instead phong.
		_PACK_FLOAT: pack float when using RGBA texture.
		_POINT_LIGHT: light type is point light.
		_PARALLEL_LIGHT: light type is parallel light.
		_SPOT_LIGHT: light type is spot light.
*/
#version 100
//#pragma optimize(off)

precision highp float;

//#define BLINN_PHONG

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
uniform float u_specularExponent;

uniform sampler2D u_fragmentMap0;	/* u_bumpTexture */
uniform sampler2D u_fragmentMap1;	/* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;	/* u_lightProjectionTexture */
uniform sampler2D u_fragmentMap3;	/* u_diffuseTexture */
uniform sampler2D u_fragmentMap4;	/* u_specularTexture */
uniform sampler2D u_fragmentMap5;	/* u_specularFalloffTexture */
#ifdef _POINT_LIGHT
uniform highp samplerCube u_fragmentCubeMap6;	/* u_shadowMapCubeTexture */
#else
uniform highp sampler2D u_fragmentMap6;	/* u_shadowMapTexture */
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
#else
varying highp vec4 var_ShadowCoord;
#endif

#if !defined(_USING_DEPTH_TEXTURE) && defined(_PACK_FLOAT)
highp float unpack (vec4 colour)
{
    const highp vec4 bitShifts = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);
    highp float dotc = dot(colour, bitShifts);
    return /*all(lessThan(colour, vec4(1.0, 1.0, 1.0, 1.0)))*/ colour.r < 1.0 ? dotc : 1.0;
}
#define DC(x) (unpack(x))
#else
#define DC(x) ((x).r)
#endif

#ifdef _DYNAMIC_BIAS
#define BIAS_SCALE 1.0 // 0.999
#define BIAS(x) ((x) * BIAS_SCALE)
#else
#define BIAS_OFFSET 0.0 // 0.001
#define BIAS(x) ((x) - BIAS_OFFSET)
#endif

void main(void)
{
    //float u_specularExponent = 4.0;

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
    // sampleOffsetTable[20] = vec3(0.0, 0.0, 0.0);\n
    highp vec3 toLightGlobal = normalize( -var_LightToVertex );
    int shadowIndex = 0;
    highp float axis[6];
    axis[0] = -toLightGlobal.x;
    axis[1] =  toLightGlobal.x;
    axis[2] = -toLightGlobal.y;
    axis[3] =  toLightGlobal.y;
    axis[4] = -toLightGlobal.z;
    axis[5] =  toLightGlobal.z;
    for( int i = 0; i < 6; i++ )
    {
        //if( axis[i] > axis[shadowIndex] ) {		shadowIndex = i;	}
        shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
    }
    highp vec4 shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
    shadowPosition.xyz /= shadowPosition.w;
    highp float currentDepth = BIAS(shadowPosition.z);
    highp float distance = JITTER_SCALE * SHADOW_MAP_SIZE_MULTIPLICATOR;
    highp vec3 texcoordCube = normalize(var_LightToVertex);
    for (int i = 0; i < SAMPLES; ++i)
    {
        vec3 jitter = sampleOffsetTable[i];
        vec3 jitterRotated = jitter;
        jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
        jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;
        jitterRotated.z = jitter.y * rot.y - jitter.x * rot.x;
        highp float shadowDepth = DC(textureCube(u_fragmentCubeMap6, normalize(var_LightToVertex + jitterRotated * distance)));
        highp float visibility = currentDepth - shadowDepth;
        shadow += 1.0 - step(0.0, visibility) * SHADOW_ALPHA;
        //shadow += visibility < 0.0 ? 1.0 : SHADOW_ALPHA;
    }
#else
#define SAMPLES 16
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
    highp float distance = JITTER_SCALE * SHADOW_MAP_SIZE_MULTIPLICATOR;
    highp vec3 shadowCoord = var_ShadowCoord.xyz / var_ShadowCoord.w;
    highp float currentDepth = BIAS(shadowCoord.z);
    for (int i = 0; i < SAMPLES; ++i)
    {
        vec2 jitter = sampleOffsetTable[i];
        vec2 jitterRotated;
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

    vec3 color;
    color = diffuseColor;
    color += specularFalloff * specularColor;
    color *= NdotL * lightProjection;
    color *= lightFalloff;
    color *= shadow;

    gl_FragColor = vec4(color, 1.0) * var_Color;
}