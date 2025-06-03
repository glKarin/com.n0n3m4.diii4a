#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec4 attr_TexCoord;
in highp vec4 attr_Vertex;

out vec2 var_TexCoord;

void main(void)
{
    gl_Position = attr_Vertex;
    gl_Position.y = -attr_Vertex.y;

    var_TexCoord = attr_TexCoord.xy;
}
