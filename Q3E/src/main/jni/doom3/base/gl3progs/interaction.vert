#version 300 es
//#pragma optimize(off)

precision highp float;

//#define BLINN_PHONG
//#define _PBR
//#define _AMBIENT

out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_TexLight;
out lowp vec4 var_Color;
out vec3 var_L;
#if defined(BLINN_PHONG) || defined(_PBR) || defined(_AMBIENT)
out vec3 var_H;
#endif
#if !defined(BLINN_PHONG) || defined(_PBR)
out vec3 var_V;
#endif
#if defined(_PBR)
out vec3 var_Normal;
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
uniform lowp float u_colorModulate; // 0 or 1/255
uniform lowp float u_colorAdd; // 0 or 1
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
#if defined(_AMBIENT)
uniform highp mat4 u_modelMatrix;
out mat3 var_TangentToWorldMatrix;
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

#if defined(_AMBIENT)
    vec3 L = normalize(vec3(0.0, 0.5, 1.0));
    var_TangentToWorldMatrix = mat3(u_modelMatrix) * M;
#else
    vec3 L = u_lightOrigin.xyz - attr_Vertex.xyz;
#endif
    vec3 V = u_viewOrigin.xyz - attr_Vertex.xyz;
#if defined(BLINN_PHONG) || defined(_PBR) || defined(_AMBIENT)
    vec3 H = normalize(L) + normalize(V);
#endif

    var_L = L * M;
#if defined(BLINN_PHONG) || defined(_PBR) || defined(_AMBIENT)
    var_H = H * M;
#endif
#if !defined(BLINN_PHONG) || defined(_PBR)
    var_V = V * M;
#endif
#if defined(_PBR)
    var_Normal = attr_Normal * M;
#endif

    var_Color = attr_Color * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
