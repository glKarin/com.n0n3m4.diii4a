#version 300 es
//#pragma optimize(off)

precision mediump float;

in highp vec4 attr_Vertex;
in vec4 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform vec4 u_clipPlane;

out vec2 var_TexDiffuse;
out vec2 var_TexClip;

void main(void)
{
    var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;

    var_TexClip = vec2( dot( u_clipPlane, attr_Vertex), 0.5 );

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
