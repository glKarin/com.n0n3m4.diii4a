#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute lowp vec4 attr_Color;
attribute vec3 attr_Tangent;
attribute vec3 attr_Bitangent;
attribute vec3 attr_Normal;
attribute vec2 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp mat4 u_modelMatrix;
uniform lowp vec4 u_colorAdd;
uniform lowp vec4 u_colorModulate;
uniform vec4 u_viewOrigin;

varying vec2 var_TexCoord;
varying lowp vec4 var_Color;
varying mat3 var_TangentToWorldMatrix;
varying highp vec3 var_toEyeWorld; //karin: must highp in GLES

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

    var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
