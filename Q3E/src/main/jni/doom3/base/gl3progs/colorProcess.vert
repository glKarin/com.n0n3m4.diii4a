#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;
in highp vec4 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform highp vec4 u_vertexParm0; // fraction
uniform highp vec4 u_vertexParm1; // target hue

out highp vec4 var_TexCoord;
out lowp vec4 var_Color;

// # parameter 0 is the fraction from the current hue to the target hue to map
// # parameter 1.rgb is the target hue
// # texture 0 is _currentRender

// # nothing to do but pass the parameters along
void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

    var_Color = u_vertexParm1;
    var_TexCoord.x = attr_TexCoord.x;
    var_TexCoord.y = 1.0f - attr_TexCoord.y;
    var_TexCoord.z = u_vertexParm0.x;
}