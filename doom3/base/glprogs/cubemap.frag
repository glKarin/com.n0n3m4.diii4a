#version 100
//#pragma optimize(off)

precision mediump float;

varying vec3 var_TexCoord;
varying lowp vec4 var_Color;

uniform samplerCube u_fragmentCubeMap0;
uniform lowp vec4 u_glColor;

void main(void)
{
    gl_FragColor = textureCube(u_fragmentCubeMap0, var_TexCoord) * u_glColor * var_Color;
}
