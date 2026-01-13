#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;

uniform lowp float u_colorModulate; // 0 or 1/255
uniform lowp float u_colorAdd; // 0 or 1
uniform vec4 u_texgenS;
uniform vec4 u_texgenT;
uniform vec4 u_texgenQ;
uniform mat4 u_textureMatrix;
uniform highp mat4 u_modelViewProjectionMatrix;

out vec4 var_TexCoord;
out lowp vec4 var_Color;

void main(void)
{
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

    vec4 texcoord0 = vec4(dot( u_texgenS, attr_Vertex ), dot( u_texgenT, attr_Vertex ), 0.0, dot( u_texgenQ, attr_Vertex )); 

    // multiply the texture matrix in
    var_TexCoord = vec4(dot( u_textureMatrix[0], texcoord0 ), dot( u_textureMatrix[1], texcoord0), texcoord0.z, texcoord0.w);

    // compute vertex modulation
    var_Color = attr_Color * u_colorModulate + u_colorAdd;
}
