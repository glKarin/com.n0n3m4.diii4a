#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;
in highp vec4 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;

out highp vec4 var_TexCoord;

// # texture 0 takes the texture coordinates unmodified

void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

    var_TexCoord = attr_TexCoord;
}
