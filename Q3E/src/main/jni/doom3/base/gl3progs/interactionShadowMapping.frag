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

uniform highp vec4 globalLightOrigin;
uniform highp float u_uniformParm2; // sample size
uniform mediump float u_uniformParm3; // shadow alpha
uniform highp float u_uniformParm4; // shadow bias
uniform highp float u_uniformParm5; // 1.0 / textureSize()
uniform highp float u_uniformParm6; // textureSize()
uniform highp sampler2DArrayShadow u_fragmentMap6;	/* u_shadowMapTexture */
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

float shadow = 0.0;
#define SAMPLES 9

vec2 sampleOffsetTable[SAMPLES] = vec2[SAMPLES](
	vec2(0.0, 0.0),
	vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(-1.0, -1.0), vec2(-1.0, 1.0),
	vec2(1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0)
);
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
	for( int i = 0; i < 6; i++ ) {
		//if( axis[i] > axis[shadowIndex] ) {		shadowIndex = i;	}
		shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
	}
	highp vec4 shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
	//vec3 c; if(shadowIndex == 0) c = vec3(1.0, 0.0, 0.0); else if(shadowIndex == 1) c = vec3(1.0, 1.0, 0.0); else if(shadowIndex == 2) c = vec3(0.0, 1.0, 0.0); else if(shadowIndex == 3) c = vec3(0.0, 1.0, 1.0); else if(shadowIndex == 4) c = vec3(0.0, 0.0, 1.0); else c = vec3(1.0, 0.0, 1.0);
	shadowPosition.xyz /= shadowPosition.w;
	shadowPosition.w = float(shadowIndex);
	highp float distance = u_uniformParm2 + length(var_VertexToLight) * (0.00000000001 * u_uniformParm6); // more far more large
#else
	highp vec4 shadowPosition = vec4(var_ShadowCoord.xyz / var_ShadowCoord.w, 0.0);
	highp float distance = u_uniformParm2;
#endif
   // end light type
	shadowPosition.z = BIAS(shadowPosition.z);
	for (int i = 0; i < SAMPLES; ++i) {
		highp float shadowDepth = texture(u_fragmentMap6, vec4(shadowPosition.st + sampleOffsetTable[i] * distance, shadowPosition.wz));
		shadow += 1.0 - (1.0 - shadowDepth) * u_uniformParm3;
		//shadow += shadowDepth > 0.0 ? 1.0 : u_uniformParm3;
	}
	const highp float sampleAvg = 1.0 / float(SAMPLES);
	shadow *= sampleAvg;

	vec3 color;
	color = diffuseColor;
	color += specularFalloff * specularColor;
	color *= NdotL * lightProjection;
	color *= lightFalloff;
	color *= shadow;

	_gl_FragColor = vec4(color, 1.0) * var_Color;
}