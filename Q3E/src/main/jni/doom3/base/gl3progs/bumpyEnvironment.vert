#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in vec2 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelMatrix;
uniform lowp float u_colorModulate; // 0 or 1/255
uniform lowp float u_colorAdd; // 0 or 1
uniform vec4 u_viewOrigin;

out vec2 var_TexCoord;
out lowp vec4 var_Color;
out mat3 var_TangentToWorldMatrix;
out highp vec3 var_toEyeWorld; //karin: must highp in GLES

void main(void)
{
    // texture 0 takes the unodified texture coordinates
    var_TexCoord = attr_TexCoord;

    // texture 1 is the vector to the eye in global coordinates
    var_toEyeWorld = mat3(u_modelMatrix) * vec3(u_viewOrigin - attr_Vertex);

    // texture 2 3 4 gets the transformed tangent
    mat3 matTangentToLocal = mat3(
        clamp(attr_Tangent, vec3(-1.0), vec3(1.0)),
        clamp(attr_Bitangent, vec3(-1.0), vec3(1.0)),
        clamp(attr_Normal, vec3(-1.0), vec3(1.0))
    );
    var_TangentToWorldMatrix = mat3(u_modelMatrix) * matTangentToLocal;

    var_Color = attr_Color * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
