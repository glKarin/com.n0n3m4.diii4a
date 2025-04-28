#version 100
//#pragma optimize(off)

precision mediump float;

uniform highp mat4 u_modelViewProjectionMatrix;

uniform highp vec4 u_vertexParm0; // local[0]    scroll
uniform highp vec4 u_vertexParm1; // local[1]    deform magnitude (1.0 is reasonable, 2.0 is twice as wavy, 0.5 is half as wavy, etc)

attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

varying vec2 var_TexCoord0; // texCoord[1] is the model surface texture coords
// varying vec4 var_TexCoord1; // texCoord[2] is the copied deform magnitude
varying vec4 var_Color;

void main(void)
{
    // texture 1 takes the texture coordinates and adds a scroll
    var_TexCoord0 = attr_TexCoord.xy;
    // var_TexCoord1 = attr_TexCoord + u_vertrxParm0;

    // magnitude of scale
    var_Color = u_vertexParm1;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
