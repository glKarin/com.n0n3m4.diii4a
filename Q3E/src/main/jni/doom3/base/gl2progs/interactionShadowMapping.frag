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
uniform highp samplerCube u_fragmentCubeMap6;	/* u_shadowMapTexture */
#else
uniform highp sampler2D u_fragmentMap6;	/* u_shadowMapTexture */
#endif

uniform highp vec4 globalLightOrigin;
uniform highp float u_uniformParm2; // sample size
uniform mediump float u_uniformParm3; // shadow alpha
uniform highp float u_uniformParm4; // bias
uniform highp float u_uniformParm5; // 1.0 / textureSize()
uniform highp float u_uniformParm6; // textureSize()

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
	return /*all(lessThan(colour, vec4(1.0, 1.0, 1.0, 1.0)))*/ colour.r < 1.0 ? dot(colour , bitShifts) : 1.0;
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
	#define SAMPLES 9

#ifdef _POINT_LIGHT
	vec3 sampleOffsetTable[SAMPLES];
	sampleOffsetTable[0] = vec3(0.0, 0.0, 0.0);
	sampleOffsetTable[1] = vec3(1.0, 1.0, 1.0); sampleOffsetTable[2] = vec3(1.0, 1.0, -1.0); sampleOffsetTable[3] = vec3(1.0, -1.0, -1.0); sampleOffsetTable[4] = vec3(1.0, -1.0, 1.0);
	sampleOffsetTable[5] = vec3(-1.0, 1.0, 1.0); sampleOffsetTable[6] = vec3(-1.0, -1.0, 1.0); sampleOffsetTable[7] = vec3(-1.0, 1.0, -1.0); sampleOffsetTable[8] = vec3(-1.0, -1.0, -1.0);
	highp vec3 toLightGlobal = normalize( -var_LightToVertex );
	int shadowIndex = 0;
	highp float axis[6];
	axis[0] = -toLightGlobal.x;
	axis[1] =  toLightGlobal.x;
	axis[2] = -toLightGlobal.y;
	axis[3] =  toLightGlobal.y;
	axis[4] = -toLightGlobal.z;
	axis[5] =  toLightGlobal.z;
	for( int i = 0; i < 6; i++ ) {
		//if( axis[i] > axis[shadowIndex] ) {		shadowIndex = i;	}
		shadowIndex = axis[i] > axis[shadowIndex] ? i : shadowIndex;
	}
	highp vec4 shadowPosition = var_VertexPosition * shadowMVPMatrix[shadowIndex];
	shadowPosition.xyz /= shadowPosition.w;
	highp float currentDepth = BIAS(shadowPosition.z);
	float distance = u_uniformParm2 + length(var_LightToVertex) * (0.0000002 * u_uniformParm6); // more far more large
	for (int i = 0; i < SAMPLES; ++i) {
		highp float shadowDepth = DC(textureCube(u_fragmentCubeMap6, normalize(var_LightToVertex + sampleOffsetTable[i] * distance)));
		highp float visibility = currentDepth - shadowDepth;
		shadow += 1.0 - step(0.0, visibility) * u_uniformParm3;
		//shadow += visibility < 0.0 ? 1.0 : u_uniformParm3;
	}
#else
	vec2 sampleOffsetTable[SAMPLES];
	sampleOffsetTable[0] = vec2( 0.0, 0.0);
	sampleOffsetTable[1] = vec2( 1.0, 1.0); sampleOffsetTable[2] = vec2( 1.0, -1.0); sampleOffsetTable[3] = vec2(-1.0, -1.0); sampleOffsetTable[4] = vec2(-1.0, 1.0);
	sampleOffsetTable[5] = vec2( 1.0, 0.0); sampleOffsetTable[6] = vec2( -1.0, 0.0); sampleOffsetTable[7] = vec2(0.0, -1.0); sampleOffsetTable[8] = vec2(0.0, 1.0);
	highp vec3 shadowCoord = var_ShadowCoord.xyz / var_ShadowCoord.w;
	highp float currentDepth = BIAS(shadowCoord.z);
	for (int i = 0; i < SAMPLES; ++i) {
		highp float shadowDepth = DC(texture2D(u_fragmentMap6, shadowCoord.st + sampleOffsetTable[i] * u_uniformParm2));
		highp float visibility = currentDepth - shadowDepth;
		shadow += 1.0 - step(0.0, visibility) * u_uniformParm3;
		//shadow += visibility < 0.0 ? 1.0 : u_uniformParm3;
	}
#endif
	const highp float sampleAvg = 1.0 / float(SAMPLES);
	shadow *= sampleAvg;

	vec3 color;
	color = diffuseColor;
	color += specularFalloff * specularColor;
	color *= NdotL * lightProjection;
	color *= lightFalloff;
	color *= shadow;

	gl_FragColor = vec4(color, 1.0) * var_Color;
}