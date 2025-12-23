#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute highp vec4 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;

varying highp vec4 var_TexCoord;

// # texture 0 takes the texture coordinates unmodified

void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

    var_TexCoord = attr_TexCoord;
}
