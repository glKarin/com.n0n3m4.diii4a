#version 100
//#pragma optimize(off)

precision mediump float;

attribute lowp vec4 attr_Color;
attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform lowp float u_colorModulate; // 0 or 1/255
uniform lowp float u_colorAdd; // 0 or 1
uniform vec4 u_viewOrigin;

varying vec3 var_TexCoord;
varying lowp vec4 var_Color;

void main(void)
{
  var_TexCoord = (u_textureMatrix * attr_TexCoord).xyz;

    var_Color = attr_Color * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
