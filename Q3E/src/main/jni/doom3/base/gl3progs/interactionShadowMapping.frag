/*
	macros:
		BLINN_PHONG: using blinn-phong instead phong.
		_POINT_LIGHT: light type is point light.
		_PARALLEL_LIGHT: light type is parallel light.
		_SPOT_LIGHT: light type is spot light.
*/
#version 300 es
//#pragma optimize(off)

precision highp float;

//#define BLINN_PHONG

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
uniform highp sampler2DArrayShadow u_fragmentMap6;	/* u_shadowMapTexture */
uniform sampler2D u_fragmentMap7;    /* u_jitterMapTexture */

uniform highp vec4 globalLightOrigin;
uniform mediump float u_uniformParm0; // shadow alpha
uniform highp vec4 u_uniformParm1; // (shadow map texture size, 1.0 / shadow map texture size, sampler factor, 0)
uniform highp vec4 u_uniformParm2; // (1.0 / screen width, 1.0 / screen height, jitter texture size, 1.0 / jitter texture size)
#define SHADOW_ALPHA u_uniformParm0
#define SHADOW_MAP_SIZE u_uniformParm1.x
#define SHADOW_MAP_SIZE_MULTIPLICATOR u_uniformParm1.y
#define JITTER_SCALE u_uniformParm1.z
#define SCREEN_SIZE_MULTIPLICATOR u_uniformParm2.xy
#ifdef _POINT_LIGHT
in highp vec4 var_VertexPosition;
uniform highp mat4 shadowMVPMatrix[6];
in highp vec3 var_VertexToLight;
#else
in highp vec4 var_ShadowCoord;
#endif

#ifdef _DYNAMIC_BIAS
#ifdef _PARALLEL_LIGHT
#define BIAS_SCALE 1.0 // 0.999991
#else
#define BIAS_SCALE 1.0 // 0.999
#endif
#define BIAS(x) ((x) * BIAS_SCALE)
#else
#define BIAS_OFFSET 0.0 // 0.001
#define BIAS(x) ((x) - BIAS_OFFSET)
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
    highp float shadow = 0.0;
#ifdef _POINT_LIGHT
    int shadowIndex = 0;
    highp vec3 toLightGlobal = normalize( var_VertexToLight );
    highp float axis[6] = float[6](
                              -toLightGlobal.x,
                              toLightGlobal.x,
                              -toLightGlobal.y,
                              toLightGlobal.y,
                              -toLightGlobal.z,
                              toLightGlobal.z
                          );
    for( int i = 0; i < 6; i++ )
    {
        //if( axis[i] > axis[shadowIndex] ) {		shadowIndex = i;	}
        shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
    }
    highp vec4 shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
    //vec3 c; if(shadowIndex == 0) c = vec3(1.0, 0.0, 0.0); else if(shadowIndex == 1) c = vec3(1.0, 1.0, 0.0); else if(shadowIndex == 2) c = vec3(0.0, 1.0, 0.0); else if(shadowIndex == 3) c = vec3(0.0, 1.0, 1.0); else if(shadowIndex == 4) c = vec3(0.0, 0.0, 1.0); else c = vec3(1.0, 0.0, 1.0);
    shadowPosition.xyz /= shadowPosition.w;
    shadowPosition.w = float(shadowIndex);
#else
    highp vec4 shadowPosition = vec4(var_ShadowCoord.xyz / var_ShadowCoord.w, 0.0);
#endif
    // end light type
    highp float distance = JITTER_SCALE * SHADOW_MAP_SIZE_MULTIPLICATOR;
    // float random = (gl_FragCoord.z + shadowPosition.z) * 0.5;
    float random = texture( u_fragmentMap7, gl_FragCoord.xy * SCREEN_SIZE_MULTIPLICATOR ).r;
    random *= 3.141592653589793;
    vec2 rot;
    rot.x = cos( random );
    rot.y = sin( random );
    shadowPosition.z = BIAS(shadowPosition.z);
    for (int i = 0; i < SAMPLES; ++i)
    {
        vec2 jitter = sampleOffsetTable[i];
        vec2 jitterRotated;
        jitterRotated.x = jitter.x * rot.x - jitter.y * rot.y;
        jitterRotated.y = jitter.x * rot.y + jitter.y * rot.x;
        highp float shadowDepth = texture(u_fragmentMap6, vec4(shadowPosition.st + jitterRotated * distance, shadowPosition.wz));
        shadow += 1.0 - (1.0 - shadowDepth) * SHADOW_ALPHA;
        //shadow += shadowDepth > 0.0 ? 1.0 : SHADOW_ALPHA;
    }
    const highp float sampleAvg = SAMPLE_MULTIPLICATOR;
    shadow *= sampleAvg;

    vec3 color;
    color = diffuseColor;
    color += specularFalloff * specularColor;
    color *= NdotL * lightProjection;
    color *= lightFalloff;
    color *= shadow;

    _gl_FragColor = vec4(color, 1.0) * var_Color;
}