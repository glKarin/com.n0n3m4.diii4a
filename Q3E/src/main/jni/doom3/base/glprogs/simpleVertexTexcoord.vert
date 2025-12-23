#version 100
//#pragma optimize(off)

precision mediump float;

attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;

varying vec2 var_TexDiffuse;

void main(void)
{
    var_TexDiffuse = attr_TexCoord.xy;

    gl_Position = attr_Vertex * u_modelViewProjectionMatrix;
}
