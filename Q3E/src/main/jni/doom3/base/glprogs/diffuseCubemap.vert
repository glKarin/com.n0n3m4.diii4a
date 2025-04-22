#version 100
//#pragma optimize(off)

precision mediump float;

// In
attribute highp vec4 attr_Vertex;
attribute lowp vec4 attr_Color;
attribute vec3 attr_TexCoord;

// Uniforms
uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform lowp float u_colorAdd;
uniform lowp float u_colorModulate;

// Out
// gl_Position
varying vec3 var_TexCoord;
varying lowp vec4 var_Color;

void main(void)
{
    var_TexCoord = (u_textureMatrix * vec4(attr_TexCoord, 0.0)).xyz;

    var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
