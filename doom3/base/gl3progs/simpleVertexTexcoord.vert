#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec4 attr_TexCoord;
in highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;

out vec2 var_TexDiffuse;

void main(void)
{
    var_TexDiffuse = attr_TexCoord.xy;

    gl_Position = attr_Vertex * u_modelViewProjectionMatrix;
}
