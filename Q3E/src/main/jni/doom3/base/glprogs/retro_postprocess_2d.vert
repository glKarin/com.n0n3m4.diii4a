#version 100
//#pragma optimize(off)

precision mediump float;

attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

varying vec2 var_TexCoord;

void main(void)
{
    gl_Position = attr_Vertex;
    gl_Position.y = -attr_Vertex.y;

    var_TexCoord = attr_TexCoord.xy;
}
