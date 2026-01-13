#version 300 es

//#pragma optimize(off)

//#define _BFG
//#define BLINN_PHONG
//#define HALF_LAMBERT

precision highp float;

in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;

uniform lowp float u_colorModulate; // 0 or 1/255
uniform lowp float u_colorAdd; // 0 or 1

uniform highp mat4 u_modelViewProjectionMatrix;
uniform vec4 u_viewOrigin;

uniform vec4 u_bumpMatrixS;
uniform vec4 u_bumpMatrixT;
uniform vec4 u_diffuseMatrixS;
uniform vec4 u_diffuseMatrixT;
uniform vec4 u_specularMatrixS;
uniform vec4 u_specularMatrixT;

out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out lowp vec4 var_Color;
out vec3 var_L;
out vec3 var_H;
#if !defined(_BFG) && !defined(BLINN_PHONG)
out vec3 var_V;
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

    vec3 L = normalize(vec3(0.0, 0.5, 1.0));
    vec3 V = u_viewOrigin.xyz - attr_Vertex.xyz;
    vec3 H = normalize(L) + normalize(V);

    var_L = L * M;
    var_H = H * M;
#if !defined(_BFG) && !defined(BLINN_PHONG)
    var_V = V * M;
#endif

    var_Color = attr_Color * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
