#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute vec4 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform vec4 u_clipPlane;

varying vec2 var_TexDiffuse;
varying vec2 var_TexClip;

void main(void)
{
    var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;

    var_TexClip = vec2( dot( u_clipPlane, attr_Vertex), 0.5 );

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
