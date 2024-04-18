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

out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_TexLight;
out lowp vec4 var_Color;
out vec3 var_L;
#if defined(BLINN_PHONG)
out vec3 var_H;
#else
out vec3 var_V;
#endif

in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;

uniform vec4 u_lightProjectionS;
uniform vec4 u_lightProjectionT;
uniform vec4 u_lightFalloff;
uniform vec4 u_lightProjectionQ;
uniform lowp vec4 u_colorModulate;
uniform lowp vec4 u_colorAdd;
uniform lowp vec4 u_glColor;

uniform vec4 u_lightOrigin;
uniform vec4 u_viewOrigin;

uniform vec4 u_bumpMatrixS;
uniform vec4 u_bumpMatrixT;
uniform vec4 u_diffuseMatrixS;
uniform vec4 u_diffuseMatrixT;
uniform vec4 u_specularMatrixS;
uniform vec4 u_specularMatrixT;

uniform highp mat4 u_modelViewProjectionMatrix;

uniform highp mat4 u_modelMatrix;
uniform highp vec4 globalLightOrigin;

#ifdef _POINT_LIGHT
   out highp vec4 var_VertexPosition;
   out highp vec3 var_VertexToLight;
#else
   uniform highp mat4 shadowMVPMatrix;
   out highp vec4 var_ShadowCoord;
#endif

void main(void)
{
	mat3 M = mat3(attr_Tangent, attr_Bitangent, attr_Normal);

	var_TexNormal.x = dot(u_bumpMatrixS, attr_TexCoord);
	var_TexNormal.y = dot(u_bumpMatrixT, attr_TexCoord);

	var_TexDiffuse.x = dot(u_diffuseMatrixS, attr_TexCoord);
	var_TexDiffuse.y = dot(u_diffuseMatrixT, attr_TexCoord);

	var_TexSpecular.x = dot(u_specularMatrixS, attr_TexCoord);
	var_TexSpecular.y = dot(u_specularMatrixT, attr_TexCoord);

	var_TexLight.x = dot(u_lightProjectionS, attr_Vertex);
	var_TexLight.y = dot(u_lightProjectionT, attr_Vertex);
	var_TexLight.z = dot(u_lightFalloff, attr_Vertex);
	var_TexLight.w = dot(u_lightProjectionQ, attr_Vertex);

	vec3 L = u_lightOrigin.xyz - attr_Vertex.xyz;
	vec3 V = u_viewOrigin.xyz - attr_Vertex.xyz;
#if defined(BLINN_PHONG)
	vec3 H = normalize(L) + normalize(V);
#endif

	var_L = L * M;
#if defined(BLINN_PHONG)
	var_H = H * M;
#else
	var_V = V * M;
#endif

	var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;

#ifdef _POINT_LIGHT
	highp vec4 posInLight = u_modelMatrix * attr_Vertex;
	var_VertexToLight = globalLightOrigin.xyz - posInLight.xyz;
	var_VertexPosition = attr_Vertex;
#else
	var_ShadowCoord = attr_Vertex * shadowMVPMatrix;
#endif
	gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}